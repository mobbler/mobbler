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
#include <akninfopopupnotecontroller.h>
#include <AknLists.h>
#include <aknnotewrappers.h>
#include <aknsutils.h>
#include <bautils.h> 

#ifndef __WINS__
#include <browserlauncher.h>
#endif

#include <mobbler.rsg>
#include <mobbler_strings.rsg>

#include <sendomfragment.h>
#include <sennamespace.h> 
#include <senxmlutils.h> 

#include "mobbler.hrh"
#include "mobblerappui.h"
#include "mobblerfriendlist.h"
#include "mobblermusiclistener.h"
#include "mobblerparser.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistview.h"
#include "mobblerstatusview.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstring.h"
#include "mobblertrack.h"
#include "mobblerutility.h"
#include "mobblerwebservicesview.h"

_LIT(KRadioFile, "c:radiostations.dat");

const TTimeIntervalMinutes KTweetFetchInterval(30);

enum TSleepTimerAction
	{
	EStopPlaying,
	EGoOffline,
	ExitMobber
	};

void CMobblerAppUi::ConstructL()
	{
	iResourceReader = CMobblerResourceReader::NewL(KLanguageRscFile, KLanguageRscVersion);
	
	iVolumeUpCallBack = TCallBack(CMobblerAppUi::VolumeUpCallBackL, this);
	iVolumeDownCallBack = TCallBack(CMobblerAppUi::VolumeDownCallBackL, this);
	
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
		
	BaseConstructL(EAknEnableSkin);
	
	AknsUtils::InitSkinSupportL();
	
	// Create view object
	iSettingView = CMobblerSettingItemListView::NewL();
	iStatusView = CMobblerStatusView::NewL();
	
	AddViewL(iSettingView);
	AddViewL(iStatusView);
	ActivateLocalViewL(iStatusView->Id());
	
	iLastFMConnection = CMobblerLastFMConnection::NewL(*this, iSettingView->Username(), iSettingView->Password(), iSettingView->IapID());
	iRadioPlayer = CMobblerRadioPlayer::NewL(*iLastFMConnection, iSettingView->BufferSize(), iSettingView->EqualizerIndex(), iSettingView->Volume());
	iMusicListener = CMobblerMusicAppListener::NewL(*iLastFMConnection);
	
	RProcess().SetPriority(EPriorityHigh);
	
#ifndef __WINS__
	iBrowserLauncher = CBrowserLauncher::NewL();
#endif
	LoadRadioStationsL();
	
	iMobblerDownload = CMobblerDownload::NewL(*this);

	iSleepTimer = CMobblerSleepTimer::NewL(EPriorityLow, *this);
	iSleepAction = EGoOffline;

	iWebServicesView = CMobblerWebServicesView::NewL();
	AddViewL(iWebServicesView);
	
	iLastFMConnection->SetModeL(iSettingView->Mode());
	}

TInt CMobblerAppUi::VolumeUpCallBackL(TAny *aSelf)
	{
	CMobblerAppUi* self = static_cast<CMobblerAppUi*>(aSelf);
	
	self->iRadioPlayer->VolumeUp();
	
	if (self->iStatusView->StatusControl())
		{
		self->iStatusView->StatusControl()->VolumeChanged();
		}
	
	return KErrNone;
	}

TInt CMobblerAppUi::VolumeDownCallBackL(TAny *aSelf)
	{
	CMobblerAppUi* self = static_cast<CMobblerAppUi*>(aSelf);
	
	self->iRadioPlayer->VolumeDown();
	
	if (self->iStatusView->StatusControl())
		{
		self->iStatusView->StatusControl()->VolumeChanged();
		}
	
	return KErrNone;
	}

void CMobblerAppUi::MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct)
	{
	// don't bother if there's a current music player track
	if ((CurrentTrack() && 
		(CurrentTrack()->RadioAuth().Compare(KNullDesC8) == 0)))
		{
		return;
		}
		
	TRequestStatus status;
	TTimeIntervalMicroSeconds32 repeatDelay;
	TTimeIntervalMicroSeconds32 repeatInterval;
	
	switch (aOperationId)
		{
		case ERemConCoreApiStop:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				iRadioPlayer->Stop();
				}
			iCoreTarget->StopResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiForward:
			{
			if (aButtonAct == ERemConCoreApiButtonClick)
				{
				if (iRadioPlayer->CurrentTrack())
					{
					iRadioPlayer->NextTrackL();
					}
				}
			iCoreTarget->ForwardResponse(status, KErrNone);
			User::WaitForRequest(status);
			break;
			}
		case ERemConCoreApiVolumeUp:
			{   
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					iRadioPlayer->VolumeUp();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					break;
				case ERemConCoreApiButtonPress:
					iRadioPlayer->VolumeUp();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					delete iVolumeUpTimer;
					iVolumeUpTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeUpTimer->Start(repeatDelay, repeatInterval, iVolumeUpCallBack);
					break;
				case ERemConCoreApiButtonRelease:
					delete iVolumeUpTimer;
					iVolumeUpTimer = NULL;
					break;
				default:
					break;
				}
			
			iCoreTarget->VolumeUpResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}   
		case ERemConCoreApiVolumeDown:
			{
			switch(aButtonAct)
				{
				case ERemConCoreApiButtonClick:
					iRadioPlayer->VolumeDown();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					break;
				case ERemConCoreApiButtonPress:
					iRadioPlayer->VolumeDown();
					if (iStatusView->StatusControl())
						{
						iStatusView->StatusControl()->VolumeChanged();
						}
					
					iEikonEnv->WsSession().GetKeyboardRepeatRate(repeatDelay, repeatInterval);
					delete iVolumeDownTimer;
					iVolumeDownTimer = CPeriodic::New(CActive::EPriorityStandard);
					iVolumeDownTimer->Start(TTimeIntervalMicroSeconds32(repeatDelay), TTimeIntervalMicroSeconds32(repeatInterval), iVolumeDownCallBack);
					break;
				case ERemConCoreApiButtonRelease:
					delete iVolumeDownTimer;
					iVolumeDownTimer = NULL;
					break;
				default:
					break;
				}
			
			iCoreTarget->VolumeDownResponse(status, KErrNone);
			User::WaitForRequest(status);   
			break;
			}
		default:
			break;
		}
	}

