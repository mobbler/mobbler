/*
mobblerlastfmconnection.cpp

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

#include <centralrepository.h>
#include <chttpformencoder.h>
#include <coemain.h>
#include <commdbconnpref.h> 
#include <httperr.h>
#include <httpstringconstants.h>
#include <ProfileEngineSDKCRKeys.h>
#include <s32file.h>

#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerdestinationsinterface.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblertransaction.h"
#include "mobblerutility.h"
#include "mobblerwebservicesquery.h"

// the scheme of the Last.fm handshake
_LIT8(KScheme, "http");

// the Last.fm URL to send the handshake to
_LIT8(KScrobbleHost, "post.audioscrobbler.com");
_LIT8(KWebServicesHost, "ws.audioscrobbler.com");

// The format for the handshake URL 
_LIT8(KScrobbleQuery, "hs=true&p=1.2&c=mlr&v=0.1&u=%S&t=%d&a=%S");

_LIT8(KRadioStationPersonal, "lastfm://user/%S/library");
_LIT8(KRadioStationPlaylist, "lastfm://playlist/%S");
_LIT8(KRadioStationLoved, "lastfm://user/%S/loved");
_LIT8(KRadioStationArtist, "lastfm://artist/%S/similarartists");
_LIT8(KRadioStationTag, "lastfm://globaltags/%S");
_LIT8(KRadioStationNeighbours, "lastfm://user/%S/neighbours");
_LIT8(KRadioStationRecommended, "lastfm://user/%S/recommended");

#ifdef BETA_BUILD
#include "mobblerbeta.h"
#else
_LIT8(KLatesverFileLocation, "http://www.mobbler.co.uk/latestver.xml");
#endif

// The file name to store the queue of listened tracks
_LIT(KTracksFile, "c:track_queue.dat");
_LIT(KCurrentTrackFile, "c:current_track.dat");
_LIT8(KLogFileHeader, "#AUDIOSCROBBLER/1.1\n#TZ/UTC\n#CLIENT/Mobbler ");
_LIT8(KLogFileListenedRating, "L");
_LIT8(KLogFileFieldSeperator, "\t");
_LIT8(KLogFileEndOfLine, "\n");

// Last.fm can accept up to this many track in one submission
const TInt KMaxSubmitTracks(50);

const TInt KOfflineProfileId(5);

CMobblerLastFMConnection* CMobblerLastFMConnection::NewL(MMobblerLastFMConnectionObserver& aObserver, 
															const TDesC& aUsername, 
															const TDesC& aPassword, 
															TUint32 aIapID, 
															TInt aBitRate)
	{
	CMobblerLastFMConnection* self = new(ELeave) CMobblerLastFMConnection(aObserver, aIapID, aBitRate);
	CleanupStack::PushL(self);
	self->ConstructL(aUsername, aPassword);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLastFMConnection::CMobblerLastFMConnection(MMobblerLastFMConnectionObserver& aObserver, TUint32 aIapID, TInt aBitRate)
	:CActive(CActive::EPriorityStandard), iIapID(aIapID), iObserver(aObserver), iBitRate(aBitRate)
	{
	CActiveScheduler::Add(this);
	}

CMobblerLastFMConnection::~CMobblerLastFMConnection()
	{
	Cancel();
	
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		}
	
	const TInt KTrackQueueCount(iTrackQueue.Count());
	for (TInt i(0) ; i < KTrackQueueCount ; ++i)
		{
		iTrackQueue[i]->Release();
		}
	iTrackQueue.Reset();
	
	delete iMp3Location;
	iRadioAudioTransaction.Close();
	
	iStateChangeObservers.Close();
	
	CloseTransactionsL(ETrue);
	
	delete iUsername;
	delete iPassword;
	
	delete iScrobbleSessionID;
	delete iNowPlayingURL;
	delete iSubmitURL;
	
	delete iWebServicesSessionKey;
	
	iHTTPSession.Close();
	iConnection.Close();
	iSocketServ.Close();
	}

void CMobblerLastFMConnection::ConstructL(const TDesC& aUsername, const TDesC& aPassword)
	{
	SetDetailsL(aUsername, aPassword);
	LoadTrackQueueL();
	
	User::LeaveIfError(iSocketServ.Connect());
	
	iScrobblingOn = ETrue;
	}

void CMobblerLastFMConnection::DoSetModeL(TMode aMode)
	{
	iMode = aMode;
	
	// notify the state change observers when we change mode too
	const TInt KObserverCount(iStateChangeObservers.Count());
	for (TInt i(0) ; i < KObserverCount ; ++i)
		{
		iStateChangeObservers[i]->HandleConnectionStateChangedL();
		}
	}

void CMobblerLastFMConnection::SetIapIDL(TUint32 aIapID)
	{
	if (aIapID != iIapID)
		{
		iIapID = aIapID;
		
		if (((iMode == EOnline || iState == EHandshaking) && iCurrentIapID != iIapID)
				|| iState == EConnecting)
			{
			// We are either online/handshaking and the new iap is different to the old one
			// or we are trying to connect to a new iap
			// so we should start connecting again
			ConnectL();
			}
		}
	}

void CMobblerLastFMConnection::SetBitRateL(TInt aBitRate)
	{
	iBitRate = aBitRate;
	}

TUint32 CMobblerLastFMConnection::IapID() const
	{
	return iIapID;
	}

void CMobblerLastFMConnection::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	if (!iUsername
			|| iUsername && iUsername->String().CompareF(aUsername) != 0
			|| !iPassword
			|| iPassword && iPassword->String().Compare(aPassword) != 0)
		{
		// There is either no username or password set
		// or there is a new user or password
		
		HBufC* usernameLower(HBufC::NewLC(aUsername.Length()));
		usernameLower->Des().Copy(aUsername);
		usernameLower->Des().LowerCase();
		CMobblerString* tempUsername(CMobblerString::NewL(*usernameLower));
		CleanupStack::PushL(tempUsername);
		CMobblerString* tempPassword(CMobblerString::NewL(aPassword));
		CleanupStack::Pop(tempUsername);
		delete iUsername;
		delete iPassword;
		iUsername = tempUsername;
		iPassword = tempPassword;
		
		CleanupStack::PopAndDestroy(usernameLower);
		
		if (iMode == EOnline)
			{
			// We are in online mode so we should
			// handshake with Last.fm again
			AuthenticateL();
			}
		}
	}

void CMobblerLastFMConnection::SetModeL(TMode aMode)
	{
	if (aMode == EOnline)
		{
		// We are being asked to switch to online mode
		
		if (iMode != EOnline &&
				iState != EConnecting && iState != EHandshaking)
			{
			// We are not already in online mode and we are neither
			// connecting nor handshaking, so start the connecting process
			
			ConnectL();
			}
		}
	else if (aMode == EOffline)
		{
		Disconnect();
		}
	
	DoSetModeL(aMode);
	}

CMobblerLastFMConnection::TMode CMobblerLastFMConnection::Mode() const
	{
	return iMode;
	}

CMobblerLastFMConnection::TState CMobblerLastFMConnection::State() const
	{
	return iState;
	}

void CMobblerLastFMConnection::ChangeStateL(TState aState)
	{
	if (iState != aState)
		{
		iState = aState;
		static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
		
		// notify the observers
		const TInt KObserverCount(iStateChangeObservers.Count());
		for (TInt i(0); i < KObserverCount; ++i)
			{
			iStateChangeObservers[i]->HandleConnectionStateChangedL();
			}
		}
	}

void CMobblerLastFMConnection::AddStateChangeObserverL(MMobblerConnectionStateObserver* aObserver)
	{
	iStateChangeObservers.InsertInAddressOrderL(aObserver);
	}

void CMobblerLastFMConnection::RemoveStateChangeObserver(MMobblerConnectionStateObserver* aObserver)
	{
	TInt pos(iStateChangeObservers.FindInAddressOrder(aObserver));
	if (pos != KErrNotFound)
		{
		iStateChangeObservers.Remove(pos);
		}
	}

void CMobblerLastFMConnection::PreferredCarrierAvailable()
	{
	CloseTransactionsL(EFalse);
	iHTTPSession.Close();
	}

void CMobblerLastFMConnection::NewCarrierActive()
	{
	User::LeaveIfError(iConnection.GetIntSetting(_L("IAP\\Id"), iCurrentIapID));
	
	iHTTPSession.OpenL();
	
	RStringPool strP(iHTTPSession.StringPool());
	RHTTPConnectionInfo connInfo(iHTTPSession.ConnectionInfo());
	connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), THTTPHdrVal(iSocketServ.Handle()));
	TInt connPtr(REINTERPRET_CAST(TInt, &iConnection));
	connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), THTTPHdrVal(connPtr));
	
	
	// submit any request that do not require authentication
	TInt KTransactionCount(iTransactions.Count());
	for (TInt i(0) ; i < KTransactionCount ; ++i)
		{
		if (!iTransactions[i]->RequiresAuthentication())
			{
			iTransactions[i]->SubmitL();
			}
		}
	
	if (iMp3Location)
		{
		// An mp3 had been requested after we had lost connection
		// so ask for it again now
		RequestMp3L(*iTrackDownloadObserver, *iMp3Location);
		}
	}
	
void CMobblerLastFMConnection::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		CMobblerDestinationsInterface* destinations(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->Destinations());
		
		if (destinations)
			{
			// Register for mobility: this should connect us
			// to better networks when they become avaliable 
			
			// TODO: leave this out for now as it is causing problems
			//destinations->RegisterMobilityL(iConnection, this);
			}
		
		User::LeaveIfError(iConnection.GetIntSetting(_L("IAP\\Id"), iCurrentIapID));
		
		iHTTPSession.Close();
		iHTTPSession.OpenL();
	
		RStringPool strP(iHTTPSession.StringPool());
		RHTTPConnectionInfo connInfo(iHTTPSession.ConnectionInfo());
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), THTTPHdrVal(iSocketServ.Handle()));
		TInt connPtr(REINTERPRET_CAST(TInt, &iConnection));
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), THTTPHdrVal(connPtr));
		
		// submit any request that does not require authentication
		TInt KTransactionCount(iTransactions.Count());
		for (TInt i(0) ; i < KTransactionCount ; ++i)
			{
			if (!iTransactions[i]->RequiresAuthentication())
				{
				iTransactions[i]->SubmitL();
				}
			}
		
		if (iMp3Location)
			{
			// An mp3 had been requested after we had lost connection
			// so ask for it again now
			RequestMp3L(*iTrackDownloadObserver, *iMp3Location);
			}
		
		// Authenticate now so that API calls that require it will be submitted.
		// This also means that any tracks in the queue will be submitted.
		AuthenticateL();
		}
	else
		{
		ChangeStateL(ENone);
		
		iObserver.HandleConnectCompleteL(iStatus.Int());
		
		CloseTransactionsL(ETrue);
		}
	
	delete iMp3Location;
	iMp3Location = NULL;
	}

void CMobblerLastFMConnection::DoCancel()
	{
	iConnection.Close();
	}

TBool CMobblerLastFMConnection::Connected()
	{
	TBool connected(EFalse);
	
	if (iConnection.SubSessionHandle() != 0)
		{
		TNifProgress nifProgress;
		iConnection.Progress(nifProgress);
		connected = (nifProgress.iStage == KLinkLayerOpen);
		}
	
	return connected;
	}

void CMobblerLastFMConnection::ConnectL()
	{
	Cancel();
	Disconnect();
	ChangeStateL(EConnecting);
	
	User::LeaveIfError(iConnection.Open(iSocketServ));
	
	TConnPref* prefs;
	TCommDbConnPref dbConnPrefs;
	TConnSnapPref snapPrefs;
	
	if (iIapID == 0)
		{
		prefs = &dbConnPrefs;
		
		// This means the users has selected to always be asked
		// which access point they want to use
		dbConnPrefs.SetDialogPreference(ECommDbDialogPrefPrompt);
		
		// Filter out operator APs when the phone profile is offline
		TInt activeProfileId;
		CRepository* repository = CRepository::NewL(KCRUidProfileEngine);
		repository->Get(KProEngActiveProfile, activeProfileId);
		delete repository;
		
		if (activeProfileId == KOfflineProfileId)
			{
			dbConnPrefs.SetBearerSet(ECommDbBearerWLAN);
			}
		}
	else
		{
		if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->Destinations())
			{
			// We are using destinations so use the TCommSnapPref
			prefs = &snapPrefs;
			snapPrefs.SetSnap(iIapID);
			}
		else
			{
			prefs = &dbConnPrefs;
			dbConnPrefs.SetIapId(iIapID);
			dbConnPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
			}
		}
	
	iConnection.Start(*prefs, iStatus);
	
	SetActive();
	}

void CMobblerLastFMConnection::AuthenticateL()
	{
	iAuthenticated = EFalse;
	
	// Handshake with Last.fm
	ChangeStateL(EHandshaking);
	ScrobbleHandshakeL();
	WebServicesHandshakeL();
#ifdef BETA_BUILD
	BetaHandshakeL();
#endif
	}

void CMobblerLastFMConnection::WebServicesHandshakeL()
	{
	// start the web services authentications

	delete iWebServicesSessionKey;
	iWebServicesSessionKey = NULL;
	
	HBufC8* passwordHash(MobblerUtility::MD5LC(iPassword->String8()));
	HBufC8* usernameAndPasswordHash(HBufC8::NewLC(passwordHash->Length() + iUsername->String8().Length()));
	usernameAndPasswordHash->Des().Copy(iUsername->String8());
	usernameAndPasswordHash->Des().Append(*passwordHash);
	
	HBufC8* authToken(MobblerUtility::MD5LC(*usernameAndPasswordHash));
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("auth.getMobileSession")));
	query->AddFieldL(_L8("authToken"), *authToken);
	query->AddFieldL(_L8("username"), iUsername->String8());
	HBufC8* queryText(query->GetQueryAuthLC());
	
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	uri->SetComponentL(*queryText, EUriQuery);
	
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = CMobblerTransaction::NewL(*this, uri);
	CleanupStack::Pop(uri);
	iWebServicesHandshakeTransaction->SubmitL();
	CleanupStack::PopAndDestroy(5, passwordHash);
	}

#ifdef BETA_BUILD
void CMobblerLastFMConnection::BetaHandshakeL()
	{
	// Start the web services authentications
	TUriParser8 uriParser;
	uriParser.Parse(KBetaTestersFileLocation);
	CUri8* uri(CUri8::NewLC(uriParser));
	
	delete iBetaTestersTransaction;
	iBetaTestersTransaction = CMobblerTransaction::NewL(*this, uri);
	CleanupStack::Pop(uri);
	iBetaTestersTransaction->SubmitL();
	}
#endif

void CMobblerLastFMConnection::CheckForUpdateL(MMobblerFlatDataObserver& aObserver)
	{
	TUriParser8 uriParser;
	uriParser.Parse(KLatesverFileLocation);
	CUri8* uri(CUri8::NewLC(uriParser));
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	CleanupStack::Pop(uri);
	transaction->SetFlatDataObserver(&aObserver);
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::TrackLoveL(const TDesC8& aArtist, const TDesC8& aTrack)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("track.love")));
	query->AddFieldL(_L8("track"), aTrack);
	query->AddFieldL(_L8("artist"), aArtist);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::PlaylistCreateL(const TDesC& aTitle, const TDesC& aDescription, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("playlist.create")));
	
	CMobblerString* title(CMobblerString::NewL(aTitle));
	CleanupStack::PushL(title);
	query->AddFieldL(_L8("title"), title->String8());
	CleanupStack::PopAndDestroy(title);
	
	CMobblerString* description(CMobblerString::NewL(aDescription));
	CleanupStack::PushL(description);
	query->AddFieldL(_L8("description"), description->String8());
	CleanupStack::PopAndDestroy(description);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::PlaylistFetchUserL(const TDesC8& aPlaylistId, MMobblerFlatDataObserver& aObserver)
	{
	_LIT8(KUserPlaylistFormat, "lastfm://playlist/%S");
	
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("playlist.fetch")));
	
	HBufC8* playlistURL(HBufC8::NewLC(KUserPlaylistFormat().Length() + aPlaylistId.Length()));
	playlistURL->Des().Format(KUserPlaylistFormat, &aPlaylistId);
	query->AddFieldL(_L8("playlistURL"), *playlistURL);
	CleanupStack::PopAndDestroy(playlistURL);
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::PlaylistFetchAlbumL(const TDesC8& aAlbumId, MMobblerFlatDataObserver& aObserver)
	{
	_LIT8(KUserPlaylistFormat, "lastfm://playlist/album/%S");
	
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("playlist.fetch")));
	
	HBufC8* playlistURL(HBufC8::NewLC(KUserPlaylistFormat().Length() + aAlbumId.Length()));
	playlistURL->Des().Format(KUserPlaylistFormat, &aAlbumId);
	query->AddFieldL(_L8("playlistURL"), *playlistURL);
	CleanupStack::PopAndDestroy(playlistURL);
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::PlaylistAddTrackL(const TDesC8& aPlaylistId, const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("playlist.addtrack")));
	query->AddFieldL(_L8("playlistID"), aPlaylistId);	
	query->AddFieldL(_L8("track"), aTrack);
	query->AddFieldL(_L8("artist"), aArtist);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::ShoutL(const TDesC8& aClass, const TDesC8& aArgument, const TDesC8& aMessage)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	TBuf8<255> signature;
	signature.Format(_L8("%S.shout"), &aClass);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(signature));
	query->AddFieldL(aClass, aArgument);
	query->AddFieldL(_L8("message"), aMessage);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RecommendedArtistsL(MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("user.getrecommendedartists")));
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RecommendedEventsL(MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("user.getrecommendedevents")));
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void  CMobblerLastFMConnection::TrackBanL(const TDesC8& aArtist, const TDesC8& aTrack)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("track.ban")));
	query->AddFieldL(_L8("track"), aTrack);
	query->AddFieldL(_L8("artist"), aArtist);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::SimilarArtistsL(const TDesC8& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("artist.getsimilar")));
	query->AddFieldL(_L8("artist"), *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::SimilarTracksL(const TDesC8& aArtist, const TDesC8& aTrack, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("track.getsimilar")));
	query->AddFieldL(_L8("artist"), *MobblerUtility::URLEncodeLC(aArtist));
	query->AddFieldL(_L8("track"), *MobblerUtility::URLEncodeLC(aTrack));
	CleanupStack::PopAndDestroy(2); // URLEncodeLC
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	CleanupStack::PopAndDestroy(query); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RecentTracksL(const TDesC8& aUser, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("user.getrecenttracks")));
	query->AddFieldL(_L8("user"), *MobblerUtility::URLEncodeLC(aUser));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::ArtistGetImageL(const TDesC& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("artist.getimages")));
	query->AddFieldL(_L8("artist"), *MobblerUtility::URLEncodeLC(aArtist));
	query->AddFieldL(_L8("limit"), _L8("1"));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::ArtistGetTagsL(const TDesC& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("artist.gettoptags")));
	query->AddFieldL(_L8("artist"), *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aTrack.Artist().String8())
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::AlbumGetInfoL(const TDesC& aAlbum, const TDesC& aArtist, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("album.getinfo")));
	
	query->AddFieldL(_L8("artist"), *MobblerUtility::URLEncodeLC(aArtist));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aArtist)
	
	query->AddFieldL(_L8("album"), *MobblerUtility::URLEncodeLC(aAlbum));
	CleanupStack::PopAndDestroy(); // *MobblerUtility::URLEncodeLC(aAlbum)
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // *query->GetQueryLC()

	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::AlbumGetInfoL(const TDesC8& aMbId, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewL());
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("album.getinfo")));
	query->AddFieldL(_L8("mbid"), aMbId);
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy();
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::WebServicesCallL(const TDesC8& aClass, const TDesC8& aMethod, const TDesC8& aText, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	_LIT8(KApiSignature, "%S.%S");
	TBuf8<255> apiSignature;
	apiSignature.Format(KApiSignature, &aClass, &aMethod);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(apiSignature));
	
	if (aClass.Compare(_L8("user")) == 0)
		{
		if (aText.Length() == 0)
			{
			query->AddFieldL(_L8("user"), iUsername->String8());
			}
		else
			{
			query->AddFieldL(_L8("user"), aText);
			}
		}
	else
		{
		query->AddFieldL(aClass, *MobblerUtility::URLEncodeLC(aText));
		CleanupStack::PopAndDestroy(); // URLEncodeLC
		}
	
	uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
	CleanupStack::PopAndDestroy(); // GetQueryLC
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::PopAndDestroy(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::TrackShareL(const TDesC8& aUserName, const TDesC8& aArtist, const TDesC8& aTrack, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("track.share")));
	query->AddFieldL(_L8("artist"), aArtist);
	query->AddFieldL(_L8("track"), aTrack);
	query->AddFieldL(_L8("recipient"), aUserName);
	
	const TDesC& tagline(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_SHARE_TAGLINE));
	HBufC8* message(HBufC8::NewLC(aMessage.Length() + tagline.Length()));
	message->Des().Append(aMessage);
	message->Des().Append(tagline);
	query->AddFieldL(_L8("message"), *message);
	CleanupStack::PopAndDestroy(message);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::ArtistShareL(const TDesC8& aUserName, const TDesC8& aArtist, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("artist.share")));
	query->AddFieldL(_L8("artist"), aArtist);
	query->AddFieldL(_L8("recipient"), aUserName);
	
	const TDesC& tagline(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_SHARE_TAGLINE));
	HBufC8* message(HBufC8::NewLC(aMessage.Length() + tagline.Length()));
	message->Des().Append(aMessage);
	message->Des().Append(tagline);
	query->AddFieldL(_L8("message"), *message);
	CleanupStack::PopAndDestroy(message);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::EventShareL(const TDesC8& aUserName, const TDesC8& aEventId, const TDesC8& aMessage, MMobblerFlatDataObserver& aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("event.share")));
	query->AddFieldL(_L8("event"), aEventId);
	query->AddFieldL(_L8("recipient"), aUserName);
	query->AddFieldL(_L8("message"), aMessage);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(&aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RadioStop()
	{
	if (iTrackDownloadObserver)
		{
		TRAP_IGNORE(iTrackDownloadObserver->DataCompleteL(CMobblerLastFMConnection::EErrorStop, KErrNone, KNullDesC8));
		iTrackDownloadObserver = NULL;
		}
	iRadioAudioTransaction.Close();
	}

void CMobblerLastFMConnection::SelectStationL(MMobblerFlatDataObserver* aObserver, TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	// Set up the Last.fm formatted station URI
	HBufC8* radioURL(HBufC8::NewLC(255));
	HBufC8* text(NULL);
	
	if (aRadioText.Length() == 0)
		{
		// no text supplied so use the username
		text = iUsername->String8().AllocLC();
		}
	else
		{
		// text supplied so use it
		text = HBufC8::NewLC(aRadioText.Length());
		text->Des().Copy(aRadioText);
		}
	
	TPtr8 textPtr(text->Des());
	
	switch (aRadioStation)
		{
		case EPersonal:	radioURL->Des().AppendFormat(KRadioStationPersonal, &textPtr); break;
		case EPlaylist: radioURL->Des().AppendFormat(KRadioStationPlaylist, &textPtr); break;
		case ERecommendations: radioURL->Des().AppendFormat(KRadioStationRecommended, &textPtr); break;
		case ENeighbourhood: radioURL->Des().AppendFormat(KRadioStationNeighbours, &textPtr); break;
		case ELovedTracks: radioURL->Des().AppendFormat(KRadioStationLoved, &textPtr); break;
		case EArtist: radioURL->Des().AppendFormat(KRadioStationArtist, &textPtr); break;
		case ETag: radioURL->Des().AppendFormat(KRadioStationTag, &textPtr); break;
		default: break;
		}
	
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("radio.tune")));
	query->AddFieldL(_L8("lang"), MobblerUtility::LanguageL());
	query->AddFieldL(_L8("station"), *radioURL);
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	CleanupStack::PopAndDestroy(text);
	CleanupStack::PopAndDestroy(radioURL);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RequestPlaylistL(MMobblerFlatDataObserver* aObserver)
	{
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	
	CMobblerWebServicesQuery* query(CMobblerWebServicesQuery::NewLC(_L8("radio.getPlaylist")));
	
	//query->AddFieldL(_L8("rtp"), _L8("?"));
	
	// Always ask for the mp3 to be downloaded at twice the speed that it plays at.
	// Should improve battery life by downloading for less time.
	query->AddFieldL(_L8("speed_multiplier"), _L8("2.0"));
	
	switch (iBitRate)
		{
		case 0:
			query->AddFieldL(_L8("bitrate"), _L8("64"));
			break;
		case 1:
			query->AddFieldL(_L8("bitrate"), _L8("128"));
			break;
		default:
			// TODO: should panic
			break;
		}
	
	CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, ETrue, uri, query));
	transaction->SetFlatDataObserver(aObserver);
	
	CleanupStack::Pop(query);
	CleanupStack::Pop(uri);
	
	AppendAndSubmitTransactionL(transaction);
	}

void CMobblerLastFMConnection::RequestMp3L(MMobblerSegDataObserver& aObserver, const TDesC8& aMp3Location)
	{
	if (iMode == EOnline)
		{
		iTrackDownloadObserver = &aObserver;
		
		if (Connected())
			{
			// Request the mp3 data
			TUriParser8 urimp3Parser;
			urimp3Parser.Parse(aMp3Location);
			
			iRadioAudioTransaction.Close();
			iRadioAudioTransaction = iHTTPSession.OpenTransactionL(urimp3Parser, *this);
			
			RStringF mobbler(iHTTPSession.StringPool().OpenFStringL(_L8("mobbler")));
			iRadioAudioTransaction.Request().GetHeaderCollection().SetFieldL(iHTTPSession.StringPool().StringF(HTTP::EConnection, RHTTPSession::GetTable()), mobbler);
			mobbler.Close();
			
			iRadioAudioTransaction.SubmitL();
			}
		else if (iState != EConnecting && iState != EHandshaking)
			{
			iMp3Location = aMp3Location.AllocL();
			ConnectL();
			}
		}
	}

void CMobblerLastFMConnection::RequestImageL(MMobblerFlatDataObserver* aObserver, const TDesC8& aImageLocation)
	{
	if (aImageLocation.Compare(KNullDesC8) != 0 && Connected())
		{
		// Request the album art data
		TUriParser8 imageLocationParser;
		imageLocationParser.Parse(aImageLocation);
		CUri8* uri(CUri8::NewLC(imageLocationParser));
		CMobblerTransaction* transaction(CMobblerTransaction::NewL(*this, uri));
		CleanupStack::Pop(uri);
		transaction->SetFlatDataObserver(aObserver);
		AppendAndSubmitTransactionL(transaction);
		}
	}

void CMobblerLastFMConnection::CancelTransaction(MMobblerFlatDataObserver* aObserver)
	{
	// cancel all image downloads associated the observer
	for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
		{
		if (iTransactions[i]->FlatDataObserver() == aObserver)
			{
			delete iTransactions[i];
			iTransactions.Remove(i);
			}
		}
	}

void CMobblerLastFMConnection::Disconnect()
	{
	Cancel();
	
	ChangeStateL(ENone);
	
	CloseTransactionsL(EFalse);
	
	// delete strings for URLs etc
	delete iScrobbleSessionID;
	iScrobbleSessionID = NULL;
	delete iNowPlayingURL;
	iNowPlayingURL = NULL;
	delete iSubmitURL;
	iSubmitURL = NULL;
	
	iAuthenticated = EFalse;
	
	iHTTPSession.Close();
	iConnection.Close();
	}
	
void CMobblerLastFMConnection::DoNowPlayingL()
	{
	if (iCurrentTrack)
		{
		iObserver.HandleTrackNowPlayingL(*iCurrentTrack);
		
		if (iMode == EOnline && iAuthenticated)
			{
			// We must be in online mode and have recieved the now playing URL and session ID from Last.fm
			// before we try to submit and tracks
			
			// create the from that will be sent to Last.fm
			CHTTPFormEncoder* nowPlayingForm(CHTTPFormEncoder::NewL());
			CleanupStack::PushL(nowPlayingForm);
			
			nowPlayingForm->AddFieldL(_L8("s"), *iScrobbleSessionID);
			nowPlayingForm->AddFieldL(_L8("a"), iCurrentTrack->Artist().String8());
			nowPlayingForm->AddFieldL(_L8("t"), iCurrentTrack->Title().String8());
			nowPlayingForm->AddFieldL(_L8("b"), iCurrentTrack->Album().String8());
			TBuf8<10> trackLength;
			trackLength.AppendNum(iCurrentTrack->TrackLength().Int());
			nowPlayingForm->AddFieldL(_L8("l"), trackLength);
			if (iCurrentTrack->TrackNumber() != KErrUnknown)
				{
				TBuf8<10> trackNumber;
				trackNumber.AppendNum(iCurrentTrack->TrackNumber());
				nowPlayingForm->AddFieldL(_L8("n"), trackNumber);
				}
			else
				{
				nowPlayingForm->AddFieldL(_L8("n"), KNullDesC8);
				}
			nowPlayingForm->AddFieldL(_L8("m"), KNullDesC8);
			
			// get the URI
			TUriParser8 uriParser;
			uriParser.Parse(*iNowPlayingURL);
			CUri8* uri(CUri8::NewLC(uriParser));
			
			delete iNowPlayingTransaction;
			iNowPlayingTransaction = CMobblerTransaction::NewL(*this, ETrue, uri, nowPlayingForm);
			iNowPlayingTransaction->SubmitL();
			CleanupStack::Pop(2, nowPlayingForm);
			}
		}
	}

void CMobblerLastFMConnection::TrackStartedL(CMobblerTrack* aTrack)
	{
	if (iCurrentTrack)
		{
		//  There is already a current track so get rid of it
		iCurrentTrack->Release();
		}
	
	iCurrentTrack = aTrack;
	iCurrentTrack->Open();
	
	DoNowPlayingL();
	}
	
void CMobblerLastFMConnection::TrackStoppedL()
	{
	// Make sure that we haven't already tried to scrobble this track
	if (iCurrentTrack && !iCurrentTrack->Scrobbled())
		{
		iCurrentTrack->SetScrobbled();
		
		TTimeIntervalSeconds listenedFor(0);
		
		if (!iCurrentTrack->IsMusicPlayerTrack())
			{
			// It's a radio track so test the amount of continuous playback
			listenedFor = iCurrentTrack->PlaybackPosition();
			}
		else
			{
			// The current track is a music player app so test the amount of continuous playback
			listenedFor = iCurrentTrack->TotalPlayed().Int();
			
			if (iCurrentTrack->TrackPlaying())
				{
				TTimeIntervalSeconds lastPlayedSection(0);
				
				TTime now;
				now.UniversalTime();
				User::LeaveIfError(now.SecondsFrom(iCurrentTrack->StartTimeUTC(), lastPlayedSection));
				
				listenedFor = listenedFor.Int() + lastPlayedSection.Int();
				}
			}
		
		// Test if the track passes Last.fm's scrobble rules
		if (iScrobblingOn 
			&& listenedFor.Int() >= iCurrentTrack->ScrobbleDuration().Int()
			&& iCurrentTrack->TrackLength().Int() >= 30				// Track length is over 30 seconds
			&& iCurrentTrack->Artist().String().Length() > 0)		// Must have an artist name
			{
			// It passed, so notify and append it to the list
			iObserver.HandleTrackQueuedL(*iCurrentTrack);
			iTrackQueue.AppendL(iCurrentTrack);
			iCurrentTrack = NULL;
			}
		}
	
	// There is no longer a current track
	if (iCurrentTrack)
		{
		iCurrentTrack->Release();
		}
	
	iCurrentTrack = NULL;
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();

	// Save the track queue and try to do a submission
	DeleteCurrentTrackFile();
	SaveTrackQueueL();
	DoSubmitL();

	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->TrackStoppedL();
	}

TBool CMobblerLastFMConnection::DoSubmitL()
	{
	TBool submitting(EFalse);
	
	if (iMode == EOnline && iAuthenticated && !iSubmitTransaction)
		{
		// We are connected and not already submitting tracks 
		// so try to submit the tracks in the queue
		
		const TInt KSubmitTracksCount(iTrackQueue.Count());
		
		if (KSubmitTracksCount > 0)
			{
			CHTTPFormEncoder* submitForm(CHTTPFormEncoder::NewL());
			CleanupStack::PushL(submitForm);
			
			_LIT8(KS, "s");
			submitForm->AddFieldL(KS, *iScrobbleSessionID);
			
			for (TInt ii(0) ; ii < KSubmitTracksCount && ii < KMaxSubmitTracks ; ++ii)
				{
				_LIT8(KA, "a[%d]");
				_LIT8(KT, "t[%d]");
				_LIT8(KI, "i[%d]");
				_LIT8(KO, "o[%d]");
				_LIT8(KR, "r[%d]");
				_LIT8(KL, "l[%d]");
				_LIT8(KB, "b[%d]");
				_LIT8(KN, "n[%d]");
				_LIT8(KM, "m[%d]");
				
				TBuf8<6> a;
				a.AppendFormat(KA, ii);
				TBuf8<6> t;
				t.AppendFormat(KT, ii);
				TBuf8<6> i;
				i.AppendFormat(KI, ii);
				TBuf8<6> o;
				o.AppendFormat(KO, ii);
				TBuf8<6> r;
				r.AppendFormat(KR, ii);
				TBuf8<6> l;
				l.AppendFormat(KL, ii);
				TBuf8<6> b;
				b.AppendFormat(KB, ii);
				TBuf8<6> n;
				n.AppendFormat(KN, ii);
				TBuf8<6> m;
				m.AppendFormat(KM, ii);
				
				submitForm->AddFieldL(a, iTrackQueue[ii]->Artist().String8());
				submitForm->AddFieldL(t, iTrackQueue[ii]->Title().String8());
				submitForm->AddFieldL(b, iTrackQueue[ii]->Album().String8());
				
				TTimeIntervalSeconds unixTimeStamp;
				TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
				User::LeaveIfError(iTrackQueue[ii]->StartTimeUTC().SecondsFrom(epoch, unixTimeStamp));
				TBuf8<20> startTimeBuf;
				startTimeBuf.AppendNum(unixTimeStamp.Int());
				submitForm->AddFieldL(i, startTimeBuf);
				
				if (iTrackQueue[ii]->IsMusicPlayerTrack())
					{
					submitForm->AddFieldL(o, _L8("P"));
					}
				else
					{
					// this track was played by the radio player
					HBufC8* sourceValue(HBufC8::NewLC(iTrackQueue[ii]->RadioAuth().Length() + 1));
					sourceValue->Des().Append(_L8("L"));
					sourceValue->Des().Append(iTrackQueue[ii]->RadioAuth());
					submitForm->AddFieldL(o, *sourceValue);
					CleanupStack::PopAndDestroy(sourceValue);
					}
				
				HBufC8* love(HBufC8::NewLC(1));
				if (iTrackQueue[ii]->Love())
					{
					love->Des().Append(_L("L"));
					TrackLoveL(iTrackQueue[ii]->Artist().String8(), iTrackQueue[ii]->Title().String8());
					}
				
				submitForm->AddFieldL(r, *love);
				CleanupStack::PopAndDestroy(love);
				
				TBuf8<10> trackLength;
				trackLength.AppendNum(iTrackQueue[ii]->TrackLength().Int());
				submitForm->AddFieldL(l, trackLength);
				
				if (iTrackQueue[ii]->TrackNumber() != KErrUnknown)
					{
					TBuf8<10> trackNumber;
					trackNumber.AppendNum(iTrackQueue[ii]->TrackNumber());
					submitForm->AddFieldL(n, trackNumber);
					}
				else
					{
					submitForm->AddFieldL(n, KNullDesC8);
					}
				submitForm->AddFieldL(m, KNullDesC8);
				}
			
			// get the URI
			TUriParser8 uriParser;
			uriParser.Parse(*iSubmitURL);
			CUri8* uri(CUri8::NewLC(uriParser));
			delete iSubmitTransaction;
			iSubmitTransaction = CMobblerTransaction::NewL(*this, ETrue, uri, submitForm);
			iSubmitTransaction->SubmitL();
			CleanupStack::Pop(2, submitForm);
			
			submitting = ETrue;
			}
		}
	
	return submitting;
	}

void CMobblerLastFMConnection::HandleHandshakeErrorL(CMobblerLastFMError* aError)
	{
	if (!aError)
		{
		// The handshake was ok
		
		if (iScrobbleSessionID && iWebServicesSessionKey
#ifdef BETA_BUILD
				&& iIsBetaTester
#endif
				)
			{
			iAuthenticated = ETrue;
			
			// only notify the UI when we are fully connected
			iObserver.HandleConnectCompleteL(KErrNone);
			ChangeStateL(ENone);
			
			DoSubmitL();
			
			const TInt KTransactionCount(iTransactions.Count());
			for (TInt i(0) ; i < KTransactionCount ; ++i)
				{
				iTransactions[i]->SubmitL();
				}
			
			if (iTrackDownloadObserver)
				{
				// There is a track observer so this must be because
				// we failed to start downloading a track, but have now reconnected
				iTrackDownloadObserver->DataCompleteL(EErrorHandshake, KErrNone, KNullDesC8);
				iTrackDownloadObserver = NULL;
				}
			}
		}
	else
		{
		// There was an error with one of the handshakes
		iObserver.HandleLastFMErrorL(*aError);
		ChangeStateL(ENone);
		iWebServicesHandshakeTransaction->Cancel();
		iHandshakeTransaction->Cancel();
		
		// close all transactions that require authentication
		// but let other ones carry on
		for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
			{
			if (iTransactions[i]->RequiresAuthentication())
				{
				delete iTransactions[i];
				iTransactions.Remove(i);
				}
			}
		}
	}

void CMobblerLastFMConnection::AppendAndSubmitTransactionL(CMobblerTransaction* aTransaction)
	{
	iTransactions.AppendL(aTransaction);
	
	if (iMode != EOnline)
		{
		if (iState != EConnecting && iState != EHandshaking)
			{
			if (iObserver.GoOnlineL())
				{
				SetModeL(EOnline);
				}
			else
				{
				CloseTransactionsL(ETrue);
				}
			}
		}
	else if (iAuthenticated && Connected())
		{
		// we are online and we have authenticated
		aTransaction->SubmitL();
		}
	else if (Connected())
		{
		// we are connected, but not authenticated
		AuthenticateL();
		}
	else
		{
		// we are online, but authentication was unsucsessful
		// so retry the whole connection procedure
		if (iState != EConnecting && iState != EHandshaking)
			{
			// we are not already connecting
			ConnectL();
			}
		}
	}

void CMobblerLastFMConnection::CloseTransactionsL(TBool aCloseTransactionArray)
	{
	// close any ongoing transactions
	delete iHandshakeTransaction;
	iHandshakeTransaction = NULL;
	delete iNowPlayingTransaction;
	iNowPlayingTransaction = NULL;
	delete iSubmitTransaction;
	iSubmitTransaction = NULL;
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = NULL;
#ifdef BETA_BUILD
	delete iBetaTestersTransaction;
	iBetaTestersTransaction = NULL;	
#endif
	
	iRadioAudioTransaction.Close();
	
	if (iTrackDownloadObserver)
		{
		iTrackDownloadObserver->DataCompleteL(EErrorCancel, KErrNone, KNullDesC8);
		iTrackDownloadObserver = NULL;
		}
	
	if (aCloseTransactionArray)
		{
		// close all the transactions and callback the observers
		for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
			{
			if (iTransactions[i]->FlatDataObserver())
				{
				iTransactions[i]->FlatDataObserver()->DataL(KNullDesC8, EErrorCancel);
				}
			
			delete iTransactions[i];
			iTransactions.Remove(i);
			}
		
		iTransactions.Close();
		}
	}

void CMobblerLastFMConnection::TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse)
	{
	if (aTransaction == iHandshakeTransaction)
		{
		CMobblerLastFMError* error(CMobblerParser::ParseHandshakeL(aResponse, iScrobbleSessionID, iNowPlayingURL, iSubmitURL));
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
	else if (aTransaction == iWebServicesHandshakeTransaction)
		{
		CMobblerLastFMError* error(CMobblerParser::ParseWebServicesHandshakeL(aResponse, iWebServicesSessionKey));
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
#ifdef BETA_BUILD
	else if (aTransaction == iBetaTestersTransaction)
		{
		CMobblerLastFMError* error(CMobblerParser::ParseBetaTestersHandshakeL(aResponse, iUsername->String8(), iIsBetaTester));
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
#endif
	else if (aTransaction == iSubmitTransaction)
		{
		CMobblerLastFMError* error(CMobblerParser::ParseScrobbleResponseL(aResponse));
		
		if (!error)
			{
			// We have done a submission so remove up to the
			// first KMaxSubmitTracks from the queued tracks array
			const TInt KCount(iTrackQueue.Count());
			for (TInt i(Min(KCount - 1, KMaxSubmitTracks - 1)) ; i >= 0 ; --i)
				{
				iObserver.HandleTrackSubmittedL(*iTrackQueue[i]);
				iTrackQueue[i]->Release();
				iTrackQueue.Remove(i);
				}
			
			SaveTrackQueueL();
			}
		else
			{
			CleanupStack::PushL(error);
			if (error->ErrorCode() == CMobblerLastFMError::EBadSession)
				{
				// The session has become invalid so handshake again
				AuthenticateL();
				}
			else
				{
				iObserver.HandleLastFMErrorL(*error);
				}
			CleanupStack::PopAndDestroy(error);
			}
		}
	else if (aTransaction == iNowPlayingTransaction)
		{
		CMobblerLastFMError* error(CMobblerParser::ParseScrobbleResponseL(aResponse));
		
		if (error)
			{
			CleanupStack::PushL(error);
			if (error->ErrorCode() == CMobblerLastFMError::EBadSession)
				{
				// The session has become invalid so handshake again
				AuthenticateL();
				}
			else
				{
				iObserver.HandleLastFMErrorL(*error);
				}
			CleanupStack::PopAndDestroy(error);
			}
		else
			{
			// there was no error so try to submit any tracks in the queue
			DoSubmitL();
			}
		}
	else
		{
		const TInt KTransactionCount(iTransactions.Count());
		for (TInt i(0) ; i < KTransactionCount ; ++i)
			{
			if (aTransaction == iTransactions[i])
				{
				if (iTransactions[i]->FlatDataObserver())
					{
					iTransactions[i]->FlatDataObserver()->DataL(aResponse, EErrorNone);
					}
				break;
				}
			}
		
		}
	}

void CMobblerLastFMConnection::TransactionCompleteL(CMobblerTransaction* aTransaction)
	{
	if (aTransaction == iSubmitTransaction)
		{
		// This pointer is used to tell if we are already submitting some tracks
		// so it is important to delete and set to NULL when finished
		delete iSubmitTransaction;
		iSubmitTransaction = NULL;
		
		if (!DoSubmitL())
			{
			// There are no more tracks to submit
			// so do a now playing
			DoNowPlayingL();
			}
		}
	else
		{
		for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
			{
			if (aTransaction == iTransactions[i])
				{
				delete aTransaction;
				iTransactions.Remove(i);
				break;
				}
			}
		}
	}

void CMobblerLastFMConnection::TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aStatus, TInt aStatusCode)
	{
#ifdef _DEBUG
	// Transaction log file 
	_LIT(KTransactionLogFile, "c:\\data\\others\\mobbler\\transaction.log");
	
	RFile file;
	CleanupClosePushL(file);
	
	CCoeEnv::Static()->FsSession().MkDirAll(KTransactionLogFile);
	
	TInt error(file.Open(CCoeEnv::Static()->FsSession(), KTransactionLogFile, EFileWrite));
	if (error != KErrNone)
		{
		error = file.Create(CCoeEnv::Static()->FsSession(), KTransactionLogFile, EFileWrite);
		}
	
	if (error == KErrNone)
		{
		// Move the seek head to the end of the file
		TInt fileSize;
		file.Size(fileSize);
		file.Seek(ESeekEnd,fileSize);
		
		// Write a line in the log file
		TBuf<255> logMessage;
		
		TTime now;
		now.UniversalTime();
		
		now.FormatL(logMessage, _L("%F%D/%M/%Y %H:%T:%S\t"));
		
		if (aTransaction == iHandshakeTransaction) logMessage.Append(_L("Handshake\t"));
		else if (aTransaction == iWebServicesHandshakeTransaction) logMessage.Append(_L("WebServicesHandsake\t"));
		else if (aTransaction == iNowPlayingTransaction) logMessage.Append(_L("NowPlaying\t"));
		else if (aTransaction == iSubmitTransaction) logMessage.Append(_L("Submit\t"));
		
		HBufC* status(HBufC::NewLC(aStatus.Length()));
		status->Des().Copy(aStatus);
		logMessage.Append(*status);
		CleanupStack::PopAndDestroy(status);
		logMessage.Append(_L("\t"));
		logMessage.AppendNum(aStatusCode);
		logMessage.Append(_L("\r\n"));
		
		HBufC8* logMessage8(HBufC8::NewLC(logMessage.Length()));
		logMessage8->Des().Copy(logMessage);
		file.Write(*logMessage8);
		
		CleanupStack::PopAndDestroy(logMessage8);
		CleanupStack::PopAndDestroy(&file);
		}
#endif
	
	if (aTransaction == iSubmitTransaction)
		{
		// This pointer is used to tell if we are already submitting some tracks
		// so it is important to delete and set to NULL when finished
		delete iSubmitTransaction;
		iSubmitTransaction = NULL;
		}
	
	if (!Connected() &&
			iState != EConnecting && iState != EHandshaking)
		{
		// The connection is not open so we should try to connect again
		ConnectL();
		}
	else
		{
		if (aTransaction ==	iHandshakeTransaction
				|| aTransaction == iWebServicesHandshakeTransaction)
			{
			iObserver.HandleCommsErrorL(aStatusCode, aStatus);
			ChangeStateL(ENone);
			}
		else
			{
			for (TInt i(iTransactions.Count() - 1) ; i >= 0 ; --i)
				{
				if (aTransaction == iTransactions[i])
					{
					if (aTransaction->FlatDataObserver())
						{
						aTransaction->FlatDataObserver()->DataL(KNullDesC8, EErrorFailed);
						}
					
					delete aTransaction;
					iTransactions.Remove(i);
					break;
					}
				}
			}
		}
	}

void CMobblerLastFMConnection::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{
	// it must be a transaction event
	TPtrC8 nextDataPartPtr;
	
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseBodyData:
			{
			aTransaction.Response().Body()->GetNextDataPart(nextDataPartPtr);
			TInt dataSize(aTransaction.Response().Body()->OverallDataSize());
			if (iTrackDownloadObserver)
				{
				iTrackDownloadObserver->DataPartL(nextDataPartPtr, dataSize);
				}
			aTransaction.Response().Body()->ReleaseData();
			}
			break;
		case THTTPEvent::EFailed:
			if (iTrackDownloadObserver && aTransaction.Response().StatusCode() != 302)
				{
				iTrackDownloadObserver->DataCompleteL(EErrorFailed, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			iTrackDownloadObserver = NULL;
			break;
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			// tell the radio player that the track has finished downloading
			if (iTrackDownloadObserver && aTransaction.Response().StatusCode() != 302)
				{
				iTrackDownloadObserver->DataCompleteL(EErrorCancel, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			break;
		case THTTPEvent::ESucceeded:
			// tell the radio player that the track has finished downloading
			if (iTrackDownloadObserver && aTransaction.Response().StatusCode() != 302)
				{
				iTrackDownloadObserver->DataCompleteL(EErrorNone, aTransaction.Response().StatusCode(), aTransaction.Response().StatusText().DesC());
				iTrackDownloadObserver = NULL;
				}
			iTrackDownloadObserver = NULL;
			break;
		default:
			break;
		}
	}

TInt CMobblerLastFMConnection::MHFRunError(TInt /*aError*/, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
	// send KErrNone back so that it doesn't panic
	return KErrNone;
	}

