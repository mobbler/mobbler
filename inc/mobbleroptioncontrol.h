/*
mobblerfriendlist.h

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

#ifndef __MOBBLEROPTIONCONTROL_H__
#define __MOBBLEROPTIONCONTROL_H__

#include <aknlists.h>
#include <e32base.h>

#include "mobblerbitmap.h"
#include "mobblerlistcontrol.h"

class CMobblerAppUi;

class CMobblerOptionControl : public CMobblerListControl, public MMobblerBitmapObserver
	{
public:
	static CMobblerOptionControl* NewL(CMobblerAppUi& aAppUi, const TRect& aRect);
	~CMobblerOptionControl();
	
	CMobblerListControl* HandleListCommandL(TInt aCommand);
	void SupportedCommandsL(RArray<TInt>& aCommands);
	
private:
	CMobblerOptionControl(CMobblerAppUi& aAppUi);
	void ConstructL(const TRect& aRect);
	
	void UpdateIconArrayL();
	
private: // from MMobblerBitmapObserver
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
		
private: // from CCoeControl
	void Draw(const TRect& aRect) const;
	CCoeControl* ComponentControl(TInt /*aIndex*/) const;
	TInt CountComponentControls() const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode);
	
	void HandleLoadedL();
	
private:	
	CAknSingleLargeStyleListBox* iListBox;
	
	CMobblerBitmap* iFriendsIcon;
	};

#endif // __MOBBLEROPTIONCONTROL_H__
