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

#include "mobblerparser.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

_LIT8(KElementTitle, "title");
_LIT8(KElementTrack, "track");
_LIT8(KElementLocation, "location");
_LIT8(KElementId, "id");
_LIT8(KElementAlbum, "album");
_LIT8(KElementCreator, "creator");
_LIT8(KElementDuration, "duration");
_LIT8(KElementImage, "image");
_LIT8(KElementTrackList, "trackList");
_LIT8(KElementStatus, "status");
_LIT8(KElementSession, "session");
_LIT8(KElementKey, "key");
_LIT8(KElementRadioAuth, "trackauth");
_LIT8(KElementError, "error");
_LIT8(KElementLink, "link");

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
CMobblerLastFMError* CMobblerParser::ParseHandshakeL(const TDesC8& aHandshakeResponse, HBufC8*& aSessionId, HBufC8*& aNowPlayingURL, HBufC8*& aSubmitURL)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:scrobblehandshake.txt"), EFileWrite));
	User::LeaveIfError(file.Write(aHandshakeResponse));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
	resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);

	CMobblerLastFMError* error = NULL;
	
	if (aHandshakeResponse.MatchF(_L8("OK*")) == 0)
		{
		TInt position = aHandshakeResponse.Find(_L8("\n"));
		
		// get the session id
		TPtrC8 last3Lines = aHandshakeResponse.Mid(position + 1, aHandshakeResponse.Length() - (position + 1) );
		position = last3Lines.Find(_L8("\n"));
		delete aSessionId;
		aSessionId = last3Lines.Mid(0, position).AllocL();
		
		// get the now playing url
		TPtrC8 last2Lines = last3Lines.Mid(position + 1, last3Lines.Length() - (position + 1) );
		position = last2Lines.Find(_L8("\n"));
		delete aNowPlayingURL;
		aNowPlayingURL = last2Lines.Mid(0, position).AllocL();
		
		// get the submit url
		TPtrC8 last1Lines = last2Lines.Mid(position + 1, last2Lines.Length() - (position + 1) );
		position = last1Lines.Find(_L8("\n"));
		delete aSubmitURL;
		aSubmitURL = last1Lines.Mid(0, position).AllocL();
		}
	else if (aHandshakeResponse.MatchF(_L8("BANNED*")) == 0)
		{
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BANNED);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBanned);
		CleanupStack::PopAndDestroy(errorText);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADAUTH*")) == 0)
		{
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BAD_AUTH);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBadAuth);
		CleanupStack::PopAndDestroy(errorText);
		}
	else if (aHandshakeResponse.MatchF(_L8("BADTIME*")) == 0)
		{
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BAD_TIME);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBadTime);
		CleanupStack::PopAndDestroy(errorText);
		}
	else
		{
		error = CMobblerLastFMError::NewL(aHandshakeResponse, CMobblerLastFMError::EOther);
		}
	
	delete resourceReader;
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
CMobblerLastFMError* CMobblerParser::ParseRadioHandshakeL(const TDesC8& aRadioHandshakeResponse, HBufC8*& aRadioSessionID, HBufC8*& aRadioStreamURL, HBufC8*& aRadioBaseURL, HBufC8*& aRadioBasePath)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:radiohandshakeresponse.txt"), EFileWrite));
	User::LeaveIfError(file.Write(aRadioHandshakeResponse));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerLastFMError* error = NULL;
	
	if (aRadioHandshakeResponse.MatchF(_L8("session*")) == 0)
		{
		if (aRadioHandshakeResponse.Find(_L8("session=FAILED")) == 0)
			{
			CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
			resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
			HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BAD_AUTH);
			error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBadAuth);
			CleanupStack::PopAndDestroy(errorText);
			delete resourceReader;
			}
		else
			{
			RDesReadStream readStream(aRadioHandshakeResponse);
			CleanupClosePushL(readStream);
			
			const TInt KLines(7);
			
			for (TInt i(0) ; i < KLines ; ++i)
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
					// stream url
					delete aRadioStreamURL;
					aRadioStreamURL = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
					}
				else if (i == 4)
					{
					//base url
					delete aRadioBaseURL;
					aRadioBaseURL = line.Mid(equalPosition + 1, line.Length() - (equalPosition + 1) - 1).AllocL();
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
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:scrobbleresponse.txt"), EFileWrite));
	User::LeaveIfError(file.Write(aScrobbleResponse));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerLastFMError* error = NULL;
	
	if (aScrobbleResponse.Compare(_L8("OK\n")) == 0)
		{
		// do nothing
		}
	else if (aScrobbleResponse.Compare(_L8("BADSESSION\n")) == 0)
		{
		CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
		resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BAD_SESSION);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBadSession);
		CleanupStack::PopAndDestroy(errorText);
		delete resourceReader;
		}
	else
		{
		error = CMobblerLastFMError::NewL(aScrobbleResponse, CMobblerLastFMError::EOther);
		}
	
	return error;
	}

