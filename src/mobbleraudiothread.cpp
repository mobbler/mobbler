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

#include "mobbleraudiocmddispatcher.h"
#include "mobbleraudiothread.h"
#include "mobblershareddata.h"

CMobblerAudioThread::CMobblerAudioThread(TAny* aData)
	: iShared(*((TMobblerSharedData*)aData))
	{
	}

CMobblerAudioThread::~CMobblerAudioThread()
	{
  delete iStream;
  delete iActiveScheduler;
  delete iCleanupStack;
  if (iActive) iActive->Cancel();  
  delete iActive;
  delete iEqualizer;
	}

TInt CMobblerAudioThread::ThreadFunction(TAny* aData)
	{
	TMobblerSharedData& shared = *((TMobblerSharedData*)aData);
		
	// tell client we're alive
	// signaled off when destroying this thread
	shared.iAliveMutex.Wait();
	shared.iAudioThread = NULL;
		  
	TRAPD(error, shared.iAudioThread = CMobblerAudioThread::CreateL(aData));
	if(error != KErrNone) return error;
	if(shared.iAudioThread == NULL) return KErrGeneral;
		
	shared.iStatusPtr = &(shared.iAudioThread->iActive->iStatus);
		
	// if we're still here, activescheduler has been constructed
	// start wait loop which runs until it's time to end the thread
	CActiveScheduler::Start();
	delete shared.iAudioThread;
	shared.iAudioThread = NULL;
		
	// tell owning thread it's safe to exit
	shared.iAliveMutex.Signal();
		
	return KErrNone;
	}

CMobblerAudioThread* CMobblerAudioThread::CreateL(TAny* aData)
	{
	CMobblerAudioThread* self = new (ELeave)CMobblerAudioThread(aData);
	if (self == NULL) return self;
	if (self->Construct() != KErrNone)
		{
		delete self;
		return NULL;
		}
	return self;
	}

TInt CMobblerAudioThread::Construct()
	{
	// create cleanup stack
	iCleanupStack = CTrapCleanup::New();
	if(iCleanupStack == NULL)
		{
		return KErrNoMemory;
		}
	TInt error = KErrNone;
	TRAP(error, ConstructL());
	return error;
	}

void CMobblerAudioThread::ConstructL()
	{
	// create active scheduler
	iActiveScheduler = new (ELeave)CActiveScheduler;
	CActiveScheduler::Install(iActiveScheduler);
	
	// sound inits
	iSet.iChannels = TMdaAudioDataSettings::EChannelsStereo;
	iSet.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz;
	iSet.iVolume = 0;
	
	iActive = new (ELeave)CMobblerAudioCmdDispatcher(this);
	iActive->Request();
	}

void CMobblerAudioThread::HandleUserInterruptCommandL()
	{
	if (iShared.iExc == EExcUserInterrupt)
		{
		switch(iShared.iCmd)
			{
			case ECmdStartAudio:
				{
				StartAudioL();
				break;
				}
			case ECmdStopAudio:
				{
				StopAudio();
				break;
				}
			case ECmdPauseAudio:
				{
				iStream->Stop();
				break;
				}
			case ECmdRestartAudio:
				{
				RestartAudioL();
				break;
				}
			case ECmdDestroyAudio:
				{
				if (iStream)
					{
					iStream->Stop();
					delete iStream;
					iStream = NULL;
					}
				if (iActive)
					{
					iActive->Cancel();
					delete iActive;
					iActive = NULL;
					}
				delete iActiveScheduler;
				iActiveScheduler = NULL;
				CActiveScheduler::Stop(); // Exit
				break;
				}
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
			case ECmdServiceBuffer:
				{
				FillBuffer();
				break;
				}
			}
		}
	else
		{
		// if unknown exception, just exit this thread
		CActiveScheduler::Stop();
		}
	}

void CMobblerAudioThread::StartAudioL()
	{
	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSet);
	iShared.iMutex.Wait();
	iShared.iMaxVolume = iStream->MaxVolume();
	iShared.iMutex.Signal();
	iEqualizer = NULL;
