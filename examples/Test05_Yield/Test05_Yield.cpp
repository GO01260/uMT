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


#if TEST_YIELD==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	YIELD_setup()
#define LOOP()	YIELD_loop()
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

	Serial.println(F("================= Yield test ================="));
	Serial.flush();

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));

	delay(2000);

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();


	Kernel.Kn_Start(FALSE);		// NO timesharing

}


static void Task2()
{
	int counter = 0;
	TaskId_t myTid;

	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("  Task2(): myTid = "));
	Serial.println(myTid.GetID());
	Serial.flush();

	while (1)
	{
		counter++;

		Serial.print(F("  Task2(): => "));
		Serial.print(counter);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isr_Kn_GetKernelTick());

		Serial.println(F("  Task2(): Tk_Yield()"));
		Serial.flush();
		delay(300);

		Kernel.Tk_Yield();
	}

}

void LOOP()		// TASK TID=1
{
	int counter = 0;
	TaskId_t Tid;

	Serial.println(F(" Task1(): Kernel.Tk_CreateTask(Task1)"));
 
	Kernel.Tk_CreateTask(Task2, Tid);

	Serial.print(F(" Task1(): Task2's Tid = "));
	Serial.println(Tid.GetID());

	Serial.println(F(" Task1(): StartTask(Task1)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid);

	TaskId_t myTid;
	
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F(" Task1(): myTid = "));
	Serial.println(myTid.GetID());
	Serial.flush();

	while (1)
	{
		counter++;

		Serial.print(F(" Task1(): => "));
		Serial.print(counter);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isr_Kn_GetKernelTick());

		Serial.println(F(" Task1(): Tk_Yield()"));
		Serial.flush();
		delay(300);

		Kernel.Tk_Yield();
	}
  
}


////////////////////// EOF