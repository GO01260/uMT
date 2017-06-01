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
// 12)	When called from ISR, uMT routine must NOT preempt the calling task but "NeedResched" is set for TimeTick further processing
// 12)	When called from ISRp, uMT routine can preempt the calling task
// 13)	To support Tk_DeleteTask() (suicide...), a dedicated STACK must be allocated in Suspend() for the Kernel.
//		The size of this task is critical: it cannot be too small (otherwise there is the risk to CRASH the kernel)
//		and it cannot be too large otherwise too much memory will be consumed in Aruino UNO.
//		AVR: 64 bytes (defined in "uMT_AVR_SysTick.cpp")
//		SAM: 256 bytes (defined in "uMT_SAM_SysTick.cpp")
//		Remember that Suspend() is executed with INTS disabled as well as the rest of the Kernel until 
//		next task switching, so this STACK will only be needed to accomodate Kernel needs.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// STRATEGY FOR CRITICAL REGIONS
//
// There are fundamentaly 2 mechanisms available in uMT to implement critical regions:
//	1 - Interrupts disabling/enabling (e.g., isr_Kn_IntLock())
//	2 - Pre-emption disabling/enabling (e.g., EnterCritRegion())
//
// isr_Kn_IntLock() must be used wen accessing data which can be accessed by ISR routines as well
// EnterCritRegion() must be used for all the other cases.
// As a consequence, primitives isr_XXX_YYY() must always disable INTS when processing.
//
// Version 2.0.x is using isr_Kn_IntLock() only.
//
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// STRATEGY FOR MEMORY MANAGEMENT
//
// Ideally, uMT would like to replace the malloc()/realloc()/free() calls with its own using 
// a thread safe approach. Unfortunately in uMT version 2 the Author has been unable to replace them from
// the AVR library so a different schema is used.
//
// AVR malloc()
// AVR malloc() can be configured by setting the 2 variables: "__malloc_heap_start" and "__malloc_heap_end" to
// the START and to the END of the memory area used for dynamic memory management. This setting MUST be performed before the first call
// to malloc(). However, if we only need to change the end of the range (__malloc_heap_end) this can only be done afterwards.
// As a consequence, in Kn_Start() the "__malloc_heap_end" is set by reducing the heap of the TID1 stack size as
// defined in the uMT configuration (static or dynamic).
//
// SAM malloc()
// With th SAM architecture, a new set of malloc()/realloc()/free() has been provided. This is also implementing
// a thread safe access, after Kn_Start() initialization. Code has been taken from GitHub (aknowledgement to the Author(s)!) 
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef uMT_H
#define uMT_H

#include "uMTconfiguration.h"
#include "uMTdataTypes.h"
#include "uMTkernelCfg.h"
#include "stdlib_private.h"

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT GENERIC options (SEM, EVENTS)
//
////////////////////////////////////////////////////////////////////////////////////

#define uMT_NULL_EVENT			0		/* NULL EVENT */

#define uMT_NULL_OPT	0x00			// Null, invalid
#define uMT_WAIT		0x01			// Wait if not free
#define uMT_NOWAIT		0x02			// Do not wait, E_WOULD_BLOCK returned
#define uMT_ANY			0x10			// ev_receive, any event
#define uMT_ALL			0x20			// ev_receive, any event

typedef uint8_t			uMToptions_t;		// 8 bits - EVENT flags


#include "uMTerrno.h"

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT INTERNAL DEFINEs, TYPEDEFs, ETCs
//
////////////////////////////////////////////////////////////////////////////////////

#if  defined(ARDUINO_ARCH_SAM)  || defined(WIN32)
extern "C" { unsigned int sysTickHook(); void pendSVHook();};

#define uMTmalloc	malloc
#define uMTfree		free
#else

extern void *uMTmalloc(size_t len);
extern void uMTfree(void *p);
extern void *uMTrealloc(void *ptr, size_t len);

