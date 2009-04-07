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
#include "mobblerwebservicesquery.h"

// The granurarity of the buffer that responses from last.fm are read into
const TInt KBufferGranularity(256);

_LIT8(KRadioStationQuery, "session=%S&url=%S&lang=%S");
_LIT8(KRadioPlaylistQuery, "sk=%S&discovery=0&desktop=1.5.1");

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFMConnection& aConnection, CUri8* aURI)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aConnection);
	CleanupStack::PushL(self);
	self->ConstructL(aURI);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFMConnection& aConnection, CUri8* aURI, CHTTPFormEncoder* aForm)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aConnection);
	CleanupStack::PushL(self);
	self->ConstructL(aURI, aForm);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFMConnection& aConnection, CUri8* aURI, CMobblerWebServicesQuery* aQuery)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aConnection);
	CleanupStack::PushL(self);
	self->ConstructL(aURI, aQuery);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFMConnection& aConnection, const TDesC8& aLastFMRadioURI)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aConnection);
	CleanupStack::PushL(self);
	self->ConstructL(aLastFMRadioURI);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction* CMobblerTransaction::NewL(CMobblerLastFMConnection& aConnection)
	{
	CMobblerTransaction* self = new(ELeave) CMobblerTransaction(aConnection);
	//CleanupStack::PushL(self);
	//self->ConstructL();
	//CleanupStack::Pop(self);
	return self;
	}

CMobblerTransaction::CMobblerTransaction(CMobblerLastFMConnection& aConnection)
	:iConnection(aConnection)
	{
	}

void CMobblerTransaction::ConstructL(CUri8* aURI)
	{
	iURI = aURI;
	}

void CMobblerTransaction::ConstructL(CUri8* aURI, CHTTPFormEncoder* aForm)
	{
	iForm = aForm;
	iURI = aURI;
	}

void CMobblerTransaction::ConstructL(CUri8* aURI, CMobblerWebServicesQuery* aQuery)
	{
	iQuery = aQuery;
	iURI = aURI;
	}

void CMobblerTransaction::ConstructL(const TDesC8& aLastFMRadioURI)
	{
	iLastFMRadioURI = aLastFMRadioURI.AllocL();
	}

void CMobblerTransaction::SubmitL()
	{
	delete iBuffer;
	iBuffer = CBufFlat::NewL(KBufferGranularity);
	
	if (iLastFMRadioURI)
		{
		// this is a start radio transaction

		// setup the path
		HBufC8* path = HBufC8::NewLC(255);
		TPtr8 pathPtr(path->Des());
		pathPtr.Copy(*iConnection.iRadioBasePath);
		pathPtr.Append(_L8("/adjust.php"));
		
		TBuf8<2> language = MobblerUtility::LanguageL();
		
		HBufC8* query = HBufC8::NewLC(255);
		
		// setup the 
		TPtr8 radioSessionIDPtr(iConnection.iRadioSessionID->Des());
		TPtr8 radioURLPtr(iLastFMRadioURI->Des());
		
		query->Des().AppendFormat(KRadioStationQuery, &radioSessionIDPtr, &radioURLPtr, &language);
		
		CUri8* uri = CUri8::NewLC();
		
		uri->SetComponentL(_L8("http"), EUriScheme);
		uri->SetComponentL(*iConnection.iRadioBaseURL, EUriHost);
		uri->SetComponentL(pathPtr, EUriPath);
		uri->SetComponentL(*query, EUriQuery);

		iTransaction = iConnection.iHTTPSession.OpenTransactionL(uri->Uri(), *this);

		CleanupStack::PopAndDestroy(3, path);
		}
	else if (iURI)
		{
		if (iQuery)
			{
			// we were passed a query so add the session key
			iQuery->AddFieldL(_L8("sk"), *iConnection.iWebServicesSessionKey);
			
			iForm = iQuery->GetFormLC();
			CleanupStack::Pop(iForm);
			delete iQuery;
			iQuery = NULL;
			}
		
		if (iForm)
			{
			// get the post string
			RStringF string;
			RStringPool stringPool = iConnection.iHTTPSession.StringPool();
			string = stringPool.StringF(HTTP::EPOST, RHTTPSession::GetTable());
			
			iTransaction = iConnection.iHTTPSession.OpenTransactionL(iURI->Uri(), *this, string);
			iTransaction.Request().SetBody(*iForm);
			
			// get the header
			RStringF contentType = stringPool.OpenFStringL(_L8("application/x-www-form-urlencoded"));
			THTTPHdrVal accVal(contentType);
			iTransaction.Request().GetHeaderCollection().SetFieldL(stringPool.StringF(HTTP::EContentType, RHTTPSession::GetTable()), accVal);
			contentType.Close();
			}
		else
			{
			iTransaction = iConnection.iHTTPSession.OpenTransactionL(iURI->Uri(), *this);
			}
		}
	else
		{
		// this must be requesting a playlist
		
		// only try to request a playlist if we have the session id
		HBufC8* path = HBufC8::NewLC(255);
		TPtr8 pathPtr(path->Des());
		pathPtr.Copy(*iConnection.iRadioBasePath);
		pathPtr.Append(_L8("/xspf.php"));
		
		TPtr8 radioSessionIDPtr(iConnection.iRadioSessionID->Des());
		
		HBufC8* query = HBufC8::NewLC(255);
		query->Des().AppendFormat(KRadioPlaylistQuery, &radioSessionIDPtr);
		
		CUri8* uri = CUri8::NewLC();
		
		uri->SetComponentL(_L8("http"), EUriScheme);
		uri->SetComponentL(*iConnection.iRadioBaseURL, EUriHost);
		uri->SetComponentL(pathPtr, EUriPath);
		uri->SetComponentL(*query, EUriQuery);
		
		iTransaction = iConnection.iHTTPSession.OpenTransactionL(uri->Uri(), *this);
		
		CleanupStack::PopAndDestroy(3, path);
		}
	
	iTransaction.SubmitL();
	}

CMobblerTransaction::~CMobblerTransaction()
	{
	iTransaction.Close();
	delete iBuffer;
	delete iForm;
	delete iURI;
	delete iLastFMRadioURI;
	delete iQuery;
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
			{
			HBufC8* response = iBuffer->Ptr(0).AllocLC();
			iConnection.TransactionResponseL(this, *response);
			CleanupStack::PopAndDestroy(response);
			}
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			iConnection.TransactionCompleteL(this);
			break;
		case THTTPEvent::EFailed:
			iConnection.TransactionFailedL(this, iTransaction.Response().StatusText().DesC(), iTransaction.Response().StatusCode());
			break;
		default:
			break;
		}
	}

TInt CMobblerTransaction::MHFRunError(TInt aError, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
	_LIT8(KMHFRunError, "MHFRunError");
	iConnection.TransactionFailedL(this, KMHFRunError, aError);
	return KErrNone;
	}

void CMobblerTransaction::SetFlatDataObserver(MMobblerFlatDataObserver* aFlatDataObserver)
	{
	iFlatDataObserver = aFlatDataObserver;
	}

MMobblerFlatDataObserver* CMobblerTransaction::FlatDataObserver()
	{
	return iFlatDataObserver;
	}
	

