/*
mobblershareddata.h

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

#ifndef __MOBBLERSHAREDDATA_H__
#define __MOBBLERSHAREDDATA_H__

#include <e32std.h>

class CAudioEqualizerUtility;
class CMobblerAudioThread;
class MMdaAudioOutputStreamCallback;

enum TMobblerAudioCmd
	{
	ECmdStartAudio = 0,
	ECmdStopAudio,
	ECmdPauseAudio,
	ECmdRestartAudio,
	ECmdDestroyAudio,
	ECmdSetVolume,
	ECmdSetEqualizer,
	ECmdServiceBuffer
	};

class TMobblerSharedData
	{
public:
	RSemaphore iAliveMutex;
	RMutex iMutex;
	RMutex iRestartMutex;
	TBool iStopped;
	TExcType iExc;
	TRequestStatus* iStatusPtr;
	TMobblerAudioCmd iCmd;
	TInt iVolume;
	TInt iMaxVolume;
	TInt iEqualizerIndex;
	RPointerArray<HBufC8> iPreBuffer;
	RPointerArray<HBufC8> iWrittenBuffer;
	RPointerArray<HBufC16> iEqualizerProfiles;
	CAudioEqualizerUtility* iEqualizer;
	TInt iPlaybackPosition;
	MMdaAudioOutputStreamCallback* iCallback;
	CMobblerAudioThread* iAudioThread;
	TBool iPaused;
	TInt iPauseOffset;
	};

#endif
