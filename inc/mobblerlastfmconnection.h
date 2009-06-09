/*
mobblerlastfmconnection.h

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

#ifndef __MOBBLERLASTFMCONNECTION_H__
#define __MOBBLERLASTFMCONNECTION_H__

#include <e32base.h>
#include <es_sock.h>
#include <http/mhttptransactioncallback.h> 
#include <http/rhttpsession.h>

#include "mobblerlastfmerror.h"

_LIT(KLogFile, "c:\\Data\\Mobbler\\.scrobbler.log");

class CMobblerString;
class CMobblerTrack;
class CMobblerTransaction;
class MMobblerFlatDataObserver;
class MMobblerLastFMConnectionObserver;
class MMobblerSegDataObserver;

class MMobblerConnectionStateObserver
	{
public:
	virtual void HandleConnectionStateChangedL() = 0;
	};

class CMobblerLastFMConnection : public CActive, public MHTTPTransactionCallback
	{
public:
	friend class CMobblerTransaction;
public:
	enum TError
		{
		EErrorNone,
		EErrorCancel,
		EErrorStop,
		EErrorHandshake,
		EErrorFailed
		};
	
	enum TRadioStation
		{
		EUnknown,
		EPersonal,
		ERecommendations,
		ENeighbourhood,
		ELovedTracks,
		EPlaylist,
		EArtist,
		ETag
		};
	
	enum TMode
		{
		EOffline,
		EOnline
		};
	
	enum TState
		{
		ENone,
		EConnecting,
		EHandshaking
		};
	
public:
	static CMobblerLastFMConnection* NewL(MMobblerLastFMConnectionObserver& aObserver, const TDesC& aUsername, const TDesC& aPassword, TUint32 aIapID);
	~CMobblerLastFMConnection();
	
	void SetDetailsL(const TDesC& aUsername, const TDesC& aPassword);
	void SetModeL(TMode aMode);
	TMode Mode() const;
	TState State() const;
	
	void SetIapIDL(TUint32 aIadID);
	TUint32 IapID() const;
	
	// state observers
	void AddStateChangeObserverL(MMobblerConnectionStateObserver* aObserver);
	void RemoveStateChangeObserver(MMobblerConnectionStateObserver* aObserver);
	
	// Updates
	void CheckForUpdateL(MMobblerFlatDataObserver& aObserver);
	
	// Scrobbling methods
	void TrackStartedL(CMobblerTrack* aCurrentTrack);
	void TrackStoppedL();
	
	// Radio APIs
	void SelectStationL(MMobblerFlatDataObserver* aObserver, TRadioStation aRadioStation, const TDesC8& aRadioText);
	void RequestPlaylistL(MMobblerFlatDataObserver* aObserver);
	
	void RequestMp3L(MMobblerSegDataObserver& aObserver, CMobblerTrack* aTrack);
	void RadioStop();
	
	void RequestImageL(MMobblerFlatDataObserver* aObserver, const TDesC8& aImageLocation);
	void CancelTransaction(MMobblerFlatDataObserver* aObserver);
	
	// Web services APIs
	void WebServicesCallL(const TDesC8& aClass, const TDesC8& aMethod, const TDesC8& aText, MMobblerFlatDataObserver& aObserver);
	
	void ShoutL(const TDesC8& aClass, const TDesC8& aArgument, const TDesC8& aMessage);
	
	void TrackLoveL(const TDesC8& aArtist, const TDesC8& aTrack);
	void TrackBanL(const TDesC8& aArtist, const TDesC8& aTrack);
	
	void TrackShareL(const TDesC8& aUser, const TDesC8& aArtist, const TDesC8& aTrack, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	void ArtistShareL(const TDesC8& aUser, const TDesC8& aArtist, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	void EventShareL(const TDesC8& aUser, const TDesC8& aEventId, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver);
	
	void RecommendedArtistsL(MMobblerFlatDataObserver& aObserver);
	void RecommendedEventsL(MMobblerFlatDataObserver& aObserver);
	
	void RecentTracksL(const TDesC8& aUser, MMobblerFlatDataObserver& aObserver);
	void SimilarTracksL(const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	
	void SimilarArtistsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver);
	void ArtistGetImageL(const TDesC& aArtist, MMobblerFlatDataObserver& aObserver);
	void ArtistGetTagsL(const TDesC& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void AlbumGetInfoL(const TDesC8& aMbId, MMobblerFlatDataObserver& aObserver);
	void AlbumGetInfoL(const TDesC& aAlbum, const TDesC& aArtist, MMobblerFlatDataObserver& aObserver);
	
	void PlaylistCreateL(const TDesC& aTitle, const TDesC& aDescription, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchUserL(const TDesC8& aPlaylistId, MMobblerFlatDataObserver& aObserver);
	void PlaylistFetchAlbumL(const TDesC8& aAlbumId, MMobblerFlatDataObserver& aObserver);
	void PlaylistAddTrackL(const TDesC8& aPlaylistId, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver);
	
	TBool ExportQueueToLogFileL();
	
	TBool ScrobblingOn() { return iScrobblingOn; }
	void ToggleScrobblingL();
	
	void LoadCurrentTrackL();
	void SaveCurrentTrackL();
	
private:
	void RunL();
	void DoCancel();
	
private: // from MHTTPTransactionCallback
	void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
	
private: // from MMobblerTransactionObserver
	void TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse);
	void TransactionCompleteL(CMobblerTransaction* aTransaction);
	void TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aStatus, TInt aStatusCode);
	
private:
	void DoNowPlayingL();
	TBool DoSubmitL();
	
	void ChangeStateL(TState aState);
	void DoSetModeL(TMode aMode);
	
	// handshaking
	void AuthenticateL();
	
	void ScrobbleHandshakeL();
	void WebServicesHandshakeL();
#ifdef BETA_BUILD
	void BetaHandshakeL();
#endif
	
	void HandleHandshakeErrorL(CMobblerLastFMError* aError);
	
private:
	void ConstructL(const TDesC& aUsername, const TDesC& aPassword);
	CMobblerLastFMConnection(MMobblerLastFMConnectionObserver& aObserver, TUint32 aIapID);
	
private:  // utilities
	void CreateAuthTokenL(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp);
	void StripOutTabs(TDes8& aString);
	
	// track queue methods
	void LoadTrackQueueL();
	void SaveTrackQueueL();

	void ConnectL();
	void Disconnect();
	
	TBool Connected();
	
	CMobblerTransaction* CreateRequestPlaylistTransactionL(MMobblerFlatDataObserver* aObserver);
	void AppendAndSubmitTransactionL(CMobblerTransaction* aTransaction);
	void CloseTransactionsL(TBool aCloseTransactionArray);
	
	void DeleteCurrentTrackFile();

private:
	RHTTPSession iHTTPSession;
	RSocketServ iSocketServ;
	RConnection iConnection;
	
	TUint32 iIapID;
	TUint32 iCurrentIapID;
	
	// authentication transactions
	CMobblerTransaction* iHandshakeTransaction;
	CMobblerTransaction* iWebServicesHandshakeTransaction;
#ifdef BETA_BUILD
	CMobblerTransaction* iBetaTestersTransaction;
	TBool iIsBetaTester;
#endif
	
	// scrobble transactions
	CMobblerTransaction* iNowPlayingTransaction;
	CMobblerTransaction* iSubmitTransaction;
	
	// audio transaction
	RHTTPTransaction iRadioAudioTransaction;
	MMobblerSegDataObserver* iTrackDownloadObserver;
	
	RPointerArray<CMobblerTransaction> iTransactions;
	
	TBool iSubscriber;

	CMobblerString* iUsername;
	CMobblerString* iPassword;
	
	HBufC8* iScrobbleSessionID;
	
	HBufC8* iWebServicesSessionKey;
	
	HBufC8* iNowPlayingURL;
	HBufC8* iSubmitURL;

	MMobblerLastFMConnectionObserver& iObserver;
	
	CMobblerTrack* iCurrentTrack;
	
	RPointerArray<CMobblerTrack> iTrackQueue;
	
	TMode iMode;
	TState iState;
	TBool iAuthenticated;
	
	TBool iScrobblingOn;
	TBool iCurrentTrackSaved;
	
	RPointerArray<MMobblerConnectionStateObserver> iStateChangeObservers;
	};

#endif // __MOBBLERLASTFMCONNECTION_H__

// End of file
