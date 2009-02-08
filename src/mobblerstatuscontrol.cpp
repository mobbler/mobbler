/*
mobblerstatuscontrol.cpp

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
#include <aknsbasicbackgroundcontrolcontext.h>
#include <aknsdrawutils.h>
#include <aknutils.h>
#include <icl/imagecodecdata.h>
#include <mobbler_strings.rsg>

#ifdef  __S60_50__
#include <mobbler/mobblertouchfeedbackinterface.h>
#include <touchfeedback.h>
#endif

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblermarquee.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstring.h"
#include "mobblertimeout.h"
#include "mobblertrack.h"

_LIT(KPngSpeakerLow, "\\resource\\apps\\mobbler\\speaker_low.png");
_LIT(KPngSpeakerHigh, "\\resource\\apps\\mobbler\\speaker_high.png");
_LIT(KPngScrobble, "\\resource\\apps\\mobbler\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler\\icon_track.png");
_LIT(KPngBan, "\\resource\\apps\\mobbler\\ban.png");
_LIT(KPngLove, "\\resource\\apps\\mobbler\\love.png");
_LIT(KPngNext, "\\resource\\apps\\mobbler\\next.png");
_LIT(KPngPlay, "\\resource\\apps\\mobbler\\play.png");
_LIT(KPngPound, "\\resource\\apps\\mobbler\\pound.png");
_LIT(KPngStop, "\\resource\\apps\\mobbler\\stop.png");
_LIT(KPngLastFM, "\\resource\\apps\\mobbler\\lastfm.png");

const TRgb KRgbLastFMRed(0xD5, 0x10, 0x07, 0xFF);

const TRgb KRgbProgressBarBack(0xE7, 0xED, 0xEF, 0xFF);
const TRgb KRgbProgressBarBuffer(0xAF, 0xBE, 0xCC, 0xFF);
const TRgb KRgbProgressBarPlayback(0xD5, 0x10, 0x07, 0xFF);
const TRgb KRgbTransparent(0x00, 0x00, 0x00, 0x00);

const TUid KMusicAppUID = {0x102072C3};
const TUid KMobblerUID = {0xA0007648};

const TUid KTouchFeedbackImplUID = {0xA000B6CD};

void CMobblerStatusControl::CMobblerPaneTextCallback::ExecuteLD(const CMobblerStatusControl& aStatusControl, const TDesC& aText)
	{
	CMobblerPaneTextCallback* self = new(ELeave) CMobblerPaneTextCallback(aStatusControl);
	CleanupStack::PushL(self);
	self->ConstructL(aText);
	CleanupStack::Pop(self);
	}

void CMobblerStatusControl::CMobblerPaneTextCallback::ConstructL(const TDesC& aText)
	{
	iText = aText.AllocL();
	
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);
	SetActive();
	}

void CMobblerStatusControl::CMobblerPaneTextCallback::RunL()
	{
	iStatusControl.DoChangePaneTextL(*iText);
	delete this;
	}
	
void CMobblerStatusControl::CMobblerPaneTextCallback::DoCancel()
	{
	// nothing to cancel
	}

CMobblerStatusControl::CMobblerPaneTextCallback::CMobblerPaneTextCallback(const CMobblerStatusControl& aStatusControl)
	:CActive(EPriorityStandard), iStatusControl(aStatusControl)
	{
	CActiveScheduler::Add(this);
	}

CMobblerStatusControl::CMobblerPaneTextCallback::~CMobblerPaneTextCallback()
	{
	delete iText;
	}

CMobblerStatusControl* CMobblerStatusControl::NewL(const TRect& aRect, const CMobblerAppUi& aAppUi)
	{
	CMobblerStatusControl* self = new(ELeave) CMobblerStatusControl(aAppUi);
	CleanupStack::PushL(self);
	self->ConstructL(aRect);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerStatusControl::CMobblerStatusControl(const CMobblerAppUi& aAppUi)
	:iAppUi(aAppUi),
	iVolumeUpCallBack(TCallBack(CMobblerStatusControl::VolumeUpCallBackL, this)),
	iVolumeDownCallBack(TCallBack(CMobblerStatusControl::VolumeDownCallBackL, this))
	{
	}

void CMobblerStatusControl::ConstructL(const TRect& aRect)
	{
#ifdef  __S60_50__
	TRAP_IGNORE(iMobblerFeedback = static_cast<CMobblerTouchFeedbackInterface*>(REComSession::CreateImplementationL(KTouchFeedbackImplUID, iDtorIdKey)));	
#endif
	
	// No parent owner, so create an own window
	CreateWindowL();
	
	// Initialize component array
	InitComponentArrayL();
	SetRect(aRect);
	
	iBgContext = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgAreaMain, aRect, ETrue);
	
	iNaviContainer = static_cast<CAknNavigationControlContainer*>(iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi)));
	iNaviLabelDecorator = iNaviContainer->CreateNavigationLabelL();
	iNaviContainer->PushL(*iNaviLabelDecorator);
	
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();

	LoadGraphicsL();
	LoadResourceFileTextL();

	ReleaseBackBuffer();
	CreateBackBufferL();

	iMobblerVolumeTimeout = CMobblerTimeout::NewL(2000000);
	iMobblerMarquee = CMobblerMarquee::NewL();

#ifdef _DEBUG
	iAlbumNameInMarquee = EFalse;
#endif
	}

TInt CMobblerStatusControl::VolumeUpCallBackL(TAny *aSelf)
	{
	CMobblerStatusControl* self = static_cast<CMobblerStatusControl*>(aSelf);
	TKeyEvent event;
	event.iCode = EKeyIncVolume;
	self->OfferKeyEventL(event, EEventNull);
	return KErrNone;
	}

TInt CMobblerStatusControl::VolumeDownCallBackL(TAny *aSelf)
	{
	CMobblerStatusControl *self = static_cast<CMobblerStatusControl*>(aSelf);
	TKeyEvent event;
	event.iCode = EKeyDecVolume;
	self->OfferKeyEventL(event, EEventNull);
	return KErrNone;
	}

void CMobblerStatusControl::ChangePaneTextL(const TDesC& aText) const
	{
	CMobblerStatusControl::CMobblerPaneTextCallback::ExecuteLD(*this, aText);
	}

void CMobblerStatusControl::DoChangePaneTextL(const TDesC& aText) const
	{
	if (static_cast<CAknNaviLabel*>(iNaviLabelDecorator->DecoratedControl())->Text()->Compare(aText) != 0)
		{
		// the text is different so change it
		static_cast<CAknNaviLabel*>(iNaviLabelDecorator->DecoratedControl())->SetTextL(aText);
		iNaviContainer->Pop();
		iNaviContainer->PushL(*iNaviLabelDecorator);
		}
	}

void CMobblerStatusControl::LoadGraphicsL()
	{
	iMobblerBitmapBan = CMobblerBitmap::NewL(*this, KPngBan, KImageTypePNGUid);
	iMobblerBitmapPound = CMobblerBitmap::NewL(*this, KPngPound, KImageTypePNGUid);
	iMobblerBitmapLove = CMobblerBitmap::NewL(*this, KPngLove, KImageTypePNGUid);
	iMobblerBitmapPlay = CMobblerBitmap::NewL(*this, KPngPlay, KImageTypePNGUid);
	iMobblerBitmapNext = CMobblerBitmap::NewL(*this, KPngNext, KImageTypePNGUid);
	iMobblerBitmapStop = CMobblerBitmap::NewL(*this, KPngStop, KImageTypePNGUid);
	iMobblerBitmapLastFM = CMobblerBitmap::NewL(*this, KPngLastFM, KImageTypePNGUid);
	iMobblerBitmapSpeakerLow = CMobblerBitmap::NewL(*this, KPngSpeakerLow, KImageTypePNGUid);
	iMobblerBitmapSpeakerHigh = CMobblerBitmap::NewL(*this, KPngSpeakerHigh, KImageTypePNGUid);
	iMobblerBitmapScrobble = CMobblerBitmap::NewL(*this, KPngScrobble, KImageTypePNGUid);
	iMobblerBitmapTrackIcon = CMobblerBitmap::NewL(*this, KPngTrackIcon, KImageTypePNGUid);
    
	// Load the Music Player icon to display when a music player track is playing
	iMobblerBitmapMusicAppIcon = CMobblerBitmap::NewL(KMusicAppUID);
	
	// Load the Mobbler icon to display when a music player track is not playing
	iMobblerBitmapAppIcon = CMobblerBitmap::NewL(KMobblerUID);
	}

void CMobblerStatusControl::LoadResourceFileTextL()
	{
	CMobblerResourceReader* resourceReader = CMobblerResourceReader::NewL();
	resourceReader->AddResourceFileL(KLanguageRscFile, KLanguageRscVersion);

	iResTextScrobbledQueuedFormat = resourceReader->AllocReadL(R_MOBBLER_SCROBBLED_QUEUED);
	iResTextOffline = resourceReader->AllocReadL(R_MOBBLER_IDLE);
	iResTextOnline = resourceReader->AllocReadL(R_MOBBLER_IDLE);
	iResTextArtist = resourceReader->AllocReadL(R_MOBBLER_ARTIST);
	iResTextTitle = resourceReader->AllocReadL(R_MOBBLER_TITLE);
	iResTextAlbum = resourceReader->AllocReadL(R_MOBBLER_ALBUM);
	iResTextStateOnline = resourceReader->AllocReadL(R_MOBBLER_STATE_ONLINE);
	iResTextStateOffline = resourceReader->AllocReadL(R_MOBBLER_STATE_OFFLINE);
	iResTextStateConnecting = resourceReader->AllocReadL(R_MOBBLER_STATE_CONNECTING);
	iResTextStateHandshaking = resourceReader->AllocReadL(R_MOBBLER_STATE_HANDSHAKING);
	iResTextStateSelectingStation = resourceReader->AllocReadL(R_MOBBLER_STATE_SELECTING_STATION);
	iResTextStateFetchingPlaylist = resourceReader->AllocReadL(R_MOBBLER_STATE_FETCHING_PLAYLIST);
	iResTextStateCheckingForUpdates = resourceReader->AllocReadL(R_MOBBLER_STATE_CHECKING_FOR_UPDATES);

	delete resourceReader;
	}

void CMobblerStatusControl::SetPositions()
	{
	// The height of the text bars
	const TInt KTextRectHeight(iMobblerFont->HeightInPixels() + iMobblerFont->DescentInPixels() + 2);
		
	// Check if the device is in portrait or landscape mode
	if (Size().iWidth <= (Size().iHeight * 11) / 10)
		{
		// Portrait graphics positions
		
		if (iMobblerFeedback)
			{
			// 5th edition
			TInt albumArtDimension = Rect().Width() - (3 * KTextRectHeight);
								
			iRectAlbumArt = 			TRect(TPoint((3 * KTextRectHeight) / 2, 0), TSize(albumArtDimension, albumArtDimension));
			
			iControlSize = TSize(64, 64);
			
			iPointBuy =					TPoint((KTextRectHeight/2) + (0 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - KTextRectHeight);
			iPointLove =				TPoint((KTextRectHeight/2) + (1 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - KTextRectHeight);
			iPointBan =					TPoint((KTextRectHeight/2) + (2 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - KTextRectHeight);
			iPointPlayStop =			TPoint((KTextRectHeight/2) + (3 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - KTextRectHeight);
			iPointSkip =				TPoint((KTextRectHeight/2) + (4 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - KTextRectHeight);
			
			iPointLastFM = 				TPoint(-200, -200);
					
			iRectTrackDetailsText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
			iRectScrobbledQueuedText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - ((5 * KTextRectHeight) / 2) - iControlSize.iHeight), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
			iRectProgressBar = 				TRect(TPoint(KTextRectHeight / 2, (iRectTrackDetailsText.iTl.iY + iRectScrobbledQueuedText.iTl.iY) / 2 ), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));				
			}
		else
			{
			// 3rd edition
			TInt albumArtDimension = Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - ((3 * KTextRectHeight) / 2);
			
			iControlSize = TSize(27, 27);
			
			iRectAlbumArt = 			TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
			TPoint controlsTopLeft = 	TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - (KTextRectHeight / 2), iRectAlbumArt.iBr.iY - (3 * iControlSize.iHeight) - 4);
			iPointBuy =					TPoint(controlsTopLeft.iX + (0 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointLove =				TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (0 * iControlSize.iHeight));
			iPointBan =					TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (2 * iControlSize.iHeight));
			iPointSkip =				TPoint(controlsTopLeft.iX + (2 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointPlayStop =			TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			
			iPointLastFM = 				TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth -  (KTextRectHeight / 2), iRectAlbumArt.iTl.iY + 4);
					
			iRectTrackDetailsText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
			iRectScrobbledQueuedText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - ((3 * KTextRectHeight) / 2)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
			iRectProgressBar = 				TRect(TPoint(KTextRectHeight / 2, (iRectTrackDetailsText.iTl.iY + iRectScrobbledQueuedText.iTl.iY) / 2 ), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));				
			}
		}
	else
		{
		// Landscape graphics positions
		
		if (iMobblerFeedback)
			{
			// 5th edition
			TInt albumArtDimension = Rect().Height() - (3 * KTextRectHeight);
						
			iRectAlbumArt =				TRect(TPoint(0, (3 * KTextRectHeight) / 2), TSize(albumArtDimension, albumArtDimension));
			
			iPointLastFM =				TPoint(albumArtDimension + (KTextRectHeight / 2), iRectAlbumArt.iTl.iY);

			iControlSize = TSize((Rect().Width() - albumArtDimension) / 5, (Rect().Width() - albumArtDimension) / 5);
						
			iPointBuy =					TPoint(albumArtDimension + (KTextRectHeight / 2) + (0 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointLove =				TPoint(albumArtDimension + (KTextRectHeight / 2) + (1 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointBan =					TPoint(albumArtDimension + (KTextRectHeight / 2) + (2 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointPlayStop =			TPoint(albumArtDimension + (KTextRectHeight / 2) + (3 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointSkip =				TPoint(albumArtDimension + (KTextRectHeight / 2) + (4 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			
			iRectTrackDetailsText = 	TRect(TPoint(albumArtDimension + (KTextRectHeight / 2) + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - iControlSize.iHeight - (5 * KTextRectHeight)), TSize(Rect().Width() - albumArtDimension - iMobblerBitmapTrackIcon->SizeInPixels().iWidth - KTextRectHeight, KTextRectHeight));
			iRectScrobbledQueuedText = 	TRect(TPoint(albumArtDimension + (KTextRectHeight / 2) + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - iControlSize.iHeight - (2 * KTextRectHeight)), TSize(Rect().Width() - albumArtDimension - iMobblerBitmapTrackIcon->SizeInPixels().iWidth - KTextRectHeight, KTextRectHeight));
			iRectProgressBar = 			TRect(TPoint(albumArtDimension + (KTextRectHeight / 2), (iRectTrackDetailsText.iTl.iY + iRectScrobbledQueuedText.iTl.iY) / 2 ), TSize(Rect().Width() - albumArtDimension - KTextRectHeight, KTextRectHeight));							
			}
		else
			{
			// 3rd edition
			TInt albumArtDimension = Min(Rect().Height() - ((3 * KTextRectHeight) / 2), Rect().Width() - (2 * KTextRectHeight) - (3 * iMobblerBitmapPlay->SizeInPixels().iWidth) - iMobblerBitmapLastFM->SizeInPixels().iWidth);
			
			iRectAlbumArt =				TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
			
			iPointLastFM =				TPoint(albumArtDimension + ((3 * KTextRectHeight) / 2) + (3 * iMobblerBitmapPlay->SizeInPixels().iWidth), KTextRectHeight / 2);
			
			TPoint controlsTopLeft = 	TPoint(albumArtDimension + KTextRectHeight, KTextRectHeight / 2);
			iPointBuy =					TPoint(controlsTopLeft.iX + (0 * iMobblerBitmapPound->SizeInPixels().iWidth), controlsTopLeft.iY + (1 * iMobblerBitmapPound->SizeInPixels().iHeight));
			iPointLove =				TPoint(controlsTopLeft.iX + (1 * iMobblerBitmapPound->SizeInPixels().iWidth), controlsTopLeft.iY + (0 * iMobblerBitmapPound->SizeInPixels().iHeight));
			iPointBan =					TPoint(controlsTopLeft.iX + (1 * iMobblerBitmapPound->SizeInPixels().iWidth), controlsTopLeft.iY + (2 * iMobblerBitmapPound->SizeInPixels().iHeight));
			iPointSkip =				TPoint(controlsTopLeft.iX + (2 * iMobblerBitmapPound->SizeInPixels().iWidth), controlsTopLeft.iY + (1 * iMobblerBitmapPound->SizeInPixels().iHeight));
			iPointPlayStop =			TPoint(controlsTopLeft.iX + (1 * iMobblerBitmapPlay->SizeInPixels().iWidth), controlsTopLeft.iY + (1 * iMobblerBitmapPlay->SizeInPixels().iHeight));
	
			iControlSize = iMobblerBitmapPound->SizeInPixels();
			
			iRectTrackDetailsText = 	TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
			iRectScrobbledQueuedText = 	TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - ((3 * KTextRectHeight) / 2)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
			
			iRectProgressBar = 			TRect(TPoint(albumArtDimension + KTextRectHeight, iRectAlbumArt.iBr.iY - KTextRectHeight), TSize(Rect().Width() -((3 * KTextRectHeight) / 2) - albumArtDimension, KTextRectHeight));
			}
		}
	
	// Set the size of the application icons to be the same as the rect we will draw them to.
	// They don't seem to like being drawn at a different size
	
	iMobblerBitmapMusicAppIcon->SetSize(iRectAlbumArt.Size());
	iMobblerBitmapAppIcon->SetSize(iRectAlbumArt.Size());
	}

void CMobblerStatusControl::HandleResourceChange(TInt aType)
	{
	TRect rect;
	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);
		SetRect(rect);
		}
	
	CCoeControl::HandleResourceChange(aType);
	}

TTypeUid::Ptr CMobblerStatusControl::MopSupplyObject(TTypeUid aId)
	{
	if (iBgContext)
		{
		return MAknsControlContext::SupplyMopObject(aId, iBgContext);
		}
	
	return CCoeControl::MopSupplyObject(aId);
	}

void CMobblerStatusControl::SizeChanged()
	{
	if (iMobblerMarquee)
		{
		iMobblerMarquee->Reset();
		}
		
	if (iBgContext)
		{
		iBgContext->SetRect(Rect());
		
		if (&Window())
			{
			iBgContext->SetParentPos(PositionRelativeToScreen());
			}
		}
	
	ReleaseBackBuffer();
	CreateBackBufferL();
	
	SetPositions();
	}


void CMobblerStatusControl::CreateBackBufferL()
	{
	// Create back buffer bitmap
	iBackBuffer = new (ELeave) CFbsBitmap;

	User::LeaveIfError(iBackBuffer->Create(Size(), iEikonEnv->DefaultDisplayMode()));

	// Create back buffer graphics context
	iBackBufferDevice = CFbsBitmapDevice::NewL(iBackBuffer);
	User::LeaveIfError(iBackBufferDevice->CreateContext(iBackBufferContext));
	iBackBufferContext->SetPenStyle(CGraphicsContext::ESolidPen);

	iBackBufferSize = iBackBuffer->SizeInPixels();

	// Get and set the font to use
	iMobblerFont = iEikonEnv->AnnotationFont();
	iBackBufferContext->UseFont(iMobblerFont);
	}
 
void CMobblerStatusControl::ReleaseBackBuffer()
	{
	if (iMobblerFont)
		{
		iBackBufferContext->DiscardFont();
		//iBackBufferDevice->ReleaseFont(iMobblerFont);
		iMobblerFont = NULL;
		}

	// Release double buffering classes
	delete iBackBufferContext;
	iBackBufferContext = NULL;

	delete iBackBufferDevice;
	iBackBufferDevice = NULL;

	delete iBackBuffer;
	iBackBuffer = NULL;

	iBackBufferSize = TSize(0, 0);
	}


void CMobblerStatusControl::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	SetPositions();
	DrawDeferred();
	}

CMobblerStatusControl::~CMobblerStatusControl()
	{
	//iNaviContainer->Pop(iNaviLabelDecorator);
	//delete iNaviLabelDecorator;
	
	delete iBgContext;
	
	delete iMobblerBitmapBan;
	delete iMobblerBitmapPound;
	delete iMobblerBitmapLove;
	delete iMobblerBitmapPlay;
	delete iMobblerBitmapNext;
	delete iMobblerBitmapStop;
	delete iMobblerBitmapLastFM;
	delete iMobblerBitmapSpeakerLow;
	delete iMobblerBitmapSpeakerHigh;
	delete iMobblerBitmapScrobble;
	delete iMobblerBitmapTrackIcon;
	delete iMobblerBitmapAppIcon;
	
	delete iMobblerBitmapMusicAppIcon;
	
	delete iInterfaceSelector;
	
	delete iResTextScrobbledQueuedFormat;
	delete iResTextOffline;
	delete iResTextOnline;
	delete iResTextArtist;
	delete iResTextTitle;
	delete iResTextAlbum;
	delete iResTextStateOnline;
	delete iResTextStateOffline;
	delete iResTextStateConnecting;
	delete iResTextStateHandshaking;
	delete iResTextStateSelectingStation;
	delete iResTextStateFetchingPlaylist;
	delete iResTextStateCheckingForUpdates;
	
	ReleaseBackBuffer();
	
	iNaviContainer->Pop(iNaviLabelDecorator);
	delete iNaviLabelDecorator;
	
	delete iMobblerVolumeTimeout;
	delete iMobblerMarquee;
	
	delete iVolumeUpTimer;
	delete iVolumeDownTimer;
	
#ifdef  __S60_50__
	if (iMobblerFeedback)
		{
		delete iMobblerFeedback;
		REComSession::DestroyedImplementation(iDtorIdKey);
		}
#endif
	}

void CMobblerStatusControl::FormatTime(TDes& aString, TTimeIntervalSeconds aSeconds, TTimeIntervalSeconds aTotalSeconds)
	{
	TInt minutes = aSeconds.Int() / 60;
	TInt seconds = (aSeconds.Int() - (minutes * 60));
	aString.Zero();

	if ((aTotalSeconds.Int()) >= 3600) // 3,600 seconds = 60 minutes
		{
		TInt hours = minutes / 60;
		minutes -= (hours * 60);
		aString.AppendFormat(_L("%02d:%02d:%02d"), hours, minutes, seconds);
		}
	else
		{
		aString.AppendFormat(_L("%02d:%02d"), minutes, seconds);
		}
	}

void CMobblerStatusControl::FormatTrackDetails(TDes& aString, const TDesC& aArtist, const TDesC& aTitle)
	{
	aString.Zero();
	aString.AppendFormat(_L("%S - %S"), &aArtist, &aTitle);
	}

#ifdef _DEBUG
void CMobblerStatusControl::FormatAlbumDetails(TDes& aString, const TDesC& aAlbum1, const TDesC& aAlbum2)
	{
	aString.Zero();
	aString.AppendFormat(_L("%S: %S"), &aAlbum1, &aAlbum2);
	}
#endif

void CMobblerStatusControl::Draw(const TRect& /*aRect*/) const
	{
	if (iAppUi.Foreground() && iAppUi.Backlight())
		{
		User::ResetInactivityTime();
		}

	// Redraw the background using the default skin
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	MAknsControlContext* cc = AknsDrawUtils::ControlContext(this);
	AknsDrawUtils::DrawBackground(skin, cc, this, *iBackBufferContext, TPoint(0, 0), Rect(), KAknsDrawParamDefault);
	
	TInt playbackTotal(1);
	TInt playbackPosition(0);
	TInt bufferTotal(1);
	TInt bufferPosition(0);
	
	const CMobblerBitmap* albumArt(iMobblerBitmapAppIcon);
	
	TBool love(EFalse);
			
	if (iAppUi.CurrentTrack())
		{
		love = iAppUi.CurrentTrack()->Love();
		
		if (iAppUi.CurrentTrack()->RadioAuth().Compare(KNullDesC8) != 0)
			{
			// This is a radio song so display the name of the current radio station
			iStateText.Copy(iAppUi.RadioPlayer()->Station().String());
			
			if (iAppUi.CurrentTrack()->AlbumArt() && iAppUi.CurrentTrack()->AlbumArt()->Bitmap())
				{
				// the current track has album art and it has finished loading
				albumArt = iAppUi.CurrentTrack()->AlbumArt();
				}
			}
		else
			{
			// This is a music player track
			iStateText.Copy(iAppUi.MusicAppNameL());
			albumArt = iMobblerBitmapMusicAppIcon;
			}
	
#ifdef _DEBUG
		if (iAlbumNameInMarquee && 
		    (iAppUi.CurrentTrack()->Album().String().Length() > 0))
			FormatAlbumDetails(iTrackDetailsText, *iResTextAlbum, iAppUi.CurrentTrack()->Album().String());
		else
#endif
		FormatTrackDetails(iTrackDetailsText, iAppUi.CurrentTrack()->Artist().String(), iAppUi.CurrentTrack()->Title().String());

		playbackTotal = iAppUi.CurrentTrack()->TrackLength().Int();
		playbackPosition = Min(iAppUi.CurrentTrack()->PlaybackPosition().Int(), playbackTotal);
		
		FormatTime(iPlaybackGoneText, iAppUi.CurrentTrack()->PlaybackPosition(), playbackTotal);
		if (playbackTotal > playbackPosition)
			{
			FormatTime(iPlaybackLeftText, playbackTotal - playbackPosition, playbackTotal);
			}
		else
			{
			FormatTime(iPlaybackLeftText, 0, playbackTotal);
			}
		
		bufferTotal = iAppUi.CurrentTrack()->DataSize();
		bufferPosition = Min(iAppUi.CurrentTrack()->Buffered(), bufferTotal);
		}
	else
		{
		FormatTrackDetails(iTrackDetailsText, *iResTextArtist, *iResTextTitle);
		FormatTime(iPlaybackLeftText, 0);
		FormatTime(iPlaybackGoneText, 0);
		
		// Decide on the state text to display
		switch (iAppUi.State())
			{
			case CMobblerLastFMConnection::ENone:
				if (iAppUi.Mode() == CMobblerLastFMConnection::EOnline)
					{
					iStateText.Copy(*iResTextStateOnline);
					}
				else
					{
					iStateText.Copy(*iResTextStateOffline);
					}
				break;
			case CMobblerLastFMConnection::EConnecting:
				iStateText.Copy(*iResTextStateConnecting);
				break;
			case CMobblerLastFMConnection::EHandshaking:
				iStateText.Copy(*iResTextStateHandshaking);
				break;
			case CMobblerLastFMConnection::ERadioSelect:
				iStateText.Copy(*iResTextStateSelectingStation);
				break;
			case CMobblerLastFMConnection::ERadioPlaylist:
				iStateText.Copy(*iResTextStateFetchingPlaylist);
				break;
			case CMobblerLastFMConnection::EUpdates:
				iStateText.Copy(*iResTextStateCheckingForUpdates);
				break;
			default:
				break;
			}
		}
	
	
	TRgb textColor; // text color when not highlighted
	AknsUtils::GetCachedColor(skin, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6);
	iBackBufferContext->SetPenColor(textColor);
	
	// Draw the album art
    //DrawRect(iRectAlbumArt, KRgbWhite, KRgbWhite);
	DrawMobblerBitmap(albumArt, iRectAlbumArt);
	
	// If the track has been loved, draw the love icon in the bottom right corner
	if (love)
		{
		DrawMobblerBitmap(iMobblerBitmapLove, TPoint(iRectAlbumArt.iBr.iX - iMobblerBitmapLove->SizeInPixels().iWidth - 4, iRectAlbumArt.iBr.iX - iMobblerBitmapLove->SizeInPixels().iHeight - 4));
		}
	
	ChangePaneTextL(iStateText);
	
	iScrobbledQueued.Zero();
	iScrobbledQueued.Format(*iResTextScrobbledQueuedFormat, iAppUi.Scrobbled(), iAppUi.Queued());
	
	DrawMobblerBitmap(iMobblerBitmapScrobble, TPoint(iRectScrobbledQueuedText.iTl.iX - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectScrobbledQueuedText.iTl.iY + 3));
	DrawText(iScrobbledQueued, iRectScrobbledQueuedText, textColor, CGraphicsContext::ELeft, iMobblerFont->WidthZeroInPixels());
	
	// draw the progress bar background
	DrawRect(iRectProgressBar, KRgbBlack, KRgbProgressBarBack);

	// Draw the progress bar line (either volume or track playback progress)
	if (!iMobblerVolumeTimeout->TimedOut())
		{
		// Draw the volume level progresses
		TRect rectBufferProgress = iRectProgressBar;
		rectBufferProgress.SetWidth((iAppUi.RadioPlayer()->Volume() * iRectProgressBar.Width()) / iAppUi.RadioPlayer()->MaxVolume());
		DrawRect(rectBufferProgress, KRgbTransparent, KRgbLastFMRed);
		
		TInt volumePercent = ((iAppUi.RadioPlayer()->Volume() * 100) / iAppUi.RadioPlayer()->MaxVolume());
		TBuf<6> volumeText;
		volumeText.AppendFormat(_L("%d%%"), volumePercent);
		DrawText(volumeText, iRectProgressBar, KRgbBlack, CGraphicsContext::ECenter, iMobblerFont->WidthZeroInPixels());
		
		// Draw the speaker icons
		DrawMobblerBitmap(iMobblerBitmapSpeakerLow, TPoint(iRectProgressBar.iTl.iX + iMobblerFont->WidthZeroInPixels(), iRectProgressBar.iTl.iY + 2));
		DrawMobblerBitmap(iMobblerBitmapSpeakerHigh, TPoint(iRectProgressBar.iBr.iX - iMobblerFont->WidthZeroInPixels() - iMobblerBitmapSpeakerHigh->SizeInPixels().iWidth, iRectProgressBar.iTl.iY + 2));
		}
	else
		{
		// Draw the playback progresses
		TRect rectBufferProgress = iRectProgressBar;
		rectBufferProgress.SetWidth((bufferPosition * iRectProgressBar.Width()) / bufferTotal);
		DrawRect(rectBufferProgress, KRgbTransparent, KRgbProgressBarBuffer);
		
		TRect rectPlaybackProgress = iRectProgressBar;
		rectPlaybackProgress.SetWidth((playbackPosition * iRectProgressBar.Width()) / playbackTotal);
		DrawRect(rectPlaybackProgress, KRgbTransparent, KRgbProgressBarPlayback);
		
		TInt scrobbleTime(playbackTotal);
		if (iAppUi.CurrentTrack())
			{
			scrobbleTime = iAppUi.CurrentTrack()->InitialPlaybackPosition().Int() + 
						   iAppUi.CurrentTrack()->ScrobbleDuration().Int();
			}
		if (scrobbleTime <= playbackTotal)
			{
			TInt scrobbleMark = iRectProgressBar.iTl.iX - 1 + 
				((iRectProgressBar.Width() * scrobbleTime) / playbackTotal);
			iBackBufferContext->SetPenColor(KRgbBlack);
			iBackBufferContext->SetPenStyle(CGraphicsContext::EDottedPen);
			iBackBufferContext->DrawLine(TPoint(scrobbleMark, iRectProgressBar.iTl.iY),
										 TPoint(scrobbleMark, iRectProgressBar.iBr.iY));
			}

		DrawText(iPlaybackLeftText, iRectProgressBar, KRgbBlack, CGraphicsContext::ERight, iMobblerFont->WidthZeroInPixels());
		DrawText(iPlaybackGoneText, iRectProgressBar, KRgbBlack, CGraphicsContext::ELeft, iMobblerFont->WidthZeroInPixels());
		}
	
	// Draw the controls
	DrawMobblerBitmap(iMobblerBitmapPound, TRect(iPointBuy, iControlSize));
	DrawMobblerBitmap(iMobblerBitmapLove, TRect(iPointLove, iControlSize));
	DrawMobblerBitmap(iMobblerBitmapBan, TRect(iPointBan, iControlSize));
	DrawMobblerBitmap(iMobblerBitmapNext, TRect(iPointSkip, iControlSize));
	
	// Draw either play or stop depending on the radio play state
	if (!iAppUi.RadioPlayer()->CurrentTrack())
		{
		// There is no current track so display the play button
		DrawMobblerBitmap(iMobblerBitmapPlay, TRect(iPointPlayStop, iControlSize));
		}
	else
		{
		// There is a track playing
		DrawMobblerBitmap(iMobblerBitmapStop, TRect(iPointPlayStop, iControlSize));
		}
	
	// Draw the Last.fm graphic
	DrawMobblerBitmap(iMobblerBitmapLastFM, iPointLastFM);
		
	// Draw the track details line
	iMobblerMarquee->Start(iTrackDetailsText, iMobblerFont->WidthZeroInPixels(), iMobblerFont->TextWidthInPixels(iTrackDetailsText), iRectTrackDetailsText.Width());
	DrawText(iTrackDetailsText, iRectTrackDetailsText, textColor, CGraphicsContext::ELeft, iMobblerMarquee->GetPosition1());
	if (iMobblerMarquee->GetPosition2() != KMaxTInt)
		{
		DrawText(iTrackDetailsText, iRectTrackDetailsText, textColor, CGraphicsContext::ELeft, iMobblerMarquee->GetPosition2());
		}
	
	DrawMobblerBitmap(iMobblerBitmapTrackIcon, TPoint(iRectTrackDetailsText.iTl.iX -  iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectTrackDetailsText.iTl.iY + 3));
		
	SystemGc().BitBlt(TPoint(0, 0), iBackBuffer);
	}

