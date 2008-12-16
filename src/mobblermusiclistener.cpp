/*
mobblermusiclistener.cpp

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

#include "mobblermusiclistener.h"
#include "mobblernowplayingcallback.h"
#include "mobblertrack.h"
#include "mobblerappui.h"

#include <mplayerremotecontrol.h>
#include <ecom/ecom.h>

#include <utf.h>

const TUid KMobblerMusicAppImplUid = {0xA0007CAA};

CMobblerMusicAppListener* CMobblerMusicAppListener::NewL(CMobblerLastFMConnection& aSubmitter)
	{
	CMobblerMusicAppListener* self = new(ELeave) CMobblerMusicAppListener(aSubmitter);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMusicAppListener::CMobblerMusicAppListener(CMobblerLastFMConnection& aSubmitter)
	:iLastFMConnection(aSubmitter)
	{
	}

void CMobblerMusicAppListener::ConstructL()
	{
	RImplInfoPtrArray implInfoPtrArray;
	CleanupClosePushL(implInfoPtrArray);
	
	REComSession::ListImplementationsL(KMobblerMusicAppImplUid, implInfoPtrArray);
	
	const TInt KImplCount(implInfoPtrArray.Count());
	for (TInt i(0) ; i < KImplCount ; ++i)
		{
		TUid dtorIdKey;
		CMobblerMusicApp* mobblerMusicApp(NULL);
		TRAPD(error, mobblerMusicApp = static_cast<CMobblerMusicApp*>(REComSession::CreateImplementationL(implInfoPtrArray[i]->ImplementationUid(), dtorIdKey, dynamic_cast<MMobblerMusicAppObserver*>(this))));
		
		if (error == KErrNone)
			{
			iMobblerMusicApps.AppendL(mobblerMusicApp);
			iDtorIdKeys.AppendL(dtorIdKey);
			}
		else
			{
			REComSession::DestroyedImplementation(dtorIdKey);
			}
		
		delete implInfoPtrArray[i];
		}
	
	CleanupStack::PopAndDestroy(&implInfoPtrArray);
	
	// check if there is a song playing whe mobbler is started
	ScheduleNowPlayingL();
	}

CMobblerMusicAppListener::~CMobblerMusicAppListener()
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		}
	
	iMobblerMusicApps.ResetAndDestroy();
	
	const TInt KDtorIdKeysCount(iDtorIdKeys.Count());
	for (TInt i(0) ; i < KDtorIdKeysCount ; ++i)
		{
		REComSession::DestroyedImplementation(iDtorIdKeys[i]);
		}
	
	iDtorIdKeys.Close();
	
	// not supposed to call this in app code, but there seems to be problems reinstalling without it
	REComSession::FinalClose();
	
	delete iNowPlayingCallback;
	}

CMobblerTrack* CMobblerMusicAppListener::CurrentTrack()
	{
	return iCurrentTrack;
	}

void CMobblerMusicAppListener::HandleTrackChangeL(const TDesC& /*aTrack*/)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		iCurrentTrack = NULL;
		}
	iLastFMConnection.TrackStoppedL();
	ScheduleNowPlayingL();
	}

