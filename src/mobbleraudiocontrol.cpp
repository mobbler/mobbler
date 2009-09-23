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

#include <aknnotewrappers.h>
#include <e32svr.h>

#include "mobbleraudiocontrol.h"
#include "mobblerappui.h"
#include "mobbleraudiothread.h"
#include "mobblerstring.h"
#include "mobblertrack.h"



const TInt KTimerDuration(250000); // 1/4 second

const TInt KMobblerAudioThreadMinHeapSize(0x100000); // 1 MB
const TInt KMobblerAudioThreadMaxHeapSize(0x300000); // 3 MB

CMobblerAudioControl* CMobblerAudioControl::NewL(MMobblerAudioControlObserver& aObserver, CMobblerTrack& aTrack, TTimeIntervalSeconds aPreBufferSize, TInt aVolume, TInt aEqualizerIndex, TInt aBitRate)
	{
	CMobblerAudioControl* self = new(ELeave) CMobblerAudioControl(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aTrack, aPreBufferSize, aVolume, aEqualizerIndex, aBitRate);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAudioControl::CMobblerAudioControl(MMobblerAudioControlObserver& aObserver)
	:CActive(CActive::EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerAudioControl::ConstructL(CMobblerTrack& aTrack, TTimeIntervalSeconds aPreBufferSize, TInt aVolume, TInt aEqualizerIndex, TInt aBitRate)
	{
	// Set up the shared memory
	iShared.iDownloadComplete = EFalse;
	iShared.iPlaying = EFalse;
	iShared.iTrack = &aTrack;
	iShared.iPreBufferSize = aPreBufferSize;
	iShared.iAudioDataSettings.iVolume = aVolume;
	
	switch (aBitRate)
		{
		case 0:
			iShared.iAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate24000Hz;
			break;
		case 1:
			iShared.iAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz;
			break;
		default:
			// TODO: should panic
			break;
		}
	
	iShared.iAudioDataSettings.iChannels = TMdaAudioDataSettings::EChannelsStereo;
	iShared.iEqualizerIndex = aEqualizerIndex;
	
	// Create the audio thread and wait for it to finish loading
	User::LeaveIfError(iAudioThread.Create(KNullDesC, ThreadFunction, KDefaultStackSize, KMobblerAudioThreadMinHeapSize, KMobblerAudioThreadMaxHeapSize, &iShared));
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

void CMobblerAudioControl::DataCompleteL(CMobblerLastFmConnection::TTransactionError aTransactionError, TInt aHTTPStatusCode, const TDesC8& aStatusText)
	{
	iShared.iDownloadComplete = ETrue;
	
	switch (aTransactionError)
		{
		case CMobblerLastFmConnection::ETransactionErrorNone:
			{
			// Do nothing. This means that we have finished
			// downloading the mp3. The audio thread will close
			// when the track completes so this RunL will get called
			// to start the next track.
			}
			break;
		case CMobblerLastFmConnection::ETransactionErrorFailed:
			{
			// Display an error with the HTTP status code of the failure
			_LIT(KAudioErrorFormat, "%d %S");
			TBuf<25> message;
			CMobblerString* string(CMobblerString::NewL(aStatusText));
			message.Format(KAudioErrorFormat, aHTTPStatusCode, &string->String());
			delete string;
			CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
			note->ExecuteLD(message);
			}
			// Intentional follow through to the next case statement
		case CMobblerLastFmConnection::ETransactionErrorCancel:
			{
			// Ask for the audio thread to close because there has been an error
			// or the transaction was cancelled (i.e. we are stopping the radio).
			// We will tell the radio player that this track has finished in
			// the logon callback
			
			SendCmd(ECmdDestroyAudio);
			}
			break;
		default:
			break;
		}
	}

void CMobblerAudioControl::SetVolume(TInt aVolume)
	{
	iShared.iAudioDataSettings.iVolume = aVolume;
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

	if (!iDestroyCmdSent)
		{
		if (aCmd == ECmdDestroyAudio)
			{
			iDestroyCmdSent = ETrue;
			}
		
		TRequestStatus status;
		iAudioThread.Rendezvous(status);
		iAudioThread.RequestComplete(iShared.iCmdStatus, aCmd);
		User::WaitForRequest(status);
		}
	}

TTimeIntervalSeconds CMobblerAudioControl::PreBufferSize() const
	{
	return iShared.iPreBufferSize;
	}

TInt CMobblerAudioControl::Volume() const
	{
	return iShared.iAudioDataSettings.iVolume;
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
