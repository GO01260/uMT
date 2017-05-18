////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTsemaphores.cpp
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

#define uMT_DEBUG 0
#include "uMTdebug.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////



#if uMT_USE_SEMAPHORES==1
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::iSm_Claim
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Sm_Claim(SemId_t Sid, uMToptions_t Options
#if uMT_USE_TIMERS==1
	, Timer_t timeout
#endif
	)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (SemId_Check(Sid) == FALSE)
		return(E_INVALID_SEMID);

	CpuStatusReg_t	CpuFlags = IntLock();	/* Enter critical region */

	uMTsem *pSem = &SemList[Sid];

	if (pSem->SemValue > 0) // Semaphore is free
	{
		pSem->SemValue--;		// Take the semaphore
		IntUnlock(CpuFlags);	/* End of critical region */

		return(E_SUCCESS);
	}

	////////////////////////////////////////////
	// Semaphore is BUSY
	////////////////////////////////////////////

	if (Options == uMT_NOWAIT)
	{
		IntUnlock(CpuFlags);	/* End of critical region */

		return(E_WOULD_BLOCK);
	}

	// Insert in the SEM queue
	pSem->SemQueue.Insert(Running);

	// Remember the Queue
	Running->pSemq = pSem;

	Running->TaskStatus = S_SBLOCKED;


#if uMT_USE_TIMERS==1
	uTimer *pTimer = &Running->TaskTimer;

	if (timeout != (Timer_t)0)
	{
		/////////////////////////////////////////////////
		// Create a TASK TIMER to manage the timeout
		////////////////////////////////////////////////
		pTimer->Timeout = timeout;

		pTimer->NextAlarm = TickCounter + timeout;
		pTimer->Flags = uMT_TM_IAM_TASK;	// To reset other flags
		
		Running->TaskStatus = S_TBLOCKED;	// TIMER BLOCKED

		DgbStringPrint("uMT(");
		DgbValuePrint(TickCounter.Low);
		DgbStringPrint("): Sm_Claim(myTid=");
		DgbValuePrint(Running->myTid);
		DgbStringPrint("): timeout = ");
		DgbValuePrint(timeout);
		DgbStringPrintLN(" - TimerQ_Insert()");

		TimerQ_Insert(pTimer);
	}
#endif

	//////////////////////////////////////////////////////////
	// Suspend task and generate a rescheduling.
	// It will "return" only when this task is S_RUNNING again
	///////////////////////////////////////////////////////////
	Suspend();

	/*************************************************************
	* Some notes:
	*** Suspend() ALWAYS returns!
	*** uMT assures that a task is restarted when the proper
	* conditions are satisfied, i.e. some event is arrived.
	* When the control is BACK again, the task is already
	* the RUNNING task.
	* INTERRUPTS are ENABLED!
	*******************************************************************/

	/* Note: When control comes back, sem is already taken. This
		is done to avoid a lucky task to enter sem in the time
		the previous user has released it and this task will
		be running */

#if uMT_USE_TIMERS==1
	if (timeout != (Timer_t)0)		// A timer was set
	{
		IntLock();	/* Reentering critical region, do NOT save FLAGS */

		DgbStringPrint("uMT(");
		DgbValuePrint(TickCounter.Low);
		DgbStringPrint("): Sm_Claim(myTid=");
		DgbValuePrint(Running->myTid);

		// Timeout expired?
		if (pTimer->Flags & uMT_TM_EXPIRED)
		{
			DgbStringPrint("): uMT_TM_EXPIRED - returning ");

			// Did we get the SEMAPHORE?
			if (Running->pSemq != NULL)
			{
				// No chance...

				DgbStringPrintLN("E_TIMEOUT");

				IntUnlock(CpuFlags);	/* End of critical region */

				return(E_TIMEOUT);	/* Return error if any */
			}

			// We now OWN the semaphore
			DgbStringPrintLN("E_SUCCESS");


		}
		else
		{
			// We now OWN the semaphore
			// Timer not expired, cancel

			DgbStringPrintLN("): not uMT_TM_EXPIRED, cancelling TIMER");

			if (TimerQ_CancelTimer(pTimer) != E_SUCCESS)
			{
				DgbStringPrint("uMT(");
				DgbValuePrint(TickCounter.Low);
				DgbStringPrint("): Sm_Claim(): Timer Flag = 0x");
				DgbValuePrint2LN(pTimer->Flags, HEX);

				iKn_FatalError(F("TimerQ_CancelTimer: Timer not found!"));
			}
		}
	}
#endif
		

	IntUnlock(CpuFlags);	/* End of critical region */

	return(E_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Sm_Release
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Sm_Release(SemId_t Sid)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (SemId_Check(Sid) == FALSE)
		return(E_INVALID_SEMID);

	if (SemList[Sid].SemValue == uMT_MAX_SEM_VALUE)
	{
		return(E_OVERFLOW_SEM);
	}

	CpuStatusReg_t	CpuFlags = IntLock();	/* Enter critical region */

	uMTsem *pSem = &SemList[Sid];

	// Any task waiting?
	if (pSem->SemQueue.Head != NULL)
	{
		/* Now remove it from sem queue */
		uTask	*pTask = pSem->SemQueue.GetFirst();

		pTask->pSemq = NULL;		// This task OWNS the semaphore and it is NOT any longer in the semaphore queue...

		/* Make this task READY... */
		ReadyTask(pTask);

		/* ... and check for preemption */
		Check4Preemption();

	}
	else
	{
		// Release Sem
		pSem->SemValue++;
	}

	IntUnlock(CpuFlags);	/* End of critical region */

	return(E_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Sm_SetQueueMode
//
// Note: default Queue Mode is QUEUE_PRIO
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Sm_SetQueueMode(SemId_t Sid, QueueMode_t Mode)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (SemId_Check(Sid) == FALSE)
		return(E_INVALID_SEMID);

	if (Mode != QUEUE_NOPRIO && Mode != QUEUE_PRIO)
		return(E_INVALID_OPTION);

	SemList[Sid].SemQueue.SetQueueMode(Mode); 

	return(E_SUCCESS);
}

#endif



/////////////////////// EOF