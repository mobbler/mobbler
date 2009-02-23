/*
mobblerbitmap.h

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

#ifndef __MOBBLERBITMAP_H__
#define __MOBBLERBITMAP_H__

#include <coecntrl.h>
#include <coedef.h>

class CImageDecoder;
class CMobblerBitmap;

class MMobblerBitmapObserver
	{
public:
	virtual void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap) = 0;
	};

class CMobblerBitmap : public CActive
	{
public:
	
	static CMobblerBitmap* NewL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex);
	static CMobblerBitmap* NewL(TUid aAppUid);
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, const TDesC& aFileName, const TUid aImageType = KNullUid, const TBool aGrayscale = EFalse);
	static CMobblerBitmap* NewL(MMobblerBitmapObserver& aObserver, const TDesC8& aData, const TUid aImageType = KNullUid);
	~CMobblerBitmap();
	
	CFbsBitmap* Bitmap() const;
	CFbsBitmap* Mask() const;
	
	void SetSize(TSize aSize);
	TSize SizeInPixels() const;
	
private:
	void ConstructL(const TDesC& aFileName, const TUid aFileUid, const TBool aGrayscale);
	void ConstructL(const TDesC8& aData, const TUid aFileUid);
	void ConstructL(TUid aAppUid);
	void ConstructL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex);
	
	CMobblerBitmap(MMobblerBitmapObserver* aObserver);
	
private:
	void RunL();
	void DoCancel();
	
private:
	MMobblerBitmapObserver* iObserver;
	
	TBool iBitmapLoaded;
	CImageDecoder* iImageDecoder;
	CFbsBitmap* iBitmap;
	CFbsBitmap* iMask;
	HBufC8* iData;
	};

#endif

