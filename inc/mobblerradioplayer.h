/*
mobblerradioplayer.h

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

#ifndef __MOBBLERRADIOPLAYER_H__
#define __MOBBLERRADIOPLAYER_H__

#include <mda\common\audio.h>
#include <MdaAudioOutputStream.h>

#include "mobblerincomingcallmonitorobserver.h"
#include "mobblerlastfmconnection.h"
#include "mobblershareddata.h"
#include "mobbleraudiocontrol.h"

class CAudioEqualizerUtility;
class CDesC8Array;
class CMobblerAudioControl;
class CMobblerIncomingCallMonitor;
class CMobblerRadioPlaylist;
class CMobblerRadioPlaylistParser;
class CMobblerString;

class MMobblerRadioPlayer
	{
public:
	virtual void SetPlaylistL(CMobblerRadioPlaylist* aPlaylist) = 0;
	virtual void NextTrackL() = 0;
	virtual void Stop() = 0;
	};

class CMobblerRadioPlayer : public CBase,
							public MMobblerRadioPlayer,
							public MMobblerIncomingCallMonitorObserver,
							public MMobblerAudioControlObserver

	{
public:
	static CMobblerRadioPlayer* NewL(CMobblerLastFMConnection& aLastFMConnection, TTimeIntervalSeconds aPreBufferSize);
	~CMobblerRadioPlayer();
	
	TInt StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioText);
	
	CMobblerTrack* CurrentTrack();

	void NextTrackL();
	void Stop();
	
	void VolumeUp();
	void VolumeDown();
	void SetEqualizer(TInt aIndex);
	void SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize);
	
	TInt Volume() const;
	TInt MaxVolume() const;	
	const CMobblerString& Station() const;
	TBool HasPlaylist() const;
	
private: // from MMobblerAudioControlObserver
	void HandleAudioPositionChangeL();
	void HandleAudioFinishedL(CMobblerAudioControl* aAudioControl);
	
private:
	CMobblerRadioPlayer(CMobblerLastFMConnection& aSubmitter, TTimeIntervalSeconds aPreBufferSize);
	void ConstructL();
	
	void SubmitCurrentTrackL();
	
	void DoStop(TBool aDeleteNextTrack);
	
private:
	void DialogDismissedL(TInt aButtonId);

private: // MMobblerRadioPlayer	
	void SetPlaylistL(CMobblerRadioPlaylist* aPlaylist);
	void WriteL(const TDesC8& aData, TInt aTotalDataSize);
	
private:
	void HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState);

private:
	TInt iCurrentTrack;
	CMobblerRadioPlaylist* iPlaylist;
	
	CMobblerLastFMConnection& iLastFMConnection;
	
	CMobblerIncomingCallMonitor* iIncomingCallMonitor;
	
	CMobblerAudioControl* iCurrentAudioControl;
	CMobblerAudioControl* iNextAudioControl;
	
	TTimeIntervalSeconds iPreBufferSize;
	TInt iVolume;
	TInt iMaxVolume;
	};

#endif // __MOBBLERRADIOPLAYER_H__

// End of file
