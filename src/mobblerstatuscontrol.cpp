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
#include <mobbler.mbg>
#include <mobbler.rsg>
#include <stringloader.h>
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
#include "mobblerstatusview.h"

_LIT(KMobblerMifFile, "\\resource\\apps\\mobbler.mif");
_LIT(KPngScrobble, "\\resource\\apps\\mobbler\\scrobble.png");
_LIT(KPngTrackIcon, "\\resource\\apps\\mobbler\\icon_track.png");
_LIT(KPngLastFM, "\\resource\\apps\\mobbler\\lastfm.png");
_LIT(KMusicAppNameAndConnectionSeperator, " - ");

const TRgb KRgbLastFMRed(0xD5, 0x10, 0x07, 0xFF);

const TRgb KRgbProgressBarBack(0xE7, 0xED, 0xEF, 0xFF);
const TRgb KRgbProgressBarBuffer(0xAF, 0xBE, 0xCC, 0xFF);
const TRgb KRgbProgressBarPlayback(0xD5, 0x10, 0x07, 0xFF);
const TRgb KRgbTransparent(0x00, 0x00, 0x00, 0x00);

const TUid KMusicAppUID = {0x102072C3};
const TUid KMobblerUID = {0xA0007648};

const TUid KTouchFeedbackImplUID = {0xA000B6CD};

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
	
	DoChangePaneTextL();

	LoadGraphicsL();

	ReleaseBackBuffer();
	CreateBackBufferL();

	iMobblerVolumeTimeout = CMobblerTimeout::NewL(2000000);
	
	iTitleMarquee = CMobblerMarquee::NewL(*this);
	iAlbumMarquee = CMobblerMarquee::NewL(*this);
	iArtistMarquee = CMobblerMarquee::NewL(*this);
	iTweetMarquee = CMobblerMarquee::NewL(*this);
	
	iAppUi.RadioPlayer().AddObserverL(this);
	iAppUi.LastFMConnection().AddStateChangeObserverL(this);
	iAppUi.MusicListener().AddObserverL(this);
	}

void CMobblerStatusControl::HandleConnectionStateChangedL()
	{
	DoChangePaneTextL();
	}

void CMobblerStatusControl::HandleRadioStateChangedL()
	{
	DoChangePaneTextL();
	}

void CMobblerStatusControl::HandleMusicAppChangeL()
	{
	DoChangePaneTextL();
	}

void CMobblerStatusControl::VolumeChanged()
	{
	iMobblerVolumeTimeout->Reset();
	DrawDeferred();
	}

void CMobblerStatusControl::DoChangePaneTextL()
	{
	TBuf<KMaxMobblerTextSize> stateText;
	
	if (iAppUi.CurrentTrack())
		{
		if (iAppUi.CurrentTrack()->RadioAuth().Compare(KNullDesC8) != 0)
			{
			// This is a radio song so display the name of the current radio station
			stateText.Copy(iAppUi.RadioPlayer().Station().String());
			}
		else
			{
			// This is a music player track
			stateText.Copy(iAppUi.MusicAppNameL());
			stateText.Append(KMusicAppNameAndConnectionSeperator);
			
			if (iAppUi.Mode() == CMobblerLastFMConnection::EOnline)
				{
				stateText.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_ONLINE));
				}
			else
				{
				stateText.Append(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_OFFLINE));
				}
			}
		}
	else
		{
		// There is no current track
	
		// Decide on the state text to display
		switch (iAppUi.State())
			{
			case CMobblerLastFMConnection::ENone:
				{
				switch (iAppUi.RadioPlayer().State())
					{
					case CMobblerRadioPlayer::ESelectingStation:
						stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_SELECTING_STATION));
						break;
					case CMobblerRadioPlayer::EFetchingPlaylist:
						stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_FETCHING_PLAYLIST));
						break;
					case CMobblerRadioPlayer::EIdle:
						{
						if (iAppUi.Mode() == CMobblerLastFMConnection::EOnline)
							{
							stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_ONLINE));
							}
						else
							{
							stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_OFFLINE));
							}
						}
						break;
					case CMobblerRadioPlayer::EPlaying:
						stateText.Copy(iAppUi.RadioPlayer().Station().String());
						break;
					default:
						break;
					}
				}
				break;
			case CMobblerLastFMConnection::EConnecting:
				stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_CONNECTING));
				break;
			case CMobblerLastFMConnection::EHandshaking:
				stateText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_STATE_HANDSHAKING));
				break;
			default:
				break;
			}
		}
	
	static_cast<CAknNaviLabel*>(iNaviLabelDecorator->DecoratedControl())->SetTextL(stateText);
	iNaviContainer->Pop();
	iNaviContainer->PushL(*iNaviLabelDecorator);
	}

