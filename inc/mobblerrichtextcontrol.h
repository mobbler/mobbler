/*
mobblerrichtextcontrol.h

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

#ifndef __MOBBLERRICHTEXTCONTROL_H__
#define __MOBBLERRICHTEXTCONTROL_H__


#include <coecntrl.h>
#include <eikrted.h>

#include "mobblerwebservicesobserver.h"

class CMobblerRichTextControl : public CCoeControl, public MWebServicesObserver
    {
public: 
	static CMobblerRichTextControl* NewL(const TRect& aRect);
	
	void SetContentL(const TDesC& aContent);
    
private:
	void ConstructL(const TRect& aRect);
	~CMobblerRichTextControl();
	
	void Prepare();
	
private: // from MWebServicesObserver
	void WebServicesResponseL(const TDesC8& aXML);

private: // from CCoeControl
    void SizeChanged();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;
    void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

private:
	CEikRichTextEditor* iRichTextEditor;
    };

#endif // __MOBBLERRICHTEXTCONTROL_H__

