/*
mobbleraudiocontrol.cpp

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

#include "mobbleraudiocontrol.h"
#include "mobblerappui.h"
#include "mobbleraudiothread.h"
#include "mobblertrack.h"

#include <e32svr.h>

const TInt KTimerDuration(250000); // 1/4 second
const TInt KMobblerHeapSize(1000000); // 1 MB

CMobblerAudioControl* CMobblerAudioControl::NewL(MMobblerAudioControlObserver& aObserver, CMobblerTrack& aTrack, TTimeIntervalSeconds aPreBufferSize, TInt aVolume, TInt aEqualizerIndex)
	{
	CMobblerAudioControl* self = new(ELeave) CMobblerAudioControl(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aTrack, aPreBufferSize, aVolume, aEqualizerIndex);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAudioControl::CMobblerAudioControl(MMobblerAudioControlObserver& aObserver)
	:CActive(CActive::EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerAudioControl::ConstructL(CMobblerTrack& aTrack, TTimeIntervalSeconds aPreBufferSize, TInt aVolume, TInt aEqualizerIndex)
	{
	// Set up the shared memory
	iShared.iDownloadComplete = EFalse;
	iShared.iPlaying = EFalse;
	iShared.iTrack = &aTrack;
	iShared.iPreBufferSize = aPreBufferSize;
	iShared.iVolume = aVolume;
	iShared.iEqualizerIndex = aEqualizerIndex;
	
	// Create the audio thread and wait for it to finish loading
	User::LeaveIfError(iAudioThread.Create(KNullDesC, ThreadFunction, KDefaultStackSize, KMinHeapSize, KMinHeapSize + KMobblerHeapSize, &iShared));
	iAudioThread.SetPriority(EPriorityRealTime);
	TRequestStatus status;
	iAudioThread.Rendezvous(status);
	iAudioThread.Resume();
	User::WaitForRequest(status);
	
	iAudioThread.Logon(iStatus);
	SetActive();
	
	iTimer = CPeriodic::NewL(CPeriodic::EPriorityStandard);
	TCallBack callBack(HandleAudioPositionChangeL, this);
	iTimer->Start(KTimerDuration, KTimerDuration, callBack);
	SetVolume(aVolume);
	SetEqualizerIndex(aEqualizerIndex);
	}

CMobblerAudioControl::~CMobblerAudioControl()
	{
	Cancel();
	
	delete iTimer;
	
	if (iAudioThread.ExitType() == EExitPending)
		{
		// The thread is still running so destroy
		// it and then wait for it to close
		TRequestStatus threadStatus;
		iAudioThread.Logon(threadStatus);
		SendCmd(ECmdDestroyAudio);
		User::WaitForRequest(threadStatus);
		}
	
	iAudioThread.Close();
	}

void CMobblerAudioControl::RunL()
	{
	iObserver.HandleAudioFinishedL(this);	
	}

void CMobblerAudioControl::DoCancel()
	{
	iAudioThread.LogonCancel(iStatus);
	}

TInt CMobblerAudioControl::HandleAudioPositionChangeL(TAny* aSelf)
	{
	static_cast<CMobblerAudioControl*>(aSelf)->iObserver.HandleAudioPositionChangeL();
	return KErrNone;
	}

void CMobblerAudioControl::DataPartL(const TDesC8& aData, TInt aTotalDataSize)
	{
	iShared.iTotalDataSize = aTotalDataSize;
	iShared.iAudioData.Set(aData);
	SendCmd(ECmdWriteData);
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerAudioControl::DataCompleteL(TInt aError)
	{
	if (aError == KErrNone || aError == KErrCancel)
		{
		iShared.iDownloadComplete = ETrue;
		}
	else
		{
		iObserver.HandleAudioFinishedL(this);
		}
	}

void CMobblerAudioControl::SetVolume(TInt aVolume)
	{
	iShared.iVolume = aVolume;
	SendCmd(ECmdSetVolume);
	}

void CMobblerAudioControl::SetEqualizerIndex(TInt aIndex)
	{
	iShared.iEqualizerIndex = aIndex;
	SendCmd(ECmdSetEqualizer);
	}

void CMobblerAudioControl::SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize)
	{
	iShared.iPreBufferSize = aPreBufferSize;
	}

void CMobblerAudioControl::SetCurrent()
	{
	iShared.iCurrent = ETrue;
	SendCmd(ECmdSetCurrent);
	}

void CMobblerAudioControl::SendCmd(TMobblerAudioCmd aCmd)
	{	    
	// send the command and wait for the audio thread to respond to it

	TRequestStatus status;
	iAudioThread.Rendezvous(status);
	iAudioThread.RequestComplete(iShared.iCmdStatus, aCmd);
	User::WaitForRequest(status);
	}

TTimeIntervalSeconds CMobblerAudioControl::PreBufferSize() const
	{
	return iShared.iPreBufferSize;
	}

TInt CMobblerAudioControl::Volume() const
	{
	return iShared.iVolume;
	}

TInt CMobblerAudioControl::MaxVolume() const
	{
	return iShared.iMaxVolume;
	}

TBool CMobblerAudioControl::Playing() const
	{
	return iShared.iPlaying;
	}

TBool CMobblerAudioControl::DownloadComplete() const
	{
	return iShared.iDownloadComplete;
	}

// End of File
