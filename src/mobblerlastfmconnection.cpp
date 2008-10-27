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

#include <chttpformencoder.h>
#include <httpstringconstants.h>

#include "coemain.h"
#include "mobblerappui.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerradioplaylist.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblertransaction.h"
#include "mobblerutility.h"
#include "mobblerwebservicesobserver.h"
#include "mobblerwebservicesquery.h"

// the scheme of the Last.fm handshake
_LIT8(KScheme, "http");

// the Last.fm URL to send the handshake to
_LIT8(KScrobbleHost, "post.audioscrobbler.com");
_LIT8(KWebServicesHost, "ws.audioscrobbler.com");

// The format for the handshake URL 
_LIT8(KScrobbleQuery, "hs=true&p=1.2&c=mlr&v=0.1&u=%S&t=%d&a=%S");

_LIT8(KRadioHost, "ws.audioscrobbler.com");
_LIT8(KRadioPath, "/radio/handshake.php");
_LIT8(KRadioQuery, "version=1.3.1.1&platform=symbian&username=%S&passwordmd5=%S&language=%S&player=mobbler");
_LIT8(KRadioStationQuery, "session=%S&url=%S&lang=%S");
_LIT8(KRadioPlaylistQuery, "sk=%S&discovery=0&desktop=1.3.1.1");

_LIT8(KRadioStationPersonal, "lastfm://user/%S/personal");
_LIT8(KRadioStationPlaylist, "lastfm://user/%S/playlist");
_LIT8(KRadioStationLoved, "lastfm://user/%S/loved");
_LIT8(KRadioStationArtist, "lastfm://artist/%S");
_LIT8(KRadioStationTag, "lastfm://globaltags/%S");
//_LIT8(KRadioStationGroup, "lastfm://group/%S");
_LIT8(KRadioStationNeighbours, "lastfm://user/%S/neighbours");
_LIT8(KRadioStationRecommended, "lastfm://user/%S/recommended/100");

_LIT8(KLatesverFileLocation, "http://www.mobbler.co.uk/latestver.xml");

// The file name to store the queue of listened tracks
_LIT(KTracksFile, "c:track_queue.dat");
_LIT(KStateFile, "c:state.dat");

// The granularity of the buffers that responses from Last.fm are read into
const TInt KBufferGranularity(256);

// Last.fm can accept up to this many track in one submission
const TInt KMaxSubmitTracks(50);

CMobblerLastFMConnection* CMobblerLastFMConnection::NewL(const TDesC& aUsername, const TDesC& aPassword)
	{
	CMobblerLastFMConnection* self = new(ELeave) CMobblerLastFMConnection;
	CleanupStack::PushL(self);
	self->ConstructL(aUsername, aPassword);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerLastFMConnection::CMobblerLastFMConnection()
	:CActive(CActive::EPriorityStandard)
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
	
	if (iDownloadingTrack)
		{
		iDownloadingTrack->Release();
		}

	const TInt KTrackQueueCount(iTrackQueue.Count());
	for (TInt i(0) ; i < KTrackQueueCount ; ++i)
		{
		iTrackQueue[i]->Release();
		}
	iTrackQueue.Reset();
	
	const TInt KLovedTrackCount(iLovedTrackQueue.Count());
	for (TInt i(0) ; i < KLovedTrackCount ; ++i)
		{
		iLovedTrackQueue[i]->Release();
		}
	iLovedTrackQueue.Reset();
	
	iObservers.Close();
	
	iRadioAudioTransaction.Close();
	
	delete iHandshakeTransaction;
	delete iRadioHandshakeTransaction;
	delete iRadioAlbumArtTransaction;
	delete iWebServicesHandshakeTransaction;
	delete iNowPlayingTransaction;
	delete iSubmitTransaction;
	delete iRadioSelectStationTransaction;
	delete iRadioPlaylistTransaction;
	delete iTrackLoveTransaction;
	delete iTrackBanTransaction;
	delete iUpdateTransaction;
	delete iArtistGetInfoTransaction;
	
	delete iUsername;
	delete iPassword;
	
	delete iSessionID;
	delete iNowPlayingURL;
	delete iSubmitURL;
	
	delete iRadioSessionID;
	delete iRadioStreamURL;
	delete iRadioBaseURL;
	delete iRadioBasePath;
	
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
	
	LoadSettingsL();
	}

