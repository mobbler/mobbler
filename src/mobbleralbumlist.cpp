/*
mobbleralbumlist.cpp

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

#include <gulicon.h>
#include <eikclbd.h>

#include "mobblerappui.h"
#include "mobbleralbumlist.h"
#include "mobblerparser.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerstring.h"

#include "mobbler.hrh"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_user.png");

CMobblerAlbumList::CMobblerAlbumList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerAlbumList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    switch (iType)
    	{
    	case EMobblerCommandUserTopAlbums:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("gettopalbums"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandArtistTopAlbums:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("gettopalbums"), iText1->String8(), *this);
    		break;
    	default:
    		break;
    	}
	}

CMobblerAlbumList::~CMobblerAlbumList()
	{
	}

CMobblerListControl* CMobblerAlbumList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandRadioStart:
			iAppUi.RadioStartL(CMobblerLastFMConnection::EArtist, iList[iListBox->CurrentItemIndex()]->Description());
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerAlbumList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandRadioStart);
	}

void CMobblerAlbumList::ParseL(const TDesC8& aXML)
	{
	CMobblerParser::ParseTopAlbumsL(aXML, *this, iList);
	}




