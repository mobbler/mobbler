/*
mobblerrichtextcontrol.cpp

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

#include <txtrich.h>
#include <eikrted.h>
#include <AknUtils.h>

#include "mobblerrichtextcontrol.h"	
#include "mobblerparser.h"

CMobblerRichTextControl* CMobblerRichTextControl::NewL(const TRect& aRect)
	{
	CMobblerRichTextControl* self = new(ELeave) CMobblerRichTextControl;
	CleanupStack::PushL(self);
	self->ConstructL(aRect);
	CleanupStack::Pop(self);
	return self;
	}


void CMobblerRichTextControl::ConstructL(const TRect& aRect)
    {
	CreateWindowL();
	
	iRichTextEditor = new (ELeave) CEikRichTextEditor;

	Prepare();
	
	iRichTextEditor->ConstructL(this, 0, 0, 0);
	iRichTextEditor->SetFocus(ETrue);
	
	SetRect(aRect);

	ActivateL();
    }

void CMobblerRichTextControl::WebServicesResponseL(const TDesC8& aXML)
	{
	HBufC* content;
	CMobblerParser::ParseArtistGetInfoL(aXML, content);
	CleanupStack::PushL(content);
	iRichTextEditor->SetTextL(content);
	CleanupStack::PopAndDestroy(content);
	
	DrawDeferred();
	}

void CMobblerRichTextControl::Prepare()
	{
	iRichTextEditor->SetAknEditorCase(EAknEditorLowerCase);
	iRichTextEditor->SetAknEditorFlags(EAknEditorFlagFixedCase | EAknEditorFlagEnableScrollBars);
	}

CMobblerRichTextControl::~CMobblerRichTextControl()
    {
	delete iRichTextEditor;
    }

void CMobblerRichTextControl::SizeChanged()
    {
	TRect ScrollBarRect = iRichTextEditor->ScrollBarFrame()->VerticalScrollBar()->Rect();
	
	iRichTextEditor->SetExtent(TPoint(0, 0), TSize(Rect().Width() - ScrollBarRect.Width(), Rect().Height()));
    }

TInt CMobblerRichTextControl::CountComponentControls() const
    {
    return 1;
    }

CCoeControl* CMobblerRichTextControl::ComponentControl(TInt aIndex) const
    {
    switch ( aIndex )
        {
        case 0:
			return iRichTextEditor;
        default:
            return NULL;
        }
    }

void CMobblerRichTextControl::Draw(const TRect& aRect) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushColor(KRgbWhite);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);
    }


TKeyResponse CMobblerRichTextControl::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
	{
	if (iRichTextEditor->IsFocused())
		{
		return iRichTextEditor->OfferKeyEventL(aKeyEvent, aType);	
		}
	else
		{
		return EKeyWasNotConsumed;
		}
	}

