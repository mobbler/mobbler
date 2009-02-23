/*
mobblerbitmap.cpp

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

#include <akniconutils.h>
#include <aknutils.h>
#include <aknsutils.h>
#include <imageconversion.h>

#include "mobblerbitmap.h"

CMobblerBitmap* CMobblerBitmap::NewL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex)
	{
	CMobblerBitmap* self = new(ELeave) CMobblerBitmap(NULL);
	CleanupStack::PushL(self);
	self->ConstructL(aMifFileName, aBitmapIndex, iMaskIndex);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(TUid aAppUid)
	{
	CMobblerBitmap* self = new(ELeave) CMobblerBitmap(NULL);
	CleanupStack::PushL(self);
	self->ConstructL(aAppUid);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, const TDesC& aFileName, const TUid aFileUid, const TBool aGrayscale)
	{
	CMobblerBitmap* self = new(ELeave) CMobblerBitmap(&aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aFileName, aFileUid, aGrayscale);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap* CMobblerBitmap::NewL(MMobblerBitmapObserver& aObserver, const TDesC8& aData, const TUid aFileUid)
	{
	CMobblerBitmap* self = new(ELeave) CMobblerBitmap(&aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aData, aFileUid);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerBitmap::CMobblerBitmap(MMobblerBitmapObserver* aObserver)
	:CActive(CActive::EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

CMobblerBitmap::~CMobblerBitmap()
	{
	Cancel();
	delete iBitmap;
	delete iMask;
	delete iImageDecoder;
	delete iData;
	}
	
CFbsBitmap* CMobblerBitmap::Bitmap() const
	{
	CFbsBitmap* bitmap = NULL;
	
	if (iBitmapLoaded)
		{
		bitmap = iBitmap;
		}
	
	return bitmap;
	}

CFbsBitmap* CMobblerBitmap::Mask() const
	{
	CFbsBitmap* mask = NULL;
	
	if (iBitmapLoaded)
		{
		mask = iMask;
		}
	
	return mask;
	}

void CMobblerBitmap::SetSize(TSize aSize)
	{
	if (this)
		{
		AknIconUtils::SetSize(iBitmap, aSize);
		}
	}

TSize CMobblerBitmap::SizeInPixels() const
	{
	TSize returnSize(0, 0);
	
	if (this && iBitmap)
		{
		returnSize = iBitmap->SizeInPixels();
		}
	
	return returnSize;
	}
	
void CMobblerBitmap::ConstructL(const TDesC& aFileName, const TUid aFileUid, const TBool aGrayscale)
	{
	// find the drive the mobbler is installed on so that we know where the graphic file is
	TParse parse;
	TFileName fileName;
	TFileName appFullName = RProcess().FileName();
	parse.Set(appFullName, NULL, NULL);
	fileName.Copy(parse.Drive());
	fileName.Append(aFileName);	
	
	iImageDecoder = CImageDecoder::FileNewL(CCoeEnv::Static()->FsSession(), fileName, CImageDecoder::EOptionAlwaysThread, aFileUid, TUid::Null(), TUid::Null());
	const TFrameInfo& info = iImageDecoder->FrameInfo();
	iBitmap = new(ELeave) CFbsBitmap();
	if (aGrayscale)
		{
		iBitmap->Create(info.iOverallSizeInPixels, EGray256);	
		}
	else
		{
		iBitmap->Create(info.iOverallSizeInPixels, info.iFrameDisplayMode);
		}
	
	if (info.iFlags & TFrameInfo::ETransparencyPossible)
		{
		iMask = new(ELeave) CFbsBitmap();
		iMask->Create(info.iOverallSizeInPixels, EGray256);
		TRequestStatus* status = &iStatus;
		iImageDecoder->Convert(status, *iBitmap, *iMask);
		}
	else
		{
		TRequestStatus* status = &iStatus;
		iImageDecoder->Convert(status, *iBitmap);
		}

	SetActive();
	}

void CMobblerBitmap::ConstructL(const TDesC8& aData, const TUid aFileUid)
	{
	iData = aData.AllocL();
	
	iImageDecoder = CImageDecoder::DataNewL(CCoeEnv::Static()->FsSession(), *iData, CImageDecoder::EOptionAlwaysThread, aFileUid, TUid::Null(), TUid::Null());
	const TFrameInfo& info = iImageDecoder->FrameInfo();
	iBitmap = new(ELeave) CFbsBitmap();
	iBitmap->Create(info.iOverallSizeInPixels, info.iFrameDisplayMode);
	
	if (info.iFlags & TFrameInfo::ETransparencyPossible)
		{
		iMask = new(ELeave) CFbsBitmap();
		iMask->Create(info.iOverallSizeInPixels, EGray256);
		TRequestStatus* status = &iStatus;
		iImageDecoder->Convert(status, *iBitmap, *iMask);
		}
	else
		{
		TRequestStatus* status = &iStatus;
		iImageDecoder->Convert(status, *iBitmap);
		}

	SetActive();
	}

void CMobblerBitmap::ConstructL(TUid aAppUid)
	{
	AknsUtils::CreateAppIconLC(AknsUtils::SkinInstance(), aAppUid,  EAknsAppIconTypeContext, iBitmap, iMask);
	CleanupStack::Pop(2);
	iBitmapLoaded = ETrue;
	}
	
void CMobblerBitmap::ConstructL(const TDesC& aMifFileName, TInt aBitmapIndex, TInt iMaskIndex)
	{
	TParse parse;
	TFileName mifFileName;
	TFileName appFullName = RProcess().FileName();
	parse.Set(appFullName, NULL, NULL);
	mifFileName.Copy(parse.Drive());
	mifFileName.Append(aMifFileName);	
	
	AknIconUtils::CreateIconL(iBitmap, iMask, mifFileName, aBitmapIndex, iMaskIndex);
	iBitmapLoaded = ETrue;
	}

void CMobblerBitmap::RunL()
	{
	delete iImageDecoder;
	iImageDecoder = NULL;
	
	if (iStatus.Int() == KErrNone)
		{
		iBitmapLoaded = ETrue;
		if (iObserver)
			{
			iObserver->BitmapLoadedL(this);
			}
		}
	}

void CMobblerBitmap::DoCancel()
	{
	iImageDecoder->Cancel();
	}
				
