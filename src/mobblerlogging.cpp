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

void CMobblerLogging::DumpDataL(const TDesC8& aData, const TDesC& aLogFile)
	{
#ifdef __WINS__
	TFileName logFile(_L("C:"));
#else
	TFileName logFile(_L("E:\\Mobbler\\"));
#endif
	logFile.Append(aLogFile);

	RFile file;
	CleanupClosePushL(file);
	CCoeEnv::Static()->FsSession().MkDirAll(logFile);
	User::LeaveIfError(file.Replace(CCoeEnv::Static()->FsSession(), logFile, EFileWrite));
	User::LeaveIfError(file.Write(aData));
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerLogging::LogL(const TDesC8& aText)
	{
	TBuf<255> text;
	text.Copy(aText);
	CEikonEnv::Static()->InfoMsg(text);

#ifdef __WINS__
	TFileName logFile(_L("C:mobbler.log"));
#else
	TFileName logFile(_L("E:\\Mobbler\\mobbler.log"));
#endif

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
	TInt position = 0;
	file.Seek(ESeekEnd, position);
	User::LeaveIfError(file.Write(aText));
	User::LeaveIfError(file.Write(_L8("\n")));
	CleanupStack::PopAndDestroy(&file);
	}

#endif // _DEBUG

// End of file
