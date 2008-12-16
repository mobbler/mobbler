/*
mobblertransaction.cpp

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

#include <http/mhttpdatasupplier.h> 
#include <httpstringconstants.h>
#include <http/mhttpdatasupplier.h>
#include <http/mhttpfilter.h>
#include <http/mhttpfilterbase.h>
#include <http/mhttptransactioncallback.h>
#include <http/rhttpconnectioninfo.h>
#include <http/rhttpfiltercollection.h>
#include <http/rhttpheaders.h>
#include <http/rhttpmessage.h>
#include <http/rhttppropertyset.h>
#include <http/rhttprequest.h>
#include <http/rhttpresponse.h>
#include <http/rhttpsession.h>
#include <http/rhttptransaction.h>
#include <http/rhttptransactionpropertyset.h>
#include <http/thttpevent.h>
#include <http/thttpfilterhandle.h>
#include <http/thttpfilteriterator.h>
#include <http/thttpfilterregistration.h>
#include <http/thttphdrfielditer.h>
#include <http/thttphdrval.h>
#include <chttpformencoder.h> 

#include "mobblertransaction.h"
#include "mobblerwebservicesobserver.h"

// The granurarity of the buffer that responses from last.fm are read into
const TInt KBufferGranularity(256);

CMobblerTransaction* CMobblerTransaction::NewL(RHTTPSession& aSession, const TUriC8& aURI, MMobblerTransactionObserver& aObserver)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aSession, aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aURI);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(RHTTPSession& aSession, const TUriC8& aURI, MMobblerTransactionObserver& aObserver, CHTTPFormEncoder* aForm)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aSession, aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aURI, aForm);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction::CMobblerTransaction(RHTTPSession& aSession, MMobblerTransactionObserver& aObserver)
	:iSession(aSession), iObserver(aObserver)
	{
	}

void CMobblerTransaction::ConstructL(const TUriC8& aURI)
	{
	iBuffer = CBufFlat::NewL(KBufferGranularity);
	
	iTransaction = iSession.OpenTransactionL(aURI, *this);
	iTransaction.SubmitL();
	}

void CMobblerTransaction::ConstructL(const TUriC8& aURI, CHTTPFormEncoder* aForm)
	{
	iBuffer = CBufFlat::NewL(KBufferGranularity);
	
	// get the post string
	RStringF string;
	RStringPool stringPool = iSession.StringPool();
	string = stringPool.StringF(HTTP::EPOST, RHTTPSession::GetTable());
	
	iTransaction = iSession.OpenTransactionL(aURI, *this, string);
	iTransaction.Request().SetBody(*aForm);
	
	// get the header
	RStringF contentType = stringPool.OpenFStringL(_L8("application/x-www-form-urlencoded"));
	THTTPHdrVal accVal(contentType);
	iTransaction.Request().GetHeaderCollection().SetFieldL(stringPool.StringF(HTTP::EContentType, RHTTPSession::GetTable()), accVal);
	contentType.Close();
	
	iTransaction.SubmitL();
	
	// take ownership of the form
	iForm = aForm;
	}

CMobblerTransaction::~CMobblerTransaction()
	{
	iTransaction.Close();
	delete iBuffer;
	delete iForm;
	}

void CMobblerTransaction::SetWebServicesObserver(MWebServicesObserver& aWebServicesObserver)
	{
	iWebServicesObserver = &aWebServicesObserver;
	}

void CMobblerTransaction::Cancel()
	{
	iTransaction.Cancel();
	}

void CMobblerTransaction::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{
	// it must be a transaction event
	TPtrC8 nextDataPartPtr;
	
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseBodyData:
			aTransaction.Response().Body()->GetNextDataPart(nextDataPartPtr);
			iBuffer->InsertL(iBuffer->Size(), nextDataPartPtr);
			aTransaction.Response().Body()->ReleaseData();
			break;
		case THTTPEvent::ESucceeded:
			HBufC8* response = iBuffer->Ptr(0).AllocLC();
			
			iObserver.TransactionResponseL(this, *response);
			
			if (iWebServicesObserver)
				{
				iWebServicesObserver->WebServicesResponseL(*response);
				}
			
			CleanupStack::PopAndDestroy(response);
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			iObserver.TransactionCompleteL(this);
			break;
		case THTTPEvent::EFailed:
			iObserver.TransactionFailedL(this, iTransaction.Response().StatusText().DesC(), iTransaction.Response().StatusCode());			
			break;
		default:
			break;
		}
	}

TInt CMobblerTransaction::MHFRunError(TInt aError, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
	_LIT8(KMHFRunError, "MHFRunError");
	iObserver.TransactionFailedL(this, KMHFRunError, aError);
	return KErrNone;
	}
	

