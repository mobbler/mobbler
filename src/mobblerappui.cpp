/*
mobblerappui.cpp

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

#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <aknsutils.h>
#include <bautils.h> 
#include <browserlauncher.h>
#include <mobbler.rsg>
#include <stringloader.h>

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerdownload.h"
#include "mobblermusiclistener.h"
#include "mobblerradioplayer.h"
#include "mobblerrichtextcontrol.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstatusview.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

_LIT(KVersionNumberDisplay,		"0.3.2");
const TVersion version(0, 3, 2);

_LIT(KSearchURL, "http://astore.amazon.co.uk/mobbler-21/search/203-4425999-4423160?node=25&keywords=%S&x=0&y=0&preview=");

const TUid KBrowserUid = {0x10008D39};

_LIT(KRadioFile, "c:radiostations.dat");

void CMobblerAppUi::ConstructL()
	{
	BaseConstructL(EAknEnableSkin);
	
	AknsUtils::InitSkinSupportL();
	
	// Create view object
	iSettingView = CMobblerSettingItemListView::NewL();
	iStatusView = CMobblerStatusView::NewL();
	
	AddViewL(iSettingView);
	AddViewL(iStatusView);
	ActivateLocalViewL(iStatusView->Id());
	
	iLastFMConnection = CMobblerLastFMConnection::NewL(*this, iSettingView->GetUserName(), iSettingView->GetPassword(), iSettingView->GetIapID(), iSettingView->GetCheckForUpdates());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFMConnection, iSettingView->GetBufferSize());
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFMConnection);
	iLastFMConnection->SetRadioPlayer(*iRadioPlayer);
	
	RProcess().SetPriority(EPriorityHigh);
	
#ifndef __WINS__
	iBrowserLauncher = CBrowserLauncher::NewL();
#endif
	iResumeStationOnConnectCompleteCallback = EFalse;
	LoadRadioStationsL();
	
	iMobblerDownload = CMobblerDownload::NewL();
	}

void CMobblerAppUi::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	iLastFMConnection->SetDetailsL(aUsername, aPassword);
	}

void CMobblerAppUi::SetCheckForUpdatesL(TBool aCheckForUpdates)
	{
	iLastFMConnection->SetCheckForUpdatesL(aCheckForUpdates);
	}

void CMobblerAppUi::SetIapIDL(TUint32 aIapID)
	{
	iLastFMConnection->SetIapIDL(aIapID);
	}

void CMobblerAppUi::SetBufferSize(TTimeIntervalSeconds aBufferSize)
	{
	iRadioPlayer->SetPreBufferSize(aBufferSize);
	}

const CMobblerTrack* CMobblerAppUi::CurrentTrack() const
	{
	const CMobblerTrack* track = iRadioPlayer->CurrentTrack();
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}

	return track;
	}

CMobblerTrack* CMobblerAppUi::CurrentTrack()
	{
	CMobblerTrack* track = iRadioPlayer->CurrentTrack();
	
	if (!track)
		{
		track = iMusicListener->CurrentTrack();
		}

	return track;
	}

CMobblerRadioPlayer* CMobblerAppUi::RadioPlayer() const
	{
	return iRadioPlayer;
	}

const TDesC& CMobblerAppUi::MusicAppNameL() const
	{
	return iMusicListener->MusicAppNameL();
	}

CMobblerAppUi::CMobblerAppUi()
	{
	}

CMobblerAppUi::~CMobblerAppUi()
	{
	delete iRadioOption;
	delete iPreviousRadioArtist;
	delete iPreviousRadioTag;
	delete iPreviousRadioUser;
	delete iMusicListener;	
	delete iRadioPlayer;
	delete iLastFMConnection;
	delete iMobblerDownload;
#ifndef __WINS__
	delete iBrowserLauncher;
#endif
	}

void CMobblerAppUi::HandleCommandL(TInt aCommand)
	{
	TApaTask task(iEikonEnv->WsSession());
	HBufC* dialogPromptText(NULL);
	CMobblerTrack* currentTrack(NULL);
	
	TBuf<250> tag;
	TBuf<250> artist;
	TBuf<250> user;

	// Don't bother going online to Last.fm if no user details entered
	if (aCommand >= EMobblerCommandOnline)
		{
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), KSettingsFile)
			== EFalse)
			{
			CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
			HBufC* errorText = StringLoader::LoadLC(R_MOBBLER_NOTE_NO_DETAILS);
			note->ExecuteLD(*errorText);
			CleanupStack::PopAndDestroy(errorText);

			// bail from the function
			return;
			}
		}

	switch (aCommand)
		{
		case EAknSoftkeyExit:
			// Send application to the background to give the user
			// a sense of a really fast shutdown. Sometimes the thread
			// doesn't shut down instantly, so best to do this without
			// interferring with the user's ability to do something
			// else with their phone.
			task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			/// Check if scrobblable first and save queue
			iLastFMConnection->TrackStoppedL();
			iRadioPlayer->Stop();
			Exit();
			break;
		case EEikCmdExit:
		case EAknSoftkeyBack:
			task.SetWgId( CEikonEnv::Static()->RootWin().Identifier());
			task.SendToBackground();
			break;
		case EMobblerCommandOnline:
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
			break;
		case EMobblerCommandOffline:
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOffline);
			break;
		case EMobblerCommandCheckForUpdates:
			
			TInt error = iLastFMConnection->CheckForUpdateL();
			
			if (error == KErrBadHandle)
				{
				// Ask if they would like to go online
				HBufC* goOnlineText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ASK_GO_ONLINE);
				
				CAknQueryDialog* dlg = CAknQueryDialog::NewL();
				TBool goOnline(dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *goOnlineText));
							
				CleanupStack::PopAndDestroy(goOnlineText);
				
				if (goOnline)
					{
					// send the web services API call
					iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
					iCheckForUpdates = ETrue;
					}
				}
			else if (error == KErrNotReady)
				{
				iCheckForUpdates = ETrue;
				}
			
			break;
		case EMobblerCommandEditSettings:
			ActivateLocalViewL(iSettingView->Id());
			break;
		case EMobblerCommandAbout:
			// create the message text
			HBufC* msg1 = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ABOUT_TEXT1);
			HBufC* msg2 = KVersionNumberDisplay().AllocLC();
			HBufC* msg3 = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ABOUT_TEXT2);
			HBufC* msg = HBufC::NewLC(msg1->Length() + msg2->Length() + msg3->Length());
			msg->Des().Append(*msg1);
			msg->Des().Append(*msg2);
			msg->Des().Append(*msg3);
			
			// create the header text
			HBufC* title = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TITLE);
			CAknMessageQueryDialog* dlg = new(ELeave) CAknMessageQueryDialog();
			
			// initialise the dialog
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(*title);
			dlg->SetMessageTextL(*msg);
			
			dlg->RunLD();
			
			CleanupStack::PopAndDestroy(title);
			CleanupStack::PopAndDestroy(4, msg1); //msg1, msg2, msg3, msg
			
			break;
			
		case EMobblerCommandBuy:
			const CMobblerTrack* track = CurrentTrack();
			
			if (track)
				{
				// There is a track playing so open the browser with the Amazon link.
				// Ask if the user wants to open the full Amazon site.
				
				CAknQueryDialog* dlg = CAknQueryDialog::NewL();
				HBufC* buyAmazon = iEikonEnv->AllocReadResourceLC(R_MOBBLER_BUY_AMAZON);
				TBool yes( dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *buyAmazon));
				CleanupStack::PopAndDestroy(buyAmazon);
				
				if (yes)
					{
					// create the URL encoded search string
					HBufC* searchString = HBufC::NewLC((track->Artist().String().Length() + track->Album().String().Length() + 1) * 4);
					
					for (TInt i(0) ; i < track->Artist().String().Length() ; ++i)
						{
						searchString->Des().AppendFormat(_L("%%%2x"), track->Artist().String()[i]);
						}
					
					searchString->Des().Append(_L("%%20"));
					
					for (TInt i(0) ; i < track->Album().String().Length() ; ++i)
						{
						searchString->Des().AppendFormat(_L("%%%2x"), track->Album().String()[i]);
						}
					
					TPtr searchStringPtr(searchString->Des());
					
					HBufC* fullURL = HBufC::NewLC(searchString->Length() + KSearchURL().Length());
					
					fullURL->Des().AppendFormat(KSearchURL, &searchStringPtr);
					
#ifndef __WINS__
					iBrowserLauncher->LaunchBrowserEmbeddedL(*fullURL);
#endif
					
					CleanupStack::PopAndDestroy(fullURL);
					CleanupStack::PopAndDestroy(searchString);
					}
				}
	
			break;
		case EMobblerCommandResumeRadio:
			if (!RadioResumable())
				{
				break;
				}
			if (iRadioPlayer->HasPlaylist() && 
				iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
				{
				iRadioPlayer->NextTrackL();
				}
			else
				{
				HBufC8* station8(NULL);
				switch (iPreviousRadioStation)
					{
					case CMobblerLastFMConnection::EArtist:
						station8 = MobblerUtility::URLEncodeLC(*iPreviousRadioArtist);
						RadioStartL(iPreviousRadioStation, *station8);
						CleanupStack::PopAndDestroy(station8);
						break;
					case CMobblerLastFMConnection::ETag:
						station8 = MobblerUtility::URLEncodeLC(*iPreviousRadioTag);
						RadioStartL(iPreviousRadioStation, *station8);
						CleanupStack::PopAndDestroy(station8);
						break;
					case CMobblerLastFMConnection::EUser:
						station8 = MobblerUtility::URLEncodeLC(*iPreviousRadioUser);
						RadioStartL(iPreviousRadioStation, *station8);
						CleanupStack::PopAndDestroy(station8);
						break;
					case CMobblerLastFMConnection::ERecommendations: // intentional fall-through
					case CMobblerLastFMConnection::ENeighbourhood:
					case CMobblerLastFMConnection::ELovedTracks:
					case CMobblerLastFMConnection::EMyPlaylist:
					default:
						RadioStartL(iPreviousRadioStation, KNullDesC8);
						break;
					}
				}
			break;
		case EMobblerCommandRadioArtist:
			// ask the user for the artist name	

			if (iPreviousRadioArtist)
				{
				artist = *iPreviousRadioArtist;
				}
			CAknTextQueryDialog* artistDialog = new(ELeave) CAknTextQueryDialog(artist);
			artistDialog->PrepareLC(R_MOBBLER_RADIO_QUERY_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_ARTIST);
			artistDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			artistDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (artistDialog->RunLD())
				{
				HBufC8* artist8 = MobblerUtility::URLEncodeLC(artist);
				RadioStartL(CMobblerLastFMConnection::EArtist, *artist8);
				CleanupStack::PopAndDestroy(artist8);
				delete iPreviousRadioArtist;
				iPreviousRadioArtist = artist.AllocL();
				SaveRadioStationsL();
				}
			
			break;
		case EMobblerCommandRadioTag:
			
			// ask the user for the tag
			if (iPreviousRadioTag)
				{
				tag = *iPreviousRadioTag;
				}
			CAknTextQueryDialog* tagDialog = new(ELeave) CAknTextQueryDialog(tag);
			tagDialog->PrepareLC(R_MOBBLER_RADIO_QUERY_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_TAG);
			tagDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			tagDialog->SetPredictiveTextInputPermitted(ETrue);

			if (tagDialog->RunLD())
				{
				HBufC8* tag8 = MobblerUtility::URLEncodeLC(tag);
				RadioStartL(CMobblerLastFMConnection::ETag, *tag8);
				CleanupStack::PopAndDestroy(tag8);
				delete iPreviousRadioTag;
				iPreviousRadioTag = tag.AllocL();
				SaveRadioStationsL();
				}

			break;
		case EMobblerCommandRadioUser:
			
			// ask the user for the user
			if (iPreviousRadioUser)
				{
				user = *iPreviousRadioUser;
				}
			CAknTextQueryDialog* userDialog = new(ELeave) CAknTextQueryDialog(user);
			userDialog->PrepareLC(R_MOBBLER_RADIO_QUERY_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_USER);
			userDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				HBufC8* user8 = MobblerUtility::URLEncodeLC(user);
				RadioStartL(CMobblerLastFMConnection::EUser, *user8);
				CleanupStack::PopAndDestroy(user8);
				delete iPreviousRadioUser;
				iPreviousRadioUser = user.AllocL();
				SaveRadioStationsL();
				}

			break;
		case EMobblerCommandRadioRecommendations:
			RadioStartL(CMobblerLastFMConnection::ERecommendations, KNullDesC8);
			break;
		case EMobblerCommandRadioPersonal:
			RadioStartL(CMobblerLastFMConnection::EPersonal, KNullDesC8);
			break;
		case EMobblerCommandRadioLoved:
			RadioStartL(CMobblerLastFMConnection::ELovedTracks, KNullDesC8);
			break;
		case EMobblerCommandRadioNeighbourhood:
			RadioStartL(CMobblerLastFMConnection::ENeighbourhood, KNullDesC8);
			break;
		case EMobblerCommandRadioMyPlaylist:
			RadioStartL(CMobblerLastFMConnection::EMyPlaylist, KNullDesC8);
			break;
		case EMobblerCommandTrackLove:
			// you can love either radio or music player tracks
			currentTrack = CurrentTrack();
			
			if (currentTrack)
				{
				if (!currentTrack->Love())
					{
					// There is a current track and it is not already loved
					HBufC* loveText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_LOVE_TRACK);
					
					CAknQueryDialog* dlg = CAknQueryDialog::NewL();
					TBool love(dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *loveText));
								
					CleanupStack::PopAndDestroy(loveText);
					
					if (love)
						{
						// set love to true (if only it were this easy)
						currentTrack->SetLove(ETrue);
						}
					}
				}
			
			break;
		case EMobblerCommandTrackBan:
			// you should only be able to ban radio tracks
			currentTrack = iRadioPlayer->CurrentTrack();
			
			if (currentTrack)
				{
				// There is a current track and it is not already loved
				
				HBufC* banText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_BAN_TRACK);
				
				CAknQueryDialog* dlg = CAknQueryDialog::NewL();
				TBool ban(dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *banText));
							
				CleanupStack::PopAndDestroy(banText);
				
				if (ban)
					{
					// send the web services API call
					iLastFMConnection->TrackBanL(*currentTrack);
					iRadioPlayer->NextTrackL();
					}
				}
			
			break;
		case EMobblerCommandArtistGetInfo:
			currentTrack = CurrentTrack();
			
			if (currentTrack)
				{
				// send the web services API call
				iLastFMConnection->ArtistGetInfoL(*currentTrack, iStatusView->ArtistInfoControlL());
				}
			
			break;
		default:
			break;
		}
	}

void CMobblerAppUi::RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioOption)
	{
	iPreviousRadioStation = aRadioStation;
	TInt error = iRadioPlayer->StartL(aRadioStation, aRadioOption);
	
	TBool startStationOnConnectCompleteCallback(EFalse);
	
	if (error == KErrBadHandle)
		{
		// Ask if they would like to go online
		HBufC* goOnlineText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ASK_GO_ONLINE);
		
		CAknQueryDialog* dlg = CAknQueryDialog::NewL();
		TBool goOnline(dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *goOnlineText));
					
		CleanupStack::PopAndDestroy(goOnlineText);
		
		if (goOnline)
			{
			// Send the web services API call
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);		
			
			startStationOnConnectCompleteCallback = ETrue;
			}
		}
	else if (error == KErrNotReady)
		{
		startStationOnConnectCompleteCallback = ETrue;
		}
			
	if (startStationOnConnectCompleteCallback)
		{
		// Setting these mean that the radio station selected 
		// will be started in HandleConnectCompleteL()
		iRadioStation = aRadioStation;
		delete iRadioOption;
		iRadioOption = aRadioOption.AllocL();
		}
	}
			

TBool CMobblerAppUi::RadioResumable() const
	{
	// Can resume only if the radio is not playing now,
	// and if a previous radio station is known.
	if (!(RadioPlayer()->CurrentTrack()) &&
		(iPreviousRadioStation != CMobblerLastFMConnection::EUnknown))
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

CMobblerLastFMConnection::TMode CMobblerAppUi::Mode() const
	{
	return iLastFMConnection->Mode();
	}

CMobblerLastFMConnection::TState CMobblerAppUi::State() const
	{
	return iLastFMConnection->State();
	}


void CMobblerAppUi::HandleStatusPaneSizeChange()
	{
	}

void CMobblerAppUi::HandleUpdateResponseL(TVersion aVersion, const TDesC8& aLocation)
	{
	if (aVersion.Name().Compare(version.Name()) > 0)
		{
		CAknQueryDialog* dlg = CAknQueryDialog::NewL();
		HBufC* update = iEikonEnv->AllocReadResourceLC(R_MOBBLER_UPDATE);
		TBool yes( dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *update));
		CleanupStack::PopAndDestroy(update);
						
		if (yes)
			{
			iMobblerDownload->DownloadL(aLocation, iLastFMConnection->IapID());
			}
		}
	else
		{
		HBufC* noUpdate = iEikonEnv->AllocReadResourceLC(R_MOBBLER_NO_UPDATE);
		CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
		note->ExecuteLD(*noUpdate);
		CleanupStack::PopAndDestroy(noUpdate);
		}
	}

void CMobblerAppUi::HandleConnectCompleteL(TInt aError)
	{
	iStatusView->DrawDeferred();
	
	if (aError == KErrNone)
		{
		if (iRadioOption)
			{
			iRadioPlayer->StartL(iRadioStation, *iRadioOption);
			delete iRadioOption;
			iRadioOption = NULL;
			}
		else if (iResumeStationOnConnectCompleteCallback)
			{
			iResumeStationOnConnectCompleteCallback = EFalse;
			if (iRadioPlayer->HasPlaylist())
				{
				iRadioPlayer->NextTrackL();
				}
			else
				{
				iRadioPlayer->StartL(iRadioStation, *iRadioOption);
				}
			}
		
		if (iCheckForUpdates)
			{
			iLastFMConnection->CheckForUpdateL();
			iCheckForUpdates = EFalse;
			}
		}
	else
		{
		delete iRadioOption;
		iRadioOption = NULL;
		
		// Tell the user that there was an error connecting
		HBufC* commsErrorText = StringLoader::LoadLC(R_MOBBLER_NOTE_COMMS_ERROR);
		CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
		note->ExecuteLD(*commsErrorText);
		CleanupStack::PopAndDestroy(commsErrorText);
		}
	}
	
void CMobblerAppUi::HandleLastFMErrorL(CMobblerLastFMError& aError)
	{
	iStatusView->DrawDeferred();
	
	CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
	note->ExecuteLD(aError.Text());
	}

void CMobblerAppUi::HandleCommsErrorL(TInt aStatusCode, const TDesC8& aStatus)
	{
	iStatusView->DrawDeferred();
	
	HBufC* noteText = HBufC::NewLC(255);

	HBufC* commsErrorText = StringLoader::LoadLC(R_MOBBLER_NOTE_COMMS_ERROR);
	noteText->Des().Append(*commsErrorText);
	CleanupStack::PopAndDestroy(commsErrorText);
	
	noteText->Des().Append(_L(" "));
	noteText->Des().AppendNum(aStatusCode);
	noteText->Des().Append(_L(" "));
	
	HBufC* status = HBufC::NewLC(aStatus.Length());
	status->Des().Copy(aStatus);
	noteText->Des().Append(*status);
	CleanupStack::PopAndDestroy(status);
	
	CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
	note->ExecuteLD(*noteText);
	
	CleanupStack::PopAndDestroy(noteText);
	}

TInt CMobblerAppUi::Scrobbled() const
	{
	return iTracksSubmitted;
	}

TInt CMobblerAppUi::Queued() const
	{
	return iTracksQueued;
	}

void CMobblerAppUi::HandleTrackNowPlayingL(const CMobblerTrack& /*aTrack*/)
	{
	// Tell the status view that the track has changed
	iStatusView->DrawDeferred();
	}