void CMobblerAppUi::SetDetailsL(const TDesC& aUsername, const TDesC& aPassword)
	{
	iLastFMConnection->SetDetailsL(aUsername, aPassword);
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

CMobblerLastFMConnection& CMobblerAppUi::LastFMConnection() const
	{
	return *iLastFMConnection;
	}

CMobblerRadioPlayer& CMobblerAppUi::RadioPlayer() const
	{
	return *iRadioPlayer;
	}

CMobblerMusicAppListener& CMobblerAppUi::MusicListener() const
	{
	return *iMusicListener;
	}

CMobblerSettingItemListView& CMobblerAppUi::SettingView() const
	{
	return *iSettingView;
	}

const TDesC& CMobblerAppUi::MusicAppNameL() const
	{
	return iMusicListener->MusicAppNameL();
	}

CMobblerAppUi::CMobblerAppUi()
	{
	iTweetFetched.UniversalTime();
	iTweetFetched -= KTweetFetchInterval;
	}

CMobblerAppUi::~CMobblerAppUi()
	{	
	delete iPreviousRadioArtist;
	delete iPreviousRadioTag;
	delete iPreviousRadioPersonal;
	delete iMusicListener;
	delete iRadioPlayer;
	delete iLastFMConnection;
	delete iMobblerDownload;
	delete iInterfaceSelector;
	delete iVolumeUpTimer;
	delete iVolumeDownTimer;
	
#ifndef __WINS__
	delete iBrowserLauncher;
#endif
	delete iResourceReader;
	delete iSleepTimer;
	
	delete iTweetText;
	delete iTweetTime;
	delete iTweetPopupNote;
	}

void CMobblerAppUi::HandleInstallStartedL()
	{
	RunAppShutter();
	}

void CMobblerAppUi::HandleCommandL(TInt aCommand)
	{
	// There has been some activity so try to update the tweet
	if (iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline &&
			iLastFMConnection->State() != CMobblerLastFMConnection::EHandshaking && iLastFMConnection->State() != CMobblerLastFMConnection::EConnecting &&
			iState != EFetchingTweet && iState != ECheckingUpdates)
		{
		// We are online and not doing any connecting, already fetching a tweet, or checking for updates
		
		// We check for a new tweet every time there is user input and KTweetFetchInterval has passed

		TTime now;
		now.UniversalTime();
		
		TTimeIntervalHours hoursSinceLastTweetFetch;
		User::LeaveIfError(now.HoursFrom(iTweetFetched, hoursSinceLastTweetFetch));
		
		if (hoursSinceLastTweetFetch >= KTweetFetchInterval)
			{
			iLastFMConnection->GetLatestTweetL(*this);
			iState = EFetchingTweet;
			}
		}
	
	
	TApaTask task(iEikonEnv->WsSession());
	const CMobblerTrack* const currentTrack(CurrentTrack());
	const CMobblerTrack* const currentRadioTrack(iRadioPlayer->CurrentTrack());
	
	TBuf<255> tag;
	TBuf<255> artist;
	TBuf<255> user;

	// Don't bother going online to Last.fm if no user details entered
	if (aCommand >= EMobblerCommandOnline)
		{
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), KSettingsFile)
			== EFalse)
			{
			CAknInformationNote* note = new (ELeave) CAknInformationNote(EFalse);
			note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_NO_DETAILS));

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
			iSettingView->SetModeL(CMobblerLastFMConnection::EOnline);
			break;
		case EMobblerCommandOffline:
			iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOffline);
			iSettingView->SetModeL(CMobblerLastFMConnection::EOffline);
			break;
		case EMobblerCommandFriends:
		case EMobblerCommandUserTopArtists:
		case EMobblerCommandRecommendedArtists:
		case EMobblerCommandRecommendedEvents:
		case EMobblerCommandUserTopAlbums:
		case EMobblerCommandUserTopTracks:
		case EMobblerCommandPlaylists:
		case EMobblerCommandUserEvents:
		case EMobblerCommandUserTopTags:
		case EMobblerCommandRecentTracks:
		case EMobblerCommandUserShoutbox:
		
			if (iLastFMConnection->Mode() != CMobblerLastFMConnection::EOnline && GoOnlineL())
				{
				iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
				}
				
			if (iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
				{
				CMobblerString* username = CMobblerString::NewL(iSettingView->Username());
				CleanupStack::PushL(username);
				ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(aCommand), username->String8());
				CleanupStack::PopAndDestroy(username);
				}
				
			break;
		case EMobblerCommandCheckForUpdates:
			iLastFMConnection->CheckForUpdateL(*this);
			iState = ECheckingUpdates;
			break;
		case EMobblerCommandEditSettings:
			ActivateLocalViewL(iSettingView->Id());
			break;
		case EMobblerCommandAbout:
			{
			// create the message text
			const TDesC& aboutText1 = iResourceReader->ResourceL(R_MOBBLER_ABOUT_TEXT1);
			const TDesC& aboutText2 = iResourceReader->ResourceL(R_MOBBLER_ABOUT_TEXT2);
			
			HBufC* msg = HBufC::NewLC(aboutText1.Length() + KVersion.Name().Length() + aboutText2.Length());
			
			msg->Des().Append(aboutText1);
			msg->Des().Append(KVersion.Name());
			msg->Des().Append(aboutText2);
			
			// create the header text
			CAknMessageQueryDialog* dlg = new(ELeave) CAknMessageQueryDialog();
			
			// initialise the dialog
			dlg->PrepareLC(R_MOBBLER_ABOUT_BOX);
			dlg->QueryHeading()->SetTextL(iResourceReader->ResourceL(R_ABOUT_DIALOG_TITLE));
			dlg->SetMessageTextL(*msg);
			
			dlg->RunLD();
			
			CleanupStack::PopAndDestroy(msg);
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
				switch (iPreviousRadioStation)
					{
					case CMobblerLastFMConnection::EArtist:
						RadioStartL(iPreviousRadioStation, iPreviousRadioArtist);
						break;
					case CMobblerLastFMConnection::ETag:
						RadioStartL(iPreviousRadioStation, iPreviousRadioTag);
						break;
					case CMobblerLastFMConnection::EPersonal:
						if (iPreviousRadioPersonal)
							{
							RadioStartL(iPreviousRadioStation, iPreviousRadioPersonal);
							}
						else
							{
							RadioStartL(iPreviousRadioStation, NULL);
							}
						break;
					case CMobblerLastFMConnection::ERecommendations: // intentional fall-through
					case CMobblerLastFMConnection::ENeighbourhood:
					case CMobblerLastFMConnection::ELovedTracks:
					case CMobblerLastFMConnection::EPlaylist:
					default:
						RadioStartL(iPreviousRadioStation, NULL);
						break;
					}
				}
			break;
		case EMobblerCommandRadioArtist:
			{
			if (!RadioStartable())
				{
				break;
				}

			// ask the user for the artist name	
			if (iPreviousRadioArtist)
				{
				artist = iPreviousRadioArtist->String();
				}
			CAknTextQueryDialog* artistDialog = new(ELeave) CAknTextQueryDialog(artist);
			artistDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			artistDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_ARTIST));
			artistDialog->SetPredictiveTextInputPermitted(ETrue);
			
			if (artistDialog->RunLD())
				{
				CMobblerString* artistString = CMobblerString::NewL(artist);
				CleanupStack::PushL(artistString);
				RadioStartL(CMobblerLastFMConnection::EArtist, artistString);
				delete iPreviousRadioArtist;
				iPreviousRadioArtist = artistString;
				CleanupStack::Pop(artistString);
				SaveRadioStationsL();
				}
			}
			break;
		case EMobblerCommandRadioTag:
			{
			if (!RadioStartable())
				{
				break;
				}
			
			// ask the user for the tag
			if (iPreviousRadioTag)
				{
				tag = iPreviousRadioTag->String();
				}
			
			CAknTextQueryDialog* tagDialog = new(ELeave) CAknTextQueryDialog(tag);
			tagDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			tagDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_TAG));
			tagDialog->SetPredictiveTextInputPermitted(ETrue);

			if (tagDialog->RunLD())
				{
				CMobblerString* tagString = CMobblerString::NewL(tag);
				CleanupStack::PushL(tagString);
				RadioStartL(CMobblerLastFMConnection::ETag, tagString);
				delete iPreviousRadioTag;
				iPreviousRadioTag = tagString;
				CleanupStack::Pop(tagString);
				SaveRadioStationsL();
				}
			}
			break;
		case EMobblerCommandRadioUser:
			{
			if (!RadioStartable())
				{
				break;
				}
			
			// ask the user for the user
			if (iPreviousRadioPersonal)
				{
				user = iPreviousRadioPersonal->String();
				}
			
			CAknTextQueryDialog* userDialog = new(ELeave) CAknTextQueryDialog(user);
			userDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			userDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_RADIO_ENTER_USER));
			userDialog->SetPredictiveTextInputPermitted(ETrue);

			if (userDialog->RunLD())
				{
				CMobblerString* userString = CMobblerString::NewL(user);
				CleanupStack::PushL(userString);
				RadioStartL(CMobblerLastFMConnection::EPersonal, userString);
				delete iPreviousRadioPersonal;
				iPreviousRadioPersonal = userString;
				CleanupStack::Pop(userString);
				SaveRadioStationsL();
				}
			}
			break;
		case EMobblerCommandRadioRecommendations:
			RadioStartL(CMobblerLastFMConnection::ERecommendations, NULL);
			SaveRadioStationsL();
			break;
		case EMobblerCommandRadioPersonal:
			RadioStartL(CMobblerLastFMConnection::EPersonal, NULL);
			SaveRadioStationsL();
			break;
		case EMobblerCommandRadioLoved:
			RadioStartL(CMobblerLastFMConnection::ELovedTracks, NULL);
			SaveRadioStationsL();
			break;
		case EMobblerCommandRadioNeighbourhood:
			RadioStartL(CMobblerLastFMConnection::ENeighbourhood, NULL);
			SaveRadioStationsL();
			break;
		case EMobblerCommandRadioPlaylist:
			RadioStartL(CMobblerLastFMConnection::EPlaylist, NULL);
			SaveRadioStationsL();
			break;
		case EMobblerCommandTrackLove:
			// you can love either radio or music player tracks
			
			if (currentTrack)
				{
				if (!currentTrack->Love())
					{
					// There is a current track and it is not already loved
					CAknQueryDialog* dlg = CAknQueryDialog::NewL();
					TBool love(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_LOVE_TRACK)));
					
					if (love)
						{
						// set love to true (if only it were this easy)
						CurrentTrack()->SetLove(ETrue);
						iLastFMConnection->TrackLoveL(currentTrack->Artist().String8(), currentTrack->Title().String8());
						}
					}
				}
			
			break;
		case EMobblerCommandTrackBan:
			// you should only be able to ban radio tracks
			if (currentRadioTrack)
				{
				// There is a current track and it is not already loved
				CAknQueryDialog* dlg = CAknQueryDialog::NewL();
				TBool ban(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_BAN_TRACK)));
				
				if (ban)
					{
					// send the web services API call
					iLastFMConnection->TrackBanL(currentRadioTrack->Artist().String8(), currentRadioTrack->Title().String8());
					iRadioPlayer->NextTrackL();
					}
				}
			
			break;
		case EMobblerCommandPlus:
			
			if (currentTrack)
				{
				CAknSinglePopupMenuStyleListBox* list = new(ELeave) CAknSinglePopupMenuStyleListBox;
			    CleanupStack::PushL(list);
			     
			    CAknPopupList* popup = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);
			    CleanupStack::PushL(popup);
			    
			    list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);

			    popup->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_CURRENT_TRACK));
			    
			    list->CreateScrollBarFrameL(ETrue);
			    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
			    
			    CDesCArrayFlat* items = new(ELeave) CDesCArrayFlat(1);
			    CleanupStack::PushL(items);
			    
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SHARE_TRACK));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SHARE_ARTIST));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_PLAYLIST_ADD_TRACK));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SIMILAR_ARTISTS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SIMILAR_TRACKS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_EVENTS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_ARTIST_SHOUTBOX));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_ALBUMS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_TRACKS));
			    items->AppendL(iResourceReader->ResourceL(R_MOBBLER_TOP_TAGS));


			    CleanupStack::Pop(items);
			    
			    list->Model()->SetItemTextArray(items);
			    list->Model()->SetOwnershipType(ELbmOwnsItemArray);
			    
			    CleanupStack::Pop(popup); //popup
			    
			    if (popup->ExecuteLD())
			    	{
			    	if (iLastFMConnection->Mode() != CMobblerLastFMConnection::EOnline && GoOnlineL())
			    		{
			    		iLastFMConnection->SetModeL(CMobblerLastFMConnection::EOnline);
			    		}
			    	
			    	if (iLastFMConnection->Mode() == CMobblerLastFMConnection::EOnline)
			    		{
				    	switch (list->CurrentItemIndex())
				    		{
				    		case 0:
				    			{
				    			// Share
				    			CMobblerString* username = CMobblerString::NewL(iSettingView->Username());
				    			CleanupStack::PushL(username);
				    			iLastFMConnection->WebServicesCallL(_L8("user"), _L8("getfriends"), username->String8(), *this);
				    			CleanupStack::PopAndDestroy(username);
				    			
				    			iState = EFetchingFriendsShareTrack;
				    			
					    		break;
				    			}
				    		case 1:
				    			{
				    			// Share
				    			CMobblerString* username = CMobblerString::NewL(iSettingView->Username());
				    			CleanupStack::PushL(username);
				    			iLastFMConnection->WebServicesCallL(_L8("user"), _L8("getfriends"), username->String8(), *this);
				    			CleanupStack::PopAndDestroy(username);
				    			
				    			iState = EFetchingFriendsShareArtist;
				    			
					    		break;
				    			}
					    	case 2:
					    		{
					    		// Playlists
					    		CMobblerString* username = CMobblerString::NewL(iSettingView->Username());
					    		CleanupStack::PushL(username);
					    		iLastFMConnection->WebServicesCallL(_L8("user"), _L8("getplaylists"), username->String8(), *this);
					    		CleanupStack::PopAndDestroy(username);
					    		
					    		iState = EFetchingPlaylists;
					    		
					    		break;
					    		}
				    		case 3:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarArtists), currentTrack->Artist().String8());
				    			break;
				    		case 4:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandSimilarTracks), currentTrack->MbTrackId().String8());
				    			break;
				    		case 5:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistEvents), currentTrack->Artist().String8());
				    			break;
				    		case 6:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistShoutbox), currentTrack->Artist().String8());
				    			break;
				    		case 7:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopAlbums), currentTrack->Artist().String8());		    			
				    			break;
				    		case 8:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTracks), currentTrack->Artist().String8()); 			
				    			break;
				    		case 9:
				    			ActivateLocalViewL(iWebServicesView->Id(), TUid::Uid(EMobblerCommandArtistTopTags), currentTrack->Artist().String8()); 	
				    			break;
				    		default:
				    			break;
				    		}
			    		}
			    	}
			     
			    CleanupStack::PopAndDestroy(list); //list
				}
			
			break;
		case EMobblerCommandToggleScrobbling:
			iLastFMConnection->ToggleScrobbling();
			break;
		case EMobblerCommandSleepTimer:
			SetSleepTimer();
			break;
		case EMobblerCommandExportQueueToLogFile:
			{
			if (iTracksQueued == 0)
				{
				CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
				note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_EXPORT_EMPTY_QUEUE));
				}
			else
				{
				TBool okToReplaceLog(ETrue);
				
				if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), KLogFile))
					{
					CAknQueryDialog* dlg = CAknQueryDialog::NewL();
					okToReplaceLog = dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_CONFIRM_REPLACE_LOG));
					}
				
				if (okToReplaceLog)
					{
					if (!iLastFMConnection->ExportQueueToLogFileL())
						{
						BaflUtils::DeleteFile(CCoeEnv::Static()->FsSession(), KLogFile);
						}
					
					CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
					note->ExecuteLD(iLastFMConnection->ExportQueueToLogFileL()?
										iResourceReader->ResourceL(R_MOBBLER_NOTE_QUEUE_EXPORTED):
										iResourceReader->ResourceL(R_MOBBLER_NOTE_QUEUE_NOT_EXPORTED));
					}
				}
			}
			break;
		default:
			if (aCommand >= EMobblerCommandEqualizerDefault && 
				aCommand <= EMobblerCommandEqualizerMaximum)
				{
				TInt index = aCommand - EMobblerCommandEqualizerDefault - 1;
				RadioPlayer().SetEqualizer(index);
				iSettingView->SetEqualizerIndexL(index);
				return;
				}
			break;
		}
	}

