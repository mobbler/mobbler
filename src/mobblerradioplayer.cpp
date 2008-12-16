/*
mobblerradioplayer.cpp

mobbler, a last.fm mobile scrobbler for Symbian smartphones.
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

#include <badesca.h>
#include <eikprogi.h>

#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerutility.h"
#include "mobblerincomingcallmonitor.h"
#include "mobblerlastfmconnection.h"
#include "mobblertrack.h"
#include "mobblerappui.h"
#include <mobbler.rsg>

const TInt KBufferSizeInPackets(32);

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
	iMdaAudioOutputStream = CMdaAudioOutputStream::NewL(*this, EMdaPriorityMax, EMdaPriorityPreferenceTimeAndQuality);
	iMdaAudioOutputStream->Open(&iMdaAudioDataSettings);
	
	iIncomingCallMonitor = CMobblerIncomingCallMonitor::NewL(*this);
	}

CMobblerRadioPlayer::~CMobblerRadioPlayer()
	{
	Cancel();
	
	iPreBuffer.ResetAndDestroy();
	iWrittenBuffer.ResetAndDestroy();
	delete iMdaAudioOutputStream;
	delete iPlaylist;
	delete iIncomingCallMonitor;
	}

void CMobblerRadioPlayer::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		if (iPreBuffer.Count() > 0)
			{
			// Write the next part of the pre-buffer
			// and transfer it to the written buffer
			iMdaAudioOutputStream->WriteL(*iPreBuffer[0]);
			iWrittenBuffer.AppendL(iPreBuffer[0]);
			iPreBuffer.Remove(0);
			}
		
		if (iPreBuffer.Count() > 0)
			{
			// There is more data in iPreBuffer so schedule another callback
			TRequestStatus* status = &iStatus;
			User::RequestComplete(status, KErrNone);
			SetActive();
			}
		}
	}

void CMobblerRadioPlayer::DoCancel()
	{
	// There is nothing to cancel as we only ever use User::RequestComplete
	}

TInt CMobblerRadioPlayer::StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	Stop();
	
#ifdef __WINS__
	iMdaAudioDataSettings.iVolume = 0;
	iMdaAudioOutputStream->SetVolume(iMdaAudioDataSettings.iVolume);
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
#endif
	
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
	iMdaAudioDataSettings.iVolume = Min(iMdaAudioDataSettings.iVolume + (iMdaAudioOutputStream->MaxVolume() / 10), iMdaAudioOutputStream->MaxVolume());
	iMdaAudioOutputStream->SetVolume(iMdaAudioDataSettings.iVolume);
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::VolumeDown()
	{
	iMdaAudioDataSettings.iVolume = Max(iMdaAudioDataSettings.iVolume - (iMdaAudioOutputStream->MaxVolume() / 10), 0);
	iMdaAudioOutputStream->SetVolume(iMdaAudioDataSettings.iVolume);
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

const CMobblerString& CMobblerRadioPlayer::Station() const
	{
	return iPlaylist->Name();
	}

void CMobblerRadioPlayer::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	iBufferSize = aBufferSize;
	}

TInt CMobblerRadioPlayer::Volume() const
	{
	return iMdaAudioDataSettings.iVolume;
	}

TInt CMobblerRadioPlayer::MaxVolume() const
	{
	return iMdaAudioOutputStream->MaxVolume();
	}

void CMobblerRadioPlayer::Stop()
	{
	DoStop();
	}

void CMobblerRadioPlayer::DoStop()
	{
	// Try to submit the last played track
	SubmitCurrentTrackL();
	
	// Stop the audio output stream
	iMdaAudioOutputStream->Stop();
	iPreBuffer.ResetAndDestroy();
	iWrittenBuffer.ResetAndDestroy();
	iPlaying = EFalse;
	iOpen = EFalse;
	
	// Delete and create a new audio output stream
	delete iMdaAudioOutputStream;
	iMdaAudioOutputStream = NULL;
	iMdaAudioOutputStream = CMdaAudioOutputStream::NewL(*this, EMdaPriorityMax, EMdaPriorityPreferenceTimeAndQuality);
	iMdaAudioOutputStream->Open(&iMdaAudioDataSettings);
	
	// Stop downloading the mp3
	iLastFMConnection.RadioStop();
	
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
		
		(*iPlaylist)[iCurrentTrack]->SetDataSize(aTotalDataSize);
		(*iPlaylist)[iCurrentTrack]->BufferAdded(aData.Length());
		
		iPreBuffer.AppendL(aData.AllocLC());
		CleanupStack::Pop(); // aData.AllocLC()
		
		TBool bufferedEnough(EFalse);
		
		if ((*iPlaylist)[iCurrentTrack]->DataSize() != 1)
			{
			TInt byteRate = (*iPlaylist)[iCurrentTrack]->DataSize() / (*iPlaylist)[iCurrentTrack]->TrackLength().Int();
			TInt buffered = (*iPlaylist)[iCurrentTrack]->Buffered() - iBufferOffset;
			
			if (byteRate != 0)
				{
				TInt secondsBuffered = buffered / byteRate;
				
				// either buffered amount of time or the track has finished downloading
				bufferedEnough = secondsBuffered >= iBufferSize.Int()
									|| (*iPlaylist)[iCurrentTrack]->Buffered() == (*iPlaylist)[iCurrentTrack]->DataSize();
				}
			}
		else
			{
			// This is just a precaution in case we don't know the size of the mp3
			bufferedEnough = iPreBuffer.Count() >= KBufferSizeInPackets;
			}
		
		if (iOpen && !iPlaying && bufferedEnough)
			{
			// start playing the track
			iPlaying = ETrue;
			
			if (!IsActive())
				{
				// The next piece of data will be written to the output stream in RunL() 
				TRequestStatus* status = &iStatus;
				User::RequestComplete(status, KErrNone);
				SetActive();
				}
			
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
		else if (iOpen && iPlaying)
			{
			// We are already playing so write some more stuff to the buffer
			
			if (!IsActive())
				{
				// The next piece of data will be written to the output stream in RunL() 
				TRequestStatus* status = &iStatus;
				User::RequestComplete(status, KErrNone);
				SetActive();
				}
			}
		}
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::MaoscOpenComplete(TInt aError)
	{
	if (aError == KErrNone)
		{
		iOpen = ETrue;
		
		if (!iAlreadyOpened)
			{
			iAlreadyOpened = ETrue;
			iMdaAudioDataSettings.iVolume = iMdaAudioOutputStream->MaxVolume() / 2;
			}
		
		iMdaAudioOutputStream->SetVolume(iMdaAudioDataSettings.iVolume);
		iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz;
		iMdaAudioDataSettings.iChannels = TMdaAudioDataSettings::EChannelsStereo;
		TRAP_IGNORE(iMdaAudioOutputStream->SetDataTypeL(KMMFFourCCCodeMP3));
		iMdaAudioOutputStream->SetAudioPropertiesL(iMdaAudioDataSettings.iSampleRate, iMdaAudioDataSettings.iChannels);
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
	{
	if (CurrentTrack())
		{
		CurrentTrack()->SetPlaybackPosition(iMdaAudioOutputStream->Position().Int64() / 1000000 );
		}
	
	delete iWrittenBuffer[0];
	iWrittenBuffer.Remove(0);
	
	if (iWrittenBuffer.Count() > 0)
		{
		// There is data already written to the
		// audio output stream so do nothing
		}
	else 
		{
		if (iPreBuffer.Count() > 0)
			{
			// There is still data to write
			// so force it to be writen now
			Cancel();
			iStatus = KErrNone;
			RunL();
			}
		else
			{
			if (!iTrackDownloading)
				{
				// The track has finished playing and finished downloading
				NextTrackL();
				}
			else
				{
				// The track is still downloading, but has run out of buffer
				iBufferOffset = (*iPlaylist)[iCurrentTrack]->Buffered();
				iPlaying = EFalse;
				}
			}
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
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


