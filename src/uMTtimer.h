////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTtimer.h
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

#ifndef uMT_TIMER_H
#define uMT_TIMER_H



#if	uMT_USE_TIMERS==1

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT  TIMERS 
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_TIMER_MAGIC			0xADDE	// Magic value...

//#define uMT_TM_NULL_OPT		0x00			// Timers 
// Permanent FLAGS
#define uMT_TM_IAM_AGENT	0x01			// I am an AGENT
#define uMT_TM_IAM_TASK		0x02			// I am a TASK

#define uMT_TM_SEND_EVENT	0x10			// Timers, send EVENT
#define uMT_TM_REPEAT		0x20			// Timers, repeat alarm
#define uMT_TM_EXPIRED		0x40			// Timer EXPIRED

typedef uint8_t			TimerFlag_t;		// Timers flags type, 8 bits

class uTask;		// Fromarw declaration

class uTimer
{
	/////////////////////////////////
	// FRIENDS!!!
	/////////////////////////////////
	friend unsigned uMTdoTicksWork();	// uMTarduinoSysTick.cpp
	friend void uMT_SystemTicks();		// uMTarduinoSysTick.cpp
	friend unsigned int sysTickHook();	// uMTarduinoSysTick.cpp

	friend class uTask;
	friend class uMT;

#if uMT_SAFERUN==1
	uint16_t	magic;			// To check consistency
#endif
	TimerId_t	myTimerId;		// My Timer ID, helper

	uTimer		*Next;			// Next in the queue
//	uTimer		*Prev;			// Prev in the queue

	uMTextendedTime	NextAlarm;		// Absolute value in ticks for next alarm
	Timer_t		Timeout;		// Alarm Value (for repetitive alarms)

	TimerFlag_t	Flags;			// uMT_TM_SEND_EVENT, uMT_TM_REPEAT, uMT_TM_IAM_AGENT, uMT_TM_IAM_TASK
	Event_t		EventToSend;	// Event to send if uMT_TM_SEND_EVENT is set
	uTask		*pTask;			// Task related to this timer

// Methods
	const __FlashStringHelper *Flags2String();

	void Init(unsigned int idx, TimerFlag_t _flags, uTask *_pTask)
	{
		Flags = _flags;
		NextAlarm = 0;
		Timeout = 0;
		myTimerId.Init(idx);	// Initial value, TimeStamp it will be overwritten by TimerQ_PopFree()
		magic = uMT_TIMER_MAGIC;
		pTask = _pTask;
		EventToSend = uMT_NULL_EVENT;
	};

};

#endif




#endif




/////////////////////////////////////// EOF