void CMobblerAppUi::RadioStartL(CMobblerLastFMConnection::TRadioStation aRadioStation, const CMobblerString* aRadioOption)
	{
	iPreviousRadioStation = aRadioStation;
	
	if (!RadioStartable())
		{
		return;
		}

	iRadioPlayer->StartL(aRadioStation, aRadioOption);
	}
			
TBool CMobblerAppUi::RadioStartable() const
	{
	// Can start only if the music player isn't already playing.
	if (iMusicListener->IsPlaying())
		{
		// Tell the user that there was an error connecting
		CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_STOP_MUSIC_PLAYER));

		return EFalse;
		}
	else
		{
		return ETrue;
		}
	}

TBool CMobblerAppUi::RadioResumable() const
	{
	// Can resume only if the radio is not playing now,
	// and if the music player isn't currently playing (paused is ok),
	// and if a previous radio station is known.
	if (!iRadioPlayer->CurrentTrack() &&
		!iMusicListener->IsPlaying() &&
		iPreviousRadioStation != CMobblerLastFMConnection::EUnknown)
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

TBool CMobblerAppUi::ScrobblingOn() const
	{
	return iLastFMConnection->ScrobblingOn();
	}

void CMobblerAppUi::HandleStatusPaneSizeChange()
	{
	}

void CMobblerAppUi::DataL(const TDesC8& aData, TInt aError)
	{
	switch (iState)
		{
		case ECheckingUpdates:
			{
			if (aError == KErrNone)
				{
				// we have just sucessfully checked for updates
				// so don't do it again for another week
				TTime now;
				now.UniversalTime();
				now += TTimeIntervalDays(KUpdateIntervalDays);
				iSettingView->SetNextUpdateCheckL(now);
				
				TVersion version;
				TBuf8<255> location;
				TInt error = CMobblerParser::ParseUpdateResponseL(aData, version, location);
				
				if (error == KErrNone)
					{
					if (version.Name().Compare(version.Name()) > 0)
						{
						CAknQueryDialog* dlg = CAknQueryDialog::NewL();
						TBool yes( dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_UPDATE)));
										
						if (yes)
							{
							iMobblerDownload->DownloadL(location, iLastFMConnection->IapID());
							}
						}
					else
						{
						CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
						note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NO_UPDATE));
						}
					}
				}
			}
			break;
		case EFetchingTweet:
			{
			iTweetFetched.UniversalTime();
			
			// create the xml reader and dom fragement and associate them with each other 
		    CSenXmlReader* xmlReader = CSenXmlReader::NewL();
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment = CSenDomFragment::NewL();
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			// parse the xml into the dom fragment
			xmlReader->ParseL(aData);			
			
			CSenElement* textElement = domFragment->AsElement().Element(_L8("text"));
			
			if (textElement)
				{
				CMobblerString* tweetString = CMobblerString::NewL(textElement->Content());
				CleanupStack::PushL(tweetString);
				HBufC* tweetText = tweetString->String().AllocL();
				CleanupStack::PopAndDestroy(tweetString);
				CleanupStack::PushL(tweetText);
				
				if (!iTweetText
						|| iTweetText && iTweetText->Compare(*tweetText) != 0)
					{
					// We haven't fetched a tweet yet or the new tweet is different
					delete iTweetText;
					iTweetText = tweetText->AllocL();
					
					
					CMobblerString* timeString = CMobblerString::NewL(domFragment->AsElement().Element(_L8("created_at"))->Content());
					CleanupStack::PushL(timeString);
					
					TMonth month;
					TInt day;
					TInt hours;
					TInt minutes;
					TInt seconds;
					TInt utcOffset;
					TInt year;

					// Get all the seperate components of the time
					TPtrC monthString = timeString->String().Mid(4, 3);
					TPtrC dayString = timeString->String().Mid(8, 2);
					TPtrC hoursString = timeString->String().Mid(11, 2);
					TPtrC minutesString = timeString->String().Mid(14, 2);
					TPtrC secondsString = timeString->String().Mid(17, 2);
					TPtrC utcOffsetSignString = timeString->String().Mid(20, 1);
					TPtrC utcOffsetString = timeString->String().Mid(21, 4);
					TPtrC yearString = timeString->String().Right(4);
					
					TLex lex(dayString);
					lex.Val(day);
					lex.Assign(hoursString);
					lex.Val(hours);
					lex.Assign(minutesString);
					lex.Val(minutes);
					lex.Assign(secondsString);
					lex.Val(seconds);
					lex.Assign(utcOffsetString);
					lex.Val(utcOffset);
					lex.Assign(yearString);
					lex.Val(year);
					
					if (monthString.CompareF(_L("Jan")) == 0) month = EJanuary;
					else if (monthString.CompareF(_L("Feb")) == 0) month = EFebruary;
					else if (monthString.CompareF(_L("Mar")) == 0) month = EMarch;
					else if (monthString.CompareF(_L("Apr")) == 0) month = EApril;
					else if (monthString.CompareF(_L("May")) == 0) month = EMay;
					else if (monthString.CompareF(_L("Jun")) == 0) month = EJune;
					else if (monthString.CompareF(_L("Jul")) == 0) month = EJuly;
					else if (monthString.CompareF(_L("Aug")) == 0) month = EAugust;
					else if (monthString.CompareF(_L("Sep")) == 0) month = ESeptember;
					else if (monthString.CompareF(_L("Oct")) == 0) month = EOctober;
					else if (monthString.CompareF(_L("Nov")) == 0) month = ENovember;
					else if (monthString.CompareF(_L("Dec")) == 0) month = EDecember;
					
					TTime tweetTimeLocal = TTime(TDateTime(year, month, day - 1, hours, minutes, seconds, 0))
					+ TTimeIntervalMinutes((utcOffsetSignString.Compare(_L("+")) == 0) ? -utcOffset : utcOffset)
					+ User::UTCOffset();
					
					delete iTweetTime;
					iTweetTime = HBufC::NewL(30);
					TPtr tweetTimePtr = iTweetTime->Des();
					tweetTimeLocal.FormatL(tweetTimePtr, KFormatTime);
					
					CleanupStack::PopAndDestroy(timeString);
					
					iStatusView->DrawDeferred();
					
					_LIT(KTweetFormat, "Mobbler on Twitter:\n%S\n%S");
					
					TBuf<255> tweetText;
					tweetText.Format(KTweetFormat, &iTweetText->Des(), &iTweetTime->Des());
					
					// display the tweet in an info popup note
				    delete iTweetPopupNote;
				    iTweetPopupNote = CAknInfoPopupNoteController::NewL();
				    iTweetPopupNote->SetTextL(tweetText);
				    iTweetPopupNote->ShowInfoPopupNote();
					}
				
				CleanupStack::PopAndDestroy(tweetText);
				}
			
			CleanupStack::PopAndDestroy(2, xmlReader);
			}
			break;
		case EFetchingFriendsShareArtist:
		case EFetchingFriendsShareTrack:
			{
			// parse and bring up a share with friends popup menu
			
			CAknSinglePopupMenuStyleListBox* list = new(ELeave) CAknSinglePopupMenuStyleListBox;
		    CleanupStack::PushL(list);
		     
		    CAknPopupList* popup = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);
		    CleanupStack::PushL(popup);
		    
		    list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);

		    popup->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_SHARE));
		    
		    list->CreateScrollBarFrameL(ETrue);
		    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
		    
		    CDesCArrayFlat* items = new(ELeave) CDesCArrayFlat(1);
		    CleanupStack::PushL(items);
		    
			// create the xml reader and dom fragement and associate them with each other 
		    CSenXmlReader* xmlReader = CSenXmlReader::NewL();
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment = CSenDomFragment::NewL();
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			// parse the xml into the dom fragment
			xmlReader->ParseL(aData);
			
			RPointerArray<CSenElement>& users = domFragment->AsElement().Element(_L8("friends"))->ElementsL();
				
			const TInt KUserCount(users.Count());
			for (TInt i(0) ; i < KUserCount; ++i)
				{
				CMobblerString* user = CMobblerString::NewL(users[i]->Element(_L8("name"))->Content());
				CleanupStack::PushL(user);
				items->AppendL(user->String());
				CleanupStack::PopAndDestroy(user);
				}
			
			CleanupStack::PopAndDestroy(2, xmlReader);

		    CleanupStack::Pop(items);
		    
		    list->Model()->SetItemTextArray(items);
		    list->Model()->SetOwnershipType(ELbmOwnsItemArray);
		    
		    CleanupStack::Pop(popup); //popup
		    
		    if (popup->ExecuteLD())
		    	{
		    	TBuf<255> message;
		    	
		    	CAknTextQueryDialog* shoutDialog = new(ELeave) CAknTextQueryDialog(message);
		    	shoutDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
		    	shoutDialog->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_SHARE));
		    	shoutDialog->SetPredictiveTextInputPermitted(ETrue);

		    	if (shoutDialog->RunLD())
		    		{
		    		CMobblerString* messageString = CMobblerString::NewL(message);
		    		CleanupStack::PushL(messageString);
		    		
			    	CMobblerString* user = CMobblerString::NewL((*items)[list->CurrentItemIndex()]);
					CleanupStack::PushL(user);
					
					if (iState == EFetchingFriendsShareTrack)
						{
						iLastFMConnection->TrackShareL(user->String8(), CurrentTrack()->Artist().String8(), CurrentTrack()->Title().String8(), messageString->String8());
						}
					else
						{
						iLastFMConnection->ArtistShareL(user->String8(), CurrentTrack()->Artist().String8(), messageString->String8());
						}
					
					CleanupStack::PopAndDestroy(2, messageString);
		    		}
		    	}
		     
		    CleanupStack::PopAndDestroy(list); //list
			
			break;
			}
		case EFetchingPlaylists:
			{
			// parse and bring up an add to playlist popup menu
			// create the xml reader and dom fragement and associate them with each other 
		    CSenXmlReader* xmlReader = CSenXmlReader::NewL();
			CleanupStack::PushL(xmlReader);
			CSenDomFragment* domFragment = CSenDomFragment::NewL();
			CleanupStack::PushL(domFragment);
			xmlReader->SetContentHandler(*domFragment);
			domFragment->SetReader(*xmlReader);
			
			CAknSinglePopupMenuStyleListBox* list = new(ELeave) CAknSinglePopupMenuStyleListBox;
		    CleanupStack::PushL(list);
		     
		    CAknPopupList* popup = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);
		    CleanupStack::PushL(popup);
		    
		    list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);

		    popup->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_PLAYLIST_ADD_TRACK));
		    
		    list->CreateScrollBarFrameL(ETrue);
		    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
		    
		    CDesCArrayFlat* items = new(ELeave) CDesCArrayFlat(1);
		    CleanupStack::PushL(items);
		    
			
			
			// parse the xml into the dom fragment
			xmlReader->ParseL(aData);
			
			RPointerArray<CSenElement>& playlists = domFragment->AsElement().Element(_L8("playlists"))->ElementsL();
				
			const TInt KPlaylistCount(playlists.Count());
			for (TInt i(0) ; i < KPlaylistCount; ++i)
				{
				CMobblerString* playlist = CMobblerString::NewL(playlists[i]->Element(_L8("title"))->Content());
				CleanupStack::PushL(playlist);
				items->AppendL(playlist->String());
				CleanupStack::PopAndDestroy(playlist);
				}
			
		    CleanupStack::Pop(items);
		    
		    list->Model()->SetItemTextArray(items);
		    list->Model()->SetOwnershipType(ELbmOwnsItemArray);
		    
		    CleanupStack::Pop(popup); //popup
		    
		    if (popup->ExecuteLD())
		    	{
				iLastFMConnection->PlaylistAddTrackL(playlists[list->CurrentItemIndex()]->Element(_L8("id"))->Content(), CurrentTrack()->Artist().String8(), CurrentTrack()->Title().String8());
		    	}
		     
		    CleanupStack::PopAndDestroy(list); //list
		    
		    CleanupStack::PopAndDestroy(2, xmlReader);
			
			break;
			}
		default:
			break;
		}
	
	iState = ENone;
	}

