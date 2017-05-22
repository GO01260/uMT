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


#if TEST_YIELD_SPEED==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	YIELD_SPEED_setup()
#define LOOP()	YIELD_SPEED_loop()
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
	Serial.println(Kernel.Kn_GetFreeRAM());

	Serial.println(F("================= Yield Speed test ================="));
	Serial.flush();

	Serial.println(F("MySetup(): => Kernel.Kn_Start(FALSE)"));
	Serial.flush();

	Kernel.Kn_Start(FALSE);		// No Timesharing

}

#define _LOOP_DEBUG	0

#define LOOP_COUNT	100000L
typedef long	Index_t;

Index_t	Yields = 0;

static void Task2()
{
	int counter = 0;
	TaskId_t myTid;


	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("*Task2(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	Index_t idx = 0L;

	while (idx <= LOOP_COUNT)
	{
#if _LOOP_DEBUG==1
		Serial.print(F("*Task2(): Loop index = "));
		Serial.println(idx);
		Serial.flush();
#endif

		Yields++;

		Kernel.Tk_Yield();

		idx++;
	}

	Kernel.Tk_Yield();

#if _LOOP_DEBUG==1
	Serial.print(F("*Task2(): end: index = "));
	Serial.println(idx);
	Serial.flush();
#endif

	while (1)
	;
}

void LOOP()		// TASK TID=1
{
	int counter = 0;
	TaskId_t Tid;


	Serial.println(F("Task1(): Kernel.Tk_CreateTask(Task1)"));
 
	Kernel.Tk_CreateTask(Task2, Tid);

	Serial.print(F("Task1(): Task2's Tid = "));
	Serial.println(Tid);

	Serial.println(F("Task1(): StartTask(Task2)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid);

	Kernel.Tk_Yield();

	TaskId_t myTid;
	
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("Task1(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	Timer_t Elapsed = millis();

	Index_t idx = 0L;

	while (idx <= LOOP_COUNT)
	{
#if _LOOP_DEBUG==1
		Serial.print(F("Task1(): Loop index = "));
		Serial.println(idx);
		Serial.flush();
#endif

		Yields++;

		Kernel.Tk_Yield();

		idx++;
	}

	Elapsed = millis() - Elapsed;

	Serial.print(F("Yields = "));
	Serial.println(Yields);
	Serial.print(F("Elapsed = "));
	Serial.println(Elapsed);

	double result = (double)Yields / ((double)Elapsed / 1000.0);
	Serial.print(F(" Task switch/second = "));
	Serial.println(result);


	Serial.println(F("================= Yield Speed test END ================="));
	Serial.flush();

	delay(5000);

	Kernel.isrKn_Reboot();

	while (1)
		;

}


////////////////////// EOF