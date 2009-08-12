/*
mobblerutility.cpp

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

#include "mobblerutility.h"

#ifndef __WINS__
#include "mobblerappui.h"
#include "mobblerstring.h"

#include <browserlauncher.h>
#endif

#include <bautils.h>
#include <coemain.h>
#include <hash.h>

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

// _LIT(KChineseUrl,		"http://cn.last.fm/"); /// No known Chinese mobile site
_LIT(KEnglishUrl,		"http://m.last.fm/");
_LIT(KFrenchUrl,		"http://m.lastfm.fr/");
_LIT(KGermanUrl,		"http://m.lastfm.de/");
_LIT(KItalianUrl,		"http://m.lastfm.it/");
_LIT(KJapaneseUrl, 		"http://m.lastfm.jp/");
_LIT(KPolishUrl,		"http://m.lastfm.pl/");
_LIT(KPortugueseUrl,	"http://m.lastfm.com.br/");
_LIT(KRussianUrl,		"http://m.lastfm.ru/");
_LIT(KSpanishUrl,		"http://m.lastfm.es/");
_LIT(KSwedishUrl,		"http://m.lastfm.se/");
_LIT(KTurkishUrl,		"http://m.lastfm.com.tr/");

_LIT8(KAmazonSearchLink, "http://www.%S/gp/search?ie=UTF8&keywords=%S%%20%S&tag=mobbler-21&index=music&linkCode=ur2&camp=1634&creative=6738");

HBufC8* MobblerUtility::MD5LC(const TDesC8& aSource)
	{
	CMD5* md5(CMD5::NewL());
	CleanupStack::PushL(md5);
	
	TPtrC8 hash(md5->Hash(aSource));
	HBufC8* hashResult(HBufC8::NewLC(hash.Length() * 2));
	
	for (TInt i(0); i < hash.Length(); ++i)
		{
		hashResult->Des().AppendFormat(_L8("%02x"), hash[i]);
		}
	
	CleanupStack::Pop(hashResult);
	CleanupStack::PopAndDestroy(md5);
	CleanupStack::PushL(hashResult);
	return hashResult;
	}

HBufC8* MobblerUtility::URLEncodeLC(const TDesC8& aString)
	{
	HBufC8* urlEncoded(HBufC8::NewLC(aString.Length() * 3));
	// sanitise the input string
	const TInt KCharCount(aString.Length());
	for (TInt i(0); i < KCharCount; ++i)
		{
		urlEncoded->Des().AppendFormat(_L8("%%%2x"), aString[i]);
		}
	
	return urlEncoded;
	}

HBufC8* MobblerUtility::URLEncodeLC(const TDesC& aString)
	{
	HBufC8* urlEncoded(HBufC8::NewLC(aString.Length() * 3));
	// sanitise the input string
	const TInt KCharCount(aString.Length());
	for (TInt i(0); i < KCharCount; ++i)
		{
		urlEncoded->Des().AppendFormat(_L8("%%%2x"), aString[i]);
		}
	
	return urlEncoded;
	}

#ifdef __WINS__
void MobblerUtility::OpenAmazonL(const TDesC8& /*aArtist*/, const TDesC8& /*aAlbum*/)
	{
#else
void MobblerUtility::OpenAmazonL(const TDesC8& aArtist, const TDesC8& aAlbum)
	{
	HBufC8* localAmazonDomain(NULL);
	
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
				localAmazonDomain = _L8("amazon.co.uk").AllocL();
				break;
			case ELangFrench:
				localAmazonDomain = _L8("amazon.fr").AllocL();
				break;
			case ELangGerman: 
				localAmazonDomain = _L8("amazon.de").AllocL();
				break;
			case ELangCanadianEnglish:
				localAmazonDomain = _L8("amazon.ca").AllocL();
				break;
			case ELangPrcChinese: 
				localAmazonDomain = _L8("joyo.com").AllocL();
				break;
			case ELangJapanese: 
				localAmazonDomain = _L8("amazon.co.jp").AllocL();
				break;
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	if (!localAmazonDomain)
		{
		// default to .com
		localAmazonDomain = _L8("amazon.com").AllocL();
		}
	
	CleanupStack::PushL(localAmazonDomain);
	
	HBufC8* urlArtist(MobblerUtility::URLEncodeLC(aArtist));
	HBufC8* urlAlbum(MobblerUtility::URLEncodeLC(aAlbum));
	HBufC8* url(HBufC8::NewLC(KAmazonSearchLink().Length() + urlArtist->Length() + urlAlbum->Length() + localAmazonDomain->Length()));
	url->Des().Format(KAmazonSearchLink, &localAmazonDomain->Des(), &urlArtist->Des(), &urlAlbum->Des());
	CMobblerString* urlString(CMobblerString::NewL(*url));
	CleanupStack::PushL(urlString);
	
	CBrowserLauncher* browserLauncher(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->BrowserLauncher());
	
	if (browserLauncher)
		{
		browserLauncher->LaunchBrowserEmbeddedL(urlString->String());
		}
	
	CleanupStack::PopAndDestroy(5);
#endif
	}

TBuf8<2> MobblerUtility::LanguageL()
	{
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

TBuf<30> MobblerUtility::LocalLastFmDomainL()
	{
	TBuf<30> url;
	url.Copy(KEnglishUrl); // default to English
	
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
				url.Copy(KEnglishUrl);
				break;
			case ELangFrench:
				url.Copy(KFrenchUrl);
				break;
			case ELangGerman: 
				url.Copy(KGermanUrl);
				break;
			case ELangSpanish: 
				url.Copy(KSpanishUrl);
				break;
			case ELangItalian: 
				url.Copy(KItalianUrl);
				break;
			case ELangSwedish: 
				url.Copy(KSwedishUrl);
				break;
			case ELangPortuguese: 
				url.Copy(KPortugueseUrl);
				break;
			case ELangRussian: 
				url.Copy(KRussianUrl);
				break;
			case ELangPolish:
				url.Copy(KPolishUrl);
				break;
/*			case ELangPrcChinese: 
				url.Copy(KChineseUrl); // No known Chinese mobile site
				break;*/
			case ELangJapanese: 
				url.Copy(KJapaneseUrl);
				break;
			case ELangTurkish:
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

// End of file