void CMobblerAppUi::HandleConnectCompleteL(TInt aError)
	{
	iStatusView->DrawDeferred();
	
	if (aError != KErrNone)
		{
		// Tell the user that there was an error connecting
		CAknResourceNoteDialog *note = new (ELeave) CAknInformationNote(EFalse);
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_NOTE_COMMS_ERROR));
		}
	else
		{
		// check for updates?
		if (iSettingView->CheckForUpdates())
			{
			TTime now;
			now.UniversalTime();
		
			if (now > iSettingView->NextUpdateCheck())
				{
				// do an update check
				iLastFMConnection->CheckForUpdateL(*this);
				iState = ECheckingUpdates;
				}
			else
				{
				iLastFMConnection->GetLatestTweetL(*this);
				iState = EFetchingTweet;
				}
			}
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

	noteText->Des().Append(iResourceReader->ResourceL(R_MOBBLER_NOTE_COMMS_ERROR));
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

void CMobblerAppUi::HandleTrackDequeued(const CMobblerTrack& /*aTrack*/)
	{
	iStatusView->DrawDeferred();
	--iTracksQueued;
	}

TBool CMobblerAppUi::GoOnlineL()
	{
	// Ask if they would like to go online
	CAknQueryDialog* dlg = CAknQueryDialog::NewL();
	TBool goOnline(dlg->ExecuteLD(R_MOBBLER_YES_NO_QUERY_DIALOG, iResourceReader->ResourceL(R_MOBBLER_ASK_GO_ONLINE)));
	
	if (goOnline)
		{
		iSettingView->SetModeL(CMobblerLastFMConnection::EOnline);
		}
	
	return goOnline;
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
	return iSettingView->Backlight();
	}

TInt CMobblerAppUi::ScrobblePercent() const
	{
	return iSettingView->ScrobblePercent();
	}

TInt CMobblerAppUi::DownloadAlbumArt() const
	{
	return iSettingView->DownloadAlbumArt();
	}

void CMobblerAppUi::SaveVolume()
	{
	iSettingView->SetVolumeL(RadioPlayer().Volume());
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
			iPreviousRadioArtist = CMobblerString::NewL(radio);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioTag;
			iPreviousRadioTag = CMobblerString::NewL(radio);
			}
		if (readStream.ReadInt8L())
			{
			readStream >> radio;
			delete iPreviousRadioPersonal;
			iPreviousRadioPersonal = CMobblerString::NewL(radio);
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
			writeStream << iPreviousRadioArtist->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioTag)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioTag->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		if (iPreviousRadioPersonal)
			{
			writeStream.WriteInt8L(ETrue);
			writeStream << iPreviousRadioPersonal->String();
			}
		else
			{
			writeStream.WriteInt8L(EFalse);
			}

		CleanupStack::PopAndDestroy(&writeStream);
		}

	CleanupStack::PopAndDestroy(&file);
	}

