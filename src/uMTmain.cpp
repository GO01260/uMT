////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTmain.cpp
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


uMT Kernel;

Bool_t	uMT::Inited = FALSE;

#ifndef WIN32
#include <stdlib.h>
#endif


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////



#if uMT_ALLOCATION_TYPE==uMT_FIXED_STATIC

///////////////////////////////////////////////////////////////////////////////////
// Allocate STACK for tasks
///////////////////////////////////////////////////////////////////////////////////

// Arduino main Loop() is NOT allocated in this area but we inherit the STACK defined by ARDUINO itself
static uint8_t Stacks[uMT_DEFAULT_TASK_NUM - 1][uMT_DEFAULT_STACK_SIZE];

// IDLE task stack is always allocated in a dedicated area so its size can be defined independently
// Because IDLE task is not calling malloc(), its stack can be allocated statically
static uint8_t IdleTaskStack[uMT_DEFAULT_IDLE_STACK_SIZE];


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupTaskStacks
//
////////////////////////////////////////////////////////////////////////////////////
void 	uMT::SetupTaskStacks()
{
	//
	// Setup Task List
	// Skip:
	//		ZERO: it is the IDLE task
	//		ONE: it is the Arduino MAIN loop()
	//
	for (int idx = uMT_MIN_FREE_TASK_LIST; idx < uMT_DEFAULT_TASK_NUM; idx++)
	{
		// Allocate STACK area for all tasks except IDLE & Arduino main loop()]
		TaskList[idx].StackBaseAddr = (StackPtr_t) &(Stacks[idx - uMT_MIN_FREE_TASK_LIST][0]);
		TaskList[idx].StackSize = uMT_DEFAULT_STACK_SIZE;
	}


	uTask *	pTask = &TaskList[uMT_ARDUINO_TASK_NUM];

	pTask->StackBaseAddr = (StackPtr_t)Kn_GetSPbase();				// Assign some sensible value...
	pTask->StackSize = (StackPtr_t)Kn_GetRAMend() - pTask->StackBaseAddr;	// Assign some sensible value...

	// Free RAM after stack allocation
	kernelCfg.FreeRAM_0 = (StackPtr_t)Kn_GetFreeRAM();

	// Setup Idle task
	IdleTaskPtr = &TaskList[uMT_IDLE_TASK_NUM];	// Pointing to the IDLE task
	IdleTaskPtr->StackBaseAddr = (StackPtr_t)&IdleTaskStack[0];
	IdleTaskPtr->StackSize = uMT_DEFAULT_IDLE_STACK_SIZE;

}

#endif

#if defined(ARDUINO_ARCH_SAM)
#define MALLOC_HDR	8		// bytes are used for MALLOC headers
#else
#define MALLOC_HDR	4		// bytes are used for MALLOC headers
#endif


#if uMT_ALLOCATION_TYPE==uMT_FIXED_DYNAMIC

// IDLE task stack is always allocated in a dedicated area so its size can be defined independently
// Because IDLE task is not calling malloc(), its stack can be allocated statically
static uint8_t IdleTaskStack[uMT_IDLE_STACK_SIZE];

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupTaskStacks
//
////////////////////////////////////////////////////////////////////////////////////
static void 	uMT::SetupTaskStacks()
{

	// Get the amount of RAM available from RAM-END (RAMEND - HeapPtr)
	// Kn_GetFreeRAMend() returns the free memory INCLUDING the STACK for the Arduiono loop() task
	 StackPtr_t RAMendFree = Kn_GetFreeRAMend();		// It can be called only at the beginning....

 	DgbStringPrint("uMT: SetupTaskStacks(): Arduino free RAM = ");
	DgbValuePrint((unsigned int)RAMendFree);	// Determine total memory for STACKS: ignore IDLE and Tid1
	StackPtr_t TotStackSize = ((kernelCfg.Tasks_Num - 2) * kernelCfg.AppTasks_Stack_Size);

	// Determine memory size to return to malloc() for application use
	StackPtr_t Mem2ReturnSize = RAMendFree - TotStackSize - (StackPtr_t)kernelCfg.Task1_Stack_Size - MALLOC_HDR;

	DgbStringPrint(" TotStackSize = ");
	DgbValuePrint((unsigned int)TotStackSize);

	DgbStringPrint(" Mem2ReturnSize = ");
	DgbValuePrintLN((unsigned int)Mem2ReturnSize);

	// Free RAM after stack allocation
	FreeRAM_0 = Mem2ReturnSize;

	// Malloc memory to return to malloc()
	uint8_t *Mem2ReturnPtr = (uint8_t *)uMTmalloc(Mem2ReturnSize);

	if (Mem2ReturnPtr == NULL)
	{
		isr_Kn_FatalError(F("Cannot allocate memory for STACKS/1"));
	}


	// Malloc memory for Stacks (do not allocate TASK 1 [Arduino loop()] stack)
	uint8_t *Mem4StacksPtr = (uint8_t *)uMTmalloc(TotStackSize);

	if (Mem4StacksPtr == NULL)
	{
		isr_Kn_FatalError(F("Cannot allocate memory for STACKS/2"));
	}


	// Free memory to return to malloc()
	uMTfree(Mem2ReturnPtr);

	//
	// Setup Task List
	// Skip:
	//		ZERO: it is the IDLE task
	//		ONE: it is the Arduino MAIN loop()
	//
	for (int idx = uMT_MIN_FREE_TASK_LIST; idx < kernelCfg.Tasks_Num; idx++)
	{
		// Allocate STACK area for all tasks except IDLE & Arduino main loop()]
		TaskList[idx].StackBaseAddr = (StackPtr_t) Mem4StacksPtr;
		TaskList[idx].StackSize = kernelCfg.AppTasks_Stack_Size;
		Mem4StacksPtr += kernelCfg.AppTasks_Stack_Size;

	}


	// Setup some sensible values for ARDUINO loop() task (Tid 1)
	uTask *	pTask = &TaskList[uMT_ARDUINO_TASK_NUM];
	pTask->StackBaseAddr = (StackPtr_t)Mem4StacksPtr;				// Assign some sensible value...
	pTask->StackSize = Kn_GetRAMend() - pTask->StackBaseAddr;	// Assign some sensible value...

	// Setup Idle task
	IdleTaskPtr = &TaskList[uMT_IDLE_TASK_NUM];	// Pointing to the IDLE task
	IdleTaskPtr->StackBaseAddr = (StackPtr_t)malloc(kernelCfg.Idle_Stack_Size);
	IdleTaskPtr->StackSize = kernelCfg.Idle_Stack_Size;


}
#endif


