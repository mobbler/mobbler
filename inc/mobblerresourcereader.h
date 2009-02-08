/*
mobblerresourcereader.h

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

#ifndef __MOBBLERRESOURCEREADER_H__
#define __MOBBLERRESOURCEREADER_H__

#include <e32base.h>
#include <s32file.h>
#include <barsc.h>

#if defined(__WINS__)
	_LIT(KLanguageRscFile,"Z:\\Resource\\apps\\mobbler_strings.r01");
#else
	_LIT(KLanguageRscFile,"C:\\Resource\\apps\\mobbler_strings.rsc");
#endif
const TInt KLanguageRscVersion(1);

class CMobblerResourceReader
	{
public:
	~CMobblerResourceReader();
	static CMobblerResourceReader* NewLC();
	static CMobblerResourceReader* NewL();

	void AddResourceFileL(const TDesC& aName, TInt aVersion);
	HBufC8* AllocRead8LC(TInt aResourceId);
	HBufC* AllocReadLC(TInt aResourceId);
	HBufC* AllocReadL(TInt aResourceId);

private:
	void ConstructL();

private:
	RResourceFile iResourceFile;
	TBool iErrorDialogShown;
	};

#endif // __MOBBLERRESOURCEREADER_H__

// End of file
