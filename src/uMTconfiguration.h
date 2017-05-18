////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTconfiguration.H
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


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT MODULES CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_USE_EVENTS			1			// Use Events
#define uMT_USE_SEMAPHORES		1			// Use Semaphores
#define uMT_USE_TIMERS			1			// Use Timers
#define uMT_USE_RESTARTTASK		1			// Use tk_Restart()
#define uMT_USE_PRINT_INTERNALS	1			// Setting to 0 can save 26 bytes...


////////////////////////////////////////////////////////////////////////////////////
//
//	DEBUGGING CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_SAFERUN			1		// Unless you need to save some memory, do NOT disable...
#define uMT_IDLE_TIMEOUT	1		// If set, this triggers some special action for IDLE task (e.g., PrintInternals()

////////////////////////////////////////////////////////////////////////////////////
//
//	uMT GENERIC CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////


#ifdef WIN32

#define uMT_MAX_TASK_NUM		10			// Max task number (cannot be less than 3!!!)
#define uMT_MAX_SEM_NUM			16			// Max number of Semaphores
#define uMT_STACK_SIZE			256			// STACK size
#define uMT_IDLE_STACK_SIZE		128			// IDLE task STACK size
#define uMT_STATIC_STACKS		1			// Where to allocate tasks' stacks: STATIC or MALLOC() in high memory

#else

#if defined(ARDUINO_ARCH_SAM)

#define uMT_STATIC_STACKS		0			// Where to allocate tasks' stacks: STATIC or MALLOC() in high memory

#define uMT_MAX_TASK_NUM		20			// Max task number (cannot be less than 3!!!)
#define uMT_MAX_SEM_NUM			32			// Max number of Semaphores
#define uMT_STACK_SIZE			1024		// STACK size
#define uMT_TID1_STACK_SIZE		uMT_STACK_SIZE	// STACK size for Arduino loop() task
#define uMT_IDLE_STACK_SIZE		1024		// IDLE task STACK size

#else

#if defined(__AVR_ATmega2560__)	// ARDUINO MEGA

#define uMT_STATIC_STACKS		0			// Where to allocate tasks' stacks: STATIC or MALLOC() in high memory

#define uMT_MAX_TASK_NUM		10			// Max task number (cannot be less than 3!!!)
#define uMT_MAX_SEM_NUM			16			// Max number of Semaphores
#define uMT_STACK_SIZE			256			// STACK size
#define uMT_TID1_STACK_SIZE		uMT_STACK_SIZE	// STACK size for Arduino loop() task
#define uMT_IDLE_STACK_SIZE		96			// IDLE task STACK size


#else		// ARDUINO UNO

#define uMT_STATIC_STACKS		1			// Where to allocate tasks' stacks: STATIC or MALLOC() in high memory

#define uMT_MAX_TASK_NUM		5			// Max task number (cannot be less than 3!!!)
#define uMT_MAX_SEM_NUM			8			// Max number of Semaphores
#define uMT_STACK_SIZE			230			// STACK size
#define uMT_TID1_STACK_SIZE		uMT_STACK_SIZE	// STACK size for Arduino loop() task
#define uMT_IDLE_STACK_SIZE		96			// IDLE task STACK size


#undef uMT_USE_RESTARTTASK
#define uMT_USE_RESTARTTASK		0			// tk_Restart() not enabled for ARDUINO UNO

#undef uMT_USE_TIMERS
#define uMT_USE_TIMERS			0			// Don't use Timers

#endif		// ATmega2560
#endif		// SAM architecture
#endif		// WIN32


#define uMT_MAX_TIMER_AGENT_NUM	uMT_MAX_TASK_NUM		// Max number of AGENT Timers (one for each Task)


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT SYSTEM TICK CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////

#define uMT_USE_WTD_4_TICKS		0			// To use WDT for timer ticks...
#define uMT_USE_TIMER0_4_TICKS	1			// To use TIMER 0 for timer ticks...

#if uMT_USE_WTD_4_TICKS==1
#define uMT_TICKS_SECONDS		8					// Depending on HW capability...
#define uMT_TICKS_TIMESHARING	uMT_TICKS_SECONDS		// 1 second
#define uMT_IDLE_TIMEOUTVALUE	(10*uMT_TICKS_SECONDS)	// 10 second
#endif

#if uMT_USE_TIMER0_4_TICKS==1
#define uMT_TICKS_SECONDS		1000					// Standard Arduino
#define uMT_TICKS_TIMESHARING	uMT_TICKS_SECONDS		// 1 second
#define uMT_IDLE_TIMEOUTVALUE	(10*uMT_TICKS_SECONDS)	// 10 second
#endif


///////////////////////////////////////////////////////////////////////////////////
//
//	KERNEL Version
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_VERSION_NUMBER		0x0102	// Version x.y


/////////////////////////// EOF