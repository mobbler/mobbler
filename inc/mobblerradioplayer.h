/*
mobblerradioplayer.h

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

#ifndef __MOBBLERRADIOPLAYER_H__
#define __MOBBLERRADIOPLAYER_H__

#include <e32base.h>

#include <s32file.h>
#include <mda\common\audio.h>
#include <Mda\Client\Utility.h>
#include <Mda\Common\Resource.h>
#include <MdaAudioOutputStream.h>
//#include <mmf\server\MmfCodec.h>
#include <mmf\server\mmfdatabuffer.h>

#include "mobblerlastfmconnection.h"

class CDesC8Array;
class CMobblerRadioPlaylistParser;
class CMobblerRadioPlaylist;
class CMobblerTrack;
class CMobblerString;

class MMobblerRadioPlayer
	{
public:
	virtual void SetPlaylistL(CMobblerRadioPlaylist* aPlaylist) = 0;
	//virtual void SetAlbumArtL(const TDesC8& aData) = 0;
	virtual void WriteL(const TDesC8& aData, TInt aTotalDataSize) = 0;
	virtual void TrackDownloadCompleteL() = 0;
	virtual void NextTrackL() = 0;
	virtual void Stop() = 0;
	};

class CMobblerRadioPlayer : public CBase,
							public MMdaAudioOutputStreamCallback,
							public MMobblerRadioPlayer

	{
public:
	static CMobblerRadioPlayer* NewL(CMobblerLastFMConnection& aLastFMConnection);
	~CMobblerRadioPlayer();
	
	TInt StartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioText);
	
	CMobblerTrack* CurrentTrack();
	
	void NextTrackL();
	void VolumeUp();
	void VolumeDown();
	
	TInt Volume() const;
	TInt MaxVolume() const;
	
	void Stop();
	
	const CMobblerString& Station() const;
	
private:
	CMobblerRadioPlayer(CMobblerLastFMConnection& aSubmitter);
	void ConstructL();
	
	void SubmitCurrentTrackL();
	
	void DoStop();
	
private:
	void DialogDismissedL(TInt aButtonId);

private: // MMobblerRadioPlayer	
	void SetPlaylistL(CMobblerRadioPlaylist* aPlaylist);
	void WriteL(const TDesC8& aData, TInt aTotalDataSize);
	void TrackDownloadCompleteL();
	
private: // from MMdaAudioOutputStreamCallback
	void MaoscOpenComplete(TInt aError);
	void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
	void MaoscPlayComplete(TInt aError);
	

private:
	RPointerArray<HBufC8> iBuffer;

	TBool iOpen;
	TBool iPlaying;
	TBool iAlreadyOpened;

	TBool iTrackDownloading;
	
	TInt iBufferOffset;

	CMdaAudioOutputStream* iMdaAudioOutputStream;
	TMdaAudioDataSettings iMdaAudioDataSettings;

	TInt iCurrentTrack;
	CMobblerRadioPlaylist* iPlaylist;
	
	CMobblerLastFMConnection& iLastFMConnection;
	};

#endif // __MOBBLERRADIOPLAYER_H__
