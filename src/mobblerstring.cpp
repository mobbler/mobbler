/*
mobblerstring.cpp

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

#include <utf.h> 

#include "mobblerstring.h"

CMobblerString* CMobblerString::NewL(const TDesC& aString)
	{
	CMobblerString* self = new(ELeave) CMobblerString;
	CleanupStack::PushL(self);
	self->ConstructL(aString);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerString* CMobblerString::NewL(const TDesC8& aString)
	{
	CMobblerString* self = new(ELeave) CMobblerString;
	CleanupStack::PushL(self);
	self->ConstructL(aString);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerString::CMobblerString()
	{
	}

void CMobblerString::ConstructL(const TDesC& aString)
	{
	iString = aString.AllocL();
	iString8 = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aString);
	
	iStringPtr.Set(*iString);
	iString8Ptr.Set(*iString8);
	}

void CMobblerString::ConstructL(const TDesC8& aString)
	{
	iString8 = aString.AllocL();
	iString = CnvUtfConverter::ConvertToUnicodeFromUtf8L(aString);
	
	iStringPtr.Set(*iString);
	iString8Ptr.Set(*iString8);
	}

CMobblerString::~CMobblerString()
	{
	delete iString;
	delete iString8;
	}

const TPtrC& CMobblerString::String() const
	{
	return iStringPtr;
	}

const TPtrC8& CMobblerString::String8() const
	{
	return iString8Ptr;
	}
