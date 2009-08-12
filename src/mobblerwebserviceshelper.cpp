/*
mobblerwebserviceshelper.cpp

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

#include <akninfopopupnotecontroller.h>
#include <AknLists.h>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <aknsutils.h>
#include <bautils.h>
#include <sendomfragment.h>
#include <senxmlutils.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerbitmapcollection.h"
#include "mobblercontacts.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerwebserviceshelper.h"

CMobblerWebServicesHelper* CMobblerWebServicesHelper::NewL(CMobblerAppUi& aAppUi)
	{
	CMobblerWebServicesHelper* self(new(ELeave) CMobblerWebServicesHelper(aAppUi));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesHelper::CMobblerWebServicesHelper(CMobblerAppUi& aAppUi)
	:iAppUi(aAppUi)
	{
	}

void CMobblerWebServicesHelper::ConstructL()
	{
	}

CMobblerWebServicesHelper::~CMobblerWebServicesHelper()
	{
	delete iEventId;
	delete iFriendFetchObserverHelperTrackShare;
	delete iFriendFetchObserverHelperArtistShare;
	delete iFriendFetchObserverHelperEventShare;
	delete iShareObserverHelper;
	delete iPlaylistAddObserverHelper;
	delete iPlaylistFetchObserverHelper;
	
	if (iTrack)
		{
		iTrack->Release();
		}
	}

void CMobblerWebServicesHelper::TrackShareL(CMobblerTrack& aTrack)
	{
	iTrack = &aTrack;
	iTrack->Open();
	
	CMobblerString* username(CMobblerString::NewL(iAppUi.SettingView().Username()));
	CleanupStack::PushL(username);
	
	delete iFriendFetchObserverHelperTrackShare;
	iFriendFetchObserverHelperTrackShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("getfriends"), username->String8(), *iFriendFetchObserverHelperTrackShare);
	
	CleanupStack::PopAndDestroy(username);
	}

void CMobblerWebServicesHelper::ArtistShareL(CMobblerTrack& aTrack)
	{
	iTrack = &aTrack;
	iTrack->Open();
	
	CMobblerString* username(CMobblerString::NewL(iAppUi.SettingView().Username()));
	CleanupStack::PushL(username);
	
	delete iFriendFetchObserverHelperArtistShare;
	iFriendFetchObserverHelperArtistShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("getfriends"), username->String8(), *iFriendFetchObserverHelperArtistShare);
	
	CleanupStack::PopAndDestroy(username);
	}

void CMobblerWebServicesHelper::PlaylistAddL(CMobblerTrack& aTrack)
	{
	iTrack = &aTrack;
	iTrack->Open();
	
	CMobblerString* username(CMobblerString::NewL(iAppUi.SettingView().Username()));
	CleanupStack::PushL(username);
	
	delete iPlaylistFetchObserverHelper;
	iPlaylistFetchObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("getplaylists"), username->String8(), *iPlaylistFetchObserverHelper);
	
	CleanupStack::PopAndDestroy(username);
	}

void CMobblerWebServicesHelper::EventShareL(const TDesC8& aEventId)
	{
	iEventId = aEventId.AllocL();
	
	CMobblerString* username(CMobblerString::NewL(iAppUi.SettingView().Username()));
	CleanupStack::PushL(username);
	
	delete iFriendFetchObserverHelperEventShare;
	iFriendFetchObserverHelperEventShare = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
	iAppUi.LastFmConnection().WebServicesCallL(_L8("user"), _L8("getfriends"), username->String8(), *iFriendFetchObserverHelperEventShare);
	
	CleanupStack::PopAndDestroy(username);
	}

HBufC* CMobblerWebServicesHelper::DisplayEmailListL(const CDesCArray& aEmails)
	{
	CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
	CleanupStack::PushL(list);
	
	CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
	CleanupStack::PushL(popup);
	
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	
	popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
	
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
	CleanupStack::PushL(items);
	
	const TInt KEmailCount(aEmails.Count());
	for (TInt i(0) ; i < KEmailCount ; ++i)
		{
		items->AppendL(aEmails[i]);
		}
	
	CleanupStack::Pop(items);
	
	list->Model()->SetItemTextArray(items);
	list->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	CleanupStack::Pop(popup);
	
	HBufC* email(NULL);
	
	if (popup->ExecuteLD())
		{
		email = aEmails[list->CurrentItemIndex()].AllocL();
		}
	
	CleanupStack::PopAndDestroy(list); //list
	
	return email;
	}

void CMobblerWebServicesHelper::BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap)
	{
	CActiveScheduler::Stop();
	}

void CMobblerWebServicesHelper::BitmapResizedL(const CMobblerBitmap* aMobblerBitmap)
	{
	
	}

HBufC* CMobblerWebServicesHelper::DisplayContactListL()
	{
	CMobblerContacts* contacts = CMobblerContacts::NewLC();
	
	CAknDoubleLargeGraphicPopupMenuStyleListBox* list(new(ELeave) CAknDoubleLargeGraphicPopupMenuStyleListBox);
	CleanupStack::PushL(list);
	
	CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
	CleanupStack::PushL(popup);
	
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	
	popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
	
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	const TInt KContactCount(contacts->Count());
	
	CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(KContactCount));
	CleanupStack::PushL(items);
	
	list->ItemDrawer()->ColumnData()->SetIconArray(new(ELeave) CArrayPtrFlat<CGulIcon>(KContactCount));
	
	CMobblerBitmap* defaultUser = iAppUi.BitmapCollection().BitmapL(*this, CMobblerBitmapCollection::EBitmapDefaultUserImage);
	CActiveScheduler::Start();
	CGulIcon* defaultUserIcon = CGulIcon::NewL(defaultUser->Bitmap(), defaultUser->Mask());
	defaultUserIcon->SetBitmapsOwnedExternally(ETrue);
	list->ItemDrawer()->ColumnData()->IconArray()->AppendL(defaultUserIcon);
	
	TInt photoNumber(1);
	
	for (TInt i(0) ; i < KContactCount ; ++i)
		{
		HBufC8* photo = contacts->GetPhotoAtL(i);
		
		if (photo)
			{
			CleanupStack::PushL(photo);
			CMobblerBitmap* bitmap = CMobblerBitmap::NewL(*this, *photo);
			CActiveScheduler::Start();
			CGulIcon* icon = CGulIcon::NewL(bitmap->Bitmap(), bitmap->Mask());
			icon->SetBitmapsOwnedExternally(ETrue);
			list->ItemDrawer()->ColumnData()->IconArray()->AppendL(icon);
			//bitmap->Close();
			CleanupStack::PopAndDestroy(photo);
			
			TBuf<1024> formatted;
			formatted.Format(_L("%d\t%S\tyeah"), photoNumber++, &contacts->GetNameAt(i));
			items->AppendL(formatted);
			}
		else
			{
			TBuf<1024> formatted;
			formatted.Format(_L("%d\t%S\tyeah"), 0, &contacts->GetNameAt(i));
			items->AppendL(formatted);
			}
		}
	
	CleanupStack::Pop(items);
	
	list->Model()->SetItemTextArray(items);
	list->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	CleanupStack::Pop(popup);
	
	HBufC* email(NULL);
	
	if (popup->ExecuteLD())
		{
		CDesCArray* emails(contacts->GetEmailsAtLC(list->CurrentItemIndex()));
		
		if (emails->Count() == 1)
			{
			email = (*emails)[0].AllocL();
			}
		else
			{
			// they have multiple emails so ask which one
			email = DisplayEmailListL(*emails);
			}
		
		CleanupStack::PopAndDestroy(emails);
		}
	
	CleanupStack::PopAndDestroy(list); //list
	CleanupStack::PopAndDestroy(contacts);
	
	return email;
	}


void CMobblerWebServicesHelper::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		// create the XML reader and DOM fragement and associate them with each other 
		CSenXmlReader* xmlReader(CSenXmlReader::NewL());
		CleanupStack::PushL(xmlReader);
		CSenDomFragment* domFragment(CSenDomFragment::NewL());
		CleanupStack::PushL(domFragment);
		xmlReader->SetContentHandler(*domFragment);
		domFragment->SetReader(*xmlReader);
		
		// parse the XML into the DOM fragment
		xmlReader->ParseL(aData);
		
		if (aObserver == iShareObserverHelper ||
				aObserver == iPlaylistAddObserverHelper )
			{
			if (domFragment->AsElement().AttrValue(_L8("status"))->Compare(_L8("ok")) == 0)
				{
				// Everything worked
				CAknConfirmationNote* note(new (ELeave) CAknConfirmationNote(EFalse));
				note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_DONE));
				}
			else
				{
				// There was an error so display it
				CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
				CMobblerString* string(CMobblerString::NewL(domFragment->AsElement().Element(_L8("error"))->Content()));
				CleanupStack::PushL(string);
				note->ExecuteLD(string->String());
				CleanupStack::Pop(string);
				}
			}
		else if (aObserver == iFriendFetchObserverHelperTrackShare ||
					aObserver == iFriendFetchObserverHelperArtistShare ||
					aObserver == iFriendFetchObserverHelperEventShare)
			{
			// Parse and bring up a share with friends popup menu
			
			CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
			CleanupStack::PushL(list);
			
			CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
			CleanupStack::PushL(popup);
			
			list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
			
			popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TO_PROMPT));
			
			list->CreateScrollBarFrameL(ETrue);
			list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
			
			CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
			CleanupStack::PushL(items);
			
			items->AppendL(_L("From contacts..."));
			
			RPointerArray<CSenElement>& users(domFragment->AsElement().Element(_L8("friends"))->ElementsL());
			
			for (TInt i(0); i < users.Count(); ++i)
				{
				CMobblerString* user(CMobblerString::NewL(users[i]->Element(_L8("name"))->Content()));
				CleanupStack::PushL(user);
				items->AppendL(user->String());
				CleanupStack::PopAndDestroy(user);
				}
			
			CleanupStack::Pop(items);
			
			list->Model()->SetItemTextArray(items);
			list->Model()->SetOwnershipType(ELbmOwnsItemArray);
			
			CleanupStack::Pop(popup);
			
			if (popup->ExecuteLD())
				{
				CMobblerString* recipient(NULL);
				
				if (list->CurrentItemIndex() == 0)
					{
					// They chose to share with contacts so bring up that list
					HBufC* contactEmail(DisplayContactListL());
					if (contactEmail)
						{
						CleanupStack::PushL(contactEmail);
						recipient = CMobblerString::NewL(*contactEmail);
						CleanupStack::PopAndDestroy(contactEmail);
						}
					else
						{
						// they cancelled when selecting a contact
						// so do nothing
						}
					}
				else
					{
					recipient = CMobblerString::NewL((*items)[list->CurrentItemIndex()]);
					}
				
				if (recipient)
					{
					CleanupStack::PushL(recipient);
					
					TBuf<EMobblerMaxQueryDialogLength> message;
					
					CAknTextQueryDialog* shoutDialog(new(ELeave) CAknTextQueryDialog(message));
					shoutDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
					shoutDialog->SetPromptL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_MESSAGE_PROMPT));
					shoutDialog->SetPredictiveTextInputPermitted(ETrue);
					
					if (shoutDialog->RunLD())
						{
						CMobblerString* messageString(CMobblerString::NewL(message));
						CleanupStack::PushL(messageString);
						
						if (aObserver == iFriendFetchObserverHelperTrackShare)
							{
							delete iShareObserverHelper;
							iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
							iAppUi.LastFmConnection().TrackShareL(recipient->String8(), iTrack->Artist().String8(), iTrack->Title().String8(), messageString->String8(), *iShareObserverHelper);
							}
						else if (aObserver == iFriendFetchObserverHelperArtistShare)
							{
							delete iShareObserverHelper;
							iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
							iAppUi.LastFmConnection().ArtistShareL(recipient->String8(), iTrack->Artist().String8(), messageString->String8(), *iShareObserverHelper);
							}
						else
							{
							// This must be sharing an event
							delete iShareObserverHelper;
							iShareObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
							iAppUi.LastFmConnection().EventShareL(recipient->String8(), *iEventId, messageString->String8(), *iShareObserverHelper);
							}
						
						CleanupStack::PopAndDestroy(messageString);
						}
					
					CleanupStack::PopAndDestroy(recipient);
					}
				}
			
			CleanupStack::PopAndDestroy(list); //list
			}
		else if (aObserver == iPlaylistFetchObserverHelper)
			{
			// parse and bring up an add to playlist popup menu
			// create the XML reader and DOM fragement and associate them with each other 
			CAknSinglePopupMenuStyleListBox* list(new(ELeave) CAknSinglePopupMenuStyleListBox);
			CleanupStack::PushL(list);
			
			CAknPopupList* popup(CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow));
			CleanupStack::PushL(popup);
			
			list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
			
			popup->SetTitleL(iAppUi.ResourceReader().ResourceL(R_MOBBLER_PLAYLIST_ADD_TRACK));
			
			list->CreateScrollBarFrameL(ETrue);
			list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
			
			CDesCArrayFlat* items(new(ELeave) CDesCArrayFlat(1));
			CleanupStack::PushL(items);
			
			RPointerArray<CSenElement>& playlists(domFragment->AsElement().Element(_L8("playlists"))->ElementsL());
			
			for (TInt i(0); i < playlists.Count(); ++i)
				{
				CMobblerString* playlist(CMobblerString::NewL(playlists[i]->Element(_L8("title"))->Content()));
				CleanupStack::PushL(playlist);
				items->AppendL(playlist->String());
				CleanupStack::PopAndDestroy(playlist);
				}
			
			CleanupStack::Pop(items);
			
			list->Model()->SetItemTextArray(items);
			list->Model()->SetOwnershipType(ELbmOwnsItemArray);
			
			CleanupStack::Pop(popup);
			
			if (popup->ExecuteLD())
				{
				if (list->CurrentItemIndex() >= 0)
					{
					delete iPlaylistAddObserverHelper;
					iPlaylistAddObserverHelper = CMobblerFlatDataObserverHelper::NewL(iAppUi.LastFmConnection(), *this, ETrue);
					iAppUi.LastFmConnection().PlaylistAddTrackL(playlists[list->CurrentItemIndex()]->Element(_L8("id"))->Content(), iTrack->Artist().String8(), iTrack->Title().String8(), *iPlaylistAddObserverHelper);
					}
				}
			
			CleanupStack::PopAndDestroy(list); //list
			}
		
		CleanupStack::PopAndDestroy(2, xmlReader);
		}
	else
		{
		CAknInformationNote* note(new (ELeave) CAknInformationNote(EFalse));
		note->ExecuteLD(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_ERROR));
		}
	
	if (iTrack)
		{
		iTrack->Release();
		iTrack = NULL;
		}
	
	delete iEventId;
	iEventId = NULL;
	}

// End of file
