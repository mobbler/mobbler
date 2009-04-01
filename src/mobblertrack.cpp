/*
mobblertrack.cpp

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

#include <bautils.h>
#include <sendomfragment.h>
#include <senxmlutils.h> 

#include "mobblerappui.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

const TPtrC KArtExtensionArray[] =
	{
	_L(".jpg"),
	_L(".gif"),
	_L(".png"),
	};

const TPtrC KArtFileArray[] =
	{
	_L("cover.jpg"),
	_L("cover.gif"),
	_L("cover.png"),
	_L("folder.jpg"),
	_L("folder.gif"),
	_L("folder.png"),
	};

CMobblerTrack* CMobblerTrack::NewL(const TDesC8& aArtist,
									const TDesC8& aTitle,
									const TDesC8& aAlbum,
									//const TDesC8& aMbAlbumId,
									const TDesC8& aMbTrackId,
									const TDesC8& aImage,
									const TDesC8& aMp3Location,
									TTimeIntervalSeconds aTrackLength,
									const TDesC8& aRadioAuth)
	{
	CMobblerTrack* self = new(ELeave) CMobblerTrack;
	CleanupStack::PushL(self);
	self->ConstructL(aArtist, aTitle, aAlbum, /*aMbAlbumId,*/ aMbTrackId, aImage, aMp3Location, aTrackLength, aRadioAuth);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrack* CMobblerTrack::NewL(RReadStream& aReadStream)
	{
	CMobblerTrack* self = new(ELeave) CMobblerTrack;
	CleanupStack::PushL(self);
	self->InternalizeL(aReadStream);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrack::CMobblerTrack()
	{
	Open();
	}

void CMobblerTrack::ConstructL(const TDesC8& aArtist,
		const TDesC8& aTitle,
		const TDesC8& aAlbum,
		//const TDesC8& aMbAlbumId,
		const TDesC8& aMbTrackId,
		const TDesC8& aImage,
		const TDesC8& aMp3Location,
		TTimeIntervalSeconds aTrackLength,
		const TDesC8& aRadioAuth)
	{
	iArtist = CMobblerString::NewL(aArtist);
	iTitle = CMobblerString::NewL(aTitle);
	iAlbum = CMobblerString::NewL(aAlbum);
	//iMbAlbumId = CMobblerString::NewL(aMbAlbumId);
	iMbTrackId = CMobblerString::NewL(aMbTrackId);
	iMp3Location = aMp3Location.AllocL();
	iTrackLength = aTrackLength;
	iRadioAuth = aRadioAuth.AllocL();
	iImage = aImage.AllocL();
	
	iStartTimeUTC = Time::NullTTime();
	iInitialPlaybackPosition = KErrUnknown;
	iTrackNumber = KErrUnknown;
	iTotalPlayed = 0;
	
	if (iAlbum->String().Length() != 0)
		{
		TFileName albumArtFileNameCache = AlbumArtCacheFileName();
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), albumArtFileNameCache))
			{
			// this album exitst in the cache so load the album art from there
			iAlbumArt = CMobblerBitmap::NewL(*this, albumArtFileNameCache);
			}
		}
	
	if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().Mode() == CMobblerLastFMConnection::EOnline
			&& iAlbum->String().Length() != 0
			&& !iAlbumArt)
		{
		// fetch the album info
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().AlbumGetInfoL(*this, *this);	
		iState = EFetchingAlbumInfo;
		}		
	}

CMobblerTrack::~CMobblerTrack()
	{
	delete iArtist;
	delete iTitle;
	delete iAlbum;
	delete iMbTrackId;
	//delete iMbAlbumId;
	delete iMp3Location;
	delete iRadioAuth;
	delete iAlbumArt;
	delete iPath;
	delete iImage;
	
	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().CancelTransaction(this);
	}

void CMobblerTrack::Open()
	{
	++iRefCount;
	}

void CMobblerTrack::Release()
	{
	if (--iRefCount == 0)
		{
		delete this;
		}
	}

TBool CMobblerTrack::operator==(const CMobblerTrack& aTrack) const
	{
	// check that the artist, album, and track name are the same
	return (iArtist->String().Compare(aTrack.iArtist->String()) == 0) && 
		   (iTitle->String().Compare(aTrack.iTitle->String()) == 0) && 
		   (iAlbum->String().Compare(aTrack.iAlbum->String()) == 0);
	}

