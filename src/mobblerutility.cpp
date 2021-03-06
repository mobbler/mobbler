/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2009, 2010, 2011  Hugo van Kemenade
Copyright (C) 2010  gw111zz

http://code.google.com/p/mobbler

This file is part of Mobbler.

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <bautils.h>
#include <coemain.h>
#include <hal.h>
#include <hash.h>
#include <sendomfragment.h>

#include "mobblerlogging.h"
#include "mobblertracer.h"
#include "mobblerutility.h"

_LIT8(KChineseLangCode, "cn");
_LIT8(KEnglishLangCode, "en");
_LIT8(KFrenchLangCode, "fr");
_LIT8(KGermanLangCode, "de");
_LIT8(KItalianLangCode, "it");
_LIT8(KJapaneseLangCode, "jp");
_LIT8(KPolishLangCode, "pl");
_LIT8(KPortugueseLangCode, "pt");
_LIT8(KRussianLangCode, "ru");
_LIT8(KSpanishLangCode, "es");
_LIT8(KSwedishLangCode, "sv");
_LIT8(KTurkishLangCode, "tu");

_LIT(KEnglishMobileUrl,		"http://m.last.fm/");
_LIT(KChineseMobileUrl,		"http://m.cn.last.fm/");
_LIT(KFrenchMobileUrl,		"http://m.lastfm.fr/");
_LIT(KGermanMobileUrl,		"http://m.lastfm.de/");
_LIT(KItalianMobileUrl,		"http://m.lastfm.it/");
_LIT(KJapaneseMobileUrl,	"http://m.lastfm.jp/");
_LIT(KPolishMobileUrl,		"http://m.lastfm.pl/");
_LIT(KPortugueseMobileUrl,	"http://m.lastfm.com.br/");
_LIT(KRussianMobileUrl,		"http://m.lastfm.ru/");
_LIT(KSpanishMobileUrl,		"http://m.lastfm.es/");
_LIT(KSwedishMobileUrl,		"http://m.lastfm.se/");
_LIT(KTurkishMobileUrl,		"http://m.lastfm.com.tr/");

_LIT(KEnglishUrl,			"http://www.last.fm/");
_LIT(KChineseUrl,			"http://cn.last.fm/");
_LIT(KFrenchUrl,			"http://www.lastfm.fr/");
_LIT(KGermanUrl,			"http://www.lastfm.de/");
_LIT(KItalianUrl,			"http://www.lastfm.it/");
_LIT(KJapaneseUrl,			"http://www.lastfm.jp/");
_LIT(KPolishUrl,			"http://www.lastfm.pl/");
_LIT(KPortugueseUrl,		"http://www.lastfm.com.br/");
_LIT(KRussianUrl,			"http://www.lastfm.ru/");
_LIT(KSpanishUrl,			"http://www.lastfm.es/");
_LIT(KSwedishUrl,			"http://www.lastfm.se/");
_LIT(KTurkishUrl,			"http://www.lastfm.com.tr/");


_LIT8(KFormat, "%02x");

const TInt KNokia5630XpressMusicMachineUid(0x2000DA61);
const TInt KNokia6710NavigatorMachineUid(0x20014DD1);
const TInt KNokiaE52MachineUid(0x20014DCC);
const TInt KNokiaE55MachineUid(0x20014DCF);
const TInt KNokiaE72MachineUid(0x20014DD0);
const TInt KNokiaN8MachineUid(0x20029A73);

TBool MobblerUtility::iEqualizerSupported = ETrue;

TBool MobblerUtility::EqualizerSupported()
	{
	TRACER_AUTO;
	if (!iEqualizerSupported)
		{
		return iEqualizerSupported;
		}

	TInt machineUid(0);
	TInt error(HAL::Get(HALData::EMachineUid, machineUid));
	if (error == KErrNone)
		{
		iEqualizerSupported = !(machineUid == KNokia5630XpressMusicMachineUid ||
								machineUid == KNokia6710NavigatorMachineUid || 
								machineUid == KNokiaE52MachineUid || 
								machineUid == KNokiaE55MachineUid || 
								machineUid == KNokiaE72MachineUid ||
								machineUid >= KNokiaN8MachineUid);
		// The N8 doesn't like the equalizer and we
		// don't expect any future device to either
		return iEqualizerSupported;
		}
	else
		{
		return EFalse;
		}
	}

void MobblerUtility::SetEqualizerNotSupported()
	{
	iEqualizerSupported = EFalse;
	}

