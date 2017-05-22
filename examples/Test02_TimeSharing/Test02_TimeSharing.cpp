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
#if TEST_TIMESHARING==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TIME_SHARING_setup()
#define LOOP()	TIME_SHARING_loop()
#endif

#include <Arduino.h>

#include <uMT.h>

#define	SEM_ID_01		1		// Semaphore id
#define	SEM_ID_02		2		// Semaphore id


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

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start();

}


static void Task2()
{
	int counter = 0;
	TaskId_t myTid;

	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("  Task2(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	Timer_t OldSysTick = Kernel.isrKn_GetKernelTick();
	Timer_t OldSMillis = millis();

	while (1)
	{
		Timer_t NowSysTick = Kernel.isrKn_GetKernelTick();
		Timer_t DeltaSysTick = (NowSysTick - OldSysTick) * 1000 / uMT_TICKS_SECONDS;	// in milliseconds
		OldSysTick = NowSysTick;

		Timer_t NowMillis = millis();
		Timer_t DeltaMillis = NowMillis - OldSMillis;
		OldSMillis = NowMillis;

		Serial.print(F("  Task2(): => "));
		Serial.print(++counter);

		Serial.print(F("  KernelTickCounter => "));
		Serial.print(NowSysTick);

		Serial.print(F(" - Delta in milliseconds => "));
		Serial.print(DeltaSysTick);

		Serial.print(F("  Delta in millis() => "));
		Serial.println(DeltaMillis);
		Serial.flush();

		delay(100);

	}

	

}

void LOOP()		// TASK TID=1
{
	int counter = 0;
	TaskId_t Tid;

	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task1)"));
 
	Kernel.Tk_CreateTask(Task2, Tid);

	Serial.print(F(" Task1(): Task2's Tid = "));
	Serial.println(Tid);

	Serial.println(F(" Task1(): StartTask(Task1)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid);

	TaskId_t myTid;
	
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F(" Task1(): myTid = "));
	Serial.println(myTid);
	Serial.flush();

	while (1)
	{
		Serial.print(F(" Task1(): => "));
		Serial.print(++counter);
		Serial.print(F("  millis() => "));
		Serial.println(millis());
		Serial.flush();

		delay(100);
	}
  
}


////////////////////// EOF
