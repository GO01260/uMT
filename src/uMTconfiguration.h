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
#define uMT_USE_EVENTS				1			// Use Events
#define uMT_USE_SEMAPHORES			1			// Use Semaphores
#define uMT_USE_TIMERS				1			// Use Timers
#define uMT_USE_RESTARTTASK			1			// Use tk_Restart()
#define uMT_USE_PRINT_INTERNALS		1			// Setting to 0 can save 26 bytes...
#define uMT_USE_MALLOC_REENTRANT	1			// malloc() and free() re-entrant using lock/unlock
#define uMT_USE_TASK_STATISTICS		2			// 1=count the number of times a task has become S_RUNNING, 2=1+measure execution time 


////////////////////////////////////////////////////////////////////////////////////
//
//	DEBUGGING CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_SAFERUN			1		// Unless you need to save some memory, do NOT disable...
#define uMT_IDLE_TIMEOUT	1		// If set, this triggers some special action for IDLE task (e.g., PrintInternals()


/////////////////////////////////////////////////////////////////////////////////////////////////
//					STACK allocation
/////////////////////////////////////////////////////////////////////////////////////////////////
// Stack allocation area is critical when malloc() is used in the Arduino environment.
// Arduino malloc() is allocating using free RAM between the end of (initialized + unitialized) DATA 
// and the Arduino loop() Stack Pointer. To define upper memory limit, malloc() is using the caller task's
// stack pointer value to determine this value. If tasks' stacks are allocated statically, they are located
// below the end of (initialized + unitialized) DATA segment. If one of this task is calling malloc(), 
// this will fail because SP < Bottom_Free_Memory.
//
// For this reason the uMT_ALLOCATION_TYPE configuration parameter is used to control where
// to allocate stacks:
//	0 - fixed size, allocated statically. This is used in Arduino UNO board because the 2KB RAM limit
//		makes not effective the utilization of malloc().
//	1 -	fixed size allocated in high memory with malloc(). With this configuration malloc() is NOT failing.
//	2 -	variale size allocated with uMTmalloc(). This internal version of the memory allocator is reserving all the
//		available memory and managing malloc() requests internally.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#define	uMT_FIXED_STATIC		0
#define	uMT_FIXED_DYNAMIC		1
#define	uMT_VARIABLE_DYNAMIC	2		// Not implemented yet


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT GENERIC CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////

#define uMT_MIN_TASK_NUM		4		// MIN task number (cannot be less than 4!!!)
#define uMT_MAX_TASK_NUM		128
#define uMT_MAX_EVENTS_NUM		32		// How many Events per task


#define uMT_MIN_SEM_NUM		uMT_MIN_TASK_NUM	// MIN number of SEMAPHORES (one for each Task)
#define uMT_MAX_SEM_NUM		uMT_MAX_TASK_NUM	// MAX number of SEMAPHORES (one for each Task)


#ifdef WIN32

#define uMT_ALLOCATION_TYPE	uMT_VARIABLE_DYNAMIC

#define uMT_DEFAULT_TASK_NUM		10		// Max task number (cannot be less than 4!!!)
#define uMT_DEFAULT_SEM_NUM			16		// Max number of Semaphores
#define uMT_DEFAULT_EVENTS_NUM		16		// How many Events per task

#define uMT_MIN_STACK_SIZE			64		// MIN new application STACK size
#define uMT_MAX_STACK_SIZE			512		// MAX new application STACK size
#define uMT_DEFAULT_STACK_SIZE		256		// DEFAULT new application STACK size

#define uMT_MIN_IDLE_STACK_SIZE		64		// MIN IDLE task STACK size
#define uMT_MAX_IDLE_STACK_SIZE		128		// MAX IDLE task STACK size
#define uMT_DEFAULT_IDLE_STACK_SIZE	128		// DEFAULT IDLE task STACK size

#define uMT_MIN_TID1_STACK_SIZE		uMT_MIN_STACK_SIZE	// MIN STACK size for Arduino loop() task
#define uMT_MAX_TID1_STACK_SIZE		uMT_MAX_STACK_SIZE	// MAX STACK size for Arduino loop() task
#define uMT_DEFAULT_TID1_STACK_SIZE	uMT_DEFAULT_STACK_SIZE	// DEFAULT STACK size for Arduino loop() task