HBufC8* CMobblerParser::DecodeURIStringLC(const TDesC8& aString)
	{
	HBufC8* result = aString.AllocLC();
	
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


CMobblerLastFMError* CMobblerParser::ParseRadioPlaylistL(const TDesC8& aXML, CMobblerRadioPlaylist*& aPlaylist)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:playlist.xml"), EFileWrite));
	User::LeaveIfError(file.Write(aXML));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerLastFMError* error = NULL;
	
	CMobblerRadioPlaylist* playlist = CMobblerRadioPlaylist::NewL();
	CleanupStack::PushL(playlist);
	
	// create the xml reader and dom fragement and associate them with each other 
	CSenXmlReader* xmlReader = CSenXmlReader::NewL();
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment = CSenDomFragment::NewL();
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the xml into the dom fragment
	xmlReader->ParseL(aXML);
	
	HBufC8* radioName = DecodeURIStringLC(domFragment->AsElement().Element(KElementTitle)->Content());
	playlist->SetNameL(*radioName);
	CleanupStack::PopAndDestroy();
	
	HBufC8* skipsText = DecodeURIStringLC(domFragment->AsElement().Element(KElementLink)->Content());
	TLex8 lex8(*skipsText);
	TInt skips(0);
	lex8.Val(skips);
	playlist->SetSkips(skips);
	CleanupStack::PopAndDestroy(skipsText);
	
	RPointerArray<CSenElement>& tracks = domFragment->AsElement().Element(KElementTrackList)->ElementsL();
	
	const TInt KTrackCount(tracks.Count());
	for (TInt i(0) ; i < KTrackCount; ++i)
		{
		// get the duration as a number
		TLex8 lex(tracks[i]->Element(KElementDuration)->Content());
		TInt durationMilliSeconds;
		lex.Val(durationMilliSeconds);
		TTimeIntervalSeconds durationSeconds(durationMilliSeconds / 1000);
		
		TPtrC8 creator = tracks[i]->Element(KElementCreator)->Content();
		HBufC8* creatorBuf = HBufC8::NewLC(creator.Length());
		SenXmlUtils::DecodeHttpCharactersL(creator, creatorBuf);
		TPtrC8 title = tracks[i]->Element(KElementTitle)->Content();
		HBufC8* titleBuf = HBufC8::NewLC(title.Length());
		SenXmlUtils::DecodeHttpCharactersL(title, titleBuf);
		TPtrC8 album = tracks[i]->Element(KElementAlbum)->Content();
		HBufC8* albumBuf = HBufC8::NewLC(album.Length());
		SenXmlUtils::DecodeHttpCharactersL(album, albumBuf);
		
		TPtrC8 image = tracks[i]->Element(KElementImage)->Content();
		TPtrC8 location = tracks[i]->Element(KElementLocation)->Content();
		TPtrC8 radioAuth = tracks[i]->Element(KNamespace, KElementRadioAuth)->Content();
		
		CMobblerTrack* track = CMobblerTrack::NewL(*creatorBuf, *titleBuf, *albumBuf, image, location, durationSeconds, radioAuth);
		CleanupStack::PushL(track);
		playlist->AppendTrackL(track);
		CleanupStack::Pop(track);
		
		CleanupStack::PopAndDestroy(3, creatorBuf);
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	
	if (playlist->Count() == 0)
		{
		CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
		resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_NO_TRACKS);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::ENoTracks);
		CleanupStack::PopAndDestroy(errorText);
		CleanupStack::PopAndDestroy(playlist);
		delete resourceReader;
		}
	else
		{
		CleanupStack::Pop(playlist);
		aPlaylist = playlist;
		}
	
	return error;
	}

