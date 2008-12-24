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

#include "mobblerappui.h"
#include "mobbleraudiothread.h"
#include "mobbleraudiocontrol.h"
#include "mobblerincomingcallmonitor.h"
#include "mobblerlastfmconnection.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include <mobbler.rsg>

const TInt KBufferSizeInPackets(32);
const TInt KTimerDuration(250000); // 1/4 second
const TInt KDefaultVolume(5);
const TInt KEqualizerOff(-1);

CMobblerRadioPlayer* CMobblerRadioPlayer::NewL(CMobblerLastFMConnection& aSubmitter, TTimeIntervalSeconds aBufferSize)
	{
	CMobblerRadioPlayer* self = new(ELeave) CMobblerRadioPlayer(aSubmitter, aBufferSize);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerRadioPlayer::CMobblerRadioPlayer(CMobblerLastFMConnection& aLastFMConnection, TTimeIntervalSeconds aBufferSize)
	:CActive(EPriorityStandard), iBufferSize(aBufferSize), iLastFMConnection(aLastFMConnection)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerRadioPlayer::ConstructL()
	{
	iTimer = CPeriodic::NewL(CActive::EPriorityLow);
	User::LeaveIfError(iMutex.CreateLocal());
	TCallBack callBack(CheckForEndOfTrack, this);
	iTimer->Start(KTimerDuration, KTimerDuration, callBack);
	iAudio = CMobblerAudioControl::NewL(this);
	iAudio->SetVolume(KDefaultVolume);
	// Set Initial Equalizer Setting here
	iAudio->SetEqualizerIndex(KEqualizerOff);
	iAudio->Start();
	}

CMobblerRadioPlayer::~CMobblerRadioPlayer()
	{
	delete iTimer;
	Cancel();
	iAudio->Stop();
	iAudio->TransferWrittenBuffer(iTrashCan);
	EmptyTrashCan();
	delete iPlaylist;
	delete iAudio;
	iMutex.Close();
	
	delete iIncomingCallMonitor;
	}

void CMobblerRadioPlayer::RunL()
	{
	Stop();
	}

TInt CMobblerRadioPlayer::CheckForEndOfTrack(TAny* aRef) // From timer every 0.25 seconds
	{
	CMobblerRadioPlayer* RadioPlayer = (CMobblerRadioPlayer*)aRef;
	RadioPlayer->iMutex.Wait();
	TBool gotoNextTrack = RadioPlayer->iGotoNextTrack;
	RadioPlayer->iMutex.Signal();
	if (gotoNextTrack && RadioPlayer->iPlaying)
		{
		RadioPlayer->NextTrackL();
		RadioPlayer->iMutex.Wait();
		RadioPlayer->iGotoNextTrack = EFalse;
		RadioPlayer->iMutex.Signal();
		}
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	return KErrNone;
	}

void CMobblerRadioPlayer::DoCancel()
	{
	// There is nothing to cancel as we only ever use User::RequestComplete
	}

TInt CMobblerRadioPlayer::StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	Stop();
	return iLastFMConnection.RadioStartL(aRadioStation, aRadioText);
	}

void CMobblerRadioPlayer::SetPlaylistL(CMobblerRadioPlaylist* aPlaylist)
	{
	iCurrentTrack = 0;
	
	if (aPlaylist->Count() > iCurrentTrack)
		{
		iBufferOffset = 0;
		iTrackDownloading = ETrue;
		iLastFMConnection.RequestMp3L((*aPlaylist)[iCurrentTrack]);
		}
	
	// take ownership of the playlist
	delete iPlaylist;
	iPlaylist = aPlaylist;
	}

void CMobblerRadioPlayer::TrackDownloadCompleteL()
	{
	iTrackDownloading = EFalse;
	}

void CMobblerRadioPlayer::NextTrackL()
	{
	DoStop();
	
	if (iPlaylist)
		{
		if (iPlaylist->Count() > iCurrentTrack + 1)
			{
			++iCurrentTrack;
			iBufferOffset = 0;
			iTrackDownloading = ETrue;
			iLastFMConnection.RequestMp3L((*iPlaylist)[iCurrentTrack]);
			}
		else
			{
			//  There is a playlist so try and fetch another
			delete iPlaylist;
			iPlaylist = NULL;
			iLastFMConnection.RequestPlaylistL();
			}
		}
	}

