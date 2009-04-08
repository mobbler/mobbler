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

#include "mobbleraudiocontrol.h"
#include "mobblerincomingcallmonitorobserver.h"

class CMobblerIncomingCallMonitor;
class CMobblerRadioPlaylist;
class CMobblerString;

class MMobblerRadioStateChangeObserver
	{
public:
	virtual void HandleRadioStateChangedL() = 0;
	};

class CMobblerRadioPlayer : public CActive,
							public MMobblerIncomingCallMonitorObserver,
							public MMobblerAudioControlObserver,
							public MMobblerFlatDataObserver

	{
public:
	enum TState
		{
		EIdle,
		EPlaying
		};
	
	enum TTransactionState
		{
		ENone,
		EFetchingPlaylist,
		ESelectingStation
		};
	
public:
	static CMobblerRadioPlayer* NewL(CMobblerLastFMConnection& aLastFMConnection, TTimeIntervalSeconds aPreBufferSize, TInt aEqualizerIndex, TInt aVolume);
	~CMobblerRadioPlayer();
	
	void AddObserverL(MMobblerRadioStateChangeObserver* aObserver);
	void RemoveObserver(MMobblerRadioStateChangeObserver* aObserver);
	
	void StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const CMobblerString* aRadioText);
	
	CMobblerTrack* CurrentTrack();
	
	TState State() const;
	TTransactionState TransactionState() const;

	void NextTrackL();
	void Stop();
	
	void VolumeUp();
	void VolumeDown();
	void SetEqualizer(TInt aIndex);
	void SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize);
	
	TInt Volume() const;
	TInt MaxVolume() const;	
	TInt EqualizerIndex() const;
	const CMobblerString& Station() const;
	TBool HasPlaylist() const;
	
private: // from MMobblerAudioControlObserver
	void HandleAudioPositionChangeL();
	void HandleAudioFinishedL(CMobblerAudioControl* aAudioControl, TInt aError);
	
private:
	CMobblerRadioPlayer(CMobblerLastFMConnection& aSubmitter, TTimeIntervalSeconds aPreBufferSize, TInt aEqualizerIndex, TInt aVolume);
	void ConstructL();
	
	void SubmitCurrentTrackL();
	
	void DoStop(TBool aDeleteNextTrack);
	
	void DoChangeStateL(TState aState);
	void DoChangeTransactionStateL(TTransactionState aState);
	
private:
	void DialogDismissedL(TInt aButtonId);

private: // MMobblerRadioPlayer	
	void DataL(const TDesC8& aData, CMobblerLastFMConnection::TError aError);
	
private:
	void HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState);
	
private:
	void RunL();
	void DoCancel();

private:
	TInt iCurrentTrackIndex;
	CMobblerRadioPlaylist* iCurrentPlaylist;
	CMobblerRadioPlaylist* iNextPlaylist;
	
	CMobblerLastFMConnection& iLastFMConnection;
	
	CMobblerIncomingCallMonitor* iIncomingCallMonitor;
	
	CMobblerAudioControl* iCurrentAudioControl;
	CMobblerAudioControl* iNextAudioControl;
	
	TTimeIntervalSeconds iPreBufferSize;
	TInt iVolume;
	TInt iMaxVolume;
	TInt iEqualizerIndex;
	
	TState iState;
	TTransactionState iTransactionState;
	
	CMobblerString* iStation;
	
	RPointerArray<MMobblerRadioStateChangeObserver> iObservers;
	
	RTimer iTimer;
	
	TBool iRestartRadioOnCallDisconnect;
	};

#endif // __MOBBLERRADIOPLAYER_H__

// End of file