CMobblerLastFMError* CMobblerParser::ParseRadioSelectStationL(const TDesC8& aXML)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:radioselectresponse.xml"), EFileWrite));
	User::LeaveIfError(file.Write(aXML));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerLastFMError* error = NULL;
	
	if (aXML.Find(_L8("response=OK")) != 0)
		{
		CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
		resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
		HBufC* errorText = resourceReader->AllocReadLC(R_MOBBLER_NOTE_BAD_STATION);
		error = CMobblerLastFMError::NewL(*errorText, CMobblerLastFMError::EBadStation);
		CleanupStack::PopAndDestroy(errorText);
		delete resourceReader;
		}
	
	return error;
	}

CMobblerLastFMError* CMobblerParser::ParseWebServicesHandshakeL(const TDesC8& aWebServicesHandshakeResponse, HBufC8*& aWebServicesSessionKey)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:webserviceshandshakeresponse.xml"), EFileWrite));
	User::LeaveIfError(file.Write(aWebServicesHandshakeResponse));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	CMobblerLastFMError* error = NULL;
	
	// create the xml reader and dom fragement and associate them with each other 
	CSenXmlReader* xmlReader = CSenXmlReader::NewL();
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment = CSenDomFragment::NewL();
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the xml into the dom fragment
	xmlReader->ParseL(aWebServicesHandshakeResponse);
	
	// get the error code
	const TDesC8* statusText = domFragment->AsElement().AttrValue(KElementStatus);
		
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

void CMobblerParser::ParseArtistGetInfoL(const TDesC8& aXML, HBufC*& aArtistInfo)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:artistgetinfo.xml"), EFileWrite));
	User::LeaveIfError(file.Write(aXML));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	// create the xml reader and dom fragement and associate them with each other 
	CSenXmlReader* xmlReader = CSenXmlReader::NewL();
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment = CSenDomFragment::NewL();
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the xml into the dom fragment
	xmlReader->ParseL(aXML);
	
	CSenElement* artistElement = domFragment->AsElement().Element(_L8("artist"));
	
	if (artistElement)
		{
		CSenElement* bioElement = artistElement->Element(_L8("bio"));
		
		if (bioElement)
			{
			CSenElement* contentElement = bioElement->Element(_L8("content"));
			
			if (contentElement)
				{
				CMobblerString* content = CMobblerString::NewL(contentElement->Content());
				CleanupStack::PushL(content);
				aArtistInfo = content->String().AllocL();
				CleanupStack::PopAndDestroy(content);
				}
			}
		}
	
	CleanupStack::PopAndDestroy(2, xmlReader);
	}

TInt CMobblerParser::ParseUpdateResponseL(const TDesC8& aXML, TVersion& aVersion, TDes8& location)
	{
#ifdef _DEBUG
	RFile file;
	CleanupClosePushL(file);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), _L("c:update.xml"), EFileWrite));
	User::LeaveIfError(file.Write(aXML));
	CleanupStack::PopAndDestroy(&file);
#endif
	
	TInt error(KErrNone);
	
	// create the xml reader and dom fragement and associate them with each other 
	CSenXmlReader* xmlReader = CSenXmlReader::NewL();
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment = CSenDomFragment::NewL();
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the xml into the dom fragment
	xmlReader->ParseL(aXML);
	
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
