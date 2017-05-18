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


#if TEST_TASK_RESTART==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TASK_RESTART_setup()
#define LOOP()	TASK_RESTART_loop()
#endif

#include <Arduino.h>

#include "uMT.h"

#if uMT_USE_RESTARTTASK==1

void SETUP() 
{
	// put your setup code here, to run once:

//	Serial.begin(9600);
	Serial.begin(57600);
//	Serial.begin(115200);

	Serial.println(F("========================================================================"));
	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.


	Serial.print(F("MySetup(): Free memory = "));
	Serial.println(uMT::Kn_GetFreeSRAM());

	Serial.println(F("================= TASK Delete test ================="));
	Serial.flush();

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start();		// Timesharing

}


int	counter = 0;

static void Task2()
{
	TaskId_t myTid;


	Kernel.Tk_GetMyTid(myTid);

	Serial.println(F("=================================="));

	if (counter++ == 0)
		Serial.print(F("  Task2(): starting, myTid = "));
	else
		Serial.print(F("  Task2(): RE-starting, myTid = "));

	Serial.println(myTid);
	Serial.flush();

	Serial.print(F("  Task2(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();

	while (1)
	{
		Serial.println(F("  Task2(): kicking..."));
		Serial.flush();

		delay(2000);
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

	Serial.println(F(" Task1(): StartTask(Task2)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid2);

	Serial.println(F(" Task1(): Yield(A)"));
	Serial.println(F(""));
	Serial.flush();

	// Run other task
	Kernel.Tk_Yield();


	uint8_t ActiveTasks;

	// Wait for the Task1 to be alive...
	do
	{
		ActiveTasks = Kernel.Tk_GetActiveTaskNo();

		Serial.print(F(" Task1(loop1): Active TASKS = "));
		Serial.println(ActiveTasks);
		Serial.flush();
		delay(500);
	} while (ActiveTasks != 2);


	Serial.println(F(""));
	Serial.println(F(" Task1(): Restarting TASK 2..."));
	Serial.println(F(""));
	Serial.flush();

	// Restart TASK 1
	Kernel.Tk_ReStartTask(Tid2);

	// Run other task
	Kernel.Tk_Yield();

	delay(5000);

	Serial.println(F(""));
	Serial.println(F(" Task1(): Rebooting..."));
	Serial.println(F("=================================="));
	Serial.flush();

	Kernel.iKn_Reboot();
}

#endif


////////////////////// EOF