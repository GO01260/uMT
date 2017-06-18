////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTevents.cpp
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


#if uMT_USE_EVENTS==1

#define uMT_DEBUG 0
#include "uMTdebug.h"


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::EventCheck
//
// Entered with INTS disabled
//
////////////////////////////////////////////////////////////////////////////////////
Bool_t uMT::EventVerified(uTask *pTask)
{
	CHECK_INTS("EventVerified");		// Verify if INTS are disabled...

	Event_t result = pTask->EV_requested & pTask->EV_received;

	if (((pTask->EV_condition & uMT_ANY) && (result != 0)) /* OR condition */
			|| (result == pTask->EV_requested) ) /* AND condition */
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}

}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::EventSend
//
// Entered with INTS disabled
// Also called from Reschedule()
//
////////////////////////////////////////////////////////////////////////////////////
void uMT::EventSend(uTask *pTask, Event_t Event)
{
	CHECK_INTS("EventSend");		// Verify if INTS are disabled...

	/* Set event */
	pTask->EV_received |= Event;

	/* Was 'tid' waiting for some event? */
	if (pTask->TaskStatus == S_EBLOCKED || pTask->TaskStatus == S_TBLOCKED)
	{
		if (EventVerified(pTask))
		{
			DgbStringPrint("uMT: EventSend(): ReadyTask => ");
			DgbValuePrintLN(pTask->myTid);

			/* Make this task READY... */
			ReadyTask(pTask);

			// Do not trigger a Suspend(), this cannot be done inside the rescheduling
			Check4Preemption();		
		}
	}

}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Ev_Send
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::doEv_Send(TaskId_t Tid, Event_t Event, Bool_t AllowPreemption)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	uTask *pTask;

	if ((pTask = GetTaskPointer(Tid)) == NULL)
		return(E_INVALID_TASKID);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	// Check for pending events
	EventSend(pTask, Event);

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	if (AllowPreemption)
		Check4NeedReschedule();		// Call Suspend() if NeedResched==TRUE

   return(E_SUCCESS);
}


/**********************************************************************
 * OKev_receive - The running task is trying to receive an event.
 * 'flags' can be mtANY condition to indicate at LEAST ONE of them.
 * 'flags' can be include mtNOWAIT that means return immediately 
 * with 'eventout' filled with already arrived events.
 * If the event requested is available, it returns events received in
 * 'eventout' otherwise the task is supended.
 **********************************************************************/
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::iEv_Receive
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::Ev_Receive(
	Event_t	eventin,
	uMToptions_t	flags,
	Event_t	*eventout
#if uMT_USE_TIMERS==1
	, Timer_t timeout
#endif
	)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	if (eventin == 0)
	{
		/* Simply return current received events */
		if (eventout != NULL)
			*eventout = Running->EV_received;

		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

		return(E_SUCCESS);
	}

	Running->EV_requested = eventin;
	Running->EV_condition = flags;

	if (EventVerified(Running) == FALSE)
	{
		///////////////////////////////////////
		// Not RECEIVED yet
		///////////////////////////////////////

		if (flags & uMT_NOWAIT)
		{
			/* Task has choosen NOT to wait */
			/* Return current pending events */
			if (eventout != NULL)
				*eventout = Running->EV_received;
		
			isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

			DgbStringPrint("uMT: Ev_Receive(): flags = 0X");
			DgbValuePrint(flags);
			DgbStringPrintLN(" - returning E_WOULD_BLOCK");

			return(E_WOULD_BLOCK);
		}


		// EVENT BLOCKED
		Running->TaskStatus = S_EBLOCKED;
	
#if uMT_USE_TIMERS==1
		uTimer *pTimer = &Running->TaskTimer;

		pTimer->Timeout = timeout;

		if (timeout != (Timer_t)0)
		{
			/////////////////////////////////////////////////
			// Create a TASK TIMER to manage the timeout
			////////////////////////////////////////////////

			pTimer->NextAlarm = msTickCounter + timeout;
			pTimer->Flags = uMT_TM_IAM_TASK;	// To reset other flags
		
			Running->TaskStatus = S_TBLOCKED;	// TIMER BLOCKED

			DgbStringPrint("uMT(");
			DgbValuePrint(msTickCounter.Low);
			DgbStringPrint("): Ev_Receive(): timeout = ");
			DgbValuePrint(timeout);
			DgbStringPrintLN(" - TimerQ_Insert()");

			TimerQ_Insert(pTimer);
		}
#endif


		DgbStringPrint("uMT(");
		DgbValuePrint(msTickCounter.Low);
		DgbStringPrintLN("): Ev_Receive(): Suspend()...");

		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

		//////////////////////////////////////////////////////////
		// Suspend task and generate a rescheduling.
		// It will "return" only when this task is S_RUNNING again
		///////////////////////////////////////////////////////////
		Suspend();
	
	
		/******************************************************************
		 * Some notes:
		 * Suspend() ALWAYS returns!
		 * MT assures that a task is restarted when the proper
		 * conditions are satisfied, i.e. some event is arrived.
		 * When the control is BACK again, the task is already the RUNNING task.
		 * INTERRUPTS are ENABLED!
		 *******************************************************************/

		CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

#if uMT_USE_TIMERS==1
		if (pTimer->Timeout != (Timer_t)0)		// A timer was set
		{
			DgbStringPrint("uMT(");
			DgbValuePrint(msTickCounter.Low);
			DgbStringPrint("): Ev_Receive(myTid=");
			DgbValuePrint(Running->myTid);

			// Timeout expired?
			if (pTimer->Flags & uMT_TM_EXPIRED)
			{
				DgbStringPrintLN("): uMT_TM_EXPIRED");

				// Give another chance....
				if (EventVerified(Running) == FALSE)
				{
					// No chance...

					if (eventout != NULL)
						*eventout = Running->EV_received;

					DgbStringPrint("uMT(");
					DgbValuePrint(msTickCounter.Low);
					DgbStringPrintLN("): Ev_Receive(): returning E_TIMEOUT");

					isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

					return(E_TIMEOUT);	/* Return error if any */
				}
			}
			else
			{
				 // Timer not expired, cancel

				DgbStringPrintLN("): not uMT_TM_EXPIRED, cancelling TIMER");

				if (TimerQ_CancelTimer(pTimer) != E_SUCCESS)
				{
					DgbStringPrint("uMT(");
					DgbValuePrint(msTickCounter.Low);
					DgbStringPrint("): Ev_Receive(): Timer Flag = 0x");
					DgbValuePrint2LN(pTimer->Flags, HEX);

					isr_Kn_FatalError(F("TimerQ_CancelTimer: Timer not found!"));
				}
			}
		}
#endif

	} // end of: if (EventVerified(Running) == FALSE)



	///////////////////////////////////////////////////////////////////////
	// Now the requested events are arrived, return current received events
	///////////////////////////////////////////////////////////////////////
	if (eventout != NULL)
		*eventout = Running->EV_received;

	// Clear events
	Running->EV_received = Running->EV_requested = uMT_NULL_EVENT;

	DgbStringPrint("uMT(");
	DgbValuePrint(msTickCounter.Low);
	DgbStringPrintLN("): Ev_Receive(): returning E_SUCCESS");

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	return(E_SUCCESS);	/* Return error if any */
}


#endif


/////////////////////// EOF