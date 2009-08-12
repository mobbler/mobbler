/*
mobblertrack.h

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

#ifndef __MOBBLERTRACK_H__
#define __MOBBLERTRACK_H__

#include <e32base.h>

#include "mobblerbitmap.h"
#include "mobblerdataobserver.h"

class CMobblerString;

class CMobblerTrack : public CBase, public MMobblerBitmapObserver, public MMobblerFlatDataObserver
	{
private:
	enum TState
		{
		ENone,
		EFetchingAlbumInfo,
		EFetchingAlbumArt,
		EFetchingArtistInfo,
		EFetchingArtistImage
		};
public:
	static CMobblerTrack* NewL(const TDesC8& aArtist,
								const TDesC8& aTitle,
								const TDesC8& aAlbum,
								//const TDesC8& aMbAlbumId,
								const TDesC8& aMbTrackId,
								const TDesC8& aImage,
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
	//const CMobblerString& MbAlbumId() const;
	const CMobblerString& MbTrackId() const;
	void SetAlbumL(const TDesC& aAlbum);
	TInt TrackNumber() const;
	void SetTrackNumber(const TInt aTrackNumber);
	const CMobblerString& Title() const;
	const TDesC8& Mp3Location() const;
	const TDesC8& RadioAuth() const;
	TTimeIntervalSeconds TrackLength() const;
	TTimeIntervalSeconds ScrobbleDuration() const;
	TTimeIntervalSeconds InitialPlaybackPosition() const;
	TTimeIntervalSeconds PlaybackPosition() const;
	
	void SetPlaybackPosition(TTimeIntervalSeconds aPlaybackPosition);
	
	void SetDataSize(TInt aDataSize);
	TInt DataSize() const;
	void BufferAdded(TInt aBufferSize);
	TInt Buffered() const;
	
	void SetPathL(const TDesC& aPath);
	const CMobblerBitmap* AlbumArt() const;
	
	void SetStartTimeUTC(const TTime& aStartTimeUTC);
	const TTime& StartTimeUTC() const;
	
	void SetLove(TBool aLove);
	TBool Love() const;
	
	void InternalizeL(RReadStream& aReadStream);
	void ExternalizeL(RWriteStream& aWriteStream) const;
	
	TBool operator==(const CMobblerTrack& aTrack) const;

	void SetTotalPlayed(TTimeIntervalSeconds aTotalPlayed);
	TTimeIntervalSeconds TotalPlayed() const;
	void SetTrackPlaying(TBool aTrackPlaying);
	TBool TrackPlaying() const;
	TBool IsMusicPlayerTrack() const;
	void DownloadAlbumArtL();
	
private:
	CMobblerTrack();
	~CMobblerTrack();
	
	void ConstructL(const TDesC8& aArtist,
						const TDesC8& aTitle,
						const TDesC8& aAlbum,
						//const TDesC8& aMbAlbumId,
						const TDesC8& aMbTrackId,
						const TDesC8& aImage,
						const TDesC8& aMp3Location,
						TTimeIntervalSeconds aTrackLength,
						const TDesC8& aRadioAuth);
	
	void FetchAlbumInfoL();
	void FetchArtistInfoL();
	TBool FetchImageL(const TDesC8& aData);
	void FetchImageL(TState aState, const TDesC8& aImageLocation);
	void SaveAlbumArtL(const TDesC8& aData);
	TBool OkToDownloadAlbumArt() const;
	
private:
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
private:
	void DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);
	
private:
	// track details
	CMobblerString* iArtist;
	CMobblerString* iTitle;
	CMobblerString* iAlbum;
	//CMobblerString* iMbAlbumId;
	CMobblerString* iMbTrackId;
	TInt iTrackNumber;
	TTime iStartTimeUTC;
	TTimeIntervalSeconds iTotalPlayed;
	TBool iTrackPlaying;
	TTimeIntervalSeconds iTrackLength;
	TTimeIntervalSeconds iInitialPlaybackPosition;
	TTimeIntervalSeconds iPlaybackPosition;
	TBool iLove;
	
	// album art
	HBufC8* iImage;
	CMobblerBitmap* iAlbumArt;
	TBool iTriedButCouldntFindAlbumArt;
	TBool iUsingArtistImage;
	
	// mp3 location
	HBufC8* iMp3Location;
	HBufC8* iRadioAuth;
	HBufC* iPath;
	
	TBool iScrobbled;
	
	TInt iRefCount;
	
	TInt iDataSize;
	TInt iBuffered;
	
	TState iState;
	};
	
#endif // __MOBBLERTRACK_H__

// End of file
