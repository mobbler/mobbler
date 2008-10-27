/*
mobblermarquee.cpp

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

#include "mobblermarquee.h"
#include "mobblerappui.h"

const TTimeIntervalMicroSeconds32 KDelay(2000000);
const TTimeIntervalMicroSeconds32 KInterval(100000);

CMobblerMarquee* CMobblerMarquee::NewL()
	{
	CMobblerMarquee* self = new(ELeave) CMobblerMarquee;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerMarquee::CMobblerMarquee()
	{
	}

void CMobblerMarquee::ConstructL()
	{
	iTimer = CPeriodic::NewL(CActive::EPriorityLow);
	}

CMobblerMarquee::~CMobblerMarquee()
	{
	delete iTimer;
	}

void CMobblerMarquee::Start(const TDesC& aText, TInt aInitialOffset, TInt aTextWidth, TInt aDisplayWidth)
	{
	if (aText.Compare(iText) != 0)
		{
		iText = aText;
		iTextWidth = aTextWidth;
		iDisplayWidth = aDisplayWidth;
		iInitialOffset = aInitialOffset;
		
		iTimer->Cancel();
		
		if (iTextWidth + aInitialOffset > iDisplayWidth)
			{
			TCallBack callBack(CallBack, this);
			iTimer->Start(KDelay, KInterval, callBack);
			}
		
		iState = EStart;
		}
	}
	
TInt CMobblerMarquee::GetPosition1() const
	{
	TInt offset(iInitialOffset);
	TInt calcOffset(0);
	TInt maxOffset(0);
	
	switch (iState)
		{
		case EStart:
			// do nothing
			break;
		case EForward:
			calcOffset = iInitialOffset - (User::TickCount() - iStartTickCount);
			maxOffset =  iInitialOffset - iTextWidth - (iInitialOffset * 5);
			offset = Max(calcOffset, maxOffset);
			break;
		}
	
	return offset;
	}

TInt CMobblerMarquee::GetPosition2() const
	{
	TInt offset(KMaxTInt);
	
	if (iTextWidth > iDisplayWidth)
		{
		offset = GetPosition1() + iTextWidth + (iInitialOffset * 5);
		}
	
	return offset;
	}

TInt CMobblerMarquee::CallBack(TAny* aRef)
	{
	static_cast<CMobblerMarquee*>(aRef)->Update();
	return KErrNone;
	}

void CMobblerMarquee::Update()
	{
	switch (iState)
		{
		case EStart:
			iState = EForward;
			iStartTickCount = User::TickCount();
			break;
		case EForward:
			if (GetPosition1() > iInitialOffset - iTextWidth - (iInitialOffset * 5))
				{
				// do nothing
				}
			else
				{
				iState = EStart;
				
				delete iTimer;
				iTimer = CPeriodic::NewL(CActive::EPriorityHigh);
				TCallBack callBack(CallBack, this);
				iTimer->Start(KDelay, KInterval, callBack);
				}
			break;
		}
	
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

