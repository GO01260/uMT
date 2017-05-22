////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMT.h
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


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ************** NOTES *******************************************
//
// 1)	A task is identified by its index in TaskList.
// 2)	Task[0] = IDLE task
// 3)	Task[1] = Arduino Main loop()
// 4)	IDLE task is never in any list/queue
// 5)	The RUNNING task is NEVER in any queue!
// 6)	[UNUSED] EnterCritRegion()/ExitCritRegion() are used to prevent rescheduling do to 
//		either Timer Tick or Sem/event signaling from ISR
// 7)	Queue management (Ready, Sem and Timers) MUST be performed with INTS disabled
// 8)	The STACK for Task[1] (Arduino Main loop()) is inherited from ARDUINO environmnent 
//		(it is using the default stack)
// 9)	After RTI instruction (AVR return from interrupts), GLOBAL INTERRUPT is ALWAYS enabled.
//		As a consequence, a return from Suspend() has always got INTS enabled.
// 10)	TIMERS are allocated in 2 places:
//		A)	In the TASK (uTask) class itself, to manage Event/Semaphore/WakeupAfter timeouts.
//			These are called TASK TIMERS (uMT_TM_IAM_TASK) and they can only manage timeouts (e.g., they cannot send Events)
//			This has been done to GARANTEE that a task can wait (be suspended) on Timers and timeout on Events or Sempahores because a
//			TIMER is always available. This simplifies the Application writing (less error checking) but it does not optimize
//			system resources because we might have a shortage of AGENT TIMERS while few TASK TIMERS might be available.
//		B)	In the uMT class to manage AGENT TIMERS (uMT_TM_IAM_AGENT) to be able to send Events at later time.
//			At any time, at most 1 AGENT can be active.
// 11)	TIMERS are numbered the same as TASK ID for TIMER_TASKS, "index+uMT_MAX_TASK_NUM" for AGENT times.
//		AGENT timers can be more than TASKS to increase flexibility in delivering future Events.
// 12)	When called from ISR, uMT routine must NOT preemptive the calling task but "NeedResched" is set for TimeTick further processing
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef uMT_H
#define uMT_H

#include "uMTconfiguration.h"

#include "uMTdataTypes.h"


///////////////////////////////////////////////////////////////////////////////////
//
//	uMT CONFIGURATION
//
//	Returned with Kn_GetConfiguration()
//
////////////////////////////////////////////////////////////////////////////////////
class uMTcfg
{
public:
	Bool_t		Use_Events;
	Bool_t		Use_Semaphores;
	Bool_t		Use_Timers;
	Bool_t		Use_RestartTask;
	Bool_t		Use_PrintInternals;

	uint16_t	Max_Tasks;
	uint16_t	Max_Semaphores;
	uint16_t	Max_Events;
	uint16_t	Max_AgentTimers;
	uint16_t	Max_AppTasks_Stack;		// Any user created STACK
	uint16_t	Max_Task1_Stack;		// Arduino loop() STACK
	uint16_t	Max_Idle_Stack;			// Idle STACK
};


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT GENERIC options (SEM, EVENTS)
//
////////////////////////////////////////////////////////////////////////////////////

#define uMT_NULL_EVENT			0		/* NULL EVENT */

#define uMT_NULL_OPT	0x00			// Null, invalid
#define uMT_WAIT		0x01			// Wait if not free
#define uMT_NOWAIT		0x02			// Do not wait, E_WOULD_BLOCK returned
#define uMT_ANY			0x04			// ev_receive, any event

typedef uint8_t			uMToptions_t;		// 8 bits - EVENT flags


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT  ERRNO
//
////////////////////////////////////////////////////////////////////////////////////