void CMobblerRadioPlayer::VolumeUp()
	{
	TInt volume = iAudio->Volume();
	TInt maxVolume = iAudio->MaxVolume();
	iAudio->SetVolume(Min(volume + (maxVolume / 10), maxVolume));
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::VolumeDown()
	{
	TInt volume = iAudio->Volume();
	TInt maxVolume = iAudio->MaxVolume();
	iAudio->SetVolume(Max(volume - (maxVolume / 10), 0));
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

TInt CMobblerRadioPlayer::Volume() const
	{
	return iAudio->Volume();
	}

TInt CMobblerRadioPlayer::MaxVolume() const
	{
	return iAudio->MaxVolume();
	}

const CMobblerString& CMobblerRadioPlayer::Station() const
	{
	return iPlaylist->Name();
	}

void CMobblerRadioPlayer::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	iBufferSize = aBufferSize;
	}

void CMobblerRadioPlayer::Stop()
	{
	DoStop();
	}

void CMobblerRadioPlayer::DoStop()
	{
	// Try to submit the last played track
	SubmitCurrentTrackL();
	iPlaying = EFalse;
	iOpen = EFalse;
	// Stop downloading the mp3
	iLastFMConnection.RadioStop();
	// Close and reopen the audio stream
	iAudio->Restart();
	iAudio->TransferWrittenBuffer(iTrashCan);
	EmptyTrashCan();
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

CMobblerTrack* CMobblerRadioPlayer::CurrentTrack()
	{
	if ((iTrackDownloading || iPlaying) && (iPlaylist && (iPlaylist->Count() > iCurrentTrack)) )
		{
		return (*iPlaylist)[iCurrentTrack];
		}
	
	return NULL;
	}

void CMobblerRadioPlayer::WriteL(const TDesC8& aData, TInt aTotalDataSize)
	{
	if (iPlaylist && iPlaylist->Count() > iCurrentTrack && iCurrentTrack >= 0)
		{
		// There is a playlist and the current track indexes a track in it
		CMobblerTrack* track = (*iPlaylist)[iCurrentTrack];
		
		track->SetDataSize(aTotalDataSize);
		track->BufferAdded(aData.Length());
		
		TInt bufferTotal = track->DataSize();
		TInt bufferPosition = Min(track->Buffered(), bufferTotal);
		TInt playbackTotal = track->TrackLength().Int();
		TInt playbackPosition = Min(track->PlaybackPosition().Int(), playbackTotal);
		if (bufferTotal != 0 && playbackTotal != 0)
			{
			// We multiple by 1000 and then later divide by 1000 to retain accuracy
			// in integer calculations. Otherwise roundoff error is intolerable
			TInt64 bufferPercent = 1000 * (TInt64)bufferPosition / (TInt64)bufferTotal;
			TInt64 playedPercent = 1000 * (TInt64)playbackPosition / (TInt64)playbackTotal;
			TInt secsAhead = (TInt)(bufferPercent - playedPercent) * playbackTotal / 1000;
			if (secsAhead > 60)
				{
				TRequestStatus* status = &iStatus;
				User::RequestComplete(status, KErrNone);
				SetActive();
				}
			}
		
		EmptyTrashCan();
		iAudio->AddToBuffer(aData, iPlaying);
		
		TBool bufferedEnough(EFalse);
		
		if (track->DataSize() != 1)
			{
			TInt byteRate = track->DataSize() / (*iPlaylist)[iCurrentTrack]->TrackLength().Int();
			TInt buffered = track->Buffered() - iBufferOffset;
			
			if (byteRate != 0)
				{
				TInt secondsBuffered = buffered / byteRate;
				
				// either buffered amount of time or the track has finished downloading
				bufferedEnough = secondsBuffered >= iBufferSize.Int() || (*iPlaylist)[iCurrentTrack]->Buffered() == (*iPlaylist)[iCurrentTrack]->DataSize();
				}
			}
		else
			{
			bufferedEnough = iAudio->PreBufferSize() >= KBufferSizeInPackets;
			}
		
		if (iOpen && !iPlaying && bufferedEnough)
			{
			// start playing the track
			iPlaying = ETrue;
			iAudio->BeginPlay();
			
			CMobblerTrack* track = (*iPlaylist)[iCurrentTrack];
			
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
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::MaoscOpenComplete(TInt aError)
	{
	if (aError == KErrNone) iOpen = ETrue;
	}

void CMobblerRadioPlayer::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
	{
	iAudio->TransferWrittenBuffer(iTrashCan);
	if (CurrentTrack())
		{
		CurrentTrack()->SetPlaybackPosition(iAudio->PlaybackPosition());
		}
	TInt Count = iAudio->PreBufferSize();
	if (Count == 0)
		{
		if (!iTrackDownloading)
			{
			iMutex.Wait();
			iGotoNextTrack = ETrue;
			iMutex.Signal();
			}
		else
			{
			iBufferOffset = (*iPlaylist)[iCurrentTrack]->Buffered();
			iPlaying = EFalse;
			}
		}
	}

void CMobblerRadioPlayer::SubmitCurrentTrackL()
	{
	if (iTrackDownloading || iPlaying)
		{
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerRadioPlayer::MaoscPlayComplete(TInt aError)
	{
	// on S60 mobiles this never get called when the track ends, or with KErrUnderflow
	
	if (aError == KErrCancel)
		{
		// The track has been asked to stop
		SubmitCurrentTrackL();
		iPlaying = EFalse;
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
			Stop();
			break;
		case EPSTelephonyCallStateUninitialized:
		case EPSTelephonyCallStateNone:
		case EPSTelephonyCallStateDisconnecting:
		case EPSTelephonyCallStateHold:
		default:
			// The normal S60 behaviour seems to be to pause and then restart
			// music when the call finishes, but we have some problems with this
			break;
		}
	}

void CMobblerRadioPlayer::SetEqualizer(TInt aIndex)
	{
	iEqualizerIndex = aIndex;
	iAudio->SetEqualizerIndex(aIndex);
	}
	
void CMobblerRadioPlayer::EmptyTrashCan()
	{
	iMutex.Wait();
	TInt Count = iTrashCan.Count();
	for (TInt x = 0; x < Count; ++x)
		{
		HBufC8* data = iTrashCan[0];
		iTrashCan.Remove(0);
		delete data;
		}
	iMutex.Signal();
	}
TBool CMobblerRadioPlayer::HasPlaylist() const
	{
	if (iPlaylist)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

// End of file
