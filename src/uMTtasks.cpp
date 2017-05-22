////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtask.cpp
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
//				CLASS uTask
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
//
//	uTask::CleanUp()
//
////////////////////////////////////////////////////////////////////////////////////
void	uTask::CleanUp()
{
#if uMT_USE_EVENTS==1
	EV_received = uMT_NULL_EVENT;
	EV_requested = uMT_NULL_EVENT;
	EV_condition = uMT_NULL_OPT;
#endif
	
}



///////////////////////////////////////////////////////////////////////////////////
//
//	uTask::Init()
//
////////////////////////////////////////////////////////////////////////////////////
void	uTask::Init(TaskId_t _myTid)
{
#if uMT_SAFERUN==1
	magic = uMT_TASK_MAGIC;			// To check consistency
#endif
	SavedSP = (StackPtr_t)NULL;
	StackBaseAddr = (StackPtr_t)NULL;
	TaskStatus = S_UNUSED;
	StackSize = 0;

//	Prev = (uTask *)NULL;
	Next = (uTask *)NULL;

	myTid = _myTid;

	CleanUp();

#if uMT_USE_RESTARTTASK==1
	StartAddress = (FuncAddress_t)NULL;
#endif


#if	uMT_USE_TIMERS==1
	TaskTimer.Init(myTid, uMT_TM_IAM_TASK, this);	// Pointer to this task
#endif
}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_CreateTask
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_CreateTask(FuncAddress_t StartAddress, TaskId_t &Tid, FuncAddress_t _BadExit)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (_BadExit == NULL)
		_BadExit = (FuncAddress_t)BadExit;

	CpuStatusReg_t	CpuFlags = isrKn_IntLock();

	// Search for a empty task slot
	if (UnusedQueue == NULL)
	{
		isrKn_IntUnlock(CpuFlags);

		DgbStringPrintLN("uMT: Tk_CreateTask(): E_NOMORE_TASKS");

		return(E_NOMORE_TASKS);
	}

	// Pickup the first one free...
	uTask *pTask = UnusedQueue;
	UnusedQueue = UnusedQueue->Next;

	// Clean up task
	pTask->CleanUp();

#if uMT_USE_RESTARTTASK==1
	// Store start address
	pTask->StartAddress = StartAddress;
	pTask->BadExit = _BadExit;
#endif

	pTask->TaskStatus = S_CREATED;
	pTask->Priority = PRIO_NORMAL;


	pTask->SavedSP = NewTask(pTask->StackBaseAddr, pTask->StackSize, StartAddress, _BadExit);


	Tid = pTask->myTid;

	ActiveTaskNo++;		// one more...

	isrKn_IntUnlock(CpuFlags);

	CHECK_TASK_MAGIC(pTask, "Tk_CreateTask");

	return(E_SUCCESS);
}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_DeleteTask
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_DeleteTask(TaskId_t Tid)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus == S_UNUSED)
		return(E_INVALID_TASKID);

	CpuStatusReg_t	CpuFlags = isrKn_IntLock();

	ActiveTaskNo--;		// one less...

	////////////////////////////////////////////
	// Remove from any QUEUE
	////////////////////////////////////////////
	Tk_RemoveFromAnyQueue(pTask);


	// Insert in the UNUSED queue
	pTask->TaskStatus = S_UNUSED;
	pTask->Next = UnusedQueue;
	UnusedQueue = pTask;

	if (Running == pTask)
	{
		// Suicide...

		// With INTS disabled...
		Reschedule();

		// It never returns!!!
	}

	isrKn_IntUnlock(CpuFlags);

	return(E_SUCCESS);

}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_StartTask
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_StartTask(TaskId_t Tid)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus != S_CREATED)
	{
		DgbStringPrint("uMT: Tk_StartTask(): E_ALREADY_STARTED => ");
		DgbValuePrintLN(Tid);
		return(E_ALREADY_STARTED);
	}


	CpuStatusReg_t	CpuFlags = isrKn_IntLock();

	/////////////////////////////////////////
	// Insert in the ready queue
	/////////////////////////////////////////
	ReadyTask(pTask);

	/* ... and check for preemption */
	Check4Preemption(NoResched > 0 ? FALSE : TRUE);

	//
	// Starting a task shall NOT trigger a RoundRobin unless the Started task has got hogher priority.
	// This will allow the calling task to complete all the initialization,
	// including "Tk_StartTask" other tasks
	//

	isrKn_IntUnlock(CpuFlags);

	return(E_SUCCESS);
}

