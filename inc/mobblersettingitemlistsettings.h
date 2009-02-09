/*
mobblersettingitemlistsettings.h

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

#ifndef __MOBBLERSETTINGITEMLISTSETTINGS_H__
#define __MOBBLERSETTINGITEMLISTSETTINGS_H__

#include <e32std.h>

_LIT(KSettingsFile, "c:settings.ini");

class CMobblerSettingItemListSettings : public CBase
	{
public:
	static CMobblerSettingItemListSettings* NewL();
	~CMobblerSettingItemListSettings();
		
private:
	CMobblerSettingItemListSettings();
	void ConstructL();

public:
	void LoadSettingValuesL();
	void SaveSettingValuesL();
	
public:
	TDes& Username();
	void SetUsernameL(const TDesC& aUserName);
	
	TDes& Password();
	void SetPasswordL(const TDesC& aPassword);
	
	TBool& Backlight();
	void SetBacklight(TBool aBacklight);
	
	TBool& CheckForUpdates();
	void SetCheckForUpdates(TBool aCheckForUpdates);
	
	TInt& IapID();
	void SetIapID(TInt aIapID);
	
	TInt& BufferSize();
	void SetBufferSize(TInt aBufferSize);
	
	TInt& EqualizerIndex();
	void SetEqualizerIndex(TInt aIndex);
	
	TInt& ScrobblePercent();
	void SetScrobblePercent(TInt aIndex);

	TInt& Volume();
	void SetVolume(TInt aVolume);

	TInt& SleepTimerMinutes();
	void SetSleepTimerMinutes(TInt aSleepTimerMinutes);

protected:
	TBuf<30> iUsername;
	TBuf<30> iPassword;
	TBool iBacklight;
	TBool iCheckForUpdates;
	TInt iIapID;
	TInt iBufferSize;
	TInt iEqualizerIndex;
	TInt iScrobblePercent;
	TInt iVolume;
	TInt iSleepTimerMinutes;
	};

#endif // __MOBBLERSETTINGITEMLISTSETTINGS_H__

// End of file
