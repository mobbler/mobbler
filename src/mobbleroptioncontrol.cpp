/*
mobbleroptioncontrol.cpp

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
#include "mobblerlastfmconnection.h"
#include "mobbleroptioncontrol.h"
#include "mobblerfriendlist.h"
#include "mobblereventlist.h"
#include "mobblertaglist.h"
#include "mobblerartistlist.h"
#include "mobblerplaylistlist.h"
#include "mobblertracklist.h"
#include "mobbleralbumlist.h"

#include "mobbler.hrh"

_LIT(KFriendsImage, "\\resource\\apps\\mobbler\\default_user.png");

CMobblerOptionControl* CMobblerOptionControl::NewL(CMobblerAppUi& aAppUi, const TRect& aRect)
	{
	CMobblerOptionControl* self = new(ELeave) CMobblerOptionControl(aAppUi);
	CleanupStack::PushL(self);
	self->ConstructL(aRect);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerOptionControl::CMobblerOptionControl(CMobblerAppUi& aAppUi)
	:CMobblerListControl(aAppUi)
	{
	}

void CMobblerOptionControl::ConstructL(const TRect& aRect)
	{
	CreateWindowL(); // This is a window owning control
	
	InitComponentArrayL();
	
	SetRect(aRect);
	
    // Create listbox 
    iListBox = new(ELeave) CAknSingleLargeStyleListBox();

    iListBox->ConstructL(this, EAknListBoxSelectionList | EAknListBoxLoopScrolling);    
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
    
    iFriendsIcon = CMobblerBitmap::NewL(*this, KFriendsImage);
    //iRadioIcon = CMobblerBitmap::NewL(*this, KRadioImage);
    
    iListBoxItems->AppendL(_L("0\tFriends"));
    iListBoxItems->AppendL(_L("0\tPlaylists"));
    iListBoxItems->AppendL(_L("0\tTags"));
    iListBoxItems->AppendL(_L("0\tEvents"));
    iListBoxItems->AppendL(_L("0\tArtists"));
    iListBoxItems->AppendL(_L("0\tAlbums"));
    iListBoxItems->AppendL(_L("0\tTracks"));
    
    iListBox->HandleItemAdditionL();
	}

CMobblerOptionControl::~CMobblerOptionControl()
	{
	delete iListBox;
	delete iListBoxItems;
	delete iFriendsIcon;
	}

CMobblerListControl* CMobblerOptionControl::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	if (aCommand == EMobblerCommandOpen)
		{
		switch (iListBox->CurrentItemIndex())
			{
			case 0:
				// friend list
				list = CMobblerFriendList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 1:
				// playlists
				list = CMobblerPlaylistList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 2:
				// tags
				list = CMobblerTagList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 3:
				// events
				list = CMobblerEventList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 4:
				// artists
				list = CMobblerArtistList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 5:
				// albums
				list = CMobblerAlbumList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			case 6:
				// tracks
				list = CMobblerTrackList::NewL(iAppUi, KNullDesC8, iAppUi.ClientRect());
				break;
			default:
				//
				break;
			}
		}
	
	return list;
	}

void CMobblerOptionControl::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandOpen);
	}

void CMobblerOptionControl::UpdateIconArrayL()
	{
	if (iFriendsIcon->Bitmap())
		{
		CArrayPtr<CGulIcon>* icons = new(ELeave) CArrayPtrFlat<CGulIcon>(2);
		CleanupStack::PushL(icons);
		
		CGulIcon* friendIcon = CGulIcon::NewL(iFriendsIcon->Bitmap(), iFriendsIcon->Mask());
		friendIcon->SetBitmapsOwnedExternally(ETrue);
		icons->AppendL(friendIcon);
		
		iListBox->ItemDrawer()->ColumnData()->SetIconArray(icons);
		CleanupStack::Pop(icons);
		
		iListBox->HandleItemAdditionL();
		}
	}

void CMobblerOptionControl::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	UpdateIconArrayL();
	}

void CMobblerOptionControl::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	UpdateIconArrayL();
	}

TKeyResponse CMobblerOptionControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode)
	{
	if ( aKeyEvent.iCode == EKeyLeftArrow 
		|| aKeyEvent.iCode == EKeyRightArrow )
		{
		// allow the web services control to get the arrow keys
		return EKeyWasNotConsumed;
		}
	
	return iListBox->OfferKeyEventL(aKeyEvent, aEventCode);
	}


void CMobblerOptionControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
   	gc.Clear(Rect());
	}

CCoeControl* CMobblerOptionControl::ComponentControl(TInt /*aIndex*/) const
	{
	return iListBox;
	}
 
TInt CMobblerOptionControl::CountComponentControls() const
	{
	return 1;
	}

void CMobblerOptionControl::HandleLoadedL()
	{
	
	}

