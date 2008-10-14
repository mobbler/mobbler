/*
mobblerutility.cpp

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

#include "mobblerutility.h"
#include <hash.h>
#include <bautils.h> 
#include <coemain.h> 

_LIT8(KEnglishLangCode, "en");
_LIT8(KFrenchLangCode, "fr");
_LIT8(KGermanLangCode, "de");
_LIT8(KSpanishLangCode, "es");
_LIT8(KItalianLangCode, "it");
_LIT8(KSwedishLangCode, "sv");
_LIT8(KPortugueseLangCode, "pt");
_LIT8(KRussianLangCode, "ru");
_LIT8(KPolishLangCode, "pl");
_LIT8(KChineseLangCode, "cn");
_LIT8(KJapaneseLangCode, "jp");

HBufC8* MobblerUtility::MD5LC(const TDesC8& aSource)
	{
	CMD5* md5 = CMD5::NewL();
	CleanupStack::PushL(md5);
	
	TPtrC8 hash = md5->Hash(aSource);
	HBufC8* hashResult = HBufC8::NewLC(hash.Length() * 2); 
	
	for (TInt i(0) ; i < hash.Length() ; ++i)
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
	HBufC8* urlEncoded = HBufC8::NewLC(aString.Length() * 3);
	// sanitise the input string
	const TInt KCharCount(aString.Length());
	for (TInt i(0) ; i < KCharCount; ++i)
		{
		urlEncoded->Des().AppendFormat(_L8("%%%2x"), aString[i]);
		}
	
	return urlEncoded;
	}

HBufC8* MobblerUtility::URLEncodeLC(const TDesC& aString)
	{
	HBufC8* urlEncoded = HBufC8::NewLC(aString.Length() * 3);
	// sanitise the input string
	const TInt KCharCount(aString.Length());
	for (TInt i(0) ; i < KCharCount; ++i)
		{
		urlEncoded->Des().AppendFormat(_L8("%%%2x"), aString[i]);
		}
	
	return urlEncoded;
	}

TBuf8<2> MobblerUtility::LanguageL()
	{
	TBuf8<2> language;
	language.Copy(KEnglishLangCode); // default to english
	
	RArray<TLanguage> downgradePath;
	CleanupClosePushL(downgradePath);
	BaflUtils::GetDowngradePathL(CCoeEnv::Static()->FsSession(), User::Language(), downgradePath);
	
	TBool languageFound(EFalse);
	
	const TInt KLanguageCount(downgradePath.Count());
	for (TInt i(0) ; i < KLanguageCount && !languageFound ; ++i)
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
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	return language;
	}

