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


#if TEST_TASK_DELETE==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TASK_DELETE_setup()
#define LOOP()	TASK_DELETE_loop()
#endif

#include <Arduino.h>

#include <uMT.h>


void SETUP() 
{
	// put your setup code here, to run once:

//	Serial.begin(9600);
	Serial.begin(57600);
//	Serial.begin(115200);

	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.


	Serial.print(F("MySetup(): Free memory = "));
	Serial.println(uMT::Kn_GetFreeRAM());

	Serial.println(F("================= TASK Delete test ================="));
	Serial.flush();

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start(FALSE);		// No Timesharing

}

#define _LOOP_DEBUG	0

#define LOOP_COUNT	100000L
typedef long	Index_t;

static void Task2()
{
	int counter = 0;
	TaskId_t myTid;


	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("  Task2(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	Serial.print(F("  Task2(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();

	Serial.println(F("  Task2(): Deleting myself..."));
	Serial.println(F(""));
	Serial.flush();

	delay(1000);

	Kernel.Tk_DeleteTask(myTid);

}

static void Task3()
{
	int counter = 0;
	TaskId_t myTid;


	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("   Task3(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	Serial.print(F("   Task3(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();

	Serial.println(F("   Task3(): Yield()"));
	Serial.println(F(""));
	Serial.flush();

	// Run other task
	Kernel.Tk_Yield();

	while (1)
	{
	}

}


void LOOP()		// TASK TID=1
{
	TaskId_t Tid2;
	TaskId_t Tid3;


	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task2)"));
 
	Kernel.Tk_CreateTask(Task2, Tid2);

	Serial.print(F(" Task1(): Task2's Tid = "));
	Serial.println(Tid2);


	Serial.print(F(" Task1(A): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();



	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task3)"));
 
	Kernel.Tk_CreateTask(Task3, Tid3);

	Serial.print(F(" Task1(): Task3's Tid = "));
	Serial.println(Tid3);



	Serial.print(F(" Task1(B): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();


	Serial.println(F(" Task1(): StartTask(Task2)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid2);

	Serial.println(F(" Task1(): Yield(A)"));
	Serial.println(F(""));
	Serial.flush();

	// Run other task
	Kernel.Tk_Yield();


	uint8_t ActiveTasks;

	// Wiat for the Tsk1 to die...
	do
	{
		ActiveTasks = Kernel.Tk_GetActiveTaskNo();

		Serial.print(F(" Task1(loop1): Active TASKS = "));
		Serial.println(ActiveTasks);
		Serial.flush();
		delay(500);
	} while (ActiveTasks != 2);


	// creating second task and then kill it....


	Serial.println(F(""));
	Serial.println(F(" Task1(): StartTask(Task3)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid3);


	Serial.print(F(" Task1(C): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();

	Serial.println(F(" Task1(): Yield(B)"));
	Serial.println(F(""));
	Serial.flush();

	// Run other task
	Kernel.Tk_Yield();

	delay(1000);

	Serial.println(F(" Task1(): Printing internals..."));
	Serial.flush();
	Kernel.Kn_PrintInternals();
	delay(1000);

	Serial.println(F(" Task1(): Deleting Task3..."));
	Serial.flush();
	Kernel.Tk_DeleteTask(Tid3);


	Serial.println(F(""));
	Serial.println(F(" Task1(): suiciding..."));
	Serial.println(F("=================================="));
	Serial.flush();

	TaskId_t myTid;
	Kernel.Tk_GetMyTid(myTid);
	
	Errno_t errno = Kernel.Tk_DeleteTask(myTid);
	if (errno != E_SUCCESS)
	{
		Serial.print(F("Tk_DeleteTask failed: errno = "));
		Serial.println(errno);
	}

	Serial.println(F(""));
	Serial.println(F(" Task1(): Rebooting..."));
	Serial.println(F("=================================="));
	Serial.flush();

	Kernel.isr_Kn_Reboot();
}


////////////////////// EOF