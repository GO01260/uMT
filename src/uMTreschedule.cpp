////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTreschedule.cpp
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

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Reschedule
//
// It can only be called from:
//		Suspend()
//		Suspend2()
//		Tk_DeleteTask()
//		BadExit()
//
// Reschedule() will RoundRobin the running task, if any
//
// It should be called with INTS disabled.

// Only if eneterd with Running->TaskStatus == S_RUNNING, it will put the task in the ready queue
// This routine NEVER RETURNS!
//
////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
void uMT::Reschedule()
#else
void __attribute__ ((noinline)) uMT::Reschedule()
#endif
{
	// On entry:
	// 1) Running still set to current task.
	// 2) Running SP already saved

	DgbStringPrintLN("uMT: Reschedule(): entry");
	
	
	// Sanity Ceck.... There is ALWAYS a RUNNING task
	CHECK_TASK_MAGIC(Running, "Reschedule(entry)");


	CHECK_INTS("Reschedule");		// Verify if INTS are disabled...

//	CpuStatusReg_t	CpuFlags = IntLock();	// Just in case we made a previous mistake...

	///////////////////////////////////////////////////////////////////////
	////			NOW RUNNING WITH INTS DISABLED!!!!
	///////////////////////////////////////////////////////////////////////

	LastRunning = Running;		// Remember last task...

	if (Running->TaskStatus == S_RUNNING)
	{
		//
		// We were probably called from the timer-ticks routine: time slicing has been expired.
		// Or we were called simply to do a round robin.
		//

		DgbStringPrint("uMT: Reschedule(): Running->TaskStatus == S_RUNNING, => ReadyTask(), SavedSP = ");
		DgbValuePrintLN((unsigned int)Running->SavedSP);

		// Add in the Ready Queue
		ReadyTask(Running);
	}


	////////////////////////////////////////////////////////////////
	// NOTE: Now THERE IS NO MORE RUNNING TASK!!!
	////////////////////////////////////////////////////////////////

#if uMT_USE_TIMERS==1

	// Check for expired Timers: AlarmExpired set in uMTdoTicksWork()
	while (AlarmExpired == TRUE)		// Just to enter the first time....
	{

		// Remove first timer from the Queue
		uTimer	*pTimer = TimerQ_Pop();

		if (pTimer == NULL)
		{
			// Embarassing...
			DgbStringPrintLN("uMT: Reschedule(AlarmExpired): NULL timer... ");

			break;
		}

		if (pTimer->Flags & uMT_TM_IAM_TASK)
		{
			DgbStringPrint("uMT: Reschedule(AlarmExpired): TASK: Tid=");
			DgbValuePrintLN(pTimer->pTask->myTid);

			// TASK: make it ready
			ReadyTask(pTimer->pTask);

			// TASK TIMER, set uMT_TM_EXPIRED 
			pTimer->Flags |= uMT_TM_EXPIRED;


			// DO NOT RELEASE THE TIMER!!! [NO TimerQ_PushFree(pTimer)]
		}
		else
		{
			DgbStringPrint("uMT: Reschedule(AlarmExpired): AGENT: Tid=");
			DgbValuePrintLN(pTimer->pTask->myTid);

			// AGENT: timer expired
			TimerQ_Expired(pTimer);
		}

		// Next Timer?
		if (TimerQueue == NULL)
		{
			// No more timers
			break;
		}

		if (TimerQueue->NextAlarm > TickCounter)
		{
			// Not yet expired
			break;
		}
	
	}
#endif


	///////////////////////////////////////////////////
	//	Setup Globals variables
	///////////////////////////////////////////////////
	NoResched = 0;
	NeedResched = FALSE;


	///////////////////////////////////////////////////
	// Choose a task to RUN
	///////////////////////////////////////////////////
	Running = (ReadyQueue.Head != NULL ? ReadyQueue.GetFirst() : IdleTaskPtr);
	Running->TaskStatus = S_RUNNING;	/* Put it running */

	
	CHECK_TASK_MAGIC(Running, "Reschedule(Resume)");


	DgbStringPrint("uMT: Reschedule(): => ResumeTask() - TID = ");
	DgbValuePrintLN(Running->myTid);


	/* Now run task */
	ResumeTask(Running->SavedSP);	// INTS enabled in ResumeTask()

	/* ... NEVER RETURNS!! */

}



//////////////////////////// EOF