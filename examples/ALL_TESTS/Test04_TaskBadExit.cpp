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


#if TEST_TASK_BADEXIT==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	BAD_EXIT_setup()
#define LOOP()	BAD_EXIT_loop()
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

	Serial.println(F("================= TASK BAD EXIT test ================="));
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

	Serial.println(F("  Task2(): Exiting..."));
	Serial.println(F(""));
	Serial.flush();

}



void LOOP()		// TASK TID=1
{
	TaskId_t Tid2;
	TaskId_t Tid3;


	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task2)"));
 
	Kernel.Tk_CreateTask(Task2, Tid2);

	Serial.print(F(" Task1(): Task2's Tid = "));
	Serial.println(Tid2);

	
	Serial.println(F(" Task1(): StartTask(Task2)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid2);

	Serial.println(F(" Task1(): Yield(A)"));
	Serial.println(F(""));
	Serial.flush();

	// Run other task
	Kernel.Tk_Yield();

	delay(1000);

	Serial.print(F(" Task1(C): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();

	Serial.println(F(""));
	Serial.println(F(" Task1(): Rebooting..."));
	Serial.println(F("=================================="));
	Serial.flush();

	delay(3000);

	Kernel.isr_Kn_Reboot();
}


////////////////////// EOF