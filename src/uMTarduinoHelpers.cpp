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


#include <Arduino.h>

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
	Serial.println(F("================= uMT Kernel: BadExit() called! ================="));
	Serial.flush();

	Kernel.iKn_FatalError();

#ifdef ZAPPED
	// Delete task
	Kernel.Tk_DeleteTask(Kernel.Running->myTid);

//	Kernel.Running->TaskStatus = S_ZOMBIE;

//	Kernel.Reschedule();

	// It never returns...
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iKn_FatalError - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::iKn_FatalError()
{
	NoPreempt = TRUE;		// Prevent rescheduling....

	Serial.println(F("========= iKn_FatalError ==========="));
	Serial.flush();

	Kn_PrintInternals();

	iKn_Reboot();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iKn_FatalError - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::iKn_FatalError(const __FlashStringHelper *String)
{
	NoPreempt = TRUE;		// Prevent rescheduling....

	Serial.print(F("========= iKn_FatalError ("));
	Serial.print(String);
	Serial.println(F(")==========="));
	Serial.flush();

	Kn_PrintInternals();

	iKn_Reboot();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iMTDummyLoop - ARDUINO_UNO
//
// The IDLE task...
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::IdleLoop()
{
	while (1)			// Do nothing....
	{
		if (Kernel.BlinkingLED)
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
			Kernel.iKn_FatalError(F("No active tasks in the system (all deleted?)"));
		}
	}
}

		

#if uMT_SAFERUN==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckTaskMagic - ARDUINO_UNO
//
// CheckTaskMagic
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::CheckTaskMagic(uTask *task, const __FlashStringHelper *String)
{
	if (task->magic != uMT_TASK_MAGIC)
	{
		Serial.print(F("uMT: CheckTaskMagic() invalid MAGIC number in TASK structure, Func="));
		Serial.println(String);
		Serial.flush();

		iKn_FatalError();
	}

	StackPtr_t MaxSP = (task->StackBaseAddr + task->StackSize - 1);

	if (task->SavedSP <  task->StackBaseAddr || task->SavedSP > MaxSP)
	{
		Serial.print(F("uMT: CheckTaskMagic(Tid="));
		Serial.print((unsigned int)task->myTid);

		Serial.print(F("): Invalid SP = "));
		Serial.print((unsigned int)task->SavedSP);

		Serial.print(F("(MIN SP = "));
		Serial.print((unsigned int)task->StackBaseAddr);

		Serial.print(F(" MAX SP = "));
		Serial.print((unsigned int)MaxSP);
		Serial.println(F(")"));

		Serial.flush();

		iKn_FatalError();
	}
	

#if uMT_DEBUG==1
	Serial.print(F("uMT: CheckTaskMagic: "));
	Serial.print(String);
	Serial.print(F(": TID = "));
	Serial.print((unsigned int)task->myTid);
	Serial.print(F(" - SP = "));
	Serial.print((unsigned int)task->SavedSP);
	Serial.print(F(" - Priority = "));
	Serial.println(task->Priority);
	Serial.flush();
#endif

}

#if uMT_USE_TIMERS==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckTimerMagic - ARDUINO_UNO
//
// CheckTimerMagic
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::CheckTimerMagic(uTimer *timer, const __FlashStringHelper *String)
{
	if (timer->magic != uMT_TIMER_MAGIC)
	{
		Serial.print(F("uMT: CheckTimerMagic() invalid MAGIC number in TIMER structure, Func="));
		Serial.println(String);
		Serial.flush();

		iKn_FatalError();
	}
}
#endif


#endif



#if uMT_USE_PRINT_INTERNALS==1

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_PrintInternals - ARDUINO_UNO/MEGA
//
// It printf internal Kernel structures for debugging ppurposes
//
/////////////////////////////////////////////////////////////////////////////////////////////////
Errno_t	uMT::Kn_PrintInternals()
{
	if (Inited == FALSE)
		return(E_NOT_INITED);

	NoResched++;		// Prevent rescheduling....

	// Print Kernel info
	Serial.println(F("=========== KERNEL INFO start =================="));

	Serial.print(F("ActiveTaskNo="));
	Serial.print(ActiveTaskNo);

	Serial.print(F(" TimeSharingEnabled="));
	Serial.print(TimeSharingEnabled);

	Serial.print(F(" NoPreempt="));
	Serial.print(NoPreempt);

	Serial.print(F(" NoResched="));
	Serial.print(NoResched);

	Serial.print(F(" NeedResched="));
	Serial.print(NeedResched);

	Serial.print(F(" TickCounter.High="));
	Serial.print(TickCounter.High);

	Serial.print(F(" TickCounter.Low="));
	Serial.print(TickCounter.Low);

	Serial.println(F(""));

	uTask *pTask;

	// Print all tasks
	Serial.println(F("=========== RAW TASK LIST =================="));

	for (int idx = 0; idx < uMT_MAX_TASK_NUM; idx++)
	{
		pTask = &TaskList[idx];

		Serial.print(F("<idx="));
		Serial.print(idx);

		Serial.print(F(" Tid="));
		Serial.print(pTask->myTid);

		Serial.print(F(" Status="));
		Serial.print(pTask->TaskStatus2String());

		Serial.print(F(" Prio="));
		Serial.print(pTask->Priority);

		Serial.print(F(" SP="));
		Serial.print((unsigned int)pTask->SavedSP);

		Serial.print(F(" SPbase="));
		Serial.print((unsigned int)pTask->StackBaseAddr);

		Serial.print(F(" SPsize="));
		Serial.print(pTask->StackSize);

		Serial.print(F(" SPfree="));
		Serial.print((unsigned int)pTask->SavedSP - (unsigned int)pTask->StackBaseAddr);

#if	uMT_USE_EVENTS==1
		Serial.print(F(" EV_recv=0x"));
		Serial.print(pTask->EV_received, HEX);

		Serial.print(F(" EV_req=0x"));
		Serial.print(pTask->EV_requested, HEX);

		Serial.print(F(" EV_cond="));
		Serial.print(EventFlag2String(pTask->EV_condition));
#endif


#if uMT_SAFERUN==1
		Serial.print(F(" magic=0x"));
		Serial.print(pTask->magic, HEX);
#endif

		Serial.println(F(">"));
	}


	// Print ReadyQueue
	pTask = ReadyQueue.Head;
	Serial.println(F("=========== ReadyQueue =================="));

	while (pTask != NULL)
	{

		Serial.print(F(" Tid="));
		Serial.print(pTask->myTid);

		Serial.print(F(" Status="));
		Serial.print(pTask->TaskStatus2String());

#if uMT_SAFERUN==1
		Serial.print(F(" magic=0x"));
		Serial.print(pTask->magic, HEX);
#endif

		Serial.println(F(">"));

		pTask = pTask->Next;
	}


	// Print SemQueue
//	Serial.println(F("=========== SemQueue =================="));


	for (int idx = 0; idx < uMT_MAX_SEM_NUM; idx++)
	{
		pTask = SemList[idx].SemQueue.Head;

		if (SemList[idx].SemValue > 0 || pTask != NULL)
		{
			Serial.print(F("=========== SemQueue ("));
			Serial.print(idx);
			Serial.print(F(") SemVal="));
			Serial.print(SemList[idx].SemValue);
			Serial.println(F(") =================="));
		}


		if (pTask == NULL)
		{
			continue;		// Next queue
		}

		while (pTask != NULL)
		{

			Serial.print(F(" Tid="));
			Serial.print(pTask->myTid);

			Serial.print(F(" Status="));
			Serial.print(pTask->TaskStatus2String());

	#if uMT_SAFERUN==1
			Serial.print(F(" magic=0X"));
			Serial.print(pTask->magic, HEX);
	#endif

			Serial.println(F(">"));

			pTask = pTask->Next;
		}
	}

	

#if uMT_USE_TIMERS==1

	// Print TimerQueue
	uTimer *pTimer = TimerQueue;

	Serial.print(F("=========== TimerQueue (total="));
	Serial.print(TotTimerQueued);
	Serial.println(F(") =================="));

	while (pTimer != NULL)
	{

		Serial.print(F(" TimerId="));
		Serial.print(pTimer->myTimerId);

		Serial.print(F(" Flags=0X"));
		Serial.print(pTimer->Flags, HEX);

		Serial.print(F(" ("));
		Serial.print(pTimer->Flags2String());

		Serial.print(F(") TaskId="));
		Serial.print(pTimer->pTask->myTid);

		Serial.print(F(" NextAlarm.High="));
		Serial.print(pTimer->NextAlarm.High);

		Serial.print(F(" NextAlarm.Low="));
		Serial.print(pTimer->NextAlarm.Low);

		Serial.print(F(" Timeout="));
		Serial.print(pTimer->Timeout);

#if	uMT_USE_EVENTS==1
		Serial.print(F(" Event=0x"));
		Serial.print(pTimer->EventToSend, HEX);
#endif

#if uMT_SAFERUN==1
		Serial.print(F(" magic=0X"));
		Serial.print(pTimer->magic, HEX);

		Serial.print(F(" TaskMagic=0X"));
		Serial.print(pTimer->pTask->magic, HEX);
#endif

		Serial.println(F(">"));

		pTimer = pTimer->Next;
	}
#endif


	Serial.println(F("=========== KERNEL INFO end =================="));

	NoResched--;		// Allow rescheduling....

	return(E_SUCCESS);
}


#if uMT_USE_TIMERS==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Flags2String - ARDUINO_UNO/MEGA
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
//	TaskStatus2String - ARDUINO_UNO/MEGA
//
/////////////////////////////////////////////////////////////////////////////////////////////////
const __FlashStringHelper *uTask::TaskStatus2String()
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
		Serial.print (c);
} // end of printProgStr
#endif




#if uMT_USE_EVENTS==1


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	EventFlag2String - ARDUINO_UNO/MEGA
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

////////////////////// EOF