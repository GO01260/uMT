////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtimersQueueMgmnt.cpp
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
//	uMT::TimerQ_Insert
//
// Entered with INTs disabled
//
////////////////////////////////////////////////////////////////////////////////////
void  uMT::TimerQ_Insert(uTimer *pTimer)	/* Timer agent */
{
	CHECK_INTS("TimerQ_Insert");		// Verify if INTS are disabled...

	TotTimerQueued++;

	// Insert in the ordered queue

	if (TimerQueue == NULL)
	{
		TimerQueue = pTimer;
		pTimer->Next = NULL;

		return;
	}

	uTimer *pScan = TimerQueue;
	uTimer *pLast = NULL;

	do
	{
		CHECK_TIMER_MAGIC(pScan, "TimerQ_Insert");

		if (pScan->NextAlarm > pTimer->NextAlarm)
		{
			// Inserted as first element?
			if (pScan == TimerQueue)
			{
				pTimer->Next = TimerQueue;
				TimerQueue = pTimer;
			}
			else
			{
				// Insert here
				pTimer->Next = pScan;
				pLast->Next = pTimer;
			}

			return;
		}

		pLast = pScan;
		pScan = pScan->Next;

	} while (pScan != NULL);

	// Add at the end
	pLast->Next = pTimer;
	pTimer->Next = NULL;

}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_Pop
//
// (Entered with INTs disabled)
//
////////////////////////////////////////////////////////////////////////////////////
uTimer * uMT::TimerQ_Pop()
{
	CHECK_INTS("TimerQ_Pop");		// Verify if INTS are disabled...

	if (TimerQueue == NULL)
	{
		if (TotTimerQueued != 0)
		{
			isrKn_FatalError(F("TimerQ_Pop: TimerQueue != TotalQueued"));
		}

		return(NULL);
	}

	TotTimerQueued--;

	uTimer *pTimer = TimerQueue;
	TimerQueue = TimerQueue->Next;

	return(pTimer);

}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_Expired
//
// (Entered with INTs disabled)
// pTimer already outside timer queue
// Agent timer only
////////////////////////////////////////////////////////////////////////////////////
void uMT::TimerQ_Expired(uTimer *pTimer)
{
	CHECK_INTS("TimerQ_Expired");		// Verify if INTS are disabled...

	uTask *pTask = pTimer->pTask;

	if (pTimer->Flags & uMT_TM_REPEAT)
	{
		// Insert in the TIMER queue again
		pTimer->NextAlarm = TickCounter + pTimer->Timeout;

		TimerQ_Insert(pTimer);
	}
	else
	{
		if (pTimer->Flags & uMT_TM_IAM_AGENT)	// Free timer
		{
			TimerQ_PushFree(pTimer);
		}
		else
		{
			// TASK TIMER, set uMT_TM_EXPIRED 
			pTimer->Flags |= uMT_TM_EXPIRED;
		}
	}

#if	uMT_USE_EVENTS==1
	if ((pTimer->Flags & uMT_TM_IAM_AGENT) && (pTimer->Flags & uMT_TM_SEND_EVENT))	// This is only valid for AGENT TIMERS
		EventSend(pTask, pTimer->EventToSend);
#endif
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_CancelTmId
//
// (Entered with INTs disabled)
// 
// Cancel a TimerId
////////////////////////////////////////////////////////////////////////////////////
Errno_t uMT::TimerQ_CancelTimer(uTimer *pTimer)
{
	CHECK_INTS("TimerQ_CancelTmId");		// Verify if INTS are disabled...

	uTimer *pScan = TimerQueue;
	uTimer *pLast = NULL;

	while (pScan != NULL)
	{
		// At least one TIMER exists...
		CHECK_TIMER_MAGIC(pScan, "TimerQ_CancelTmId");

		if (pScan == pTimer)
		{
			// FOUND!

			if (pScan == TimerQueue)		// Is it the first element?
			{
				TimerQueue = TimerQueue->Next;
			}
			else
			{
				// pLast always exists...
				pLast->Next = pScan->Next;
			}

			if (pScan->Flags & uMT_TM_IAM_AGENT)	// Free timer
				TimerQ_PushFree(pScan);

			return(E_SUCCESS);
		}
	}

	return(E_NOT_OWNED_TIMER);
}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_CancelAll
//
// (Entered with INTs disabled)
//
// Cancel all timers owned by a TASK
////////////////////////////////////////////////////////////////////////////////////
void uMT::TimerQ_CancelAll(uTask *pTask)
{
	CHECK_INTS("TimerQ_CancelAll");		// Verify if INTS are disabled...

	uTimer *pScan = TimerQueue;
	uTimer *pLast = NULL;

	while (pScan != NULL)
	{
		// At least one TIMER exists...
		CHECK_TIMER_MAGIC(pScan, "TimerQ_CancelAll");

		if (pScan->pTask == pTask)
		{
			// FOUND!

			if (pScan == TimerQueue)		// Is it the first element?
			{
				TimerQueue = TimerQueue->Next;
			}
			else
			{
				// pLast always exists...
				pLast->Next = pScan->Next;
			}

			if (pScan->Flags & uMT_TM_IAM_AGENT)	// Free timer
				TimerQ_PushFree(pScan);
		}
	}




}
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_PopFree
//
// (Entered with INTs disabled)
//
////////////////////////////////////////////////////////////////////////////////////
uTimer * uMT::TimerQ_PopFree()
{
	CHECK_INTS("TimerQ_PopFree");		// Verify if INTS are disabled...

	if (FreeTimerQueue == NULL)
		return(NULL);


	uTimer *pTimer = FreeTimerQueue;
	FreeTimerQueue = FreeTimerQueue->Next;

	CHECK_TIMER_MAGIC(pTimer, "TimerQ_PopFree");

	return(pTimer);
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::TimerQ_PushFree
//
// (Entered with INTs disabled)
//
////////////////////////////////////////////////////////////////////////////////////
void uMT::TimerQ_PushFree(uTimer *pTimer)
{
	CHECK_INTS("TimerQ_PushFree");		// Verify if INTS are disabled...

	CHECK_TIMER_MAGIC(pTimer, "TimerQ_PopFree");

	if (pTimer->Flags & uMT_TM_IAM_AGENT)
	{
#if uMT_SAFERUN==1
		if (pTimer->myTimerId < uMT_MAX_TASK_NUM)
		{
			SerialPrintln(F("uMT: TimerQ_PushFree: trying to free an TASK Timer"));

			isrKn_FatalError();

		}
#endif

		// We can only free Timer AGENT
		pTimer->Next = FreeTimerQueue;
		FreeTimerQueue = pTimer;
	}
}



#endif

//////////////// EOF