void CMobblerTrack::InternalizeL(RReadStream& aReadStream)
	{
	TInt high = aReadStream.ReadInt32L();
	TInt low = aReadStream.ReadInt32L();
	iStartTimeUTC = MAKE_TINT64(high, low);
	iTrackLength = aReadStream.ReadInt32L();
	delete iArtist;
	delete iTitle;
	iArtist = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	iTitle = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	iLove = aReadStream.ReadInt8L();
	delete iRadioAuth;
	iRadioAuth = HBufC8::NewL(aReadStream, KMaxTInt);
	delete iAlbum; 
	iAlbum = CMobblerString::NewL(*HBufC8::NewLC(aReadStream, KMaxTInt));
	CleanupStack::PopAndDestroy();
	}

void CMobblerTrack::ExternalizeL(RWriteStream& aWriteStream) const
	{
	aWriteStream.WriteInt32L(I64HIGH(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(I64LOW(iStartTimeUTC.Int64()));
	aWriteStream.WriteInt32L(iTrackLength.Int());
	aWriteStream << iArtist->String8();
	aWriteStream << iTitle->String8();
	aWriteStream.WriteInt8L(iLove);
	aWriteStream << *iRadioAuth;
	aWriteStream << iAlbum->String8();
	}

void CMobblerTrack::SetDataSize(TInt aDataSize)
	{
	iDataSize = aDataSize;
	}

TInt CMobblerTrack::DataSize() const
	{
	// if we don't know the data size then return 1
	return (iDataSize == KErrNotFound) || (iDataSize == 0) ? 1 : iDataSize;
	}

void CMobblerTrack::BufferAdded(TInt aBufferSize)
	{
	iBuffered += aBufferSize;
	}

TInt CMobblerTrack::Buffered() const
	{
	// if we don't know the data size then return 0
	return (iDataSize == KErrNotFound) ? 0 : iBuffered;
	}

void CMobblerTrack::SetScrobbled()
	{
	iScrobbled = ETrue;
	}

TBool CMobblerTrack::Scrobbled() const
	{
	return iScrobbled;
	}

void CMobblerTrack::SetStartTimeUTC(const TTime& aStartTimeUTC)
	{
	iStartTimeUTC = aStartTimeUTC;
	iTrackPlaying = ETrue;
	}

const TTime& CMobblerTrack::StartTimeUTC() const
	{
	return iStartTimeUTC;
	}

void CMobblerTrack::SetTotalPlayed(TTimeIntervalSeconds aTotalPlayed)
	{

	iTotalPlayed = aTotalPlayed;
	iTrackPlaying = EFalse;
	}

TTimeIntervalSeconds CMobblerTrack::TotalPlayed() const
	{
	return iTotalPlayed;
	}

const CMobblerString& CMobblerTrack::Artist() const
	{
	return *iArtist;
	}

const CMobblerString& CMobblerTrack::Title() const
	{
	return *iTitle;
	}

const CMobblerString& CMobblerTrack::Album() const
	{
	return *iAlbum;
	}

void CMobblerTrack::SetAlbumL(const TDesC& aAlbum)
	{
	delete iAlbum;
	iAlbum = CMobblerString::NewL(aAlbum);
	
	if (iAlbum->String().Length() != 0)
		{
		// There is an album name!
		
		TFileName albumArtFileNameCache = AlbumArtCacheFileName();
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), albumArtFileNameCache))
			{
			// This album exists in the cache so load the album art from there
			iAlbumArt = CMobblerBitmap::NewL(*this, albumArtFileNameCache);
			}
		else if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().Mode() == CMobblerLastFMConnection::EOnline)
			{
			// We are online so try to fetch the album info.
			// Once this is fetched we will try to fetch the album art
			// in the callback
			static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().AlbumGetInfoL(*this, *this);	
			iState = EFetchingAlbumInfo;
			}
		}
	else if (iImage->Length() != 0)
		{
		// We don't know the album name, but there was album art in the playlist
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().RequestImageL(this, *iImage);	
		iState = EFetchingAlbumArt;
		}
	}

/*const CMobblerString& CMobblerTrack::MbAlbumId() const
	{
	return *iMbAlbumId;
	}
*/
TInt CMobblerTrack::TrackNumber() const
	{
	return iTrackNumber;
	}