enum Errno_t
{
/* 00 */ E_SUCCESS,					// SUCCESS
/* 01 */ E_ALREADY_INITED,			// KERNEL already inited
/* 02 */ E_ALREADY_STARTED,			// TASK already started
/* 03 */ E_NOT_INITED,				// KERNEL not inited
/* 04 */ E_WOULD_BLOCK,				// Operation would block calling TASK
/* 05 */ E_NOMORE_TASKS,			// No more TASK entries available
/* 06 */ E_INVALID_TASKID,			// Invalid TASK Id
/* 07 */ E_INVALID_TIMERID,			// Invalid TIMER Id
/* 08 */ E_INVALID_TOTAL_TASK_NUM,	// Not enough TASK entries configured in the Kernel (Kn_Start)
/* 09 */ E_INVALID_SEMID,			// Invalid SEMAPHORE Id
/* 10 */ E_INVALID_TIMEOUT,			// Invalid timeout (zero or too large)
/* 11 */ E_OVERFLOW_SEM,			// Semaphore counter overflow
/* 12 */ E_NOMORE_TIMERS,			// No more Timers available
/* 13 */ E_NOT_OWNED_TIMER,			// TIMER is not owned by this task
/* 14 */ E_TASK_NOT_STARTED,		// TASK not started, cannot ReStartTask()
/* 15 */ E_TIMEOUT,					// Timeout
/* 16 */ E_NOT_ALLOWED,				// Not allowed [Tk_ReStartTask() suicide]
/* 17 */ E_INVALID_OPTION			// Invalid additional option

};


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT INTERNAL DEFINEs, TYPEDEFs, ETCs
//
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT other INCLUDES
//
////////////////////////////////////////////////////////////////////////////////////

extern "C" { unsigned int sysTickHook();};

#include "uMT_ExtendedTime.h"
#include "uMTtimer.h"
#include "uMTtask.h"
#include "uMTqueue.h"
#include "uMTsemaphores.h"


class uMT
{
	/////////////////////////////////
	// FRIENDS!!!
	/////////////////////////////////
	friend void KLL_TaskLoop();			// Test0_KernelLowLevel.cpp
	friend void KLL_MainLoop();			// Test0_KernelLowLevel.cpp
	friend unsigned uMTdoTicksWork();	// uMTarduinoSysTick.cpp
	friend void uMT_SystemTicks();		// uMTarduinoSysTick.cpp
	friend unsigned int sysTickHook();	// uMTarduinoSysTick.cpp

	friend class uMTtaskQueue;

private:

	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: GENERIC
	//////////////////////////////////////////////////////////////////////////////////////////
static	Bool_t	Inited;				// if uMT is inited or not
volatile Bool_t	BlinkingLED;		// Set for Led to blink


	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Task Queue management
	//////////////////////////////////////////////////////////////////////////////////////////
	void		Tk_RemoveFromAnyQueue(uTask *pTask);	// Remove TASK from any QUEUE
	void		ReadyTask(uTask *pTask);		// Make a task ready and insert in the Ready list
	void		ReadyTaskLocked(uTask *pTask);	// Make a task ready and insert in the Ready list
	void		Check4Preemption(Bool_t AllowPreemption);	// Check is an higher priority task must preempt me...



#define LEGACY_CRIT_REGIONS
#ifdef LEGACY_CRIT_REGIONS
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Critical Regions
	//
	// The following is a legacy from APSDexec, they do not seem to be necessary/useful in uMT
	//
	//////////////////////////////////////////////////////////////////////////////////////////
volatile uint8_t	NoResched;			// If set, prevent rescheduling
volatile Bool_t		NeedResched;		// Set if a Reschedule is needed

inline	Bool_t		InsideCriticalRegion() { return (NoResched > 0); };

inline void		EnterCritRegion() { 
	CpuStatusReg_t CpuFlags = isrKn_IntLock();
	NoResched++; 
	isrKn_IntUnlock(CpuFlags);
};

inline void		ExitCritRegion() {
	CpuStatusReg_t CpuFlags = isrKn_IntLock();
	NoResched--; 
	isrKn_IntUnlock(CpuFlags);
};

#endif





	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: SYSTEM SPECIFIC
	//////////////////////////////////////////////////////////////////////////////////////////
	void		SetupSysTicks();
//	void		SwitchStack();
	StackPtr_t	NewTask(StackPtr_t TaskStackBase, StackSize_t StackSize, void (*TaskStartAddr)(), void (*BadExit)());
	void		ResumeTask(StackPtr_t StackPtr);
	void		Suspend();
	void		Suspend2();

#ifndef WIN32		// Arduino

void	CheckTaskMagic(uTask *task, const __FlashStringHelper*String);
void	CheckInterrupts(const __FlashStringHelper *String);

#if uMT_USE_TIMERS==1
void	CheckTimerMagic(uTimer *timer, const __FlashStringHelper *String);
#endif

#endif

	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Tasks
	//////////////////////////////////////////////////////////////////////////////////////////

