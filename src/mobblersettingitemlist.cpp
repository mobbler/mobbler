/*
mobblersettingitemlist.cpp

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

#include <aknappui.h>
#include <AknTextSettingPage.h>
#include <aknviewappui.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <barsread.h>
#include <eikappui.h>
#include <eikcmobs.h>
#include <eikedwin.h>
#include <eikenv.h>
#include <eikmenup.h>
#include <eikseced.h>
#include <gdi.h>
#include <mobbler.rsg>
#include <stringloader.h> 

#include "coemain.h"
#include "mobbler.hrh"
#include "mobbleraccesspointsettingitem.h"
#include "mobblersettingitemlist.h"
#include "mobblersettingitemlistsettings.h"
#include "mobblersettingitemlistview.h"
#include "mobblerutility.h"

CMobblerSettingItemList::CMobblerSettingItemList(CMobblerSettingItemListSettings& aSettings, MEikCommandObserver* aCommandObserver)
	:iSettings(aSettings), iCommandObserver(aCommandObserver)
	{
	}


CMobblerSettingItemList::~CMobblerSettingItemList()
	{
	}

CAknSettingItem* CMobblerSettingItemList::CreateSettingItemL(TInt aId)
	{
	switch ( aId )
		{
		case EMobblerSettingItemListViewUsername:
			{		
			CAknTextSettingItem* item = new(ELeave) CAknTextSettingItem(aId, iSettings.Username());
			item->SetSettingPageFlags(CAknTextSettingPage::EPredictiveTextEntryPermitted);
			return item;
			}
		case EMobblerSettingItemListViewPassword:
			{			
			CAknSettingItem* item = new(ELeave) CAknPasswordSettingItem(aId, CAknPasswordSettingItem::EAlpha, iSettings.Password());
			return item;
			}
		case EMobblerSettingItemListViewBacklight:
			{			
			CAknSettingItem* item = new(ELeave) CAknBinaryPopupSettingItem(aId, iSettings.Backlight());
			return item;
			}
		case EMobblerSettingItemListViewAutoUpdatesOn:
			{			
			CAknSettingItem* item = new(ELeave) CAknBinaryPopupSettingItem(aId, iSettings.CheckForUpdates());
			return item;
			}
		case EMobblerSettingItemListViewIap:
			{			
			CAknSettingItem* item = new(ELeave) CMobblerAccessPointSettingItem(aId, iSettings.IapID());
			return item;
			}
		case EMobblerSettingItemListViewBufferSize:
			{			
			CAknSettingItem* item = new(ELeave) CAknSliderSettingItem(aId, iSettings.BufferSize());
			return item;
			}
		}
		
	return NULL;
	}
	
void CMobblerSettingItemList::EditItemL(TInt aIndex, TBool aCalledFromMenu)
	{
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
	(*SettingItemArray())[aIndex]->StoreL();
	}

void CMobblerSettingItemList::HandleResourceChange(TInt aType)
	{
	CAknSettingItemList::HandleResourceChange(aType);
	SetRect(iAvkonViewAppUi->View(TUid::Uid(0xA0007CA8))->ClientRect());
	}
				
TKeyResponse CMobblerSettingItemList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	if ( aKeyEvent.iCode == EKeyLeftArrow 
		|| aKeyEvent.iCode == EKeyRightArrow )
		{
		// allow the tab control to get the arrow keys
		return EKeyWasNotConsumed;
		}
	
	return CAknSettingItemList::OfferKeyEventL(aKeyEvent, aType);
	}

void CMobblerSettingItemList::SizeChanged()
	{
	}
				
