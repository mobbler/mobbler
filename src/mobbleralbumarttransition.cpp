/*
mobbleralbumarttransition.cpp

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

#include <e32math.h>
#include <gulicon.h>
#include <hal.h>

#include "mobbleralbumarttransition.h"
#include "mobblerappui.h"
#include "mobblerbitmap.h"
#include "mobblerstatuscontrol.h"

const TTimeIntervalMicroSeconds32 KMobblerAlbumArtInterval(100000);
const TInt KTotalSlideTime(500000);

CMobblerAlbumArtTransition* CMobblerAlbumArtTransition::NewL(CMobblerStatusControl& aStatusControl)
	{
	CMobblerAlbumArtTransition* self = new(ELeave) CMobblerAlbumArtTransition(aStatusControl);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAlbumArtTransition::CMobblerAlbumArtTransition(CMobblerStatusControl& aStatusControl)
	:iStatusControl(aStatusControl)
	{
	}

void CMobblerAlbumArtTransition::ConstructL()
	{
	}

TInt CMobblerAlbumArtTransition::Clamp(TReal aValue, TReal aMin, TReal aMax) const
	{
	// Restrict value to the bounds of min and max.
	// If value is less than min, return min.
	// If value is more than max, return max.
	// Else return value.
	
	if (aValue < aMin)
		{
		return aMin;
		}
	else if (aValue > aMax)
		{
		return aMax;
		}
	else
		{
		return aValue;
		}
	}

TInt CMobblerAlbumArtTransition::Slide(TReal aTime, TReal aTotal, TReal aStart, TReal aEnd) const
	{ 
	TReal output;
	Math::Sin(output, ((Clamp(aTime, 0, aTotal) / aTotal) * KPi) - (KPi / 2));
	return static_cast<TInt>(((output + 1) / 2) * (aEnd - aStart));
	}

TInt CMobblerAlbumArtTransition::SlideAmount(TInt aWidth) const
	{
	TInt tickPeriod;
	HAL::Get(HAL::ESystemTickPeriod, tickPeriod);
	TInt timeSoFar((iNowTickCount  - iStartTickCount) * tickPeriod);
	return Slide(timeSoFar, KTotalSlideTime, 0, aWidth);
	}

CMobblerAlbumArtTransition::~CMobblerAlbumArtTransition()
	{
	if (iLastAlbumArt)
		{
		iLastAlbumArt->Close();
		}
		
	if (iCurrentAlbumArt)
		{
		iCurrentAlbumArt->Close();
		}
	
	delete iTimer;
	delete iDoubleBitmap;
	delete iDoubleMask;
	}

void CMobblerAlbumArtTransition::DrawAlbumArtL(const CMobblerBitmap* aCurrentAlbumArt, const CMobblerBitmap* aNextAlbumArt, TRect aAlbumArtRect, TInt aSlideAmount)
	{
	if ((!iTimer || iSlide == ESlideLeft) &&
			aCurrentAlbumArt != iCurrentAlbumArt &&
			aCurrentAlbumArt->Bitmap())
		{
		// The album art is different to our current one and it has loaded
		
		if (iLastAlbumArt)
			{
			// This is not the first time so start a transition
			iLastAlbumArt->Close();
			iLastAlbumArt = iCurrentAlbumArt;
			iCurrentAlbumArt = aCurrentAlbumArt;
			iCurrentAlbumArt->Open();
			
			// Start the callbacks
			delete iTimer;
			iTimer = CPeriodic::NewL(CActive::EPriorityLow);
			TCallBack callBack(CallBack, this);
			iTimer->Start(KMobblerAlbumArtInterval, KMobblerAlbumArtInterval, callBack);
			
			// Record the starting tick count
			iStartTickCount = User::TickCount();
			}
		else
			{
			// This is the first image so just keep a reference to it
			iLastAlbumArt = aCurrentAlbumArt;
			iLastAlbumArt->Open();
			iCurrentAlbumArt = aCurrentAlbumArt;
			iCurrentAlbumArt->Open();
			}
		}
	
	iNowTickCount = User::TickCount();
	
	if (!iTimer || iTimer && (SlideAmount(aAlbumArtRect.Width()) >= aAlbumArtRect.Width()))
		{
		if (iTimer)
			{
			iTimer->Cancel();
			delete iTimer;
			iTimer = NULL;
			
			iSlide = ESlideNormal;
			}
		
		// The album art is not transitioning
		
		if (aNextAlbumArt && aSlideAmount > 0)
			{
			// The album art is being moved by a finger
			
			DoDrawAlbumArtL(aCurrentAlbumArt, aNextAlbumArt, aAlbumArtRect, Clamp(aSlideAmount, 0, aAlbumArtRect.Width()));
			}
		else
			{
			// It's not transitioning or being moved by a finger
			// so just display the current album art
			CMobblerBitmap::LongSidesEqual(aCurrentAlbumArt->Bitmap()->SizeInPixels(), aAlbumArtRect.Size())?
				iStatusControl.BitBltMobblerBitmap(aCurrentAlbumArt, aAlbumArtRect.iTl, TRect(TPoint(0, 0), aCurrentAlbumArt->SizeInPixels())):
				iStatusControl.DrawMobblerBitmap(aCurrentAlbumArt, aAlbumArtRect, aCurrentAlbumArt->SizeInPixels());
			}
		}
	else
		{
		// The album art is transitioning so draw it
		
		switch (iSlide)
			{
			case ESlideNormal:
				{
				DoDrawAlbumArtL(iLastAlbumArt, iCurrentAlbumArt, aAlbumArtRect, SlideAmount(aAlbumArtRect.Width()));
				}
				break;
			case ESlideLeft:
				{
				TInt slideAmount(SlideAmount(aAlbumArtRect.Width()));
				TInt actualSlideAmount(iFingerUpOffset + (slideAmount * (aAlbumArtRect.Width() - iFingerUpOffset)) / aAlbumArtRect.Width());
				DoDrawAlbumArtL(iLastAlbumArt, iCurrentAlbumArt, aAlbumArtRect, Clamp(actualSlideAmount, 0, aAlbumArtRect.Width()));
				}
				break;
			case ESlideRight:
				{
				TInt slideAmount(SlideAmount(aAlbumArtRect.Width()));
				TInt actualSlideAmount(iFingerUpOffset - (slideAmount * iFingerUpOffset) / aAlbumArtRect.Width());
				DoDrawAlbumArtL(aCurrentAlbumArt, aNextAlbumArt, aAlbumArtRect, Clamp(actualSlideAmount, 0, aAlbumArtRect.Width()));
				}
				break;
				
			}
		}
	}

void CMobblerAlbumArtTransition::DoDrawAlbumArtL(const CMobblerBitmap* aLeft, const CMobblerBitmap* aRight, TRect aAlbumArtRect, TInt aPosition)
	{
	// Draw the first bitmap by the slide amount
	CMobblerBitmap::LongSidesEqual(aLeft->Bitmap()->SizeInPixels(), aAlbumArtRect.Size())?
		iStatusControl.BitBltMobblerBitmap(aLeft, 
				aAlbumArtRect.iTl, 
				TRect(TPoint(aPosition, 0), TSize(aAlbumArtRect.Width() - aPosition, aLeft->SizeInPixels().iHeight))):
		iStatusControl.DrawMobblerBitmap(aLeft, 
				TRect(aAlbumArtRect.iTl, TSize(aAlbumArtRect.Width() - aPosition, aAlbumArtRect.Height())),
				TRect(TPoint(aPosition, 0), TSize((((aLeft->SizeInPixels().iWidth - aPosition) * aLeft->SizeInPixels().iWidth) / aAlbumArtRect.Width()), aLeft->SizeInPixels().iHeight)));
	
	// Draw the second bitmap by the slide amount
	CMobblerBitmap::LongSidesEqual(aRight->Bitmap()->SizeInPixels(), aAlbumArtRect.Size())?
		iStatusControl.BitBltMobblerBitmap(aRight, 
				TPoint(aAlbumArtRect.iTl.iX - aPosition + aAlbumArtRect.Width(), aAlbumArtRect.iTl.iY), 
				TRect(TPoint(0, 0), TSize(aPosition, aRight->SizeInPixels().iHeight))):
		iStatusControl.DrawMobblerBitmap(aRight,
				TRect(aAlbumArtRect.iTl - TPoint(aPosition - aAlbumArtRect.Width(), 0), TSize(aPosition, aAlbumArtRect.Height())), 
				TRect(TPoint(0, 0), TSize(((aPosition * aRight->SizeInPixels().iWidth) / aAlbumArtRect.Width()), aRight->SizeInPixels().iHeight)));
	}

void CMobblerAlbumArtTransition::FingerUpL(TInt aPosition, TSlide aSlide)
	{
	iFingerUpOffset = aPosition;
	iSlide = aSlide;
	
	// Start the callbacks
	delete iTimer;
	iTimer = CPeriodic::NewL(CActive::EPriorityLow);
	TCallBack callBack(CallBack, this);
	iTimer->Start(KMobblerAlbumArtInterval, KMobblerAlbumArtInterval, callBack);
	
	// Record the starting tick count
	iStartTickCount = User::TickCount();
	}

TInt CMobblerAlbumArtTransition::CallBack(TAny* /*aRef*/)
	{
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->StatusDrawDeferred();
	return KErrNone;
	}

// End of file