void CMobblerLastFMConnection::CreateAuthTokenL(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp)
	{
	HBufC8* passwordHash(MobblerUtility::MD5LC(iPassword->String8()));
	HBufC8* passwordHashAndTimeStamp(HBufC8::NewLC(passwordHash->Length() + 20));
	passwordHashAndTimeStamp->Des().Append(*passwordHash);
	passwordHashAndTimeStamp->Des().AppendNum(aUnixTimeStamp.Int());
	HBufC8* authToken(MobblerUtility::MD5LC(*passwordHashAndTimeStamp));
	aHash.Copy(*authToken);
	CleanupStack::PopAndDestroy(3, passwordHash);
	}

void CMobblerLastFMConnection::ScrobbleHandshakeL()
	{
	delete iSubmitURL;
	iSubmitURL = NULL;
	delete iNowPlayingURL;
	iNowPlayingURL = NULL;
	delete iScrobbleSessionID;
	iScrobbleSessionID = NULL;
	
	iAuthenticated = EFalse;
	
	TTime now;
	now.UniversalTime();
	TTimeIntervalSeconds unixTimeStamp;
	TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	User::LeaveIfError(now.SecondsFrom(epoch, unixTimeStamp));
	
	HBufC8* authToken(HBufC8::NewLC(255));
	TPtr8 authTokenPtr(authToken->Des());
	CreateAuthTokenL(authTokenPtr, unixTimeStamp);
	
	HBufC8* path(HBufC8::NewLC(255));
	path->Des().AppendFormat(KScrobbleQuery, &iUsername->String8(), unixTimeStamp.Int(), &authTokenPtr);
	
	CUri8* uri(CUri8::NewLC());
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KScrobbleHost, EUriHost);
	uri->SetComponentL(*path, EUriQuery);
	
	delete iHandshakeTransaction;
	iHandshakeTransaction = CMobblerTransaction::NewL(*this, uri);
	CleanupStack::Pop(uri);
	iHandshakeTransaction->SubmitL();
	CleanupStack::PopAndDestroy(2, authToken);
	}