#if uMT_USE_RESTARTTASK==1
////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_ReStartTask
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_ReStartTask(TaskId_t Tid)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus == S_UNUSED)
		return(E_TASK_NOT_STARTED);

	if (pTask->TaskStatus == S_CREATED)
		return(Tk_StartTask(Tid));		// Do simply a StartTask...

	if (Running == pTask)
	{
		// Suicide...  we cannot do it, we would need to set up a KERNEL private stack and this cannot be done at the moment...
		return(E_NOT_ALLOWED);
	}

	CpuStatusReg_t	CpuFlags = isrKn_IntLock();

	////////////////////////////////////////////
	// Remove from any QUEUE
	////////////////////////////////////////////
	Tk_RemoveFromAnyQueue(pTask);


	// Clean up task
	pTask->CleanUp();

	///////////////////////////////////////////////
	// Setup basic data again..
	///////////////////////////////////////////////
	pTask->TaskStatus = S_CREATED;
//	pTask->Priority = PRIO_NORMAL;			// Priority is NOT reset.
		
	pTask->SavedSP = NewTask(pTask->StackBaseAddr, pTask->StackSize, pTask->StartAddress, pTask->BadExit);
	
#ifdef ZAPPED		// Cannot do it without a KERNEL private STACK
	if (Running == pTask)
	{
		// Suicide...

		// With INTS disabled...
		Tk_StartTask(pTask->myTid);

		Reschedule();

		// It never returns!!!
	}
#endif



	// Perform a StartTask
	isrKn_IntUnlock(CpuFlags);

	// Start another task (not the Running)
	return(Tk_StartTask(pTask->myTid));		// Do a StartTask

}
#endif


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_SetParam
//
// Parameter can only be set when TASK is in S_CREATED state
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_SetParam(TaskId_t Tid, Param_t _parameter)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus != S_CREATED)
	{
		return(E_ALREADY_STARTED);
	}

	pTask->Parameter = _parameter;

	return(E_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_Yield
//
// It can only be called from Suspend()
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_Yield()
{
	DgbStringPrintLN("uMT: Tk_Yield(): => Suspend()");

	// Suspend task and generate a rescheduling: it will "return" only when this task is S_RUNNING again 
	// Reschedule() will RoundRobin the task
	Suspend();

	return(E_SUCCESS);
}




////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_GetMyTid
//
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_GetMyTid(TaskId_t &Tid)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	Tid = Running->myTid;

	return(E_SUCCESS);
}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_SetPriority
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_SetPriority(TaskId_t Tid, TaskPrio_t npriority, TaskPrio_t &ppriority)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];


	ppriority = pTask->Priority;

	if (npriority == 0)	/* Return current priority */
		return(E_SUCCESS);

	CpuStatusReg_t	CpuFlags = isrKn_IntLock();

	if (pTask == Running) 	/* Running task */
	{
		Running->Priority = npriority;
	} 
	else	/* Any other task */
	{
		pTask->Priority = npriority;

		/* Task blocked in some queue */

		
#if uMT_USE_SEMAPHORES==1
		if (pTask->TaskStatus == S_SBLOCKED)		// Semaphore queue
		{
			/* In a priorized queue: remove and insert again */
			pTask->pSemq->SemQueue.Remove(pTask);
			pTask->pSemq->SemQueue.Insert(pTask);	
		}
		else 
#endif
		{
			/* Task in the ready list */
			if (pTask->TaskStatus == S_READY)
			{
				ReadyQueue.Remove(pTask);

				/* Make this task READY again ... */
				ReadyTask(pTask);
			}
		}

	}

	/* ... and check for preemption */
	Check4Preemption(TRUE);

	isrKn_IntUnlock(CpuFlags);

	return(E_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_GetPriority
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_GetPriority(TaskId_t Tid, TaskPrio_t &ppriority)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	uTask *pTask = &TaskList[Tid];

	ppriority = pTask->Priority;

	return(E_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_GetPriority
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_GetPriority(TaskPrio_t &ppriority)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	ppriority = Running->Priority;

	return(E_SUCCESS);
}


///////////////////// EOF