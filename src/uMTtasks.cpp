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
	Run = 0;

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
Errno_t	uMT::Tk_CreateTask(FuncAddress_t StartAddress, TaskId_t &Tid, FuncAddress_t _BadExit, StackSize_t _StackSize)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	if (_StackSize == 0)
		_StackSize = kernelCfg.AppTasks_Stack_Size;	// Use default

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC

	if (_StackSize < uMT_MIN_STACK_SIZE)
	{
		DgbStringPrintLN("uMT: Tk_CreateTask(): E_INVALID_STACK_SIZE");

		return(E_INVALID_STACK_SIZE);
	}

#endif

	if (_BadExit == NULL)
		_BadExit = (FuncAddress_t)BadExit;

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	// Search for a empty task slot
	if (UnusedQueue == NULL)
	{
		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

		DgbStringPrintLN("uMT: Tk_CreateTask(): E_NOMORE_TASKS");

		return(E_NOMORE_TASKS);
	}

	// Pickup the first one free...
	uTask *pTask = UnusedQueue;

	// Clean up task
	pTask->CleanUp();

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC

	// Allocate task's STACK memory
	pTask->StackBaseAddr = (StackPtr_t)malloc(_StackSize);

	if (pTask->StackBaseAddr == NULL)
	{
		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

		DgbStringPrintLN("uMT: Tk_CreateTask(): E_NO_MORE_MEMORY");

		return(E_NO_MORE_MEMORY);
	}

	pTask->StackSize = _StackSize;

#endif

	SetupStackGuard(pTask);			// Store stack guard mark

	UnusedQueue = UnusedQueue->Next;

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

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	CHECK_TASK_MAGIC(pTask, "Tk_CreateTask");

	return(E_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::doDeleteTask
//
////////////////////////////////////////////////////////////////////////////////////
void	uMT::doDeleteTask(uTask *pTask)
{
	// Insert in the UNUSED queue
	pTask->TaskStatus = S_UNUSED;
	pTask->Next = UnusedQueue;
	UnusedQueue = pTask;

	ActiveTaskNo--;		// one less...

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC
	// Release STACK memory
	uMTfree((void *)pTask->StackBaseAddr);
#endif
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

	if (Tid == uMT_ARDUINO_TASK_NUM)		// Arduino loop() cannot be deleted....
		return(E_INVALID_TASKID);


	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus == S_UNUSED)
		return(E_INVALID_TASKID);

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	////////////////////////////////////////////
	// Remove from any QUEUE
	////////////////////////////////////////////
	Tk_RemoveFromAnyQueue(pTask);

	if (Running == pTask)
	{
		// Suicide...

		pTask->TaskStatus = S_ZOMBIE;	// It will be deleted by Resched()

		isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

		Suspend();

		// The following ONLY for the SAM architecture, just in case...
		while (1)
			;
	}
	else
	{
		doDeleteTask(pTask);
	}

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

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


	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	/////////////////////////////////////////
	// Insert in the ready queue
	/////////////////////////////////////////
	ReadyTask(pTask);

	/* ... and check for preemption */
	Check4Preemption();

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	//
	// Starting a task shall NOT trigger a RoundRobin unless the Started task has got higher priority.
	// This will allow the calling task to complete all the initialization,
	// including "Tk_StartTask" other tasks
	//
	Check4NeedReschedule();		// Call Suspend() if NeedResched==TRUE

	return(E_SUCCESS);
}

#if uMT_USE_RESTARTTASK==1

