/*
mobblerparser.h

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

#ifndef __MOBBLERPARSER_H__
#define __MOBBLERPARSER_H__

#include <e32base.h>

#include "mobblerlastfmconnection.h"

class CSenXmlReader;
class CSenDomFragment;
class CMobblerRadioPlaylist;

class CMobblerFriendList;
class CMobblerGroupList;
class CMobblerEventList;
class CMobblerPlaylistList;
class CMobblerTagList;
class CMobblerArtistList;
class CMobblerAlbumList;
class CMobblerTrackList;
class CMobblerListItem;
class CMobblerShoutbox;

class CMobblerParser : public CBase
	{
public:
	static CMobblerLastFMError* ParseHandshakeL(const TDesC8& aHandshakeResponse, HBufC8*& aSessionId, HBufC8*& aNowPlayingURL, HBufC8*& aSubmitURL);
	static CMobblerLastFMError* ParseRadioHandshakeL(const TDesC8& aRadioHandshakeResponse, HBufC8*& aRadioSessionID, HBufC8*& aRadioBaseURL, HBufC8*& aRadioBasePath);
	static CMobblerLastFMError* ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey);
	static CMobblerLastFMError* ParseScrobbleResponseL(const TDesC8& aScrobbleResponse);
	
	static CMobblerLastFMError* ParseRadioSelectStationL(const TDesC8& aXML);
	static CMobblerLastFMError* ParseRadioPlaylistL(const TDesC8& aXML, CMobblerRadioPlaylist*& aPlaylist);

	static TInt ParseUpdateResponseL(const TDesC8& aXML, TVersion& aVersion, TDes8& location);
	
	static void ParseFriendListL(const TDesC8& aXML, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseTopArtistsL(const TDesC8& aXML, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecommendedArtistsL(const TDesC8& aXML, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarArtistsL(const TDesC8& aXML, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseEventsL(const TDesC8& aXML, CMobblerEventList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseTopAlbumsL(const TDesC8& aXML, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList);
	
	static void ParseUserTopTracksL(const TDesC8& aXML, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseArtistTopTracksL(const TDesC8& aXML, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseRecentTracksL(const TDesC8& aXML, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseSimilarTracksL(const TDesC8& aXML, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList);
		
	static void ParseTopTagsL(const TDesC8& aXML, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParsePlaylistsL(const TDesC8& aXML, CMobblerPlaylistList& aObserver, RPointerArray<CMobblerListItem>& aList);
	static void ParseShoutboxL(const TDesC8& aXML, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList);
		
private:
	static HBufC8* DecodeURIStringLC(const TDesC8& aString);
	};

#endif // __MOBBLERPARSER_H__
