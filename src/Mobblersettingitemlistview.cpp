/*
mobblersettingitemlistview.cpp

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

#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include <akncontext.h>
#include <akntitle.h>
#include <stringloader.h>
#include <barsread.h>
#include <eikbtgpc.h>
#include <mobbler.rsg>
#include <aknprogressdialog.h>

#include "mobbler.hrh"
#include "mobblersettingitemlistview.h"
#include "mobblersettingitemlist.h"
#include "mobblerappui.h"

CMobblerSettingItemListView* CMobblerSettingItemListView::NewL()
	{
	CMobblerSettingItemListView* self = new(ELeave) CMobblerSettingItemListView;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerSettingItemListView::CMobblerSettingItemListView()
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
	CMobblerSettingItemList::LoadSettingValuesL(*iSettings);
	}

TUid CMobblerSettingItemListView::Id() const
	{
	return TUid::Uid(0xA0007CA9);
	}

void CMobblerSettingItemListView::HandleCommandL(TInt aCommand)
	{
	// let the app ui handle the event
	if (aCommand == EAknSoftkeyOk || aCommand == EAknSoftkeyBack)
		{
		// switch back to the status view
		static_cast<CMobblerAppUi*>(AppUi())->SetDetailsL(iSettings->Username(), iSettings->Password());
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
		iMobblerSettingItemList->ConstructFromResourceL( R_MOBBLER_SETTING_ITEM_LIST );
		iMobblerSettingItemList->LoadSettingsL();
		iMobblerSettingItemList->LoadSettingValuesL(*iSettings);
		iMobblerSettingItemList->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerSettingItemList);
		}
	}

void CMobblerSettingItemListView::DoDeactivate()
	{
	if (iMobblerSettingItemList)
		{
		iMobblerSettingItemList->SaveSettingValuesL();
		AppUi()->RemoveFromStack(iMobblerSettingItemList);
		delete iMobblerSettingItemList;
		iMobblerSettingItemList = NULL;
		}
	}

const TDesC& CMobblerSettingItemListView::GetUserName()
	{
	return iSettings->Username();
	}

const TDesC& CMobblerSettingItemListView::GetPassword()
	{
	return iSettings->Password();
	}

void CMobblerSettingItemListView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	}

TBool CMobblerSettingItemListView::HandleChangeSelectedSettingItemL(TInt /*aCommand*/)
	{
	return ETrue;
	}

