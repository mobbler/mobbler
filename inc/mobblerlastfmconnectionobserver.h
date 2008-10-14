/*
mobblerlastfmconnectionobserver.h

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

#ifndef __MOBBLERLASTFMCONNECTIONOBSERVER_H__
#define __MOBBLERLASTFMCONNECTIONOBSERVER_H__

#include "mobblerlastfmconnection.h"
#include "mobblerlastfmerror.h"

class CMobblerTrack;

class MMobblerLastFMConnectionObserver
	{
public:
	virtual void HandleConnectCompleteL() = 0;
	virtual void HandleLastFMErrorL(CMobblerLastFMError& aError) = 0;
	virtual void HandleCommsErrorL(const TDesC& aTransaction, const TDesC8& aStatus) = 0;
	
	virtual void HandleTrackNowPlayingL(const CMobblerTrack& aTrack) = 0;
	virtual void HandleTrackQueuedL(const CMobblerTrack& aTrack) = 0;
	virtual void HandleTrackSubmittedL(const CMobblerTrack& aTrack) = 0;
	
	virtual void HandleUpdateResponseL(TVersion aVersion, const TDesC8& aLocation) = 0;
	};
	
#endif
	
