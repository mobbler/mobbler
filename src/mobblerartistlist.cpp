/*
mobblerartistlist.cpp

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
#include "mobblerartistlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerwebserviceshelper.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_artist.png");

CMobblerArtistList::CMobblerArtistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerArtistList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    switch (iType)
	    {
	    case EMobblerCommandUserTopArtists:
	    	iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("gettopartists"), iText1->String8(), *this);
	    	break;
	    case EMobblerCommandRecommendedArtists:
	    	iAppUi.LastFMConnection().RecommendedArtistsL(*this);
	    	break;
	    case EMobblerCommandSimilarArtists:
	    	iAppUi.LastFMConnection().SimilarArtistsL(iText1->String8(), *this);
	    	break;
	    case EMobblerCommandTagTopArtists:
	    	iAppUi.LastFMConnection().WebServicesCallL(_L8("tag"), _L8("gettopartists"), iText1->String8(), *this);
	    	break;
	    case EMobblerCommandSearchArtist:
            iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("search"), iText1->String8(), *this);
            break;
	    default:
	    	break;
	    }
	}

CMobblerArtistList::~CMobblerArtistList()
	{
	}

CMobblerListControl* CMobblerArtistList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandRadioStart:
			iAppUi.RadioStartL(CMobblerLastFMConnection::EArtist, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		case EMobblerCommandSimilarArtists:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandSimilarArtists, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistEvents:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandArtistEvents, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistShoutbox:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandArtistShoutbox, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistTopAlbums:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandArtistTopAlbums, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistTopTracks:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandArtistTopTracks, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);		
			break;
		case EMobblerCommandArtistTopTags:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandArtistTopTags, iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8);
			break;
		case EMobblerCommandArtistShare:
		    {
		    CMobblerTrack* track = CMobblerTrack::NewL(iList[iListBox->CurrentItemIndex()]->Title()->String8(), KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8);
            delete iWebServicesHelper;
            iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
            iWebServicesHelper->ArtistShareL(*track);            
            track->Release();
		    }
		default:
			break;	
		}
	
	return list;
	}

void CMobblerArtistList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandRadioStart);
	
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandSimilarArtists);
	aCommands.AppendL(EMobblerCommandArtistEvents);
	aCommands.AppendL(EMobblerCommandArtistShoutbox);
	aCommands.AppendL(EMobblerCommandArtistTopAlbums);
	aCommands.AppendL(EMobblerCommandArtistTopTracks);
	aCommands.AppendL(EMobblerCommandArtistTopTags);
	
    aCommands.AppendL(EMobblerCommandShare);
    aCommands.AppendL(EMobblerCommandArtistShare);
	}

void CMobblerArtistList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, CMobblerLastFMConnection::TError /*aError*/)
    {
    }

void CMobblerArtistList::ParseL(const TDesC8& aXML)
	{
	switch (iType)
	    {
	    case EMobblerCommandUserTopArtists:
	    	CMobblerParser::ParseTopArtistsL(aXML, *this, iList);
	    	break;
	    case EMobblerCommandRecommendedArtists:
	    	CMobblerParser::ParseRecommendedArtistsL(aXML, *this, iList);
	    	break;
	    case EMobblerCommandSimilarArtists:
	    	CMobblerParser::ParseSimilarArtistsL(aXML, *this, iList);
	    	break;
	    case EMobblerCommandTagTopArtists:
	    	CMobblerParser::ParseTopArtistsL(aXML, *this, iList);
	    	break;
	    case EMobblerCommandSearchArtist:
	        CMobblerParser::ParseSearchArtistL(aXML, *this, iList);
	        break;
	    default:
	    	break;
	    }
	}

// End of file