HBufC8* MobblerUtility::MD5LC(const TDesC8& aSource)
	{
	TRACER_AUTO;
	CMD5* md5(CMD5::NewL());
	CleanupStack::PushL(md5);
	
	TPtrC8 hash(md5->Hash(aSource));
	HBufC8* hashResult(HBufC8::NewLC(hash.Length() * 2));
	
	for (TInt i(0); i < hash.Length(); ++i)
		{
		hashResult->Des().AppendFormat(KFormat, hash[i]);
		}
	
	CleanupStack::Pop(hashResult);
	CleanupStack::PopAndDestroy(md5);
	CleanupStack::PushL(hashResult);
	return hashResult;
	}

HBufC8* MobblerUtility::URLEncodeLC(const TDesC8& aString, TBool aEncodeAll)
	{
	TRACER_AUTO;
	
	_LIT8(KFormatCode, "%%%02X");
	
	if (aString.Length() == 0)
		{
		return NULL;
		}

	_LIT8(KDontEncode, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~");

	// Alloc to the maximum size of URL if every char are encoded
	HBufC8* encoded(HBufC8::NewLC(aString.Length() * 3));

	// Parse the chars in the URL
	for (TInt i(0); i < aString.Length(); ++i)
		{
		const TUint8& cToFind(aString[i]);
		if (aEncodeAll || (KErrNotFound == KDontEncode().Locate(cToFind)))
			{
			// Char not found, encode it
			encoded->Des().AppendFormat(KFormatCode, cToFind);
			}
		else
			{
			// Char found, just copy it
			encoded->Des().Append(cToFind);
			}
		}

	return encoded;
	}

TBuf8<2> MobblerUtility::LanguageL()
	{
	TRACER_AUTO;
	TBuf8<2> language;
	language.Copy(KEnglishLangCode); // default to English
	
	RArray<TLanguage> downgradePath;
	CleanupClosePushL(downgradePath);
	BaflUtils::GetDowngradePathL(CCoeEnv::Static()->FsSession(), User::Language(), downgradePath);
	
	TBool languageFound(EFalse);
	
	const TInt KLanguageCount(downgradePath.Count());
	for (TInt i(0); i < KLanguageCount && !languageFound; ++i)
		{
		languageFound = ETrue;
		
		switch (downgradePath[i])
			{
			case ELangEnglish:
				language.Copy(KEnglishLangCode);
				break;
			case ELangFrench:
				language.Copy(KFrenchLangCode);
				break;
			case ELangGerman: 
				language.Copy(KGermanLangCode);
				break;
			case ELangSpanish: 
				language.Copy(KSpanishLangCode);
				break;
			case ELangItalian: 
				language.Copy(KItalianLangCode);
				break;
			case ELangSwedish: 
				language.Copy(KSwedishLangCode);
				break;
			case ELangPortuguese: 
				language.Copy(KPortugueseLangCode);
				break;
			case ELangRussian: 
				language.Copy(KRussianLangCode);
				break;
			case ELangPolish:
				language.Copy(KPolishLangCode);
				break;
			case ELangPrcChinese: 
				language.Copy(KChineseLangCode);
				break;
			case ELangJapanese: 
				language.Copy(KJapaneseLangCode);
				break;
			case ELangTurkish:
				language.Copy(KTurkishLangCode);
				break;
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	return language;
	}

TBuf<30> MobblerUtility::LocalLastFmDomainL(const TInt aMobile)
	{
	TRACER_AUTO;
	TBuf<30> url;
	url.Copy(KEnglishMobileUrl); // default to mobile English
	
	RArray<TLanguage> downgradePath;
	CleanupClosePushL(downgradePath);
	BaflUtils::GetDowngradePathL(CCoeEnv::Static()->FsSession(), User::Language(), downgradePath);
	
	TBool languageFound(EFalse);
	
	const TInt KLanguageCount(downgradePath.Count());
	for (TInt i(0); i < KLanguageCount && !languageFound; ++i)
		{
		languageFound = ETrue;
		
		switch (downgradePath[i])
			{
			case ELangEnglish:
				aMobile?
					url.Copy(KEnglishMobileUrl):
					url.Copy(KEnglishUrl);
				break;
			case ELangFrench:
				aMobile?
					url.Copy(KFrenchMobileUrl):
					url.Copy(KFrenchUrl);
				break;
			case ELangGerman: 
				aMobile?
					url.Copy(KGermanMobileUrl):
					url.Copy(KGermanUrl);
				break;
			case ELangSpanish: 
				aMobile?
					url.Copy(KSpanishMobileUrl):
					url.Copy(KSpanishUrl);
				break;
			case ELangItalian: 
				aMobile?
					url.Copy(KItalianMobileUrl):
					url.Copy(KItalianUrl);
				break;
			case ELangSwedish: 
				aMobile?
					url.Copy(KSwedishMobileUrl):
					url.Copy(KSwedishUrl);
				break;
			case ELangPortuguese: 
				aMobile?
					url.Copy(KPortugueseMobileUrl):
					url.Copy(KPortugueseUrl);
				break;
			case ELangRussian: 
				aMobile?
					url.Copy(KRussianMobileUrl):
					url.Copy(KRussianUrl);
				break;
			case ELangPolish:
				aMobile?
					url.Copy(KPolishMobileUrl):
					url.Copy(KPolishUrl);
				break;
			case ELangPrcChinese:
				aMobile?
					url.Copy(KChineseMobileUrl):
					url.Copy(KChineseUrl);
					break;
			case ELangJapanese: 
				aMobile?
					url.Copy(KJapaneseMobileUrl):
					url.Copy(KJapaneseUrl);
				break;
			case ELangTurkish:
				aMobile?
					url.Copy(KTurkishMobileUrl):
					url.Copy(KTurkishUrl);
				break;
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	return url;
	}

void MobblerUtility::StripUnwantedTagsFromHtmlL(HBufC8*& aHtml)
	{
	TRACER_AUTO;
	_LIT8(KAnchorStart, "<a");

	TPtr8 htmlPtr(aHtml->Des());

	TInt pos(KErrNotFound);
	while ((pos = htmlPtr.Find(KAnchorStart)) != KErrNotFound)
		{
		TPtrC8 ptrFromPos(htmlPtr.MidTPtr(pos));
		TInt endBracketPos(ptrFromPos.Locate('>'));
		if (endBracketPos == KErrNotFound)
			{
			break; // HTML not well-formed, just stop
			}
		htmlPtr.Delete(pos, endBracketPos + 1);
		}

	_LIT8(KAnchorEnd, "</a>");
	while ((pos = htmlPtr.Find(KAnchorEnd)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KAnchorEnd().Length());
		}

	_LIT8(KBandMemberTag, "[bandmember]");
	while ((pos = htmlPtr.Find(KBandMemberTag)) != KErrNotFound)
		{
		TPtrC8 ptrFromPos(htmlPtr.MidTPtr(pos));
		TInt endBracketPos = ptrFromPos.Locate(']');
		if (endBracketPos == KErrNotFound)
			{
			break; // Tag not well-formed, just stop
			}
		htmlPtr.Delete(pos, endBracketPos + 1);
		}

	_LIT8(KBandMemberEndTag, "[/bandmember]");
	while ((pos = htmlPtr.Find(KBandMemberEndTag)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KBandMemberEndTag().Length());
		}

	// It makes the tidying code easier to use <br /><br /> as paragraph
	// breaks rather than <p>...</p>

	// Calculate if the descriptor's maximum length is big
	// enough to store the extra characters for the line breaks
	// Finally, replace paragraph with newlines
	_LIT8(KBrTag1, "<br /><br />");
	_LIT8(KNewLine, "\x20\x0A");

	// We don't want to keep growing the descriptor so do a quick
	// scan of the HTML and see if there is enough space in the
	// descriptor to store the extra <br />
	// In most cases, we will have gained some space from stripping
	// out anchors etc.
	while ((pos = htmlPtr.Find(KNewLine)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KNewLine().Length());

		if ((htmlPtr.Length() + KBrTag1().Length()) > htmlPtr.MaxLength())
			{
			CleanupStack::PushL(aHtml);
			aHtml = aHtml->ReAllocL(htmlPtr.Length() + KBrTag1().Length() * 3); // 3 times so that we don't have to keep reallocating
			CleanupStack::Pop();
			htmlPtr.Set(aHtml->Des());
			}
		htmlPtr.Insert(pos, KBrTag1);
		}
	}

CSenDomFragment* MobblerUtility::PrepareDomFragmentLC(CSenXmlReader& aXmlReader, const TDesC8& aXml)
	{
	TRACER_AUTO;
	// Create the DOM fragment and associate with the XML reader
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	aXmlReader.SetContentHandler(*domFragment);
	domFragment->SetReader(aXmlReader);
	
	// Parse the XML into the DOM fragment
	aXmlReader.ParseL(aXml);
	
	return domFragment;
	}

// End of file
