/*
mobblersettingitemlistview.cpp

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

#include <AknTextSettingPage.h>
#include <eikfrlbd.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobblerappui.h"
#include "mobbleraccesspointsettingitem.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlist.h"
#include "mobblersettingitemlistview.h"
#include "mobblerslidersettingitem.h"

CMobblerSettingItemListView* CMobblerSettingItemListView::NewL()
	{
	CMobblerSettingItemListView* self = new(ELeave) CMobblerSettingItemListView;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerSettingItemListView::CMobblerSettingItemListView()
	: iOrdinal(1)
	{
	}

CMobblerSettingItemListView::~CMobblerSettingItemListView()
	{
	delete iMobblerSettingItemList;
	delete iSettings;
	}

void CMobblerSettingItemListView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_SETTING_ITEM_LIST_VIEW);
	iSettings = CMobblerSettingItemListSettings::NewL();
	iSettings->LoadSettingValuesL();
	}

TUid CMobblerSettingItemListView::Id() const
	{
	return TUid::Uid(0xA0007CA9);
	}

void CMobblerSettingItemListView::HandleCommandL(TInt aCommand)
	{
	// let the app ui handle the event
	if (aCommand == EAknSoftkeyOk)
		{
		// save and set details then switch back to the status view
		iSettings->SaveSettingValuesL();
		static_cast<CMobblerAppUi*>(AppUi())->SetDetailsL(iSettings->Username(), iSettings->Password());
		static_cast<CMobblerAppUi*>(AppUi())->SetCheckForUpdatesL(iSettings->CheckForUpdates());
		static_cast<CMobblerAppUi*>(AppUi())->SetIapIDL(iSettings->IapID());
		static_cast<CMobblerAppUi*>(AppUi())->SetBufferSize(iSettings->BufferSize());
		AppUi()->ActivateLocalViewL(TUid::Uid(0xA0007CA8));
		}
	else if (aCommand == EAknSoftkeyCancel)
		{
		// reset the details then switch back to the status view
		iSettings->LoadSettingValuesL();
		AppUi()->ActivateLocalViewL(TUid::Uid(0xA0007CA8));
		}
	else
		{
		AppUi()->HandleCommandL(aCommand);
		}
	}

void CMobblerSettingItemListView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	if (!iMobblerSettingItemList)
		{
		iMobblerSettingItemList = new (ELeave) CMobblerSettingItemList(*iSettings, this);
		iMobblerSettingItemList->SetMopParent(this);
		iMobblerSettingItemList->ConstructFromResourceL(R_MOBBLER_SETTING_ITEM_LIST);
		iMobblerSettingItemList->LoadSettingsL();
		
		LoadListL();
		
		iSettings->LoadSettingValuesL();
		iMobblerSettingItemList->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerSettingItemList);
		}
	}

void CMobblerSettingItemListView::DoDeactivate()
	{
	if (iMobblerSettingItemList)
		{
		AppUi()->RemoveFromStack(iMobblerSettingItemList);
		delete iMobblerSettingItemList;
		iMobblerSettingItemList = NULL;
		}
	}

const TDesC& CMobblerSettingItemListView::UserName() const
	{
	return iSettings->Username();
	}

const TDesC& CMobblerSettingItemListView::Password() const
	{
	return iSettings->Password();
	}

TBool CMobblerSettingItemListView::Backlight() const
	{
	return iSettings->Backlight();
	}

TBool CMobblerSettingItemListView::CheckForUpdates() const
	{
	return iSettings->CheckForUpdates();
	}

TUint32 CMobblerSettingItemListView::IapID() const
	{
	return iSettings->IapID();
	}

TUint8 CMobblerSettingItemListView::BufferSize() const
	{
	return iSettings->BufferSize();
	}

TInt CMobblerSettingItemListView::EqualizerIndex() const
	{
	return iSettings->EqualizerIndex();
	}

void CMobblerSettingItemListView::SetEqualizerIndexL(TInt aIndex)
	{
	iSettings->SetEqualizerIndex(aIndex);
	iSettings->SaveSettingValuesL();
	}

TInt CMobblerSettingItemListView::ScrobblePercent() const
	{
	return iSettings->ScrobblePercent();
	}

TInt CMobblerSettingItemListView::Volume() const
	{
	return iSettings->Volume();
	}

void CMobblerSettingItemListView::SetVolumeL(TInt aVolume)
	{
	iSettings->SetVolume(aVolume);
	iSettings->SaveSettingValuesL();
	}

TInt CMobblerSettingItemListView::SleepTimerMinutes() const
	{
	return iSettings->SleepTimerMinutes();
	}

void CMobblerSettingItemListView::SetSleepTimerMinutesL(TInt aSleepTimerMinutes)
	{
	iSettings->SetSleepTimerMinutes(aSleepTimerMinutes);
	iSettings->SaveSettingValuesL();
	}

void CMobblerSettingItemListView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	}

TBool CMobblerSettingItemListView::HandleChangeSelectedSettingItemL(TInt /*aCommand*/)
	{
	return ETrue;
	}

