/*
mobblerstatuscontrol.cpp

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

#include <avkon.hrh>
#include <avkon.rsg>
#include <eikmenup.h>
#include <aknappui.h>
#include <eikcmobs.h>
#include <barsread.h>
#include <stringloader.h>
#include <gdi.h>
#include <eikedwin.h>
#include <eikenv.h>
#include <eikseced.h>
#include <eikappui.h>
#include <aknviewappui.h>
#include <gulfont.h>
#include <s32file.h>
#include <aknnavi.h> 
#include <aknnavide.h> 
#include <eikcmbut.h> 
#include <aknsdrawutils.h>
#include <aknsutils.h>
#include <aknutils.h>
#include <aknsbasicbackgroundcontrolcontext.h>
#include <aknscontrolcontext.h>
#include <icl/imagecodecdata.h>
#include <aknsskininstance.h>
#include <bitmaptransforms.h>
#include <gdi.h>
#include <aknnavilabel.h>

#include <mobbler.rsg>

#include "mobbler.hrh"
#include "mobblerstatuscontrol.h"
#include "mobblerutility.h"
#include "mobblertrack.h"
#include "mobblerappui.h"
#include "mobblerradioplayer.h"
#include "mobblerbitmap.h"
#include "mobblerstring.h"
#include "mobblertimeout.h"
#include "mobblermarquee.h"

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
	:iAppUi(aAppUi)
	{
	}

void CMobblerStatusControl::ConstructL(const TRect& aRect)
	{
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
	CFbsBitmap* musicAppIcon(NULL);
    CFbsBitmap* musicAppIconMask(NULL);
    AknsUtils::CreateAppIconLC(AknsUtils::SkinInstance(), KMusicAppUID,  EAknsAppIconTypeContext, musicAppIcon, musicAppIconMask);
    iMobblerBitmapMusicAppIcon = CMobblerBitmap::NewL(*this, musicAppIcon, musicAppIconMask);
    CleanupStack::Pop(2);
    
    // Load the Mobbler icon to display when a music player track is playing
    CFbsBitmap* appIcon(NULL);
    CFbsBitmap* appIconMask(NULL);
    AknsUtils::CreateAppIconLC(AknsUtils::SkinInstance(), KMobblerUID,  EAknsAppIconTypeContext, appIcon, appIconMask);
    iMobblerBitmapAppIcon = CMobblerBitmap::NewL(*this, appIcon, appIconMask);
    CleanupStack::Pop(2);
	}

void CMobblerStatusControl::LoadResourceFileTextL()
	{
	iResTextScrobbledQueuedFormat = StringLoader::LoadL(R_MOBBLER_SCROBBLED_QUEUED);
    iResTextOffline = StringLoader::LoadL(R_MOBBLER_IDLE);
    iResTextOnline = StringLoader::LoadL(R_MOBBLER_IDLE);
    iResTextArtist = StringLoader::LoadL(R_MOBBLER_ARTIST);
    iResTextTitle = StringLoader::LoadL(R_MOBBLER_TITLE);
    iResTextAlbum = StringLoader::LoadL(R_MOBBLER_ALBUM);
    iResTextStateOnline = StringLoader::LoadL(R_MOBBLER_STATE_ONLINE);
    iResTextStateOffline = StringLoader::LoadL(R_MOBBLER_STATE_OFFLINE);
    iResTextStateConnecting = StringLoader::LoadL(R_MOBBLER_STATE_CONNECTING);
    iResTextStateHandshaking = StringLoader::LoadL(R_MOBBLER_STATE_HANDSHAKING);
    iResTextStateSelectingStation = StringLoader::LoadL(R_MOBBLER_STATE_SELECTING_STATION);
    iResTextStateFetchingPlaylist = StringLoader::LoadL(R_MOBBLER_STATE_FETCHING_PLAYLIST);
    iResTextStateCheckingForUpdates = StringLoader::LoadL(R_MOBBLER_STATE_CHECKING_FOR_UPDATES);
	}

void CMobblerStatusControl::SetPositions()
	{
	// The height of the text bars
	const TInt KTextRectHeight(iMobblerFont->HeightInPixels() + iMobblerFont->DescentInPixels() + 2);
		
	// Check if the device is in portrait or landscape mode
	if (Size().iWidth <= (Size().iHeight * 11) / 10)
		{
		// Portrait graphics positions
		TInt albumArtDimension = Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - ((3 * KTextRectHeight) / 2);
		
		iRectAlbumArt = 			TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
		iPointControls = 			TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - (KTextRectHeight / 2), iRectAlbumArt.iBr.iY - (3 * iMobblerBitmapPound->SizeInPixels().iHeight) - 4);
		iPointLastFM = 				TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth -  (KTextRectHeight / 2), iRectAlbumArt.iTl.iY + 4);
		
		iRectTrackDetailsText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
		iRectScrobbledQueuedText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - ((3 * KTextRectHeight) / 2)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
		iRectProgressBar = 				TRect(TPoint(KTextRectHeight / 2, (iRectTrackDetailsText.iTl.iY + iRectScrobbledQueuedText.iTl.iY) / 2 ), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
		}
	else
		{
		// Landscape graphics positions
		TInt albumArtDimension = Min(Rect().Height() - ((3 * KTextRectHeight) / 2), Rect().Width() - (2 * KTextRectHeight) - (3 * iMobblerBitmapPlay->SizeInPixels().iWidth) - iMobblerBitmapLastFM->SizeInPixels().iWidth);
		
		iRectAlbumArt =				TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
		iRectProgressBar =			TRect(TPoint(KTextRectHeight / 2, Rect().Height() - ((3 * KTextRectHeight) / 2)), TSize(albumArtDimension, KTextRectHeight));
		
		iPointLastFM =				TPoint(albumArtDimension + ((3 * KTextRectHeight) / 2) + (3 * iMobblerBitmapPlay->SizeInPixels().iWidth), KTextRectHeight / 2);
		iPointControls = 			TPoint(albumArtDimension + KTextRectHeight, KTextRectHeight / 2);
		
		iRectTrackDetailsText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
		iRectScrobbledQueuedText = 		TRect(TPoint(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, Rect().Height() - ((3 * KTextRectHeight) / 2)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));
		
		iRectProgressBar = 				TRect(TPoint(albumArtDimension + KTextRectHeight, iRectAlbumArt.iBr.iY - KTextRectHeight), TSize(Rect().Width() -((3 * KTextRectHeight) / 2) - albumArtDimension, KTextRectHeight));
		}
	
	// Set the size of the application icons to be the same as the rect we will draw them to.
	// They don't seem to like being drawn at a different size
	if (iMobblerBitmapMusicAppIcon)
		{
		AknIconUtils::SetSize(iMobblerBitmapMusicAppIcon->Bitmap(), iRectAlbumArt.Size(), EAspectRatioNotPreserved);
		}
	
	if (iMobblerBitmapAppIcon)
		{
		AknIconUtils::SetSize(iMobblerBitmapAppIcon->Bitmap(), iRectAlbumArt.Size(), EAspectRatioNotPreserved);
		}
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
	}

void CMobblerStatusControl::FormatTime(TDes& aString, TTimeIntervalSeconds aSeconds)
	{
	TInt minutes = aSeconds.Int() / 60;
	TInt seconds = (aSeconds.Int() - (minutes * 60));
	aString.Zero();
	aString.AppendFormat(_L("%02d:%02d"), minutes, seconds);
	}

void CMobblerStatusControl::FormatTrackDetails(TDes& aString, const TDesC& aArtist, const TDesC& aTitle)
	{
	aString.Zero();
	aString.AppendFormat(_L("%S - %S"), &aArtist, &aTitle);
	}

void CMobblerStatusControl::Draw(const TRect& /*aRect*/) const
	{
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
	
		FormatTrackDetails(iTrackDetailsText, iAppUi.CurrentTrack()->Artist().String(), iAppUi.CurrentTrack()->Title().String());
		FormatTime(iPlaybackGoneText, iAppUi.CurrentTrack()->PlaybackPosition());
		FormatTime(iPlaybackLeftText, iAppUi.CurrentTrack()->TrackLength().Int() - iAppUi.CurrentTrack()->PlaybackPosition().Int());
		
		playbackTotal = iAppUi.CurrentTrack()->TrackLength().Int();
		playbackPosition = Min(iAppUi.CurrentTrack()->PlaybackPosition().Int(), playbackTotal);
		
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
		
		DrawText(iPlaybackLeftText, iRectProgressBar, KRgbBlack, CGraphicsContext::ERight, iMobblerFont->WidthZeroInPixels());
		DrawText(iPlaybackGoneText, iRectProgressBar, KRgbBlack, CGraphicsContext::ELeft, iMobblerFont->WidthZeroInPixels());
		}
	
	// Draw the controls
	DrawMobblerBitmap(iMobblerBitmapPound, TPoint(iPointControls.iX + (0 * iMobblerBitmapPound->SizeInPixels().iWidth), iPointControls.iY + (1 * iMobblerBitmapPound->SizeInPixels().iHeight)));
	DrawMobblerBitmap(iMobblerBitmapLove, TPoint(iPointControls.iX + (1 * iMobblerBitmapPound->SizeInPixels().iWidth), iPointControls.iY + (0 * iMobblerBitmapPound->SizeInPixels().iHeight)));
	DrawMobblerBitmap(iMobblerBitmapBan, TPoint(iPointControls.iX + (1 * iMobblerBitmapPound->SizeInPixels().iWidth), iPointControls.iY + (2 * iMobblerBitmapPound->SizeInPixels().iHeight)));
	DrawMobblerBitmap(iMobblerBitmapNext, TPoint(iPointControls.iX + (2 * iMobblerBitmapPound->SizeInPixels().iWidth), iPointControls.iY + (1 * iMobblerBitmapPound->SizeInPixels().iHeight)));
	
	// Draw either play or stop depending on the radio play state
	if (!iAppUi.RadioPlayer()->CurrentTrack())
		{
		// There is no current track so display the play button
		DrawMobblerBitmap(iMobblerBitmapPlay, TPoint(iPointControls.iX + (1 * iMobblerBitmapPlay->SizeInPixels().iWidth), iPointControls.iY + (1 * iMobblerBitmapPlay->SizeInPixels().iHeight)));
		}
	else
		{
		// There is a track playing
		DrawMobblerBitmap(iMobblerBitmapStop, TPoint(iPointControls.iX + (1 * iMobblerBitmapStop->SizeInPixels().iWidth), iPointControls.iY + (1 * iMobblerBitmapStop->SizeInPixels().iHeight)));
		}
	
	// Draw the Last.fm graphic
	DrawMobblerBitmap(iMobblerBitmapLastFM, iPointLastFM);
		
	// Draw The track details line
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
            if (aButtonAct == ERemConCoreApiButtonClick)
                {
                TKeyEvent event;
                event.iCode = EKeyIncVolume;
                OfferKeyEventL(event, EEventNull);
                }
            iCoreTarget->VolumeUpResponse(status, KErrNone);
            User::WaitForRequest(status);   
            break;
            }       
        case ERemConCoreApiVolumeDown:
            {
            if (aButtonAct == ERemConCoreApiButtonClick)
                {
                TKeyEvent event;
                event.iCode = EKeyDecVolume;
                OfferKeyEventL(event, EEventNull);
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
			iAppUi.RadioPlayer()->NextTrackL();
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
				// load the menu to start playing
				//iEikonEnv->LaunchPopupMenuL(R_MOBBLER_STATUS_OPTIONS_MENU, TPoint(0, 0), EPopupTargetTopLeft);
				}
			response = EKeyWasConsumed;
			break;
		case EKeyLeftArrow:
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandBuy);
			response = EKeyWasConsumed;
			break;
		case '1':
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandArtistGetInfo);
			response = EKeyWasConsumed;
			break;
		default:
			break;
		}
	
	DrawDeferred();
	
	return response;
	}



				
