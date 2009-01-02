/*
mobbleraudiothread.cpp

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

#include <e32svr.h>

#include "mobbleraudiothread.h"
#include "mobblershareddata.h"
#include "mobblertrack.h"

const TInt KBufferSizeInPackets(32);

TInt ThreadFunction(TAny* aData)
	{
	__UHEAP_MARK;
	
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	
	CActiveScheduler* activeScheduler = new CActiveScheduler;
	CActiveScheduler::Install(activeScheduler);
	
	CMobblerAudioThread* audioThread(NULL);
		  
	TRAPD(error, audioThread = CMobblerAudioThread::NewL(aData));
	
	if (error == KErrNone)
		{
		// if we're still here, activescheduler has been constructed
		// start wait loop which runs until it's time to end the thread
		CActiveScheduler::Start();
		}
	
	delete audioThread;
	delete activeScheduler;
	delete cleanupStack;
	
	__UHEAP_MARKEND;
		
	return error;
	}

CMobblerAudioThread* CMobblerAudioThread::NewL(TAny* aData)
	{
	CMobblerAudioThread* self = new(ELeave) CMobblerAudioThread(aData);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAudioThread::CMobblerAudioThread(TAny* aData)
	:CActive(CActive::EPriorityStandard), iShared(*static_cast<TMobblerSharedData*>(aData))
	{
	CActiveScheduler::Add(this);
	
	// sound inits
	iSet.iChannels = TMdaAudioDataSettings::EChannelsStereo;
	iSet.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz;
	iSet.iVolume = iShared.iVolume;
	
	iShared.iPlaying = EFalse;
	iPreBufferOffset = 0;
	}

void CMobblerAudioThread::ConstructL()
	{
	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSet);
	
#ifndef __WINS__
	// Emulator seems to crap out even when TRAP_IGNORE is used,
	// it doesn't support the equalizer anyway
	TRAP_IGNORE(iShared.iEqualizer = CAudioEqualizerUtility::NewL(*iStream));
#endif
	
	SetVolume();
	SetEqualizerIndexL();
	
	Request();
	}

CMobblerAudioThread::~CMobblerAudioThread()
	{
	Cancel();
	
	delete iShared.iEqualizer;
	delete iStream;

	iBuffer.ResetAndDestroy();
	}

void CMobblerAudioThread::Request()
	{
	iShared.iCmdStatus = &iStatus;
	iStatus = KRequestPending;
	SetActive();
	}

void CMobblerAudioThread::RunL()
	{
	switch (iStatus.Int())
		{
		case ECmdSetVolume:
			{
			SetVolume();
			break;
			}
		case ECmdSetEqualizer:
			{
			SetEqualizerIndexL();
			break;
			}
		case ECmdWriteData:
			{
			// Put the data in the buffer
			iBuffer.AppendL(iShared.iAudioData.AllocLC());
			CleanupStack::Pop(); // aData.AllocLC()
			
			FillBufferL(ETrue);
			}
			break;
		case ECmdServiceBuffer:
			{
			FillBufferL(EFalse);
			}
			break;
		case ECmdDestroyAudio:
			{
			DestroyAudio();
			break;
			}
		}
	
	// Let the main thread know that we are ready for another command
	Request();
	RThread().Rendezvous(KErrNone);
	}

void CMobblerAudioThread::DoCancel()
	{
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrCancel);
	}

TInt CMobblerAudioThread::RunError(TInt aError)
	{
	// There was an error in the RunL so just end the thread
	
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	return KErrNone;
	}

void CMobblerAudioThread::DestroyAudio()
	{
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	}

void CMobblerAudioThread::SetVolume()
	{
	iStream->SetVolume(iShared.iVolume);
	}

void CMobblerAudioThread::SetEqualizerIndexL()
	{
	if (iShared.iEqualizer)
		{
		if (iShared.iEqualizerIndex < 0)
			{
			iShared.iEqualizer->Equalizer().DisableL();
			}
		else
			{
			iShared.iEqualizer->Equalizer().EnableL();
			iShared.iEqualizer->ApplyPresetL(iShared.iEqualizerIndex);
			}
		}
	}

TBool CMobblerAudioThread::PreBufferFilled() const
	{
	TBool preBufferedFilled(EFalse);
	
	TInt bufferTotal = iShared.iTrack->DataSize();
	TInt bufferPosition = Min(iShared.iTrack->Buffered(), bufferTotal);
	TInt playbackTotal = iShared.iTrack->TrackLength().Int();
	TInt playbackPosition = Min(iShared.iTrack->PlaybackPosition().Int(), playbackTotal);
	
	if (iShared.iTrack->DataSize() != 1)
		{
		TInt byteRate = iShared.iTrack->DataSize() / iShared.iTrack->TrackLength().Int();
		TInt buffered = iShared.iTrack->Buffered() - iPreBufferOffset.Int();
		
		if (byteRate != 0)
			{
			TInt secondsBuffered = buffered / byteRate;
			
			// either buffered amount of time or the track has finished downloading
			preBufferedFilled = secondsBuffered >= iShared.iPreBufferSize.Int() || iShared.iTrack->Buffered() == iShared.iTrack->DataSize();
			}
		}
	else
		{
		preBufferedFilled = iBuffer.Count() >= KBufferSizeInPackets;
		}
	
	return preBufferedFilled;
	}

void CMobblerAudioThread::FillBufferL(TBool aDataAdded)
	{	
	if (iShared.iPlaying)
		{
		if (aDataAdded)
			{
			// we are already playing so add the last
			//piece of the buffer to the stream
			iStream->WriteL(*iBuffer[iBuffer.Count() - 1]);
			}
		}
	else if (!iShared.iPlaying && iShared.iCurrent && PreBufferFilled())
		{
		// Start playing the track
		iShared.iPlaying = ETrue;
		
		// write all the data we have to the audio output stream
		const TInt KBufferCount(iBuffer.Count());
		for (TInt i(0) ; i < KBufferCount ; ++i)
			{
			iStream->WriteL(*iBuffer[i]);
			}
		}
	}

void CMobblerAudioThread::MaoscBufferCopied(TInt aError, const TDesC8& /*aBuffer*/)
	{
	delete iBuffer[0];
	iBuffer.Remove(0);
	
	const TInt KMicrosecondsInOneSecond(1000000);
	iShared.iTrack->SetPlaybackPosition(iStream->Position().Int64() / KMicrosecondsInOneSecond);
	
	if (iBuffer.Count() == 0)
		{
		if (iShared.iDownloadComplete)
			{
			// We don't have anything to write to the audio output stream
			// and the mp3 download has finished so exit
			
			if (!iActiveSchedulerStopped)
				{
				iActiveSchedulerStopped = ETrue;
				CActiveScheduler::Stop();
				}
			}
		else
			{
			// There is more data on its way so start buffering again
			iShared.iPlaying = EFalse;
			iPreBufferOffset = iShared.iTrack->Buffered();
			}
		}
	}

void CMobblerAudioThread::MaoscOpenComplete(TInt /*aError*/)
	{
	TRAP_IGNORE(iStream->SetDataTypeL(KMMFFourCCCodeMP3));
	iStream->SetAudioPropertiesL(iSet.iSampleRate, iSet.iChannels);
	iStream->SetVolume(iShared.iVolume);
	iShared.iMaxVolume = iStream->MaxVolume();
	
	// Tell the creating thread that we are now running
	RThread().Rendezvous(KErrNone);
	}

void CMobblerAudioThread::MaoscPlayComplete(TInt /*aError*/)
	{
	}

// End of file