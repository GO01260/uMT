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


#include "Test_Config.h"


// This is done to force compilation (and error checking) even if test is not selected
#if TEST_STACK_UTILIZATION==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TEST_STACK_UTILIZATION_setup()
#define LOOP()	TEST_STACK_UTILIZATION_loop()
#endif


#include <uMT.h>

#include <Arduino.h>

void SETUP() 
{
	// put your setup code here, to run once:

//	Serial.begin(9600);
	Serial.begin(57600);
//	Serial.begin(115200);

	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.


	Serial.print(F("MySetup(): Free memory = "));
	Serial.println(Kernel.Kn_GetFreeRAM());

	Serial.println(F("================= Stack Utilization test ================="));
	Serial.flush();

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start(FALSE);		// No timesharing

}

static void Print(uMTtaskInfo &Info)
{
	Kernel.Tk_GetTaskInfo(Info);
	Kernel.Tk_PrintInfo(Info);
//	Kernel.Kn_PrintInternals();
}

static void Stack64()
{
	uint8_t	local[64];
	uMTtaskInfo Info;

	local[1] = 0;

	Print(Info);
}

static void Stack96()
{
	uint8_t	local[96];
	uMTtaskInfo Info;

	local[1] = 0;

	Print(Info);
}

static void Stack128()
{
	uint8_t	local[128];
	uMTtaskInfo Info;

	local[1] = 0;

	Print(Info);
}

static void Execute()
{
	Serial.print(F("=> Stack64()"));
	Serial.println(F(""));
	Stack64();

	Serial.print(F("=> Stack96()"));
	Serial.println(F(""));
	Stack96();

//	Serial.print(F("=> Stack128()"));
//	Serial.println(F(""));
//	Stack128();

}

static void Task2()
{
	Execute();

	Kernel.Kn_PrintInternals(TRUE);

	while (1)
	{
		Kernel.Tm_WakeupAfter(2000);
	}

	Kernel.Tk_DeleteTask();
}

void LOOP()		// TASK TID=1
{
	TaskId_t Tid2;

	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task2)"));
 
	Kernel.Tk_CreateTask(Task2, Tid2);
	Kernel.Tk_StartTask(Tid2);

	Kernel.Kn_PrintInternals(TRUE);

	// Run other task...
	Kernel.Tk_Yield();

	Execute();


	while (1)
	{
		Kernel.Tm_WakeupAfter(4000);
		Kernel.Kn_PrintInternals(TRUE);
	}

}


////////////////////// EOF