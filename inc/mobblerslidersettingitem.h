/*
mobblerslidersettingitem.h

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

#include <aknsettingitemlist.h>

class CMobblerSliderSettingItem : public CAknSliderSettingItem
	{
public:
	CMobblerSliderSettingItem(TInt aIdentifier,
							  TInt& aSliderValue,
							  TInt aResourceId);
	CMobblerSliderSettingItem(TInt aIdentifier,
							  TInt& aSliderValue,
							  TInt aResourceIdSingular,
							  TInt aResourceIdPlural);

protected: // Functions from base classes
	void CreateAndExecuteSettingPageL();
	CFbsBitmap* CreateBitmapL();
	const TDesC& SettingTextL();

private:
	TInt iResourceIdSingular;
	TInt iResourceIdPlural;
	TBuf<256> iText;

	};

// End of file