void CMobblerStatusControl::DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TRect& aRect) const
	{
	if (aMobblerBitmap)
		{
		if (aMobblerBitmap->Bitmap())
			{
			if (aMobblerBitmap->Mask())
				{
				iBackBufferContext->DrawBitmapMasked(aRect, aMobblerBitmap->Bitmap(), TRect(TPoint(0, 0), aMobblerBitmap->SizeInPixels()), aMobblerBitmap->Mask(), EFalse);
				}
			else
				{
				iBackBufferContext->DrawBitmap(aRect, aMobblerBitmap->Bitmap());
				}
			}
		}
	}

void CMobblerStatusControl::DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TPoint& aPoint) const
	{
	if (aMobblerBitmap)
		{
		if (aMobblerBitmap->Bitmap())
			{
			if (aMobblerBitmap->Mask())
				{
				iBackBufferContext->DrawBitmapMasked(TRect(aPoint, aMobblerBitmap->Bitmap()->SizeInPixels()), aMobblerBitmap->Bitmap(), TRect(TPoint(0, 0), aMobblerBitmap->Bitmap()->SizeInPixels()), aMobblerBitmap->Mask(), EFalse);
				}
			else
				{
				iBackBufferContext->DrawBitmap(aPoint, aMobblerBitmap->Bitmap());
				}
			}
		}
	}

