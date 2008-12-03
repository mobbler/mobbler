/*
mobblerstatuscontrol.h

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

#ifndef __MOBBLERSTATUSCONTROL_H__
#define __MOBBLERSTATUSCONTROL_H__

#include <coecntrl.h>
#include <coedef.h>
#include <remconcoreapitargetobserver.h>    // link against RemConCoreApi.lib
#include <remconcoreapitarget.h>            // and
#include <remconinterfaceselector.h>        // RemConInterfaceBase.lib

#include "mobblerbitmap.h" 

const TInt KMaxMobblerTextSize(255);

class CMobblerTrack;
class CMobblerAppUi;
class CAknNavigationControlContainer;
class CAknNavigationDecorator;
class CMobblerBitmap;
class CBitmapScaler;
class CAknsBasicBackgroundControlContext;
class CMobblerTimeout;
class CMobblerMarquee;

class CMobblerStatusControl : public CCoeControl, public MMobblerBitmapObserver, public MRemConCoreApiTargetObserver
	{
private:
	class CMobblerPaneTextCallback : public CActive
		{
	public:
		static void ExecuteLD(const CMobblerStatusControl& aStatusControl, const TDesC& aText);
		
	private:
		CMobblerPaneTextCallback(const CMobblerStatusControl& aStatusControl);
		void ConstructL(const TDesC& aText);
		
		~CMobblerPaneTextCallback();
	private: // from CActive
		void RunL();
		void DoCancel();
		
		
		
	private:
		const CMobblerStatusControl& iStatusControl;
		HBufC* iText;
		};
	
public:
	static CMobblerStatusControl* NewL(const TRect& aRect, const CMobblerAppUi& aAppUi);
	~CMobblerStatusControl();
	
	//void UpdateVolume() const;
	
private:
	void ConstructL(const TRect& aRect);
	CMobblerStatusControl(const CMobblerAppUi& aAppUi);
	
	void LoadGraphicsL();
	void LoadResourceFileTextL();
	
	void SetPositions();
	
	// double buffering
	void CreateBackBufferL();
	void ReleaseBackBuffer();
	
	// callback from CMobblerBitmap when graphic is loaded
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	
	// Drawing helper methods
	void DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TRect& aRect) const;
	void DrawMobblerBitmap(const CMobblerBitmap* aMobblerBitmap, const TPoint& aPoint) const;
	void DrawText(const TDesC& aText, const TRect& aRect, const TRgb& aPenColor, CGraphicsContext::TTextAlign aTextAlign, TInt aOffset) const;
	void DrawRect(const TRect& aRect, const TRgb& aPenColor, const TRgb& aBrushColor) const;
	
	// formatting text
	static void FormatTime(TDes& aString, TTimeIntervalSeconds aSeconds, TTimeIntervalSeconds aTotalSeconds = 0);
	static void FormatTrackDetails(TDes& aString, const TDesC& aArtist, const TDesC& aTitle);
#ifdef _DEBUG
	static void FormatAlbumDetails(TDes& aString, const TDesC& aAlbum1, const TDesC& aAlbum2);
#endif
	
	// Observer of media button clicks
	void MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct);
	
	void ChangePaneTextL(const TDesC& aText) const;
	void DoChangePaneTextL(const TDesC& aText) const;
	
private: // auto-repeat audio button callbacks
	static TInt VolumeUpCallBackL(TAny *self);
	static TInt VolumeDownCallBackL(TAny *self);
	
private: // from CCoeControl
	void HandleResourceChange(TInt aType);
	TTypeUid::Ptr MopSupplyObject(TTypeUid aId);
	void SizeChanged();
	void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	
private:
	const CMobblerAppUi& iAppUi;
	CAknsBasicBackgroundControlContext* iBgContext;
	
	CAknNavigationControlContainer *iNaviContainer;
	CAknNavigationDecorator* iNaviLabelDecorator;
	
	// Graphics
	CMobblerBitmap* iMobblerBitmapBan;
	CMobblerBitmap* iMobblerBitmapPound;
	CMobblerBitmap* iMobblerBitmapLove;
	CMobblerBitmap* iMobblerBitmapPlay;
	CMobblerBitmap* iMobblerBitmapNext;
	CMobblerBitmap* iMobblerBitmapStop;
	CMobblerBitmap* iMobblerBitmapLastFM;
	CMobblerBitmap* iMobblerBitmapSpeakerLow;
	CMobblerBitmap* iMobblerBitmapSpeakerHigh;
	CMobblerBitmap* iMobblerBitmapScrobble;
	CMobblerBitmap* iMobblerBitmapTrackIcon;
	CMobblerBitmap* iMobblerBitmapMusicAppIcon;
	CMobblerBitmap* iMobblerBitmapAppIcon;
	
	// media buttons
	CRemConInterfaceSelector* iInterfaceSelector;
	CRemConCoreApiTarget*     iCoreTarget;
	
	// timers and callbacks for media buttons autorepeat
	CPeriodic* iVolumeUpTimer;
	CPeriodic* iVolumeDownTimer;
	TCallBack iVolumeUpCallBack;
	TCallBack iVolumeDownCallBack;

	
	// for double buffering
    CFbsBitmap*                     iBackBuffer;
    CFbsBitmapDevice*               iBackBufferDevice;
    CFbsBitGc*                      iBackBufferContext;
    TSize                           iBackBufferSize;
    
    // text from resource files
    HBufC* iResTextScrobbledQueuedFormat;
    HBufC* iResTextOffline;
    HBufC* iResTextOnline;
    HBufC* iResTextArtist;
    HBufC* iResTextTitle;
    HBufC* iResTextAlbum;
    HBufC* iResTextStateOnline;
    HBufC* iResTextStateOffline;
    HBufC* iResTextStateConnecting;
    HBufC* iResTextStateHandshaking;
    HBufC* iResTextStateSelectingStation;
    HBufC* iResTextStateFetchingPlaylist;
    HBufC* iResTextStateCheckingForUpdates;
    
    // text displayed
    mutable TBuf<KMaxMobblerTextSize> iScrobbledQueued;
    mutable TBuf<KMaxMobblerTextSize> iTrackDetailsText;
    mutable TBuf<KMaxMobblerTextSize> iStateText;
	mutable TBuf<KMaxMobblerTextSize> iPlaybackLeftText;
	mutable TBuf<KMaxMobblerTextSize> iPlaybackGoneText;
	
	// graphics and text positions
	TRect iRectAlbumArt;
	TPoint iPointControls;
	TPoint iPointLastFM;
	//TRect iRectStateText;
	TRect iRectTrackDetailsText;
	TRect iRectScrobbledQueuedText;
	TRect iRectProgressBar;
	
	const CFont* iMobblerFont;
	
	CMobblerTimeout* iMobblerVolumeTimeout;
	CMobblerMarquee* iMobblerMarquee;

#ifdef _DEBUG
	TBool iAlbumNameInMarquee;
#endif
	};
	
#endif // __MOBBLERSTATUSCONTROL_H__
