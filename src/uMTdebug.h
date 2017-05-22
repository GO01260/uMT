////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTdebug.H
//	AUTHOR: Antonio Pastore - March 2017
//	Program originally written by Antonio Pastore, Torino, ITALY.
//	UPDATED: 27 April 2017
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


#ifndef uMT_DEBUG
#define uMT_DEBUG			0		// Internal debugging, do NOT enable unless you know what you are doing...
#endif

#ifdef WIN32

#define SerialPrint(x)
#define SerialPrintln(x)
#define SPrF(x)  
#define SPrLF(x) 


#define DgbStringPrint(x)
#define DgbStringPrintLN(x)
#define DgbValuePrint(x)
#define DgbValuePrintLN(x)
#define DgbValuePrint2(x, y)
#define DgbValuePrint2LN(x, y)

#define	DebugWait(x)

//#define SPrF(x)  
//#define SPrLF(x)

#define CHECK_TASK_MAGIC(task, message) { if (task->magic != uMT_TASK_MAGIC) Kernel.isrKn_FatalError(); }
#define CHECK_TIMER_MAGIC(timer, message) { if (timer->magic != uMT_TIMER_MAGIC) Kernel.isrKn_FatalError(); }
#define CHECK_INTS(message)



#else		/// WIN32

#include "Arduino.h"


//#include <avr/wdt.h>

#define SerialPrint(x)		Serial.print(x)
#define SerialPrintln(x)	Serial.println(x)

#define SPrF(x)  Serial.print(F(x))                        // short for F-Macro Serial.print
#define SPrLF(x) Serial.println(F(x))                       // short for F-Macro Serial.println

#if uMT_DEBUG==1

#define DgbStringPrint(x)	{ SPrF(x); Serial.flush(); }
#define DgbStringPrintLN(x)	{ SPrLF(x); Serial.flush(); }
#define DgbValuePrint(x)	{ Serial.print(x); Serial.flush(); }
#define DgbValuePrintLN(x)	{ Serial.println(x); Serial.flush(); }
#define DgbValuePrint2(x, y)	{ Serial.print(x, y); Serial.flush(); }
#define DgbValuePrint2LN(x, y)	{ Serial.println(x, y); Serial.flush(); }

#define	DebugWait(x)			delay(x)

#else

#define DgbStringPrint(x)
#define DgbStringPrintLN(x)
#define DgbValuePrint(x)
#define DgbValuePrintLN(x)
#define DgbValuePrint2(x, y)
#define DgbValuePrint2LN(x, y)
#define	DebugWait(x)

#endif

#if uMT_SAFERUN==1
#define CHECK_TASK_MAGIC(task, message)	Kernel.CheckTaskMagic(task, F(message))
#define CHECK_TIMER_MAGIC(timer, message) Kernel.CheckTimerMagic(timer, F(message))
#define CHECK_INTS(message)	Kernel.CheckInterrupts(F(message))

#else
#define CHECK_TASK_MAGIC(task, message)
#define CHECK_TIMER_MAGIC(timer, message)
#define CHECK_INTS(message)
#endif

#endif

/////////////////////////// EOF