void CMobblerStatusControl::DrawText(const TDesC& aText, const TRect& aRect, const TRgb& aPenColor, CGraphicsContext::TTextAlign aTextAlign, TInt aOffset) const
	{
	iBackBufferContext->SetDrawMode(CGraphicsContext::EDrawModePEN);
	iBackBufferContext->SetPenColor(aPenColor);
	iBackBufferContext->SetBrushColor(TRgb(255, 255, 255, 0));
	iBackBufferContext->SetBrushStyle(CGraphicsContext::ENullBrush);
	iBackBufferContext->DrawText(aText, aRect, aRect.Height() - iMobblerFont->DescentInPixels() - 1, aTextAlign, aOffset);
	}

void CMobblerStatusControl::DrawRect(const TRect& aRect, const TRgb& aPenColor, const TRgb& aBrushColor) const
	{
	iBackBufferContext->SetDrawMode(CGraphicsContext::EDrawModePEN);
	iBackBufferContext->SetPenColor(aPenColor);
	iBackBufferContext->SetBrushColor(aBrushColor);
	iBackBufferContext->SetBrushStyle(CGraphicsContext::ESolidBrush);
	iBackBufferContext->DrawRect(aRect);
	}
				
void CMobblerStatusControl::MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct)
	{
	// don't bother if there's a current music player track
	if ((iAppUi.CurrentTrack() && 
		(iAppUi.CurrentTrack()->RadioAuth().Compare(KNullDesC8) == 0)))
		{
		return;
		}
		
	TRequestStatus status;
	
	switch (aOperationId)
		{
		case ERemConCoreApiStop:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				TKeyEvent event;
				event.iCode = EKeyDevice3;
				OfferKeyEventL(event, EEventNull);
				}
			iCoreTarget->StopResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiForward:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				TKeyEvent event;
				event.iCode = EKeyRightArrow;
				OfferKeyEventL(event, EEventNull);
				}
			iCoreTarget->ForwardResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiVolumeUp:
			{   
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					{
					TKeyEvent event;
					event.iCode = EKeyIncVolume;
					OfferKeyEventL(event, EEventNull);
					break;
					}

				case ERemConCoreApiButtonPress:
				{
				TKeyEvent event;
				event.iCode = EKeyIncVolume;
				OfferKeyEventL(event, EEventNull);

					TTimeIntervalMicroSeconds32 repeatDelay;
					TTimeIntervalMicroSeconds32 repeatInterval;
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					
					delete iVolumeUpTimer;
					iVolumeUpTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeUpTimer->Start(repeatDelay, repeatInterval, iVolumeUpCallBack);
					break;
					}

				case ERemConCoreApiButtonRelease:
					delete iVolumeUpTimer;
					iVolumeUpTimer = NULL;
					break;

				default:
					break;
				}
			
			iCoreTarget->VolumeUpResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}   
		case ERemConCoreApiVolumeDown:
			{
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					{
					TKeyEvent event;
					event.iCode = EKeyDecVolume;
					OfferKeyEventL(event, EEventNull);
					break;
					}

				case ERemConCoreApiButtonPress:
				{
				TKeyEvent event;
				event.iCode = EKeyDecVolume;
				OfferKeyEventL(event, EEventNull);

					TTimeIntervalMicroSeconds32 repeatDelay;
					TTimeIntervalMicroSeconds32 repeatInterval;
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					
					delete iVolumeDownTimer;
					iVolumeDownTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeDownTimer->Start(TTimeIntervalMicroSeconds32(repeatDelay), TTimeIntervalMicroSeconds32(repeatInterval), iVolumeDownCallBack);
					break;
					}

				case ERemConCoreApiButtonRelease:
					delete iVolumeDownTimer;
					iVolumeDownTimer = NULL;
					break;

				default:
					break;
				}
			
			iCoreTarget->VolumeDownResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}
		default:
			break;
		}
	}

