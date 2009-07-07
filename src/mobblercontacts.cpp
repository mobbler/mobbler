/*
mobblercontacts.cpp

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

#include "mobblercontacts.h"

CMobblerContacts* CMobblerContacts::NewLC()
	{
	CMobblerContacts* self = new(ELeave) CMobblerContacts();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CMobblerContacts::CMobblerContacts()
	{
	}

CMobblerContacts::~CMobblerContacts()
	{
	delete iNameList;
	delete iEmailList;
	delete iDb;
	}

void CMobblerContacts::ConstructL()
	{
	iDb = CContactDatabase::OpenL();
	RContactViewSortOrder sortOrder;
	CleanupClosePushL(sortOrder);
	sortOrder.AppendL(KUidContactFieldGivenName);
	sortOrder.AppendL(KUidContactFieldFamilyName);
	sortOrder.AppendL(KUidContactFieldCompanyName);
	sortOrder.AppendL(KUidContactFieldEMail);

	iRemoteView = CContactNamedRemoteView::NewL(*this, _L("Mobbler"), *iDb, sortOrder, EContactsOnly);
	iFilteredView = CContactFilteredView::NewL(*this, *iDb, *iRemoteView, CContactDatabase::EMailable);

	CleanupStack::PopAndDestroy(&sortOrder);
	CActiveScheduler::Start();
	}

TInt CMobblerContacts::Count() const
	{
	return iNameList->Count();
	}

TPtrC CMobblerContacts::GetNameAt(TInt aIndex) const
	{
	return (*iNameList)[aIndex];
	}

TPtrC CMobblerContacts::GetEmailAt(TInt aIndex) const
	{
	return (*iEmailList)[aIndex];
	}

void CMobblerContacts::BuildListsL()
	{
	const TInt KNumContacts(iFilteredView->CountL());
	iNameList = new(ELeave) CDesCArrayFlat(KNumContacts);
	iEmailList = new(ELeave) CDesCArrayFlat(KNumContacts);
	
	for (TInt i = 0; i < KNumContacts; ++i)
		{
		const CViewContact& contact = iFilteredView->ContactAtL(i);
		HBufC* nameBuf = HBufC::NewLC(contact.Field(EFirstName).Length()
				+ contact.Field(ELastName).Length() + 1); // +1 for a space char separator 
		TPtr namePtr = nameBuf->Des();
		_LIT(KFormatStr, "%S %S");
		TPtrC fnamePtr(contact.Field(EFirstName));
		TPtrC lnamePtr(contact.Field(ELastName));
		namePtr.AppendFormat(KFormatStr, &fnamePtr, &lnamePtr);
		namePtr.TrimAll();
	
		if(namePtr.Length())
			{
			iNameList->AppendL(namePtr);
			}
		else
			{
			HBufC* companyBuf = HBufC::NewLC(contact.Field(ECompanyName).Length());
			TPtr companyPtr = companyBuf->Des();
			companyPtr.TrimAll();

			if (companyPtr.Length())
				{
				iNameList->AppendL(companyPtr);
				}
			else
				{
				_LIT(KHyphen, "-");
				iNameList->AppendL(KHyphen);
				}

			CleanupStack::PopAndDestroy(companyBuf);
			}

		CleanupStack::PopAndDestroy(nameBuf);
		iEmailList->AppendL(contact.Field(EEmail));
		}
	iFilteredView->Close(*this);
	iRemoteView->Close(*this);
	}

void CMobblerContacts::HandleContactViewEvent(const CContactViewBase& aView, const TContactViewEvent& aEvent)
	{
	if (&aView == iRemoteView && aEvent.iEventType == TContactViewEvent::EReady)
		{
		++iNumViews;
		}		

	else if (&aView == iFilteredView && aEvent.iEventType == TContactViewEvent::EReady)
		{
		++iNumViews;
		}		
	
	// wait until both the views are ready and then build the lists 
	if (iNumViews == 2)
		{
		BuildListsL();
		CActiveScheduler::Stop();
		}		
	}
