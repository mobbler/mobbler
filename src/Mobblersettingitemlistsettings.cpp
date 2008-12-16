/*
mobblersettingitemlistsettings.cpp

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

#include <e32base.h>
#include <stringloader.h>
#include <barsread.h>
#include <mobbler.rsg>
#include "mobblersettingitemlistsettings.h"


CMobblerSettingItemListSettings* CMobblerSettingItemListSettings::NewL()
	{
	CMobblerSettingItemListSettings* self = new(ELeave) CMobblerSettingItemListSettings;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);	
	return self;
	}

CMobblerSettingItemListSettings::CMobblerSettingItemListSettings()
	:iBufferSize(KDefaultBufferSizeSeconds)
	{
	}

CMobblerSettingItemListSettings::~CMobblerSettingItemListSettings()
	{
	}

void CMobblerSettingItemListSettings::ConstructL()
	{
	}
	
TDes& CMobblerSettingItemListSettings::Username()
	{
	return iUsername;
	}

void CMobblerSettingItemListSettings::SetUsernameL(const TDesC& aUsername)
	{
	iUsername.Copy(aUsername);
	}

TDes& CMobblerSettingItemListSettings::Password()
	{
	return iPassword;
	}

void CMobblerSettingItemListSettings::SetPasswordL(const TDesC& aPassword)
	{
	iPassword.Copy(aPassword);
	}

TBool& CMobblerSettingItemListSettings::Backlight()
	{
	return iBacklight;
	}

void CMobblerSettingItemListSettings::SetBacklight(TBool aBacklight)
	{
	iBacklight = aBacklight;
	}

TBool& CMobblerSettingItemListSettings::CheckForUpdates()
	{
	return iCheckForUpdates;
	}

void CMobblerSettingItemListSettings::SetCheckForUpdates(TBool aCheckForUpdates)
	{
	iCheckForUpdates = aCheckForUpdates;
	}

TInt& CMobblerSettingItemListSettings::IapID()
	{
	return iIapID;
	}

void CMobblerSettingItemListSettings::SetIapID(TInt aIapID)
	{
	iIapID = aIapID;
	}

TInt& CMobblerSettingItemListSettings::BufferSize()
	{
	return iBufferSize;
	}

void CMobblerSettingItemListSettings::SetBufferSize(TInt aBufferSize)
	{
	iBufferSize = aBufferSize;
	}

