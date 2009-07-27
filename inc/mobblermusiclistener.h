/*
mobblermusiclistener.h

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

#ifndef __MOBBLERMUSICLISTENER_H__
#define __MOBBLERMUSICLISTENER_H__

#include <e32base.h>

#include "..\contentlisting\inc\mobblercontentlistingobserver.h"
#include "mobblerlastfmconnection.h"
#include "mobblermusicapp.h"

class CMobblerNowPlayingCallback;
class CMobblerRadioPlayer;
class CMobblerContentListingInterface;

class MMobblerMusicAppListenerObserver
	{
public:
	virtual void HandleMusicAppChangeL() = 0;
	};

class CMobblerMusicAppListener : public CBase, public MMobblerMusicAppObserver,
								 public MMobblerContentListingObserver
	{
public:
	static CMobblerMusicAppListener* NewL(CMobblerLastFMConnection& aSubmitter);
	~CMobblerMusicAppListener();
	
	void AddObserverL(MMobblerMusicAppListenerObserver* aObserver);
	void RemoveObserver(MMobblerMusicAppListenerObserver* aObserver);
	
	CMobblerTrack* CurrentTrack();
	void NowPlayingL();
	const TDesC& MusicAppNameL() const;
	TBool IsPlaying() const;
	
private:
	CMobblerMusicAppListener(CMobblerLastFMConnection& aSubmitter);
	void ConstructL();
	
	void NotifyChangeL();
	
private: 
	void HandleTrackChangeL(const TDesC& aTrack);
	void HandleMusicStateChangeL(TInt aState);
	
	void ScheduleNowPlayingL();
	
private: // from MMobblerMusicAppObserver
	void PlayerStateChangedL(TMPlayerRemoteControlState aState);
	void TrackInfoChangedL(const TDesC& aTitle, const TDesC& aArtist);
	void CommandReceivedL(TMPlayerRemoteControlCommands aCommand);
	void PlayerPositionL(TTimeIntervalSeconds aPlayerPosition);

private: // from MMobblerContentListingObserver
	void SetAlbumL(const TDesC& aAlbum);
	void SetTrackNumber(const TInt aTrackNumber);
	void SetPathL(const TDesC& aPath);
    
private:
	// Music app observer
	RPointerArray<CMobblerMusicApp> iMobblerMusicApps;
	RArray<TUid> iDtorIdKeys;

	CMobblerLastFMConnection& iLastFMConnection;
	
	CMobblerNowPlayingCallback* iNowPlayingCallback;
	
	CMobblerTrack* iCurrentTrack;
	
	mutable TBuf<255> iMusicAppName;

	CMobblerContentListingInterface* iMobblerContentListing;
	TUid iDtorIdKey;

	TMPlayerRemoteControlState iMusicPlayerState;
	
	RPointerArray<MMobblerMusicAppListenerObserver> iObservers;
	};

#endif // __MOBBLERMUSICLISTENER_H__

// End of file
