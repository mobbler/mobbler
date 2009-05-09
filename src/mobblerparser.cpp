/*
mobblerparser.cpp

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

#include <mobbler_strings.rsg>
#include <sendomfragment.h>
#include <sennamespace.h> 
#include <senxmlutils.h> 

#include "mobbleralbumlist.h"
#include "mobblerappui.h"
#include "mobblerartistlist.h"
#include "mobblereventlist.h"
#include "mobblerfriendlist.h"
#include "mobblerlogging.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerplaylistlist.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerstring.h"
#include "mobblertaglist.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"

_LIT8(KElementAlbum, "album");
_LIT8(KElementCreator, "creator");
_LIT8(KElementDuration, "duration");
_LIT8(KElementError, "error");
_LIT8(KElementId, "id");
_LIT8(KElementImage, "image");
_LIT8(KElementKey, "key");
_LIT8(KElementLink, "link");
_LIT8(KElementLocation, "location");
_LIT8(KElementRadioAuth, "trackauth");
_LIT8(KElementSession, "session");
_LIT8(KElementStatus, "status");
_LIT8(KElementTitle, "title");
//_LIT8(KElementTrack, "track");
_LIT8(KElementTrackList, "trackList");

_LIT8(KUpdateVersionMajor,		"major");
_LIT8(KUpdateVersionMinor,		"minor");
_LIT8(KUpdateVersionBuild,		"build");
_LIT8(KUpdateLocation,			"location");

_LIT8(KNamespace, "http://www.audioscrobbler.net/dtd/xspf-lastfm");

/*
OK
    This indicates that the handshake was successful. Three lines will follow the OK response:

       1. Session ID
       2. Now-Playing URL
       3. Submission URL

BANNED
    This indicates that this client version has been banned from the server. This usually happens if the client is violating the protocol in a destructive way. Users should be asked to upgrade their client application.
BADAUTH
    This indicates that the authentication details provided were incorrect. The client should not retry the handshake until the user has changed their details.
BADTIME
    The timestamp provided was not close enough to the current time. The system clock must be corrected before re-handshaking.
FAILED <reason>
    This indicates a temporary server failure. The reason indicates the cause of the failure. The client should proceed as directed in the Hard Failures section.
All other responses should be treated as a hard failure.
    An error may be reported to the user, but as with other messages this should be kept to a minimum. 
*/
CMobblerLastFMError* CMobblerParser::ParseHandshakeL(const TDesC8& aHandshakeResponse, HBufC8*& aSessionId, HBufC8*& aNowPlayingUrl, HBufC8*& aSubmitUrl)
	{
	DUMPDATA(aHandshakeResponse, _L("scrobblehandshake.txt"));

	CMobblerLastFMError* error(NULL);
	
	if (aHandshakeResponse.MatchF(_L8("OK*")) == 0)
		{
		TInt position(aHandshakeResponse.Find(_L8("\n")));
		
		// get the session ID
		TPtrC8 last3Lines(aHandshakeResponse.Mid(position + 1, aHandshakeResponse.Length() - (position + 1)));
		position = last3Lines.Find(_L8("\n"));
		delete aSessionId;
		aSessionId = last3Lines.Mid(0, position).AllocL();
		
		// get the now playing URL
		TPtrC8 last2Lines(last3Lines.Mid(position + 1, last3Lines.Length() - (position + 1)));
		position = last2Lines.Find(_L8("\n"));
		delete aNowPlayingUrl;
		aNowPlayingUrl = last2Lines.Mid(0, position).AllocL();
		
		// get the submit URL
		TPtrC8 last1Lines(last2Lines.Mid(position + 1, last2Lines.Length() - (position + 1)));
		position = last1Lines.Find(_L8("\n"));
		delete aSubmitUrl;
		aSubmitUrl = last1Lines.Mid(0, position).AllocL();
		}
	else if (aHandshakeResponse.MatchF(_L8("BANNED*")) == 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BANNED), CMobblerLastFMError::EBanned);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADAUTH*")) == 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_AUTH), CMobblerLastFMError::EBadAuth);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADTIME*")) == 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_TIME), CMobblerLastFMError::EBadTime);
		}
	else
		{
		error = CMobblerLastFMError::NewL(aHandshakeResponse, CMobblerLastFMError::EOther);
		}
	
	return error;
	}

