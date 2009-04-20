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

#ifndef __MOBBLERAPPUI_H__
#define __MOBBLERAPPUI_H__

#include <aknviewappui.h>
#include <remconcoreapitargetobserver.h>    // link against RemConCoreApi.lib
#include <remconcoreapitarget.h>            // and
#include <remconinterfaceselector.h>        // RemConInterfaceBase.lib

#include "mobblerdataobserver.h"
#include "mobblerdownload.h"
#include "mobblergesturesinterface.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblersleeptimer.h"

const TVersion KVersion(0, 4, 5);
_LIT(KFormatTime, "%F%D %N %-B%J%:1%T%+B"); // 21 March 11:20 am

#ifdef BETA_BUILD
const TInt KUpdateIntervalDays(1);
#else
const TInt KUpdateIntervalDays(7);
#endif

class CBrowserLauncher;
class CMobblerBitmapCollection;
class CMobblerDownload;
class CMobblerMusicAppListener;
class CMobblerRadioPlayer;
class CMobblerResourceReader;
class CMobblerSettingItemListView;
class CMobblerStatusView;
class CMobblerString;
class CMobblerTrack;
class CMobblerWebServicesView;
class CMobblerWebServicesHelper;

class CMobblerAppUi : public CAknViewAppUi,
						public MMobblerLastFMConnectionObserver,
						public MMobblerDownloadObserver,
						public MMobblerSleepTimerNotify,
						public MRemConCoreApiTargetObserver,
						public MMobblerFlatDataObserverHelper,
						public MMobblerGestures
	{	
public:
	void ConstructL();
	CMobblerAppUi();
	~CMobblerAppUi();
	
	const CMobblerTrack* CurrentTrack() const;
	CMobblerTrack* CurrentTrack();
	
	CMobblerRadioPlayer& RadioPlayer() const;
	CMobblerLastFMConnection& LastFMConnection() const;
	CMobblerMusicAppListener& MusicListener() const;
	CMobblerBitmapCollection& BitmapCollection() const;
	
	CMobblerSettingItemListView& SettingView() const;
	const TDesC& MusicAppNameL() const;
	
	void RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, 
					 const CMobblerString* aRadioOption, 
					 TBool aSaveStations = ETrue);
	
	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetIapIDL(TUint32 aIapID);
	void SetBufferSize(TTimeIntervalSeconds aBufferSize);
	void SetAccelerometerGestures(TBool aAccelerometerGestures);
	TBool AccelerometerGesturesAvailable() const;
	
	TInt Scrobbled() const;
	TInt Queued() const;
	
	void StatusDrawDeferred();
	
	CMobblerLastFMConnection::TMode Mode() const;
	CMobblerLastFMConnection::TState State() const;
	TBool ScrobblingOn() const;

	TBool RadioResumable() const;
	TBool Foreground() const;
	TBool Backlight() const;
	TInt ScrobblePercent() const;
	TInt DownloadAlbumArt() const;
	void SaveVolume();

	CMobblerResourceReader& CMobblerAppUi::ResourceReader() const;

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
	
	TBool GoOnlineL();
	
	void LoadRadioStationsL();
	void SaveRadioStationsL();
	void SetSleepTimer();
	TBool RadioStartable() const;

private: // from MMobblerSleepTimerNotify
	void TimerExpiredL(TAny* aTimer, TInt aError);
	
private: // auto-repeat audio button callbacks
	static TInt VolumeUpCallBackL(TAny *self);
	static TInt VolumeDownCallBackL(TAny *self);

private:
	void LoadGesturesPluginL();

	// Gestures, from MMobblerGestures
	void HandleSingleShakeL(TMobblerShakeGestureDirection aDirection);
	
	// Observer of media button clicks
	void MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct);
	
private:
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFMConnection::TError aError);
	
private:
	// the view classes
	CMobblerSettingItemListView* iSettingView;
	CMobblerStatusView* iStatusView;
	CMobblerWebServicesView* iWebServicesView;
	
	// The application engine classes
	CMobblerLastFMConnection* iLastFMConnection;
	CMobblerRadioPlayer* iRadioPlayer;
	CMobblerMusicAppListener* iMusicListener;

	// Gesture observer plugin
	TUid iGesturePluginDtorUid;
	CMobblerGesturesInterface* iGesturePlugin;
	
	// The current track submit and queue count
	TInt iTracksSubmitted;
	TInt iTracksQueued;
	
	// media buttons
	CRemConInterfaceSelector* iInterfaceSelector;
	CRemConCoreApiTarget*     iCoreTarget;
	
	// timers and callbacks for media buttons autorepeat
	CPeriodic* iVolumeUpTimer;
	CPeriodic* iVolumeDownTimer;
	TCallBack iVolumeUpCallBack;
	TCallBack iVolumeDownCallBack;

	CMobblerLastFMConnection::TRadioStation iPreviousRadioStation;
	
	CMobblerString* iPreviousRadioArtist;
	CMobblerString* iPreviousRadioTag;
	CMobblerString* iPreviousRadioPersonal;
	
	CMobblerBitmapCollection* iBitmapCollection;
	
#ifndef __WINS__
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;
	
	CMobblerDownload* iMobblerDownload;

	CMobblerResourceReader* iResourceReader;

	CMobblerSleepTimer* iSleepTimer;
	TTime iTimeToSleep;
	TInt iSleepAction;
	
	CMobblerWebServicesHelper* iWebServicesHelper;
	
	CMobblerFlatDataObserverHelper* iCheckForUpdatesObserver;
	};

#endif // __MOBBLERAPPUI_H__

// End of file
