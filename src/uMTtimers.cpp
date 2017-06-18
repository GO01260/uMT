////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtimers.cpp
//	AUTHOR: Antonio Pastore - March 2017
//	Program originally written by Antonio Pastore, Torino, ITALY.
//	UPDATED: 28 April 2017
//
////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (C) <2017>  Antonio Pastore, Torino, ITALY.
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//	 the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////////




#include "uMT.h"


#if uMT_USE_TIMERS==1

#define uMT_DEBUG 0
#include "uMTdebug.h"

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tm_WakeupAfter
//
// Wake up a task after a time interval.
// CANNOT call from ISR.
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::Tm_WakeupAfter(Timer_t timeout)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	if (timeout != (Timer_t)0)
	{
		/* Just suspend this task and put it in timer list */


		// Get a FREE TIMER
		uTimer *pTimer = &Running->TaskTimer;

		DgbStringPrint(F(" msTickCounter.High="));
		DgbStringPrint(msTickCounter.High);

		DgbStringPrint(F(" msTickCounter.Low="));
		DgbStringPrint(msTickCounter.Low);

		DgbStringPrint(F(" timeout="));
		DgbStringPrint(timeout);

		Running->TaskStatus = S_TBLOCKED;
		pTimer->NextAlarm = msTickCounter + timeout;
		pTimer->Timeout = timeout;

// NOT NEEDED		pTimer->pTask = Running;		// Running task
		pTimer->Flags = uMT_TM_IAM_TASK;	// To reset other flags

		DgbStringPrint(F(" NextAlarm.High="));
		DgbStringPrint(pTimer->NextAlarm.High);

		DgbStringPrint(F(" NextAlarm.Low="));
		DgbStringPrintLN(pTimer->NextAlarm.Low);

		DgbStringPrintLN("uMT: Tm_WakeupAfter(): => TimerQ_Insert()");

		// Insert in the TIMER queue
		TimerQ_Insert(pTimer);

	}	/* else, just do a Round Robin */


	DgbStringPrintLN("uMT: Tm_WakeupAfter(): => Suspend()");


	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region, technically NOT needed  */

	//
	// ...and suspend
	//
	// NOTE:
	// Resched() will insert the running task in the READY list
	//
	Suspend();

	// When returned, timeout is expired
	// Interrupts already enabled!
	

	return(E_SUCCESS);
}


#if	uMT_USE_EVENTS==1
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Timer_EventTimout
//
// Send an EVENT after a time interval.
// CANNOT call from ISR.
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::Timer_EventTimout(Timer_t timeout, Event_t Event, TimerId_t &TmId, TimerFlag_t _Flags)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (timeout == (Timer_t)0)
		return(E_INVALID_TIMEOUT);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	// Get a FREE TIMER
	uTimer *pTimer = TimerQ_PopFree();

	if (pTimer == NULL)
	{
		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */
		return(E_NOMORE_TIMERS);
	}

	DgbStringPrint(F(" msTickCounter.High="));
	DgbStringPrint(msTickCounter.High);

	DgbStringPrint(F(" msTickCounter.Low="));
	DgbStringPrint(msTickCounter.Low);

	DgbStringPrint(F(" timeout="));
	DgbStringPrint(timeout);

	pTimer->NextAlarm = msTickCounter + timeout;
	pTimer->Timeout = timeout;
	pTimer->pTask = Running;		// Running task
	pTimer->Flags = _Flags;	
	pTimer->EventToSend = Event;

	DgbStringPrint(F(" NextAlarm.High="));
	DgbStringPrint(pTimer->NextAlarm.High);

	DgbStringPrint(F(" NextAlarm.Low="));
	DgbStringPrintLN(pTimer->NextAlarm.Low);

	TmId = pTimer->myTimerId;		// Return TIMER ID

	DgbStringPrintLN("uMT: Timer_EventTimout(): => TimerQ_Insert()");

	// Insert in the TIMER queue
	TimerQ_Insert(pTimer);

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	return(E_SUCCESS);
}

#endif

#ifdef uMT_USE_SEMAPHORES

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tm_EvAfter
//
// Send an EVENT after a time interval.
// CANNOT call from ISR.
////////////////////////////////////////////////////////////////////////////////////
// INLINED...

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tm_EvEvery
//
// Send an EVENT after a time interval.
// CANNOT call from ISR.
////////////////////////////////////////////////////////////////////////////////////
// INLINED...


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tm_Cancel
//
// Cancel a TIMER
// CANNOT call from ISR.
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::Tm_Cancel(TimerId_t TmId)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	uTimer *pTimer = Tmid2TimerPtr(TmId);

	if (pTimer == NULL)
		return(E_INVALID_TIMERID);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	Errno_t errno = TimerQ_CancelTimer(pTimer);

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	return(errno);
}
#endif


#endif

//////////////// EOF