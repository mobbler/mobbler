/*
mobblercontentlisting.cpp

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

#include <ContentListingFactory.h>
#include <ecom/implementationproxy.h>
#include <MCLFContentListingEngine.h>
#include <MCLFItem.h>
#include <MCLFItemListModel.h>
#include <MCLFSortingStyle.h>

#include "mobblercontentlisting.h"

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x2002661E};
#else
const TInt KImplementationUid = {0xA000BEB3};
#endif

const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerContentListing::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

void DoContentListingRefreshL(CMobblerContentListing::TSharedData* aSharedData)
	{
	// Create Content Listing Engine and a list model
	aSharedData->iClfEngine = ContentListingFactory::NewContentListingEngineLC();
	aSharedData->iClfModel = aSharedData->iClfEngine->CreateListModelLC(*aSharedData->iObserver);
	
	// Create an array for the desired media types
	RArray<TInt> array;
	CleanupClosePushL(array);
	array.AppendL(ECLFMediaTypeMusic);
	
	// Set wanted media types array to the model
	aSharedData->iClfModel->SetWantedMediaTypesL(array.Array());
	CleanupStack::PopAndDestroy(&array);
	
	// set the sort order
	aSharedData->iSortingStyle = ContentListingFactory::NewSortingStyleLC();
	aSharedData->iSortingStyle->AddFieldL(ECLFFieldIdSongName);
	aSharedData->iSortingStyle->SetSortingDataType(ECLFItemDataTypeDesC);
	aSharedData->iSortingStyle->SetOrdering(ECLFOrderingAscending);
	aSharedData->iSortingStyle->SetUndefinedItemPosition(ECLFSortingStyleUndefinedEnd);
	aSharedData->iClfModel->SetSortingStyle(aSharedData->iSortingStyle);
	
	aSharedData->iSecSortingStyle = ContentListingFactory::NewSortingStyleLC();
	aSharedData->iSecSortingStyle->AddFieldL(ECLFFieldIdArtist);
	aSharedData->iSecSortingStyle->SetSortingDataType(ECLFItemDataTypeDesC);
	aSharedData->iSecSortingStyle->SetOrdering(ECLFOrderingAscending);
	aSharedData->iClfModel->AppendSecondarySortingStyleL(*aSharedData->iSecSortingStyle);
	
	do
		{
		aSharedData->iClfModel->RefreshL();
		CActiveScheduler::Start();
		RThread().Rendezvous(KErrNone);
		RThread().Suspend();
		}
		while (aSharedData->iState == CMobblerContentListing::EMobblerClfModelRefreshing);
	
	CleanupStack::PopAndDestroy(4);
	}

TInt ThreadFunction(TAny* aRef)
	{
	__UHEAP_MARK;
	
	CTrapCleanup* cleanupStack(CTrapCleanup::New());
	
	CActiveScheduler* activeScheduler(new CActiveScheduler);
	CActiveScheduler::Install(activeScheduler);
	
	TRAPD(error, DoContentListingRefreshL(static_cast< CMobblerContentListing::TSharedData* >(aRef)));

	delete activeScheduler;
	delete cleanupStack;
	
	__UHEAP_MARKEND;
	
	return error;
	}

CMobblerContentListing* CMobblerContentListing::NewL()
	{
	CMobblerContentListing* self(new(ELeave) CMobblerContentListing());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CMobblerContentListing::CMobblerContentListing()
	:CMobblerContentListingInterface(CActive::EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}
	
void CMobblerContentListing::ConstructL()
	{
	RefreshL();
	}

void CMobblerContentListing::RefreshL()
	{
	if (!iThreadCreated)
		{
		iThreadCreated = ETrue;
		User::LeaveIfError(iThread.Create(_L("Mobbler CLF"), ThreadFunction, KDefaultStackSize, NULL, &iSharedData));
		}

	iSharedData.iState = EMobblerClfModelRefreshing;
	iSharedData.iObserver = this;
	iThread.Rendezvous(iStatus);
	SetActive();
	iThread.Resume();
	}

void CMobblerContentListing::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		if (iSharedData.iState == EMobblerClfModelReady)
			{
			if (iOperations.Count() > 0)
				{
				DoFindLocalTrackL();
				}
			}
		else if (iSharedData.iState == EMobblerClfModelError)
			{
			// there was an error refreshing the list so just complete
			// all the requests as if we didn't find anything
	
			for (TInt i(iOperations.Count() - 1) ; i >= 0 ; --i)
				{
				iOperations[i].iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
				
				delete iOperations[i].iArtist;
				delete iOperations[i].iTitle;
				iOperations.Remove(i);
				}
			}
		}
	else
		{
		iSharedData.iState = EMobblerClfModelError;
		}
	}

void CMobblerContentListing::DoCancel()
	{
	iThread.RendezvousCancel(iStatus);
	}

CMobblerContentListing::~CMobblerContentListing()
	{
	Cancel();
	
	iSharedData.iState = EMobblerClfModelClosing;
	iThread.Logon(iStatus);
	iThread.Resume();
	User::WaitForRequest(iStatus);
	
	const TInt KOperationCount(iOperations.Count());
	for (TInt i(0) ; i < KOperationCount ; ++i)
		{
		delete iOperations[i].iArtist;
		delete iOperations[i].iTitle;
		}
	
	iOperations.Close();
	}

void CMobblerContentListing::FindLocalTrackL(const TDesC& aArtist, const TDesC& aTitle, MMobblerContentListingObserver* aObserver)
	{
	TMobblerContentListingOperation operation;
	operation.iObserver = aObserver;
	operation.iArtist = aArtist.AllocLC();
	operation.iTitle  = aTitle.AllocLC();
	
	iOperations.AppendL(operation);
	
	CleanupStack::Pop(2, operation.iArtist);
	
	DoFindLocalTrackL();
	}

void CMobblerContentListing::CancelFindLocalTrack(MMobblerContentListingObserver* aObserver)
	{
	const TInt KOperationCount(iOperations.Count());
	for (TInt i(0) ; i < KOperationCount ; ++i)
		{
		if (iOperations[i].iObserver == aObserver)
			{
			delete iOperations[i].iArtist;
			delete iOperations[i].iTitle;
			iOperations.Remove(i);
			break;
			}
		}
	}

void CMobblerContentListing::DoFindLocalTrackL()
	{
	if (iSharedData.iState == EMobblerClfModelOutdated)
		{
		RefreshL();
		}
	else if (iSharedData.iState == EMobblerClfModelError)
		{
		// there was an error when we refreshed so just tell them that we couldn't find it
		iOperations[0].iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
		}
	else if (iSharedData.iState == EMobblerClfModelReady)
		{
		// the list is sorted by track title and the by artist name
		// so perform a binary search for 
		
		const TInt KOperationCount(iOperations.Count());
		
		for (TInt i(0) ; i < KOperationCount ; ++i)
			{
			TInt low(0);
			TInt high(iSharedData.iClfModel->ItemCount() - 1);
			
			TBool found(EFalse);
			TInt current(low + ((high - low) / 2));
			
			while (!found && low <= high)
				{
				current = low + ((high - low) / 2);
				
				TPtrC title(KNullDesC);
				
				if (iSharedData.iClfModel->Item(current).GetField(ECLFFieldIdSongName, title) == KErrNone)
					{
					TInt titleCompareResult(iOperations[0].iTitle->Compare(title));
					
					if (titleCompareResult < 0)
						high = current - 1;
					else if (titleCompareResult > 0)
						low = current + 1;
					else
						{
						// we have found the track title so now
						// check that the artist is correct 
						
						TPtrC artist;
						
						if (iSharedData.iClfModel->Item(current).GetField(ECLFFieldIdArtist,  artist) == KErrNone)
							{
							TInt artistCompareResult(iOperations[0].iArtist->Compare(artist));
							
							if (artistCompareResult < 0)
								high = current - 1;
							else if (artistCompareResult > 0)
								low = current + 1;
							else
								{
								// it's the correct track and artist so fetch
								// some other info and notify the observer
								
								TPtrC album(KNullDesC);
								iSharedData.iClfModel->Item(current).GetField(ECLFFieldIdAlbum, album);
								
								TInt32 trackNumber(KErrNotFound);
								iSharedData.iClfModel->Item(current).GetField(ECLFFieldIdTrackNumber, trackNumber);
								
								TPtrC localFile(KNullDesC);
								iSharedData.iClfModel->Item(current).GetField(ECLFFieldIdFileNameAndPath, localFile);
								
								iOperations[0].iObserver->HandleFindLocalTrackCompleteL(trackNumber, album, localFile);
								found = ETrue;
								}
							}
						else
							high = current - 1; // there was an error fetching the artist so assume it was blank
						}
					}
				else
					high = current - 1; // there was an error fetching the song name so assume it was blank
				}
			
			if (!found)
				{
				// Always set the album, even if we don't know it
				// CMobblerTrack needs to know that we don't know it 
				iOperations[0].iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
				}
			
			delete iOperations[0].iArtist;
			delete iOperations[0].iTitle;
			iOperations.Remove(0);
			}
		}
	}

void CMobblerContentListing::HandleOperationEventL(TCLFOperationEvent aOperationEvent, TInt aError)
	{
	if (aOperationEvent == ECLFRefreshComplete)
		{
		// this sould only be called by the other thread
		CActiveScheduler::Stop();
	
		if (aError == KErrNone)
			{
			// We can now look for the album of anything now playing
			iSharedData.iState = EMobblerClfModelReady;
			}
		else
			{
			// there was an error refreshing the list so just complete
			// all the requests as if we didn't find anything
			
			iSharedData.iState = EMobblerClfModelError;
			}
		}
	else if (aOperationEvent == ECLFModelOutdated)
		{
		// Cannot look for any more albums until a refresh
		iSharedData.iState = EMobblerClfModelOutdated;
		}
	}

// End of file
