/*
mobblertaglist.cpp

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
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblertaglist.h"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_tag.png");

CMobblerTagList::CMobblerTagList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerTagList::ConstructL()
	{
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultTagImage);
	
	switch (iType)
		{
		case EMobblerCommandUserTopTags:
			iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("gettoptags"), iText1->String8(), *this);
			break;
		case EMobblerCommandArtistTopTags:
			iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("gettoptags"), iText1->String8(), *this);
			break;
		case EMobblerCommandSearchTag:
			iAppUi.LastFMConnection().WebServicesCallL(_L8("tag"), _L8("search"), iText1->String8(), *this);
			break;
		default:
			break;
		}
	}

CMobblerTagList::~CMobblerTagList()
	{
	}

CMobblerListControl* CMobblerTagList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{
		case EMobblerCommandRadioStart:
			iAppUi.RadioStartL(EMobblerCommandRadioTag, iList[iListBox->CurrentItemIndex()]->Title());
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerTagList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandRadioStart);
	}

void CMobblerTagList::ParseL(const TDesC8& aXml)
	{
	switch (iType)
		{
		case EMobblerCommandUserTopTags:
		case EMobblerCommandArtistTopTags:
			CMobblerParser::ParseTopTagsL(aXml, *this, iList);
			break;
		case EMobblerCommandSearchTag:
			CMobblerParser::ParseSearchTagL(aXml, *this, iList);
			break;
		default:
			break;
		}
	}

// End of file