/*
session
stream_url=http://87.117.229.85:80/last.mp3?Session=ae1eb54a11615e605d61d6e83dde71bc
subscriber=0
framehack=0
base_url=ws.audioscrobbler.com
base_path=/radio
info_message=
fingerprint_upload_url=http://ws.audioscrobbler.com/fingerprint/upload.php
*/
CMobblerLastFMError* CMobblerParser::ParseRadioHandshakeL(const TDesC8& aRadioHandshakeResponse, HBufC8*& aRadioSessionID, HBufC8*& aRadioBaseUrl, HBufC8*& aRadioBasePath)
	{
	DUMPDATA(aRadioHandshakeResponse, _L("radiohandshakeresponse.txt"));
	
	CMobblerLastFMError* error(NULL);
	
	if (aRadioHandshakeResponse.MatchF(_L8("session*")) == 0)
		{
		if (aRadioHandshakeResponse.Find(_L8("session=FAILED")) == 0)
			{
			error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_AUTH), CMobblerLastFMError::EBadAuth);
			}
		else
			{
			RDesReadStream readStream(aRadioHandshakeResponse);
			CleanupClosePushL(readStream);
			
			const TInt KLines(7);
			
			for (TInt i(0); i < KLines; ++i)
				{
				_LIT(KDelimeter, "\n");
				
				TBuf8<255> line;
				readStream.ReadL(line, TChar(KDelimeter()[0]));
				
				_LIT8(KEquals, "=");
				TInt equalPosition(KErrNotFound);
				
				equalPosition = line.Find(KEquals);
				
				if (i == 0)
					{
					//session id
					delete aRadioSessionID;
					aRadioSessionID = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 1)
					{
					// stream URL
					//delete aRadioStreamUrl;
					//aRadioStreamUrl = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 4)
					{
					//base URL
					delete aRadioBaseUrl;
					aRadioBaseUrl = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 5)
					{
					//base path
					delete aRadioBasePath;
					aRadioBasePath = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				}
			
			CleanupStack::PopAndDestroy(&readStream);
			}
		}
	else
		{
		error = CMobblerLastFMError::NewL(aRadioHandshakeResponse, CMobblerLastFMError::EOther);
		}

	return error;	
	}

CMobblerLastFMError* CMobblerParser::ParseScrobbleResponseL(const TDesC8& aScrobbleResponse)
	{
	DUMPDATA(aScrobbleResponse, _L("scrobbleresponse.txt"));
	
	CMobblerLastFMError* error(NULL);
	
	if (aScrobbleResponse.Compare(_L8("OK\n")) == 0)
		{
		// do nothing
		}
	else if (aScrobbleResponse.Compare(_L8("BADSESSION\n")) == 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_SESSION), CMobblerLastFMError::EBadSession);
		}
	else
		{
		error = CMobblerLastFMError::NewL(aScrobbleResponse, CMobblerLastFMError::EOther);
		}
	
	return error;
	}

HBufC8* CMobblerParser::DecodeURIStringLC(const TDesC8& aString)
	{
	HBufC8* result(aString.AllocLC());
	
	_LIT8(KPlus, "+");
	_LIT8(KSpace, " ");
	_LIT8(KPercent, "%");
	
	TInt pos(result->Find(KPlus));
	while (pos != KErrNotFound)
		{
		// replace the plus with a space
		result->Des().Delete(pos, 1);
		result->Des().Insert(pos, KSpace);
		
		// try to find the next one
		pos = result->Find(KPlus);
		}
	
	pos = result->Find(KPercent);
	while (pos != KErrNotFound)
		{
		// get the two numbers after the percent
		TLex8 lex(result->Mid(pos + 1, 2));
		
		TUint8 value;
		if (lex.Val(value, EHex) == KErrNone)
			{
			TBuf8<1> replaceChar;
			replaceChar.Append(value);
			
			result->Des().Delete(pos, 3);
			result->Des().Insert(pos, replaceChar);
			
			// try to find the next one
			pos = result->Find(KPercent());
			}
		else
			{
			// There was an error converting the number in to 
			// a character so leave the string half done
			break;
			}
		}
	
	result->Des().Trim();
	
	return result;
	}


