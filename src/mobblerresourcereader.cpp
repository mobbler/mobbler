/*
mobblerresourcereader.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#include <aknnotewrappers.h>
#include <aknutils.h>
#include <barsread.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobblerappui.h"
#include "mobblerresourcereader.h"

_LIT8(KStringNotFoundInResouce, "???");

CMobblerResourceReader* CMobblerResourceReader::NewLC()
	{
	CMobblerResourceReader* self = new (ELeave) CMobblerResourceReader;
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CMobblerResourceReader* CMobblerResourceReader::NewL()
	{
	CMobblerResourceReader* self = NewLC();
	CleanupStack::Pop();
	return self;
	}

void CMobblerResourceReader::ConstructL()
	{
	iErrorDialogShown = EFalse;
	}

CMobblerResourceReader::~CMobblerResourceReader()
	{
	iResourceFile.Close();
	}

void CMobblerResourceReader::AddResourceFileL(const TDesC& aName, TInt aVersion)
	{
	iResourceFile.OpenL(CCoeEnv::Static()->FsSession(), aName);
	TRAPD(error, iResourceFile.ConfirmSignatureL(aVersion));
	
	if (error != KErrNone)
		{
		iResourceFile.Close();

		if (!iErrorDialogShown)
			{
			// Warn the user
			HBufC* text = AllocReadLC(R_MOBBLER_GET_LATEST_LANGUAGE);
			CAknQueryDialog* dlg = CAknQueryDialog::NewL();
			dlg->ExecuteLD(R_MOBBLER_GET_LATEST_LANGUAGE_DIALOG, *text);
			CleanupStack::PopAndDestroy(text);

			iErrorDialogShown = ETrue;
			}
		}
	}

HBufC8* CMobblerResourceReader::AllocRead8LC(TInt aResourceId)
	{
	if (iResourceFile.OwnsResourceId(aResourceId))
		{
		return iResourceFile.AllocReadLC(aResourceId);
		}
	else
		{
		if (!iErrorDialogShown)
			{
			// Warn the user
			HBufC* text = AllocReadLC(R_MOBBLER_GET_LATEST_LANGUAGE);
			CAknQueryDialog* dlg = CAknQueryDialog::NewL();
			dlg->ExecuteLD(R_MOBBLER_GET_LATEST_LANGUAGE_DIALOG, *text);
			CleanupStack::PopAndDestroy(text);

			iErrorDialogShown = ETrue;
			}

		return KStringNotFoundInResouce().AllocLC();
		}
	}

HBufC* CMobblerResourceReader::AllocReadLC(TInt aResourceId)
	{
	HBufC* textBuffer = AllocReadL(aResourceId);
	CleanupStack::PushL(textBuffer);
	return textBuffer;
	}

HBufC* CMobblerResourceReader::AllocReadL(TInt aResourceId)
	{
	HBufC8* readBuffer = AllocRead8LC(aResourceId);
	TResourceReader reader;
	reader.SetBuffer(readBuffer);
	TPtrC textPtr = reader.ReadTPtrC();

	HBufC* textBuffer = HBufC::NewL(textPtr.Length());
	*textBuffer = textPtr;
	CleanupStack::PopAndDestroy(readBuffer);
	return textBuffer;
	}

// End of file