#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC

//extern "C" char *sbrk(int i);
extern char *__malloc_heap_end;
extern char *__malloc_heap_start;

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupMalloc
//
////////////////////////////////////////////////////////////////////////////////////
void 	uMT::SetupMallocLimits()
{
	// Get the amount of RAM available from RAM-END (RAMEND - HeapPtr)
	// Kn_GetFreeRAMend() returns the free memory INCLUDING the STACK for the Arduiono loop() task
	kernelCfg.FreeRAM_0 = Kn_GetFreeRAMend();		// It can be called only at the beginning....

 	DgbStringPrint("uMT: SetupTaskStacks(): Arduino initial free RAM = ");
	DgbValuePrint((unsigned int)FreeRAM_0);

	kernelCfg.FreeRAM_0 -= kernelCfg.Task1_Stack_Size;		// Remove TID 1 stack

	// Set HIGHMARK point (top of HEAP) for AVR malloc(), excluding TID1 stack size
	__malloc_heap_end = (char *)(Kn_GetRAMend() - kernelCfg.Task1_Stack_Size);

	// __malloc_heap_start already/will be setup in malloc() first call

}

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupTaskStacks
//
////////////////////////////////////////////////////////////////////////////////////
void 	uMT::SetupTaskStacks()
{
	// Setup some sensible values for ARDUINO loop() task (Tid 1)
	uTask *	pTask = &TaskList[uMT_ARDUINO_TASK_NUM];
	pTask->StackBaseAddr = (StackPtr_t)(__malloc_heap_end);
	pTask->StackSize = Kn_GetRAMend() - pTask->StackBaseAddr;

	// Setup Idle task
	IdleTaskPtr = &TaskList[uMT_IDLE_TASK_NUM];	// Pointing to the IDLE task
	IdleTaskPtr->StackBaseAddr = (StackPtr_t)uMTmalloc(kernelCfg.Idle_Stack_Size);
	IdleTaskPtr->StackSize = kernelCfg.Idle_Stack_Size;
}

#endif