void CMobblerStatusControl::LoadGraphicsL()
	{
	iMobblerBitmapLastFM = CMobblerBitmap::NewL(*this, KPngLastFM, KImageTypePNGUid);
	iMobblerBitmapScrobble = CMobblerBitmap::NewL(*this, KPngScrobble, KImageTypePNGUid);
	iMobblerBitmapTrackIcon = CMobblerBitmap::NewL(*this, KPngTrackIcon, KImageTypePNGUid);
	
	iMobblerBitmapMore = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerMore, EMbmMobblerMore_mask);
	iMobblerBitmapLove = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerLove, EMbmMobblerLove_mask);
	iMobblerBitmapBan = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerBan, EMbmMobblerBan_mask);
	iMobblerBitmapPlay = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerPlay, EMbmMobblerPlay_mask);
	iMobblerBitmapNext = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerNext, EMbmMobblerNext_mask);
	iMobblerBitmapStop = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerStop, EMbmMobblerStop_mask);
	iMobblerBitmapSpeakerHigh = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerSpeaker_high, EMbmMobblerSpeaker_high_mask);
	iMobblerBitmapSpeakerLow = CMobblerBitmap::NewL(*this, KMobblerMifFile, EMbmMobblerSpeaker_low, EMbmMobblerSpeaker_low_mask);
    
	// Load the Music Player icon to display when a music player track is playing
	iMobblerBitmapMusicAppIcon = CMobblerBitmap::NewL(*this, KMusicAppUID);
	
	// Load the Mobbler icon to display when a music player track is not playing
	iMobblerBitmapAppIcon = CMobblerBitmap::NewL(*this, KMobblerUID);
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
			
			iControlSize = TSize(Rect().Width() / 6, Rect().Width() / 6);
			
			iPointMore =					TPoint((KTextRectHeight/2) + (0 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointLove =				TPoint((KTextRectHeight/2) + (1 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointBan =					TPoint((KTextRectHeight/2) + (2 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointPlayStop =			TPoint((KTextRectHeight/2) + (3 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointSkip =				TPoint((KTextRectHeight/2) + (4 * ((Rect().Width() - KTextRectHeight - iControlSize.iWidth) / 4)), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			
			iPointLastFM = 				TPoint(-200, -200);
						
			TSize infoSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight);
						
			TInt textX(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth);
			
			iRectTitleText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((0 * (Rect().Height() - iControlSize.iHeight - iRectAlbumArt.iBr.iY - KTextRectHeight)) / 5) ), infoSize);
			iRectArtistText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((1 * (Rect().Height() - iControlSize.iHeight - iRectAlbumArt.iBr.iY - KTextRectHeight)) / 5) ), infoSize);
			iRectAlbumText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((2 * (Rect().Height() - iControlSize.iHeight - iRectAlbumArt.iBr.iY - KTextRectHeight)) / 5) ), infoSize);
			iRectProgressBar = 				TRect(TPoint(KTextRectHeight / 2, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((3 * (Rect().Height() - iControlSize.iHeight - iRectAlbumArt.iBr.iY - KTextRectHeight)) / 5) ), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));				
			iRectScrobbledQueuedText = 		TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((4 * (Rect().Height() - iControlSize.iHeight - iRectAlbumArt.iBr.iY - KTextRectHeight)) / 5) ), infoSize);
			}
		else
			{
			// 3rd edition
			TInt albumArtDimension = Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - ((3 * KTextRectHeight) / 2);
			
			iControlSize = TSize(27, 27);
			
			iRectAlbumArt = 			TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
			TPoint controlsTopLeft = 	TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth - (KTextRectHeight / 2), iRectAlbumArt.iBr.iY - (3 * iControlSize.iHeight) - 4);
			iPointMore =				TPoint(controlsTopLeft.iX + (0 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointLove =				TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (0 * iControlSize.iHeight));
			iPointBan =					TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (2 * iControlSize.iHeight));
			iPointSkip =				TPoint(controlsTopLeft.iX + (2 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointPlayStop =			TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			
			iPointLastFM = 				TPoint(Rect().Width() - iMobblerBitmapLastFM->SizeInPixels().iWidth -  (KTextRectHeight / 2), iRectAlbumArt.iTl.iY + 4);
			
			
			iRectProgressBar = 				TRect(TPoint(KTextRectHeight / 2, Rect().Height() -  (2 * KTextRectHeight)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));				
			
			TSize infoSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight);
			
			TInt textX(KTextRectHeight / 2 + iMobblerBitmapTrackIcon->SizeInPixels().iWidth);
			
			iRectTitleText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((0 * (Rect().Height() - ((KTextRectHeight) / 2) - iRectAlbumArt.iBr.iY)) / 5) ), infoSize);
			iRectArtistText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((1 * (Rect().Height() - ((KTextRectHeight) / 2) - iRectAlbumArt.iBr.iY)) / 5) ), infoSize);
			iRectAlbumText = 				TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((2 * (Rect().Height() - ((KTextRectHeight) / 2) - iRectAlbumArt.iBr.iY)) / 5) ), infoSize);
			iRectScrobbledQueuedText = 		TRect(TPoint(textX, iRectAlbumArt.iBr.iY + (KTextRectHeight / 2) + ((4 * (Rect().Height() - ((KTextRectHeight) / 2) - iRectAlbumArt.iBr.iY)) / 5) ), infoSize);
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

			iControlSize = TSize((Rect().Width() - albumArtDimension) / 6, (Rect().Width() - albumArtDimension) / 6);
						
			iPointMore =				TPoint(albumArtDimension + (KTextRectHeight / 2) + (0 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointLove =				TPoint(albumArtDimension + (KTextRectHeight / 2) + (1 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointBan =					TPoint(albumArtDimension + (KTextRectHeight / 2) + (2 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointPlayStop =			TPoint(albumArtDimension + (KTextRectHeight / 2) + (3 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			iPointSkip =				TPoint(albumArtDimension + (KTextRectHeight / 2) + (4 * (Rect().Width() - albumArtDimension - KTextRectHeight) / 5), Rect().Height() - iControlSize.iHeight - (KTextRectHeight / 2));
			
			TSize infoSize(Rect().Width() - albumArtDimension - iMobblerBitmapTrackIcon->SizeInPixels().iWidth - KTextRectHeight, KTextRectHeight);
						
			TInt textX(albumArtDimension + (KTextRectHeight / 2) + iMobblerBitmapTrackIcon->SizeInPixels().iWidth);
			
			TInt infoHeight(Rect().Height() - iControlSize.iHeight - ((3 * KTextRectHeight) / 2) - iPointLastFM.iY - iMobblerBitmapLastFM->SizeInPixels().iHeight);
						
			iRectTitleText = 				TRect(TPoint(textX, iPointLastFM.iY + iMobblerBitmapLastFM->SizeInPixels().iHeight + (KTextRectHeight / 2) + ((0 * (infoHeight)) / 5) ), infoSize);
			iRectArtistText = 				TRect(TPoint(textX, iPointLastFM.iY + iMobblerBitmapLastFM->SizeInPixels().iHeight + (KTextRectHeight / 2) + ((1 * (infoHeight)) / 5) ), infoSize);
			iRectAlbumText = 				TRect(TPoint(textX, iPointLastFM.iY + iMobblerBitmapLastFM->SizeInPixels().iHeight + (KTextRectHeight / 2) + ((2 * (infoHeight)) / 5) ), infoSize);
			iRectProgressBar = 				TRect(TPoint(albumArtDimension + (KTextRectHeight / 2), iPointLastFM.iY + iMobblerBitmapLastFM->SizeInPixels().iHeight + (KTextRectHeight / 2) + ((3 * (infoHeight)) / 5) ), TSize(Rect().Width() - albumArtDimension - KTextRectHeight, KTextRectHeight));				
			iRectScrobbledQueuedText = 		TRect(TPoint(textX, iPointLastFM.iY + iMobblerBitmapLastFM->SizeInPixels().iHeight + (KTextRectHeight / 2) + ((4 * (infoHeight)) / 5) ), infoSize);
			}
		else
			{
			// 3rd edition
			TInt albumArtDimension = 	Rect().Height() - ((3 * KTextRectHeight) / 2) - iMobblerBitmapLastFM->SizeInPixels().iHeight;
			iRectAlbumArt =				TRect(TPoint(KTextRectHeight / 2, KTextRectHeight / 2), TSize(albumArtDimension, albumArtDimension));
			
			iControlSize = TSize(20, 20);
			
			iControlSize = TSize(27, 27);
			
			TPoint controlsTopLeft = 	TPoint(albumArtDimension + KTextRectHeight, KTextRectHeight / 2);
			iPointMore =				TPoint(controlsTopLeft.iX + (0 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointLove =				TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (0 * iControlSize.iHeight));
			iPointBan =					TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (2 * iControlSize.iHeight));
			iPointSkip =				TPoint(controlsTopLeft.iX + (2 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			iPointPlayStop =			TPoint(controlsTopLeft.iX + (1 * iControlSize.iWidth), controlsTopLeft.iY + (1 * iControlSize.iHeight));
			
			iPointLastFM =				TPoint(albumArtDimension + ((3 * KTextRectHeight) / 2) + (3 * iControlSize.iWidth), KTextRectHeight / 2);
			
			TInt textX(albumArtDimension + KTextRectHeight + iMobblerBitmapTrackIcon->SizeInPixels().iWidth);
			TInt infoY((3 * iControlSize.iHeight) + KTextRectHeight / 2);
			TInt infoHeight(albumArtDimension - (3 * iControlSize.iHeight));
			TSize infoSize(Rect().Width() - textX - (KTextRectHeight / 2), KTextRectHeight);
						
			iRectTitleText = 			TRect(TPoint(textX, infoY + ((0 * (infoHeight)) / 3) ), infoSize);
			iRectArtistText = 			TRect(TPoint(textX, infoY + ((1 * (infoHeight)) / 3) ), infoSize);
			iRectAlbumText = 			TRect(TPoint(textX, infoY + ((2 * (infoHeight)) / 3) ), infoSize);
			
			TInt progressY(albumArtDimension + KTextRectHeight);
			TInt progressHeight(Rect().Height() - progressY);
			
			iRectProgressBar = 			TRect(TPoint(KTextRectHeight / 2, progressY + ((0 * progressHeight) / 2)), TSize(Rect().Width() - KTextRectHeight, KTextRectHeight));				
			iRectScrobbledQueuedText = 	TRect(TPoint((KTextRectHeight / 2) + iMobblerBitmapTrackIcon->SizeInPixels().iWidth, progressY + ((1 * progressHeight) / 2)), TSize(Rect().Width() - KTextRectHeight - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, KTextRectHeight));
			}
		}
	
	// Set the size of the application icons to be the same as the rect we will draw them to.
	// They don't seem to like being drawn at a different size
	
	iMobblerBitmapMusicAppIcon->SetSize(iRectAlbumArt.Size());
	iMobblerBitmapAppIcon->SetSize(iRectAlbumArt.Size());
	
	iMobblerBitmapBan->SetSize(iControlSize);
	iMobblerBitmapMore->SetSize(iControlSize);
	iMobblerBitmapLove->SetSize(iControlSize);
	iMobblerBitmapPlay->SetSize(iControlSize);
	iMobblerBitmapNext->SetSize(iControlSize);
	iMobblerBitmapStop->SetSize(iControlSize);
	
	TSize speakerSize(KTextRectHeight, KTextRectHeight);
	iMobblerBitmapSpeakerLow->SetSize(speakerSize);
	iMobblerBitmapSpeakerHigh->SetSize(speakerSize);
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
	if (iTitleMarquee)
		{
		iTitleMarquee->Reset();
		}
	if (iAlbumMarquee)
		{
		iAlbumMarquee->Reset();
		}
	if (iArtistMarquee)
		{
		iArtistMarquee->Reset();
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
	iBackBuffer = new(ELeave) CFbsBitmap;

	User::LeaveIfError(iBackBuffer->Create(Size(), iEikonEnv->DefaultDisplayMode()));

	// Create back buffer graphics context
	iBackBufferDevice = CFbsBitmapDevice::NewL(iBackBuffer);
	User::LeaveIfError(iBackBufferDevice->CreateContext(iBackBufferContext));
	iBackBufferContext->SetPenStyle(CGraphicsContext::ESolidPen);

	iBackBufferSize = iBackBuffer->SizeInPixels();

	// Get and set the font to use
	_LIT(fontName, "SwissA");
#ifdef __WINS__
	TFontSpec fontSpec(fontName, 80);
#else
	TFontSpec fontSpec(fontName, 120);
#endif
	fontSpec.iFontStyle = TFontStyle(EPostureUpright, EStrokeWeightNormal, EPrintPosNormal);
	fontSpec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
	iBackBufferDevice->GetNearestFontInTwips(iMobblerFont, fontSpec);
	iBackBufferContext->UseFont(iMobblerFont);
	

	iTitleFont = NULL;
	TFontSpec fontSpec2(fontName, 135);
	fontSpec2.iFontStyle = TFontStyle(EPostureUpright, EStrokeWeightBold, EPrintPosNormal);
	fontSpec2.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
	iBackBufferDevice->GetNearestFontInTwips(iTitleFont, fontSpec2);
	}
 
void CMobblerStatusControl::ReleaseBackBuffer()
	{
	if (iMobblerFont)
		{
		iBackBufferContext->DiscardFont();
		iBackBufferDevice->ReleaseFont(iMobblerFont);
		iMobblerFont = NULL;
		}

	if (iTitleFont)
		{
		iBackBufferContext->DiscardFont();
		iBackBufferDevice->ReleaseFont(iTitleFont);
		iTitleFont = NULL;
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

void CMobblerStatusControl::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	DrawDeferred();
	}

CMobblerStatusControl::~CMobblerStatusControl()
	{
	iAppUi.RadioPlayer().RemoveObserver(this);
	iAppUi.LastFMConnection().RemoveStateChangeObserver(this);
	iAppUi.MusicListener().RemoveObserver(this);
	
	delete iBgContext;
	
	delete iMobblerBitmapBan;
	delete iMobblerBitmapMore;
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
	delete iMobblerBitmapTwitterIcon;
	
	delete iMobblerBitmapMusicAppIcon;
	
	ReleaseBackBuffer();
	
	iNaviContainer->Pop(iNaviLabelDecorator);
	delete iNaviLabelDecorator;
	
	delete iMobblerVolumeTimeout;
	delete iTitleMarquee;
	delete iAlbumMarquee;
	delete iArtistMarquee;
	delete iTweetMarquee;
	
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
	
	TBool loveDisabled(EFalse);
	TBool banDisabled(EFalse);
	TBool playStopDisabled(EFalse);
	TBool skipDisabled(EFalse);
	TBool moreDisabled(EFalse);
	
	const CMobblerBitmap* albumArt(iMobblerBitmapAppIcon);
	const CMobblerBitmap* infoIcon(iMobblerBitmapTrackIcon);
	
	TBool love(EFalse);
			
	if (iAppUi.CurrentTrack())
		{
		love = iAppUi.CurrentTrack()->Love();
		
		if (iAppUi.CurrentTrack()->RadioAuth().Compare(KNullDesC8) != 0)
			{
			if (iAppUi.CurrentTrack()->AlbumArt() && iAppUi.CurrentTrack()->AlbumArt()->Bitmap())
				{
				// The current track has album art and it has finished loading
				albumArt = iAppUi.CurrentTrack()->AlbumArt();
				
				if (iAppUi.CurrentTrack()->AlbumArt()->ScaleSatus() == CMobblerBitmap::EMobblerScaleNone
						&& iAppUi.CurrentTrack()->AlbumArt()->SizeInPixels() != iRectAlbumArt.Size())
					{
					const_cast<CMobblerBitmap*>(iAppUi.CurrentTrack()->AlbumArt())->ScaleL(iRectAlbumArt.Size());
					}
				}
			}
		else
			{
			// This is a music player track
			if (iAppUi.CurrentTrack()->AlbumArt() && iAppUi.CurrentTrack()->AlbumArt()->Bitmap())
				{
				// The current track has album art and it has finished loading
				albumArt = iAppUi.CurrentTrack()->AlbumArt();
				
				if (iAppUi.CurrentTrack()->AlbumArt()->ScaleSatus() == CMobblerBitmap::EMobblerScaleNone
						&& iAppUi.CurrentTrack()->AlbumArt()->SizeInPixels() != iRectAlbumArt.Size())
					{
					const_cast<CMobblerBitmap*>(iAppUi.CurrentTrack()->AlbumArt())->ScaleL(iRectAlbumArt.Size());
					}
				}
			else
				{
				albumArt = iMobblerBitmapMusicAppIcon;
				}
			
			banDisabled = ETrue;
			skipDisabled = ETrue;
			playStopDisabled = ETrue;
			}
		
		iAppUi.CurrentTrack()->Title().String().Length() > 0 ?
				iTitleText.Copy(iAppUi.CurrentTrack()->Title().String()):
				iTitleText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TITLE));
				
		iAppUi.CurrentTrack()->Album().String().Length() > 0 ?
				iAlbumText.Copy(iAppUi.CurrentTrack()->Album().String()):
				iAlbumText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_ALBUM));
						
		iAppUi.CurrentTrack()->Artist().String().Length() > 0 ?
				iArtistText.Copy(iAppUi.CurrentTrack()->Artist().String()):
				iArtistText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_ARTIST));

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
		// There is no current track
		
		// gray out the correct buttons
		loveDisabled = ETrue;
		banDisabled = ETrue;
		skipDisabled = ETrue;
		moreDisabled = ETrue;
		
		iTitleText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_TITLE));
		iArtistText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_ARTIST));
		iAlbumText.Copy(iAppUi.ResourceReader().ResourceL(R_MOBBLER_ALBUM));
		
		FormatTime(iPlaybackLeftText, 0);
		FormatTime(iPlaybackGoneText, 0);
		}
	
	TRgb textColor; // text color when not highlighted
	AknsUtils::GetCachedColor(skin, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6);
	iBackBufferContext->SetPenColor(textColor);
	
	// Draw the album art
    //DrawRect(iRectAlbumArt, KRgbWhite, KRgbWhite);
	DrawMobblerBitmap(albumArt, iRectAlbumArt, EFalse);
	
	// If the track has been loved, draw the love icon in the bottom right corner
	if (love)
		{
		DrawMobblerBitmap(iMobblerBitmapLove, TPoint(iRectAlbumArt.iBr.iX - iMobblerBitmapLove->SizeInPixels().iWidth - 4, iRectAlbumArt.iBr.iX - iMobblerBitmapLove->SizeInPixels().iHeight - 4), EFalse);
		}
	
	if (iAppUi.ScrobblingOn())
		{
		iScrobbledQueued.Zero();
		iScrobbledQueued.Format(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SCROBBLED_QUEUED), iAppUi.Scrobbled(), iAppUi.Queued());
		
		DrawMobblerBitmap(iMobblerBitmapScrobble, TPoint(iRectScrobbledQueuedText.iTl.iX - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectScrobbledQueuedText.iTl.iY + 3), EFalse);
		DrawText(iScrobbledQueued, iRectScrobbledQueuedText, textColor, CGraphicsContext::ELeft, iMobblerFont->WidthZeroInPixels());
		}
	else
		{
		DrawMobblerBitmap(iMobblerBitmapScrobble, TPoint(iRectScrobbledQueuedText.iTl.iX - iMobblerBitmapTrackIcon->SizeInPixels().iWidth, iRectScrobbledQueuedText.iTl.iY + 3), ETrue);
		DrawText(iAppUi.ResourceReader().ResourceL(R_MOBBLER_SCROBBLING_OFF), iRectScrobbledQueuedText, textColor, CGraphicsContext::ELeft, iMobblerFont->WidthZeroInPixels());
		}
	
	// draw the progress bar background
	DrawRect(iRectProgressBar, KRgbBlack, KRgbProgressBarBack);

	// Draw the progress bar line (either volume or track playback progress)
	if (!iMobblerVolumeTimeout->TimedOut())
		{
		// Draw the volume level progresses
		TRect rectBufferProgress = iRectProgressBar;
		rectBufferProgress.SetWidth((iAppUi.RadioPlayer().Volume() * iRectProgressBar.Width()) / iAppUi.RadioPlayer().MaxVolume());
		DrawRect(rectBufferProgress, KRgbTransparent, KRgbLastFMRed);
		
		TInt volumePercent = ((iAppUi.RadioPlayer().Volume() * 100) / iAppUi.RadioPlayer().MaxVolume());
#ifdef __WINS__
		TBuf<8> volumeText;
#else
		TBuf<6> volumeText;
#endif
		volumeText.AppendFormat(_L("%d%%"), volumePercent);
		DrawText(volumeText, iRectProgressBar, KRgbBlack, CGraphicsContext::ECenter, iMobblerFont->WidthZeroInPixels());
		
		// Draw the speaker icons
		DrawMobblerBitmap(iMobblerBitmapSpeakerLow, TPoint(iRectProgressBar.iTl.iX + iMobblerFont->WidthZeroInPixels(), iRectProgressBar.iTl.iY), EFalse);
		DrawMobblerBitmap(iMobblerBitmapSpeakerHigh, TPoint(iRectProgressBar.iBr.iX - iMobblerFont->WidthZeroInPixels() - iMobblerBitmapSpeakerHigh->SizeInPixels().iWidth, iRectProgressBar.iTl.iY), EFalse);
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
		if (iAppUi.ScrobblingOn() && scrobbleTime <= playbackTotal)
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
	DrawMobblerBitmap(iMobblerBitmapMore, iPointMore, moreDisabled);
	DrawMobblerBitmap(iMobblerBitmapLove, iPointLove, loveDisabled);
	DrawMobblerBitmap(iMobblerBitmapBan, iPointBan, banDisabled);
	DrawMobblerBitmap(iMobblerBitmapNext, iPointSkip, skipDisabled);
	
	// Draw either play or stop depending on if there is a track playing
	if (!iAppUi.CurrentTrack())
		{
		// There is no current track so display the play button
		DrawMobblerBitmap(iMobblerBitmapPlay, iPointPlayStop, playStopDisabled);
		}
	else
		{
		// There is a track playing
		DrawMobblerBitmap(iMobblerBitmapStop, iPointPlayStop, playStopDisabled);
		}
	
	// Draw the Last.fm graphic
	DrawMobblerBitmap(iMobblerBitmapLastFM, iPointLastFM, EFalse);
		
	// Draw the title details line
	iTitleMarquee->Start(iTitleText, iMobblerFont->WidthZeroInPixels(), iMobblerFont->TextWidthInPixels(iTitleText), iRectTitleText.Width());
	DrawText(iTitleText, iRectTitleText, textColor, CGraphicsContext::ELeft, iTitleMarquee->GetPosition1());
	if (iTitleMarquee->GetPosition2() != KMaxTInt)
		{
		DrawText(iTitleText, iRectTitleText, textColor, CGraphicsContext::ELeft, iTitleMarquee->GetPosition2());
		}
	
	// Draw the album details line
	iAlbumMarquee->Start(iAlbumText, iMobblerFont->WidthZeroInPixels(), iMobblerFont->TextWidthInPixels(iAlbumText), iRectAlbumText.Width());
	DrawText(iAlbumText, iRectAlbumText, textColor, CGraphicsContext::ELeft, iAlbumMarquee->GetPosition1());
	if (iAlbumMarquee->GetPosition2() != KMaxTInt)
		{
		DrawText(iAlbumText, iRectAlbumText, textColor, CGraphicsContext::ELeft, iAlbumMarquee->GetPosition2());
		}
	
	// Draw the artist details line
	iArtistMarquee->Start(iArtistText, iMobblerFont->WidthZeroInPixels(), iMobblerFont->TextWidthInPixels(iArtistText), iRectArtistText.Width());
	DrawText(iArtistText, iRectArtistText, textColor, CGraphicsContext::ELeft, iArtistMarquee->GetPosition1());
	if (iArtistMarquee->GetPosition2() != KMaxTInt)
		{
		DrawText(iArtistText, iRectArtistText, textColor, CGraphicsContext::ELeft, iArtistMarquee->GetPosition2());
		}
	
	DrawMobblerBitmap(infoIcon, TPoint(iRectTitleText.iTl.iX -  infoIcon->SizeInPixels().iWidth, iRectTitleText.iTl.iY + 3), EFalse);
		
	SystemGc().BitBlt(TPoint(0, 0), iBackBuffer);
	}

