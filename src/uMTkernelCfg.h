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

#ifndef uMTKernelCfg_H
#define uMTKernelCfg_H

///////////////////////////////////////////////////////////////////////////////////
//
//	uMT CONFIGURATION
//
//	Returned with Kn_GetConfiguration()
//
////////////////////////////////////////////////////////////////////////////////////

class roCfg
{
public:
	Bool_t		Use_Events;				// Readonly
	Bool_t		Use_Semaphores;			// Readonly
	Bool_t		Use_Timers;				// Readonly
	Bool_t		Use_RestartTask;		// Readonly
	Bool_t		Use_PrintInternals;		// Readonly
	Cfg_data_t	Events_Num;				// Readonly

	void Init()
	{
		Use_Events			= uMT_USE_EVENTS;
		Use_Semaphores		= uMT_USE_SEMAPHORES;
		Use_Timers			= uMT_USE_TIMERS;
		Use_RestartTask		= uMT_USE_RESTARTTASK;
		Use_PrintInternals	= uMT_USE_PRINT_INTERNALS;

		Events_Num = uMT_DEFAULT_EVENTS_NUM;
	};

};

class rwCfg
{
	/////////////////////////////////
	// FRIENDS!!!
	/////////////////////////////////
	friend class uMT;

public:
	// If Kn_GetConfiguration() called BEFORE Kn_Start(), free memory BEFORE STACK allocation (=> memory availale for application)
	// If Kn_GetConfiguration() called AFTER Kn_Start(), free memory AFTER STACK allocation (=> memory availale for application)
	// In short, it contains the free RAM at the time Kn_GetConfiguration() has been called.
	StackPtr_t	FreeRAM_0;

	Cfg_data_t	Tasks_Num;				// Max task number (cannot be less than 3!!!)
	Cfg_data_t	Semaphores_Num;			// Max number of Semaphores
	Cfg_data_t	AgentTimers_Num;		// Max number of AGENT Timers
	Cfg_data_t	AppTasks_Stack_Size;	// STACK size for all the newly created tasks
	Cfg_data_t	Task1_Stack_Size;		// STACK size for Arduino loop() task
	Cfg_data_t	Idle_Stack_Size;		// Idle task STACK size


volatile Bool_t	BlinkingLED;			// Set for Led to blink
volatile Bool_t	IdleLED;				// Set for Led to turn HIGH in IDLE
volatile Bool_t	TimeSharingEnabled;		// It controls TimeSharing

	void Init()
	{
		Tasks_Num			= uMT_DEFAULT_TASK_NUM;
		Semaphores_Num		= uMT_DEFAULT_SEM_NUM;
		AgentTimers_Num		= uMT_DEFAULT_TIMER_AGENT_NUM;
		AppTasks_Stack_Size	= uMT_DEFAULT_STACK_SIZE;
		Task1_Stack_Size	= uMT_DEFAULT_TID1_STACK_SIZE;
		Idle_Stack_Size		= uMT_DEFAULT_IDLE_STACK_SIZE;

		BlinkingLED = TRUE;
		IdleLED = TRUE;

		TimeSharingEnabled = TRUE;
		FreeRAM_0 = 0;				// Clear
	};
};

class uMTcfg
{
public:
	roCfg	ro;
	rwCfg	rw;
};





#endif


/////////////////////// EOF
