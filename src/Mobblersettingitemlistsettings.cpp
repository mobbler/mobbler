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

#include <coemain.h>
#include <mobbler_strings.rsg>

#include "mobblerappui.h"
#include "mobblerstring.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistsettings.h"

const TInt KDefaultBufferSizeSeconds(5);
const TInt KDefaultScrobblePercent(50);
const TInt KDefaultEqualizerIndex(-1);
#ifdef __WINS__
const TInt KDefaultVolume(0);
#else
const TInt KDefaultVolume(5);
#endif
const TInt KDefaultSleepTimerMinutes(30);

CMobblerSettingItemListSettings* CMobblerSettingItemListSettings::NewL()
	{
	CMobblerSettingItemListSettings* self = new(ELeave) CMobblerSettingItemListSettings;
	return self;
	}

CMobblerSettingItemListSettings::CMobblerSettingItemListSettings()
	:iBufferSize(KDefaultBufferSizeSeconds)
	{
	}

CMobblerSettingItemListSettings::~CMobblerSettingItemListSettings()
	{
	}

void CMobblerSettingItemListSettings::LoadSettingValuesL()
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError = file.Open(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileRead | EFileShareAny);
	
	// Default values if the settings do not already exist
	TBool backlight(EFalse);
	TBool autoUpdatesOn(ETrue);
	TUint32 iapId(0);
	TUint8 bufferSize(KDefaultBufferSizeSeconds);
	TInt equalizerIndex(KDefaultEqualizerIndex);
	TInt scrobblePercent(KDefaultScrobblePercent);
	TInt volume(KDefaultVolume);
	TInt sleepTimerMinutes(KDefaultSleepTimerMinutes);
	TTime nextUpdateCheck;
	nextUpdateCheck.UniversalTime();
	nextUpdateCheck += TTimeIntervalDays(7); // the default update check should be 7 days after install
	CMobblerLastFMConnection::TMode mode(CMobblerLastFMConnection::EOffline);
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TBuf<255> username;
		TBuf<255> password;

		 // Ignore KErrEof if these settings are not yet saved in the file
		TRAP_IGNORE(readStream >> username);
		TRAP_IGNORE(readStream >> password);
		TRAP_IGNORE(backlight = readStream.ReadInt8L());
		TRAP_IGNORE(autoUpdatesOn = readStream.ReadInt8L());
		TRAP_IGNORE(iapId = readStream.ReadUint32L());
		TRAP_IGNORE(bufferSize = readStream.ReadUint8L());
		TRAP_IGNORE(equalizerIndex = readStream.ReadInt16L());
		TRAP_IGNORE(scrobblePercent = readStream.ReadInt16L());
		TRAP_IGNORE(volume = readStream.ReadInt16L());
		TRAP_IGNORE(sleepTimerMinutes = readStream.ReadInt16L());
		
		TUint32 high(0);
		TUint32 low(0);
		TRAPD(errorHigh, high = readStream.ReadInt32L());
		TRAPD(errorLow, low = readStream.ReadInt32L());
		if (errorHigh == KErrNone && errorLow == KErrNone)
			{
			nextUpdateCheck = TTime(MAKE_TINT64(high, low));
			}
		
		TRAP_IGNORE(mode = static_cast<CMobblerLastFMConnection::TMode>(readStream.ReadInt8L()));
		
		SetUsername(username);
		SetPassword(password);

		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		// there was no file there so read from the resource file
		SetUsername(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_USERNAME));
		SetPassword(_L("password"));
		}

	SetBacklight(backlight);
	SetCheckForUpdates(autoUpdatesOn);
	SetIapID(iapId);
	SetBufferSize(bufferSize);
	SetEqualizerIndex(equalizerIndex);
	SetScrobblePercent(scrobblePercent);
	SetVolume(volume);
	SetSleepTimerMinutes(sleepTimerMinutes);
	SetNextUpdateCheck(nextUpdateCheck);
	SetMode(mode);

	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerSettingItemListSettings::SaveSettingValuesL()
	{
	RFile file;
	CleanupClosePushL(file);
	CCoeEnv::Static()->FsSession().MkDirAll(KSettingsFile);
	TInt error = file.Replace(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileWrite | EFileShareAny);
	
	if (error == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		writeStream << Username();
		writeStream << Password();
		writeStream.WriteInt8L(Backlight());
		writeStream.WriteInt8L(CheckForUpdates());
		writeStream.WriteUint32L(IapID());
		writeStream.WriteUint8L(BufferSize());
		writeStream.WriteInt16L(EqualizerIndex());
		writeStream.WriteInt16L(ScrobblePercent());
		writeStream.WriteInt16L(Volume());
		writeStream.WriteInt16L(SleepTimerMinutes());
		writeStream.WriteInt32L(I64HIGH(NextUpdateCheck().Int64()));
		writeStream.WriteInt32L(I64LOW(NextUpdateCheck().Int64()));
		writeStream.WriteInt8L(Mode());
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}
	
// End of file
