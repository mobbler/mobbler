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

#include "mobblerappui.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

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
	iScrobbleTime = (TTimeIntervalSeconds)Min(240, (iTrackLength.Int()/2));
	iInitialPlaybackPosition = KErrUnknown;
	iTotalPlayed = 0;
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
	return iScrobbleTime;
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

// End of file
