/*
mobblertrack.cpp

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

#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerappui.h"
#include "mobblerstring.h"

CMobblerTrack* CMobblerTrack::NewL(const TDesC8& aArtist,
									const TDesC8& aTitle,
									const TDesC8& aAlbum,
									const TDesC8& aAlbumArtLocation,
									const TDesC8& aMp3Location,
									TTimeIntervalSeconds aTrackLength,
									const TDesC8& aRadioAuth)
	{
	CMobblerTrack* self = new(ELeave) CMobblerTrack;
	CleanupStack::PushL(self);
	self->ConstructL(aArtist, aTitle, aAlbum, aAlbumArtLocation, aMp3Location, aTrackLength, aRadioAuth);
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
								const TDesC8& aAlbumArtLocation,
								const TDesC8& aMp3Location,
								TTimeIntervalSeconds aTrackLength,
								const TDesC8& aRadioAuth)
	{
	iArtist = CMobblerString::NewL(aArtist);
	iTitle = CMobblerString::NewL(aTitle);
	iAlbum = CMobblerString::NewL(aAlbum);
	iAlbumArtLocation = aAlbumArtLocation.AllocL();
	iMp3Location = aMp3Location.AllocL();
	iTrackLength = aTrackLength;
	iRadioAuth = aRadioAuth.AllocL();
	
	iStartTimeUTC = Time::NullTTime();
	}

CMobblerTrack::~CMobblerTrack()
	{
	delete iArtist;
	delete iTitle;
	delete iAlbum;
	delete iAlbumArtLocation;
	delete iMp3Location;
	delete iRadioAuth;
	delete iAlbumArt;
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
	return (iArtist->String().Compare(aTrack.iArtist->String())) && (iTitle->String().Compare(aTrack.iTitle->String())) && (iAlbum->String().Compare(aTrack.iAlbum->String()));
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
	}

const TTime& CMobblerTrack::StartTimeUTC() const
	{
	return iStartTimeUTC;
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

const TDesC8& CMobblerTrack::Mp3Location() const
	{
	return *iMp3Location;
	}

const TDesC8& CMobblerTrack::AlbumArtLocation() const
	{
	return *iAlbumArtLocation;
	}

void CMobblerTrack::SetAlbumArtL(const TDesC8& aAlbumArt)
	{
	delete iAlbumArt;
	iAlbumArt = CMobblerBitmap::NewL(*this, aAlbumArt);
	}

void CMobblerTrack::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}

TTimeIntervalSeconds CMobblerTrack::PlaybackPosition() const
	{
	return iPlaybackPosition;
	}

void CMobblerTrack::SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition)
	{
	iPlaybackPosition = aPlaybackPosition;
	}

const CMobblerBitmap* CMobblerTrack::AlbumArt() const
	{
	return iAlbumArt;
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
