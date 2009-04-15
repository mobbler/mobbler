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
#include "mobblerlogging.h"
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
	: iTrackNumber(KErrUnknown), iStartTimeUTC(Time::NullTTime()), 
	  iTotalPlayed(0), iInitialPlaybackPosition(KErrUnknown),
	  iTriedButCouldntFindAlbumArt(EFalse),
	  iUsingArtistImage(EFalse)
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
	
	if (!iAlbumArt && iAlbum->String().Length() != 0)
		{
		// fetch the album info
		LOG(_L8("0 FetchAlbumInfoL()"));
		FetchAlbumInfoL();
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
	LOG(_L8("CMobblerTrack::SetAlbumL"));
	LOG(aAlbum);

	delete iAlbum;
	iAlbum = CMobblerString::NewL(aAlbum);

	// Check if there's a something better online
	if ((!iAlbumArt || iUsingArtistImage)
				&& iState == ENone)
		{
		if (iAlbum->String().Length() != 0)
			{
			// There is an album name!

			// Try to fetch the album info. Once this is fetched
			// we will try to fetch the album art in the callback.
			LOG(_L8("2 FetchAlbumInfoL()"));
			FetchAlbumInfoL();
			}
		else if (iImage->Length() != 0)
			{
			// We don't know the album name, but there was album art in the playlist
			LOG(_L8("3 FetchImageL(album)"));
			FetchImageL(EFetchingAlbumArt, *iImage);
			}
		else if (!iUsingArtistImage)
			{
			 // No album art, let's try the artist image instead.
			// Try to fetch the artist info. Once this is fetched
			// we will try to fetch the album art in the callback.
			LOG(_L8("4 FetchArtistInfoL()"));
			FetchArtistInfoL();
			}
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
	LOG(_L8("CMobblerTrack::SetPathL"));
	LOG(aPath);

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
				LOG(_L8("Found %album%.jpg/gif/png"));
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
			LOG(_L8("Found cover.jpg/gif/png, folder.jpg/gif/png"));
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
			fileName.Append(iArtist->SafeFsString());
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found %artist%.jpg/gif/png"));
				found = ETrue;
				iUsingArtistImage = ETrue;
				break;
				}
			}
		}

	if (found)
		{
		LOG(fileName);
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
		if (IsMusicPlayerTrack())
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

void CMobblerTrack::DataL(const TDesC8& aData, CMobblerLastFMConnection::TError aError)
	{
	switch (iState)
		{
		case EFetchingAlbumInfo:
			{
			LOG(_L8("5 DataL album info fetched"));
			DUMPDATA(aData, _L("albumgetinfo.xml"));

			TBool found(EFalse);
			if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				LOG(_L8("6 FetchImageL(album)"));
				found = FetchImageL(aData);
				}
			
			if (!found)
				{
				// we failed to fetch the album details
				// so try to fetch the album art from the playlist
				if (iImage->Length() > 0)
					{
					LOG(_L8("7 FetchImageL(album)"));
					FetchImageL(EFetchingAlbumArt, *iImage);
					}
				else if (!iUsingArtistImage)
					{
					// Couldn't fetch album art, try artist image instead
					iTriedButCouldntFindAlbumArt = ETrue;
					LOG(_L8("8 FetchArtistInfoL()"));
					FetchArtistInfoL();
					}
				}
			break;
			}

		case EFetchingAlbumArt:
			{
			LOG(_L8("9 DataL album art fetched"));
			if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				SaveAlbumArtL(aData);
				}
			else
				{
				// Couldn't fetch album art, try artist image instead
				LOG(_L8("10 FetchArtistInfoL()"));
				iTriedButCouldntFindAlbumArt = ETrue;
				FetchArtistInfoL();
				}
			break;
			}

		case EFetchingArtistInfo:
			{
			LOG(_L8("11 DataL artist info fetched"));
			DUMPDATA(aData, _L("artistgetimage.xml"));

			if (iAlbumArt)
				{
				iState = ENone;
				}
			else if (!iTriedButCouldntFindAlbumArt && iAlbum->String().Length() != 0)
				{
				// Just got artist info but there's now an album name, give up and try album art
				LOG(_L8("12 FetchAlbumInfoL()"));
				FetchAlbumInfoL();
				}
			else if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				FetchImageL(aData);
				}
			break;
			}

		case EFetchingArtistImage:
			{
			LOG(_L8("13 DataL artist image fetched"));
			if (iAlbumArt)
				{
				iState = ENone;
				}
			else if (aError == CMobblerLastFMConnection::EErrorNone)
				{
				SaveAlbumArtL(aData);

				if (!iTriedButCouldntFindAlbumArt && 
					iAlbum->String().Length() != 0)
					{
					// Just got artist art but there's now an album name, try album image
					LOG(_L8("14 FetchAlbumInfoL()"));
					FetchAlbumInfoL();
					}
				}
			break;
			}

		default:
			break;
		}
	}