#endif


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT other INCLUDES
//
////////////////////////////////////////////////////////////////////////////////////

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

	friend unsigned uMTdoTicksWork();	// uMTarduinoCommon.cpp

	friend void uMT_SystemTicks();		// uMT_AVR_SysTick.cpp
	friend void pendSVHook();			// uMT_SAM_SysTick.cpp
	friend unsigned int sysTickHook();	// uMT_SAM_SysTick.cpp

	friend class uMTtaskQueue;

private:

	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: GENERIC
	//////////////////////////////////////////////////////////////////////////////////////////
	static	Bool_t	Inited;				// if uMT is inited or not
	Bool_t			KernelStackMode;	// Set to TRUE when STACK is PRIVATE KERNEL

Errno_t		doStart();

	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: CONFIGURATION
	//////////////////////////////////////////////////////////////////////////////////////////
	rwCfg	kernelCfg;			// Kernel configuration


	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Task Queue management
	//////////////////////////////////////////////////////////////////////////////////////////
	void		Tk_RemoveFromAnyQueue(uTask *pTask);	// Remove TASK from any QUEUE
	void		ReadyTask(uTask *pTask);		// Make a task ready and insert in the Ready list

	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Preemption
	//////////////////////////////////////////////////////////////////////////////////////////
volatile Bool_t		NeedResched;		// Set if a Reschedule is needed

// Check is an higher priority task must preempt me...
	void		Check4Preemption();

	// If reschedule is required, suspend me...
inline	void		Check4NeedReschedule() { if (NeedResched && NoPreempt == FALSE) Suspend(); };




#define LEGACY_CRIT_REGIONS 1		// Used by ArduinoCommon.cpp
#if LEGACY_CRIT_REGIONS==1
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	// INTERNAL: Critical Regions
	//
	//////////////////////////////////////////////////////////////////////////////////////////
volatile uint8_t	NoResched;			// If set, prevent rescheduling
inline	Bool_t		InsideCriticalRegion() { return (NoResched > 0); };

inline void		EnterCritRegion() { 
	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();
	NoResched++; 
	isr_Kn_IntUnlock(CpuFlags);
};

inline void		ExitCritRegion() {
	CpuStatusReg_t CpuFlags = isr_Kn_IntLock();
	NoResched--; 
	isr_Kn_IntUnlock(CpuFlags);
};

	void		ReadyTaskLocked(uTask *pTask);	// Make a task ready and insert in the Ready list

#endif

volatile Bool_t	NoPreempt;			// If set, current task cannot be pre-empted


	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: SYSTEM SPECIFIC
	//////////////////////////////////////////////////////////////////////////////////////////
	void		SetupSysTicks();
	void		NewStackReschedule();	// Switch to a private STACK and call Reschedule()
	void		NewStackReborn();	// Switch a private STACK and call Reborn()
	StackPtr_t	NewTask(StackPtr_t TaskStackBase, StackSize_t StackSize, void (*TaskStartAddr)(), void (*BadExit)());
	void		ResumeTask(StackPtr_t StackPtr);
	void		Suspend();
	void		doDeleteTask(uTask *pTask);


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

	uint8_t		ActiveTaskNo;		// Number of Active tasks, excluding IDLE

	uMTtaskQueue ReadyQueue;		// Ready QUEUE

	uTask		*UnusedQueue;		// Pointer to the UNUSED task list (no TotQueue counter)

	uTask		*Running;			// Pointer to the running task
	uTask		*LastRunning;		// Pointer to the running task
	uTask		*IdleTaskPtr;		// pointer to the IDLE task (shortcut)

#if uMT_ALLOCATION_TYPE==uMT_FIXED_STATIC
	uTask		TaskList[uMT_DEFAULT_TASK_NUM];		// Task list
#else
	uTask		*TaskList;			// Task list
#endif

static void		IdleLoop();				// Idle routine
	void		Reschedule();			// Find next RUNNING task
	void		Reborn();				// Restart current task
	void 		SetupTaskStacks();		// Setting up tasks' stacks
	void 		SetupMallocLimits();	// Setup MALLOC() limitis

#if uMT_USE_RESTARTTASK==1
		Errno_t	doReStartTask(uTask *pTask);
