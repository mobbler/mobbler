/*
mobblerstatusview.cpp

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

#include <akncontext.h>
#include <aknnotewrappers.h>
#include <akntitle.h>
#include <audioequalizerutility.h>
#include <barsread.h>
#include <mdaaudiooutputstream.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerradioplayer.h"
#include "mobblerresourcereader.h"
#include "mobblerstatuscontrol.h"
#include "mobblerstatusview.h"
#include "mobblerutility.h"

_LIT(KShortcut0, " (0)");
_LIT(KShortcut5, " (5)");
_LIT(KShortcut8, " (8)");

CMobblerStatusView* CMobblerStatusView::NewL()
	{
	CMobblerStatusView* self(new(ELeave) CMobblerStatusView);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerStatusView::CMobblerStatusView()
	{
	}

CMobblerStatusView::~CMobblerStatusView()
	{
	delete iMobblerStatusControl;
	iMobblerStatusControl = NULL;
	}

void CMobblerStatusView::ConstructL()
	{
	BaseConstructL(R_MOBBLER_STATUS_VIEW);
	SetupStatusPaneL();
	}

void CMobblerStatusView::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/) 
	{
	}

void CMobblerStatusView::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	}

void CMobblerStatusView::SetMenuItemTextL(CEikMenuPane* aMenuPane,
										  TInt aResourceId, TInt aCommandId)
	{
	HBufC* menuText(static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(aResourceId).AllocLC());

	const TInt KTextLimit(CEikMenuPaneItem::SData::ENominalTextLength);

	TPtrC shortcut(KNullDesC);
	
	// Shortcut keys
	if (aCommandId == EMobblerCommandPlusVisitLastFm)
		{
		shortcut.Set(KShortcut0);
		}
	else if (aCommandId == EMobblerCommandToggleScrobbling)
		{
		shortcut.Set(KShortcut5);
		}
	else if (aCommandId == EMobblerCommandEditSettings)
		{
		shortcut.Set(KShortcut8);
		}
	
	TBuf<KTextLimit> newText(menuText->Left(KTextLimit - shortcut.Length()));
	newText.Append(shortcut);
	CleanupStack::PopAndDestroy(menuText);
	menuText = newText.AllocLC();

	if (menuText->Length() > KTextLimit)
		{
		TBuf<KTextLimit> newText(menuText->Left(KTextLimit));
		CleanupStack::PopAndDestroy(menuText);
		menuText = newText.AllocLC();
		}

	aMenuPane->SetItemTextL(aCommandId, *menuText);
	CleanupStack::PopAndDestroy(menuText);
	}

void CMobblerStatusView::DisplayPlusMenu()
	{
	iDisplayPlusMenu = ETrue;
	MenuBar()->TryDisplayMenuBarL();
	}

void CMobblerStatusView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
	{
	// First load the menu text so as not to confuse any dimming logic
	if (aResourceId == R_MOBBLER_STATUS_MENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_START,		EMobblerCommandRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RESUME_RADIO,		EMobblerCommandResumeRadio);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_GO_ONLINE,		EMobblerCommandOnline);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_GO_OFFLINE,		EMobblerCommandOffline);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VIEW,				EMobblerCommandView);
        SetMenuItemTextL(aMenuPane, R_MOBBLER_SEARCH,           EMobblerCommandSearch);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EQUALIZER,		EMobblerCommandEqualizer);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOOLS_SUBMENU,	EMobblerCommandTools);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SETTINGS,			EMobblerCommandEditSettings);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ABOUT,			EMobblerCommandAbout);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXIT,				EAknSoftkeyExit);
		
		// the plus menu commands
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VISIT_LASTFM_MENU,	EMobblerCommandPlusVisitLastFm);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHARE,				EMobblerCommandPlusShare);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR,				EMobblerCommandPlusSimilar);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP,					EMobblerCommandPlusTop);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLIST_ADD_TRACK,	EMobblerCommandPlusPlaylistAddTrack);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,				EMobblerCommandPlusEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST_SHOUTBOX,		EMobblerCommandPlusArtistShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TAG,					EMobblerCommandPlusTag);
		}
	else if(aResourceId == R_MOBBLER_PLUS_SHARE_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHARE_TRACK,			EMobblerCommandPlusShareTrack);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHARE_ARTIST,			EMobblerCommandPlusShareArtist);
		}
	else if(aResourceId == R_MOBBLER_PLUS_SIMILAR_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_ARTISTS,		EMobblerCommandPlusSimilarArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SIMILAR_TRACKS,		EMobblerCommandPlusSimilarTracks);
		}
	else if(aResourceId == R_MOBBLER_PLUS_TOP_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,			EMobblerCommandPlusTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,			EMobblerCommandPlusTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,				EMobblerCommandPlusTopTags);
		}
	else if(aResourceId == R_MOBBLER_TAG_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK_ADD_TAG,			EMobblerCommandTrackAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TRACK_REMOVE_TAG,			EMobblerCommandTrackRemoveTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ALBUM_ADD_TAG,			EMobblerCommandAlbumAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ALBUM_REMOVE_TAG,			EMobblerCommandAlbumRemoveTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST_ADD_TAG,			EMobblerCommandArtistAddTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ARTIST_REMOVE_TAG,		EMobblerCommandArtistRemoveTag);
		}
	else if(aResourceId == R_RADIO_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_ARTIST,				EMobblerCommandRadioArtist);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_TAG,				EMobblerCommandRadioTag);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_USER,				EMobblerCommandRadioUser);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_PERSONAL,			EMobblerCommandRadioPersonal);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_RECOMMENDATIONS,	EMobblerCommandRadioRecommendations);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_LOVED,				EMobblerCommandRadioLoved);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RADIO_NEIGHBOURHOOD,		EMobblerCommandRadioNeighbourhood);
		}
	else if(aResourceId == R_MOBBLER_VIEW_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_FRIENDS,					EMobblerCommandFriends);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ARTISTS,				EMobblerCommandUserTopArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RECOMMENDED_ARTISTS,		EMobblerCommandRecommendedArtists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RECOMMENDED_EVENTS,		EMobblerCommandRecommendedEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_ALBUMS,				EMobblerCommandUserTopAlbums);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TRACKS,				EMobblerCommandUserTopTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_PLAYLISTS,				EMobblerCommandPlaylists);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,					EMobblerCommandUserEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EVENTS,					EMobblerCommandArtistEvents);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_TOP_TAGS,					EMobblerCommandUserTopTags);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_RECENT_TRACKS,			EMobblerCommandRecentTracks);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,					EMobblerCommandUserShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,					EMobblerCommandArtistShoutbox);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SHOUTBOX,					EMobblerCommandEventShoutbox);
		}
	else if(aResourceId == R_MOBBLER_SEARCH_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SEARCH_TRACKS,				EMobblerCommandSearchTrack);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SEARCH_ALBUMS,				EMobblerCommandSearchAlbum);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SEARCH_ARTISTS,				EMobblerCommandSearchArtist);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SEARCH_TAGS,					EMobblerCommandSearchTag);
		}
	else if(aResourceId == R_MOBBLER_EQUALIZER_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EQUALIZER_PROFILE_DEFAULT, EMobblerCommandEqualizerDefault);
		}
	else if(aResourceId == R_MOBBLER_TOOLS_SUBMENU_PANE)
		{
		SetMenuItemTextL(aMenuPane, R_MOBBLER_CHECK_FOR_UPDATES,		EMobblerCommandCheckForUpdates);
		if (static_cast<CMobblerAppUi*>(AppUi())->ScrobblingOn())
			{
			SetMenuItemTextL(aMenuPane, R_MOBBLER_DISABLE_SCROBBLING,	EMobblerCommandToggleScrobbling);
			}
		else
			{
			SetMenuItemTextL(aMenuPane, R_MOBBLER_ENABLE_SCROBBLING,	EMobblerCommandToggleScrobbling);
			}
		SetMenuItemTextL(aMenuPane, R_MOBBLER_VIEW_SCROBBLE_LOG,		EMobblerCommandViewScrobbleLog);
#ifdef __SYMBIAN_SIGNED__
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SET_AS_WALLPAPER,			EMobblerCommandSetAsWallpaper); // TODO only if album art available
#endif
		SetMenuItemTextL(aMenuPane, R_MOBBLER_SLEEP_TIMER,				EMobblerCommandSleepTimer);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_ALARM,					EMobblerCommandAlarm);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_EXPORT_QUEUE_TO_LOG,		EMobblerCommandExportQueueToLogFile);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_LANGUAGE_PATCHES,			EMobblerCommandLanguagePatches);
		SetMenuItemTextL(aMenuPane, R_MOBBLER_QR_CODE,					EMobblerCommandQrCode);
		}

	// Now the menu text is set, dimming logic is next
	if (aResourceId == R_MOBBLER_STATUS_MENU_PANE)
		{
		if (iDisplayPlusMenu)
			{
			iDisplayPlusMenu = EFalse;
			
			// hide everything that isn't a plus menu command
			aMenuPane->SetItemDimmed(EMobblerCommandRadio, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandResumeRadio, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandOnline, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandOffline, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandView, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandSearch, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandEqualizer, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandTools, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandEditSettings, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandAbout, ETrue);
			aMenuPane->SetItemDimmed(EAknSoftkeyExit, ETrue);
			}
		else
			{
			// hide the plus menu commands
			aMenuPane->SetItemDimmed(EMobblerCommandPlusShare, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusSimilar, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusTop, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusPlaylistAddTrack, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusEvents, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusArtistShoutbox, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusVisitLastFm, ETrue);
			aMenuPane->SetItemDimmed(EMobblerCommandPlusTag, ETrue);
			
			if (static_cast<CMobblerAppUi*>(AppUi())->Mode() == CMobblerLastFmConnection::EOnline ||
					static_cast<CMobblerAppUi*>(AppUi())->State() == CMobblerLastFmConnection::EConnecting ||
					static_cast<CMobblerAppUi*>(AppUi())->State() == CMobblerLastFmConnection::EHandshaking)
				{
				aMenuPane->SetItemDimmed(EMobblerCommandOnline, ETrue);
				}
			else
				{
				aMenuPane->SetItemDimmed(EMobblerCommandOffline, ETrue);
				}
			
			if (MobblerUtility::EqualizerSupported())
				{
				MMdaAudioOutputStreamCallback* dummyCallback(NULL);
				CMdaAudioOutputStream* tempStream(CMdaAudioOutputStream::NewL(*dummyCallback));
				CAudioEqualizerUtility* tempEqualizer(NULL);
				TRAP_IGNORE(tempEqualizer = CAudioEqualizerUtility::NewL(*tempStream));
				if (!tempEqualizer)
					{
					aMenuPane->SetItemDimmed(EMobblerCommandEqualizer, ETrue);
					}
				
				tempStream->Stop();
				delete tempEqualizer;
				delete tempStream;
				}
			else
				{
				aMenuPane->SetItemDimmed(EMobblerCommandEqualizer, ETrue);
				}
			
			aMenuPane->SetItemDimmed(EMobblerCommandResumeRadio, 
						!static_cast<CMobblerAppUi*>(AppUi())->RadioResumable());
			}
		}
	else if (aResourceId == R_MOBBLER_EQUALIZER_SUBMENU_PANE)
		{
		MMdaAudioOutputStreamCallback* dummyCallback(NULL);
		CMdaAudioOutputStream* tempStream(CMdaAudioOutputStream::NewL(*dummyCallback));
		CleanupStack::PushL(tempStream);
		CAudioEqualizerUtility* tempEqualizer(NULL);
		TRAP_IGNORE(tempEqualizer = CAudioEqualizerUtility::NewL(*tempStream));
		if (tempEqualizer)
			{
			CleanupStack::PushL(tempEqualizer);
			TInt count(tempEqualizer->Presets().Count());
			for (TInt i(0); i < count; ++i)
				{
				CEikMenuPaneItem::SData item;
				item.iCascadeId = 0;
				item.iCommandId = EMobblerCommandEqualizerDefault + i + 1;
				if (i == count - 1)
					{
					item.iFlags = EEikMenuItemRadioEnd;
					}
				else
					{
					item.iFlags = EEikMenuItemRadioMiddle;
					}
				item.iText = tempEqualizer->Presets()[i].iPresetName;
				aMenuPane->AddMenuItemL(item);
				}
			TInt equalizerIndex(static_cast<CMobblerAppUi*>(AppUi())->RadioPlayer().EqualizerIndex());
			aMenuPane->SetItemButtonState(EMobblerCommandEqualizerDefault + equalizerIndex + 1, EEikMenuItemSymbolOn);
			tempStream->Stop();
			CleanupStack::PopAndDestroy(tempEqualizer);
			}
		CleanupStack::PopAndDestroy(tempStream);
		}
	else if (aResourceId == R_MOBBLER_VIEW_SUBMENU_PANE)
		{
		// hide similar menu items from this menu
		aMenuPane->SetItemDimmed(EMobblerCommandSimilarArtists, ETrue);
		aMenuPane->SetItemDimmed(EMobblerCommandSimilarTracks, ETrue);
		
		// should only be able to access the user shoutbox from this menu
		aMenuPane->SetItemDimmed(EMobblerCommandArtistShoutbox, ETrue);
		aMenuPane->SetItemDimmed(EMobblerCommandEventShoutbox, ETrue);
		
		aMenuPane->SetItemDimmed(EMobblerCommandArtistEvents, ETrue);
		aMenuPane->SetItemDimmed(EMobblerCommandArtistTopAlbums, ETrue);
		aMenuPane->SetItemDimmed(EMobblerCommandArtistTopTracks, ETrue);
		aMenuPane->SetItemDimmed(EMobblerCommandArtistTopTags, ETrue);
		
		aMenuPane->SetItemDimmed(EMobblerCommandEventWebPage, ETrue);
		}
	
	// Third edition only due to an S60 5th edition bug (issue 364)
	aMenuPane->EnableMarqueeL(!iMobblerStatusControl->IsFifthEdition());
	}

TUid CMobblerStatusView::Id() const
	{
	return TUid::Uid(KMobblerStatusViewUid);
	}

void CMobblerStatusView::HandleCommandL(TInt aCommand)
	{
	// let the app ui handle the event
	AppUi()->HandleCommandL(aCommand);
	}

void CMobblerStatusView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	if (!iMobblerStatusControl)
		{
		iMobblerStatusControl = CMobblerStatusControl::NewL(ClientRect(), *static_cast<CMobblerAppUi*>(AppUi()));
		iMobblerStatusControl->SetMopParent(AppUi());
		
		iMobblerStatusControl->ActivateL();
		AppUi()->AddToStackL(*this, iMobblerStatusControl);
		
		// Change the Back softkey to Hide
		TInt pos(Cba()->PositionById(EAknSoftkeyBack));
		Cba()->RemoveCommandFromStack(pos, EAknSoftkeyBack);
		Cba()->SetCommandL(pos, EAknSoftkeyBack, static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(R_MOBBLER_SOFTKEY_HIDE));
		}
	
	SettingsWizardL();
	}

void CMobblerStatusView::DoDeactivate()
	{
	if (iMobblerStatusControl)
		{
		AppUi()->RemoveFromStack(iMobblerStatusControl);
		delete iMobblerStatusControl;
		iMobblerStatusControl = NULL;
		}
	}

void CMobblerStatusView::HandleStatusPaneSizeChange()
	{
	CAknView::HandleStatusPaneSizeChange();
	TRAP_IGNORE(SetupStatusPaneL()); 
	}

void CMobblerStatusView::SetupStatusPaneL()
	{
	TUid contextPaneUid(TUid::Uid(EEikStatusPaneUidContext));
	CEikStatusPaneBase::TPaneCapabilities subPaneContext(StatusPane()->PaneCapabilities(contextPaneUid));
	
	if (subPaneContext.IsPresent() && subPaneContext.IsAppOwned())
		{
		CAknContextPane* context(static_cast<CAknContextPane*>(StatusPane()->ControlL(contextPaneUid)));
		context->SetPictureToDefaultL();
		}
	
	// setup the title pane
	TUid titlePaneUid(TUid::Uid(EEikStatusPaneUidTitle));
	CEikStatusPaneBase::TPaneCapabilities subPaneTitle(StatusPane()->PaneCapabilities(titlePaneUid));
	
	if (subPaneTitle.IsPresent() && subPaneTitle.IsAppOwned())
		{
		CAknTitlePane* title(static_cast<CAknTitlePane*>(StatusPane()->ControlL(titlePaneUid)));
		TResourceReader reader;
		iEikonEnv->CreateResourceReaderLC(reader, R_MOBBLER_TITLE_RESOURCE);
		title->SetFromResourceL(reader);
		CleanupStack::PopAndDestroy();
		}
	}

void CMobblerStatusView::DrawDeferred() const
	{
	if (iMobblerStatusControl)
		{
		iMobblerStatusControl->DrawDeferred();
		}
	}

void CMobblerStatusView::DrawNow() const
	{
	if (iMobblerStatusControl)
		{
		iMobblerStatusControl->DrawNow();
		}
	}

CMobblerStatusControl* CMobblerStatusView::StatusControl()
	{
	return iMobblerStatusControl;
	}

void CMobblerStatusView::SettingsWizardL()
	{
	if (static_cast<CMobblerAppUi*>(AppUi())->DetailsNeeded())
		{
		// Display info note
		CAknInformationNote* note(new (ELeave) CAknInformationNote(ETrue));
		note->ExecuteLD(static_cast<CMobblerAppUi*>(AppUi())->
			ResourceReader().ResourceL(R_MOBBLER_NOTE_NO_DETAILS));
		
		// Query username and password
		TBuf<KMobblerMaxUsernameLength> username;
		TBuf<KMobblerMaxPasswordLength> password;
		CAknMultiLineDataQueryDialog* dlg(CAknMultiLineDataQueryDialog::NewL(username, password));
		dlg->SetPromptL(static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(R_MOBBLER_USERNAME),
						static_cast<CMobblerAppUi*>(AppUi())->ResourceReader().ResourceL(R_MOBBLER_PASSWORD));
		if (dlg->ExecuteLD(R_MOBBLER_USERNAME_PASSWORD_QUERY_DIALOG))
			{
			static_cast<CMobblerAppUi*>(AppUi())->SetDetailsL(username, password, ETrue);
			}
		}
	}

// End of file