void CMobblerAppUi::HandleTrackSubmittedL(const CMobblerTrack& /*aTrack*/)
	{
	iStatusView->DrawDeferred();
	++iTracksSubmitted;
	--iTracksQueued;
	}

void CMobblerAppUi::HandleTrackQueuedL(const CMobblerTrack& /*aTrack*/)
	{		
	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}
	// update the track queued count and change the status bar text
	++iTracksQueued;
	}

void CMobblerAppUi::StatusDrawDeferred()
	{
	if (iStatusView)
		{
		iStatusView->DrawDeferred();
		}
	}

void CMobblerAppUi::HandleForegroundEventL(TBool aForeground)
	{
	CAknAppUi::HandleForegroundEventL(aForeground);
	iForeground = aForeground;
	}

TBool CMobblerAppUi::Foreground() const
	{
	return iForeground;
	}

TBool CMobblerAppUi::Backlight() const
	{
	return iSettingView->GetBacklight();
	}

void CMobblerAppUi::LoadRadioStationsL()
	{
	RFile file;
	CleanupClosePushL(file);
	TInt openError = file.Open(CCoeEnv::Static()->FsSession(), KRadioFile, EFileRead);

	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);

		iPreviousRadioStation = (CMobblerLastFMConnection::TRadioStation)readStream.ReadInt32L();

		TBuf<255> radio;
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioArtist;
			iPreviousRadioArtist = radio.AllocL();
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioTag;
			iPreviousRadioTag = radio.AllocL();
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioUser;
			iPreviousRadioUser = radio.AllocL();
			}

		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		iPreviousRadioStation = CMobblerLastFMConnection::EUnknown;
		}

	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerAppUi::SaveRadioStationsL()
	{
	CCoeEnv::Static()->FsSession().MkDirAll(KRadioFile);

	RFile file;
	CleanupClosePushL(file);
	TInt replaceError = file.Replace(CCoeEnv::Static()->FsSession(), KRadioFile, EFileWrite);

	if (replaceError == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);

		writeStream.WriteInt32L(iPreviousRadioStation);

		if (iPreviousRadioArtist)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << *iPreviousRadioArtist;
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioTag)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << *iPreviousRadioTag;
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioUser)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << *iPreviousRadioUser;
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		CleanupStack::PopAndDestroy(&writeStream);
		}

	CleanupStack::PopAndDestroy(&file);
	}

// End of File
