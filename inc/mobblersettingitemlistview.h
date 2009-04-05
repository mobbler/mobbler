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

#include "mobblersettingitemlistsettings.h"

class CAknTextSettingItem;
class CMobblerResourceReader;
class CMobblerSettingItemList;

class CMobblerSettingItemListView : public CAknView
	{
public:
	static CMobblerSettingItemListView* NewL();
	~CMobblerSettingItemListView();
	
	TUid Id() const;
	void HandleCommandL(TInt aCommand);
	
	const TDesC& Username() const	{ return iSettings->Username(); }
	const TDesC& Password() const	{ return iSettings->Password(); }
	TBool Backlight() const			{ return iSettings->Backlight(); }
	TBool CheckForUpdates() const	{ return iSettings->CheckForUpdates(); }
	TUint32 IapID() const			{ return iSettings->IapID(); };
	TUint8 BufferSize() const		{ return iSettings->BufferSize(); }
	TBool EqualizerIndex() const	{ return iSettings->EqualizerIndex(); }
	TInt ScrobblePercent() const	{ return iSettings->ScrobblePercent(); }
	TInt Volume() const				{ return iSettings->Volume(); }
	TInt SleepTimerMinutes() const 	{ return iSettings->SleepTimerMinutes(); }
	TTime NextUpdateCheck()			{ return iSettings->NextUpdateCheck(); }
	CMobblerLastFMConnection::TMode Mode() { return iSettings->Mode(); }
	TInt DownloadAlbumArt() const	{ return iSettings->DownloadAlbumArt(); }
	TBool AccelerometerGestures() { return iSettings->AccelerometerGestures(); }
	
	void SetEqualizerIndexL(TInt aIndex);
	void SetVolumeL(TInt aVolume);
	void SetSleepTimerMinutesL(TInt aSleepTimerMinutes);
	void SetNextUpdateCheckL(TTime aNextUpdateCheck);
	void SetModeL(CMobblerLastFMConnection::TMode aMode);
	
private:
	CMobblerSettingItemListView();        
	void ConstructL();

	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void DoDeactivate();
	void HandleStatusPaneSizeChange();
	
	TBool HandleChangeSelectedSettingItemL(TInt aCommand);
	
	void LoadListL();
	void CreateTextItemL(TDes& aText, 
							const TInt aTitleResource, 
							const TInt aPageResource);
	void CreatePasswordItemL(TDes& aPassword, 
							const TInt aTitleResource, 
							const TInt aPageResource);
	void CreateIapItemL(TInt& aIapId, 
							const TInt aTitleResource, 
							const TInt aPageResource);
	void CreateSliderItemL(TInt& aSliderValue, 
							const TInt aTitleResource, 
							const TInt aPageResource, 
							const TInt aResourceSingular, 
							const TInt aResourcePlural);
	void CreateBinaryItemL(TBool& aBinaryValue, 
							const TInt aTitleResource, 
							const TInt aPageResource,
							const TInt aFirstEnumResource,
							const TInt aSecondEnumResource);
	void CreateEnumItemL(TInt& aEnumId, 
							const TInt aTitleResource, 
							const TInt aPageResource);

private:
	CMobblerSettingItemList* iMobblerSettingItemList;
	CMobblerSettingItemListSettings* iSettings;
	CAknWaitDialog* iConnectingDialog;

	TInt iOrdinal;
	TBool iIsNumberedStyle;
	CArrayPtr<CGulIcon>* iIcons;
	};

#endif

// End of file