CMobblerLastFMError* CMobblerParser::ParseRadioPlaylistL(const TDesC8& aXml, CMobblerRadioPlaylist*& aPlaylist)
	{
	DUMPDATA(aXml, _L("playlist.xml"));
	
	CMobblerLastFMError* error(NULL);
	
	CMobblerRadioPlaylist* playlist(CMobblerRadioPlaylist::NewL());
	CleanupStack::PushL(playlist);
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	HBufC8* radioName(DecodeURIStringLC(domFragment->AsElement().Element(KElementTitle)->Content()));
	playlist->SetNameL(*radioName);
	CleanupStack::PopAndDestroy();
	
	HBufC8* skipsText(DecodeURIStringLC(domFragment->AsElement().Element(KElementLink)->Content()));
	TLex8 lex8(*skipsText);
	TInt skips(0);
	lex8.Val(skips);
	playlist->SetSkips(skips);
	CleanupStack::PopAndDestroy(skipsText);
	
	RPointerArray<CSenElement>& tracks = domFragment->AsElement().Element(KElementTrackList)->ElementsL();
	
	const TInt KTrackCount(tracks.Count());
	for (TInt i(0); i < KTrackCount; ++i)
		{
		// get the duration as a number
		TLex8 lex(tracks[i]->Element(KElementDuration)->Content());
		TInt durationMilliSeconds;
		lex.Val(durationMilliSeconds);
		TTimeIntervalSeconds durationSeconds(durationMilliSeconds / 1000);
		
		TPtrC8 creator(tracks[i]->Element(KElementCreator)->Content());
		HBufC8* creatorBuf(HBufC8::NewLC(creator.Length()));
		SenXmlUtils::DecodeHttpCharactersL(creator, creatorBuf);
		TPtrC8 title(tracks[i]->Element(KElementTitle)->Content());
		HBufC8* titleBuf(HBufC8::NewLC(title.Length()));
		SenXmlUtils::DecodeHttpCharactersL(title, titleBuf);
		TPtrC8 album(tracks[i]->Element(KElementAlbum)->Content());
		HBufC8* albumBuf(HBufC8::NewLC(album.Length()));
		SenXmlUtils::DecodeHttpCharactersL(album, albumBuf);
/*		RPointerArray<CSenElement>& Elems = tracks[i]->ElementsL();
		HBufC8* albumIDBuf(NULL);
		for (TInt x = 0; x < Elems.Count(); x++)
			{
			if (Elems[x]->LocalName() == _L8("albumId"))
				{
				TPtrC8 albumID(Elems[x]->Content());
				albumIDBuf = HBufC8::NewLC(albumID.Length());
				SenXmlUtils::DecodeHttpCharactersL(albumID, albumIDBuf);
				break;
				}
			}*/
		TPtrC8 image(tracks[i]->Element(KElementImage)->Content());
		TPtrC8 location(tracks[i]->Element(KElementLocation)->Content());
		TPtrC8 radioAuth(tracks[i]->Element(KNamespace, KElementRadioAuth)->Content());
		TPtrC8 musicBrainzId(tracks[i]->Element(KElementId)->Content());
		
		CMobblerTrack* track(CMobblerTrack::NewL(*creatorBuf, *titleBuf, *albumBuf, /**albumIDBuf,*/ musicBrainzId, image, location, durationSeconds, radioAuth));
		CleanupStack::PushL(track);
		playlist->AppendTrackL(track);
		CleanupStack::Pop(track);
		
		if (track->Album().String().Length() == 0)
			{
			// We do this so that the track knows for sure
			// that there is no album name and will use the
			// album art from the playlist
			track->SetAlbumL(KNullDesC);
			}
		
		CleanupStack::PopAndDestroy(3, creatorBuf);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	if (playlist->Count() == 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_NO_TRACKS), CMobblerLastFMError::ENoTracks);
		CleanupStack::PopAndDestroy(playlist);
		}
	else
		{
		CleanupStack::Pop(playlist);
		aPlaylist = playlist;
		}
	
	return error;
	}

