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
#if TEST_MALLOC==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	MALLOC_setup()
#define LOOP()	MALLOC_loop()
#endif


#include "uMT.h"

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

	Serial.println(F("================= MALLOC test ================="));
	Serial.flush();

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));

	Serial.print(F("MySetup(): => sizeof(size_t)="));
	Serial.println(sizeof(size_t));
	Serial.flush();

	delay(2000);

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();


	Kernel.Kn_Start(FALSE);		// No timesharing

}

static void Task2()
{
	unsigned counter = 0;
	TaskId_t myTid;

	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("** Task2(): myTid = "));
	Serial.println(myTid);
	Serial.flush();
	char *oldptr = NULL;

	while (1)
	{
		if (counter == 0)
			counter = 1;
		else
			counter <<= 1;
		Serial.print(F("** Task2(): malloc("));
		Serial.print(counter);
		Serial.println(F(")"));
		Serial.flush();

		char *ptr = (char *)malloc(counter);

		Serial.print(F("** Task2(): malloc("));
		Serial.print(counter);

		if (ptr == NULL)
		{
			Serial.println(F("): malloc FAILURE!"));
			Serial.flush();
			delay(5000);
		}
		else
		{
			Serial.println(F("): malloc SUCCESS!"));
			Serial.flush();
			if (oldptr != NULL)
				free(oldptr);
		}

		oldptr = ptr;

		Serial.println(F("** Task2(): Yield(A)"));
		Serial.println(F(""));
		Serial.flush();

		// Run other task
		Kernel.Tk_Yield();

		delay(500);
	}


}


void LOOP()		// TASK TID=1
{
	TaskId_t Tid2;


	Serial.println(F("Task1(): Kernel.Tk_CreateTask(Task2)"));
 
	Kernel.Tk_CreateTask(Task2, Tid2);

	Serial.print(F("Task1(): Task2's Tid = "));
	Serial.println(Tid2);

	Serial.println(F("Task1(): StartTask(Task2)"));
	Serial.flush();

 	Kernel.Tk_StartTask(Tid2);


	while (1)
	{
		Serial.println(F("Task1(): Yield(A)"));
		Serial.println(F(""));
		Serial.flush();

		// Run other task
		Kernel.Tk_Yield();
	}
 
}


////////////////////// EOF