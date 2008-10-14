/*
mobblertransaction.h

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

#ifndef __MOBBLERTRANSACTION_H__
#define __MOBBLERTRANSACTION_H__

#include <e32base.h>
#include <badesca.h>
#include <es_sock.h>

#include <http/rhttpsession.h>
#include <http/mhttptransactioncallback.h> 
#include <http/mhttpdatasupplier.h>

#include "mobblerlastfmconnection.h"
#include "mobblerutility.h"
#include "mobblertransactionobserver.h"

class CHTTPFormEncoder;
class CMobblerTrack;
class MMobblerRadioPlayer;
class CMobblerParser;
class CMobblerTransaction;
class MWebServicesObserver;

class CMobblerTransaction : public CBase, public MHTTPTransactionCallback
	{
public:
	static CMobblerTransaction* NewL(RHTTPSession& aSession, const TUriC8& aURI, MMobblerTransactionObserver& aObserver);
	static CMobblerTransaction* NewL(RHTTPSession& aSession, const TUriC8& aURI, MMobblerTransactionObserver& aObserver, CHTTPFormEncoder* aForm);
	~CMobblerTransaction();
	
	void Cancel();
	
	void SetWebServicesObserver(MWebServicesObserver& aWebServicesObserver);
	
private: // from MHTTPTransactionCallback
	void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent);
	TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	
private:
	CMobblerTransaction(RHTTPSession& aHTTPSession, MMobblerTransactionObserver& aObserver);
	void ConstructL(const TUriC8& aURI);
	void ConstructL(const TUriC8& aURI, CHTTPFormEncoder* aForm);
	
private:
	RHTTPSession& iSession;
	
	MMobblerTransactionObserver& iObserver;
	
	RHTTPTransaction iTransaction;
	CBufBase* iBuffer;
	CHTTPFormEncoder* iForm;
	
	MWebServicesObserver* iWebServicesObserver;
	};

#endif // __MOBBLERTRANSACTION_H__
