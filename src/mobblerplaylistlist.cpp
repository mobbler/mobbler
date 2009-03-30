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
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerplaylistlist.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_playlist.png");

CMobblerPlaylistList::CMobblerPlaylistList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerPlaylistList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getplaylists"), iText1->String8(), *this);
	}

CMobblerPlaylistList::~CMobblerPlaylistList()
	{
	}

CMobblerListControl* CMobblerPlaylistList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandPlaylistAddTrack:
			if (iAppUi.CurrentTrack())
				{
				iAppUi.LastFMConnection().PlaylistAddTrackL(iList[iListBox->CurrentItemIndex()]->Id(),
																iAppUi.CurrentTrack()->Artist().String8(),
																iAppUi.CurrentTrack()->Title().String8());
				}
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerPlaylistList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
	}


void CMobblerPlaylistList::ParseL(const TDesC8& aXML)
	{
	CMobblerParser::ParsePlaylistsL(aXML, *this, iList);
	}

// End of file