#endif


#if uMT_USE_SEMAPHORES==1
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Semaphore
	//////////////////////////////////////////////////////////////////////////////////////////
#if uMT_ALLOCATION_TYPE==uMT_FIXED_STATIC
	uMTsem		SemList[uMT_DEFAULT_SEM_NUM];
#else
	uMTsem		*SemList;
#endif
	
inline Bool_t	SemId_Check(SemId_t Sid) { return((Sid >= kernelCfg.Semaphores_Num) ? FALSE : TRUE); };
	Errno_t		doSm_Release(SemId_t Sid, Bool_t AllowPreemption);
#endif



#if uMT_USE_TIMERS==1
	//////////////////////////////////////////////////////////////////////////////////////////
	// INTERNAL: Timers used for not blocked tasks
	//////////////////////////////////////////////////////////////////////////////////////////
	Bool_t		AlarmExpired;		// Set if an alarm is expired.
	uTimer		*TimerQueue;		// Pointer to the Timer queue
	uTimer		*FreeTimerQueue;	// Pointer to the FREE Timer queue
	uint8_t		TotTimerQueued;		// Total queued

#if uMT_ALLOCATION_TYPE==uMT_FIXED_STATIC
	uTimer		TimerAgentList[uMT_DEFAULT_TIMER_AGENT_NUM];	// Timer list
#else
	uTimer		*TimerAgentList;	// Timer list
#endif

	void		TimerQ_Insert(uTimer *pTimer);
	uTimer		*TimerQ_Pop();
	void		TimerQ_Expired(uTimer *pTimer);
	void		TimerQ_PushFree(uTimer *pTimer);
	uTimer		*TimerQ_PopFree();
	Errno_t		TimerQ_CancelTimer(uTimer *pTimer);
	void		TimerQ_CancelAll(uTask *pTask);
	inline uTimer	*Tmid2TimerPtr(TimerId_t TmId) { return(&TimerAgentList[TmId - kernelCfg.Tasks_Num]);}; // Tmid MUST be VALID!!!

inline 	Bool_t	TimerId_Check(TimerId_t TmId) { 
	return((TmId >= kernelCfg.Tasks_Num + kernelCfg.AgentTimers_Num) ? FALSE : TRUE); };
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
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	// PUBLIC ENTRY POINTS (isr_Xx_nnn() can be called from ISR)
	//
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////
public:

	////////////////////////////////////////////////////////
	// Generic KERNEL
	////////////////////////////////////////////////////////
	static	inline Bool_t	Kn_Inited() { return(Inited); };				// if uMT is inited or not

	Errno_t		Kn_Start() { if (Inited == TRUE) return(E_ALREADY_INITED); kernelCfg.Init(); return(doStart()); };

	// Simple configuration...
	Errno_t		Kn_Start(Bool_t	_TimeSharingEnabled) {
		if (Inited == TRUE)
			return(E_ALREADY_INITED);
		kernelCfg.Init();
		kernelCfg.TimeSharingEnabled = _TimeSharingEnabled;
		return(doStart());
	};


#if defined(ARDUINO_ARCH_SAM) || defined(__AVR_ATmega2560__) || defined(WIN32)
	// WIN32 to force compilation
	Errno_t		Kn_Start(uMTcfg &Cfg) { if (Inited == TRUE) return(E_ALREADY_INITED); kernelCfg = Cfg.rw; return(doStart()); };
#else
	// Arduino UNO CANNOT be configured run-time!
	Errno_t		Kn_Start(uMTcfg &Cfg) 
	{
		if (Inited == TRUE) return(E_ALREADY_INITED);
		kernelCfg.Init();
		kernelCfg.BlinkingLED = Cfg.rw.BlinkingLED; 
		kernelCfg.IdleLED = Cfg.rw.IdleLED; 
		kernelCfg.TimeSharingEnabled = Cfg.rw.TimeSharingEnabled; 
		return(doStart()); 
	};
