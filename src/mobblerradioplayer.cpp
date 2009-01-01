/*
mobblerradioplayer.cpp

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

#include "mobblerappui.h"
#include "mobbleraudiocontrol.h"
#include "mobbleraudiothread.h"
#include "mobblerincomingcallmonitor.h"
#include "mobblerlastfmconnection.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include <mobbler.rsg>

#ifdef __WINS__
const TInt KDefaultVolume(0);
#else
const TInt KDefaultVolume(5);
#endif
const TInt KEqualizerOff(-1);

const TInt KDefaultMaxVolume(10);

CMobblerRadioPlayer* CMobblerRadioPlayer::NewL(CMobblerLastFMConnection& aSubmitter, TTimeIntervalSeconds aPreBufferSize)
	{
	CMobblerRadioPlayer* self = new(ELeave) CMobblerRadioPlayer(aSubmitter, aPreBufferSize);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerRadioPlayer::CMobblerRadioPlayer(CMobblerLastFMConnection& aLastFMConnection, TTimeIntervalSeconds aPreBufferSize)
	:iLastFMConnection(aLastFMConnection), iPreBufferSize(aPreBufferSize), iVolume(KDefaultVolume), iMaxVolume(KDefaultMaxVolume)
	{
	}

void CMobblerRadioPlayer::ConstructL()
	{
	iIncomingCallMonitor = CMobblerIncomingCallMonitor::NewL(*this);
	}

CMobblerRadioPlayer::~CMobblerRadioPlayer()
	{
	delete iPlaylist;
	delete iCurrentAudioControl;
	delete iNextAudioControl;
	
	delete iIncomingCallMonitor;
	}



void CMobblerRadioPlayer::HandleAudioPositionChangeL()
	{
	// if we are finished downloading
	// and only have the prebuffer amout of tim eleft of the track
	// then start downloading th next track
	
	if (iCurrentTrack + 1 < iPlaylist->Count() &&
			!iNextAudioControl &&
			iCurrentAudioControl &&
			iCurrentAudioControl->DownloadComplete() &&
			(( (*iPlaylist)[iCurrentTrack]->TrackLength().Int() - (*iPlaylist)[iCurrentTrack]->PlaybackPosition().Int() ) <= iCurrentAudioControl->PreBufferSize().Int() ))
		{
		// There is another track in the playlist
		// We have not created the next track yet
		// The download on the current track is complete
		// and there is only the length of the pre-buffer to go on the current track 
		// so start downloading the next track now 
		
		iNextAudioControl = CMobblerAudioControl::NewL(*this,  *(*iPlaylist)[iCurrentTrack + 1], iPreBufferSize, iVolume);
		iLastFMConnection.RequestMp3L(*iNextAudioControl, (*iPlaylist)[iCurrentTrack + 1]);
		}
	
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::HandleAudioFinishedL(CMobblerAudioControl* aAudioControl)
	{
	NextTrackL();
	}

TInt CMobblerRadioPlayer::StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	Stop();
	return iLastFMConnection.RadioStartL(aRadioStation, aRadioText);
	}

void CMobblerRadioPlayer::SetPlaylistL(CMobblerRadioPlaylist* aPlaylist)
	{
	iCurrentTrack = 0;
	
	if (aPlaylist->Count() > iCurrentTrack)
		{		
		delete iCurrentAudioControl;
		iCurrentAudioControl = NULL;
		
		if (iNextAudioControl)
			{
			iCurrentAudioControl = iNextAudioControl;
			iNextAudioControl = NULL;
			}
		else
			{
			iCurrentAudioControl = CMobblerAudioControl::NewL(*this, *(*aPlaylist)[iCurrentTrack], iPreBufferSize, iVolume);
			iLastFMConnection.RequestMp3L(*iCurrentAudioControl, (*aPlaylist)[iCurrentTrack]);	
			}
		
		iCurrentAudioControl->SetCurrent();
		
		// We have started playing the track so tell Last.fm
		CMobblerTrack* track = (*aPlaylist)[iCurrentTrack];
		
		if (track->StartTimeUTC() == Time::NullTTime())
			{
			// we haven't set the start time for this track yet
			// so this must be the first time we are writing data
			// to the output stream.  Set the start time now.
			TTime now;
			now.UniversalTime();
			track->SetStartTimeUTC(now);
			}
		
		iLastFMConnection.TrackStartedL(track);
		}
	
	// take ownership of the playlist
	delete iPlaylist;
	iPlaylist = aPlaylist;
	}

void CMobblerRadioPlayer::NextTrackL()
	{	
	DoStop(EFalse);
	
	if (iPlaylist)
		{
		if (iPlaylist->Count() > iCurrentTrack + 1)
			{
			++iCurrentTrack;
			
			delete iCurrentAudioControl;
			iCurrentAudioControl = NULL;
			
			if (iNextAudioControl)
				{
				iCurrentAudioControl = iNextAudioControl;
				iNextAudioControl = NULL;
				}
			else
				{
				iCurrentAudioControl = CMobblerAudioControl::NewL(*this, *(*iPlaylist)[iCurrentTrack], iPreBufferSize, iVolume);
				iLastFMConnection.RequestMp3L(*iCurrentAudioControl, (*iPlaylist)[iCurrentTrack]);
				}
			
			iCurrentAudioControl->SetCurrent();
			
			// We have started playing the track so tell Last.fm
			CMobblerTrack* track = (*iPlaylist)[iCurrentTrack];
			
			if (track->StartTimeUTC() == Time::NullTTime())
				{
				// we haven't set the start time for this track yet
				// so this must be the first time we are writing data
				// to the output stream.  Set the start time now.
				TTime now;
				now.UniversalTime();
				track->SetStartTimeUTC(now);
				}
			
			iLastFMConnection.TrackStartedL(track);
			}
		else
			{
			//  There is a playlist so try and fetch another
			delete iPlaylist;
			iPlaylist = NULL;
			iLastFMConnection.RequestPlaylistL();
			}
		}
	}

void CMobblerRadioPlayer::VolumeUp()
	{
	TInt volume = Volume();
	TInt maxVolume = MaxVolume();
	iVolume = Min(volume + (maxVolume / 10), maxVolume);
	
	if (iCurrentAudioControl) iCurrentAudioControl->SetVolume(iVolume);
	if (iNextAudioControl) iNextAudioControl->SetVolume(iVolume);
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerRadioPlayer::VolumeDown()
	{
	TInt volume = Volume();
	TInt maxVolume = MaxVolume();
	iVolume = Max(volume - (maxVolume / 10), 0);
		
	if (iCurrentAudioControl) iCurrentAudioControl->SetVolume(iVolume);
	if (iNextAudioControl) iNextAudioControl->SetVolume(iVolume);
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

TInt CMobblerRadioPlayer::Volume() const
	{
	TInt volume(iVolume);
	
	if (iCurrentAudioControl)
		{
		volume = iCurrentAudioControl->Volume();
		}
	
	return volume;
	}

TInt CMobblerRadioPlayer::MaxVolume() const
	{
	TInt maxVolume(iMaxVolume);
	
	if (iCurrentAudioControl)
		{
		if (iMaxVolume != iCurrentAudioControl->MaxVolume())
			{
			// The max audio volume has changed
			// so correction the volume for this
			iCurrentAudioControl->SetVolume((iCurrentAudioControl->Volume() * iCurrentAudioControl->MaxVolume()) / iMaxVolume);
			}
		
		maxVolume = iCurrentAudioControl->MaxVolume();
		}
	
	return maxVolume;
	}

const CMobblerString& CMobblerRadioPlayer::Station() const
	{
	return iPlaylist->Name();
	}

void CMobblerRadioPlayer::SetPreBufferSize(TTimeIntervalSeconds aPreBufferSize)
	{
	iPreBufferSize = aPreBufferSize;
	if (iCurrentAudioControl)
		{
		iCurrentAudioControl->SetPreBufferSize(aPreBufferSize);
		}
	}

void CMobblerRadioPlayer::Stop()
	{
	DoStop(ETrue);
	}

void CMobblerRadioPlayer::DoStop(TBool aDeleteNextTrack)
	{
	// Try to submit the last played track
	SubmitCurrentTrackL();
	
	if (aDeleteNextTrack)
		{
		// we want to also delete the next track if it has started to download
		
		// stop all radio downloads
		iLastFMConnection.RadioStop();
		
		if (iNextAudioControl)
			{
			// if there was a next track then interment the current track
			// becasue we can't download the same track twice
			++iCurrentTrack;
			}
		
		delete iNextAudioControl;
		iNextAudioControl = NULL;
		}
	else
		{
		if (!iNextAudioControl)
			{
			// we don't want to delete the next track, but
			// it hasn't started downloading let so stop
			// the current one
			iLastFMConnection.RadioStop();
			}
		}
	
	delete iCurrentAudioControl;
	iCurrentAudioControl = NULL;
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

CMobblerTrack* CMobblerRadioPlayer::CurrentTrack()
	{
	if (iCurrentAudioControl && (!iCurrentAudioControl->DownloadComplete() ||
			iCurrentAudioControl->Playing()) && (iPlaylist && (iPlaylist->Count() > iCurrentTrack)) )
		{
		return (*iPlaylist)[iCurrentTrack];
		}
	
	return NULL;
	}

void CMobblerRadioPlayer::SubmitCurrentTrackL()
	{
	if (iCurrentAudioControl && (!iCurrentAudioControl->DownloadComplete() || iCurrentAudioControl->Playing()))
		{
		iLastFMConnection.TrackStoppedL();
		}
	}

void CMobblerRadioPlayer::HandleIncomingCallL(TPSTelephonyCallState aPSTelephonyCallState)
	{
	switch (aPSTelephonyCallState)
		{
		case EPSTelephonyCallStateAlerting:
		case EPSTelephonyCallStateRinging:
		case EPSTelephonyCallStateDialling:
		case EPSTelephonyCallStateAnswering:
		case EPSTelephonyCallStateConnected:
			// There was an incoming call so stop playing the radio
			Stop();
			break;
		case EPSTelephonyCallStateUninitialized:
		case EPSTelephonyCallStateNone:
		case EPSTelephonyCallStateDisconnecting:
		case EPSTelephonyCallStateHold:
		default:
			// The normal S60 behaviour seems to be to pause and then restart
			// music when the call finishes, but we have some problems with this
			break;
		}
	}

void CMobblerRadioPlayer::SetEqualizer(TInt aIndex)
	{
	if (iCurrentAudioControl) iCurrentAudioControl->SetEqualizerIndex(aIndex);
	if (iNextAudioControl) iNextAudioControl->SetEqualizerIndex(aIndex);
	}

TBool CMobblerRadioPlayer::HasPlaylist() const
	{
	if (iPlaylist)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

// End of file