void CMobblerMusicAppListener::HandleMusicStateChangeL(TInt aState)
	{
	if (aState == EMPlayerRCtrlPlaying)
		{
		ScheduleNowPlayingL();
		}
	else
		{
		if (iCurrentTrack)
			{
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerMusicAppListener::ScheduleNowPlayingL()
	{
	if (iNowPlayingCallback && iNowPlayingCallback->IsActive())
		{
		// We are already waiting for a callback so let that happen
		}
	else
		{
		delete iNowPlayingCallback;
		iNowPlayingCallback = CMobblerNowPlayingCallback::NewL(*this);	
		}
	}

const TDesC& CMobblerMusicAppListener::MusicAppNameL() const
	{
	iMusicAppName.Zero(); 
	
	const TInt KMusicAppCount(iMobblerMusicApps.Count());
	for (TInt i(0) ; i < KMusicAppCount ; ++i)
		{
		if (iMobblerMusicApps[i]->PlayerState() == EMPlayerRCtrlPlaying)
			{
			HBufC* appName = iMobblerMusicApps[i]->NameL();
			iMusicAppName.Copy(*appName);
			delete appName;
			break;
			}
		}
	
	return iMusicAppName;
	}

void CMobblerMusicAppListener::NowPlayingL()
	{
	if ( iCurrentTrack )
		{
		// We are currently listenning to a track so just send the old one again
		iLastFMConnection.TrackStartedL(iCurrentTrack);
		}
	else
		{
		// We are not listening to a track so fetch the details
		// and try to submit it as 'now playing'
		TBuf<255> trackArtist;
		TBuf<255> trackTitle;
		
		TTimeIntervalSeconds trackLength(0);
		
		TTime startTimeUTC;
		startTimeUTC.UniversalTime();
		startTimeUTC -= KNowPlayingCallbackDelay;
		
		// find the first music app observer that is playing a track
		TInt musicAppIndex(KErrNotFound);
		const TInt KMusicAppCount(iMobblerMusicApps.Count());
		for (TInt i(0) ; i < KMusicAppCount ; ++i)
			{
			if (iMobblerMusicApps[i]->PlayerState() == EMPlayerRCtrlPlaying)
				{
				musicAppIndex = i;
				break;
				}
			}
		
		if (musicAppIndex != KErrNotFound)
			{
			trackArtist.Copy(iMobblerMusicApps[musicAppIndex]->Artist());
			trackTitle.Copy(iMobblerMusicApps[musicAppIndex]->Title());
			trackLength = iMobblerMusicApps[musicAppIndex]->Duration();
			
			if (trackLength.Int() >= 30 && trackArtist.Length() > 0)
				{
				HBufC8* artist = CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackArtist);
				CleanupStack::PushL(artist);
				HBufC8* title = CnvUtfConverter::ConvertFromUnicodeToUtf8L(trackTitle);
				CleanupStack::PushL(title);
				
				if (iCurrentTrack)
					{
					iCurrentTrack->Release();
					}
				
				iCurrentTrack = CMobblerTrack::NewL(*artist, *title, KNullDesC8, KNullDesC8, KNullDesC8, trackLength, KNullDesC8);
				CleanupStack::PopAndDestroy(2, artist);
				iCurrentTrack->SetStartTimeUTC(startTimeUTC);
				iLastFMConnection.TrackStartedL(iCurrentTrack);
				}
			}
		}
	}

void CMobblerMusicAppListener::PlayerStateChangedL(TMPlayerRemoteControlState aState)
	{
	if (aState == EMPlayerRCtrlPlaying)
		{
		ScheduleNowPlayingL();
		}
	else
		{
		if (iCurrentTrack)
			{
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerMusicAppListener::TrackInfoChangedL(const TDesC& /*aTitle*/, const TDesC& /*aArtist*/)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		iCurrentTrack = NULL;
		}
	iLastFMConnection.TrackStoppedL();
	ScheduleNowPlayingL();
	}

void CMobblerMusicAppListener::CommandReceivedL(TMPlayerRemoteControlCommands aCommand)
	{
	if (aCommand == EMPlayerRCtrlCmdPlay)
		{
		ScheduleNowPlayingL();
		}
	else if (aCommand != EMPlayerRCtrlCmdBack 
				&& aCommand != EMPlayerRCtrlCmdNoCommand)
		{
		if (iCurrentTrack)
			{
			iCurrentTrack->Release();
			iCurrentTrack = NULL;
			}
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerMusicAppListener::PlayerPositionL(TTimeIntervalSeconds aPlayerPosition)
	{
	if (iCurrentTrack)
		{
		iCurrentTrack->SetPlaybackPosition(aPlayerPosition);
		static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
		}
	}
	