void CMobblerLastFMConnection::LoadTrackQueueL()
	{
	const TInt KTrackQueueCount(iTrackQueue.Count());
	for (TInt i(0) ; i < KTrackQueueCount ; ++i)
		{
		iTrackQueue[i]->Release();
		}
	iTrackQueue.Reset();
	
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KTracksFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TInt trackCount(0);
		TRAP_IGNORE(trackCount = readStream.ReadInt32L());
		
		for (TInt i(0); i < trackCount; ++i)
			{
			CMobblerTrack* track(CMobblerTrack::NewL(readStream));
			CleanupStack::PushL(track);
			iTrackQueue.AppendL(track);
			CleanupStack::Pop(track);
			}
		
		CleanupStack::PopAndDestroy(&readStream);
		
		const TInt KTrackQueueCount(iTrackQueue.Count());
		for (TInt i(KTrackQueueCount - 1) ; i >= 0  ; --i)
			{
			iObserver.HandleTrackQueuedL(*iTrackQueue[i]);
			}
		}
		
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFMConnection::SaveTrackQueueL()
	{
	CCoeEnv::Static()->FsSession().MkDirAll(KTracksFile);
	
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KTracksFile, EFileWrite));
	
	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		const TInt KTracksCount(iTrackQueue.Count());
		
		writeStream.WriteInt32L(KTracksCount);
		
		for (TInt i(0) ; i < KTracksCount ; ++i)
			{
			writeStream << *iTrackQueue[i];
			}
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