#endif
	
		void	isr_Kn_FatalError();
		void	isr_Kn_FatalError(const __FlashStringHelper *String);
		void	isr_Kn_Reboot(); 

inline	uint16_t isr_Kn_GetVersion() { return (uMT_VERSION_NUMBER);};

static	StackPtr_t	Kn_GetSPbase();			// If STATIC STACK ALLOCATION, it returns the HeapPointer (end of unitialized + initialized data)
static	StackPtr_t	Kn_GetRAMend();			// Return RAMEND (top address in RAM)
static	StackPtr_t	Kn_GetFreeRAM();			// Return => (StackPointer - HeapPointer)
static	StackPtr_t	Kn_GetFreeRAMend();		// Return => (RAM_END - HeapPointer)
static	void		BadExit();			// Helper routine

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

static 	CpuStatusReg_t	isr_Kn_IntLock();
static 	void			isr_Kn_IntUnlock(CpuStatusReg_t Flags);

	// Generic KERNEL, can be called from ISR
inline Timer_t	isr_Kn_GetKernelTick() { return (TickCounter.Low); };

	////////////////////////////////////////////////////////
	// TASK management
	////////////////////////////////////////////////////////
		Errno_t Tk_CreateTask(FuncAddress_t StartAddress, TaskId_t &Tid, FuncAddress_t _BadExit = NULL, StackSize_t _StackSize = 0);
		Errno_t Tk_DeleteTask(TaskId_t Tid);
		Errno_t Tk_DeleteTask() { if (Inited == FALSE) return(E_NOT_INITED); return(Tk_DeleteTask(Running->myTid)); };
		Errno_t	Tk_StartTask(TaskId_t Tid);
		Errno_t	Tk_GetMyTid(TaskId_t &Tid);
		Errno_t	Tk_Yield();

#if uMT_USE_RESTARTTASK==1
		Errno_t	Tk_ReStartTask(TaskId_t Tid);
		Errno_t	Tk_ReStartTask() { return(doReStartTask(Running)); }
#endif

inline	uint8_t Tk_GetActiveTaskNo() { return(ActiveTaskNo); };

	// TASK management: can be called from ISR?
inline	Bool_t	Tk_SetTimeSharing(Bool_t NewValue) { Bool_t oldValue = kernelCfg.TimeSharingEnabled; kernelCfg.TimeSharingEnabled = NewValue; return(oldValue); };
inline	Bool_t	Tk_GetTimeSharing(Bool_t NewValue) { return(kernelCfg.TimeSharingEnabled); };

inline	Bool_t	Tk_SetPreemption(Bool_t NewValue) { Bool_t oldValue = NoPreempt; NoPreempt = NewValue; return(oldValue);};
inline	Bool_t	Tk_GetPreemption() { return(NoPreempt);};

inline	Bool_t	Tk_SetBlinkingLED(Bool_t NewValue) { Bool_t oldValue = kernelCfg.BlinkingLED; kernelCfg.BlinkingLED = NewValue; return(oldValue);};
inline	Bool_t	Tk_GetBlinkingLED() { return(kernelCfg.BlinkingLED);};

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
inline	Errno_t	isr_Sm_Claim(SemId_t Sid) {Sm_Claim(Sid, uMT_NOWAIT, 0); };		// uMT_NOWAIT!!!!!
#else
	Errno_t	Sm_Claim(SemId_t Sid, uMToptions_t Options);
#endif

inline	Errno_t	Sm_Release(SemId_t Sid) {return(doSm_Release(Sid, TRUE)); };
inline	Errno_t	isr_Sm_Release(SemId_t Sid) {return(doSm_Release(Sid, FALSE)); };
inline	Errno_t	isr_p_Sm_Release(SemId_t Sid) {return(doSm_Release(Sid, TRUE)); };


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
inline 	Errno_t	isr_Ev_Send(TaskId_t Tid, Event_t Event) {return(doEv_Send(Tid, Event, FALSE));};
inline 	Errno_t	isr_p_Ev_Send(TaskId_t Tid, Event_t Event) {return(doEv_Send(Tid, Event, TRUE));};
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
