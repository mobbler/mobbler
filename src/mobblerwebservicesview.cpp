/*
mobblerwebservicesview.cpp

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

#include <akncontext.h>
#include <akntitle.h>
#include <audioequalizerutility.h>
#include <barsread.h>
#include <eikmenub.h>
#include <mdaaudiooutputstream.h>
#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerlistcontrol.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblershoutbox.h"
#include "mobblerwebservicescontrol.h"
#include "mobblerwebservicesview.h"

CMobblerWebServicesView* CMobblerWebServicesView::NewL()
	{
	CMobblerWebServicesView* self = new(ELeave) CMobblerWebServicesView;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesView::CMobblerWebServicesView()
	{
	}

CMobblerWebServicesView::~CMobblerWebServicesView()
	{
	delete iWebServicesControl;
	}

void CMobblerWebServicesView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_WEBSERVICES_VIEW);
	}

void CMobblerWebServicesView::SetMenuItemTextL(CEikMenuPane* aMenuPane, TInt aResourceId, TInt aCommandId)
	{
	HBufC* menuText(static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(aResourceId).AllocLC());

	const TInt KTextLimit(CEikMenuPaneItem::SData::ENominalTextLength);
	if (menuText->Length() > KTextLimit)
		{
		TBuf<KTextLimit> newText(menuText->Left(KTextLimit));
		CleanupStack::PopAndDestroy(menuText);
		menuText = newText.AllocLC();
		}

	aMenuPane->SetItemTextL(aCommandId, *menuText);
	CleanupStack::PopAndDestroy(menuText);
	}

void CMobblerWebServicesView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
	{
	// First load the menu text so as not to confuse any dimming logic
	if (aResourceId == R_MOBBLER_WEBSERVICES_MENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_OPEN,					EMobblerCommandOpen);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO,				EMobblerCommandRadioStart);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_START,			EMobblerCommandRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHARE,				EMobblerCommandShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VIEW,					EMobblerCommandView);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUT,				EMobblerCommandShout);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXIT,					EAknSoftkeyExit);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_LOVE_TRACK,			EMobblerCommandTrackLove);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLIST_CREATE,		EMobblerCommandPlaylistCreate);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLIST_ADD_TRACK,	EMobblerCommandPlaylistAddTrack);
		}
	else if (aResourceId == R_MOBBLER_WEBSERVICES_RADIO_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_PERSONAL,		EMobblerCommandRadioPersonal);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_NEIGHBOURHOOD,	EMobblerCommandRadioNeighbourhood);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_LOVED,			EMobblerCommandRadioLoved);
		}
	else if (aResourceId == R_MOBBLER_SHARE_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK,			EMobblerCommandTrackShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST,			EMobblerCommandArtistShare);
		}	
	else if (aResourceId == R_MOBBLER_VIEW_SUBMENU_PANE)
		{		
		SetMenuItemTextL(aMenuPane, R_MOBBLER_FRIENDS,			EMobblerCommandFriends);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ARTISTS,		EMobblerCommandUserTopArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,		EMobblerCommandUserTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,		EMobblerCommandUserTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLISTS,		EMobblerCommandPlaylists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,			EMobblerCommandUserEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,			EMobblerCommandArtistEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,			EMobblerCommandUserTopTags);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RECENT_TRACKS,	EMobblerCommandRecentTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,			EMobblerCommandUserShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,			EMobblerCommandArtistShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,			EMobblerCommandEventShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_ARTISTS,	EMobblerCommandSimilarArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_TRACKS,	EMobblerCommandSimilarTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,		EMobblerCommandArtistTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,		EMobblerCommandArtistTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,			EMobblerCommandArtistTopTags);
		}
	
	// Now the menu text is set, dimming logic is next
	RArray<TInt> supportedCommands;
	CleanupClosePushL(supportedCommands);
	
	// Always support the exit command
	supportedCommands.Append(EAknSoftkeyExit);
	
	if (iWebServicesControl->TopControl()->Count() > 0)
		{
		// Only allow list specific commands if there are items in the list
		iWebServicesControl->TopControl()->SupportedCommandsL(supportedCommands);
		}
	
	for (TInt i(EMobblerCommandOnline); i < EMobblerCommandLast; ++i)
		{
		TInt position(0);
		if (aMenuPane->MenuItemExists(i, position))
			{
			aMenuPane->SetItemDimmed(i, supportedCommands.Find(i) == KErrNotFound);
			}
		}
	
	CleanupStack::PopAndDestroy(&supportedCommands);
	
	if (aResourceId == R_MOBBLER_WEBSERVICES_MENU_PANE)
		{
		if (iWebServicesControl->TopControl()->Type() == EMobblerCommandFriends)
		aMenuPane->SetItemDimmed(EMobblerCommandShare, 
						!static_cast<CMobblerAppUi*>(AppUi())->CurrentTrack() || supportedCommands.Find(EMobblerCommandShare) == KErrNotFound);
		}
	else if (aResourceId == R_MOBBLER_SHOUT_SUBMENU_PANE)
		{
		// This must be a shoutbox
		CMobblerShoutbox* shoutbox(static_cast<CMobblerShoutbox*>(iWebServicesControl->TopControl()));
		
		HBufC* shoutTextUser(shoutbox->ShoutAtTextUserLC());
		HBufC* shoutTextOwner(shoutbox->ShoutAtTextOwnerLC());

		aMenuPane->SetItemTextL(EMobblerCommandShoutUser, *shoutTextUser);

		shoutTextUser->Compare(*shoutTextOwner) == 0 ?
			aMenuPane->SetItemDimmed(EMobblerCommandShoutOwner, ETrue) :
			aMenuPane->SetItemTextL(EMobblerCommandShoutOwner, *shoutTextOwner);

		CleanupStack::PopAndDestroy(shoutTextOwner);
		CleanupStack::PopAndDestroy(shoutTextUser);
		}
	}

TUid CMobblerWebServicesView::Id() const
	{
	return TUid::Uid(0xA000B6CE);
	}

void CMobblerWebServicesView::HandleCommandL(TInt aCommand)
	{
	iWebServicesControl->HandleListCommandL(aCommand);
	
	// let the app ui handle the event
	switch (aCommand)
		{
		case EAknSoftkeyBack:
			iWebServicesControl->Back();
			break;
		case EAknSoftkeyExit:
			AppUi()->HandleCommandL(aCommand);
			break;
		case EMobblerCommandRadioStart:
		case EMobblerCommandRadioPersonal:
		case EMobblerCommandRadioNeighbourhood:
		case EMobblerCommandRadioLoved:
			// we have started a radio station so switch back to the status view
			AppUi()->ActivateLocalViewL(TUid::Uid(0xA0007CA8)); 
			break;
		default:
			// do nothing
			break;
		}
	}

void CMobblerWebServicesView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	if (!iWebServicesControl)
		{
		// create the first view
		iWebServicesControl = CMobblerWebServicesControl::NewL(*static_cast<CMobblerAppUi*>(AppUi()), AppUi()->ClientRect(), aCustomMessageId, aCustomMessage);
		iWebServicesControl->SetMopParent(AppUi());
		
		iWebServicesControl->ActivateL();
		// activate the top control in order
		AppUi()->AddToStackL(*this, iWebServicesControl);
		}
	}

void CMobblerWebServicesView::DoDeactivate()
	{
	if (iWebServicesControl)
		{
		// deactivate the top control
		AppUi()->RemoveFromStack(iWebServicesControl);
		delete iWebServicesControl;
		iWebServicesControl = NULL;
		}
	}

void CMobblerWebServicesView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	}

// End of file
