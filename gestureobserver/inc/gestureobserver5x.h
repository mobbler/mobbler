/*
gestureobserver5x.h

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

#ifndef GESTUREOBSERVER5X_H_
#define GESTUREOBSERVER5X_H_

#include "gestureobserver.h"
#include "gesturetimeout.h"

#include <sensrvchannel.h> 
#include <sensrvchannelconditionlistener.h>
#include <sensrvchannelfinder.h> 
#include <sensrvdatalistener.h>


class CMobblerGestureObserver5x : public CMobblerGestureObserver,
								  public MSensrvDataListener
	{
public:
	~CMobblerGestureObserver5x();
	static CMobblerGestureObserver* NewL();
	
private: // from MSensrvDataListener
	void DataReceived(CSensrvChannel& aChannel, TInt aCount, TInt aDataLost);
	void DataError(CSensrvChannel& aChannel, TSensrvErrorSeverity aError);
	void GetDataListenerInterfaceL(TUid aInterfaceUid, TAny*& aInterface);
	
private:
	// S60 5th edition implementation for template functions.
	void DoStartObservingL();
	void DoStopObservingL();
	
private:
	CMobblerGestureObserver5x();
	void ConstructL();
	
	TSensrvChannelInfo GetChannelL();
	TInt Transform(TInt aValue);
	
private:
	CSensrvChannel* iSensrvChannel;
	CSensrvChannelConditionSet* iConditions;
	};


#endif /*GESTUREOBSERVER5X_H_*/

// End of file

