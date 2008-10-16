/*
mobblertrack.h

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

#ifndef __TRACKINFO_H__
#define __TRACKINFO_H__

#include <e32base.h>
#include <s32file.h>

#include "mobblerbitmap.h"

class CMobblerString;

class CMobblerTrack : public CBase, public MMobblerBitmapObserver
	{
public:
	static CMobblerTrack* NewL(const TDesC8& aArtist,
								const TDesC8& aTitle,
								const TDesC8& aAlbum,
								const TDesC8& aAlbumArtLocation,
								const TDesC8& aMp3Location,
								TTimeIntervalSeconds aTrackLength,
								const TDesC8& aRadioAuth);
	
	static CMobblerTrack* NewL(RReadStream& aReadStream);
	
	void Open();
	void Release();
	
	void SetScrobbled();
	TBool Scrobbled() const;
	
	const CMobblerString& Artist() const;
	const CMobblerString& Album() const;
	const CMobblerString& Title() const;
	const TDesC8& Mp3Location() const;
	const TDesC8& AlbumArtLocation() const;
	const TDesC8& RadioAuth() const;
	TTimeIntervalSeconds TrackLength() const;
	TTimeIntervalSeconds PlaybackPosition() const;
	
	void SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition);
	
	void SetDataSize(TInt aDataSize);
	TInt DataSize() const;
	void BufferAdded(TInt aBufferSize);
	TInt Buffered() const;
	
	void SetAlbumArtL(const TDesC8& aAlbumArt);
	const CMobblerBitmap* AlbumArt() const;
	
	void SetStartTimeUTC(const TTime& aStartTimeUTC);
	const TTime& StartTimeUTC() const;
	
	void SetLove(TBool aLove);
	TBool Love() const;
	
	void InternalizeL(RReadStream& aReadStream);
	void ExternalizeL(RWriteStream& aWriteStream) const;
	
	TBool operator==(const CMobblerTrack& aTrack) const;
	
private:
	CMobblerTrack();
	~CMobblerTrack();
	
	void ConstructL(const TDesC8& aArtist,
						const TDesC8& aTitle,
						const TDesC8& aAlbum,
						const TDesC8& aAlbumArtLocation,
						const TDesC8& aMp3Location,
						TTimeIntervalSeconds aTrackLength,
						const TDesC8& aRadioAuth);
	
private:
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	
private:
	// track details
	CMobblerString* iArtist;
	CMobblerString* iTitle;
	CMobblerString* iAlbum;
	TTime iStartTimeUTC;
	TTimeIntervalSeconds iTrackLength;
	TTimeIntervalSeconds iPlaybackPosition;
	TBool iLove;
	
	// album art
	HBufC8* iAlbumArtLocation;
	HBufC8* iAlbumArtBuffer;
	
	CMobblerBitmap* iAlbumArt;
	
	// mp3 location
	HBufC8* iMp3Location;
	HBufC8* iRadioAuth;
	
	TBool iScrobbled;
	
	TInt iRefCount;
	
	TInt iDataSize;
	TInt iBuffered;
	};
	
#endif // __TRACKINFO_H__
