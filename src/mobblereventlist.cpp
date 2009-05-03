/*
mobblereventlist.cpp

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


#include "mobblerappui.h"
#include "mobblereventlist.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerparser.h"
#include "mobblerstring.h"
#include "mobblerwebserviceshelper.h"

#include "mobbler.hrh"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_event.png");

CMobblerEventList::CMobblerEventList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerEventList::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    switch (iType)
    	{
    	case EMobblerCommandUserEvents:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getevents"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandArtistEvents:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("getevents"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandRecommendedEvents:
    		iAppUi.LastFMConnection().RecommendedEventsL(*this);
    		break;
    	default:
    		break;
    	}
	}

CMobblerEventList::~CMobblerEventList()
	{
	}

CMobblerListControl* CMobblerEventList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandEventShoutbox:
			list = CMobblerListControl::CreateListL(iAppUi, iWebServicesControl, EMobblerCommandEventShoutbox, iList[iListBox->CurrentItemIndex()]->Title()->String8(), iList[iListBox->CurrentItemIndex()]->Id());
			break;
    	case EMobblerCommandEventShare:
    		delete iWebServicesHelper;
    		iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
    		iWebServicesHelper->EventShareL(iList[iListBox->CurrentItemIndex()]->Id());
		default:
			break;	
		}
	
	return list;
	}

void CMobblerEventList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandView);
	aCommands.AppendL(EMobblerCommandEventShoutbox);

	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandEventShare);
	}

void CMobblerEventList::DataL(CMobblerFlatDataObserverHelper* /*aObserver*/, const TDesC8& /*aData*/, CMobblerLastFMConnection::TError /*aError*/)
	{
	}

void CMobblerEventList::ParseL(const TDesC8& aXML)
	{
	CMobblerParser::ParseEventsL(aXML, *this, iList);
	}

// End of file