CMobblerResourceReader& CMobblerAppUi::ResourceReader() const
	{
	return *iResourceReader;
	}

void CMobblerAppUi::SetSleepTimer()
	{
	TInt sleepMinutes(iSettingView->SleepTimerMinutes());
	if (iSleepTimer->IsActive())
		{
		TTime now;
		now.UniversalTime();
#ifdef __WINS__
		TTimeIntervalSeconds minutes(0);
		iTimeToSleep.SecondsFrom(now, minutes);
#else
		TTimeIntervalMinutes minutes(0);
		iTimeToSleep.MinutesFrom(now, minutes);
#endif
		sleepMinutes = minutes.Int();
		}

	CAknNumberQueryDialog* sleepDlg = CAknNumberQueryDialog::NewL(sleepMinutes,
													CAknQueryDialog::ENoTone);
	sleepDlg->PrepareLC(R_MOBBLER_SLEEP_TIMER_QUERY_DIALOG);
	sleepDlg->SetPromptL(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_PROMPT));

	CEikButtonGroupContainer* cba = &sleepDlg->ButtonGroupContainer();
	MEikButtonGroup* buttonGroup = cba->ButtonGroup();

	cba->SetCommandL(buttonGroup->CommandId(0), iResourceReader->ResourceL(R_MOBBLER_SOFTKEY_SET));

	if (iSleepTimer->IsActive())
		{
		cba->SetCommandL(buttonGroup->CommandId(2), iResourceReader->ResourceL(R_MOBBLER_SOFTKEY_REMOVE));
		}

	TBool removeTimer(EFalse);

	if (sleepDlg->RunLD())
		{
		CEikTextListBox* list = new(ELeave) CAknSinglePopupMenuStyleListBox;
		CleanupStack::PushL(list);
		CAknPopupList* popupList = CAknPopupList::NewL(list, 
				R_AVKON_SOFTKEYS_SELECT_CANCEL, AknPopupLayouts::EMenuWindow);
		CleanupStack::PushL(popupList);

		list->ConstructL(popupList, CEikListBox::ELeftDownInViewRect);
		list->CreateScrollBarFrameL(ETrue);
		list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
														CEikScrollBarFrame::EAuto);

		if (iSleepTimer->IsActive())
			{
			CEikButtonGroupContainer* cba = popupList->ButtonGroupContainer();
			MEikButtonGroup* buttonGroup = cba->ButtonGroup();
			cba->SetCommandL(buttonGroup->CommandId(2), iResourceReader->ResourceL(R_MOBBLER_SOFTKEY_REMOVE));
			}

		CDesCArrayFlat* items = new CDesCArrayFlat(3);
		CleanupStack::PushL(items);

		items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_ACTION_STOP));
		items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_ACTION_OFFLINE));
		items->AppendL(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_ACTION_EXIT));
		
		CTextListBoxModel* model = list->Model();
		model->SetItemTextArray(items);
		model->SetOwnershipType(ELbmOwnsItemArray);
		CleanupStack::Pop();

		popupList->SetTitleL(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_ACTION));
		
		list->SetCurrentItemIndex(iSleepAction);
		TInt popupOk = popupList->ExecuteLD();
		CleanupStack::Pop();
		
		if (popupOk)
			{
			iSleepAction = list->CurrentItemIndex();
			iSettingView->SetSleepTimerMinutesL(sleepMinutes);
#ifdef __WINS__
			TTimeIntervalSeconds delay(sleepMinutes);
#else
			TTimeIntervalMinutes delay(sleepMinutes);
#endif
			iTimeToSleep.UniversalTime();
			iTimeToSleep += delay;
			iSleepTimer->AtUTC(iTimeToSleep);

			CEikonEnv::Static()->InfoMsg(_L("Timer set"));
			}
		else
			{
			removeTimer = ETrue;
			}
		}
	else
		{
		removeTimer = ETrue;
		}

	if (removeTimer && iSleepTimer->IsActive())
		{
		iSleepTimer->Cancel();
		CAknInformationNote* note = new (ELeave) CAknInformationNote(ETrue);
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_REMOVED));
		}
	}