extern void loop();
#define ARDUINO_LOOP loop

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::doStart
//
// On entry, kernelCfg already configured and Inited already tested
// In case of failure, no memory is freed (indeed a fatal error...)
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::doStart()
{
	////////////////////////////////////////////////
	//			Validate CONFIGURATION
	////////////////////////////////////////////////

	// At least uMT_MIN_TASK_NUM tasks...
	if (kernelCfg.Tasks_Num < uMT_MIN_TASK_NUM || kernelCfg.Tasks_Num > uMT_MAX_TASK_NUM)
		return(E_INVALID_MAX_TASK_NUM);

#if uMT_USE_TIMERS==1
	if (kernelCfg.AgentTimers_Num < uMT_MIN_TIMER_AGENT_NUM || 
		kernelCfg.AgentTimers_Num > uMT_MAX_TIMER_AGENT_NUM)
		return(E_INVALID_MAX_TIMER_NUM);
#endif

#if uMT_USE_SEMAPHORES==1
	if (kernelCfg.Semaphores_Num < uMT_MIN_SEM_NUM ||
		kernelCfg.Semaphores_Num > uMT_MAX_SEM_NUM)
		return(E_INVALID_MAX_SEM_NUM);
#endif



#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC

	////////////////////////////////////////////////
	//			Alloca TASK and STACK
	////////////////////////////////////////////////

	// Setup MALLOC()
	SetupMallocLimits();


	// Allocate space for TASKS
	TaskList = new uTask[kernelCfg.Tasks_Num];

	if (TaskList == NULL)
		return(E_NO_MORE_MEMORY);

#endif



	// Init members
	Running = (uTask *)NULL;
	
	ReadyQueue.Init();
	ActiveTaskNo = 1;			// Arduino main loop()

#if LEGACY_CRIT_REGIONS==1
	NoResched = 0;
#endif

	NeedResched = FALSE;
	NoPreempt = FALSE;


#if uMT_USE_TIMERS==1

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC
	// Allocate space for Semaphore
	TimerAgentList = new uTimer[kernelCfg.AgentTimers_Num];

	if (TimerAgentList == NULL)
		return(E_NO_MORE_MEMORY);
#endif

	AlarmExpired = FALSE;
	TimerQueue = NULL;
	TotTimerQueued = 0;

	//
	// Setup AGENT TIMER List
	//
	for (int idx = 0; idx < kernelCfg.AgentTimers_Num; idx++)
	{
		uTimer *pTimer = &TimerAgentList[idx];

		pTimer->Init(kernelCfg.Tasks_Num + idx, uMT_TM_IAM_AGENT, (uTask *)NULL);

		pTimer->Next = (idx == uMT_MAX_TIMER_AGENT_NUM - 1) ? NULL : &TimerAgentList[idx + 1];
	}

	FreeTimerQueue = &TimerAgentList[0];

#endif

	TickCounter = 0;			// Kernel tick counter


	// Init Task List (all tasks)
	for (int idx = 0; idx < kernelCfg.Tasks_Num; idx++)
	{
		TaskList[idx].Init(idx);
	}

	// Setup Task List
	SetupTaskStacks();

	//
	// Setup UNUSED Task List
	// Skip:
	//		ZERO: it is the IDLE task
	//		ONE: it is the Arduino MAIN loop()
	//
	for (int idx = uMT_MIN_FREE_TASK_LIST; idx < kernelCfg.Tasks_Num; idx++)
	{
		TaskList[idx].Next = (idx == kernelCfg.Tasks_Num - 1 ? NULL : &TaskList[idx + 1]);
	}

	// Build up the UNUSED task list
	UnusedQueue = &TaskList[uMT_MIN_FREE_TASK_LIST];

	// Setup Idle task
	IdleTaskPtr->TaskStatus = S_READY;		// Always ready!
	IdleTaskPtr->Priority = PRIO_LOWEST;
	IdleTaskPtr->SavedSP = NewTask(IdleTaskPtr->StackBaseAddr, IdleTaskPtr->StackSize, IdleLoop, BadExit);
#if uMT_USE_RESTARTTASK==1
	// Store start address
	IdleTaskPtr->StartAddress = IdleLoop;
	IdleTaskPtr->BadExit = BadExit;
#endif	

	// Setup task "Arduino"
	DgbStringPrint("uMT: Kn_Start(): Arduino SP = ");
	DgbValuePrintLN((unsigned int)GetSP());

	//
	// There must be ALWAYS a RUNNING task...
	//
	Running = &TaskList[uMT_ARDUINO_TASK_NUM];
	Running->SavedSP = Kn_GetSP();
	Running->TaskStatus = S_RUNNING;
	Running->Priority = PRIO_NORMAL;
	TimeSlice = uMT_TICKS_TIMESHARING; // Load
#if uMT_USE_RESTARTTASK==1
	// Store start address
	Running->StartAddress = ARDUINO_LOOP;
	Running->BadExit = BadExit;
#endif	

#if uMT_USE_SEMAPHORES==1

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC
	// Allocate space for Semaphore
	SemList = new uMTsem[kernelCfg.Semaphores_Num];

	if (SemList == NULL)
		return(E_NO_MORE_MEMORY);

#endif

	// Init Semaphore List
	for (int idx = 0; idx < kernelCfg.Semaphores_Num; idx++)
	{
		SemList[idx].Init();	// Init
	}

	SemList[CLIB_SEM].SemValue = 1;		// CLIB semaphore is initialized to FREE...

#endif

	// Now KERNEL inited....
	Inited = TRUE;

	// Not in Kernel mode
	KernelStackMode = FALSE;

	// Setup SYSTEM TICK
	SetupSysTicks();

	return(E_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Kn_GetConfiguration
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Kn_GetConfiguration(uMTcfg &Cfg)
{
	Cfg.ro.Init();
	Cfg.rw.Init();
	
	// Free RAM BEFORE stack allocation
	Cfg.rw.FreeRAM_0 = (StackPtr_t)Kn_GetFreeRAM();

	return(E_SUCCESS);
}


//////////////////////////// EOF