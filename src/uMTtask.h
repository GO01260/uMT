////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtask.h
//	AUTHOR: Antonio Pastore - March 2017
//	Program originally written by Antonio Pastore, Torino, ITALY.
//	UPDATED: 6 May 2017
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

#ifndef uMT_TASK_H
#define uMT_TASK_H


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT TASK priorities
//
////////////////////////////////////////////////////////////////////////////////////

#define PRIO_LOWEST			0x00		// 0
#define PRIO_LOW			0x04		// 4
#define PRIO_NORMAL			0x08		// 8
#define PRIO_HIGH			0x0C		// 12
#define PRIO_HIGHEST		0x0F		// 15
#define PRIO_MAXPRIO_MASK	0x0F		// Only 0-15

typedef uint8_t			TaskPrio_t;			// 8 bits, task priority 0-16



////////////////////////////////////////////////////////////////////////////////////
//
//	INTERNAL configuration
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_IDLE_TASK_NUM		0		// IDLE task number
#define uMT_ARDUINO_TASK_NUM	1		// Arduino loop() task number
#define uMT_MIN_FREE_TASK_LIST	2		// Free task list starts from....

#define uMT_TASK_MAGIC			0xDEAD	// Magic value...

//typedef uint8_t		TaskId_t;		// 8 bits
typedef uMTobject_id	TaskId_t;		// TASK ID


///////////////////////////////////////////////////////////////////////////////////
//
//					STACK GUARD 
//
// To allow a run-time verification of the used stack area, uMT fills the stack 
// with uMT_STACK_GUARD_MARK value [in SetupStackGuard()]
// In Tk_GetTaskInfo(), the stack area is scanned to find which part has been modified
////////////////////////////////////////////////////////////////////////////////////////

#define uMT_STACK_GUARD_MARK	0xADDE	// 2 bytes
#define uMT_STACK_GUARD_LIMIT	32		// Do not set uMT_STACK_GUARD_MAGIC in the last ... bytes - it must be 4 bytes aligned
typedef uint16_t StackGuard_t;			// This also set uMT_STACK_GUARD_MARK size



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT Task's STATUS
//
////////////////////////////////////////////////////////////////////////////////////

enum Status_t
{
/* 0 */	S_UNUSED,		// Slot is unused 
/* 1 */	S_CREATED,		// Task has been just created, NOT in any queue; Next status is READY!
/* 2 */	S_READY,		// Task is ready to run
/* 3 */	S_RUNNING,		// Task is the running task
/* 4 */	S_SBLOCKED,		// Task is blocked in then SEM queue
/* 5 */	S_EBLOCKED,		// Task is blocked but NOT in any queue
/* 6 */	S_TBLOCKED,		// Task is blocked in the TIMER queue
/* 7 */	S_SUSPENDED,	// Task is suspended, NOT in any queue; Next status is READY!
/* 8 */	S_ZOMBIE		// After entering in Tk_delete()!!!
};

//	S_IDLETASK,		// This is the IDLE task (it is managed in a special way)
//	S_DORMANT,		// Task is dormant

class uMTsem;		// Forward declaration

class uTask
{
	/////////////////////////////////
	// FRIENDS!!!
	/////////////////////////////////
	friend class uMT;
	friend class uTimer;
	friend	class uMTtaskQueue;

	friend void KLL_TaskLoop();			// Test0_KernelLowLevel.cpp
	friend void KLL_MainLoop();			// Test0_KernelLowLevel.cpp

	friend unsigned uMTdoTicksWork();	// uMTarduinoCommon.cpp

	friend void uMT_SystemTicks();		// uMT_AVR_SysTick.cpp
	friend void pendSVHook();			// uMT_SAM_SysTick.cpp

private:

#if uMT_SAFERUN==1
	uint16_t	magic;			// To check consistency
#endif

	TaskPrio_t	Priority;		// Task priority

	StackPtr_t	SavedSP;		// Saved STACK pointer
	StackPtr_t	StackBaseAddr;	// Pointer to the STACK memory area (down in Arduino UNO)
	StackSize_t	StackSize;		// Stack's size
	Status_t	TaskStatus;		// Task's status

	uTask	*Next;				// Next in the queue
//	uTask	*Prev;				// Prev in the queue

	TaskId_t	myTid;			// My TID, helper

#if	uMT_USE_EVENTS==1
	Event_t			EV_received;	// Bit mask of received events
	Event_t			EV_requested;	// Bit mask of requested events
	uMToptions_t	EV_condition;	// OR/AND event condition
#endif

#if	uMT_USE_TASK_STATISTICS>=1
	RunValue_t		Run;		// How many run
#endif

#if	uMT_USE_TASK_STATISTICS>=2
	uMTextendedTime	usRunningTime;	// Total Running Time in uSeconds
	Timer_t			usLastRun;		// uSeconds of the last run
#endif

	Param_t			Parameter;	// Here can be stored specifc task's parameter for Tk_Start()

#if	uMT_USE_TIMERS==1
	// Timer used for blocked tasks
	// This is used for Event/Semaphore/WakeupAfter timeout management
	uTimer		TaskTimer;		
#endif


#if uMT_USE_SEMAPHORES==1
	//
	// Pointer to the Semaphore Queue in which this task is queued or NULL if the task OWNS the semaphore.
	uMTsem	*pSemq;		
#endif

#if uMT_USE_RESTARTTASK==1
	FuncAddress_t	StartAddress;	// Task's starting address, used by restart()
	FuncAddress_t	BadExit;		// BadExit address, used by restart()

#endif



public:
	void Init(unsigned int myIndex);
	void CleanUp();

static 	const __FlashStringHelper *TaskStatus2String(Status_t TaskStatus);
const __FlashStringHelper *TaskStatus2String() { return(TaskStatus2String(TaskStatus));};

};


////////////////////////////////////////////////////
// Returned in Tk_GetTaskInfo()
////////////////////////////////////////////////////

class uMTtaskInfo
{
public:
	TaskId_t		Tid;			// Task ID
	TaskPrio_t		Priority;		// Task priority
	Status_t		TaskStatus;		// Task's status
#if	uMT_USE_TASK_STATISTICS>=1
	RunValue_t		Run;		// How many run
#endif

#if	uMT_USE_TASK_STATISTICS>=2
	uMTextendedTime	usRunningTime;	// Elapsed time in RUNNING mode
#endif

	StackSize_t		StackSize;		// Stack's size in bytes
	StackSize_t		FreeStack;		// Free stack size in bytes
	StackSize_t		MaxUsedStack;	// Maximum used stack in bytes

};

#endif



/////////////////////////////////////// EOF