void CMobblerStatusControl::DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TRect& aRect, TBool aGray) const
	{
	if (aMobblerBitmap)
		{
		if (aMobblerBitmap->Bitmap())
			{
			CFbsBitmap* bitmap = aGray ? aMobblerBitmap->BitmapGrayL() : aMobblerBitmap->Bitmap();

			
			if (aMobblerBitmap->Mask())
				{
				iBackBufferContext->DrawBitmapMasked(aRect, bitmap, TRect(TPoint(0, 0), aMobblerBitmap->SizeInPixels()), aMobblerBitmap->Mask(), EFalse);
				}
			else
				{
				iBackBufferContext->DrawBitmap(aRect, bitmap);
				}
			}
		}
	}

void CMobblerStatusControl::DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TPoint& aPoint, TBool aGray) const
	{
	if (aMobblerBitmap)
		{
		if (aMobblerBitmap->Bitmap())
			{
			CFbsBitmap* bitmap = aGray ? aMobblerBitmap->BitmapGrayL() : aMobblerBitmap->Bitmap();
			
			if (aMobblerBitmap->Mask())
				{
				iBackBufferContext->DrawBitmapMasked(TRect(aPoint, aMobblerBitmap->Bitmap()->SizeInPixels()), bitmap, TRect(TPoint(0, 0), aMobblerBitmap->Bitmap()->SizeInPixels()), aMobblerBitmap->Mask(), EFalse);
				}
			else
				{
				iBackBufferContext->DrawBitmap(aPoint, bitmap);
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
				
TKeyResponse CMobblerStatusControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode /*aType*/)
	{
	TKeyResponse response(EKeyWasNotConsumed);
	
	switch (aKeyEvent.iCode)
		{
		case EKeyRightArrow: // skip to the next track
			if (iAppUi.RadioPlayer().CurrentTrack())
				{
				iAppUi.RadioPlayer().NextTrackL();
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
			iAppUi.RadioPlayer().VolumeUp();
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
			iAppUi.RadioPlayer().VolumeDown();
			response = EKeyWasConsumed;
			//UpdateVolume();
			break;
		case EKeyDevice3: // play or stop
			if (iAppUi.RadioPlayer().CurrentTrack())
				{
				iAppUi.RadioPlayer().Stop();
				}
			else
				{
				const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandResumeRadio);
				}
			response = EKeyWasConsumed;
			break;
		case EKeyLeftArrow:
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandPlus);
			response = EKeyWasConsumed;
			break;
#ifdef _DEBUG
		case '5':
			const_cast<CMobblerAppUi&>(iAppUi).HandleCommandL(EMobblerCommandToggleScrobbling);
			DrawDeferred();
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
	
//	DrawDeferred();
	
	return response;
	}

void CMobblerStatusControl::HandlePointerEventL(const TPointerEvent& aPointerEvent)
	{
	// Check if they have touched any of the buttons
	// if so, issue a command
	
	TRect moreRect(iPointMore, iControlSize);
	TRect loveRect(iPointLove, iControlSize);
	TRect playStopRect(iPointPlayStop, iControlSize);
	TRect banRect(iPointBan, iControlSize);
	TRect skipRect(iPointSkip, iControlSize);
	
	TKeyEvent event;
	event.iCode = EKeyNull;
	
	switch( aPointerEvent.iType )
		{
		case TPointerEvent::EButton1Down:
			
			if (moreRect.Contains(aPointerEvent.iPosition))
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
			
			if (moreRect.Contains(aPointerEvent.iPosition) && moreRect.Contains(iLastPointerEvent.iPosition))
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