TKeyResponse CMobblerStatusControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode /*aType*/)
	{
	TKeyResponse response(EKeyWasNotConsumed);
	
	switch (aKeyEvent.iCode)
		{
		case EKeyRightArrow: // skip to the next track
			if (iAppUi.RadioPlayer()->CurrentTrack())
				{
				iAppUi.RadioPlayer()->NextTrackL();
				}
			response = EKeyWasConsumed;
			break;
		case EKeyUpArrow: // love
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandTrackLove);
			response = EKeyWasConsumed;
			break;
		case EKeyIncVolume:
		case '#':
			iMobblerVolumeTimeout->Reset();
			iAppUi.RadioPlayer()->VolumeUp();
			response = EKeyWasConsumed;
			//UpdateVolume();
			break;
		case EKeyDownArrow: // ban
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandTrackBan);
			response = EKeyWasConsumed;
			break;
		case EKeyDecVolume:
		case '*':
			iMobblerVolumeTimeout->Reset();
			iAppUi.RadioPlayer()->VolumeDown();
			response = EKeyWasConsumed;
			//UpdateVolume();
			break;
		case EKeyDevice3: // play or stop
			if (iAppUi.RadioPlayer()->CurrentTrack())
				{
				iAppUi.RadioPlayer()->Stop();
				}
			else
				{
				const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandResumeRadio);
				}
			response = EKeyWasConsumed;
			break;
		case EKeyLeftArrow:
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandBuy);
			response = EKeyWasConsumed;
			break;
