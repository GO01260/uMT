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




///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////
//					STACK allocation
/////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino malloc() is typically using free RAM between end of (initialized + unitialized) DATA 
// and the Arduino loop() Stack Pointer.
// If uMT tasks' stacks are allocated statically, if a uMT task (except loop()) is allocating
// malloc() memory, this could fail.
// For this reason the uMT_STATIC_STACKS configuration parameter is used to control where
// to allocate stacks.
// IDLE task stack, however, is always allocated statically because IDLE does not
// call any malloc()
/////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t IdleTaskStack[uMT_IDLE_STACK_SIZE];

#if uMT_STATIC_STACKS==1
///////////////////////////////////////////////////////////////////////////////////
// Allocate STACK for tasks
// Arduino main Loop() NOT in this area but we inherit the STACK defined by ARDUINO itself
///////////////////////////////////////////////////////////////////////////////////
static uint8_t Stacks[uMT_MAX_TASK_NUM - 1][uMT_STACK_SIZE];



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
	for (int idx = uMT_MIN_FREE_TASK_LIST; idx < uMT_MAX_TASK_NUM; idx++)
	{
		// Allocate STACK area for all tasks except IDLE & Arduino main loop()]
		TaskList[idx].StackBaseAddr = (StackPtr_t) &(Stacks[idx - uMT_MIN_FREE_TASK_LIST][0]);
		TaskList[idx].StackSize = uMT_STACK_SIZE;

		TaskList[idx].Next = (idx == uMT_MAX_TASK_NUM - 1 ? NULL : &TaskList[idx + 1]);
	}
}

#else


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupTaskStacks
//
////////////////////////////////////////////////////////////////////////////////////
void 	uMT::SetupTaskStacks()
{
	
	// Determine total memory for STACKS
	SRAMsize_t TotStackSize = (uMT_MAX_TASK_NUM * uMT_STACK_SIZE);

	// Determine memory size to return to malloc()
	SRAMsize_t Mem2ReturnSize = Kn_GetFreeSRAM() - TotStackSize - (SRAMsize_t)uMT_TID1_STACK_SIZE;

	DgbStringPrint("uMT: SetupTaskStacks(): Arduino free RAM = ");
	DgbValuePrint((unsigned int)Kn_GetFreeSRAM());

	DgbStringPrint(" TotStackSize = ");
	DgbValuePrint((unsigned int)TotStackSize);

	DgbStringPrint(" Mem2ReturnSize = ");
	DgbValuePrintLN((unsigned int)Mem2ReturnSize);


	// Malloc memory to return to malloc()
	uint8_t *Mem2ReturnPtr = malloc(Mem2ReturnSize);

	if (Mem2ReturnPtr == NULL)
	{
		iKn_FatalError(F("Cannot allocate memory for STACKS/1"));
	}


	// Malloc memory for Stacks (do not allocate TASK 1 [Arduino loop()] stack)
	uint8_t *Mem4StacksPtr = malloc(TotStackSize);

	if (Mem4StacksPtr == NULL)
	{
		iKn_FatalError(F("Cannot allocate memory for STACKS/2"));
	}


	// Free memory to return to malloc()
	free(Mem2ReturnPtr);


	//
	// Setup Task List
	// Skip:
	//		ZERO: it is the IDLE task
	//		ONE: it is the Arduino MAIN loop()
	//
	for (int idx = uMT_MIN_FREE_TASK_LIST; idx < uMT_MAX_TASK_NUM; idx++)
	{
		// Allocate STACK area for all tasks except IDLE & Arduino main loop()]
		TaskList[idx].StackBaseAddr = (StackPtr_t) Mem4StacksPtr;
		TaskList[idx].StackSize = uMT_STACK_SIZE;
		Mem4StacksPtr += uMT_STACK_SIZE;

		TaskList[idx].Next = (idx == uMT_MAX_TASK_NUM - 1 ? NULL : &TaskList[idx + 1]);
	}
}