TBool CMobblerTrack::IsMusicPlayerTrack() const
	{
	return (iRadioAuth->Compare(KNullDesC8) == 0);

	// TODO or return (iRadioAuth->Length() != 0) ?
	}

void CMobblerTrack::FetchAlbumInfoL()
	{
	TInt downloadAlbumArt(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt());
	TBool okTodownloadAlbumArt((downloadAlbumArt == 1 && !IsMusicPlayerTrack()) || (downloadAlbumArt == 2));
	
	if (okTodownloadAlbumArt && 
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().Mode() == CMobblerLastFMConnection::EOnline)
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().AlbumGetInfoL(*this, *this);
		iState = EFetchingAlbumInfo;
		}
	}

void CMobblerTrack::FetchArtistInfoL()
	{
	TInt downloadAlbumArt(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt());
	TBool okTodownloadAlbumArt((downloadAlbumArt == 1 && !IsMusicPlayerTrack()) || (downloadAlbumArt == 2));
	
	if (okTodownloadAlbumArt && 
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().Mode() == CMobblerLastFMConnection::EOnline)
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().ArtistGetImageL(iArtist->String(), *this);
		iState = EFetchingArtistInfo;
		}
	}
	
void CMobblerTrack::FetchImageL(TState aState, const TDesC8& aImageLocation)
	{
	TInt downloadAlbumArt(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt());
	TBool okTodownloadAlbumArt((downloadAlbumArt == 1 && !IsMusicPlayerTrack()) || (downloadAlbumArt == 2));
	
	if (okTodownloadAlbumArt && 
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().Mode() == CMobblerLastFMConnection::EOnline)
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFMConnection().RequestImageL(this, aImageLocation);
		iState = aState;
		}
	}

TBool CMobblerTrack::FetchImageL(const TDesC8& aData)
	{
	TBool found(EFalse);
	
	// create the XML reader and DOM fragement and associate them with each other 
	CSenXmlReader* xmlReader = CSenXmlReader::NewL();
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment = CSenDomFragment::NewL();
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);
	
	// parse the XML into the DOM fragment
	xmlReader->ParseL(aData);
	
	RPointerArray<CSenElement> imageArray;
	CleanupClosePushL(imageArray);
	
	if (iState == EFetchingAlbumInfo)
		{
		User::LeaveIfError(domFragment->AsElement().Element(_L8("album"))->ElementsL(imageArray, _L8("image")));
		}
	else
		{
		CSenElement* element = domFragment->AsElement().Element(_L8("images"));
		
		if (element)
			{
			element = element->Element(_L8("image"));
			
			if (element)
				{
				element = element->Element(_L8("sizes"));
				
				if (element)
					{
					User::LeaveIfError(element->ElementsL(imageArray, _L8("size")));
					}
				}
			}
		}
	
	const TInt KImageCount(imageArray.Count());
	for (TInt i(0); i < KImageCount; ++i)
		{
		if ((iState == EFetchingAlbumInfo &&
			 imageArray[i]->AttrValue(_L8("size"))->Compare(_L8("extralarge")) == 0)
				||
			(iState == EFetchingArtistInfo &&
			 imageArray[i]->AttrValue(_L8("name"))->Compare(_L8("largesquare")) == 0))
			{
			if (imageArray[i]->Content().Length() > 0)
				{
				found = ETrue;
				iState == EFetchingAlbumInfo ?
					LOG(_L8("15 FetchImageL(album)")):
					LOG(_L8("16 FetchImageL(artist)"));
				iState == EFetchingAlbumInfo ?
					FetchImageL(EFetchingAlbumArt, imageArray[i]->Content()):
					FetchImageL(EFetchingArtistImage, imageArray[i]->Content());
				}
			break;
			}
		}
	
	CleanupStack::PopAndDestroy(&imageArray);
	CleanupStack::PopAndDestroy(domFragment);
	CleanupStack::PopAndDestroy(xmlReader);

	return found;
	}

void CMobblerTrack::SaveAlbumArtL(const TDesC8& aData)
	{
	if (iPath)
		{
		// try to save the album art in the album folder
		
		TFileName albumArtFileName;
		albumArtFileName.Append(*iPath);
		if (iState == EFetchingAlbumArt)
			{
			albumArtFileName.Append(KArtFileArray[0]);
			}
		else
			{
			albumArtFileName.Append(iArtist->SafeFsString());
			albumArtFileName.Append(_L(".jpg"));
			}
		
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
	
	iAlbumArt = CMobblerBitmap::NewL(*this, aData);
	iState = ENone;
	}

// End of file