void CMobblerTrack::SetTrackNumber(TInt aTrackNumber)
	{
	iTrackNumber = aTrackNumber;
	}

const TDesC8& CMobblerTrack::Mp3Location() const
	{
	return *iMp3Location;
	}

void CMobblerTrack::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

void CMobblerTrack::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	}

void CMobblerTrack::SetPathL(const TDesC& aPath)
	{
	TParse parse;
	parse.Set(aPath, NULL, NULL);
	TFileName fileName;
	TBool found(EFalse);
	
	iPath = parse.DriveAndPath().AllocL();

	// First check for %album%.jpg/gif/png
	if (iAlbum->String().Length() > 0)
		{
		const TInt arraySize = sizeof(KArtExtensionArray) / sizeof(TPtrC);
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(parse.DriveAndPath());
			fileName.Append(iAlbum->String());
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				found = ETrue;
				break;
				}
			}
		}

	// If not found, check for cover.jpg/gif/png, folder.jpg/gif/png
	const TInt arraySize = sizeof(KArtFileArray) / sizeof(TPtrC);
	for (TInt i(0); i < arraySize && !found; ++i)
		{
		fileName.Copy(parse.DriveAndPath());
		fileName.Append(KArtFileArray[i]);

		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
			{
			found = ETrue;
			break;
			}
		}

	// If still not found, check for %artist%.jpg/gif/png
	if (!found && iArtist->String().Length() > 0)
		{
		const TInt arraySize = sizeof(KArtExtensionArray) / sizeof(TPtrC);
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(parse.DriveAndPath());
			fileName.Append(iArtist->String());
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				found = ETrue;
				break;
				}
			}
		}

	if (found)
		{
		delete iAlbumArt;
		iAlbumArt = CMobblerBitmap::NewL(*this, fileName);
		}
	}

TTimeIntervalSeconds CMobblerTrack::PlaybackPosition() const
	{
	return iPlaybackPosition;
	}

void CMobblerTrack::SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition)
	{
	iPlaybackPosition = aPlaybackPosition;

	if (iInitialPlaybackPosition.Int() == KErrUnknown)
		{
		iInitialPlaybackPosition = aPlaybackPosition;

		// Only need to correct music player tracks, because 
		// iInitialPlaybackPosition may be up to 5 seconds off
		if (iRadioAuth->Compare(KNullDesC8) == 0)
			{
			TTimeIntervalSeconds offset(0);
			TTime now;
			now.UniversalTime();
			TInt error = now.SecondsFrom(iStartTimeUTC, offset);
			if (error == KErrNone && offset.Int() > 0)
				{
				iInitialPlaybackPosition = Max(0, iInitialPlaybackPosition.Int() - offset.Int());
				}
			}
		}	
	}

const CMobblerBitmap* CMobblerTrack::AlbumArt() const
	{
	return iAlbumArt;
	}

const CMobblerString& CMobblerTrack::MbTrackId() const
	{
	return *iMbTrackId;
	}

const TDesC8& CMobblerTrack::RadioAuth() const
	{
	return *iRadioAuth;
	}

void CMobblerTrack::SetLove(TBool aLove)
	{
	iLove = aLove;
	}

TBool CMobblerTrack::Love() const
	{
	return iLove;
	}

TTimeIntervalSeconds CMobblerTrack::TrackLength() const
	{
	return iTrackLength.Int() == 0 ? 1 : iTrackLength;
	}

TTimeIntervalSeconds CMobblerTrack::ScrobbleDuration() const
	{
	TInt scrobblePercent = static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ScrobblePercent();
	return (TTimeIntervalSeconds)Min(240, (iTrackLength.Int() *  scrobblePercent / 100));
	}

TTimeIntervalSeconds CMobblerTrack::InitialPlaybackPosition() const
	{
	if (iInitialPlaybackPosition.Int() == KErrUnknown)
		{
		return 0;
		}
	else
		{
		return iInitialPlaybackPosition;
		}
	}

void CMobblerTrack::SetTrackPlaying(TBool aTrackPlaying)
	{
	iTrackPlaying = aTrackPlaying;
	}

TBool CMobblerTrack::TrackPlaying() const
	{
	return iTrackPlaying;
	}