void CMobblerLastFMConnection::LoadSettingsL()
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError = file.Open(CCoeEnv::Static()->FsSession(), KStateFile, EFileRead);
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TInt8 mode = readStream.ReadInt8L();
		TInt iap = readStream.ReadInt32L();
		
		// future proofing
		delete HBufC8::NewL(readStream, KMaxTInt);
		
		CleanupStack::PopAndDestroy(&readStream);
		
		SetModeL(static_cast<TMode>(mode));
		}
		
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFMConnection::SaveSettingsL()
	{
	CCoeEnv::Static()->FsSession().MkDirAll(KStateFile);
	
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError = file.Replace(CCoeEnv::Static()->FsSession(), KStateFile, EFileWrite);
	
	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
			
		writeStream.WriteInt8L(iMode);
		writeStream.WriteInt32L(0);
		
		// future proofing
		writeStream << KNullDesC8;
	
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFMConnection::DoSetModeL(TMode aMode)
	{
	iMode = aMode;
	SaveSettingsL();
	}

void CMobblerLastFMConnection::AddObserverL(MMobblerLastFMConnectionObserver* aObserver)
	{
	iObservers.InsertInAddressOrderL(aObserver);
	
	// observers need to be notified about all the current queued tracks
	// because the saved tracks queue is loaded before the observers are added
	const TInt KTrackQueueCount(iTrackQueue.Count());
	for (TInt i(KTrackQueueCount - 1) ; i >= 0  ; --i)
		{
		aObserver->HandleTrackQueuedL(*iTrackQueue[i]);
		}
	}

void CMobblerLastFMConnection::RemoveObserverL(MMobblerLastFMConnectionObserver* aObserver)
	{
	TInt position = iObservers.FindInAddressOrderL(aObserver);
	if (position >= 0)
		{
		iObservers.Remove(position);
		}
	}

void CMobblerLastFMConnection::NotifyConnectCompleteL()
	{
	// notify all observers that the connection is complete
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleConnectCompleteL();
		}
	}

void CMobblerLastFMConnection::NotifyLastFMErrorL(CMobblerLastFMError& aError)
	{
	// notify all observers that Last.fm returned
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleLastFMErrorL(aError);
		}
	}

void CMobblerLastFMConnection::NotifyCommsErrorL(const TDesC& aTransaction, const TDesC8& aStatus)
	{
	// notify all observers that there has been a communications error
	
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleCommsErrorL(aTransaction, aStatus);
		}
	}

void CMobblerLastFMConnection::NotifyTrackSubmittedL(const CMobblerTrack& aTrack)
	{
	// notify all observers that a track has been sucessfully submitted to Last.fm
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleTrackSubmittedL(aTrack);
		}
	}

void CMobblerLastFMConnection::NotifyTrackQueuedL(const CMobblerTrack& aTrack)
	{
	// notify all observers that a track has been listened to and queued
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleTrackQueuedL(aTrack);
		}
	}

void CMobblerLastFMConnection::NotifyTrackNowPlayingL(const CMobblerTrack& aTrack)
	{
	// notify all observers that a track has been registered as now playing
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleTrackNowPlayingL(aTrack);
		}
	}

void CMobblerLastFMConnection::NotifyUpdateResponseL(TVersion aVersion, const TDesC8& aLocation)
	{
	// notify all observers that a track has been registered as now playing
	const TInt KObserverCount(iObservers.Count());
	for (TInt i (0) ; i < KObserverCount; ++i)
		{
		iObservers[i]->HandleUpdateResponseL(aVersion, aLocation);
		}
	}

void CMobblerLastFMConnection::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	HBufC* tempUsername = aUsername.AllocL();
	HBufC* tempPassword = aPassword.AllocL();
	delete iUsername;
	delete iPassword;
	iUsername = tempUsername;
	iPassword = tempPassword;
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
		DoSetModeL(aMode);
		SaveSettingsL();
		Disconnect();
		}
	}

CMobblerLastFMConnection::TMode CMobblerLastFMConnection::Mode() const
	{
	return iMode;
	}

CMobblerLastFMConnection::TState CMobblerLastFMConnection::State() const
	{
	return iState;
	}

void CMobblerLastFMConnection::ChangeState(TState aState)
	{
	iState = aState;
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
	}
	
void CMobblerLastFMConnection::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		//User::LeaveIfError(iConnection.GetIntSetting(_L("IAP\\Id"), iIap));
		SaveSettingsL();
		
		iHTTPSession.Close();
		iHTTPSession.OpenL();
	
		RStringPool strP = iHTTPSession.StringPool();
		RHTTPConnectionInfo connInfo = iHTTPSession.ConnectionInfo();
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), THTTPHdrVal(iSocketServ.Handle()));
		TInt connPtr = REINTERPRET_CAST(TInt, &iConnection);
		connInfo.SetPropertyL(strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), THTTPHdrVal(connPtr));
		
		// Handshake with Last.fm
		ChangeState(EHandshaking);
		HandshakeL();
		RadioHandshakeL();
		WSHandshakeL();
		}
	else
		{
		iRadioPlayer->Stop();
		ChangeState(ENone);
		}
	}

void CMobblerLastFMConnection::DoCancel()
	{
	iConnection.Close();
	}

void CMobblerLastFMConnection::ConnectL()
	{
	Cancel();
	Disconnect();
	ChangeState(EConnecting);
	User::LeaveIfError(iConnection.Open(iSocketServ));
	iConnection.Start(iStatus);
	SetActive();
	}

void CMobblerLastFMConnection::SetRadioPlayer(MMobblerRadioPlayer& aRadioPlayer)
	{
	iRadioPlayer = &aRadioPlayer;
	}

