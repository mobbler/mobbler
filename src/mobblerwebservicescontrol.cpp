/*
mobblerwebservicescontrol.cpp

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

#include <aknnavide.h>
#include <aknnavilabel.h>

#include "mobblerappui.h"
#include "mobblerlistcontrol.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerwebservicescontrol.h"

#include "mobbler.hrh"

CMobblerWebServicesControl* CMobblerWebServicesControl::NewL(CMobblerAppUi& aAppUi, const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	CMobblerWebServicesControl* self = new(ELeave) CMobblerWebServicesControl(aAppUi);
	CleanupStack::PushL(self);
	self->ConstructL(aRect, aCustomMessageId, aCustomMessage);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerWebServicesControl::CMobblerWebServicesControl(CMobblerAppUi& aAppUi)
	:iAppUi(aAppUi)
	{
	}

void CMobblerWebServicesControl::ConstructL(const TRect& aRect, TUid aCustomMessageId, const TDesC8& aCustomMessage)
	{
	CreateWindowL(); // This is a window owning control
	InitComponentArrayL();
	SetRect(aRect);
	
	CMobblerListControl* control(NULL);
	
	switch (aCustomMessageId.iUid)
		{
		case EMobblerCommandFriends:
		case EMobblerCommandUserTopArtists:
		case EMobblerCommandRecommendedArtists:
		case EMobblerCommandRecommendedEvents:
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandPlaylists:
		case EMobblerCommandUserEvents:
		case EMobblerCommandArtistEvents:
		case EMobblerCommandUserTopTags:
		case EMobblerCommandRecentTracks:
		case EMobblerCommandUserShoutbox:
		case EMobblerCommandArtistShoutbox:
		case EMobblerCommandEventShoutbox:
		case EMobblerCommandArtistTopAlbums:
		case EMobblerCommandArtistTopTracks:
		case EMobblerCommandArtistTopTags:
			control = CMobblerListControl::CreateListL(iAppUi, *this, aCustomMessageId.iUid, aCustomMessage, KNullDesC8);
			break;
		case EMobblerCommandSimilarArtists:
		case EMobblerCommandSimilarTracks:
			// These can only be called if there is a current track playing
			if (iAppUi.CurrentTrack())
				{
				control = CMobblerListControl::CreateListL(iAppUi, *this, aCustomMessageId.iUid, iAppUi.CurrentTrack()->Artist().String8(), iAppUi.CurrentTrack()->Title().String8());
				}
			
		default:
			// we should panic if we get here
			break;
		};
	
	iControls.AppendL(control);
	control->SetMopParent(&iAppUi);
	control->ActivateL();
	iAppUi.AddToStackL(control);
	
	iNaviContainer = static_cast<CAknNavigationControlContainer*>(iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi)));
	iNaviLabelDecorator = iNaviContainer->CreateNavigationLabelL();
	iNaviContainer->PushL(*iNaviLabelDecorator);
	
	iAppUi.LastFMConnection().AddStateChangeObserverL(this);
	
	ChangePaneTextL();
	}

CMobblerWebServicesControl::~CMobblerWebServicesControl()
	{
	iAppUi.LastFMConnection().RemoveStateChangeObserver(this);
	
	// remove the top control from the stack
	iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
	iControls.ResetAndDestroy();
	
	iNaviContainer->Pop(iNaviLabelDecorator);
	delete iNaviLabelDecorator;
	}

CMobblerListControl* CMobblerWebServicesControl::TopControl()
	{
	return iControls[iControls.Count() - 1];
	}

void CMobblerWebServicesControl::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list = iControls[iControls.Count() - 1]->HandleListCommandL(aCommand);
	
	if (list)
		{
		// remove the old top control from the stack
		iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
		
		// add and activate the new list 
		iControls.AppendL(list);
		list->SetMopParent(&iAppUi);
		list->ActivateL();
		iAppUi.AddToStackL(list);
		
		DrawDeferred();
		
		ChangePaneTextL();
		}
	}

void CMobblerWebServicesControl::HandleConnectionStateChangedL()
	{
	ChangePaneTextL();
	}

void CMobblerWebServicesControl::HandleListControlStateChangedL()
	{
	ChangePaneTextL();
	}

void CMobblerWebServicesControl::ChangePaneTextL()
	{
	TBuf<255> text;
	
	text.AppendFill('>', Max(0, iControls.Count() - 1));
	
	CMobblerListControl::TState state = iControls.Count() > 0 ? TopControl()->State() : CMobblerListControl::ELoading;
	
	switch (state)
		{
		case CMobblerListControl::ELoading:
			switch (iAppUi.LastFMConnection().State())
				{
				case CMobblerLastFMConnection::EConnecting:
					text.Append(_L("Connecting"));	// TODO localise
					break;
				case CMobblerLastFMConnection::EHandshaking:
					text.Append(_L("Handshaking"));	// TODO localise
					break;
				case CMobblerLastFMConnection::ENone:
					text.Append(_L("Loading"));		// TODO localise
					break;
				}
			break;
		case CMobblerListControl::ENormal:
			if (iControls.Count() > 0)
				{
				HBufC* name = TopControl()->NameL();
				text.Append(*name);
				delete name;
				}
			break;
		case CMobblerListControl::EFailed:
			text.Append(_L("Failed"));				// TODO localise
			break;
		default:
			break;
		}
	
	if (iNaviLabelDecorator)
		{
		static_cast<CAknNaviLabel*>(iNaviLabelDecorator->DecoratedControl())->SetTextL(text);
		iNaviContainer->Pop();
		iNaviContainer->PushL(*iNaviLabelDecorator);	
		}
	}

void CMobblerWebServicesControl::Back()
	{
	if (iControls.Count() == 1)
		{
		// switch back to the staus view
		iAppUi.ActivateLocalViewL(TUid::Uid(0xA0007CA8)); // switch back to the status view		
		}
	else
		{
		// remove and delete the top control
		iAppUi.RemoveFromStack(iControls[iControls.Count() - 1]);
		delete iControls[iControls.Count() - 1];
		iControls.Remove(iControls.Count() - 1);

		// add the new top control to the stack
		iAppUi.AddToStackL(iControls[iControls.Count() - 1]);
		
		ChangePaneTextL();
		}
	
	DrawDeferred();
	}

TKeyResponse CMobblerWebServicesControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode /*aEventCode*/)
	{
	TKeyResponse response(EKeyWasNotConsumed);
	RArray<TInt> commands;
	CleanupClosePushL(commands);
			
	switch (aKeyEvent.iCode)
		{
		case EKeyLeftArrow:
			// the user pressed left so try to go up a control
			Back();
			response = EKeyWasConsumed;
			break;
		case EKeyRightArrow:
			TopControl()->SupportedCommandsL(commands);
			
			if (commands.Find(EMobblerCommandOpen) != KErrNotFound)
				{
				HandleListCommandL(EMobblerCommandOpen);
				}
			
			response = EKeyWasConsumed;
			break;
		default:
			break;
		}
	
	DrawDeferred();
	
	CleanupStack::PopAndDestroy(&commands);
	
	ChangePaneTextL();
	
	return response;
	}


void CMobblerWebServicesControl::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
   	gc.Clear(Rect());
	}

CCoeControl* CMobblerWebServicesControl::ComponentControl(TInt /*aIndex*/) const
	{
	return iControls[iControls.Count() - 1];
	}
 
TInt CMobblerWebServicesControl::CountComponentControls() const
	{
	return 1;
	}

// End of file
