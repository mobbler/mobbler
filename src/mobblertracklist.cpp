/*
mobblertracklist.cpp

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

#include <aknquerydialog.h>
#include <sendomfragment.h>
#include <senxmlutils.h> 

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerliterals.h"
#include "mobblerparser.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblertracklist.h"
#include "mobblerwebserviceshelper.h"

_LIT8(KGetTopTracks, "gettoptracks");
_LIT8(KTrack, "track");

CMobblerTrackList::CMobblerTrackList(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerTrackList::ConstructL()
	{
	iDefaultImage = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultTrackImage);

	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(KArtist, KGetTopTracks, iText1->String8(), *this);
			break;
		case EMobblerCommandUserTopTracks:
			iAppUi.LastFmConnection().WebServicesCallL(KUser, KGetTopTracks, iText1->String8(), *this);
			break;
		case EMobblerCommandRecentTracks:
			iAppUi.LastFmConnection().RecentTracksL(iText1->String8(), *this);
			break;
		case EMobblerCommandSimilarTracks:
			iAppUi.LastFmConnection().SimilarTracksL(iText1->String8(), iText2->String8(), *this);
			break;
		case EMobblerCommandPlaylistFetchUser:
			iAppUi.LastFmConnection().PlaylistFetchUserL(iText2->String8(), *this);
			break;
		case EMobblerCommandPlaylistFetchAlbum:
			if (iText2->String8().Length() > 10)
				{
				// This is a MusicBrainz ID so fetch the Last.fm ID before getting the playlist
				delete iAlbumInfoObserver;
				iAlbumInfoObserver = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, EFalse);
				iAppUi.LastFmConnection().AlbumGetInfoL(iText2->String8(), *iAlbumInfoObserver);
				}
			else
				{
				// This is the Last.fm ID so just fetch the playlist using it
				iAppUi.LastFmConnection().PlaylistFetchAlbumL(iText2->String8(), *this);
				}
			break;
		case EMobblerCommandSearchTrack:
			iAppUi.LastFmConnection().WebServicesCallL(KTrack, KSearch, iText1->String8(), *this);
			break;
		case EMobblerCommandViewScrobbleLog:
			{
			iAsyncCallBack = new(ELeave) CAsyncCallBack(CActive::EPriorityStandard);
			iCallBack = TCallBack(CMobblerTrackList::ViewScrobbleLogCallBackL, this);
			iAsyncCallBack->Set(iCallBack);
			iAsyncCallBack->Call();
			}
			break;
		default:
			break;
		}
	}

CMobblerTrackList::~CMobblerTrackList()
	{
	delete iAsyncCallBack;
	delete iAlbumInfoObserver;
	delete iWebServicesHelper;
	}

void CMobblerTrackList::GetArtistAndTitleName(TPtrC8& aArtist, TPtrC8& aTitle)
	{
	if (iType == EMobblerCommandViewScrobbleLog)
		{
		if (iAppUi.LastFmConnection().ScrobbleLogCount() > iListBox->CurrentItemIndex()
				&& iAppUi.LastFmConnection().ScrobbleLogCount() > 0)
			{
			aArtist.Set(iAppUi.LastFmConnection().ScrobbleLogItem(iListBox->CurrentItemIndex()).Artist().String8());
			aTitle.Set(iAppUi.LastFmConnection().ScrobbleLogItem(iListBox->CurrentItemIndex()).Title().String8());
			}
		}
	else
		{
		if (iList.Count() > 0)
			{
			aTitle.Set(iList[iListBox->CurrentItemIndex()]->Title()->String8());
			
			if (iType == EMobblerCommandArtistTopTracks)
				{
				aArtist.Set(iText1->String8());
				}
			else
				{
				aArtist.Set(iList[iListBox->CurrentItemIndex()]->Description()->String8());
				}
			}
		}
	}

CMobblerListControl* CMobblerTrackList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	TPtrC8 artist(KNullDesC8);
	TPtrC8 title(KNullDesC8);
	
	GetArtistAndTitleName(artist, title);
		
	switch(aCommand)
		{
		case EMobblerCommandTrackLove:
			iAppUi.LastFmConnection().TrackLoveL(artist, title);
			break;
		case EMobblerCommandTrackAddTag:
			{
			TBuf<KMobblerMaxQueryDialogLength> tag;
			
			CAknTextQueryDialog* tagDialog(new(ELeave) CAknTextQueryDialog(tag));
			tagDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			tagDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TRACK_ADD_TAG));
			tagDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (tagDialog->RunLD())
				{
				CMobblerString* tagString(CMobblerString::NewL(tag));
				CleanupStack::PushL(tagString);
				
				delete iTrackTagAddHelper;
				iTrackTagAddHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
				iAppUi.LastFmConnection().TrackAddTagL(title, artist, tagString->String8(), *iTrackTagAddHelper);
				
				CleanupStack::PopAndDestroy(tagString);
				}
			}
			break;
		case EMobblerCommandTrackRemoveTag:
			{
			// fetch the user's tags for this track
			delete iTrackTagsHelper;
			iTrackTagsHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
			iAppUi.LastFmConnection().TrackGetTagsL(title, artist, *iTrackTagsHelper);
			}
			break;
		case EMobblerCommandTrackShare:
		case EMobblerCommandArtistShare:
		case EMobblerCommandPlaylistAddTrack:
			{
			CMobblerTrack* track(CMobblerTrack::NewL(artist, title, KNullDesC8, KNullDesC8, KNullDesC8, KNullDesC8, 0, KNullDesC8));
			
			delete iWebServicesHelper;
			iWebServicesHelper = CMobblerWebServicesHelper::NewL(iAppUi);
			
			switch (aCommand)
				{
				case EMobblerCommandTrackShare: iWebServicesHelper->TrackShareL(*track); break;
				case EMobblerCommandArtistShare: iWebServicesHelper->ArtistShareL(*track); break;
				case EMobblerCommandPlaylistAddTrack: iWebServicesHelper->PlaylistAddL(*track); break;
				}
				
			track->Release();
			}
			break;
		case EMobblerCommandScrobbleLogRemove:
			{
			// Get the list box items model
			MDesCArray* listArray(iListBox->Model()->ItemTextArray());
			CDesCArray* itemArray(static_cast<CDesCArray*>(listArray));
			
			// Number of items in the list
			const TInt KCount(itemArray->Count());
			
			// Validate index then delete
			const TInt KIndex(iListBox->CurrentItemIndex());
			if (KIndex >= 0 && KIndex < KCount)
				{
				// Remove the item from the scrobble log
				iAppUi.LastFmConnection().RemoveScrobbleLogItemL(KIndex);
				
				// remove the item from out list box
				itemArray->Delete(KIndex, 1);
				AknListBoxUtils::HandleItemRemovalAndPositionHighlightL(iListBox, KIndex, ETrue);
				iListBox->DrawNow();
				}
			}
			break;
		default:
			break;
		}
	
	return list;
	}

void CMobblerTrackList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandTrackLove);
	
	aCommands.AppendL(EMobblerCommandShare);
	aCommands.AppendL(EMobblerCommandTrackShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	
	aCommands.AppendL(EMobblerCommandTag);
	aCommands.AppendL(EMobblerCommandTrackAddTag);
	aCommands.AppendL(EMobblerCommandTrackRemoveTag);
	
	aCommands.AppendL(EMobblerCommandPlaylistAddTrack);
	
	if (iType == EMobblerCommandViewScrobbleLog)
		{
		aCommands.AppendL(EMobblerCommandScrobbleLogRemove);
		}
	}

void CMobblerTrackList::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		// Create the XML reader and DOM fragment and associate them with each other
		CSenXmlReader* xmlReader(CSenXmlReader::NewL());
		CleanupStack::PushL(xmlReader);
		CSenDomFragment* domFragment(CSenDomFragment::NewL());
		CleanupStack::PushL(domFragment);
		xmlReader->SetContentHandler(*domFragment);
		domFragment->SetReader(*xmlReader);
		
		// Parse the XML into the DOM fragment
		xmlReader->ParseL(aData);
			
		if (aObserver == iAlbumInfoObserver)
			{	
			iAppUi.LastFmConnection().PlaylistFetchAlbumL(domFragment->AsElement().Element(KElementAlbum)->Element(KElementId)->Content(), *this);
			}
		else if (aObserver == iTrackTagsHelper)
			{
			RPointerArray<CSenElement>& tags(domFragment->AsElement().Element(_L8("tags"))->ElementsL());
			
			const TInt KTagCount(tags.Count());
			
			CDesCArray* textArray(new(ELeave) CDesCArrayFlat(KTagCount));
			CleanupStack::PushL(textArray);
			
			for (TInt i(0) ; i < KTagCount ; ++i)
				{
				CMobblerString* tagName(CMobblerString::NewL(tags[i]->Element(_L8("name"))->Content()));
				CleanupStack::PushL(tagName);
				textArray->AppendL(tagName->String());
				CleanupStack::PopAndDestroy(tagName);
				}
			
			TInt index;
			CAknListQueryDialog* tagRemoveDialog = new(ELeave) CAknListQueryDialog(&index);
			tagRemoveDialog->PrepareLC(R_MOBBLER_TAG_REMOVE_QUERY);
			tagRemoveDialog->SetItemTextArray(textArray); 
			tagRemoveDialog->SetOwnershipType(ELbmDoesNotOwnItemArray); 
		 
			if (tagRemoveDialog->RunLD())
				{
				// remove the tag!!!
				
				TPtrC8 artist(KNullDesC8);
				TPtrC8 title(KNullDesC8);
					
				GetArtistAndTitleName(artist, title);
				
				CMobblerString* tagName(CMobblerString::NewL((*textArray)[index]));
				CleanupStack::PushL(tagName);
				
				iTrackTagAddHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
				iAppUi.LastFmConnection().TrackRemoveTagL(title, artist, tagName->String8(), *iTrackTagAddHelper);
				
				CleanupStack::PopAndDestroy(tagName);
				}
			
			CleanupStack::Pop(textArray);
			}
		
		CleanupStack::PopAndDestroy(2);
		}

	else
		{
		// TODO
		}
	}

TInt CMobblerTrackList::ViewScrobbleLogCallBackL(TAny* aPtr)
	{
	static_cast<CMobblerTrackList*>(aPtr)->CMobblerListControl::DataL(KNullDesC8, CMobblerLastFmConnection::ETransactionErrorNone);
	}

void CMobblerTrackList::ParseL(const TDesC8& aXML)
	{
	switch (iType)
		{
		case EMobblerCommandArtistTopTracks:
			CMobblerParser::ParseArtistTopTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandUserTopTracks:
			CMobblerParser::ParseUserTopTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandRecentTracks:
			CMobblerParser::ParseRecentTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandSimilarTracks:
			CMobblerParser::ParseSimilarTracksL(aXML, *this, iList);
			break;
		case EMobblerCommandPlaylistFetchUser:
		case EMobblerCommandPlaylistFetchAlbum:
			CMobblerParser::ParsePlaylistL(aXML, *this, iList);
			break;
		case EMobblerCommandSearchTrack:
			CMobblerParser::ParseSearchTrackL(aXML, *this, iList);
			break;
		case EMobblerCommandViewScrobbleLog:
			{
			const TInt KScrobbleLogCount(iAppUi.LastFmConnection().ScrobbleLogCount());
			for (TInt i(0) ; i < KScrobbleLogCount ; ++i)
				{
				// Add an item to the list box			
				CMobblerListItem* item(CMobblerListItem::NewL(*this,
																iAppUi.LastFmConnection().ScrobbleLogItem(i).Title().String8(),
																iAppUi.LastFmConnection().ScrobbleLogItem(i).Artist().String8(),
																KNullDesC8));

				CleanupStack::PushL(item);
				iList.AppendL(item);
				CleanupStack::Pop(item);
				}
			}
			break;
		default:
			break;
		}
	}

// End of file
