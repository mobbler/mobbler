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
#include <s32file.h>
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

void CMobblerSettingItemList::LoadSettingValuesL(CMobblerSettingItemListSettings& aSettings)
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError = file.Open(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileRead | EFileShareAny);
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TBuf<255> username;
		TBuf<255> password;
		
		// Default values if the settings do not already exist
		TBool backlight = EFalse;
		TBool autoUpdatesOn = ETrue;
		TUint32 iapId(0);
		TUint8 bufferSize(KDefaultBufferSizeSeconds);
		
		readStream >> username;
		readStream >> password;

		 // Ignore KErrEof if these settings are not yet saved in the file
		TRAP_IGNORE(backlight = readStream.ReadInt8L());
		TRAP_IGNORE(autoUpdatesOn = readStream.ReadInt8L());
		TRAP_IGNORE(iapId = readStream.ReadUint32L());
		TRAP_IGNORE(bufferSize = readStream.ReadUint8L());

		aSettings.SetUsernameL(username);
		aSettings.SetPasswordL(password);
		aSettings.SetBacklight(backlight);
		aSettings.SetCheckForUpdates(autoUpdatesOn);
		aSettings.SetIapID(iapId);
		aSettings.SetBufferSize(bufferSize);
		
		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		// there was no file there so read from the resource file
		HBufC* username  = StringLoader::LoadLC(R_MOBBLER_USERNAME);
		aSettings.SetUsernameL(*username);
		CleanupStack::PopAndDestroy(username);
		aSettings.SetPasswordL(_L("password"));
		aSettings.SetBacklight(EFalse);
		aSettings.SetCheckForUpdates(ETrue);
		aSettings.SetIapID(0);
		aSettings.SetBufferSize(KDefaultBufferSizeSeconds);
		}
		
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerSettingItemList::SaveSettingValuesL()
	{
	RFile file;
	CleanupClosePushL(file);
	CCoeEnv::Static()->FsSession().MkDirAll(KSettingsFile);
	TInt error = file.Replace(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileWrite | EFileShareAny);
	
	if (error == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		writeStream << iSettings.Username();
		writeStream << iSettings.Password();
		writeStream.WriteInt8L(iSettings.Backlight());
		writeStream.WriteInt8L(iSettings.CheckForUpdates());
		writeStream.WriteUint32L(iSettings.IapID());
		writeStream.WriteUint8L(iSettings.BufferSize());
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerSettingItemList::SizeChanged()
	{
	}
				
