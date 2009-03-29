/*
mobblerfriendlist.cpp

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

#include <gulicon.h>
#include <eikclbd.h>

#include "mobblerappui.h"
#include "mobblerfriend.h"
#include "mobblerfriendlist.h"
#include "mobblerparser.h"
#include "mobblerlastfmconnection.h"

#include "mobbler.hrh"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_user.png");

CMobblerFriendList* CMobblerFriendList::NewL(CMobblerAppUi& aAppUi, const TDesC8& aUser, const TRect& aRect)
	{
	CMobblerFriendList* self = new(ELeave) CMobblerFriendList(aAppUi);
	CleanupStack::PushL(self);
	self->ConstructL(aUser, aRect);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerFriendList::CMobblerFriendList(CMobblerAppUi& aAppUi)
	:CMobblerListControl(aAppUi)
	{
	}

void CMobblerFriendList::ConstructL(const TDesC8& aUser, const TRect& aRect)
	{
	CreateWindowL(); // This is a window owning control
	
	InitComponentArrayL();
	
	SetRect(aRect);
	
    // Create listbox 
    iListBox = new(ELeave) CAknSingleLargeStyleListBox();

    iListBox->ConstructL(this, EAknListBoxSelectionList | EAknListBoxLoopScrolling );    
    iListBox->SetContainerWindowL(*this);
    // Set scrollbars
    iListBox->CreateScrollBarFrameL(ETrue);
    iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);    
    
    // Activate Listbox
    iListBox->SetRect(Rect());
    
    iListBoxItems = new (ELeave) CDesCArrayFlat(4);
    iListBox->Model()->SetItemTextArray(iListBoxItems);
    iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
    
    iListBox->ActivateL();
    
    iMobblerBitmapDefaultUser = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    iAppUi.LastFMConnection().UserGetFriendsL(aUser, *this);
	}

CMobblerFriendList::~CMobblerFriendList()
	{
	const TInt KFriendListCount(iFriendList.Count());
	for (TInt i(0) ; i < KFriendListCount ; ++i)
		{
		iAppUi.LastFMConnection().CancelTransaction(iFriendList[i]);
		}
	
	iFriendList.ResetAndDestroy();
	delete iListBox;
	delete iListBoxItems;
	delete iMobblerBitmapDefaultUser;
	}

CMobblerListControl* CMobblerFriendList::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandUserGetFriends:
			list = CMobblerFriendList::NewL(iAppUi, iFriendList[iListBox->CurrentItemIndex()]->UserName(), iAppUi.ClientRect());
			break;
		case EMobblerCommandTrackShare:
			if (iAppUi.CurrentTrack()) iAppUi.LastFMConnection().TrackShareL(iFriendList[iListBox->CurrentItemIndex()]->UserName(), *iAppUi.CurrentTrack());
			break;
		case EMobblerCommandAlbumShare:
			break;
		case EMobblerCommandArtistShare:
			break;
		case EMobblerCommandRadioPersonal:
			iAppUi.RadioStartL(CMobblerLastFMConnection::EPersonal, iFriendList[iListBox->CurrentItemIndex()]->UserName());
			break;
		case EMobblerCommandRadioNeighbourhood:
			iAppUi.RadioStartL(CMobblerLastFMConnection::ENeighbourhood, iFriendList[iListBox->CurrentItemIndex()]->UserName());
			break;
		case EMobblerCommandRadioLoved:
			iAppUi.RadioStartL(CMobblerLastFMConnection::ELovedTracks, iFriendList[iListBox->CurrentItemIndex()]->UserName());
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerFriendList::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandUserGetFriends);
	aCommands.AppendL(EMobblerCommandTrackShare);
	aCommands.AppendL(EMobblerCommandAlbumShare);
	aCommands.AppendL(EMobblerCommandArtistShare);
	aCommands.AppendL(EMobblerCommandRadio);
	}

void CMobblerFriendList::HandleFriendLoadedL()
	{
	UpdateIconArrayL();
	}

void CMobblerFriendList::UpdateIconArrayL()
	{
	if (iMobblerBitmapDefaultUser->Bitmap() && iFriendList.Count() > 0)
		{
		// only update the icons if we have loaded the default icon
		
		const TInt KFriendsListCount(iFriendList.Count());
		
		CArrayPtr<CGulIcon>* icons = new(ELeave) CArrayPtrFlat<CGulIcon>(KFriendsListCount);
		CleanupStack::PushL(icons);
		
		for (TInt i(0) ; i < KFriendsListCount ; ++i)
			{
			CGulIcon* icon = NULL;
			
			if (iFriendList[i]->Image() && iFriendList[i]->Image()->Bitmap())
				{
				icon = CGulIcon::NewL(iFriendList[i]->Image()->Bitmap(), iFriendList[i]->Image()->Mask());
				}
			else
				{
				icon = CGulIcon::NewL(iMobblerBitmapDefaultUser->Bitmap(), iMobblerBitmapDefaultUser->Mask());
				}
			
			icon->SetBitmapsOwnedExternally(ETrue);
			icons->AppendL(icon);
			}
		
		iListBox->ItemDrawer()->ColumnData()->SetIconArray(icons);
		CleanupStack::Pop(icons);
		
		iListBox->HandleItemAdditionL();
		}
	}

void CMobblerFriendList::DataL(const TDesC8& aXML, TInt aError)
	{
	if (aError == KErrNone)
		{
		CMobblerParser::ParseFriendListL(aXML, *this, iFriendList);
		
		const TInt KFriendsListCount(iFriendList.Count());
		for (TInt i(0) ; i < KFriendsListCount ; ++i)
			{
			// request the image for this friend
			iAppUi.LastFMConnection().RequestImageL(iFriendList[i], iFriendList[i]->ImageLocation());
		
			// add the formatted text to the array
			HBufC8* format8 = HBufC8::NewLC(1024);
			_LIT8(KFormat8, "%d\t%S");
			format8->Des().Format(KFormat8, i, &iFriendList[i]->UserName());
			HBufC* format = HBufC::NewLC(1024);
			format->Des().Copy(*format8);
			iListBoxItems->AppendL(*format);
			CleanupStack::PopAndDestroy(2, format8);
			}
		
	    iListBox->HandleItemAdditionL();
	
	    UpdateIconArrayL();
		}
	}

void CMobblerFriendList::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	UpdateIconArrayL();
	}

TKeyResponse CMobblerFriendList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	if ( aKeyEvent.iCode == EKeyLeftArrow 
		|| aKeyEvent.iCode == EKeyRightArrow )
		{
		// allow the web services control to get the arrow keys
		return EKeyWasNotConsumed;
		}
	
	return iListBox->OfferKeyEventL(aKeyEvent, aEventCode);
	}


void CMobblerFriendList::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
   	gc.Clear(Rect());
	}

CCoeControl* CMobblerFriendList::ComponentControl(TInt /*aIndex*/) const
	{
	return iListBox;
	}
 
TInt CMobblerFriendList::CountComponentControls() const
	{
	return 1;
	}
