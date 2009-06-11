/*
mobblerradioplayer.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008  Michael Coffey

http://code.google.com/p/mobbler

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <aknnotewrappers.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobbleraudiocontrol.h"
#include "mobbleraudiothread.h"
#include "mobblerincomingcallmonitor.h"
#include "mobblerlastfmconnection.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"

const TInt KDefaultMaxVolume(10);

// The radio should timeout and delete its playlists after 5 minutes
// so that we do not get tracks that can't be downloaded when restarting
const TTimeIntervalMicroSeconds32 KRadioTimeout(5 * 60 * 1000000);

CMobblerRadioPlayer* CMobblerRadioPlayer::NewL(CMobblerLastFMConnection& aSubmitter, 
												TTimeIntervalSeconds aPreBufferSize,
												TInt aEqualizerIndex, 
												TInt aVolume,
												TInt aBitRate)
	{
	CMobblerRadioPlayer* self = new(ELeave) CMobblerRadioPlayer(aSubmitter, aPreBufferSize, aEqualizerIndex, aVolume, aBitRate);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerRadioPlayer::CMobblerRadioPlayer(CMobblerLastFMConnection& aLastFMConnection,
											TTimeIntervalSeconds aPreBufferSize,
											TInt aEqualizerIndex,
											TInt aVolume,
											TInt aBitRate)
	:CActive(CActive::EPriorityStandard), 
	iLastFMConnection(aLastFMConnection), 
	iPreBufferSize(aPreBufferSize), 
	iVolume(aVolume), 
	iMaxVolume(KDefaultMaxVolume), 
	iEqualizerIndex(aEqualizerIndex),
	iBitRate(aBitRate)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerRadioPlayer::ConstructL()
	{
	iIncomingCallMonitor = CMobblerIncomingCallMonitor::NewL(*this);
	iStation = CMobblerString::NewL(KNullDesC);
	User::LeaveIfError(iTimer.CreateLocal());
	}

CMobblerRadioPlayer::~CMobblerRadioPlayer()
	{
	Cancel();
	iTimer.Close();
	
	delete iCurrentPlaylist;
	delete iNextPlaylist;
	delete iCurrentAudioControl;
	delete iNextAudioControl;
	delete iStation;
	
	delete iIncomingCallMonitor;
	
	iObservers.Close();
	}

void CMobblerRadioPlayer::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		if (!CurrentTrack())
			{
			// The radio has not been playing for 5 minutes - delete the playlists
			delete iCurrentPlaylist;
			iCurrentPlaylist = NULL;
			delete iNextPlaylist;
			iNextPlaylist = NULL;
			}
		}
	}

void CMobblerRadioPlayer::DoCancel()
	{
	iTimer.Cancel();
	}

void CMobblerRadioPlayer::AddObserverL(MMobblerRadioStateChangeObserver* aObserver)
	{
	iObservers.InsertInAddressOrder(aObserver);
	}

void CMobblerRadioPlayer::RemoveObserver(MMobblerRadioStateChangeObserver* aObserver)
	{
	TInt position(iObservers.FindInAddressOrder(aObserver));
	
	if (position != KErrNotFound)
		{
		iObservers.Remove(position);
		}
	}

void CMobblerRadioPlayer::DoChangeStateL(TState aState)
	{
	iState = aState;
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleRadioStateChangedL();
		}
	
	if (iState == EIdle)
		{
		// make sure the timeout timer has started  
		if (!IsActive())
			{
			iTimer.After(iStatus, KRadioTimeout);
			SetActive();
			}
		}
	else
		{
		// cancel the timeout timer
		Cancel();
		}
	}

void CMobblerRadioPlayer::DoChangeTransactionStateL(TTransactionState aTransactionState)
	{
	iTransactionState = aTransactionState;
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i(0); i < KObserverCount; ++i)
		{
		iObservers[i]->HandleRadioStateChangedL();
		}
	}

void CMobblerRadioPlayer::HandleAudioPositionChangeL()
	{
	// if we are finished downloading
	// and only have the prebuffer amount of time left of the track
	// then start downloading the next track
	
	if (!iNextAudioControl &&
			iCurrentAudioControl &&
			iCurrentAudioControl->DownloadComplete() &&
			iCurrentPlaylist &&
			(( (*iCurrentPlaylist)[iCurrentTrackIndex]->TrackLength().Int() - (*iCurrentPlaylist)[iCurrentTrackIndex]->PlaybackPosition().Int() ) <= iCurrentAudioControl->PreBufferSize().Int() ))
		{
		// There is another track in the playlist.
		// We have not created the next track yet.
		// The download on the current track is complete
		// and there is only the length of the pre-buffer to go on the current track 
		// so start downloading the next track now.
		
		if (iCurrentTrackIndex + 1 < iCurrentPlaylist->Count())
			{
			// There is more in the playlist so start fetching the next track
			iNextAudioControl = CMobblerAudioControl::NewL(*this,  *(*iCurrentPlaylist)[iCurrentTrackIndex + 1], iPreBufferSize, iVolume, iEqualizerIndex, iBitRate);
			iLastFMConnection.RequestMp3L(*iNextAudioControl, (*iCurrentPlaylist)[iCurrentTrackIndex + 1]->Mp3Location());
			DoChangeStateL(EPlaying);
			}
		else if (iNextPlaylist)
			{
			// there is another playlist so fetch the first track for that
			iNextAudioControl = CMobblerAudioControl::NewL(*this,  *(*iNextPlaylist)[0], iPreBufferSize, iVolume, iEqualizerIndex, iBitRate);
			iLastFMConnection.RequestMp3L(*iNextAudioControl, (*iNextPlaylist)[0]->Mp3Location());
			DoChangeStateL(EPlaying);
			}
		}
	
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawDeferred();
	iLastFMConnection.SaveCurrentTrackL();
	}

void CMobblerRadioPlayer::HandleAudioFinishedL(CMobblerAudioControl* aAudioControl, TInt aError)
	{
	if (aError == KErrCancel)
		{
		// The mp3 was cancelled so change to idle 
		DoChangeStateL(EIdle);
		}
	else
		{
		if (aAudioControl == iCurrentAudioControl)
			{
			// The current track has finished so try to start the new one 
			SkipTrackL();
			}
		else if (aAudioControl == iNextAudioControl)
			{
			// The next audio track failed before this one so
			// delete it and remove it from the playlist
			delete iNextAudioControl;
			iNextAudioControl = NULL;
			
			if (iCurrentPlaylist->Count() > iCurrentTrackIndex + 1)
				{
				iCurrentPlaylist->RemoveAndReleaseTrack(iCurrentTrackIndex + 1);
				}
			else if (iNextPlaylist && iNextPlaylist->Count() > 0)
				{
				iNextPlaylist->RemoveAndReleaseTrack(0);
				}
			}
		}
	}

CMobblerRadioPlayer::TState CMobblerRadioPlayer::State() const
	{
	return iState;
	}

CMobblerRadioPlayer::TTransactionState CMobblerRadioPlayer::TransactionState() const
	{
	return iTransactionState;
	}

void CMobblerRadioPlayer::StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const CMobblerString* aRadioText)
	{
	delete iStation;
	iStation = NULL;
	
	TBuf<255> station;
	TPtrC text(KNullDesC);
	
	if (!aRadioText)
		{
		// use the user's username
		text.Set(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->SettingView().Username());
		}
	else
		{
		text.Set(aRadioText->String());
		}

	switch (aRadioStation)
		{
		case CMobblerLastFMConnection::EPersonal:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_PERSONAL_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::ERecommendations:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_RECOMMENDATIONS_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::ENeighbourhood:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NEIGHBOURHOOD_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::ELovedTracks:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_LOVED_TRACKS_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::EPlaylist:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::EArtist:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ARTIST_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::ETag:
			station.Format(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_TAG_FORMAT), &text);
			break;
		case CMobblerLastFMConnection::EUnknown:
		default:
			break;
		}

	iStation = CMobblerString::NewL(station);
	
	// Stop the radio and also make sure that
	// we get rid of any playlists hanging around
	DoStop(ETrue);
	
	delete iCurrentPlaylist;
	iCurrentPlaylist = NULL;
	delete iNextPlaylist;
	iNextPlaylist = NULL;
	
	// now ask for the radio to start again
	if (aRadioText)
		{
		HBufC8* urlEncoded(MobblerUtility::URLEncodeLC(aRadioText->String()));
		iLastFMConnection.SelectStationL(this, aRadioStation, *urlEncoded);
		CleanupStack::PopAndDestroy(urlEncoded);
		}
	else
		{
		iLastFMConnection.SelectStationL(this, aRadioStation, KNullDesC8);
		}
	
	DoChangeTransactionStateL(ESelectingStation);
	}

void CMobblerRadioPlayer::DataL(const TDesC8& aData, CMobblerLastFMConnection::TError aError)
	{
	switch (iTransactionState)
		{
		case ESelectingStation:
			{
			if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				delete iStation;
				iStation = CMobblerParser::ParseRadioTuneL(aData);
				
				DoChangeTransactionStateL(EFetchingPlaylist);
				iLastFMConnection.RequestPlaylistL(this);
				}
			else
				{
				DoChangeTransactionStateL(ENone);
				
				CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
				note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION));
				}
			}
			break;
		case EFetchingPlaylist:
			{
			if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				DoChangeTransactionStateL(ENone);
		
				CMobblerRadioPlaylist* playlist(NULL);
				CMobblerLastFMError* error(CMobblerParser::ParseRadioPlaylistL(aData, playlist));
				
				if (!error)
					{
					if (iCurrentPlaylist)
						{
						// we are still using the current playlist
						// so this must be the next one
						
						delete iNextPlaylist;
						iNextPlaylist = playlist;
						}
					else
						{
						iCurrentPlaylist = playlist;
						
						iCurrentTrackIndex = -1;
						SkipTrackL();
						}
					}
				else
					{
					CleanupStack::PushL(error);
					DoChangeTransactionStateL(ENone);
					
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(error->Text());
					CleanupStack::PopAndDestroy(error);
					}
				}
			else
				{
				DoChangeTransactionStateL(ENone);
				
				if (aData.Length() != 0)
					{
					// Display an error if we were given some text because this is a false response from Last.fm
					
					CMobblerString* errorText(CMobblerString::NewL(aData));
					CleanupStack::PushL(errorText);
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(errorText->String());
					CleanupStack::PopAndDestroy(errorText);
					}
				else
					{
					DoChangeTransactionStateL(ENone);
					
					CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
					note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION));
					}
				}
			}
			break;
		default:
			DoChangeTransactionStateL(ENone);
			break;
		}
	}

void CMobblerRadioPlayer::SkipTrackL()
	{	
	DoStop(EFalse);
	
	if (iCurrentPlaylist)
		{
		if (iCurrentPlaylist->Count() > iCurrentTrackIndex + 1)
			{
			++iCurrentTrackIndex;
			
			if (iCurrentPlaylist->Count() - 1 == iCurrentTrackIndex)
				{
				// This is the last track in the playlist so
				// fetch the next playlist
				
				DoChangeTransactionStateL(EFetchingPlaylist);
				iLastFMConnection.RequestPlaylistL(this);
				}
			
			if (iCurrentPlaylist->Count() > iCurrentTrackIndex)
				{
				// We are indexing a track in this playlist so start it
				
				delete iCurrentAudioControl;
				iCurrentAudioControl = NULL;
				
				if (iNextAudioControl)
					{
					// We have already created the next audio control so use that
					iCurrentAudioControl = iNextAudioControl;
					iNextAudioControl = NULL;
					}
				else
					{
					// Create the next audio control and request the mp3
					iCurrentAudioControl = CMobblerAudioControl::NewL(*this, *(*iCurrentPlaylist)[iCurrentTrackIndex], iPreBufferSize, iVolume, iEqualizerIndex, iBitRate);
					iLastFMConnection.RequestMp3L(*iCurrentAudioControl, (*iCurrentPlaylist)[iCurrentTrackIndex]->Mp3Location());
					DoChangeStateL(EPlaying);
					}
				
				// Tell the audio thread that is should start writing data to the output stream
				iCurrentAudioControl->SetCurrent();
				
				// We have started playing the track so tell Last.fm
				CMobblerTrack* track((*iCurrentPlaylist)[iCurrentTrackIndex]);
				
				if (track->StartTimeUTC() == Time::NullTTime())
					{
					// we haven't set the start time for this track yet
					// so this must be the first time we are writing data
					// to the output stream.  Set the start time now.
					TTime now;
					now.UniversalTime();
					track->SetStartTimeUTC(now);
					}
				
				iLastFMConnection.TrackStartedL(track);
				}
			}
		else
			{
			// There is a playlist and the next one would have been
			// fetched when the last track in this playlist was started
			
			delete iCurrentPlaylist;
			iCurrentPlaylist = NULL;
			
			if (iNextPlaylist)
				{
				// We have already fetched the next plalist so start using it.
				// If we don't have the next playlist then it is still downloading.
				
				iCurrentPlaylist = iNextPlaylist;
				iNextPlaylist = NULL;
				
				iCurrentTrackIndex = -1;
				SkipTrackL();
				}
			}
		}
	}

void CMobblerRadioPlayer::VolumeUp()
	{
	TInt volume(Volume());
	TInt maxVolume(MaxVolume());
	iVolume = Min(volume + (maxVolume / 10), maxVolume);

	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetVolume(iVolume);
		}
	if (iNextAudioControl)
		{
		iNextAudioControl->SetVolume(iVolume);
		}

	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::VolumeDown()
	{
	TInt volume(Volume());
	TInt maxVolume(MaxVolume());
	iVolume = Max(volume - (maxVolume / 10), 0);
		
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetVolume(iVolume);
		}
	if (iNextAudioControl)
		{
		iNextAudioControl->SetVolume(iVolume);
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

TInt CMobblerRadioPlayer::Volume() const
	{
	TInt volume(iVolume);
	
	if (iCurrentAudioControl)
		{
		volume = iCurrentAudioControl->Volume();
		}
	
	return volume;
	}

TInt CMobblerRadioPlayer::MaxVolume() const
	{
	TInt maxVolume(iMaxVolume);
	
	if (iCurrentAudioControl)
		{
		if (iMaxVolume != iCurrentAudioControl->MaxVolume())
			{
			// The max audio volume has changed
			// so correct the volume for this
			iCurrentAudioControl->SetVolume((iCurrentAudioControl->Volume() * iCurrentAudioControl->MaxVolume()) / iMaxVolume);
			}
		
		maxVolume = iCurrentAudioControl->MaxVolume();
		}
	
	return maxVolume;
	}

TInt CMobblerRadioPlayer::EqualizerIndex() const
	{
	return iEqualizerIndex;
	}

const CMobblerString& CMobblerRadioPlayer::Station() const
	{
	return *iStation;
	}

void CMobblerRadioPlayer::SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize)
	{
	iPreBufferSize = aPreBufferSize;
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetPreBufferSize(aPreBufferSize);
		}
	}

void CMobblerRadioPlayer::SetBitRateL(TInt aBitRate)
	{
	if (aBitRate != iBitRate)
		{
		// The sample rate has changed so we need to
		// fetch a new playlist for the next song.
		iBitRate = aBitRate;
		
		// Remove the rest of the songs from this playlist
		for (TInt i(iCurrentPlaylist->Count() - 1) ; i > iCurrentTrackIndex ; --i)
			{
			iCurrentPlaylist->RemoveAndReleaseTrack(i);
			}
		
		// Remove the next playlist and fetch a new one.
		// The next track will have the correct new sample rate.
		delete iNextPlaylist;
		iNextPlaylist = NULL;
		
		DoChangeTransactionStateL(EFetchingPlaylist);
		iLastFMConnection.RequestPlaylistL(this);
		}
	}

void CMobblerRadioPlayer::Stop()
	{
	DoStop(ETrue);
	}

void CMobblerRadioPlayer::DoStop(TBool aDeleteNextTrack)
	{
	// Try to submit the last played track
	SubmitCurrentTrackL();
	
	if (aDeleteNextTrack)
		{
		// we want to also delete the next track if it has started to download
		
		// stop all radio downloads
		iLastFMConnection.RadioStop();
		
		if (iNextAudioControl)
			{
			// if there was a next track then increment the current track
			// because we can't download the same track twice
			++iCurrentTrackIndex;
			}
		
		delete iNextAudioControl;
		iNextAudioControl = NULL;
		}
	else
		{
		if (!iNextAudioControl)
			{
			// we don't want to delete the next track, but
			// it hasn't started downloading yet so stop
			// the current one
			iLastFMConnection.RadioStop();
			}
		}
	
	delete iCurrentAudioControl;
	iCurrentAudioControl = NULL;
	
	DoChangeStateL(EIdle);
//	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

CMobblerTrack* CMobblerRadioPlayer::CurrentTrack()
	{
	if (iCurrentAudioControl && (!iCurrentAudioControl->DownloadComplete() ||
			iCurrentAudioControl->Playing()) && (iCurrentPlaylist && (iCurrentPlaylist->Count() > iCurrentTrackIndex)) )
		{
		return (*iCurrentPlaylist)[iCurrentTrackIndex];
		}
	
	return NULL;
	}

CMobblerTrack* CMobblerRadioPlayer::NextTrack()
	{
	if (iCurrentPlaylist && (iCurrentPlaylist->Count() > iCurrentTrackIndex + 1))
		{
		return (*iCurrentPlaylist)[iCurrentTrackIndex + 1];
		}
	else if (iCurrentPlaylist && (iCurrentPlaylist->Count() == iCurrentTrackIndex + 1) && iNextPlaylist && (iNextPlaylist->Count() > 0))
		{
		// The next track is the first song in the next playlist so return that
		return (*iNextPlaylist)[0];
		}
	
	return NULL;
	}

void CMobblerRadioPlayer::SubmitCurrentTrackL()
	{
	if (iCurrentAudioControl && (!iCurrentAudioControl->DownloadComplete() || iCurrentAudioControl->Playing()))
		{
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerRadioPlayer::HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState)
	{
	switch (aPSTelephonyCallState)
		{
		case EPSTelephonyCallStateAlerting:
		case EPSTelephonyCallStateRinging:
		case EPSTelephonyCallStateDialling:
		case EPSTelephonyCallStateAnswering:
		case EPSTelephonyCallStateConnected:
			// There was an incoming call so stop playing the radio
			if (iState == EPlaying)
				{
				DoStop(ETrue);
				iRestartRadioOnCallDisconnect = ETrue;
				}
			break;
		case EPSTelephonyCallStateDisconnecting:
			if (iRestartRadioOnCallDisconnect)
				{
				iRestartRadioOnCallDisconnect = EFalse;
				SkipTrackL();
				}
			break;
		case EPSTelephonyCallStateUninitialized:
		case EPSTelephonyCallStateNone:
		case EPSTelephonyCallStateHold:
		default:
			// do nothing
			break;
		}
	}

void CMobblerRadioPlayer::SetEqualizer(TInt aIndex)
	{
	iEqualizerIndex = aIndex;
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetEqualizerIndex(aIndex);
		}
	if (iNextAudioControl)
		{
		iNextAudioControl->SetEqualizerIndex(aIndex);
		}
	}

TBool CMobblerRadioPlayer::HasPlaylist() const
	{
	if (iCurrentPlaylist)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

// End of file
