/*
mobbleraudiocontrol.cpp

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

#include "mobbleraudiocontrol.h"
#include "mobbleraudiothread.h"

#include <e32svr.h>

_LIT(KMobblerThreadName, "MobblerAudio");

const TInt KWaitForEndTime(250000); // 1/4 second
const TInt KInitialCount(1);
const TInt KMobblerHeapSize(1000000); // 1 MB

CMobblerAudioControl* CMobblerAudioControl::NewL(MMdaAudioOutputStreamCallback* aCallback)
	{
	CMobblerAudioControl* self = new (ELeave)CMobblerAudioControl();
	CleanupStack::PushL(self);
	self->ConstructL(aCallback);
	CleanupStack::Pop(self);
	return self;
	}

void CMobblerAudioControl::ConstructL(MMdaAudioOutputStreamCallback* aCallback)
	{
	iShared.iCallback = aCallback;
	User::LeaveIfError(iShared.iAliveMutex.CreateLocal(KInitialCount));
	User::LeaveIfError(iShared.iMutex.CreateLocal());
	User::LeaveIfError(iAudioThread.Create(KMobblerThreadName, CMobblerAudioThread::ThreadFunction, KDefaultStackSize, KMinHeapSize, KMinHeapSize + KMobblerHeapSize, &iShared));
	iAudioThread.SetPriority(EPriorityRealTime);
	iAudioThread.Resume();
	iCreated = EFalse;
	iShared.iPaused = EFalse;
	}

CMobblerAudioControl::CMobblerAudioControl()
	{
	}

CMobblerAudioControl::~CMobblerAudioControl()
	{
	SendCmd(ECmdDestroyAudio);
	iShared.iAliveMutex.Wait(KWaitForEndTime);
	iShared.iAliveMutex.Close();
	iShared.iMutex.Close();
	iShared.iPreBuffer.ResetAndDestroy();
	if (iShared.iAudioThread) delete iShared.iAudioThread;
	}

void CMobblerAudioControl::Restart()
	{
	iShared.iMutex.Wait();
	iShared.iPreBuffer.ResetAndDestroy();
	iShared.iMutex.Signal();
	SendCmd(ECmdRestartAudio);
	iShared.iMutex.Wait();
	iShared.iPauseOffset = 0;
	iShared.iPaused = EFalse;
	iShared.iMutex.Signal();
	}

void CMobblerAudioControl::Stop()
	{
	iShared.iMutex.Wait();
	iShared.iPreBuffer.ResetAndDestroy();
	iShared.iMutex.Signal();
	if (iCreated)
		{
		iCreated = EFalse;
		SendCmd(ECmdStopAudio);
		while (!iShared.iStopped);
		}
	}

void CMobblerAudioControl::Start()
	{
	if (iCreated) return;
	iCreated = ETrue;
	SendCmd(ECmdStartAudio);
	iShared.iMutex.Wait();
	iShared.iPauseOffset = 0;
	iShared.iPaused = EFalse;
	iShared.iMutex.Signal();
	}

void CMobblerAudioControl::Pause(TBool aPlaying)
	{
	if (!iCreated || !aPlaying) return;
	SendCmd(ECmdPauseAudio);
	iShared.iMutex.Wait();
	iShared.iPaused = !iShared.iPaused;
	if (iShared.iPaused)
		{
		iShared.iPauseOffset += iShared.iPlaybackPosition;
		}
	else
		{
		SendCmd(ECmdServiceBuffer);
		}
	iShared.iMutex.Signal();
	}

TBool CMobblerAudioControl::IsPaused()
	{
	iShared.iMutex.Wait();
	TBool pause = iShared.iPaused;
	iShared.iMutex.Signal();
	return pause;
	}

void CMobblerAudioControl::SetVolume(TInt aVolume)
	{
	iShared.iMutex.Wait();
	iShared.iVolume = aVolume;
	iShared.iMutex.Signal();
	if (iCreated) SendCmd(ECmdSetVolume);
	}

void CMobblerAudioControl::SetEqualizerIndex(TInt aIndex)
	{
	iShared.iMutex.Wait();
	iShared.iEqualizerIndex = aIndex;
	iShared.iMutex.Signal();
	if (iCreated) SendCmd(ECmdSetEqualizer);
	}

void CMobblerAudioControl::AddToBuffer(const TDesC8& aData, TBool aRunning)
	{
	iShared.iMutex.Wait();
	TInt count = iShared.iPreBuffer.Count();
	iShared.iPreBuffer.Append(aData.AllocLC());
	CleanupStack::Pop(); // aData.AllocLC()
	iShared.iMutex.Signal();
	if ((count == 0) && aRunning && !IsPaused()) SendCmd(ECmdServiceBuffer);
	}

void CMobblerAudioControl::BeginPlay()
	{
	SendCmd(ECmdServiceBuffer);
	}

void CMobblerAudioControl::SendCmd(TMobblerAudioCmd aCmd)
	{
	iShared.iMutex.Wait();
	iShared.iCmd = aCmd;
	iShared.iMutex.Signal();
	iShared.iExc = EExcUserInterrupt;
	    
	TRequestStatus* status = iShared.iStatusPtr;
	if(status->Int() == KRequestPending)
		{
		iAudioThread.RequestComplete(status, KErrNone);
		}
	else
		{
		// This should never happen, but we add this code to be safe
		}
	}

TInt CMobblerAudioControl::Volume()
	{
	iShared.iMutex.Wait();
	TInt volume = iShared.iVolume;
	iShared.iMutex.Signal();
	return volume;
	}

TInt CMobblerAudioControl::MaxVolume()
	{
	iShared.iMutex.Wait();
	TInt maxVolume = iShared.iMaxVolume;
	iShared.iMutex.Signal();
	return maxVolume;
	}

TInt CMobblerAudioControl::PlaybackPosition()
	{
	iShared.iMutex.Wait();
	TInt position = iShared.iPlaybackPosition + iShared.iPauseOffset;
	iShared.iMutex.Signal();
	return position;
	}

TInt CMobblerAudioControl::PreBufferSize()
	{
	iShared.iMutex.Wait();
	TInt count = iShared.iPreBuffer.Count();
	iShared.iMutex.Signal();
	return count;
	}

RPointerArray<HBufC16>& CMobblerAudioControl::EqualizerProfiles()
	{
	iShared.iMutex.Wait();
	RPointerArray<HBufC16>& profiles = iShared.iEqualizerProfiles;
	iShared.iMutex.Signal();
	return profiles;
	}

void CMobblerAudioControl::TransferWrittenBuffer(RPointerArray<HBufC8>& aTrashCan)
	{
	iShared.iMutex.Wait();
	for (TInt i = 0; i < iShared.iWrittenBuffer.Count(); ++i)
		{
		aTrashCan.Append(iShared.iWrittenBuffer[i]);
		}
	iShared.iWrittenBuffer.Reset();
	iShared.iMutex.Signal();
	}

// End of File
