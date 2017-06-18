////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTarduinoHelpers.cpp
//	AUTHOR: Antonio Pastore - April 2017
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

#ifdef WIN32


#define SerialPRINTln(x)
#define SerialPRINT(x)
#define SerialPRINT2ln(x, y)
#define SerialPRINT2(x, y)

#define SerialFLUSH()

#define digitalWrite(LED_BUILTIN, HIGH)


#else

#define SerialPRINTln		Serial.println
#define SerialPRINT			Serial.print
#define SerialPRINT2ln		Serial.println
#define SerialPRINT2		Serial.print
#define	SerialFLUSH			Serial.flush
#endif


//#include <Arduino.h>

#include "uMT.h"

#include "uMTdebug.h"

///////////////////////////////////////////////////////////////////////////////////
//
// This file contains generic ARDUINO dependent helpers routines 
// 
///////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	BadExit
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::BadExit()
{
	SerialPRINTln(F("================= uMT Kernel: BadExit() called! ================="));
	SerialFLUSH();

	Kernel.isr_Kn_FatalError();

#ifdef ZAPPED
	// Delete task
	Kernel.Tk_DeleteTask(Kernel.Running->myTid);

	// It never returns...
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isr_Kn_FatalError - ARDUINO
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::isr_Kn_FatalError()
{
	NoPreempt = TRUE;		// Prevent rescheduling....

	SerialPRINTln(F("========= isr_Kn_FatalError ==========="));
	SerialFLUSH();

	Kn_PrintInternals();

	isr_Kn_Reboot();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isr_Kn_FatalError - ARDUINO
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::isr_Kn_FatalError(const __FlashStringHelper *String)
{
	NoPreempt = TRUE;		// Prevent rescheduling....

	SerialPRINT(F("========= isr_Kn_FatalError ("));
	SerialPRINT(String);
	SerialPRINTln(F(")==========="));
	SerialFLUSH();

	Kn_PrintInternals();

	isr_Kn_Reboot();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iMTDummyLoop - ARDUINO UNO/MEGA/SAM
//
// The IDLE task...
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::IdleLoop()
{
	while (1)			// Do nothing....
	{
		if (Kernel.kernelCfg.IdleLED)
		{
			digitalWrite(LED_BUILTIN, HIGH);
		}

#if uMT_IDLE_TIMEOUT==1
		// Every time the timeslice for IDLE expires, this triggers some special action (e.g., PrintInternals()

		if (Kernel.TimeSlice <= 1)
		{
//			digitalWrite(LED_BUILTIN, HIGH);
			Kernel.Kn_PrintInternals();
			
			// Reset
			Kernel.TimeSlice = uMT_IDLE_TIMEOUTVALUE;
		}
#endif

		if (Kernel.ActiveTaskNo == 0)
		{
			Kernel.isr_Kn_FatalError(F("No active tasks in the system (all deleted?)"));
		}

		if (Kernel.NeedResched)
		{
			// If there is any higher priority task which needs to run, do it...
			Kernel.Tk_Yield();
		}

	}
}

		

#if uMT_SAFERUN==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckTaskMagic - ARDUINO
//
// CheckTaskMagic
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::CheckTaskMagic(uTask *task, const __FlashStringHelper *String)
{
	if (task->magic != uMT_TASK_MAGIC)
	{
		SerialPRINT(F("uMT: CheckTaskMagic() invalid MAGIC number in TASK structure, Func="));
		SerialPRINTln(String);
		SerialFLUSH();

		isr_Kn_FatalError();
	}

	StackPtr_t MaxSP = (task->StackBaseAddr + task->StackSize - 1);

	if (task->SavedSP <  task->StackBaseAddr || task->SavedSP > MaxSP)
	{
		SerialPRINT(F("uMT: CheckTaskMagic(Tid="));
		SerialPRINT((unsigned int)task->myTid.GetID());

		SerialPRINT(F("): Invalid SP = "));
		SerialPRINT((unsigned int)task->SavedSP);

		SerialPRINT(F("(MIN SP = "));
		SerialPRINT((unsigned int)task->StackBaseAddr);

		SerialPRINT(F(" MAX SP = "));
		SerialPRINT((unsigned int)MaxSP);
		SerialPRINTln(F(")"));

		SerialFLUSH();

		isr_Kn_FatalError();
	}
	
#ifdef ZAPPED
#if uMT_DEBUG==1
	SerialPRINT(F("uMT: CheckTaskMagic: "));
	SerialPRINT(String);
	SerialPRINT(F(": TID = "));
	SerialPRINT((unsigned int)task->myTid);
	SerialPRINT(F(" - SP = "));
	SerialPRINT((unsigned int)task->SavedSP);
	SerialPRINT(F(" - Priority = "));
	SerialPRINTln(task->Priority);
	SerialFLUSH();
#endif
#endif

}

#if uMT_USE_TIMERS==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckTimerMagic - ARDUINO
//
// CheckTimerMagic
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::CheckTimerMagic(uTimer *timer, const __FlashStringHelper *String)
{
	if (timer->magic != uMT_TIMER_MAGIC)
	{
		SerialPRINT(F("uMT: CheckTimerMagic() invalid MAGIC number in TIMER structure, Func="));
		SerialPRINTln(String);
		SerialFLUSH();

		isr_Kn_FatalError();
	}
}
#endif


#endif



#if uMT_USE_PRINT_INTERNALS==1

#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
#define PRINT_MODE	HEX
#else
#define PRINT_MODE	DEC
#endif

#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC
	extern char *__malloc_heap_end;
	extern "C" char *sbrk(int i);
	extern char *__brkval;
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	HELPER routines - ARDUINO
//
/////////////////////////////////////////////////////////////////////////////////////////////////


static void PrintMilliSeconds(Timer_t RunningTime)
{
	unsigned int d, h, m, s, milli;

	milli = RunningTime % 1000;

	RunningTime /= 1000;			// In seconds
	s = RunningTime % 60;

	RunningTime /= 60;				// In minutes
	m = RunningTime % 60;

	RunningTime /= 60;				// In hours
	h = RunningTime % 60;

	d = RunningTime / 24;			// in days

	if (d > 0)
	{
		SerialPRINT(d);
		SerialPRINT(F("d:"));
	}
	if (h > 0)
	{
		SerialPRINT(h);
		SerialPRINT(F("h:"));
	}

	if (m > 0)
	{
		SerialPRINT(m);
		SerialPRINT(F("m:"));
	}

	if (s > 0)
	{
		SerialPRINT(s);
		SerialPRINT(F("s:"));
	}

	if (milli > 0)
	{
		SerialPRINT(milli);
		SerialPRINT(F("ms"));
	}
}




#if	uMT_USE_TASK_STATISTICS>=2
static void PrintMilliSeconds(uMTextendedTime RunningTime)
{
	unsigned int d, h, m, s, milli;

	milli = RunningTime.Low % 1000;

	RunningTime = RunningTime.DivideBy(1000);			// In seconds
	s = RunningTime.Low % 60;

	RunningTime = RunningTime.DivideBy(60);			// In minutes
	m = RunningTime.Low % 60;

	RunningTime = RunningTime.DivideBy(60);			// In hours
	h = RunningTime.Low % 60;

	RunningTime = RunningTime.DivideBy(60);			// in days
	d = RunningTime.Low;

	if (d > 0)
	{
		SerialPRINT(d);
		SerialPRINT(F("d:"));
	}

	if (h > 0)
	{
		SerialPRINT(h);
		SerialPRINT(F("h:"));
	}

	if (m > 0)
	{
		SerialPRINT(m);
		SerialPRINT(F("m:"));
	}

	if (s > 0)
	{
		SerialPRINT(s);
		SerialPRINT(F("s:"));
	}

	if (milli > 0)
	{
		SerialPRINT(milli);
		SerialPRINT(F("ms"));
	}
}

static void PrintMicroSeconds(uMTextendedTime RunningTime)
{
	unsigned int us = RunningTime.Low % 1000;

	RunningTime = RunningTime.DivideBy(1000);

	if (RunningTime.High != 0 || RunningTime.Low != 0)
	{
		PrintMilliSeconds(RunningTime);
		SerialPRINT(F(":"));
	}

	SerialPRINT(us);
	SerialPRINT(F("us"));
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_PrintInternals - ARDUINO
//
// It printf internal Kernel structures for debugging purposes
//
/////////////////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Kn_PrintInternals(Bool_t PrintMaxUsedStack)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

#if	uMT_USE_TASK_STATISTICS>=2
	uMTextendedTime usTotalRunning;

	usTotalRunning.Clear();
#endif
	

	EnterCritRegion();		// Prevent rescheduling....

	// Print Kernel info
	SerialPRINTln(F("=========== KERNEL INFO start =================="));

	SerialPRINT(F("ActiveTaskNo="));
	SerialPRINT(ActiveTaskNo);

	SerialPRINT(F(" TimeSharingEnabled="));
	SerialPRINT(kernelCfg.TimeSharingEnabled);

	SerialPRINT(F(" NoPreempt="));
	SerialPRINT(NoPreempt);

#if LEGACY_CRIT_REGIONS==1
	SerialPRINT(F(" NoResched="));
	SerialPRINT(NoResched);
#endif

	SerialPRINT(F(" NeedResched="));
	SerialPRINT(NeedResched);

	SerialPRINT(F(" HEAP_FreeRAM(bytes)="));
#if uMT_ALLOCATION_TYPE==uMT_VARIABLE_DYNAMIC
	SerialPRINT((size_t)(__malloc_heap_end - __brkval));
#else
	SerialPRINT(Kn_GetFreeRAM());
#endif
	SerialPRINTln(F(""));

	uTask *pTask;

	// Print all tasks
	SerialPRINTln(F("=========== RAW TASK LIST =================="));

	for (int idx = 0; idx < kernelCfg.Tasks_Num; idx++)
	{
		pTask = &TaskList[idx];

//		SerialPRINT(F("< idx="));
//		SerialPRINT(idx);

		SerialPRINT(F("< Tid="));
		SerialPRINT(pTask->myTid.GetID());

		SerialPRINT(F(" Status="));
		SerialPRINT(pTask->TaskStatus2String());

		SerialPRINT(F(" Prio="));
		SerialPRINT(pTask->Priority);

		SerialPRINT(F(" SP="));
		SerialPRINT2((unsigned int)pTask->SavedSP, PRINT_MODE);

		SerialPRINT(F(" SPbase="));
		SerialPRINT2((unsigned int)pTask->StackBaseAddr, PRINT_MODE);

		SerialPRINT(F(" SPsize="));
		SerialPRINT(pTask->StackSize);

		SerialPRINT(F(" SPfree="));
		SerialPRINT(((unsigned int)pTask->SavedSP - (unsigned int)pTask->StackBaseAddr));

#if	uMT_USE_EVENTS==1
		SerialPRINT(F(" EV_recv=0x"));
		SerialPRINT2(pTask->EV_received, HEX);

		SerialPRINT(F(" EV_req=0x"));
		SerialPRINT2(pTask->EV_requested, HEX);

		SerialPRINT(F(" EV_cond="));
		SerialPRINT(EventFlag2String(pTask->EV_condition));
#endif


#if uMT_SAFERUN==1
		SerialPRINT(F(" magic=0x"));
		SerialPRINT2(pTask->magic, HEX);
#endif
		if (PrintMaxUsedStack)
		{
			SerialPRINT(F(" MaxUsedStack="));
			SerialPRINT(MaxUsedStack(pTask));
		}

#if	uMT_USE_TASK_STATISTICS>=1
		SerialPRINT(F(" Run#="));
		SerialPRINT(pTask->Run);
#endif

#if	uMT_USE_TASK_STATISTICS>=2
		SerialPRINT(F(" RunningTime="));
		PrintMicroSeconds(pTask->usRunningTime);

		usTotalRunning = usTotalRunning + pTask->usRunningTime;

		SerialPRINT(F(" LastRun="));
		PrintMicroSeconds(pTask->usLastRun);
#endif

		SerialPRINTln(F(" >"));
	}

	SerialPRINT(F("=========== UPTIME: "));

	CpuStatusReg_t	CpuFlags = Kernel.isr_Kn_IntLock();	/* Enter critical region */

	uMTextendedTime msNow = msTickCounter;

	Kernel.isr_Kn_IntUnlock(CpuFlags);

	SerialPRINT(F(" msTickCounter="));
	PrintMilliSeconds(msTickCounter);

#if	uMT_USE_TASK_STATISTICS>=2
	SerialPRINT(F(" UserTime="));
	PrintMicroSeconds(usTotalRunning);

	SerialPRINT(F(" KernelTime="));
	PrintMicroSeconds(usKernelRunningTime);

	SerialPRINT(F(" msTickCounter-UserTime="));
	msNow = msNow - usTotalRunning.DivideBy(1000);
	PrintMilliSeconds(msNow);

#ifdef ZAPPED
	SerialPRINT(F(" msTickCounter.Low="));
	SerialPRINT(msNow.Low);

	SerialPRINT(F(" msTickCounter.High="));
	SerialPRINT(msNow.High);

	SerialPRINT(F(" usTotalRunning.Low="));
	SerialPRINT(usTotalRunning.Low);

	SerialPRINT(F(" usTotalRunning.High="));
	SerialPRINT(usTotalRunning.High);
#endif


#endif


	SerialPRINTln(F(" =================="));

	// Print ReadyQueue
	pTask = ReadyQueue.Head;
	SerialPRINTln(F("=========== ReadyQueue =================="));

	while (pTask != NULL)
	{

		SerialPRINT(F(" Tid="));
		SerialPRINT(pTask->myTid.GetID());

		SerialPRINT(F(" Status="));
		SerialPRINT(pTask->TaskStatus2String());

#if uMT_SAFERUN==1
		SerialPRINT(F(" magic=0x"));
		SerialPRINT2(pTask->magic, HEX);
#endif

		SerialPRINTln(F(">"));

		pTask = pTask->Next;
	}


	// Print SemQueue
//	SerialPRINTln(F("=========== SemQueue =================="));


	for (int idx = 0; idx < kernelCfg.Semaphores_Num; idx++)
	{
		pTask = SemList[idx].SemQueue.Head;

		if (SemList[idx].SemValue > 0 || pTask != NULL)
		{
			SerialPRINT(F("=========== SemQueue ("));
			SerialPRINT(idx);
			SerialPRINT(F(") SemVal="));
			SerialPRINT(SemList[idx].SemValue);
			SerialPRINTln(F(") =================="));
		}


		if (pTask == NULL)
		{
			continue;		// Next queue
		}

		while (pTask != NULL)
		{

			SerialPRINT(F(" Tid="));
			SerialPRINT(pTask->myTid.GetID());

			SerialPRINT(F(" Status="));
			SerialPRINT(pTask->TaskStatus2String());

	#if uMT_SAFERUN==1
			SerialPRINT(F(" magic=0X"));
			SerialPRINT2(pTask->magic, HEX);
	#endif

			SerialPRINTln(F(">"));

			pTask = pTask->Next;
		}
	}

	

#if uMT_USE_TIMERS==1

	// Print TimerQueue
	uTimer *pTimer = TimerQueue;

	SerialPRINT(F("=========== TimerQueue (total="));
	SerialPRINT(TotTimerQueued);
	SerialPRINTln(F(") =================="));

	while (pTimer != NULL)
	{

		SerialPRINT(F(" TimerId="));
		SerialPRINT(pTimer->myTimerId.GetID());

		SerialPRINT(F(" Flags=0X"));
		SerialPRINT2(pTimer->Flags, HEX);

		SerialPRINT(F(" ("));
		SerialPRINT(pTimer->Flags2String());

		SerialPRINT(F(") TaskId="));
		SerialPRINT(pTimer->pTask->myTid.GetID());

		SerialPRINT(F(" NextAlarm.High="));
		SerialPRINT(pTimer->NextAlarm.High);

		SerialPRINT(F(" NextAlarm.Low="));
		SerialPRINT(pTimer->NextAlarm.Low);

		SerialPRINT(F(" Timeout="));
		SerialPRINT(pTimer->Timeout);

#if	uMT_USE_EVENTS==1
		SerialPRINT(F(" Event=0x"));
		SerialPRINT2(pTimer->EventToSend, HEX);
#endif

#if uMT_SAFERUN==1
		SerialPRINT(F(" magic=0X"));
		SerialPRINT2(pTimer->magic, HEX);

		SerialPRINT(F(" TaskMagic=0X"));
		SerialPRINT2(pTimer->pTask->magic, HEX);
#endif

		SerialPRINTln(F(">"));

		pTimer = pTimer->Next;
	}
#endif

	SerialPRINTln(F("=========== KERNEL INFO end =================="));

	ExitCritRegion();		// Allow rescheduling....

	return(E_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tk_PrintInfo - ARDUINO
//
// It printf internal Kernel task structures for debugging purposes
//
/////////////////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Tk_PrintInfo(uMTtaskInfo &Info)
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	EnterCritRegion();		// Prevent rescheduling....

	SerialPRINTln(F("=========== TASK INFO PRINT start =================="));

	SerialPRINT(F("Tid           : "));
	SerialPRINTln(Info.Tid.GetID());
	SerialPRINT(F("Priority      : "));
	SerialPRINTln(Info.Priority);
	SerialPRINT(F("TaskStatus    : "));
	SerialPRINTln(uTask::TaskStatus2String(Info.TaskStatus));

#if	uMT_USE_TASK_STATISTICS>=1
	SerialPRINT(F("Run           : "));
	SerialPRINTln(Info.Run);
#endif

#if	uMT_USE_TASK_STATISTICS>=2
	SerialPRINT(F("RunningTime   : "));
	PrintMicroSeconds(Info.usRunningTime);
#endif

	SerialPRINT(F("StackSize     : "));
	SerialPRINTln(Info.StackSize);
	SerialPRINT(F("FreeStack     : "));
	SerialPRINTln(Info.FreeStack);
	SerialPRINT(F("MaxUsedStack  : "));
	SerialPRINTln(Info.MaxUsedStack);

	SerialPRINTln(F("=========== TASK INFO PRINT end =================="));

	ExitCritRegion();		// Allow rescheduling....

	return(E_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_PrintConfiguration - ARDUINO
//
// It printf configuration structures for debugging purposes
//
/////////////////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Kn_PrintConfiguration(uMTcfg &Cfg)
{
	if (Inited)
		EnterCritRegion();		// Prevent rescheduling....

	SerialPRINTln(F("=========== KERNEL CONFIGURATION PRINT start =================="));


	SerialPRINT(F("Use_Events           : "));
	SerialPRINTln((Cfg.ro.Use_Events ? F("YES") : F("NO")));
	SerialPRINT(F("Use_Semaphores       : "));
	SerialPRINTln((Cfg.ro.Use_Semaphores ? F("YES") : F("NO")));
	SerialPRINT(F("Use_Timers           : "));
	SerialPRINTln((Cfg.ro.Use_Timers ? F("YES") : F("NO")));
	SerialPRINT(F("Use_RestartTask      : "));
	SerialPRINTln((Cfg.ro.Use_RestartTask ? F("YES") : F("NO")));
	SerialPRINT(F("Use_PrintInternals   : "));
	SerialPRINTln((Cfg.ro.Use_PrintInternals ? F("YES") : F("NO")));

	SerialPRINT(F("Tasks_Num            : "));
	SerialPRINTln(Cfg.rw.Tasks_Num);
	SerialPRINT(F("Events_Num           : "));
	SerialPRINTln(Cfg.ro.Events_Num);
	SerialPRINT(F("Semaphores_Num       : "));
	SerialPRINTln(Cfg.rw.Semaphores_Num);
	SerialPRINT(F("Events_Num           : "));
	SerialPRINTln(Cfg.ro.Events_Num);
	SerialPRINT(F("AgentTimers_Num      : "));
	SerialPRINTln(Cfg.rw.AgentTimers_Num);
	SerialPRINT(F("AppTasks_Stack_Size  : "));
	SerialPRINTln(Cfg.rw.AppTasks_Stack_Size);
	SerialPRINT(F("Max_Task1_Stack      : "));
	SerialPRINTln(Cfg.rw.Task1_Stack_Size);
	SerialPRINT(F("Idle_Stack_Size      : "));
	SerialPRINTln(Cfg.rw.Idle_Stack_Size);

	SerialPRINT(F("RAM_Start(Heap start): "));
	SerialPRINT2ln(Cfg.ro.RAM_Start, PRINT_MODE);
	SerialPRINT(F("RAM_End              : "));
	SerialPRINT2ln(Cfg.ro.RAM_End, PRINT_MODE);
	SerialPRINT(F("FreeRAM              : "));
	SerialPRINTln(Cfg.ro.FreeRAM);
	SerialPRINT(F("BlinkingLED          : "));
	SerialPRINTln(Cfg.rw.BlinkingLED);
	SerialPRINT(F("IdleLED              : "));
	SerialPRINTln(Cfg.rw.IdleLED);
	SerialPRINT(F("TimeSharingEnable    : "));
	SerialPRINTln(Cfg.rw.TimeSharingEnabled);

	SerialPRINTln(F("=========== KERNEL CONFIGURATION PRINT end =================="));

	if (Inited)
		ExitCritRegion();		// Allow rescheduling....

	return(E_SUCCESS);
}


#if uMT_USE_TIMERS==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Flags2String - ARDUINO
//
/////////////////////////////////////////////////////////////////////////////////////////////////
const __FlashStringHelper *uTimer::Flags2String()
{
	switch (Flags)
	{
	case (uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT | uMT_TM_REPEAT):	return(F("uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT | uMT_TM_REPEAT")); break;
	case (uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT):					return(F("uMT_TM_IAM_AGENT | uMT_TM_SEND_EVENT")); break;
	case (uMT_TM_IAM_AGENT | uMT_TM_REPEAT):						return(F("uMT_TM_IAM_AGENT | uMT_TM_REPEAT")); break;
	case uMT_TM_IAM_TASK:											return(F("uMT_TM_IAM_TASK")); break;
	default:														return(F("???")); break;
	}
};
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	TaskStatus2String - ARDUINO
//
/////////////////////////////////////////////////////////////////////////////////////////////////
const __FlashStringHelper *uTask::TaskStatus2String(Status_t TaskStatus)
	{
	switch (TaskStatus)
	{
	case S_UNUSED:		return(F("S_UNUSED   ")); break;
	case S_CREATED:		return(F("S_CREATED  ")); break;
	case S_READY:		return(F("S_READY    ")); break;
	case S_RUNNING:		return(F("S_RUNNING  ")); break;
	case S_SBLOCKED:	return(F("S_SBLOCKED ")); break;
	case S_EBLOCKED:	return(F("S_EBLOCKED ")); break;
	case S_TBLOCKED:	return(F("S_TBLOCKED ")); break;
	case S_SUSPENDED:	return(F("S_SUSPENDED")); break;
	default:			return(F("invalid    ")); break;
	}
};



#ifdef ZAPPED
// Print a string from Program Memory directly to save RAM 
void printProgStr (const char * str)
{
	char c;

	if (!str) 
		return;

	while ((c = pgm_read_byte(str++)))
		SerialPRINT (c);
} // end of printProgStr
#endif




#if uMT_USE_EVENTS==1


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	EventFlag2String - ARDUINO
//
/////////////////////////////////////////////////////////////////////////////////////////////////
const __FlashStringHelper *uMT::EventFlag2String(uMToptions_t EventFlag)
	{
	switch (EventFlag)
	{
	case uMT_NULL_OPT:	return(F("uMT_NULL_OPT")); break;
	case uMT_WAIT:		return(F("uMT_WAIT")); break;
	case uMT_NOWAIT:	return(F("uMT_NOWAIT")); break;
	case uMT_ANY:		return(F("uMT_ANY")); break;
	default:			return(F("invalid")); break;

	}
};
#endif

#else

Errno_t	uMT::Kn_PrintInternals()
{
	return(E_SUCCESS);
}
#endif



////////////////////////////////////////////////////////////////////////////////////
//
//	uMTdoTicksWork
//
// Return 1 is a Reschedule is needed
////////////////////////////////////////////////////////////////////////////////////
unsigned uMTdoTicksWork()
{
	int	ForceReschedule = 0;		// Assume NO

	// Decrement slice counter
	Kernel.TimeSlice--;

	// Can we be preempted?
	if (Kernel.NoPreempt == TRUE)
		return(0);		// NO

#if LEGACY_CRIT_REGIONS==1
	// Can we be rescheduled?
	if (Kernel.NoResched != 0)
		return(0);		// NO
#endif

#if uMT_USE_TIMERS==1
	// Check if some ALARM is expired
	if (Kernel.TimerQueue != NULL)
	{
		if (Kernel.TimerQueue->NextAlarm <= Kernel.msTickCounter)
		{
			Kernel.AlarmExpired = TRUE;
			ForceReschedule = 1;
		}
	}
#endif

	if (Kernel.NeedResched)
	{
		ForceReschedule = 1;
	}
	else
	{
		if (Kernel.kernelCfg.TimeSharingEnabled == TRUE)
		{
			if (Kernel.TimeSlice <= 0)
			{
				if (Kernel.ReadyQueue.Head != NULL)
					ForceReschedule = 1;
				else
					Kernel.TimeSlice = (Kernel.Running == Kernel.IdleTaskPtr ? uMT_IDLE_TIMEOUTVALUE : uMT_TICKS_TIMESHARING); // Reload
			}
		}
	}

	if (ForceReschedule == 0)
	{
		// Check for higher priority tasks
		// NoPreempt & NoResched already checked before...
		if (Kernel.ReadyQueue.Head != NULL && Kernel.ReadyQueue.Head->Priority > Kernel.Running->Priority)
		{
			ForceReschedule = 1;
		}
	}

	return(ForceReschedule);
}



////////////////////// EOF