///////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Reborn
//
// Entered with INTS disabled
// Entered with PRIVATE Stack
////////////////////////////////////////////////////////////////////////////////////
void uMT::Reborn()
{
	uTask *pTask = Running;

	// Sanity Ceck.... There is ALWAYS a RUNNING task
	CHECK_TASK_MAGIC(Running, "Reborn(entry)");

	// We cannot create a "fresh" stack before switching to a private Kernel stack...
	pTask->SavedSP = NewTask(pTask->StackBaseAddr, pTask->StackSize, pTask->StartAddress, pTask->BadExit);

	/////////////////////////////////////////
	// Insert in the ready queue
	/////////////////////////////////////////
	ReadyTask(pTask);

#ifndef WIN32

//	Serial.println(F("Reborn()"));
//	Serial.flush();

#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD) 
	// Call Suspend() + Reschedule()
	Suspend();

	while (1)
		;
	// It never returns!!!

#elif defined(ARDUINO_ARCH_AVR)

	NoPreempt = TRUE;		// Prevent further rescheduling.... until next 

	Reschedule();

	// It never returns!!!

#else
#error "Unsupported architecture!"
#endif

#endif		// WIN32

}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_ReStartTask
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::doReStartTask(uTask *pTask)
{
	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */


	////////////////////////////////////////////
	// Remove from any QUEUE
	////////////////////////////////////////////
	Tk_RemoveFromAnyQueue(pTask);


	///////////////////////////////////////////////
	// Clean up task
	///////////////////////////////////////////////
	pTask->CleanUp();

	///////////////////////////////////////////////
	// Setup basic data again..
	///////////////////////////////////////////////
	pTask->TaskStatus = S_CREATED;
//	pTask->Priority = PRIO_NORMAL;			// Priority is NOT reset.
		
	if (Running == pTask)
	{
		// Suicide...  
			
		NewStackReborn();

		// It never returns!!!
	}

	pTask->SavedSP = NewTask(pTask->StackBaseAddr, pTask->StackSize, pTask->StartAddress, pTask->BadExit);

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	// Start another task (not the Running)
	return(Tk_StartTask(pTask->myTid));		// Do a StartTask

}


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

	if (Tid == uMT_ARDUINO_TASK_NUM)		// Cannot restart Arduino, missing StartAddress...
		return(E_NOT_ALLOWED);
	
	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus == S_UNUSED)
		return(E_TASK_NOT_STARTED);

	if (pTask->TaskStatus == S_CREATED)
		return(Tk_StartTask(Tid));		// Do simply a StartTask...

	return(doReStartTask(pTask));
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

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

	uTask *pTask = &TaskList[Tid];

	if (pTask->TaskStatus != S_CREATED)
	{
		return(E_ALREADY_STARTED);
	}

	pTask->Parameter = _parameter;

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

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

	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();	/* Enter critical region */

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
	Check4Preemption();

	isr_Kn_IntUnlock(CpuFlags);	/* End of critical region */

	Check4NeedReschedule();		// Call Suspend() if NeedResched==TRUE

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

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::MaxUsedStack
//
////////////////////////////////////////////////////////////////////////////////////
StackSize_t	uMT::MaxUsedStack(uTask *pTask)
{
	if (pTask->TaskStatus == S_UNUSED)
		return(0);

	int limit = pTask->StackSize / sizeof(StackGuard_t);
	int idx;

	StackGuard_t *StackPtr = (StackGuard_t *)pTask->StackBaseAddr;

	for (idx = 0; idx < limit; idx++)
	{
		if (StackPtr[idx] != uMT_STACK_GUARD_MARK)
			break;
	}

	return(pTask->StackSize - (idx * sizeof(StackGuard_t)));
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::doGetTaskInfo
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::doGetTaskInfo(uTask *pTask, uMTtaskInfo &Info)
{
	Info.Tid = pTask->myTid;
	Info.Priority = pTask->Priority;
	Info.StackSize = pTask->StackSize;

	Info.TaskStatus = pTask->TaskStatus;
	Info.Run = pTask->Run;


	if (pTask == Running)
	{
		Info.FreeStack = Kn_GetSP() - Running->StackBaseAddr;
	}
	else
	{
		Info.FreeStack = pTask->SavedSP - pTask->StackBaseAddr;
	}

	Info.MaxUsedStack = MaxUsedStack(pTask);
	
	return(E_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Tk_GetTaskInfo
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_GetTaskInfo(TaskId_t Tid, uMTtaskInfo &Info)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	CHECK_VALID_TASK(Tid);

	return(doGetTaskInfo(&TaskList[Tid], Info));
}

///////////////////// EOF