/*
		case '1':
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandArtistGetInfo);
			response = EKeyWasConsumed;
			break;
*/
#ifdef _DEBUG
		case '2':
			iAlbumNameInMarquee =! iAlbumNameInMarquee;
			if (iAppUi.CurrentTrack())
				{
				if (iAlbumNameInMarquee)
					{
					CEikonEnv::Static()->InfoMsg(iAppUi.CurrentTrack()->Album().String());
					}
				else
					{
					CEikonEnv::Static()->InfoMsg(iAppUi.CurrentTrack()->Title().String());
					}
				}
			response = EKeyWasConsumed;
			break;
#endif
#ifdef _DEBUG
		case '7':
			if (iAppUi.Backlight())
				{
				CEikonEnv::Static()->InfoMsg(_L("On when active"));
				}
			else
				{
				CEikonEnv::Static()->InfoMsg(_L("System default"));
				}
			response = EKeyWasConsumed;
			break;
#endif
		default:
			break;
		}
	
	DrawDeferred();
	
	return response;
	}

void CMobblerStatusControl::HandlePointerEventL(const TPointerEvent& aPointerEvent)
	{
	// Check if they have touched any of the buttons
	// if so, issue a command
	
	TRect buyRect(iPointBuy, iControlSize);
	TRect loveRect(iPointLove, iControlSize);
	TRect playStopRect(iPointPlayStop, iControlSize);
	TRect banRect(iPointBan, iControlSize);
	TRect skipRect(iPointSkip, iControlSize);
	
	TKeyEvent event;
	event.iCode = EKeyNull;
	
	switch( aPointerEvent.iType )
		{
		case TPointerEvent::EButton1Down:
			
			if (buyRect.Contains(aPointerEvent.iPosition))
				event.iCode = EKeyLeftArrow;
			else if (loveRect.Contains(aPointerEvent.iPosition))
				event.iCode = EKeyUpArrow;
			else if (playStopRect.Contains(aPointerEvent.iPosition))
				event.iCode = EKeyDevice3;
			else if (banRect.Contains(aPointerEvent.iPosition))
				event.iCode = EKeyDownArrow;
			else if (skipRect.Contains(aPointerEvent.iPosition))
				event.iCode = EKeyRightArrow;
			
			if (event.iCode != EKeyNull)
				{
				if (iMobblerFeedback)
					{
#ifdef  __S60_50__
					iMobblerFeedback->InstantFeedback(ETouchFeedbackBasic);
#endif
					}
				}
			
			break;
		case TPointerEvent::EButton1Up:
			
			if (buyRect.Contains(aPointerEvent.iPosition) && buyRect.Contains(iLastPointerEvent.iPosition))
				event.iCode = EKeyLeftArrow;
			else if (loveRect.Contains(aPointerEvent.iPosition) && loveRect.Contains(iLastPointerEvent.iPosition))
				event.iCode = EKeyUpArrow;
			else if (playStopRect.Contains(aPointerEvent.iPosition) && playStopRect.Contains(iLastPointerEvent.iPosition))
				event.iCode = EKeyDevice3;
			else if (banRect.Contains(aPointerEvent.iPosition) && banRect.Contains(iLastPointerEvent.iPosition))
				event.iCode = EKeyDownArrow;
			else if (skipRect.Contains(aPointerEvent.iPosition) && skipRect.Contains(iLastPointerEvent.iPosition))
				event.iCode = EKeyRightArrow;
			
			if (event.iCode != EKeyNull)
				{
				OfferKeyEventL(event, EEventNull);
				}
			
			break;
		default:
			break;
		}
	
	iLastPointerEvent = aPointerEvent;
	
	CCoeControl::HandlePointerEventL(aPointerEvent);
	}

// End of file