void CMobblerLastFMConnection::WSHandshakeL()
	{
	// start the web services authentications

	delete iWebServicesSessionKey;
	iWebServicesSessionKey = NULL;
	
	HBufC8* password = HBufC8::NewLC(iPassword->Length());
	password->Des().Copy(*iPassword);
	HBufC8* passwordHash = MobblerUtility::MD5LC(*password);
	
	HBufC8* username8 = HBufC8::NewLC(iUsername->Length());
	username8->Des().Copy(*iUsername);
	
	HBufC8* usernameAndPasswordHash = HBufC8::NewLC(passwordHash->Length() + iUsername->Length());
	usernameAndPasswordHash->Des().Copy(*iUsername);
	usernameAndPasswordHash->Des().Append(*passwordHash);
	
	HBufC8* authToken = MobblerUtility::MD5LC(*usernameAndPasswordHash);
	
	CMobblerWebServicesQuery* query = CMobblerWebServicesQuery::NewLC(_L8("auth.getMobileSession"));
	query->AddFieldL(_L8("authToken"), *authToken);
	query->AddFieldL(_L8("username"), *username8);
	HBufC8* queryText = query->GetQueryAuthLC();
	
	CUri8* uri = CUri8::NewL();
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KWebServicesHost, EUriHost);
	uri->SetComponentL(_L8("/2.0/"), EUriPath);
	uri->SetComponentL(*queryText, EUriQuery);
	
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
	
	CleanupStack::PopAndDestroy(8, password);
	}

TInt CMobblerLastFMConnection::CheckForUpdatesL()
	{
	TInt error(KErrNone);
	
	if (iMode == EOnline)
		{
		TUriParser8 uriParser;
		uriParser.Parse(KLatesverFileLocation);
			
		delete iUpdateTransaction;
		iUpdateTransaction = CMobblerTransaction::NewL(iHTTPSession, uriParser, *this);
		ChangeState(EUpdates);
		}
	else if (iState != EConnecting && iState != EHandshaking)
		{
		error = KErrBadHandle;
		}
	else
		{
		error = KErrNotReady;
		}
	
	return error;
	}

void CMobblerLastFMConnection::DoTrackLoveL()
	{
	if (iMode == EOnline && iLovedTrackQueue.Count() > 0)
		{		
		// we are connected and there are loved tracks to submit
		CUri8* uri = CUri8::NewL();
		CleanupStack::PushL(uri);
		
		uri->SetComponentL(KScheme, EUriScheme);
		uri->SetComponentL(KWebServicesHost, EUriHost);
		uri->SetComponentL(_L8("/2.0/"), EUriPath);
		
		CMobblerWebServicesQuery* query = CMobblerWebServicesQuery::NewLC(_L8("track.love"));	
		query->AddFieldL(_L8("track"), iLovedTrackQueue[0]->Title().String8());
		query->AddFieldL(_L8("artist"), iLovedTrackQueue[0]->Artist().String8());
		query->AddFieldL(_L8("sk"), *iWebServicesSessionKey); 
		
		CHTTPFormEncoder* form = query->GetFormLC();
		
		delete iTrackLoveTransaction;
		iTrackLoveTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this, form);
		CleanupStack::Pop(form);
		
		CleanupStack::PopAndDestroy(2, uri);
		}
	}

TInt CMobblerLastFMConnection::TrackBanL(const CMobblerTrack& aTrack)
	{
	TInt error(KErrNone);
	
	if (iMode == EOnline)
		{		
		CUri8* uri = CUri8::NewL();
		CleanupStack::PushL(uri);
		
		uri->SetComponentL(KScheme, EUriScheme);
		uri->SetComponentL(KWebServicesHost, EUriHost);
		uri->SetComponentL(_L8("/2.0/"), EUriPath);
		
		CMobblerWebServicesQuery* query = CMobblerWebServicesQuery::NewLC(_L8("track.ban"));	
		query->AddFieldL(_L8("track"), aTrack.Title().String8());
		query->AddFieldL(_L8("artist"), aTrack.Artist().String8());
		query->AddFieldL(_L8("sk"), *iWebServicesSessionKey); 
		
		CHTTPFormEncoder* form = query->GetFormLC();
		
		delete iTrackBanTransaction;
		iTrackBanTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this, form);
		CleanupStack::Pop(form);
		
		CleanupStack::PopAndDestroy(2, uri);
		}
	else if (iState != EConnecting && iState != EHandshaking)
		{
		error = KErrBadHandle;
		}
	else
		{
		error = KErrNotReady;
		}
	
	return error; 
	}

TInt CMobblerLastFMConnection::UserGetFriendsL(const TDesC8& /*aUsername*/, MWebServicesObserver& /*aObserver*/)
	{
	/*
	TInt error(KErrNone);
	
	if (iMode == EOnline)
		{
		CMobblerWebServicesQuery* query = CMobblerWebServicesQuery::NewLC(_L8("user.getfriends"));	
		
		if (aUsername.Length() == 0)
			{
			HBufC8* username8 = HBufC8::NewLC(iUsername->Length());
			username8->Des().Copy(*iUsername);
			query->AddFieldL(_L8("user"), *username8);
			CleanupStack::PopAndDestroy(username8);
			}
		else
			{
			query->AddFieldL(_L8("user"), aUsername);
			}
		
		CUri8* uri = CUri8::NewL();
		CleanupStack::PushL(uri);
		uri->SetComponentL(KScheme, EUriScheme);
		uri->SetComponentL(KWebServicesHost, EUriHost);
		uri->SetComponentL(_L8("/2.0/"), EUriPath);
		uri->SetComponentL(*query->GetQueryLC(), EUriQuery);
		CleanupStack::PopAndDestroy(); // query->GetQueryLC()
		
		delete iTrackBanTransaction;
		iTrackTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
		iTrackTransaction->SetWebServicesObserver(aObserver);
		
		CleanupStack::PopAndDestroy(2, query);
		}
	else if (iState != EConnecting && iState != EHandshaking)
		{
		error = KErrBadHandle;
		}
	else
		{
		error = KErrNotReady;
		}
		
	return error;
	*/
	
	return KErrNone;
	}

