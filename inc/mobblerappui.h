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
#include <aknprogressdialog.h>

#include "mobblerlastfmconnection.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerwebservicesobserver.h"


class CMobblerSettingItemListView;
class CMobblerMusicAppListener;
class CAknNavigationControlContainer;
class CAknNavigationDecorator;
class CMobblerDownload;
class CMobblerStatusView;
class CMobblerRadioPlayer;
class CMobblerTrack;
class CBrowserLauncher;

class CMobblerAppUi : public CAknViewAppUi,
						public MMobblerLastFMConnectionObserver
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

public: // CEikAppUi
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);

private:
	void HandleStatusPaneSizeChange();
	
	void HandleConnectCompleteL(TInt aError);
	void HandleLastFMErrorL(CMobblerLastFMError& aError);
	void HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus);
	void HandleTrackSubmittedL(const CMobblerTrack& aTrack);
	void HandleTrackQueuedL(const CMobblerTrack& aTrack);
	void HandleTrackNowPlayingL(const CMobblerTrack& aTrack);
	void HandleUpdateResponseL(TVersion aVersion, const TDesC8& aLocation);
	
	void RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioOption);
	
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
	TBool iRadioStartedAtLeastOnce;
	TBool iResumeStationOnConnectCompleteCallback;

	HBufC* iPreviousRadioArtist;
	HBufC* iPreviousRadioTag;
	HBufC* iPreviousRadioUser;
	
#ifndef __WINS__
	CBrowserLauncher* iBrowserLauncher;
#endif

	TBool iForeground;
	
	CMobblerDownload* iMobblerDownload;
	};

#endif // __MOBBLERAPPUI_h__

