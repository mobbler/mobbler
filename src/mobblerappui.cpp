/*
mobblerappui.cpp

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

#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknqueryvaluetext.h> 
#include <aknnotewrappers.h>
#include <w32std.h> 
#include <aknnavi.h> 
#include <akntitle.h> 
#include <aknnavilabel.h> 
#include <akncontext.h> 
#include <aknnavi.h> 
#include <aknnavide.h> 
#include <stringloader.h>
#include <aknquerydialog.h> 
#include <apgcli.h> 
#include <aknsutils.h>

#include <browserlauncher.h>
#include <browseroverriddensettings.h> 

#include <mobbler.rsg>
#include "mobbler.hrh"
#include "mobblerapplication.h"
#include "mobblerappui.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstatusview.h"
#include "mobblermusiclistener.h"
#include "mobblertrack.h"
#include "mobblerradioplayer.h"
#include "mobblerlastfmconnectionobserver.h"
#include "mobblerutility.h"
#include "mobblerstring.h"

_LIT(KVersionNumberDisplay,		"0.3.0");
const TVersion version(0, 3, 0);

_LIT(KSearchURL, "http://astore.amazon.co.uk/mobbler-21/search/203-4425999-4423160?node=25&keywords=%S&x=0&y=0&preview=");

const TUid KBrowserUid = {0x10008D39};

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
	
	iLastFMConnection = CMobblerLastFMConnection::NewL(iSettingView->GetUserName(), iSettingView->GetPassword());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFMConnection);
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFMConnection);
	iLastFMConnection->SetRadioPlayer(*iRadioPlayer);
	iLastFMConnection->AddObserverL(iMusicListener); 
	iLastFMConnection->AddObserverL(this); 
	
	RProcess().SetPriority(EPriorityHigh);
	
#ifndef __WINS__
	iBrowserLauncher = CBrowserLauncher::NewL();
#endif
	}

void CMobblerAppUi::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	iLastFMConnection->SetDetailsL(aUsername, aPassword);
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
	delete iMusicListener;	
	delete iRadioPlayer;
	delete iLastFMConnection;
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
	
	switch (aCommand)
		{
		case EAknSoftkeyExit:
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
			
			TInt error = iLastFMConnection->CheckForUpdatesL();
			
			if (error == KErrNotReady)
				{
				// Ask if they would like to go online
				HBufC* goOnlineText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_GO_ONLINE);
				
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
			
			// initialize the dialog
			dlg->PrepareLC(R_AVKON_MESSAGE_QUERY_DIALOG);
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
				// There is a track playing so open the browser with the amazon link 
				// ask if the user want to open the full amazon site
				
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
		case EMobblerCommandRadioArtist:
			// ask the user for the artist name	
			CAknTextQueryDialog* artistDialog = new(ELeave) CAknTextQueryDialog(artist);
			artistDialog->PrepareLC(R_ARTIST_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_ARTIST);
			artistDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			artistDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (artistDialog->RunLD())
				{
				HBufC8* artist8 = MobblerUtility::URLEncodeLC(artist);
				RadioStartL(CMobblerLastFMConnection::EArtist, *artist8);
				CleanupStack::PopAndDestroy(artist8);
				}
			
			break;
		case EMobblerCommandRadioTag:
			
			// ask the user for the tag
			CAknTextQueryDialog* tagDialog = new(ELeave) CAknTextQueryDialog(tag);
			tagDialog->PrepareLC(R_TAG_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_TAG);
			tagDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			tagDialog->SetPredictiveTextInputPermitted(ETrue);

			if (tagDialog->RunLD())
				{
				HBufC8* tag8 = MobblerUtility::URLEncodeLC(tag);
				RadioStartL(CMobblerLastFMConnection::ETag, *tag8);
				CleanupStack::PopAndDestroy(tag8);
				}

			break;
		case EMobblerCommandRadioUser:
			
			// ask the user for the tag
			CAknTextQueryDialog* userDialog = new(ELeave) CAknTextQueryDialog(user);
			userDialog->PrepareLC(R_USER_DIALOG);
			dialogPromptText = StringLoader::LoadLC(R_MOBBLER_RADIO_ENTER_USER);
			userDialog->SetPromptL(*dialogPromptText);
			CleanupStack::PopAndDestroy(dialogPromptText);
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				HBufC8* user8 = MobblerUtility::URLEncodeLC(user);
				RadioStartL(CMobblerLastFMConnection::EUser, *user8);
				CleanupStack::PopAndDestroy(user8);
				}

			break;
		case EMobblerCommandRadioRecommedations:
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
			else
				{
				// There is no current track so tell the user that they are being silly
				CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
				HBufC* text = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ERROR_NO_TRACK);
				note->ExecuteLD(*text);
				CleanupStack::PopAndDestroy(text);
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
			else
				{
				// There is no current radio track so tell the user they are being silly
				CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
				HBufC* text = iEikonEnv->AllocReadResourceLC(R_MOBBLER_ERROR_NO_RADIO_TRACK);
				note->ExecuteLD(*text);
				CleanupStack::PopAndDestroy(text);
				}
			
			break;
		default:
			break;
		}
	}

void CMobblerAppUi::RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const TDesC8& aRadioOption)
	{
	TInt error = iRadioPlayer->StartL(aRadioStation, aRadioOption);
	
	if (error == KErrNotReady)
		{
		// Ask if they would like to go online
		HBufC* goOnlineText = iEikonEnv->AllocReadResourceLC(R_MOBBLER_GO_ONLINE);
		
		CAknQueryDialog* dlg = CAknQueryDialog::NewL();
		TBool goOnline(dlg->ExecuteLD(R_MOBBLER_QUERY_DIALOG, *goOnlineText));
					
		CleanupStack::PopAndDestroy(goOnlineText);
		
		if (goOnline)
			{
			// send the web services API call
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
			iRadioStation = aRadioStation;
			delete iRadioOption;
			iRadioOption = aRadioOption.AllocL();
			}
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
			HBufC* location16 = HBufC::NewLC(aLocation.Length());
			location16->Des().Copy(aLocation);
#ifndef __WINS__
			iBrowserLauncher->LaunchBrowserEmbeddedL(*location16);
#endif
			CleanupStack::PopAndDestroy(location16);
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

void CMobblerAppUi::HandleConnectCompleteL()
	{
	iStatusView->DrawDeferred();
	
	if (iRadioOption)
		{
		iRadioPlayer->StartL(iRadioStation, *iRadioOption);
		delete iRadioOption;
		iRadioOption = NULL;
		}
	
	if (iCheckForUpdates)
		{
		iLastFMConnection->CheckForUpdatesL();
		iCheckForUpdates = EFalse;
		}
	}
	
void CMobblerAppUi::HandleLastFMErrorL(CMobblerLastFMError& aError)
	{
	iStatusView->DrawDeferred();

	CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
	note->ExecuteLD(aError.Text());
	}

void CMobblerAppUi::HandleCommsErrorL(const TDesC& aTransaction, const TDesC8& aStatus)
	{
	iStatusView->DrawDeferred();
	
	HBufC* noteText = HBufC::NewLC(255);

	HBufC* commsErrorText = StringLoader::LoadLC(R_MOBBLER_NOTE_COMMS_ERROR);
	noteText->Des().Append(*commsErrorText);
	CleanupStack::PopAndDestroy(commsErrorText);
	
	noteText->Des().Append(_L(" "));
	noteText->Des().Append(aTransaction);
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

// End of File