void CMobblerAppUi::TimerExpiredL(TAny* /*aTimer*/, TInt aError)
	{
	if (aError == KErrNone)
		{
		// Do this for all actions, it gives Mobbler a chance to scrobble
		// the newly stopped song to Last.fm whilst displaying the dialog
		iLastFMConnection->TrackStoppedL();
		iRadioPlayer->Stop();

		CEikonEnv::Static()->InfoMsg(_L("Timer expired!"));
		CAknInformationNote* note = new (ELeave) CAknInformationNote(ETrue);
		note->ExecuteLD(iResourceReader->ResourceL(R_MOBBLER_SLEEP_TIMER_EXPIRED));

		switch (iSleepAction)
			{
			case EStopPlaying:
				// Nothing more to do
				break;
			case EGoOffline:
				HandleCommandL(EMobblerCommandOffline);
				break;
			case ExitMobber:
				HandleCommandL(EAknSoftkeyExit);
				break;
			default:
				break;
			}
		}

	// When the system time changes, At() timers will complete immediately with
	// KErrAbort. This can happen either if the user changes the time, or if 
	// the phone is set to auto-update with the network operator time.
	else if (aError == KErrAbort)
		{
		// Reset the timer
		iSleepTimer->AtUTC(iTimeToSleep);
		}	
	}

// End of File