CMobblerLastFMError* CMobblerParser::ParseRadioSelectStationL(const TDesC8& aXml)
	{
	DUMPDATA(aXml, _L("radioselectresponse.xml"));
	
	CMobblerLastFMError* error(NULL);
	
	if (aXml.Find(_L8("response=OK")) != 0)
		{
		error = CMobblerLastFMError::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_NOTE_BAD_STATION), CMobblerLastFMError::EBadStation);
		}
	
	return error;
	}

CMobblerLastFMError* CMobblerParser::ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey)
	{
	DUMPDATA(aWebServicesHandshakeResponse, _L("webserviceshandshakeresponse.xml"));
	
	CMobblerLastFMError* error(NULL);
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aWebServicesHandshakeResponse);
	
	// get the error code
	const TDesC8* statusText(domFragment->AsElement().AttrValue(KElementStatus));
		
	if (statusText && (statusText->CompareF(_L8("ok")) == 0 ) )
		{
		aWebServicesSessionKey = domFragment->AsElement().Element(KElementSession)->Element(KElementKey)->Content().AllocL();
		}
	else
		{
		error = CMobblerLastFMError::NewL(domFragment->AsElement().Element(KElementError)->Content(), CMobblerLastFMError::EWebServices);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	return error;
	}

#ifdef BETA_BUILD
CMobblerLastFMError* CMobblerParser::ParseBetaTestersHandshakeL(const TDesC8& aHandshakeResponse, const TDesC8& aUsername, TBool& aIsBetaTester)
	{
	CMobblerLastFMError* error(NULL);
	
	// create the XML reader and DOM fragement and associate them with each other 
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the XML into the DOM fragment
	xmlReader->ParseL(aHandshakeResponse);
	
	RPointerArray<CSenElement>& testers = domFragment->AsElement().ElementsL();
	
	aIsBetaTester = EFalse;
	
	const TInt KTesterCount(testers.Count());
	for (TInt i(0); i < KTesterCount ; ++i)
		{
		if (testers[i]->Content().CompareF(aUsername) == 0)
			{
			aIsBetaTester = ETrue;
			break;
			}
		}
	
	if (!aIsBetaTester)
		{
		error = CMobblerLastFMError::NewL(_L8("Sorry. You're not registered to use this private beta version. Please visit http://code.google.com/p/mobbler"), CMobblerLastFMError::EOther);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	return error;
	}
#endif
	
void CMobblerParser::ParseSearchTrackL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
    {
    DUMPDATA(aXml, _L("searchtrack.xml"));
    
    // Create the XML reader and DOM fragement and associate them with each other
    CSenXmlReader* xmlReader(CSenXmlReader::NewL());
    CleanupStack::PushL(xmlReader);
    CSenDomFragment* domFragment(CSenDomFragment::NewL());
    CleanupStack::PushL(domFragment);
    xmlReader->SetContentHandler(*domFragment);
    domFragment->SetReader(*xmlReader);
    
    // Parse the XML into the DOM fragment
    xmlReader->ParseL(aXml);
    
    RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("results"))->Element(_L8("trackmatches"))->ElementsL();
        
    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
        {
        HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
                        KNullDesC8().AllocLC() :
                        items[i]->Element(KElementImage)->Content().AllocLC());
        
        CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Content()),
                                                        *image));
                                                        
        CleanupStack::PopAndDestroy(3);
                                                        
        CleanupStack::PushL(item);
        aList.AppendL(item);
        CleanupStack::Pop(item);
        }
    
    CleanupStack::PopAndDestroy(2, xmlReader);
    }

void CMobblerParser::ParseSearchAlbumL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList)
    {
    DUMPDATA(aXml, _L("searchalbum.xml"));
    
    // Create the XML reader and DOM fragement and associate them with each other
    CSenXmlReader* xmlReader(CSenXmlReader::NewL());
    CleanupStack::PushL(xmlReader);
    CSenDomFragment* domFragment(CSenDomFragment::NewL());
    CleanupStack::PushL(domFragment);
    xmlReader->SetContentHandler(*domFragment);
    domFragment->SetReader(*xmlReader);
    
    // Parse the XML into the DOM fragment
    xmlReader->ParseL(aXml);
    
    RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("results"))->Element(_L8("albummatches"))->ElementsL();
        
    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
        {
        HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
                        KNullDesC8().AllocLC() :
                        items[i]->Element(KElementImage)->Content().AllocLC());
        
        CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Content()),
                                                        *image));
                                                        
        CleanupStack::PopAndDestroy(3);
        
        item->SetIdL(items[i]->Element(_L8("id"))->Content());
                                                        
        CleanupStack::PushL(item);
        aList.AppendL(item);
        CleanupStack::Pop(item);
        }
    
    CleanupStack::PopAndDestroy(2, xmlReader);
    
    }

