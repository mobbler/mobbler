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
#include <aknserverapp.h>	// MAknServerAppExitObserver
#include <remconcoreapitargetobserver.h>    // link against RemConCoreApi.lib
#include <remconcoreapitarget.h>            // and
#include <remconinterfaceselector.h>        // RemConInterfaceBase.lib

#include "mobblerdataobserver.h"
#include "mobblerdownload.h"
#include "mobblergesturesinterface.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblersleeptimer.h"

_LIT(KFormatTime, "%F%D %N %-B%J%:1%T%+B"); // 21 March 11:20 am

#ifdef BETA_BUILD
const TInt KUpdateIntervalDays(1);
#else
const TInt KUpdateIntervalDays(7);
#endif

const TInt KBuildNumber(6182);

#ifdef __SYMBIAN_SIGNED__
const TVersion KVersion(1, 0, KBuildNumber);
const TInt KMobblerAppUid = 0x2002655A;
const TInt KMobblerSettingsViewUid = 0x2002655C;
const TInt KMobblerStatusViewUid = 0x2002655B;
const TInt KMobblerWebServicesViewUid = 0x2002656B;
#else
const TVersion KVersion(0, 6, KBuildNumber);
const TInt KMobblerAppUid = 0xA0007648;
const TInt KMobblerSettingsViewUid = 0xA0007CA9;
const TInt KMobblerStatusViewUid = 0xA0007CA8;
const TInt KMobblerWebServicesViewUid = 0xA000B6C3;
#endif

class CBrowserLauncher;
class CMobblerBitmapCollection;
class CMobblerDestinationsInterface;
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
						public MMobblerLastFmConnectionObserver,
						public MMobblerDownloadObserver,
						public MMobblerSleepTimerNotify,
						public MRemConCoreApiTargetObserver,
						public MMobblerFlatDataObserverHelper,
						public MMobblerGestures,
						public MAknServerAppExitObserver
	{
public:
	enum TDownloadAlbumArt
		{
		ENever,
		EOnlyRadio,
		EAlwaysWhenOnline
		};
	
private:
	enum TPlusOptions
		{
		EPlusOptionVisitLastFm,
		EPlusOptionShareTrack,
		EPlusOptionShareArtist,
		EPlusOptionPlaylistAddTrack,
		EPlusOptionSimilarArtists,
		EPlusOptionSimilarTracks,
		EPlusOptionEvents,
		EPlusOptionArtistShoutbox,
		EPlusOptionTopAlbums,
		EPlusOptionTopTracks,
		EPlusOptionTopTags
		};

public:
	void ConstructL();
	CMobblerAppUi();
	~CMobblerAppUi();
	
	const CMobblerTrack* CurrentTrack() const;
	CMobblerTrack* CurrentTrack();
	
	CMobblerRadioPlayer& RadioPlayer() const;
	CMobblerLastFmConnection& LastFmConnection() const;
	CMobblerMusicAppListener& MusicListener() const;
	CMobblerBitmapCollection& BitmapCollection() const;
	CMobblerDestinationsInterface* Destinations() const;
	CBrowserLauncher* BrowserLauncher() const;
	
	CMobblerSettingItemListView& SettingView() const;
	const TDesC& MusicAppNameL() const;
	
	void RadioStartL(TInt aRadioStation, 
					 const CMobblerString* aRadioOption, 
					 TBool aSaveStations = ETrue);
	
	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetIapIDL(TUint32 aIapID);
	void SetBufferSize(TTimeIntervalSeconds aBufferSize);
	void SetAccelerometerGesturesL(TBool aAccelerometerGestures);
	TBool AccelerometerGesturesAvailable() const;
	void SetSleepTimerL(const TInt aMinutes);
	void SetAlarmTimerL(const TTime aTime);
	void SetBitRateL(TInt aBitRate);
	
	TInt Scrobbled() const;
	TInt Queued() const;
	
	void StatusDrawDeferred();
	void StatusDrawNow();
	
	CMobblerLastFmConnection::TMode Mode() const;
	CMobblerLastFmConnection::TState State() const;
	TBool ScrobblingOn() const;

	TBool RadioResumable() const;
	TBool Foreground() const;
	TBool Backlight() const;
	TInt ScrobblePercent() const;
	TInt DownloadAlbumArt() const;
	void TrackStoppedL();

	CMobblerResourceReader& CMobblerAppUi::ResourceReader() const;

	TBool SleepTimerActive() const { return iSleepTimer->IsActive(); }
	TBool AlarmActive() const { return iAlarmTimer->IsActive(); }
	void RemoveSleepTimerL();
	void RemoveAlarmL();

public: // CEikAppUi
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);

private:
	void HandleInstallStartedL();
	
private:
	void HandleStatusPaneSizeChange();
	
	void HandleConnectCompleteL(TInt aError);
	void HandleLastFmErrorL(CMobblerLastFmError& aError);
	void HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus);
	void HandleTrackSubmittedL(const CMobblerTrack& aTrack);
	void HandleTrackQueuedL(const CMobblerTrack& aTrack);
	void HandleTrackDequeued(const CMobblerTrack& aTrack);
	void HandleTrackNowPlayingL(const CMobblerTrack& aTrack);
	
	TBool GoOnlineL();
	
	void LoadRadioStationsL();
	void SaveRadioStationsL();
	void SleepL();
	TBool RadioStartableL() const;
	
	void LaunchFileEmbeddedL(const TDesC& aFilename);
	void GoToLastFmL(TInt aCommand);

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
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);
	
private: // from MAknServerAppExitObserver
	void HandleServerAppExit(TInt aReason);
 
private: // from MAknWsEventObserver
	void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);
 
private:
	// the view classes
	CMobblerSettingItemListView* iSettingView;
	CMobblerStatusView* iStatusView;
	CMobblerWebServicesView* iWebServicesView;
	
	// The application engine classes
	CMobblerLastFmConnection* iLastFmConnection;
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

	TInt iPreviousRadioStation;
	
	CMobblerString* iPreviousRadioArtist;
	CMobblerString* iPreviousRadioTag;
	CMobblerString* iPreviousRadioUser;
	CMobblerString* iPreviousRadioPlaylistId;
	
	CMobblerBitmapCollection* iBitmapCollection;
	
#ifndef __WINS__
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;
	
	CMobblerDownload* iMobblerDownload;

	CMobblerResourceReader* iResourceReader;

	CMobblerSleepTimer* iSleepTimer;
	TTime iTimeToSleep;
	TBool iSleepAfterTrackStopped;
	CMobblerSleepTimer* iAlarmTimer;
	
	CMobblerWebServicesHelper* iWebServicesHelper;
	
	CMobblerFlatDataObserverHelper* iCheckForUpdatesObserver;
	
	CMobblerDestinationsInterface* iDestinations;
	TUid iDestinationsDtorUid;
	};

#endif // __MOBBLERAPPUI_H__

// End of file