#define malloc(x)			x
#define free(x)				
#define micros()	0

#elif defined(ARDUINO_ARCH_SAM) // ARDUINO DUE  //////////////////////////////////////////////////////////////////////////

#define uMT_ALLOCATION_TYPE	uMT_VARIABLE_DYNAMIC

#define uMT_DEFAULT_TASK_NUM		20		// Max task number (cannot be less than 4!!!)
#define uMT_DEFAULT_SEM_NUM			32		// Max number of Semaphores
#define uMT_DEFAULT_EVENTS_NUM		32		// How many Events per task

#define uMT_MIN_STACK_SIZE			256		// MIN new application STACK size
#define uMT_MAX_STACK_SIZE			4096	// MAX new application STACK size
#define uMT_DEFAULT_STACK_SIZE		1024	// DEFAULT new application STACK size

#define uMT_MIN_IDLE_STACK_SIZE		256		// MIN STACK size
#define uMT_MAX_IDLE_STACK_SIZE		1024	// MAX IDLE task STACK size
#define uMT_DEFAULT_IDLE_STACK_SIZE	512		// DEFAULT IDLE task STACK size

#define uMT_MIN_TID1_STACK_SIZE		uMT_MIN_STACK_SIZE	// MIN STACK size for Arduino loop() task
#define uMT_MAX_TID1_STACK_SIZE		uMT_MAX_STACK_SIZE	// MAX STACK size for Arduino loop() task
#define uMT_DEFAULT_TID1_STACK_SIZE	uMT_DEFAULT_STACK_SIZE	// DEFAULT STACK size for Arduino loop() task
#define uMT_KERNEL_STACK_SIZE		256		// uMT Kernel STACK size

#elif defined(ARDUINO_ARCH_SAMD) // ARDUINO ZERO //////////////////////////////////////////////////////////////////////////

#define uMT_ALLOCATION_TYPE	uMT_VARIABLE_DYNAMIC

#define uMT_DEFAULT_TASK_NUM		15		// Max task number (cannot be less than 4!!!)
#define uMT_DEFAULT_SEM_NUM			32		// Max number of Semaphores
#define uMT_DEFAULT_EVENTS_NUM		32		// How many Events per task

#define uMT_MIN_STACK_SIZE			256		// MIN new application STACK size
#define uMT_MAX_STACK_SIZE			4096	// MAX new application STACK size
#define uMT_DEFAULT_STACK_SIZE		1024	// DEFAULT new application STACK size

#define uMT_MIN_IDLE_STACK_SIZE		256		// MIN STACK size
#define uMT_MAX_IDLE_STACK_SIZE		1024	// MAX IDLE task STACK size
#define uMT_DEFAULT_IDLE_STACK_SIZE	512		// DEFAULT IDLE task STACK size

#define uMT_MIN_TID1_STACK_SIZE		uMT_MIN_STACK_SIZE	// MIN STACK size for Arduino loop() task
#define uMT_MAX_TID1_STACK_SIZE		uMT_MAX_STACK_SIZE	// MAX STACK size for Arduino loop() task
#define uMT_DEFAULT_TID1_STACK_SIZE	uMT_DEFAULT_STACK_SIZE	// DEFAULT STACK size for Arduino loop() task
#define uMT_KERNEL_STACK_SIZE		256		// uMT Kernel STACK size

#elif defined(__AVR_ATmega2560__)	// ARDUINO MEGA //////////////////////////////////////////////////////////////////////////

#define uMT_ALLOCATION_TYPE	uMT_VARIABLE_DYNAMIC

#define uMT_DEFAULT_TASK_NUM		10		// Max task number (cannot be less than 4!!!)
#define uMT_DEFAULT_SEM_NUM			16		// Max number of Semaphores
#define uMT_DEFAULT_EVENTS_NUM		16		// How many Events per task

#define uMT_MIN_STACK_SIZE			64		// MIN new application STACK size
#define uMT_MAX_STACK_SIZE			292		// MAX new application STACK size
#define uMT_DEFAULT_STACK_SIZE		256		// DEFAULT new application STACK size

#define uMT_MIN_IDLE_STACK_SIZE		96		// MIN STACK size
#define uMT_MAX_IDLE_STACK_SIZE		256		// MAX IDLE task STACK size
#define uMT_DEFAULT_IDLE_STACK_SIZE	128		// DEFAULT IDLE task STACK size

