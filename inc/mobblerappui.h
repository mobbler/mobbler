/*
mobblerappui.h

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

#ifndef __MOBBLERAPPUI_h__
#define __MOBBLERAPPUI_h__

#include <aknviewappui.h>

#include "mobblerdownload.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblersleeptimer.h"


_LIT(KVersionNumberDisplay,		"0.3.4");
const TVersion version(0, 3, 4);

class CBrowserLauncher;
class CMobblerDownload;
class CMobblerMusicAppListener;
class CMobblerRadioPlayer;
class CMobblerResourceReader;
class CMobblerSettingItemListView;
class CMobblerStatusView;
class CMobblerTrack;

class CMobblerAppUi : public CAknViewAppUi,
						public MMobblerLastFMConnectionObserver,
						public MMobblerDownloadObserver,
					    public MMobblerSleepTimerNotify
	{
public:
	void ConstructL();
	CMobblerAppUi();
	~CMobblerAppUi();
	
	const CMobblerTrack* CurrentTrack() const;
	CMobblerTrack* CurrentTrack();
	
	CMobblerRadioPlayer* RadioPlayer() const;
	const TDesC& MusicAppNameL() const;
	
	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetCheckForUpdatesL(TBool aAutoUpdatesOn);
	void SetIapIDL(TUint32 aIapID);
	void SetBufferSize(TTimeIntervalSeconds aBufferSize);
	
	TInt Scrobbled() const;
	TInt Queued() const;
	
	void StatusDrawDeferred();
	
	CMobblerLastFMConnection::TMode Mode() const;
	CMobblerLastFMConnection::TState State() const;

	TBool RadioResumable() const;
	TBool Foreground() const;
	TBool Backlight() const;
	TInt ScrobblePercent() const;
	void SaveVolume();

	HBufC* AllocReadLC(TInt aResourceId);

public: // CEikAppUi
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);

private:
	void HandleInstallStartedL();
	
private:
	void HandleStatusPaneSizeChange();
	
	void HandleConnectCompleteL(TInt aError);
	void HandleLastFMErrorL(CMobblerLastFMError& aError);
	void HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus);
	void HandleTrackSubmittedL(const CMobblerTrack& aTrack);
	void HandleTrackQueuedL(const CMobblerTrack& aTrack);
	void HandleTrackDequeued(const CMobblerTrack& aTrack);
	void HandleTrackNowPlayingL(const CMobblerTrack& aTrack);
	void HandleUpdateResponseL(TVersion aVersion, const TDesC8& aLocation);
	
	void RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioOption);

	void LoadRadioStationsL();
	void SaveRadioStationsL();
	void SetSleepTimer();
private: // from MMobblerSleepTimerNotify
	void TimerExpiredL(TAny* aTimer, TInt aError);
	
private:
	// the view classes
	CMobblerSettingItemListView* iSettingView;
	CMobblerStatusView* iStatusView;
	
	// The application engine classes
	CMobblerLastFMConnection* iLastFMConnection;
	CMobblerRadioPlayer* iRadioPlayer;
	CMobblerMusicAppListener* iMusicListener;

	// The current track submit and queue count
	TInt iTracksSubmitted;
	TInt iTracksQueued;
	
	CMobblerLastFMConnection::TRadioStation iRadioStation;
	HBufC8* iRadioOption;
	TBool iCheckForUpdates;
	TBool iResumeStationOnConnectCompleteCallback;

	CMobblerLastFMConnection::TRadioStation iPreviousRadioStation;
	HBufC* iPreviousRadioArtist;
	HBufC* iPreviousRadioTag;
	HBufC* iPreviousRadioUser;
	
#ifndef __WINS__
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;
	
	CMobblerDownload* iMobblerDownload;

	CMobblerResourceReader* iResourceReader;

	CMobblerSleepTimer* iSleepTimer;
	TTime iTimeToSleep;
	TInt iSleepAction;
	};

#endif // __MOBBLERAPPUI_h__

// End of file
