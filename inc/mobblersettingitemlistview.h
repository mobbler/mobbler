/*
mobblersettingitemlistview.h

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

#ifndef __MOBBLERSETTINGITEMLISTVIEW_H__
#define __MOBBLERSETTINGITEMLISTVIEW_H__

#include <aknview.h>
#include <aknwaitdialog.h>
#include <aknprogressdialog.h>

#include "mobblersettingitemlistsettings.h"

_LIT(KSettingsFile, "c:settings.ini");

class CMobblerSettingItemList;

class CMobblerSettingItemListView : public CAknView
	{
public:
	static CMobblerSettingItemListView* NewL();
	~CMobblerSettingItemListView();
	
	TUid Id() const;
	void HandleCommandL( TInt aCommand );
	
	const TDesC& GetUserName();
	const TDesC& GetPassword();
	const TBool GetBacklight();
	
private:
	CMobblerSettingItemListView();        
	void ConstructL();

	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage );
	void DoDeactivate();
	void HandleStatusPaneSizeChange();
	
	TBool HandleChangeSelectedSettingItemL( TInt aCommand );
	
private:
	CMobblerSettingItemList* iMobblerSettingItemList;
	CMobblerSettingItemListSettings* iSettings;
	CAknWaitDialog* iConnectingDialog;
	};

#endif
