/*
mobblerdownloadmanager.cpp

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

#include <eikenv.h>
#include <apgcli.h>
#include <aknwaitdialog.h>
#include <mobbler.rsg>
#include "mobblerdownload.h"

TUid KMobblerAppUid = {0xA0007648};

CMobblerDownload* CMobblerDownload::NewL()
	{
	CMobblerDownload* self = new(ELeave) CMobblerDownload;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerDownload::CMobblerDownload()
	{
	}

void CMobblerDownload::ConstructL()
	{
	iDownloadMgr.ConnectL(KMobblerAppUid, *this, EFalse);
    iDownloadMgr.DeleteAll();
	}

CMobblerDownload::~CMobblerDownload()
	{
	if (iWait)
		{
		iWait->ProcessFinishedL();
		}
	    		
	iDownloadMgr.Close();
	}

void CMobblerDownload::DownloadL(const TDesC8& aDownloadUrl, TUint32 aIap)
	{
	User::LeaveIfError(iDownloadMgr.SetIntAttribute(EDlMgrIap, aIap)); 
	RHttpDownload& download = iDownloadMgr.CreateDownloadL(aDownloadUrl);
	download.SetBoolAttribute(EDlAttrNoContentTypeCheck, ETrue);
	download.Start();
	
	// start a progress dialog
	if (iWait)
		{
		iWait->ProcessFinishedL();
		iWait = NULL;
		}
	
	iWait = new(ELeave) CAknWaitDialog((REINTERPRET_CAST(CEikDialog**, &iWait)));
	iWait->ExecuteLD(R_MOBBLER_DOWNLOADING_DIALOG);		
	}

void CMobblerDownload::HandleDMgrEventL(RHttpDownload& aDownload, THttpDownloadEvent aEvent)
	{
    if(EHttpContentTypeReceived == aEvent.iProgressState)
    	{
        // Start download again if content-type is acceptable 
        // and UiLib is not installed
        User::LeaveIfError(aDownload.Start());
        }
    
    switch (aEvent.iDownloadState)
    	{
    	case EHttpDlCompleted:
	    	{
	        TFileName fileName;
	        aDownload.GetStringAttribute(EDlAttrDestFilename, fileName);
	        
	        RFile file;	
	        TInt openError = file.Open(CEikonEnv::Static()->FsSession(), fileName, EFileRead);
	        
	        if (openError == KErrNone)
	        	{
	        	CleanupClosePushL(file);
	            RApaLsSession apaLsSession;
		        CleanupClosePushL(apaLsSession);
		        User::LeaveIfError(apaLsSession.Connect());
		        
		        if (iWait)
		        	{
			        iWait->ProcessFinishedL();
			        iWait = NULL;
		        	}
		        
		        TThreadId threadId;
		        TRequestStatus status;
		        apaLsSession.StartDocument(file, threadId, &status);
		        User::WaitForRequest(status);
		        
		        CleanupStack::PopAndDestroy(&apaLsSession);
		        CleanupStack::PopAndDestroy(&file);
	        	}
	        
	        if (iWait)
	        	{
		        iWait->ProcessFinishedL();
		        iWait = NULL;
	        	}
	    	}
	    	break;
    	case EHttpDlFailed:
    		{
	        if (iWait)
	        	{
		        iWait->ProcessFinishedL();
		        iWait = NULL;
	        	}
    		
	        TInt32 error = 0;
	        TInt32 globalError = 0;
	        aDownload.GetIntAttribute(EDlAttrErrorId, error);
	        aDownload.GetIntAttribute(EDlAttrGlobalErrorId, globalError);
	        }
    		break;
    	default:
    		break;
    	};
	}
