/*
mobblertracklist.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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


#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertracklist.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_track.png");

CMobblerTrackList::CMobblerTrackList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerTrackList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    switch (iType)
    	{
    	case EMobblerCommandArtistTopTracks:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("gettoptracks"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandUserTopTracks:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("gettoptracks"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandRecentTracks:
    		iAppUi.LastFMConnection().RecentTracksL(iText1->String8(), *this);
    		break;
    	case EMobblerCommandSimilarTracks:
    		iAppUi.LastFMConnection().SimilarTracksL(iText1->String8(), iText2->String8(), *this);
    		break;
    	default:
    		break;
    	}
	}

CMobblerTrackList::~CMobblerTrackList()
	{
	}

CMobblerListControl* CMobblerTrackList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch(aCommand)
		{	
		case EMobblerCommandTrackLove:
			switch (iType)
				{
				case EMobblerCommandArtistTopTracks:
					iAppUi.LastFMConnection().TrackLoveL(iText1->String8(), iList[iListBox->CurrentItemIndex()]->Title()->String8());
					break;
				case EMobblerCommandUserTopTracks:
				case EMobblerCommandRecentTracks:
				case EMobblerCommandSimilarTracks:
					iAppUi.LastFMConnection().TrackLoveL(iList[iListBox->CurrentItemIndex()]->Description()->String8(), iList[iListBox->CurrentItemIndex()]->Title()->String8());
					break;
				default:
					break;
				}
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerTrackList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandTrackLove);
	}

void CMobblerTrackList::ParseL(const TDesC8& aXML)
	{
    switch (iType)
    	{
    	case EMobblerCommandArtistTopTracks:
    		CMobblerParser::ParseArtistTopTracksL(aXML, *this, iList);
    		break;
    	case EMobblerCommandUserTopTracks:
    		CMobblerParser::ParseUserTopTracksL(aXML, *this, iList);
    		break;
    	case EMobblerCommandRecentTracks:
    		CMobblerParser::ParseRecentTracksL(aXML, *this, iList);
    		break;
    	case EMobblerCommandSimilarTracks:
    		CMobblerParser::ParseSimilarTracksL(aXML, *this, iList);
    		break;
    	default:
    		break;
    	}
	}

// End of file
