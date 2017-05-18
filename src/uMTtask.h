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

///////////////////////////////////////////////////////////////////////////////
// This macro check for VALID TASK ID and returns E_INVALID_TASKID if not valid
// TASK 0 is the IDLE task
///////////////////////////////////////////////////////////////////////////////
#define CHECK_VALID_TASK(Tid) { if (Tid == 0 || Tid >= uMT_MAX_TASK_NUM) return(E_INVALID_TASKID); }



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
#define uMT_MIN_TASK_NUM		4		// MIN task number (cannot be less than 4!!!)
#define uMT_IDLE_TASK_NUM		0		// IDLE task number
#define uMT_ARDUINO_TASK_NUM	1		// Arduino loop() task number
#define uMT_MIN_FREE_TASK_LIST	2		// Free task list starts from....

#define uMT_TASK_MAGIC			0xDEAD	// Magic value...


typedef	uint8_t			*StackPtr_t;		// Stack alignement
typedef uint16_t		StackSize_t;		// 16 bits, system dependent. In MEGA the number of bits in an address is 22 but 16 seems to be sufficient
typedef uint8_t			CpuStatusReg_t;		// 8 bits - Lock/Unlock
typedef uint8_t			TaskId_t;			// 8 bits



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
/* 7 */	S_SUSPENDED		// Task is suspended, NOT in any queue; Next status is READY!
};

// UNUSED
//	S_ZOMBIE,		// After entering in BadExit()!!!
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
	friend unsigned uMTdoTicksWork();	// uMTarduinoSysTick.cpp
	friend void uMT_SystemTicks();		// uMTarduinoSysTick.cpp

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
	void Init(TaskId_t _myTid);
	void CleanUp();

	const __FlashStringHelper *TaskStatus2String();

};

#endif



/////////////////////////////////////// EOF

