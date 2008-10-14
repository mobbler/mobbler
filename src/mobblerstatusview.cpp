/*
mobblerstatusview.cpp

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

#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include <akncontext.h>
#include <akntitle.h>
#include <stringloader.h>
#include <barsread.h>
#include <eikbtgpc.h>
#include <mobbler.rsg>
#include <aknprogressdialog.h>
#include <icl\imagecodecdata.h>
#include <gulicon.h>

#include "mobbler.hrh"
#include "mobblerstatusview.h"
#include "mobblerstatuscontrol.h"
#include "mobblerappui.h"
#include "mobblertrack.h"

CMobblerStatusView* CMobblerStatusView::NewL()
	{
	CMobblerStatusView* self = new(ELeave) CMobblerStatusView;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerStatusView::CMobblerStatusView()
	{
	}


CMobblerStatusView::~CMobblerStatusView()
	{
	delete iMobblerStatusControl;
	iMobblerStatusControl = NULL;
	}

void CMobblerStatusView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_STATUS_VIEW);
	SetupStatusPaneL();
	}

void CMobblerStatusView::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/) 
	{
	}

void CMobblerStatusView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
	{
	if (aResourceId == R_MOBBLER_STATUS_MENU_PANE)
		{
		if (static_cast<CMobblerAppUi*>(AppUi())->Mode() == CMobblerLastFMConnection::EOnline)
			{
			aMenuPane->SetItemDimmed(EMobblerCommandOnline, ETrue);
			}
		else
			{
			aMenuPane->SetItemDimmed(EMobblerCommandOffline, ETrue);
			}
		
		//TInt index(KErrNotFound);
		//aMenuPane->ItemAndPos(EMobblerCommandRadio, index)->CreateIconL(iRadioBitmap->Bitmap(), iRadioBitmap->Mask());
		//aMenuPane->ItemAndPos(EMobblerCommandRadio, index)->SetBitmapsOwnedExternally(ETrue);
		}
	/*
	else if (R_RADIO_SUBMENU_PANE == aResourceId)
		{
		TInt index(KErrNotFound);
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetIconBitmapL(iRadioLovedBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetIconMaskL(iRadioLovedBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetBitmapsOwnedExternally(ETrue);
		
		aMenuPane->ItemAndPos(EMobblerCommandRadioPersonal, index)->SetIconBitmapL(iRadioPersonalBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioPersonal, index)->SetIconMaskL(iRadioPersonalBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioPersonal, index)->SetBitmapsOwnedExternally(ETrue);
		
		aMenuPane->ItemAndPos(EMobblerCommandRadioNeighbourhood, index)->SetIconBitmapL(iRadioNeighboursBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioNeighbourhood, index)->SetIconMaskL(iRadioNeighboursBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioNeighbourhood, index)->SetBitmapsOwnedExternally(ETrue);
		
		aMenuPane->ItemAndPos(EMobblerCommandRadioMyPlaylist, index)->SetIconBitmapL(iRadioPlaylistBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioMyPlaylist, index)->SetIconMaskL(iRadioPlaylistBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioMyPlaylist, index)->SetBitmapsOwnedExternally(ETrue);
		
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetIconBitmapL(iRadioLovedBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetIconMaskL(iRadioLovedBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioLoved, index)->SetBitmapsOwnedExternally(ETrue);
		
		aMenuPane->ItemAndPos(EMobblerCommandRadioRecommedations, index)->SetIconBitmapL(iRadioRecommendedBitmap->Bitmap());
		aMenuPane->ItemAndPos(EMobblerCommandRadioRecommedations, index)->SetIconMaskL(iRadioRecommendedBitmap->Mask());
		aMenuPane->ItemAndPos(EMobblerCommandRadioRecommedations, index)->SetBitmapsOwnedExternally(ETrue);
		}*/
	}

TUid CMobblerStatusView::Id() const
	{
	return TUid::Uid(0xA0007CA8);
	}

void CMobblerStatusView::HandleCommandL(TInt aCommand)
	{
	// let the app ui handle the event
	AppUi()->HandleCommandL(aCommand);
	}

void CMobblerStatusView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	if (!iMobblerStatusControl)
		{
		iMobblerStatusControl = CMobblerStatusControl::NewL(ClientRect(), *static_cast<CMobblerAppUi*>(AppUi()));
		iMobblerStatusControl->SetMopParent(AppUi());
		iMobblerStatusControl->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerStatusControl);
		}
	else
		{
		iMobblerStatusControl->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerStatusControl);
		}
	}

void CMobblerStatusView::DoDeactivate()
	{
	if (iMobblerStatusControl)
		{
		AppUi()->RemoveFromStack(iMobblerStatusControl);
		//delete iMobblerStatusControl;
		//iMobblerStatusControl = NULL;
		}
	}

void CMobblerStatusView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	TRAP_IGNORE(SetupStatusPaneL()); 
	}

void CMobblerStatusView::SetupStatusPaneL()
	{
	TUid contextPaneUid = TUid::Uid( EEikStatusPaneUidContext );
	CEikStatusPaneBase::TPaneCapabilities subPaneContext = StatusPane()->PaneCapabilities(contextPaneUid);
	
	if ( subPaneContext.IsPresent() && subPaneContext.IsAppOwned() )
		{
		CAknContextPane* context = static_cast<CAknContextPane*>(StatusPane()->ControlL(contextPaneUid ));
		context->SetPictureToDefaultL();
		}
	
	// setup the title pane
	TUid titlePaneUid = TUid::Uid( EEikStatusPaneUidTitle );
	CEikStatusPaneBase::TPaneCapabilities subPaneTitle = StatusPane()->PaneCapabilities(titlePaneUid);
	
	if ( subPaneTitle.IsPresent() && subPaneTitle.IsAppOwned() )
		{
		CAknTitlePane* title = static_cast<CAknTitlePane*>(StatusPane()->ControlL(titlePaneUid));
		TResourceReader reader;
		iEikonEnv->CreateResourceReaderLC(reader, R_MOBBLER_TITLE_RESOURCE);
		title->SetFromResourceL(reader);
		CleanupStack::PopAndDestroy();
		}		
	}
/*
void CMobblerStatusView::ExecuteConnectingDialogLD(const TDesC* aOverrideText)
	{
	iConnectingDialog = new (ELeave) CAknWaitDialog(reinterpret_cast<CEikDialog**>(&iConnectingDialog), ETrue);
	
	if ( aOverrideText != NULL )
		{
		iConnectingDialog->SetTextL( *aOverrideText );
		}
	iConnectingDialog->ExecuteLD(R_MOBBLER_CONNECTING_DIALOG);
	}

void CMobblerStatusView::RemoveConnectingDialogL()
	{
	if ( iConnectingDialog != NULL )
		{
		iConnectingDialog->SetCallback( NULL );
		iConnectingDialog->ProcessFinishedL();    // deletes the dialog
		iConnectingDialog = NULL;
		}
	}

void CMobblerStatusView::SetConnectingDialogCallbackL(MProgressDialogCallback* aCallback)
	{
	if ( iConnectingDialog != NULL )
		{
		iConnectingDialog->SetCallback(aCallback);
		}
	}

void CMobblerStatusView::SetDialogToNull()
	{
	iConnectingDialog = NULL;
	}
*/
void CMobblerStatusView::DrawDeferred() const
	{
	if (iMobblerStatusControl)
		{
		iMobblerStatusControl->DrawDeferred();
		}
	}
