/*
mobberlogging.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#ifdef _DEBUG

#include <bautils.h>
#include <coemain.h>
#include <eikenv.h>
#include <f32file.h>

#include "mobblerlogging.h"
#include "mobblerstring.h"

#ifdef __WINS__
_LIT(KLogPath, "C:");
#else
_LIT(KLogPath, "C:\\Mobbler\\");
#endif

_LIT(KLogFilename, "mobbler.log");

void CMobblerLogging::DumpDataL(const TDesC8& aData, const TDesC& aLogFile)
	{
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
	TBuf8<255> text8;
	text8.AppendNum(aNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TInt aFirstNumber, const TInt aSecondNumber)
	{
	TBuf8<255> text8;
	text8.AppendNum(aFirstNumber);
	text8.Append(_L8(", "));
	text8.AppendNum(aSecondNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC& aText)
	{
	HBufC8* text8(HBufC8::NewLC(aText.Length()));
	text8->Des().Copy(aText);
	LogL(*text8);
	CleanupStack::PopAndDestroy(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aText, const TInt aNumber)
	{
	TBuf8<255> text8;
	text8.Append(aText);
	text8.Append(_L8(", "));
	text8.AppendNum(aNumber);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aFirstText, const TDesC8& aSecondText)
	{
	TBuf8<255> text8;
	text8.Append(aFirstText);
	text8.Append(_L8(", "));
	text8.Append(aSecondText);
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aFirstText, const TDesC& aSecondText)
	{
	TBuf8<255> text8;
	text8.Append(aFirstText);
	text8.Append(_L8(", "));
	text8.Append(aSecondText);
	LogL(text8);
	}

void CMobblerLogging::LogL(const CMobblerString* aMobblerString)
	{
	TBuf8<255> text8;
	if (aMobblerString)
		{
		text8.Append(aMobblerString->String8());
		}
	else
		{
		text8.Append(_L8("NULL"));
		}
	LogL(text8);
	}

void CMobblerLogging::LogL(const TDesC8& aText)
	{
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
	User::LeaveIfError(file.Write(_L8("\n")));
	CleanupStack::PopAndDestroy(&file);
	}

#endif // _DEBUG

// End of file
