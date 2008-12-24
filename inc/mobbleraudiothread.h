/*
mobbleraudiothread.h

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

#ifndef __MOBBLERAUDIOTHREAD_H__
#define __MOBBLERAUDIOTHREAD_H__

#include <audioequalizerbase.h>
#include <audioequalizerutility.h>
#include <e32base.h>
#include <mdaaudiooutputstream.h>
#include <mda\common\audio.h>

#include "mobblershareddata.h"

class CMobblerAudioCmdDispatcher;
class CAudioEqualizerUtility;

class CMobblerAudioThread : CBase, MMdaAudioOutputStreamCallback
	{
public:
	CMobblerAudioThread();
	virtual ~CMobblerAudioThread();
	
public:
    static TInt ThreadFunction(TAny* aData);
    void HandleUserInterruptCommandL();
    
private:
    CMobblerAudioThread(TAny* aData);
    static CMobblerAudioThread* CreateL(TAny* aData);
    TInt Construct();
    void ConstructL();
    
public:
    void StartAudioL();
    void StopAudio();
    void RestartAudioL();
    void SetVolume();
    void SetEqualizerIndexL();
    void FillBuffer();
    void SetEqualizer(TInt aIndex);
    
    void MaoscPlayComplete(TInt aError);
    void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
    void MaoscOpenComplete(TInt aError);
    
private:
	CTrapCleanup* iCleanupStack;
	CActiveScheduler* iActiveScheduler;
	CMdaAudioOutputStream* iStream;
	TMdaAudioDataSettings iSet;
	CAudioEqualizerUtility* iEqualizer;
  
	TInt iError;   // contains CMdaAudioOutput errors

	TMobblerSharedData& iShared;  // reference to shared data with client

	CMobblerAudioCmdDispatcher* iActive;
	};

#endif // __MOBBLERAUDIOTHREAD_H__