TInt CMobblerLastFMConnection::ArtistGetInfoL(const CMobblerTrack& aTrack, MWebServicesObserver& aObserver)
	{
	TInt error(KErrNone);
	
	if (iMode == EOnline)
		{		
		CUri8* uri = CUri8::NewL();
		CleanupStack::PushL(uri);
		
		uri->SetComponentL(KScheme, EUriScheme);
		uri->SetComponentL(KWebServicesHost, EUriHost);
		uri->SetComponentL(_L8("/2.0/"), EUriPath);
		
		CMobblerWebServicesQuery* query = CMobblerWebServicesQuery::NewLC(_L8("artist.getinfo"));
		query->AddFieldL(_L8("artist"), aTrack.Artist().String8());
		
		CHTTPFormEncoder* form = query->GetFormLC();
		
		delete iArtistGetInfoTransaction;
		iArtistGetInfoTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this, form);
		iArtistGetInfoTransaction->SetWebServicesObserver(aObserver);
		CleanupStack::Pop(form);
		
		CleanupStack::PopAndDestroy(2, uri);
		}
	else
		{
		error = KErrNotReady;
		}
	
	return error;
	
	}

void CMobblerLastFMConnection::RadioHandshakeL()
	{
	// start the radio handshake transaction
	
	delete iRadioSessionID;
	iRadioSessionID = NULL;
	delete iRadioStreamURL;
	iRadioStreamURL = NULL;
	delete iRadioBaseURL;
	iRadioBaseURL = NULL;
	delete iRadioBasePath;
	iRadioBasePath = NULL;
	
	HBufC8* password = HBufC8::NewLC(iPassword->Length());
	password->Des().Copy(*iPassword);
	HBufC8* passwordHash = MobblerUtility::MD5LC(*password);
	TPtr8 passwordHashPtr(passwordHash->Des());
	
	HBufC8* path = HBufC8::NewLC(255);
	HBufC8* username = HBufC8::NewLC(iUsername->Length());
	TPtr8 usernamePtr(username->Des());
	usernamePtr.Copy(*iUsername);
	
	// get the phone language code
	TBuf8<2> language = MobblerUtility::LanguageL();
	
	path->Des().AppendFormat(KRadioQuery, &usernamePtr, &passwordHashPtr, &language);
	
	CUri8* uri = CUri8::NewL();
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KRadioHost, EUriHost);
	uri->SetComponentL(KRadioPath, EUriPath);
	uri->SetComponentL(*path, EUriQuery);
	
	delete iRadioHandshakeTransaction;
	iRadioHandshakeTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
	
	CleanupStack::PopAndDestroy(5, password);
	}

void CMobblerLastFMConnection::RadioStop()
	{
	iRadioAudioTransaction.Close();
	delete iRadioAlbumArtTransaction;
	iRadioAlbumArtTransaction = NULL;
	iRadioPlayer->TrackDownloadCompleteL();
	}

TInt CMobblerLastFMConnection::RadioStartL(TRadioStation aRadioStation, const TDesC8& aRadioText)
	{
	TInt error(KErrNone);
	
	if (iMode == EOnline)
		{
		HBufC8* path = HBufC8::NewLC(255);
		TPtr8 pathPtr(path->Des());
		pathPtr.Copy(*iRadioBasePath);
		pathPtr.Append(_L8("/adjust.php"));
		
		TPtr8 radioSessionIDPtr(iRadioSessionID->Des());
		
		HBufC8* radioURL = HBufC8::NewLC(255);
		
		HBufC8* username = HBufC8::NewLC(iUsername->Length());
		TPtr8 usernamePtr(username->Des());
		usernamePtr.Copy(*iUsername);
		
		switch (aRadioStation)
			{
			case EPersonal:
				radioURL->Des().AppendFormat(KRadioStationPersonal, &usernamePtr);
				break;
			case EUser:
				radioURL->Des().AppendFormat(KRadioStationPersonal, &aRadioText);
				break;
			case EMyPlaylist:
				radioURL->Des().AppendFormat(KRadioStationPlaylist, &usernamePtr);
				break;
			case ERecommendations:
				radioURL->Des().AppendFormat(KRadioStationRecommended, &usernamePtr);
				break;
			case ENeighbourhood:
				radioURL->Des().AppendFormat(KRadioStationNeighbours, &usernamePtr);
				break;
			case ELovedTracks:
				radioURL->Des().AppendFormat(KRadioStationLoved, &usernamePtr);
				break;
			case EArtist:
				radioURL->Des().AppendFormat(KRadioStationArtist, &aRadioText);
				break;
			case ETag:
				radioURL->Des().AppendFormat(KRadioStationTag, &aRadioText);
				break;
			default:
				break;
			}
		
		TPtr8 radioURLPtr(radioURL->Des());
		
		TBuf8<2> language = MobblerUtility::LanguageL();
		
		HBufC8* query = HBufC8::NewLC(255);
		query->Des().AppendFormat(KRadioStationQuery, &radioSessionIDPtr, &radioURLPtr, &language);
		
		CUri8* uri = CUri8::NewL();
		CleanupStack::PushL(uri);
		
		uri->SetComponentL(KScheme, EUriScheme);
		uri->SetComponentL(*iRadioBaseURL, EUriHost);
		uri->SetComponentL(pathPtr, EUriPath);
		uri->SetComponentL(*query, EUriQuery);
		
		delete iRadioSelectStationTransaction;
		iRadioSelectStationTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
		ChangeState(ERadioSelect);
		
		CleanupStack::PopAndDestroy(5, path);
		}
	else if (iState != EConnecting && iState != EHandshaking)
		{
		error = KErrBadHandle;
		}
	else
		{
		error = KErrNotReady;
		}
	
	return error;
	}

