////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtasksQueueMgmnt.cpp
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
//	uMT::ReadyTask
//
// Make a task READY and insert in the READY queue
//
// Entered with INTS disabled
//
////////////////////////////////////////////////////////////////////////////////////
void	uMT::ReadyTask(uTask *pTask)
{
	CHECK_INTS("ReadyTask");		// Verify if INTS are disabled...


	DgbStringPrint("uMT: ReadyTask(): TID => ");
	DgbValuePrintLN(pTask->myTid);

	if (pTask == IdleTaskPtr)		// IDLE task SHALL NOT BE in any queue...
	{
		pTask->TaskStatus = S_READY;		/* Now ready to run */
	}
	else
	{
		// Ignore already READY task: they are already in the READY queue!!!
		if (pTask->TaskStatus != S_READY)
		{
			/* Put task in the ready list */
			ReadyQueue.Insert(pTask);
			pTask->TaskStatus = S_READY;		/* Now ready to run */
		}
	}

}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Check4Preemption
//
// Set NeedResched and return it.
//
// This routine can also be called from ISR, so AllowPreemption controls if a real preemption can be done.
//
// It must be entered with INTS disabled
////////////////////////////////////////////////////////////////////////////////////
void	uMT::Check4Preemption()
{
	CHECK_INTS("Check4Preemption");		// Verify if INTS are disabled...

	if (ReadyQueue.Head != NULL &&
		ReadyQueue.Head->Priority > Running->Priority &&
		!(NoPreempt))
	{
		/* If running task has lower priority than this
		   task and can be preempted, force a rescheduling */
		NeedResched = TRUE; /* Set attention flag */
	}
}

#if LEGACY_CRIT_REGIONS==1
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::ReadyTaskLocked
//
// Make a task READY and insert in the READY queue
//
// Entered with INTS ENABLED!
//
////////////////////////////////////////////////////////////////////////////////////
void	uMT::ReadyTaskLocked(uTask *pTask)
{
	CpuStatusReg_t	CpuFlags = isr_Kn_IntLock();

	ReadyTask(pTask);

	isr_Kn_IntUnlock(CpuFlags);
}
#endif


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_RemoveFromAnyQueue
//
// Enter with INTS disabled
////////////////////////////////////////////////////////////////////////////////////
void  uMT::Tk_RemoveFromAnyQueue(uTask *pTask)
{

#if	uMT_USE_TIMERS==1
	// Remove any Timer related to this task
	TimerQ_CancelAll(pTask);
#endif

#if uMT_USE_SEMAPHORES==1
	if (pTask->TaskStatus == S_SBLOCKED)
	{
		// Remove from the Sem queue
		pTask->pSemq->SemQueue.Remove(pTask);
		pTask->pSemq = NULL;
	}
#endif

	if (pTask->TaskStatus == S_READY)
	{
		// Remove from the ready queue
		ReadyQueue.Remove(pTask);
	}
}


//////////////// EOF