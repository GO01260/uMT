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


#ifndef uMT_ERRNO_H
#define uMT_ERRNO_H

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
/* 08 */ E_INVALID_MAX_TASK_NUM,	// Not enough TASK entries configured in the Kernel (Kn_Start) or too many
/* 09 */ E_INVALID_SEMID,			// Invalid SEMAPHORE Id
/* 10 */ E_INVALID_TIMEOUT,			// Invalid timeout (zero or too large)
/* 11 */ E_OVERFLOW_SEM,			// Semaphore counter overflow
/* 12 */ E_NOMORE_TIMERS,			// No more Timers available
/* 13 */ E_NOT_OWNED_TIMER,			// TIMER is not owned by this task
/* 14 */ E_TASK_NOT_STARTED,		// TASK not started, cannot ReStartTask()
/* 15 */ E_TIMEOUT,					// Timeout
/* 16 */ E_NOT_ALLOWED,				// Not allowed [Tk_ReStartTask() suicide]
/* 17 */ E_INVALID_OPTION,			// Invalid additional option
/* 18 */ E_NO_MORE_MEMORY,			// No more memory available [Tk_CreateTask()]
/* 19 */ E_INVALID_STACK_SIZE,		// Invalid STACK size [Tk_CreateTask()]
/* 20 */ E_INVALID_MAX_TIMER_NUM,	// Invalid max Timer number [Kn_start()]
/* 21 */ E_INVALID_MAX_SEM_NUM		// Invalid max Semaphore number [Kn_start()]
};


#endif

////////////// EOF