void CMobblerLastFMConnection::RequestPlaylistL()
	{
	if (iMode == EOnline)
		{
		if (iRadioSessionID)
			{
			// only try to request a playlist if we have the session id
			HBufC8* path = HBufC8::NewLC(255);
			TPtr8 pathPtr(path->Des());
			pathPtr.Copy(*iRadioBasePath);
			pathPtr.Append(_L8("/xspf.php"));
			
			TPtr8 radioSessionIDPtr(iRadioSessionID->Des());
			
			HBufC8* query = HBufC8::NewLC(255);
			query->Des().AppendFormat(KRadioPlaylistQuery, &radioSessionIDPtr);
			
			CUri8* uri = CUri8::NewL();
			CleanupStack::PushL(uri);
			
			uri->SetComponentL(KScheme, EUriScheme);
			uri->SetComponentL(*iRadioBaseURL, EUriHost);
			uri->SetComponentL(pathPtr, EUriPath);
			uri->SetComponentL(*query, EUriQuery);
			
			delete iRadioPlaylistTransaction;
			iRadioPlaylistTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
			ChangeState(ERadioPlaylist);
			
			CleanupStack::PopAndDestroy(3, path);
			}
		}
	}

void CMobblerLastFMConnection::RequestMp3L(CMobblerTrack* aTrack)
	{
	if (iMode == EOnline)
		{
		if (iDownloadingTrack)
			{
			iDownloadingTrack->Release();
			}
		
		iDownloadingTrack = aTrack;
		iDownloadingTrack->Open();
		
		// Request the mp3 data
		TUriParser8 urimp3Parser;
		urimp3Parser.Parse(iDownloadingTrack->Mp3Location());
		
		iRadioAudioTransaction.Close();
		iRadioAudioTransaction = iHTTPSession.OpenTransactionL(urimp3Parser, *this);
		iRadioAudioTransaction.SubmitL();
		
		if (iDownloadingTrack->AlbumArtLocation().Compare(KNullDesC8) != 0)
			{
			// Request the album art data
			TUriParser8 uriAlbumArtParser;
			uriAlbumArtParser.Parse(iDownloadingTrack->AlbumArtLocation());
			
			delete iRadioAlbumArtTransaction;
			iRadioAlbumArtTransaction = CMobblerTransaction::NewL(iHTTPSession, uriAlbumArtParser, *this);
			}
		}
	}

void CMobblerLastFMConnection::Disconnect()
	{
	Cancel();
	
	ChangeState(ENone);
	
	if (iRadioPlayer)
		{
		iRadioPlayer->Stop();
		}
	
	// close any ongoing transactions
	delete iHandshakeTransaction;
	iHandshakeTransaction = NULL;
	delete iRadioHandshakeTransaction;
	iRadioHandshakeTransaction = NULL;
	delete iRadioSelectStationTransaction;
	iRadioSelectStationTransaction = NULL;
	delete iRadioPlaylistTransaction;
	iRadioPlaylistTransaction = NULL;
	delete iRadioAlbumArtTransaction;
	iRadioAlbumArtTransaction = NULL;
	delete iNowPlayingTransaction;
	iNowPlayingTransaction = NULL;
	delete iSubmitTransaction;
	iSubmitTransaction = NULL;
	delete iWebServicesHandshakeTransaction;
	iWebServicesHandshakeTransaction = NULL;
	delete iTrackLoveTransaction;
	iTrackLoveTransaction = NULL;
	delete iTrackLoveTransaction;
	iTrackLoveTransaction = NULL;
	delete iUpdateTransaction;
	iUpdateTransaction = NULL;
	delete iArtistGetInfoTransaction;
	iArtistGetInfoTransaction = NULL;
	
	iRadioAudioTransaction.Close();
	
	// delete strings for URLs etc
	delete iSessionID;
	iSessionID = NULL;
	delete iNowPlayingURL;
	iNowPlayingURL = NULL;
	delete iSubmitURL;
	iSubmitURL = NULL;
	delete iRadioSessionID;
	iRadioSessionID = NULL;
	delete iRadioStreamURL;
	iRadioStreamURL = NULL;
	delete iRadioBaseURL;
	iRadioBaseURL = NULL;
	delete iRadioBasePath;
	iRadioBasePath = NULL;
	
	iHTTPSession.Close();
	iConnection.Close();
	}
	