void CMobblerTrack::DataL(const TDesC8& aData, TInt aError)
	{
	if (iState == EFetchingAlbumInfo)
		{
		if (aError == KErrNone)
			{
			iState = ENone;
			
			// create the xml reader and dom fragement and associate them with each other 
			CSenXmlReader* xmlReader = CSenXmlReader::NewL();
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment = CSenDomFragment::NewL();
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			// parse the xml into the dom fragment
			xmlReader->ParseL(aData);
			
			RPointerArray<CSenElement> imageArray;
			CleanupClosePushL(imageArray);
			User::LeaveIfError(domFragment->AsElement().Element(_L8("album"))->ElementsL(imageArray, _L8("image")));
			
			const TInt KImageCount(imageArray.Count());
			for (TInt i(0) ; i < KImageCount ; ++i)
				{
				if (imageArray[i]->AttrValue(_L8("size"))->Compare(_L8("extralarge")) == 0)
					{
					if (imageArray[i]->Content().Length() > 0)
						{
						static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().RequestImageL(this, imageArray[i]->Content());	
						iState = EFetchingAlbumArt;
						}
					else if (iImage->Length() > 0)
						{
						// the extralarge image content was blank, but
						// there was an album art location in the playlist
						static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().RequestImageL(this, *iImage);	
						iState = EFetchingAlbumArt;
						}
					
					break;
					}
				}
			
			CleanupStack::PopAndDestroy(&imageArray);
			CleanupStack::PopAndDestroy(domFragment);
			CleanupStack::PopAndDestroy(xmlReader);
			}
		else
			{
			// we failed to fetch the album details
			// so try to fetch the album art from the playlist
			
			if (iImage->Length() > 0)
				{
				static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().RequestImageL(this, *iImage);				
				iState = EFetchingAlbumArt;
				}
			}
		}
	else if (iState == EFetchingAlbumArt)
		{
		if (aError == KErrNone)
			{
			if (iPath)
				{
				// try to save the album art in the album folder
				
				TFileName albumArtFileName;
				albumArtFileName.Append(*iPath);
				albumArtFileName.Append(_L("cover.jpg"));
				
				RFile albumArtFile;
				TInt createError = albumArtFile.Create(CCoeEnv::Static()->FsSession(), albumArtFileName, EFileWrite);
				
				if (createError == KErrNone)
					{
					TInt writeError = albumArtFile.Write(aData);
					if (writeError != KErrNone)
						{
						albumArtFile.Close();
						CCoeEnv::Static()->FsSession().Delete(albumArtFileName);
						}
					}
				
				albumArtFile.Close();
				}
			
			TFileName albumArtFileName = AlbumArtCacheFileName();

			TInt error = CCoeEnv::Static()->FsSession().MkDirAll(albumArtFileName);
			
			if (error == KErrNone || error == KErrAlreadyExists)
				{
				RFile file;
				TInt createError = file.Create(CCoeEnv::Static()->FsSession(), albumArtFileName, EFileWrite);
				if (createError == KErrNone)
					{
					TInt writeError = file.Write(aData);
					if (writeError != KErrNone)
						{
						file.Close();
						CCoeEnv::Static()->FsSession().Delete(albumArtFileName);
						}
					}
				
				file.Close();
				}
			
			iAlbumArt = CMobblerBitmap::NewL(*this, aData);
			}
		}
	}

TFileName CMobblerTrack::AlbumArtCacheFileName()
	{
	_LIT(KCoverCacheFolder, "e:\\data\\mobbler\\");
	
	HBufC8* md5Input8 = HBufC8::NewLC(iArtist->String().Length() + iAlbum->String().Length()); 
	md5Input8->Des().Append(iArtist->String8());
	md5Input8->Des().Append(iAlbum->String8());
	
	HBufC8* md5Output8 = MobblerUtility::MD5LC(*md5Input8);
	
	HBufC* md5Output = HBufC::NewLC(md5Output8->Length());
	md5Output->Des().Copy(*md5Output8);
	
	TFileName albumArtFileName;
	albumArtFileName.Append(KCoverCacheFolder);
	albumArtFileName.Append(*md5Output);
	albumArtFileName.Append(_L(".jpg"));
	
	CleanupStack::PopAndDestroy(3, md5Input8);
	
	return albumArtFileName;
	}


// End of file
