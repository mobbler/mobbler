/*
mobbleraudiocontrol.h

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

#ifndef __MOBBLERAUDIOCONTROL_H__
#define __MOBBLERAUDIOCONTROL_H__

// INCLUDES
#include <E32Base.h>
#include "mobblershareddata.h"

class CMobblerAudioControl : public CBase
	{
public:
	static CMobblerAudioControl* NewL(MMdaAudioOutputStreamCallback* aCallback);
	~CMobblerAudioControl();

private:
	CMobblerAudioControl();
	void ConstructL(MMdaAudioOutputStreamCallback* aCallback);

public:
	void Restart();
	void Stop();
	void Start();
	void Pause(TBool aPlaying);
	void SetVolume(TInt aVolume);
	void SetEqualizerIndex(TInt aIndex);
	void AddToBuffer(const TDesC8& aData, TBool aRunning);
	void BeginPlay();
	void TransferWrittenBuffer(RPointerArray<HBufC8>& aTrashCan);
	
	TInt Volume();
	TInt MaxVolume();
	TInt PlaybackPosition();
	RPointerArray<HBufC16>& EqualizerProfiles();
	TInt PreBufferSize();
	TBool IsPaused();

private:
	void SendCmd(TMobblerAudioCmd aCmd);

private:
	TMobblerSharedData iShared;
	RThread iAudioThread;
	TBool iCreated;
	};
  
#endif

// End of File