void CMobblerSettingItemListView::LoadListL()
	{
	iResourceReader = CMobblerResourceReader::NewL();
	iResourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
	iIsNumberedStyle = iMobblerSettingItemList->IsNumberedStyle();
	iIcons = iMobblerSettingItemList->ListBox()->
								ItemDrawer()->FormattedCellData()->IconArray();

	// Username text setting item
	CreateTextItemL(iSettings->Username(),
					R_MOBBLER_USERNAME, 
					R_MOBBLER_SETTING_PAGE_USERNAME);

	// Password setting item
	CreatePasswordItemL(iSettings->Password(),
						R_MOBBLER_PASSWORD,
						R_MOBBLER_SETTING_PAGE_PASSWORD);

	// IAP enumerated text setting item
	CreateIapItemL(iSettings->IapID(),
				   R_MOBBLER_IAP,
				   R_MOBBLER_SETTING_PAGE_ENUM);

	// Buffer size slider setting item
	CreateSliderItemL(iSettings->BufferSize(),
					  R_MOBBLER_BUFFER_SIZE,
					  R_MOBBLER_SLIDER_SETTING_PAGE_BUFFER_SIZE,
					  R_MOBBLER_BUFFER_SIZE_SECOND,
					  R_MOBBLER_BUFFER_SIZE_SECONDS);

	// Scrobble percent slider setting item
	CreateSliderItemL(iSettings->ScrobblePercent(),
					  R_MOBBLER_SCROBBLE_PERCENT,
					  R_MOBBLER_SLIDER_SETTING_PAGE_SCROBBLE_PERCENT,
					  R_MOBBLER_PERCENT,
					  R_MOBBLER_PERCENT);

	// Check for updates binary popup setting item
	CreateBinaryItemL(iSettings->CheckForUpdates(),
					  R_MOBBLER_CHECK_FOR_UPDATES_ONCE_A_WEEK,
					  R_MOBBLER_BINARY_SETTING_PAGE,
					  R_MOBBLER_CHECK_FOR_UPDATES_NO,
					  R_MOBBLER_CHECK_FOR_UPDATES_YES);
	
	// Backlight binary popup setting item
	CreateBinaryItemL(iSettings->Backlight(),
					  R_MOBBLER_BACKLIGHT,
					  R_MOBBLER_BINARY_SETTING_PAGE,
					  R_MOBBLER_BACKLIGHT_SYSTEM_DEFAULT,
					  R_MOBBLER_BACKLIGHT_ON_WHEN_ACTIVE);
	
	// Required when there is only one setting item
	iMobblerSettingItemList->SettingItemArray()->RecalculateVisibleIndicesL();

	iMobblerSettingItemList->HandleChangeInItemArrayOrVisibilityL();

	delete iResourceReader;
	}

