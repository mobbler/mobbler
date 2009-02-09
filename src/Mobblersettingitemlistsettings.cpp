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
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TBuf<255> username;
		TBuf<255> password;
		
		readStream >> username;
		readStream >> password;

		 // Ignore KErrEof if these settings are not yet saved in the file
		TRAP_IGNORE(backlight = readStream.ReadInt8L());
		TRAP_IGNORE(autoUpdatesOn = readStream.ReadInt8L());
		TRAP_IGNORE(iapId = readStream.ReadUint32L());
		TRAP_IGNORE(bufferSize = readStream.ReadUint8L());
		TRAP_IGNORE(equalizerIndex = readStream.ReadInt16L());
		TRAP_IGNORE(scrobblePercent = readStream.ReadInt16L());
		TRAP_IGNORE(volume = readStream.ReadInt16L());
		TRAP_IGNORE(sleepTimerMinutes = readStream.ReadInt16L());

		SetUsernameL(username);
		SetPasswordL(password);

		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		// there was no file there so read from the resource file
		CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
		resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);
		HBufC* username  = resourceReader->AllocReadLC(R_MOBBLER_USERNAME);
		delete resourceReader;
		SetUsernameL(*username);
		CleanupStack::PopAndDestroy(username);
		SetPasswordL(_L("password"));
		}

	SetBacklight(backlight);
	SetCheckForUpdates(autoUpdatesOn);
	SetIapID(iapId);
	SetBufferSize(bufferSize);
	SetEqualizerIndex(equalizerIndex);
	SetScrobblePercent(scrobblePercent);
	SetVolume(volume);
	SetSleepTimerMinutes(sleepTimerMinutes);

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
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
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

TInt& CMobblerSettingItemListSettings::EqualizerIndex()
	{
	return iEqualizerIndex;
	}

void CMobblerSettingItemListSettings::SetEqualizerIndex(TInt aIndex)
	{
	iEqualizerIndex = aIndex;
	}

TInt& CMobblerSettingItemListSettings::ScrobblePercent()
	{
	return iScrobblePercent;
	}

void CMobblerSettingItemListSettings::SetScrobblePercent(TInt aScrobblePercent)
	{
	iScrobblePercent = aScrobblePercent;
	}

TInt& CMobblerSettingItemListSettings::Volume()
	{
	return iVolume;
	}

void CMobblerSettingItemListSettings::SetVolume(TInt aVolume)
	{
	iVolume = aVolume;
	}

TInt& CMobblerSettingItemListSettings::SleepTimerMinutes()
	{
	return iSleepTimerMinutes;
	}

void CMobblerSettingItemListSettings::SetSleepTimerMinutes(TInt aSleepTimerMinutes)
	{
	iSleepTimerMinutes = aSleepTimerMinutes;
	}

// End of file