void CMobblerLastFMConnection::DoNowPlayingL()
	{
	if (iCurrentTrack)
		{
		NotifyTrackNowPlayingL(*iCurrentTrack);
		
		if (iMode == EOnline && iNowPlayingURL)
			{
			// We must be in online mode and have recieved the now playing URL and session ID from Last.fm
			// before we try to submit and tracks
			
			// create the from that will be sent to Last.fm
			CHTTPFormEncoder* nowPlayingForm = CHTTPFormEncoder::NewL();
			CleanupStack::PushL(nowPlayingForm);
			
			nowPlayingForm->AddFieldL(_L8("s"), *iSessionID);
			nowPlayingForm->AddFieldL(_L8("a"), iCurrentTrack->Artist().String8());
			nowPlayingForm->AddFieldL(_L8("t"), iCurrentTrack->Title().String8());
			nowPlayingForm->AddFieldL(_L8("b"), iCurrentTrack->Album().String8());
			TBuf8<10> trackLength;
			trackLength.AppendNum(iCurrentTrack->TrackLength().Int());
			nowPlayingForm->AddFieldL(_L8("l"), trackLength);
			nowPlayingForm->AddFieldL(_L8("n"), KNullDesC8);
			nowPlayingForm->AddFieldL(_L8("m"), KNullDesC8);
			
			// get the uri
			TUriParser8 uriParser;
			uriParser.Parse(*iNowPlayingURL);
			
			delete iNowPlayingTransaction;
			iNowPlayingTransaction = CMobblerTransaction::NewL(iHTTPSession, uriParser, *this, nowPlayingForm);
			CleanupStack::Pop(nowPlayingForm);
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
		
		if (iCurrentTrack->RadioAuth().Length() != 0)
			{
			// It's a radio track so test the amount of continuous playback
			listenedFor = iCurrentTrack->PlaybackPosition();
			}
		else
			{
			// The current track is a music player app so test the amount of continuous playback
			
			TTime now;
			now.UniversalTime();
			User::LeaveIfError(now.SecondsFrom(iCurrentTrack->StartTimeUTC(), listenedFor));
			}
		
		// Test if the track passes Last.fm's scrobble rules
		if ( ((listenedFor.Int() * 2) >= iCurrentTrack->TrackLength().Int()	// They have listened to over half the track in one go
				|| listenedFor.Int() >= 240)								// or more than 4 minutes.
				&& iCurrentTrack->TrackLength().Int() >= 30					// the track length is over 30 seconds.
				&& iCurrentTrack->Artist().String().Length() > 0 )					// must have an artist name	
									
			{
			// It passed, so notify and append it to the list
			NotifyTrackQueuedL(*iCurrentTrack);
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
	
	// Save the track queue and try to do a submission
	SaveTrackQueue();
	DoSubmitL();
	}

TBool CMobblerLastFMConnection::DoSubmitL()
	{
	TBool submitting(EFalse);
	
	if (iMode == EOnline && !iSubmitTransaction && iSubmitURL)
		{
		// We are connected and not already submitting tracks 
		// so try to submit the tracks in the queue
		
		const TInt KSubmitTracksCount(iTrackQueue.Count());
		
		if (KSubmitTracksCount > 0)
			{
			CHTTPFormEncoder* submitForm = CHTTPFormEncoder::NewL();
			CleanupStack::PushL(submitForm);
			
			_LIT8(KS, "s");
			submitForm->AddFieldL(KS, *iSessionID);
			
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
				
				if (iTrackQueue[ii]->RadioAuth().Compare(KNullDesC8) == 0)
					{
					submitForm->AddFieldL(o, _L8("P"));
					}
				else
					{
					// this track was played by the radio player
					HBufC8* sourceValue = HBufC8::NewLC(iTrackQueue[ii]->RadioAuth().Length() + 1);
					sourceValue->Des().Append(_L8("L"));
					sourceValue->Des().Append(iTrackQueue[ii]->RadioAuth());
					submitForm->AddFieldL(o, *sourceValue);
					CleanupStack::PopAndDestroy(sourceValue);
					}
				
				HBufC8* love = HBufC8::NewLC(1);
				if (iTrackQueue[ii]->Love())
					{
					love->Des().Append(_L("L"));
					}
				
				submitForm->AddFieldL(r, *love);
				CleanupStack::PopAndDestroy(love);
				
				TBuf8<10> trackLength;
				trackLength.AppendNum(iTrackQueue[ii]->TrackLength().Int());
				submitForm->AddFieldL(l, trackLength);
				
				submitForm->AddFieldL(n, KNullDesC8);
				submitForm->AddFieldL(m, KNullDesC8);
				}
			
			// get the uri
			TUriParser8 uriParser;
			uriParser.Parse(*iSubmitURL);
			
			delete iSubmitTransaction;
			iSubmitTransaction = CMobblerTransaction::NewL(iHTTPSession, uriParser, *this, submitForm);
			CleanupStack::Pop(submitForm);
			
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
		
		if (iSessionID && iRadioSessionID && iWebServicesSessionKey)
			{
			// only notify the UI when we are fully connected
			DoSetModeL(EOnline);
			NotifyConnectCompleteL();
			ChangeState(ENone);
			DoSubmitL();
			}
		}
	else
		{
		// There was an error with one of the handshakes
		NotifyLastFMErrorL(*aError);
		ChangeState(ENone);
		iWebServicesHandshakeTransaction->Cancel();
		iHandshakeTransaction->Cancel();
		iRadioHandshakeTransaction->Cancel();
		}
	}
		
void CMobblerLastFMConnection::TransactionResponseL(CMobblerTransaction* aTransaction, const TDesC8& aResponse)
	{
	if (aTransaction == iHandshakeTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseHandshakeL(aResponse, iSessionID, iNowPlayingURL, iSubmitURL);
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
	else if (aTransaction == iRadioHandshakeTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseRadioHandshakeL(aResponse, iRadioSessionID, iRadioStreamURL, iRadioBaseURL, iRadioBasePath);
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
	else if (aTransaction == iWebServicesHandshakeTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseWebServicesHandshakeL(aResponse, iWebServicesSessionKey);
		CleanupStack::PushL(error);
		HandleHandshakeErrorL(error);
		CleanupStack::PopAndDestroy(error);
		}
	else if (aTransaction == iUpdateTransaction)
		{
		ChangeState(ENone);
		TVersion version;
		TBuf8<255> location;
		CMobblerParser::ParseUpdateResponseL(aResponse, version, location);
		NotifyUpdateResponseL(version, location);
		}
	else if (aTransaction == iRadioSelectStationTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseRadioSelectStationL(aResponse);
		
		if (!error)
			{	
			RequestPlaylistL();
			}
		else
			{
			ChangeState(ENone);
			iRadioPlayer->Stop();
			CleanupStack::PushL(error);
			HandleHandshakeErrorL(error);
			CleanupStack::PopAndDestroy(error);
			}
		}
	else if (aTransaction == iRadioPlaylistTransaction)
		{
		CMobblerRadioPlaylist* playlist;
		CMobblerLastFMError* error = CMobblerParser::ParseRadioPlaylistL(aResponse, playlist);
		
		ChangeState(ENone);
		
		if (!error)
			{
			iRadioPlayer->SetPlaylistL(playlist);
			}
		else
			{
			
			iRadioPlayer->Stop();
			CleanupStack::PushL(error);
			NotifyLastFMErrorL(*error);
			CleanupStack::PopAndDestroy(error);
			}
		}
	else if (aTransaction == iRadioAlbumArtTransaction)
		{
		if (iDownloadingTrack)
			{
			iDownloadingTrack->SetAlbumArtL(aResponse);
			iDownloadingTrack->Release();
			iDownloadingTrack = NULL;
			}
		}
	else if (aTransaction == iSubmitTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseScrobbleResponseL(aResponse);
		
		if (!error)
			{
			// We have done a submission so remove up to the
			// first KMaxSubmitTracks from the queued tracks array
			const TInt KCount(iTrackQueue.Count());
			for (TInt i(Min(KCount - 1, KMaxSubmitTracks - 1)) ; i >= 0 ; --i)
				{
				NotifyTrackSubmittedL(*iTrackQueue[i]);
				
				if (iTrackQueue[i]->Love())
					{
					iLovedTrackQueue.AppendL(iTrackQueue[i]);
					iTrackQueue.Remove(i);
					}
				else
					{
					iTrackQueue[i]->Release();
					iTrackQueue.Remove(i);
					}
				}
			
			SaveTrackQueue();
			DoTrackLoveL();
			}
		else
			{
			CleanupStack::PushL(error);
			if (error->ErrorCode() == CMobblerLastFMError::EBadSession)
				{
				// The session has become invalid so handshake again
				HandshakeL();
				}
			else
				{
				NotifyLastFMErrorL(*error);
				}
			CleanupStack::PopAndDestroy(error);
			}
		}
	else if (aTransaction == iNowPlayingTransaction)
		{
		CMobblerLastFMError* error = CMobblerParser::ParseScrobbleResponseL(aResponse);
		
		if (error)
			{
			CleanupStack::PushL(error);
			if (error->ErrorCode() == CMobblerLastFMError::EBadSession)
				{
				// The session has become invalid so handshake again
				HandshakeL();
				}
			else
				{
				NotifyLastFMErrorL(*error);
				}
			CleanupStack::PopAndDestroy(error);
			}
		else
			{
			// there was no error so try to submit any tracks in the queue
			DoSubmitL();
			}
		}
	else if (aTransaction == iTrackLoveTransaction)
		{
		iLovedTrackQueue[0]->Release();
		iLovedTrackQueue.Remove(0);
		
		// try to submit another loved track
		DoTrackLoveL();
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
	}

void CMobblerLastFMConnection::TransactionFailedL(CMobblerTransaction* aTransaction, const TDesC8& aStatus)
	{
	// fail silently
	if (aTransaction ==	iHandshakeTransaction
			|| aTransaction == iRadioHandshakeTransaction
			|| aTransaction == iWebServicesHandshakeTransaction)
		{
		NotifyCommsErrorL(_L("<connect>"), aStatus);
		
		ChangeState(ENone);
		}
	else if (aTransaction == iRadioSelectStationTransaction
				|| aTransaction == iRadioPlaylistTransaction)
		{
		// tell the radio player to stop
		iRadioPlayer->Stop();
		NotifyCommsErrorL(_L("<radio station/playlist>"), aStatus);
		ChangeState(ENone);
		}
	else if (aTransaction == iSubmitTransaction)
		{
		// This pointer is used to tell if we are already submitting some tracks
		// so it is important to delete and set to NULL when finished
		delete iSubmitTransaction;
		iSubmitTransaction = NULL;
		}
	}

void CMobblerLastFMConnection::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{	
	// it must be a transaction event
	TPtrC8 nextDataPartPtr;
		
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseBodyData:
			aTransaction.Response().Body()->GetNextDataPart(nextDataPartPtr);
			TInt dataSize = aTransaction.Response().Body()->OverallDataSize();
			iRadioPlayer->WriteL(nextDataPartPtr, dataSize);	
			aTransaction.Response().Body()->ReleaseData();
			break;
		case THTTPEvent::EFailed:
			iRadioPlayer->TrackDownloadCompleteL();
			iRadioPlayer->NextTrackL();
			break;
		case THTTPEvent::ESucceeded:
		case THTTPEvent::ECancel:
		case THTTPEvent::EClosed:
			// tell the radio player that the track has finished downloading
			iRadioPlayer->TrackDownloadCompleteL();
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

void CMobblerLastFMConnection::CreateAuthToken(TDes8& aHash, TTimeIntervalSeconds aUnixTimeStamp)
	{
	HBufC8* password = HBufC8::NewLC(iPassword->Length());
	password->Des().Copy(*iPassword);
	HBufC8* passwordHash = MobblerUtility::MD5LC(*password);
	HBufC8* passwordHashAndTimeStamp = HBufC8::NewLC(passwordHash->Length() + 20);
	passwordHashAndTimeStamp->Des().Append(*passwordHash);
	passwordHashAndTimeStamp->Des().AppendNum(aUnixTimeStamp.Int());
	HBufC8* authToken = MobblerUtility::MD5LC(*passwordHashAndTimeStamp);
	aHash.Copy(*authToken);
	CleanupStack::PopAndDestroy(4, password);
	}
	
void CMobblerLastFMConnection::HandshakeL()
	{
	delete iSubmitURL;
	iSubmitURL = NULL;
	delete iNowPlayingURL;
	iNowPlayingURL = NULL;
	delete iSessionID;
	iSessionID = NULL;
	
	TTime now;
	now.UniversalTime();
	TTimeIntervalSeconds unixTimeStamp;
	TTime epoch(TDateTime(1970, EJanuary, 0, 0, 0, 0, 0));
	User::LeaveIfError(now.SecondsFrom(epoch, unixTimeStamp));
	
	HBufC8* authToken = HBufC8::NewLC(255);
	TPtr8 authTokenPtr(authToken->Des());
	CreateAuthToken(authTokenPtr, unixTimeStamp);
	
	HBufC8* path = HBufC8::NewLC(255);
	HBufC8* username = HBufC8::NewLC(iUsername->Length());
	TPtr8 usernamePtr(username->Des());
	usernamePtr.Copy(*iUsername);
	path->Des().AppendFormat(KScrobbleQuery, &usernamePtr, unixTimeStamp.Int(), &authTokenPtr);
	
	CUri8* uri = CUri8::NewL();
	CleanupStack::PushL(uri);
	
	uri->SetComponentL(KScheme, EUriScheme);
	uri->SetComponentL(KScrobbleHost, EUriHost);
	uri->SetComponentL(*path, EUriQuery);
	
	delete iHandshakeTransaction;
	iHandshakeTransaction = CMobblerTransaction::NewL(iHTTPSession, uri->Uri(), *this);
	
	CleanupStack::PopAndDestroy(4, authToken);
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
	TInt openError = file.Open(CCoeEnv::Static()->FsSession(), KTracksFile, EFileRead);
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		const TInt KCount(readStream.ReadInt32L());

		for (TInt i(0) ; i < KCount ; ++i)
			{
			CMobblerTrack* track = CMobblerTrack::NewL(readStream);
			CleanupStack::PushL(track);
			iTrackQueue.AppendL(track);
			CleanupStack::Pop(track);
			}
		
		CleanupStack::PopAndDestroy(&readStream);
		
		const TInt KTrackQueueCount(iTrackQueue.Count());
		for (TInt i(KTrackQueueCount - 1) ; i >= 0  ; --i)
			{
			NotifyTrackQueuedL(*iTrackQueue[i]);
			}
		}
		
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLastFMConnection::SaveTrackQueue()
	{
	CCoeEnv::Static()->FsSession().MkDirAll(KTracksFile);
	
	RFile file;
	CleanupClosePushL(file);
	TInt replaceError = file.Replace(CCoeEnv::Static()->FsSession(), KTracksFile, EFileWrite);
	
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