	// The following members must disable INTS when used
volatile uint16_t TimeSlice;			// How many ticks the current task can run


//volatile Timer_t	TickCounter;	// Ticks counter
	uMT_ExtTime	TickCounter;		// Ticks counter

volatile Bool_t	TimeSharingEnabled;	// It controls TimeSharing
volatile Bool_t	NoPreempt;			// If set, current task cannot be pre-empted

	uint8_t		ActiveTaskNo;		// Number of Active tasks, excluding IDLE

	uMTtaskQueue ReadyQueue;		// Ready QUEUE

	uTask		*UnusedQueue;		// Pointer to the UNUSED task list (no TotQueue counter)

	uTask		*Running;			// Pointer to the running task
	uTask		*LastRunning;		// Pointer to the running task
	uTask		*IdleTaskPtr;		// pointer to the IDLE task (shortcut)

	uTask		TaskList[uMT_MAX_TASK_NUM];		// Task list


static void		BadExit();			// Helper routine
static void		IdleLoop();			// Idle routine
	void		Reschedule();
	void 		SetupTaskStacks();	// Setting up tasks' stacks



#if uMT_USE_SEMAPHORES==1
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Semaphore
	//////////////////////////////////////////////////////////////////////////////////////////
	uMTsem		SemList[uMT_MAX_SEM_NUM];
	
inline Bool_t	SemId_Check(SemId_t Sid) { return((Sid >= uMT_MAX_SEM_NUM) ? FALSE : TRUE); };
Errno_t			doSm_Release(SemId_t Sid, Bool_t AllowPreemption);

#endif



#if uMT_USE_TIMERS==1
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Timers used for not blocked tasks
	//////////////////////////////////////////////////////////////////////////////////////////
	Bool_t		AlarmExpired;		// Set if an alarm is expired.
	uTimer		*TimerQueue;		// Pointer to the Timer queue
	uTimer		*FreeTimerQueue;	// Pointer to the FREE Timer queue
	uint8_t		TotTimerQueued;		// Total queued

	uTimer		TimerAgentList[uMT_MAX_TIMER_AGENT_NUM];	// Timer list

	void		TimerQ_Insert(uTimer *pTimer);
	uTimer		*TimerQ_Pop();
	void		TimerQ_Expired(uTimer *pTimer);
	void		TimerQ_PushFree(uTimer *pTimer);
	uTimer		*TimerQ_PopFree();
	Errno_t		TimerQ_CancelTimer(uTimer *pTimer);
	void		TimerQ_CancelAll(uTask *pTask);
inline uTimer	*Tmid2TimerPtr(TimerId_t TmId) { return(&TimerAgentList[TmId - uMT_MAX_TASK_NUM]);}; // Tmid MUST be VALID!!!

inline 	Bool_t	TimerId_Check(TimerId_t TmId) { return((TmId >= uMT_MAX_TASK_NUM+uMT_MAX_TIMER_AGENT_NUM) ? FALSE : TRUE); };
	Errno_t		Timer_EventTimout(Timer_t timeout, Event_t Event, TimerId_t &TmId, TimerFlag_t _Flags);

#endif

#if uMT_USE_EVENTS==1
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Events
	//////////////////////////////////////////////////////////////////////////////////////////
	void		EventSend(uTask *pTask, Event_t Event);
	Errno_t		doEv_Send(TaskId_t Tid, Event_t Event, Bool_t AllowPreemption);
	Bool_t		EventVerified(uTask *pTask);
	const __FlashStringHelper *EventFlag2String(uMToptions_t EventFlag);
#endif


	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: RAM
	//////////////////////////////////////////////////////////////////////////////////////////
	StackPtr_t	FreeRAM_0;			// Free memory after STACK allocation (=> memory availale for application)



	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	// PUBLIC ENTRY POINTS (isrXx_nnn() can be called from ISR)
	//
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
public:

	////////////////////////////////////////////////////////
	// Generic KERNEL
	////////////////////////////////////////////////////////
		Errno_t	Kn_Start(Bool_t _TimeSharingEnabled = TRUE, Bool_t _BlinkingLED = TRUE);
		void	isrKn_FatalError();
		void	isrKn_FatalError(const __FlashStringHelper *String);
		void	isrKn_Reboot(); 

inline	uint16_t isrKn_GetVersion() { return (uMT_VERSION_NUMBER);};

static	StackPtr_t Kn_GetSPbase();			// If STATIC STACK ALLOCATION, it returns the HeapPointer (end of unitialized + initialized data)
static	StackPtr_t Kn_GetRAMend();			// Return RAMEND (top address in RAM)
static	StackPtr_t Kn_GetFreeRAM();			// Return => (StackPointer - HeapPointer)
static	StackPtr_t Kn_GetFreeRAMend();		// Return => (RAM_END - HeapPointer)

#ifdef WIN32

StackPtr_t	Kn_GetSP();

#else	// Arduino

#if defined(ARDUINO_ARCH_AVR)
#define	Kn_GetSP() (StackPtr_t)SP
#endif

#if defined(ARDUINO_ARCH_SAM)
static	StackPtr_t	Kn_GetSP();
#endif
#endif

		Errno_t	Kn_PrintInternals();					// Print internals structure to Serial

		Errno_t	Kn_GetConfiguration(uMTcfg &pCfg);		// Returns internal configuration
		Errno_t	Kn_PrintConfiguration(uMTcfg &pCfg);	// Print configuration to Serial

static 	CpuStatusReg_t	isrKn_IntLock();
static 	void			isrKn_IntUnlock(CpuStatusReg_t Flags);

	// Generic KERNEL, can be called from ISR
inline Timer_t	isrKn_GetKernelTick() { return (TickCounter.Low); };

	////////////////////////////////////////////////////////
	// TASK management
	////////////////////////////////////////////////////////
		Errno_t Tk_CreateTask(FuncAddress_t StartAddress, TaskId_t &Tid, FuncAddress_t _BadExit = NULL);
		Errno_t Tk_DeleteTask(TaskId_t Tid);
		Errno_t Tk_DeleteTask() { if (Inited == FALSE) return(E_NOT_INITED); return(Tk_DeleteTask(Running->myTid)); };
		Errno_t	Tk_StartTask(TaskId_t Tid);
		Errno_t	Tk_GetMyTid(TaskId_t &Tid);
		Errno_t	Tk_Yield();

#if uMT_USE_RESTARTTASK==1
		Errno_t	Tk_ReStartTask(TaskId_t Tid);
#endif

inline	uint8_t Tk_GetActiveTaskNo() { return(ActiveTaskNo); };