void CMobblerParser::ParseSearchArtistL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
    {
    DUMPDATA(aXml, _L("searchartist.xml"));
    
    // Create the XML reader and DOM fragement and associate them with each other
    CSenXmlReader* xmlReader(CSenXmlReader::NewL());
    CleanupStack::PushL(xmlReader);
    CSenDomFragment* domFragment(CSenDomFragment::NewL());
    CleanupStack::PushL(domFragment);
    xmlReader->SetContentHandler(*domFragment);
    domFragment->SetReader(*xmlReader);
    
    // Parse the XML into the DOM fragment
    xmlReader->ParseL(aXml);
    
    RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("results"))->Element(_L8("artistmatches"))->ElementsL();
        
    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
        {
        HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
                        KNullDesC8().AllocLC() :
                        items[i]->Element(KElementImage)->Content().AllocLC());
        
        CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
                                                        KNullDesC8,
                                                        *image));
                                                        
        CleanupStack::PopAndDestroy(2);
        
        item->SetIdL(items[i]->Element(_L8("mbid"))->Content());
                                                        
        CleanupStack::PushL(item);
        aList.AppendL(item);
        CleanupStack::Pop(item);
        }
    
    CleanupStack::PopAndDestroy(2, xmlReader);
    }

void CMobblerParser::ParseSearchTagL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList)
    {
    DUMPDATA(aXml, _L("searchtag.xml"));
    
    // Create the XML reader and DOM fragement and associate them with each other
    CSenXmlReader* xmlReader(CSenXmlReader::NewL());
    CleanupStack::PushL(xmlReader);
    CSenDomFragment* domFragment(CSenDomFragment::NewL());
    CleanupStack::PushL(domFragment);
    xmlReader->SetContentHandler(*domFragment);
    domFragment->SetReader(*xmlReader);
    
    // Parse the XML into the DOM fragment
    xmlReader->ParseL(aXml);
    
    RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("results"))->Element(_L8("tagmatches"))->ElementsL();
        
    const TInt KCount(items.Count());
    for (TInt i(0); i < KCount; ++i)
        {
        HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
                        KNullDesC8().AllocLC() :
                        items[i]->Element(KElementImage)->Content().AllocLC());
        
        CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
                                                        *SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
                                                        items[i]->Element(_L8("count"))->Content(),
                                                        *image));
                                                        
        CleanupStack::PopAndDestroy(2);
                                                        
        CleanupStack::PushL(item);
        aList.AppendL(item);
        CleanupStack::Pop(item);
        }
    
    CleanupStack::PopAndDestroy(2, xmlReader);
    }

void CMobblerParser::ParseFriendListL(const TDesC8& aXml, CMobblerFriendList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergetfriends.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("friends"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														items[i]->Element(_L8("name"))->Content(),
														items[i]->Element(_L8("realname"))->Content(),
														*image));
														
		CleanupStack::PopAndDestroy(1);
														
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergettopartists.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("topartists"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("playcount"))->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}


