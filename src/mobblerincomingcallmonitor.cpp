/*
mobblerincomingcallmonitor.cpp

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

#include "mobblerincomingcallmonitor.h"

//Telephony call handling PS Uid. 
const TUid KPSUidTelephonyCallHandling = {0x101F8787};

//State of ongoing call(s). 
const TUint32 KTelephonyCallState = 0x00000004;

CMobblerIncomingCallMonitor* CMobblerIncomingCallMonitor::NewL(MMobblerIncomingCallMonitorObserver& aObserver)
	{
	CMobblerIncomingCallMonitor* self = new(ELeave) CMobblerIncomingCallMonitor(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerIncomingCallMonitor::CMobblerIncomingCallMonitor(MMobblerIncomingCallMonitorObserver& aObserver)
	:CActive(EPriorityStandard), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}

void CMobblerIncomingCallMonitor::ConstructL()
	{
	iProperty.Attach(KPSUidTelephonyCallHandling, KTelephonyCallState);
	iProperty.Subscribe(iStatus);
	SetActive();
	}


CMobblerIncomingCallMonitor::~CMobblerIncomingCallMonitor()
	{
	Cancel();
	iProperty.Close();
	}

void CMobblerIncomingCallMonitor::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		// register for more notifications
		iProperty.Subscribe(iStatus);
		SetActive();
		
		TInt state;
		User::LeaveIfError(iProperty.Get(state));
		iObserver.HandleIncomingCallL(static_cast<MMobblerIncomingCallMonitorObserver::TPSTelephonyCallState>(state));
		}
	}

void CMobblerIncomingCallMonitor::DoCancel()
	{
	iProperty.Cancel();
	}