#ifndef _DEBUG
	// Emulator seems to crap out even when TRAP_IGNORE is used,
	// The emulator doesn't support the equalizer anyway, and so
	// I just don't compile this line for the debug version
	TRAP_IGNORE(iEqualizer = CAudioEqualizerUtility::NewL(*iStream));
#endif
	SetVolume();
	SetEqualizerIndexL();
	iShared.iMutex.Wait();
	iShared.iEqualizer = iEqualizer;
	if (iEqualizer)
		{
		for (TInt i = 0; i < iEqualizer->Presets().Count(); ++i)
			{
			iShared.iEqualizerProfiles.Append(iEqualizer->Presets()[i].iPresetName.Alloc());
			}
		}
	iShared.iStopped = EFalse;
	iShared.iMutex.Signal();
	}

void CMobblerAudioThread::StopAudio()
	{
	if (iStream)
		{
		iStream->Stop();
		delete iStream;
		iStream = NULL;
		}
	iShared.iMutex.Wait();
	iShared.iStopped = ETrue;
	iShared.iEqualizerProfiles.ResetAndDestroy();
	iShared.iMutex.Signal();
	}

void CMobblerAudioThread::RestartAudioL()
	{
	StopAudio();
	StartAudioL();
	}

void CMobblerAudioThread::SetVolume()
	{
	iStream->SetVolume(iShared.iVolume);
	}

void CMobblerAudioThread::SetEqualizerIndexL()
	{
	if (!iEqualizer) return;
	iShared.iMutex.Wait();
	TInt index = iShared.iEqualizerIndex;
	iShared.iMutex.Signal();
	if (index < 0)
		{
		iEqualizer->Equalizer().DisableL();
		}
	else
		{
		iEqualizer->Equalizer().EnableL();
		iEqualizer->ApplyPresetL(index);
		}
	}

void CMobblerAudioThread::FillBuffer()
	{
	iShared.iMutex.Wait();
	if ((iShared.iPreBuffer.Count() > 0) && !iShared.iPaused)
		{
		iStream->WriteL(*iShared.iPreBuffer[0]);
		iShared.iWrittenBuffer.AppendL(iShared.iPreBuffer[0]);
		iShared.iPreBuffer.Remove(0);
		}
	iShared.iMutex.Signal();
	}

void CMobblerAudioThread::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
	{
	const TInt KMilliSecsInOneSecond(1000000);
	iShared.iMutex.Wait();
	iShared.iPlaybackPosition = iStream->Position().Int64() / KMilliSecsInOneSecond;
	MMdaAudioOutputStreamCallback* callback = iShared.iCallback;
	iShared.iMutex.Signal();
	callback->MaoscBufferCopied(aError, aBuffer);
	FillBuffer();
	}

void CMobblerAudioThread::MaoscOpenComplete(TInt aError)
	{
	if (aError)
		{
		iError = aError;
		}
	else
		{
		TRAP_IGNORE(iStream->SetDataTypeL(KMMFFourCCCodeMP3));
		iStream->SetAudioPropertiesL(iSet.iSampleRate, iSet.iChannels);
		iShared.iMutex.Wait();
		MMdaAudioOutputStreamCallback* callback = iShared.iCallback;
		iShared.iMutex.Signal();
		callback->MaoscOpenComplete(aError);
		}
	}

void CMobblerAudioThread::MaoscPlayComplete(TInt aError)
	{
	if( aError )
		{
		iError = aError;
		}
	else
		{
		iShared.iMutex.Wait();
		MMdaAudioOutputStreamCallback* callback = iShared.iCallback;
		iShared.iMutex.Signal();
		callback->MaoscPlayComplete(aError);
		}
	}

void CMobblerAudioThread::SetEqualizer(TInt aIndex)
	{
	iShared.iMutex.Wait();
	iShared.iEqualizerIndex = aIndex;
	iShared.iMutex.Signal();
	if (!iEqualizer) return;
	if (aIndex < 0)
		{
		iEqualizer->Equalizer().DisableL();
		}
	else
		{
		iEqualizer->Equalizer().EnableL();
		iEqualizer->ApplyPresetL(aIndex);
		}
	}
