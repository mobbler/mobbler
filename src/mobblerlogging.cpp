/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009, 2010, 2011  Hugo van Kemenade
Copyright (C) 2009  Michael Coffey

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

#ifdef _DEBUG

#include <bautils.h>
#include <coemain.h>
#include <eikenv.h>
#include <f32file.h>

#include "mobblerlogging.h"
#include "mobblerstring.h"
//#include "mobblertracer.h"

_LIT(KLogPath, "C:\\data\\Mobbler\\");
_LIT(KLogFilename, "mobbler.log");
_LIT8(KCommaSpace, ", ");
_LIT8(KNewline, "\n");
_LIT8(KNull, "NULL");
const TInt KMaxLoggingTextSize(255);

void CMobblerLogging::DumpDataL(const TDesC8& aData, const TDesC& aLogFile)
	{
//	TRACER_AUTO;
	TFileName logFile(KLogPath);
	logFile.Append(aLogFile);

	RFile file;
	CleanupClosePushL(file);
	CCoeEnv::Static()->FsSession().MkDirAll(logFile);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), logFile, EFileWrite));
	User::LeaveIfError(file.Write(aData));
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLogging::LogL(const TInt aNumber)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	text8.AppendNum(aNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TInt aFirstNumber, const TInt aSecondNumber)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	text8.AppendNum(aFirstNumber);
	text8.Append(KCommaSpace);
	text8.AppendNum(aSecondNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC& aText)
	{
//	TRACER_AUTO;
	HBufC8* text8(HBufC8::NewLC(aText.Length()));
	text8->Des().Copy(aText);
	LogL(*text8);
	CleanupStack::PopAndDestroy(text8);
	}

void CMobblerLogging::LogL(const TUid aUid)
	{
//	TRACER_AUTO;
	LogL(aUid.Name());
	}

void CMobblerLogging::LogL(const TDesC8& aText, const TInt aNumber)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	text8.Append(aText);
	text8.Append(KCommaSpace);
	text8.AppendNum(aNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aFirstText, const TDesC8& aSecondText)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	text8.Append(aFirstText);
	text8.Append(KCommaSpace);
	text8.Append(aSecondText);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aFirstText, const TDesC& aSecondText)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	text8.Append(aFirstText);
	text8.Append(KCommaSpace);
	text8.Append(aSecondText);
	LogL(text8);
	}

void CMobblerLogging::LogL(const CMobblerString* aMobblerString)
	{
//	TRACER_AUTO;
	TBuf8<KMaxLoggingTextSize> text8;
	if (aMobblerString)
		{
		text8.Append(aMobblerString->String8());
		}
	else
		{
		text8.Append(KNull);
		}
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aText)
	{
//	TRACER_AUTO;
	HBufC* text(HBufC::NewLC(aText.Length()));
	text->Des().Copy(aText);
	CEikonEnv::Static()->InfoMsg(*text);
	CleanupStack::PopAndDestroy(text);

	TFileName logFile(KLogPath);
	logFile.Append(KLogFilename);

	RFile file;
	CleanupClosePushL(file);
	if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), logFile))
		{
		User::LeaveIfError(file.Open(CCoeEnv::Static()->FsSession(), logFile, EFileWrite));
		}
	else
		{
		CCoeEnv::Static()->FsSession().MkDirAll(logFile);
		User::LeaveIfError(file.Create(CCoeEnv::Static()->FsSession(), logFile, EFileWrite));
		}
	TInt position(0);
	file.Seek(ESeekEnd, position);
	User::LeaveIfError(file.Write(aText));
	User::LeaveIfError(file.Write(KNewline));
	CleanupStack::PopAndDestroy(&file);
	}

#endif // _DEBUG

// End of file