TBool CMobblerLastFMConnection::ExportQueueToLogFileL()
	{
	// Format described here: http://www.audioscrobbler.net/wiki/Portable_Player_Logging
	// Uploaders can be found here: http://www.rockbox.org/twiki/bin/view/Main/LastFMLog
	
	const TInt KTracksCount(iTrackQueue.Count());
	if (KTracksCount == 0)
		{
		return EFalse;
		}
	
	CCoeEnv::Static()->FsSession().MkDirAll(KLogFile);
	
	TInt errors(KErrNone);
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError(file.Replace(CCoeEnv::Static()->FsSession(), KLogFile, EFileWrite));
	if (replaceError != KErrNone)
		{
		return EFalse;
		}
	else
		{
		errors = file.Write(KLogFileHeader);
		TBuf8<10> version;
		version.Copy(KVersion.Name());
		errors += file.Write(version);
		errors += file.Write(KLogFileEndOfLine);
		}
	
	if (errors != KErrNone)
		{
		return EFalse;
		}
	
	errors = KErrNone;
	for (TInt i(0); i < KTracksCount && errors == KErrNone; ++i)
		{
		// artist name
		TBuf8<255> buf(iTrackQueue[i]->Artist().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// album name (optional)
		buf.Zero();
		buf.Append(iTrackQueue[i]->Album().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// track name
		buf.Zero();
		buf.Append(iTrackQueue[i]->Title().String8());
		StripOutTabs(buf);
		errors += file.Write(buf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// track position on album (optional)
		if (iTrackQueue[i]->TrackNumber() != KErrUnknown)
			{
			TBuf8<10> trackNumber;
			trackNumber.AppendNum(iTrackQueue[i]->TrackNumber());
			errors += file.Write(trackNumber);
			}
		file.Write(KLogFileFieldSeperator);
		
		// song duration in seconds
		TBuf8<10> trackLength;
		trackLength.AppendNum(iTrackQueue[i]->TrackLength().Int());
		errors += file.Write(trackLength);
		errors += file.Write(KLogFileFieldSeperator);
		
		// rating (L if listened at least 50% or S if skipped)
		errors += file.Write(KLogFileListenedRating);
		errors += file.Write(KLogFileFieldSeperator);
		
		// UNIX timestamp when song started playing
		TTimeIntervalSeconds unixTimeStamp;
		TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
		iTrackQueue[i]->StartTimeUTC().SecondsFrom(epoch, unixTimeStamp);
		TBuf8<20> startTimeBuf;
		startTimeBuf.AppendNum(unixTimeStamp.Int());
		errors += file.Write(startTimeBuf);
		errors += file.Write(KLogFileFieldSeperator);
		
		// MusicBrainz Track ID (optional)
		errors += file.Write(KLogFileEndOfLine);
		}
	
	if (errors != KErrNone)
		{
		return EFalse;
		}
		
	// No errors, ok to empty the queue
	for (TInt i(KTracksCount - 1); i >= 0; --i)
		{
		// If the track was loved, tough, that can't be submitted via log file
		iObserver.HandleTrackDequeued(*iTrackQueue[i]);
		iTrackQueue[i]->Release();
		iTrackQueue.Remove(i);
		}
	SaveTrackQueueL();
	
	CleanupStack::PopAndDestroy(&file);
	
	return ETrue;
	}

void CMobblerLastFMConnection::StripOutTabs(TDes8& aString)
	{
	TInt position(aString.Find(KLogFileFieldSeperator));
	 while (position != KErrNotFound)
		{
		aString.Delete(position, 1);
		position = aString.Find(KLogFileFieldSeperator);
		}
	}

void CMobblerLastFMConnection::ToggleScrobblingL()
	{
	iScrobblingOn = !iScrobblingOn;
	
	iScrobblingOn ?
		SaveCurrentTrackL() :
		DeleteCurrentTrackFile();
	}

void CMobblerLastFMConnection::LoadCurrentTrackL()
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(),
							   KCurrentTrackFile, EFileRead));
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		if (iCurrentTrack)
			{
			//  There is already a current track so get rid of it
			iCurrentTrack->Release();
			}
		iCurrentTrack  = CMobblerTrack::NewL(readStream);
		iCurrentTrack->SetPlaybackPosition(readStream.ReadInt32L());
		iCurrentTrack->SetTotalPlayed(readStream.ReadInt32L());
		CleanupStack::PopAndDestroy(&readStream);
		TrackStoppedL();
		DeleteCurrentTrackFile();
		}
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFMConnection::SaveCurrentTrackL()
	{
	if (!iCurrentTrackSaved &&
		iScrobblingOn && 
		iCurrentTrack)
		{
		TInt playbackPosition(Min(iCurrentTrack->PlaybackPosition().Int(), 
								  iCurrentTrack->TrackLength().Int()));
		TInt scrobbleTime(iCurrentTrack->InitialPlaybackPosition().Int() + 
						  iCurrentTrack->ScrobbleDuration().Int());

		if (playbackPosition >= scrobbleTime)
			{
			// Save the track to file
			CCoeEnv::Static()->FsSession().MkDirAll(KCurrentTrackFile);
			
			RFile file;
			CleanupClosePushL(file);
			TInt createError(file.Create(CCoeEnv::Static()->FsSession(), 
										   KCurrentTrackFile, EFileWrite));
			
			if (createError == KErrNone)
				{
				RFileWriteStream writeStream(file);
				CleanupClosePushL(writeStream);
				writeStream << *iCurrentTrack;
				writeStream.WriteInt32L(iCurrentTrack->PlaybackPosition().Int());
				writeStream.WriteInt32L(iCurrentTrack->TotalPlayed().Int());
				
				CleanupStack::PopAndDestroy(&writeStream);
				iCurrentTrackSaved = ETrue;
				}
			
			CleanupStack::PopAndDestroy(&file);
			}
		}
	}

void CMobblerLastFMConnection::DeleteCurrentTrackFile()
	{
	CCoeEnv::Static()->FsSession().Delete(KCurrentTrackFile);
	iCurrentTrackSaved = EFalse;
	}

// End of file
