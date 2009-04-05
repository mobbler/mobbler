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
#include <barsread.h>
#include <bautils.h> 
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobblerresourcereader.h"

#if defined(__WINS__)
_LIT(KLanguageRscFile, "\\Resource\\apps\\mobbler_strings.r01");
_LIT(KLanguageRscFile2,"\\Resource\\apps\\mobbler_strings2.r01");
#else
_LIT(KLanguageRscFile, "\\Resource\\apps\\mobbler_strings.rsc");
_LIT(KLanguageRscFile2,"\\Resource\\apps\\mobbler_strings2.rsc");
#endif
	
const TInt KLanguageRscVersion(1);

_LIT(KStringNotFoundInResouce, "???");

const TTimeIntervalMicroSeconds32 KTimeoutPeriod(1000000);

CMobblerResourceReader::CMobblerResource::CMobblerResource(TInt aResourceId, HBufC* aString)
	:iResourceId(aResourceId), iString(aString)
	{
	}

CMobblerResourceReader::CMobblerResource::~CMobblerResource()
	{
	delete iString;
	}

TInt CMobblerResourceReader::CMobblerResource::Compare(const TInt* aResourceId, const CMobblerResource& aResource)
	{
	return *aResourceId - aResource.iResourceId;
	}

TInt CMobblerResourceReader::CMobblerResource::Compare(const CMobblerResource& aLeft, const CMobblerResource& aRight)
	{
	return Compare(&aLeft.iResourceId, aRight);
	}

const TDesC& CMobblerResourceReader::CMobblerResource::String() const
	{
	return *iString;
	}

CMobblerResourceReader* CMobblerResourceReader::NewL()
	{
	CMobblerResourceReader* self = new(ELeave) CMobblerResourceReader();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerResourceReader::CMobblerResourceReader()
	:CActive(CActive::EPriorityStandard), iLinearOrder(CMobblerResource::Compare)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerResourceReader::ConstructL()
	{
	iStringNotFoundInResouce = KStringNotFoundInResouce().AllocL();
	
	User::LeaveIfError(iTimer.CreateLocal());
	
	TParse parse;
	TFileName appFullName = RProcess().FileName();
	parse.Set(appFullName, NULL, NULL);
	
	iLanguageRscFile.Append(parse.Drive());
	iLanguageRscFile.Append(KLanguageRscFile);
	
	iLanguageRscFile2.Append(parse.Drive());
	iLanguageRscFile2.Append(KLanguageRscFile2);
	}

CMobblerResourceReader::~CMobblerResourceReader()
	{
	Cancel();
	iTimer.Close();
	
	iResourceFile.Close();
	
	delete iStringNotFoundInResouce;
	iResources.ResetAndDestroy();
	}

void CMobblerResourceReader::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		iResourceFile.Close();
		}
	}

void CMobblerResourceReader::DoCancel()
	{
	iTimer.Cancel();
	}

const TDesC& CMobblerResourceReader::ResourceL(TInt aResourceId)
	{
	TInt position = iResources.FindInOrder(aResourceId, CMobblerResource::Compare);
	
	if (position != KErrNotFound)
		{
		return iResources[position]->String();
		}
	else
		{
		// The string has not already been read so do it now
		
		if (!IsActive())
			{
			// We are not active so the file must be closed
			
			TRAPD(error, iResourceFile.OpenL(CCoeEnv::Static()->FsSession(), iLanguageRscFile2));
			if (error != KErrNone)
				{
				iResourceFile.OpenL(CCoeEnv::Static()->FsSession(), iLanguageRscFile);
				}
			
			TRAP(error, iResourceFile.ConfirmSignatureL(KLanguageRscVersion));
			
			if (error != KErrNone)
				{
				if (!iErrorDialogShown)
					{
					// Warn the user
					CAknQueryDialog* dlg = CAknQueryDialog::NewL();
					dlg->ExecuteLD(R_MOBBLER_GET_LATEST_LANGUAGE_DIALOG, 
								   ResourceL(R_MOBBLER_GET_LATEST_LANGUAGE));
			
					iErrorDialogShown = ETrue;
					}
				
				// There was an error so just return
				return *iStringNotFoundInResouce;
				}
			
			// Close the file sometime later
			iTimer.After(iStatus, KTimeoutPeriod);
			SetActive();
			}
		else
			{
			// We are active so the file must be open, but reset the timer
			Cancel();
			iTimer.After(iStatus, KTimeoutPeriod);
			SetActive();
			}
		
		// The resource file must be sucessfully open here
	
		if (iResourceFile.OwnsResourceId(aResourceId))
			{
			HBufC8* resource8 = iResourceFile.AllocReadLC(aResourceId);
			TResourceReader reader;
			reader.SetBuffer(resource8);
			HBufC* text = reader.ReadTPtrC().AllocLC();
			CMobblerResource* resource = new(ELeave) CMobblerResource(aResourceId, text);
			CleanupStack::Pop(text);
			CleanupStack::PopAndDestroy(resource8);
			CleanupStack::PushL(resource);
			iResources.InsertInOrderL(resource, iLinearOrder);
			CleanupStack::Pop(resource);
			return resource->String();
			}
		else
			{
			if (!iErrorDialogShown)
				{
				// Warn the user
				CAknQueryDialog* dlg = CAknQueryDialog::NewL();
				dlg->ExecuteLD(R_MOBBLER_GET_LATEST_LANGUAGE_DIALOG, 
							   ResourceL(R_MOBBLER_GET_LATEST_LANGUAGE));
	
				iErrorDialogShown = ETrue;
				
				return *iStringNotFoundInResouce;
				}
			}
		}
	
	return *iStringNotFoundInResouce;
	}

// End of file