#endif


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Kn_Start
//
////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Kn_Start(Bool_t _TimeSharingEnabled, Bool_t _BlinkingLED)
{
	if (Inited)
		return(E_ALREADY_INITED);

	// At least uMT_MIN_TASK_NUM tasks...
	if (uMT_MAX_TASK_NUM < uMT_MIN_TASK_NUM)
	{
		DgbStringPrintLN("uMT: Kn_Start(): E_INVALID_TOTAL_TASK_NUM");
		return(E_INVALID_TOTAL_TASK_NUM);
	}

	// Init members
	Inited = TRUE;
	Running = (uTask *)NULL;

	ReadyQueue.Init();


	ActiveTaskNo = 1;			// Arduino main loop()

	NoResched = 0;
	NeedResched = FALSE;

	NoPreempt = FALSE;
	TimeSharingEnabled = _TimeSharingEnabled;
	BlinkingLED = _BlinkingLED;

#if uMT_USE_TIMERS==1

#if uMT_MAX_TIMER_AGENT_NUM<uMT_MAX_TASK_NUM
#error uMT_MAX_TIMER_AGENT_NUM < uMT_MAX_TASK_NUM!
#endif

	AlarmExpired = FALSE;
	TimerQueue = NULL;
	TotTimerQueued = 0;

	//
	// Setup AGENT TIMER List
	//
	for (int idx = 0; idx < uMT_MAX_TIMER_AGENT_NUM; idx++)
	{
		uTimer *pTimer = &TimerAgentList[idx];

		pTimer->Init(uMT_MAX_TASK_NUM + idx, uMT_TM_IAM_AGENT, (uTask *)NULL);

		pTimer->Next = (idx == uMT_MAX_TIMER_AGENT_NUM - 1) ? NULL : &TimerAgentList[idx + 1];
	}

	FreeTimerQueue = &TimerAgentList[0];

#endif

	TickCounter = 0;			// Kernel tick counter


	// Init Task List (all tasks)
	for (int idx = 0; idx < uMT_MAX_TASK_NUM; idx++)
	{
		TaskList[idx].Init(idx);
	}

	// Setup Task List
	SetupTaskStacks();


	// Build up the UNUSED task list
	UnusedQueue = &TaskList[uMT_MIN_FREE_TASK_LIST];


	// Setup Idle task
	IdleTaskPtr = &TaskList[uMT_IDLE_TASK_NUM];	// Pointing to the IDLE task
	IdleTaskPtr->TaskStatus = S_READY;		// Always ready!
	IdleTaskPtr->StackBaseAddr = &IdleTaskStack[0];
	IdleTaskPtr->StackSize = uMT_IDLE_STACK_SIZE;
	IdleTaskPtr->Priority = PRIO_LOWEST;

	IdleTaskPtr->SavedSP = NewTask(IdleTaskPtr->StackBaseAddr, IdleTaskPtr->StackSize, IdleLoop, BadExit);
	

	// Setup task "Arduino"
	DgbStringPrint("uMT: Kn_Start(): Arduino SP = ");
	DgbValuePrintLN((unsigned int)GetSP());

	//
	// There must be ALWAYS a RUNNING task...
	//
	Running = &TaskList[uMT_ARDUINO_TASK_NUM];
	Running->SavedSP = GetSP();

#if defined(ARDUINO_ARCH_AVR) // ARDUINO MEGA or UNO
	Running->StackBaseAddr = Kn_GetSPbase();								// Assign some sensible value...
	Running->StackSize = Running->SavedSP - Running->StackBaseAddr + 16;	// Assign some sensible value...
#endif



	Running->TaskStatus = S_RUNNING;
	Running->Priority = PRIO_NORMAL;
	TimeSlice = uMT_TICKS_TIMESHARING; // Load


#if uMT_USE_SEMAPHORES==1
	// Init Semaphore List
	for (int idx = 0; idx < uMT_MAX_SEM_NUM; idx++)
	{
		SemList[idx].Init();	// Init
	}
#endif

	// Setup SYSTEM TICK
	SetupSysTicks();

	return(E_SUCCESS);
}



//////////////////////////// EOF