void CMobblerParser::ParseRecommendedArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("recommendedartists.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("recommendations"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														KNullDesC8,
														*image));
		CleanupStack::PopAndDestroy(2);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseSimilarArtistsL(const TDesC8& aXml, CMobblerArtistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("similarartists.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("similarartists"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("match"))->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseEventsL(const TDesC8& aXml, CMobblerEventList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergetevents.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("events"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("title"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("venue"))->Element(_L8("name"))->Content()),
														*image));
		
		item->SetIdL(items[i]->Element(_L8("id"))->Content());
		
		CleanupStack::PopAndDestroy(3);
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopAlbumsL(const TDesC8& aXml, CMobblerAlbumList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usergettopalbums.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("topalbums"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());
		
		
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Element(_L8("name"))->Content()),
														*image));
		
		item->SetIdL(items[i]->Element(_L8("mbid"))->Content());
		
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseArtistTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("artisttoptracks.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("toptracks"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(_L8("image")) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(_L8("image"))->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														items[i]->Element(_L8("playcount"))->Content(),
														*image));
		CleanupStack::PopAndDestroy(2);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseUserTopTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("usertoptracks.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("toptracks"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
						
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Element(_L8("name"))->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParsePlaylistL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("fetchplaylist.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Child(0)->Element(_L8("trackList"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{				
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("title"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("creator"))->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	}

void CMobblerParser::ParseSimilarTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("similartracks.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("similartracks"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* artistName((items[i]->Element(_L8("artist")) == NULL) ?
				KNullDesC8().AllocLC() :
				SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Element(_L8("name"))->Content()));
				
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
				KNullDesC8().AllocLC() :
				items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*artistName,
														*image));
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseRecentTracksL(const TDesC8& aXml, CMobblerTrackList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("recenttracks.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("recenttracks"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(_L8("image")) == NULL) ?
				KNullDesC8().AllocLC():
				items[i]->Element(_L8("image"))->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("artist"))->Content()),
														*image));
		
		if (items[i]->AttrValue(_L8("nowplaying"))
				&& items[i]->AttrValue(_L8("nowplaying"))->Compare(_L8("true")) == 0)
			{
			// We set null as a constant meaning 'now'
			item->SetTimeL(KNullDesC8);
			}
		else
			{
			item->SetTimeL(*items[i]->Element(_L8("date"))->AttrValue(_L8("uts")));
			}
		
		CleanupStack::PopAndDestroy(3);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParsePlaylistsL(const TDesC8& aXml, CMobblerPlaylistList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("userplaylists.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("playlists"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		HBufC8* image((items[i]->Element(KElementImage) == NULL) ?
						KNullDesC8().AllocLC() :
						items[i]->Element(KElementImage)->Content().AllocLC());
		
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("title"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("description"))->Content()),
														*image));
		CleanupStack::PopAndDestroy(3);
		
		item->SetIdL(items[i]->Element(_L8("id"))->Content());
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	}

void CMobblerParser::ParseShoutboxL(const TDesC8& aXml, CMobblerShoutbox& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("shoutbox.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("shouts"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < Min(KCount, 25); ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("author"))->Content()),
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("body"))->Content()),
														KNullDesC8));
		CleanupStack::PopAndDestroy(2);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

void CMobblerParser::ParseTopTagsL(const TDesC8& aXml, CMobblerTagList& aObserver, RPointerArray<CMobblerListItem>& aList)
	{
	DUMPDATA(aXml, _L("toptags.xml"));
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	RPointerArray<CSenElement>& items = domFragment->AsElement().Element(_L8("toptags"))->ElementsL();
		
	const TInt KCount(items.Count());
	for (TInt i(0); i < KCount; ++i)
		{
		CMobblerListItem* item(CMobblerListItem::NewL(aObserver,
														*SenXmlUtils::DecodeHttpCharactersLC(items[i]->Element(_L8("name"))->Content()),
														items[i]->Element(_L8("count"))->Content(),
														KNullDesC8));
		CleanupStack::PopAndDestroy(1);
		
		CleanupStack::PushL(item);
		aList.AppendL(item);
		CleanupStack::Pop(item);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

TInt CMobblerParser::ParseUpdateResponseL(const TDesC8& aXml, TVersion& aVersion, TDes8& location)
	{
	DUMPDATA(aXml, _L("update.xml"));
	
	TInt error(KErrNone);
	
	// Create the XML reader and DOM fragement and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// Parse the XML into the DOM fragment
	xmlReader->ParseL(aXml);
	
	TLex8 lex8;
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMajor)->Content());
	error |= lex8.Val(aVersion.iMajor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionMinor)->Content());
	error |= lex8.Val(aVersion.iMinor);
	lex8.Assign(domFragment->AsElement().Element(KUpdateVersionBuild)->Content());
	error |= lex8.Val(aVersion.iBuild);
	location.Copy(domFragment->AsElement().Element(KUpdateLocation)->Content());
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	return error;
	}

// End of file