	// TASK management: can be called from ISR?
inline	Bool_t	Tk_SetTimeSharing(Bool_t NewValue) { Bool_t oldValue = TimeSharingEnabled; TimeSharingEnabled = NewValue; return(oldValue); };
inline	Bool_t	Tk_GetTimeSharing(Bool_t NewValue) { return(TimeSharingEnabled); };

inline	Bool_t	Tk_SetPreemption(Bool_t NewValue) { Bool_t oldValue = NoPreempt; NoPreempt = NewValue; return(oldValue);};
inline	Bool_t	Tk_GetPreemption() { return(NoPreempt);};

inline	Bool_t	Tk_SetBlinkingLED(Bool_t NewValue) { Bool_t oldValue = BlinkingLED; BlinkingLED = NewValue; return(oldValue);};
inline	Bool_t	Tk_GetBlinkingLED() { return(BlinkingLED);};

		Errno_t	Tk_SetPriority(TaskId_t Tid, TaskPrio_t npriority, TaskPrio_t &ppriority);
		Errno_t	Tk_GetPriority(TaskId_t Tid, TaskPrio_t &ppriority);
		Errno_t	Tk_GetPriority(TaskPrio_t &ppriority);

		Errno_t	Tk_SetParam(TaskId_t Tid, Param_t _parameter);
inline	Errno_t	Tk_GetParam(Param_t &_parameter) { if (Inited == FALSE) return(E_NOT_INITED); _parameter = Running->Parameter; return(E_SUCCESS);};





#if uMT_USE_SEMAPHORES==1
	////////////////////////////////////////////////////////
	// SEMAPHORE management
	////////////////////////////////////////////////////////
#if uMT_USE_TIMERS==1
		Errno_t	Sm_Claim(SemId_t Sid, uMToptions_t Options, Timer_t timeout=(Timer_t)0);
inline	Errno_t	isrSm_Claim(SemId_t Sid) {Sm_Claim(Sid, uMT_NOWAIT, 0); };		// uMT_NOWAIT!!!!!
#else
	Errno_t	Sm_Claim(SemId_t Sid, uMToptions_t Options);
#endif
inline	Errno_t	Sm_Release(SemId_t Sid) {return(doSm_Release(Sid, FALSE)); };
inline	Errno_t	isrSm_Release(SemId_t Sid) {return(doSm_Release(Sid, NoResched > 0 ? FALSE : TRUE)); };


		Errno_t	Sm_SetQueueMode(SemId_t Sid, QueueMode_t Mode);
#endif



#if uMT_USE_EVENTS==1
	////////////////////////////////////////////////////////
	// EVENT management
	////////////////////////////////////////////////////////
#if uMT_USE_TIMERS==1
		Errno_t	Ev_Receive(Event_t	eventin, uMToptions_t flags, Event_t *eventout, Timer_t timeout=(Timer_t)0);
#else
		Errno_t	Ev_Receive(Event_t	eventin, uMToptions_t flags, Event_t *eventout);
#endif

	// EVENT management can be called from ISR
inline 	Errno_t	isrEv_Send(TaskId_t Tid, Event_t Event) {return(doEv_Send(Tid, Event, FALSE));};
inline 	Errno_t	Ev_Send(TaskId_t Tid, Event_t Event) {return(doEv_Send(Tid, Event, TRUE));};

#endif



#if uMT_USE_TIMERS==1
	////////////////////////////////////////////////////////
	// TIMER management
	////////////////////////////////////////////////////////

	// Wakeup after some time
		Errno_t Tm_WakeupAfter(Timer_t timeout);	

	// Send an event after some time
//inline 	
		Errno_t Tm_EvAfter(Timer_t timeout, Event_t Event, TimerId_t &TmId)
{
	return(Timer_EventTimout(timeout, Event, TmId, (uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT))); 
};

	// Send an event every ticks
//inline 
		Errno_t Tm_EvEvery(Timer_t timeout, Event_t Event, TimerId_t &TmId)
{
	return(Timer_EventTimout(timeout, Event, TmId, (uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT | uMT_TM_REPEAT)));
};

		Errno_t Tm_Cancel(TimerId_t TmId);
#endif

};



extern uMT Kernel;

#endif

//////////////////// EOF
