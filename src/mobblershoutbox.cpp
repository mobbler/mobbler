/*
mobblershoutbox.cpp

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

#include <aknquerydialog.h>
#include <gulicon.h>
#include <eikclbd.h>

#include "mobblerappui.h"
#include "mobblershoutbox.h"
#include "mobblereventlist.h"
#include "mobblerplaylistlist.h"
#include "mobblerparser.h"
#include "mobblerlastfmconnection.h"
#include "mobblerlistitem.h"
#include "mobblerresourcereader.h"
#include "mobblerstring.h"
#include "mobblersettingitemlistview.h"

#include "mobbler_strings.rsg"
#include "mobbler.rsg"

#include "mobbler.hrh"

_LIT(KDefaultImage, "\\resource\\apps\\mobbler\\default_user.png");

CMobblerShoutbox::CMobblerShoutbox(CMobblerAppUi& aAppUi, CMobblerWebServicesControl& aWebServicesControl)
	:CMobblerListControl(aAppUi, aWebServicesControl)
	{
	}

void CMobblerShoutbox::ConstructL()
	{
    iDefaultImage = CMobblerBitmap::NewL(*this, KDefaultImage);
    
    switch (iType)
    	{
    	case EMobblerCommandUserShoutbox:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("user"), _L8("getshouts"), iText1->String8(), *this);
    		break;
    	case EMobblerCommandEventShoutbox:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("event"), _L8("getshouts"), iText2->String8(), *this);
    		break;
    	case EMobblerCommandArtistShoutbox:
    		iAppUi.LastFMConnection().WebServicesCallL(_L8("artist"), _L8("getshouts"), iText1->String8(), *this);
    		break;
    	default:
    		break;
    	}
	}

CMobblerShoutbox::~CMobblerShoutbox()
	{
	}

HBufC* CMobblerShoutbox::ShoutAtTextOwnerLC()
	{
	HBufC* shoutAtText(NULL);

	if (iText1->String().Length() == 0)
		{
		CMobblerString* name = CMobblerString::NewL(iAppUi.SettingView().UserName());
		CleanupStack::PushL(name);
		shoutAtText = ShoutAtTextLC(name->String8());
		CleanupStack::Pop(shoutAtText);
		CleanupStack::PopAndDestroy(name);
		CleanupStack::PushL(shoutAtText);
		}
	else
		{
		shoutAtText = ShoutAtTextLC(iText1->String8());
		}
	
	return shoutAtText;
	}

HBufC* CMobblerShoutbox::ShoutAtTextUserLC()
	{
	return ShoutAtTextLC(iList[iListBox->CurrentItemIndex()]->Title()->String8());
	}

HBufC* CMobblerShoutbox::ShoutAtTextLC(const TDesC8& aName)
	{
	const TDesC& format = iAppUi.ResourceReader().ResourceL(R_MOBBLER_SHOUT_AT);
	HBufC* text = HBufC::NewLC(format.Length() + aName.Length());
	
	CMobblerString* name = CMobblerString::NewL(aName);
	CleanupStack::PushL(name);
	text->Des().Format(format, &name->String());
	CleanupStack::PopAndDestroy(name);
	
	return text;
	}

CMobblerListControl* CMobblerShoutbox::HandleListCommandL(TInt aCommand)
	{
	CMobblerListControl* list(NULL);
	
	CAknTextQueryDialog* shoutDialog(NULL);
	TBuf<255> shoutMessage;
	
	HBufC* dialogPromptText(NULL);
	
	switch (aCommand)
		{	
		case EMobblerCommandShoutUser:
		case EMobblerCommandShoutOwner:
			
			if (aCommand == EMobblerCommandShoutUser)
				{
				dialogPromptText = ShoutAtTextUserLC();
				}
			else
				{
				dialogPromptText = ShoutAtTextOwnerLC();
				}
			
			shoutDialog = new(ELeave) CAknTextQueryDialog(shoutMessage);
			shoutDialog->PrepareLC(R_MOBBLER_TEXT_QUERY_DIALOG);
			shoutDialog->SetPromptL(*dialogPromptText);
			shoutDialog->SetPredictiveTextInputPermitted(ETrue);

			if (shoutDialog->RunLD())
				{
				CMobblerString* shout = CMobblerString::NewL(shoutMessage);
				CleanupStack::PushL(shout);
				switch (iType)
					{
					case EMobblerCommandUserShoutbox:
						iAppUi.LastFMConnection().ShoutL(_L8("user"), iList[iListBox->CurrentItemIndex()]->Title()->String8(), shout->String8());
						break;
					case EMobblerCommandEventShoutbox:
						iAppUi.LastFMConnection().ShoutL(_L8("event"), iText2->String8(), shout->String8());
						break;
					case EMobblerCommandArtistShoutbox:
						iAppUi.LastFMConnection().ShoutL(_L8("artist"), iList[iListBox->CurrentItemIndex()]->Title()->String8(), shout->String8());
						break;
					default:
						break;
					}
				
				CleanupStack::PopAndDestroy(shout);
				}
			
			CleanupStack::PopAndDestroy(dialogPromptText);
			
			break;
		default:
			break;	
		}
	
	return list;
	}

void CMobblerShoutbox::SupportedCommandsL(RArray<TInt>& aCommands)
	{
	aCommands.AppendL(EMobblerCommandShout);
	aCommands.AppendL(EMobblerCommandShoutUser);
	aCommands.AppendL(EMobblerCommandShoutOwner);
	}

void CMobblerShoutbox::ParseL(const TDesC8& aXML)
	{
	CMobblerParser::ParseShoutboxL(aXML, *this, iList);
	}