void CMobblerSettingItemListView::CreateTextItemL(TDes& aText,
												  const TInt aTitleResource, 
												  const TInt aPageResource)
	{
	CAknTextSettingItem* item = new (ELeave) CAknTextSettingItem(iOrdinal, aText);
	CleanupStack::PushL(item);

	HBufC* text = iResourceReader->AllocReadLC(aTitleResource);
	item->SetEmptyItemTextL(*text);
	item->ConstructL(iIsNumberedStyle, iOrdinal, *text, iIcons, aPageResource, -1);
	CleanupStack::PopAndDestroy(text);
	item->SetSettingPageFlags(CAknTextSettingPage::EPredictiveTextEntryPermitted);

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreatePasswordItemL(TDes& aPassword,
													  const TInt aTitleResource, 
													  const TInt aPageResource)
	{
	CAknPasswordSettingItem* item = new (ELeave) CAknPasswordSettingItem(
						iOrdinal, CAknPasswordSettingItem::EAlpha, aPassword);
	CleanupStack::PushL(item);

	HBufC* text = iResourceReader->AllocReadLC(aTitleResource);
	item->SetEmptyItemTextL(*text);
	item->ConstructL(iIsNumberedStyle, iOrdinal, *text, iIcons, aPageResource, -1);
	CleanupStack::PopAndDestroy(text);

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateIapItemL(TInt& aIapId, 
												 const TInt aTitleResource, 
												 const TInt aPageResource)
	{
	// To avoid "Setting Item Lis 6" panic
	TInt tempIapId = aIapId;
	aIapId = 0;

	CMobblerAccessPointSettingItem* item = new (ELeave) 
			CMobblerAccessPointSettingItem(iOrdinal, aIapId);
	CleanupStack::PushL(item);

	// The same resource ID can be used for multiple enumerated text setting pages
	HBufC* text = iResourceReader->AllocReadLC(aTitleResource);
	item->ConstructL(iIsNumberedStyle, iOrdinal, *text, iIcons, 
				aPageResource, -1, 0, R_MOBBLER_POPUP_SETTING_TEXTS_ENUM);
	CleanupStack::PopAndDestroy(text);

	CArrayPtr<CAknEnumeratedText>* texts = item->EnumeratedTextArray();
	texts->ResetAndDestroy();
	CAknEnumeratedText* enumText;

	// "Always ask" text
	text = iResourceReader->AllocReadLC(R_MOBBLER_ALWAYS_ASK);
	enumText = new (ELeave) CAknEnumeratedText(0, text);
	CleanupStack::Pop(text);
	CleanupStack::PushL(enumText);
	texts->AppendL(enumText);
	CleanupStack::Pop(enumText);

	// Load list of IAPs
	item->LoadIapListL();

	// Set the real value for the item
	aIapId = tempIapId;

	// Load list of IAPs
	item->LoadL();

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateSliderItemL(TInt& aSliderValue, 
													const TInt aTitleResource, 
													const TInt aPageResource,
													const TInt aResourceSingular,
							  						const TInt aResourcePlural)
	{
	CMobblerSliderSettingItem* item = new (ELeave) CMobblerSliderSettingItem(
				iOrdinal, aSliderValue, aResourceSingular, aResourcePlural);
	CleanupStack::PushL(item);

	HBufC* text = iResourceReader->AllocReadLC(aTitleResource);
	item->ConstructL(iIsNumberedStyle, iOrdinal, *text, iIcons, aPageResource, -1);
	CleanupStack::PopAndDestroy(text);

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}

void CMobblerSettingItemListView::CreateBinaryItemL(TBool& aBinaryValue, 
													const TInt aTitleResource, 
													const TInt aPageResource,
													const TInt aFirstEnumResource,
													const TInt aSecondEnumResource)
	{
	CAknBinaryPopupSettingItem* item = new (ELeave) 
				CAknBinaryPopupSettingItem(iOrdinal, aBinaryValue);
	CleanupStack::PushL(item);

	HBufC* text = iResourceReader->AllocReadLC(aTitleResource);
	// The same resource ID can be used for multiple binary setting pages
	item->ConstructL(iIsNumberedStyle, iOrdinal, *text, iIcons, 
				aPageResource, -1, 0, R_MOBBLER_POPUP_SETTING_BINARY_TEXTS);
	CleanupStack::PopAndDestroy(text);

	// Load text dynamically
	CArrayPtr<CAknEnumeratedText>* texts = item->EnumeratedTextArray();
	texts->ResetAndDestroy();
	// Text 1
	text = iResourceReader->AllocReadLC(aFirstEnumResource);
	CAknEnumeratedText* enumText = new (ELeave) CAknEnumeratedText(0, text);
	CleanupStack::Pop(text);
	CleanupStack::PushL(enumText);
	texts->AppendL(enumText);
	CleanupStack::Pop(enumText);
	// Text 2
	text = iResourceReader->AllocReadLC(aSecondEnumResource);
	enumText = new (ELeave) CAknEnumeratedText(1, text);
	CleanupStack::Pop(text);
	CleanupStack::PushL(enumText);
	texts->AppendL(enumText);
	CleanupStack::Pop(enumText);

	// Set the correct text visible
	item->LoadL();

	iMobblerSettingItemList->SettingItemArray()->AppendL(item);
	CleanupStack::Pop(item);
	++iOrdinal;
	}


// End of file