#define uMT_MIN_TID1_STACK_SIZE		uMT_MIN_STACK_SIZE	// MIN STACK size for Arduino loop() task
#define uMT_MAX_TID1_STACK_SIZE		uMT_MAX_STACK_SIZE	// MAX STACK size for Arduino loop() task
#define uMT_DEFAULT_TID1_STACK_SIZE	uMT_DEFAULT_STACK_SIZE	// DEFAULT STACK size for Arduino loop() task
#define uMT_KERNEL_STACK_SIZE		64		// uMT Kernel STACK size


#else		// ARDUINO UNO //////////////////////////////////////////////////////////////////////////

// The following cannot be reconfigured from "xxx.INO"...
#define uMT_ALLOCATION_TYPE	uMT_FIXED_STATIC

#define uMT_DEFAULT_TASK_NUM		5		// Max task number (cannot be less than 4!!!)
#define uMT_DEFAULT_SEM_NUM			8		// Max number of Semaphores
#define uMT_DEFAULT_EVENTS_NUM		16		// How many Events per task

#define uMT_MIN_STACK_SIZE			64		// MIN new application STACK size
#define uMT_MAX_STACK_SIZE			256		// MAX new application STACK size
#define uMT_DEFAULT_STACK_SIZE		200		// DEFAULT new application STACK size

#define uMT_MIN_IDLE_STACK_SIZE		96		// MIN STACK size
#define uMT_MAX_IDLE_STACK_SIZE		128		// MAX IDLE task STACK size
#define uMT_DEFAULT_IDLE_STACK_SIZE	128		// DEFAULT IDLE task STACK size

#define uMT_MIN_TID1_STACK_SIZE		uMT_MIN_STACK_SIZE	// MIN STACK size for Arduino loop() task
#define uMT_MAX_TID1_STACK_SIZE		uMT_MAX_STACK_SIZE	// MAX STACK size for Arduino loop() task
#define uMT_DEFAULT_TID1_STACK_SIZE	uMT_DEFAULT_STACK_SIZE	// DEFAULT STACK size for Arduino loop() task

// The following cannot be reconfigured from "xxx.INO" ...
#undef uMT_USE_RESTARTTASK
#define uMT_USE_RESTARTTASK		0		// tk_Restart() not enabled for ARDUINO UNO

#undef uMT_USE_TASK_STATISTICS
#define uMT_USE_TASK_STATISTICS	0		// Save memory...

#endif	

#ifndef uMT_DEFAULT_TIMER_AGENT_NUM
#define uMT_MIN_TIMER_AGENT_NUM		(uMT_DEFAULT_TASK_NUM / 2)	// MIN number of AGENT Timers
#define uMT_MAX_TIMER_AGENT_NUM		(uMT_DEFAULT_TASK_NUM * 2)	// MAX number of AGENT Timers
#define uMT_DEFAULT_TIMER_AGENT_NUM	uMT_DEFAULT_TASK_NUM		// DEFAULT number of AGENT Timers (one for each Task)
#endif


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT SYSTEM TICK CONFIGURATIONS
//
////////////////////////////////////////////////////////////////////////////////////

#define uMT_USE_WTD_4_TICKS		0			// To use WDT for timer ticks... (AVR only)
#define uMT_USE_TIMER0_4_TICKS	1			// To use TIMER 0 for timer ticks...

#if uMT_USE_WTD_4_TICKS==1
#define uMT_TICKS_SECONDS		8					// Depending on HW capability...
#endif

#if uMT_USE_TIMER0_4_TICKS==1
#define uMT_TICKS_SECONDS		1000					// Standard Arduino
#endif

#define uMT_TICKS_TIMESHARING	uMT_TICKS_SECONDS		// 1 second
#define uMT_IDLE_TIMEOUTVALUE	(10*uMT_TICKS_SECONDS)	// 10 second


#define	CLIB_SEM		0		// Semaphore #0 used to make C library re-entrant [malloc()/free()]

///////////////////////////////////////////////////////////////////////////////////
//
//	KERNEL Version
//
////////////////////////////////////////////////////////////////////////////////////
#define uMT_VERSION_NUMBER		0x0260	// Version 2.6.0 (version x.y.z: X = 8 bits, Y and Z = 4 bits)



/////////////////////////// EOF