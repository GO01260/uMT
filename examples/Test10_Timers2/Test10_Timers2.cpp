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


#if TEST_TIMERS2==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TIMERS2_setup()
#define LOOP()	TIMERS2_loop()
#endif

#include <Arduino.h>

#include <uMT.h>


#if uMT_USE_TIMERS==1

void SETUP() 
{
	// put your setup code here, to run once:

//	Serial.begin(9600);
	Serial.begin(57600);
//	Serial.begin(115200);

//	Serial.begin(9600);

	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.


	Serial.print(F("MySetup(): Free memory = "));
	Serial.println(Kernel.Kn_GetFreeRAM());

	Serial.println(F("================= TIMER test ================="));
	Serial.flush();

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();


	Kernel.Kn_Start();

//	Kernel.Kn_Start(FALSE);		// NO timesharing


	Kernel.Kn_PrintInternals();
}

#define TEST_TIMEOUT1	1000		// 2 seconds
#define TEST_TIMEOUT2	1500		// 2 seconds
#define TEST_TIMEOUT3	2500		// 2 seconds

#define LOOP_COUNT		10


static void Test_Tm_WakeupAfter()
{
	int counter = 0;
	Errno_t error;
	TimerId_t TmId;

	// Tm_WakeupAfter
	Serial.println(F("================= Tm_WakeupAfter() - BEGIN test ================="));
	Serial.flush();

	while (1)
	{
		Serial.print(F(" WakeupAfter(): => "));
		Serial.print(counter);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isrKn_GetKernelTick());

		Serial.print(F(" WakeupAfter(): Tm_WakeupAfter() - timeout = "));
		Serial.println(TEST_TIMEOUT1);
		Serial.flush();

		error = Kernel.Tm_WakeupAfter(TEST_TIMEOUT1); 	// Wake up after ... seconds

		if (error != E_SUCCESS)
		{
			Serial.print(F(" WakeupAfter(): Tm_WakeupAfter() Failure! - returned "));
			Serial.print((unsigned)error);
			Serial.flush();

			delay(10000);
		}
	}

	Serial.println(F("================= Tm_WakeupAfter() - END test ================="));
	Serial.flush();
}



static void Test_Tm_EvAfter()
{
	int counter = 0;
	Errno_t error;
	TimerId_t TmId;


	// Tm_WakeupAfter
	Serial.println(F("================= Tm_EvAfter() - BEGIN test ================="));
	Serial.flush();

	while (1)
	{
		Serial.print(F(" EvAfter(): => "));
		Serial.print(counter);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isrKn_GetKernelTick());

		Serial.print(F(" EvAfter(): Tm_EvAfter() - timeout = "));
		Serial.println(TEST_TIMEOUT2);
		Serial.flush();

		error = Kernel.Tm_EvAfter(TEST_TIMEOUT2, 1, TmId); 	// Wake up after ... seconds

		if (error != E_SUCCESS)
		{
			Serial.print(F(" EvAfter(): Tm_EvAfter() Failure! - returned "));
			Serial.println((unsigned)error);
			Serial.flush();

			delay(10000);
		}

		Event_t	eventout;
		error = Kernel.Ev_Receive(1, uMT_ANY, &eventout);

		if (error != E_SUCCESS)
		{
			Serial.print(F(" EvAfter(): Ev_Receive() Failure! - returned "));
			Serial.println((unsigned)error);
			Serial.flush();

			delay(10000);
		}
		else
		{
			if (eventout != 1)
			{
				Serial.print(F(" EvEvery(): INVALID EVENT received = "));
				Serial.println((unsigned)eventout);
				Serial.flush();

				Kernel.isrKn_FatalError();
			}
			else
			{
				Serial.print(F(" EvEvery(): EVENT received = "));
				Serial.println((unsigned)eventout);
				Serial.flush();
			}
		}

	}

	Serial.println(F("================= Tm_EvAfter() - END test  ================="));
	Serial.flush();
}


static void Test_Tm_EvEvery()
{
	int counter = 0;
	Errno_t error;
	TimerId_t TmId;

	// Tm_WakeupAfter
	Serial.println(F("================= Tm_EvEvery() - BEGIN test ================="));
	Serial.flush();

	Serial.print(F(" EvEvery(): Tm_EvEvery() - timeout = "));
	Serial.println(TEST_TIMEOUT3);
	Serial.flush();


	error = Kernel.Tm_EvEvery(TEST_TIMEOUT3, 1, TmId); 	// Wake up after ... seconds

	if (error != E_SUCCESS)
	{
		Serial.print(F(" EvEvery(): Tm_EvEvery() Failure! - returned "));
		Serial.println((unsigned)error);
		Serial.flush();

		delay(10000);
	}


	while (1)
	{
		Serial.print(F(" EvEvery(): => "));
		Serial.print(counter);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isrKn_GetKernelTick());


		Event_t	eventout;
		error = Kernel.Ev_Receive(1, uMT_ANY, &eventout);

		if (error != E_SUCCESS)
		{
			Serial.print(F(" EvEvery(): Ev_Receive() Failure! - returned "));
			Serial.println((unsigned)error);
			Serial.flush();

			delay(10000);

		}
		else
		{
			if (eventout != 1)
			{
				Serial.print(F(" EvEvery(): INVALID EVENT received = "));
				Serial.println((unsigned)eventout);
				Serial.flush();

				Kernel.isrKn_FatalError();
			}
			else
			{
				Serial.print(F(" EvEvery(): EVENT received = "));
				Serial.println((unsigned)eventout);
				Serial.flush();
			}
		}

	}

	// Deleting timer...
	error = Kernel.Tm_Cancel(TmId);

	if (error != E_SUCCESS)
	{
		Serial.print(F(" EvEvery(): Tm_cancel() Failure! - returned "));
		Serial.println((unsigned)error);
		Serial.flush();

		delay(10000);

	}


	Serial.println(F("================= Tm_EvEvery() - END test  ================="));
	Serial.flush();

}




void LOOP()		// TASK TID=1
{	
	TaskId_t myTid;
	Errno_t error;

	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F(" LOOP(): myTid = "));
	Serial.println(myTid);
	Serial.flush();


	TaskId_t TWkAf;
	TaskId_t TEvAf;
	TaskId_t TEvev;


	Serial.println(F(" LOOP(): Kernel.Tk_CreateTask()"));
 
	error = Kernel.Tk_CreateTask(Test_Tm_WakeupAfter, TWkAf);
	if (error != E_SUCCESS)
	{
		Serial.print(F(" Tk_CreateTask(): Failure! - returned "));
		Serial.println((unsigned)error);
		Serial.flush();

		Kernel.isrKn_FatalError();
	}


	error = Kernel.Tk_CreateTask(Test_Tm_EvAfter, TEvAf);
	if (error != E_SUCCESS)
	{
		Serial.print(F(" Tk_CreateTask(): Failure! - returned "));
		Serial.println((unsigned)error);
		Serial.flush();

		Kernel.isrKn_FatalError();
	}

	Serial.print(F(" LOOP(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();
	

	Serial.println(F(" LOOP(): StartTask()"));
	Serial.flush();

	Kernel.Tk_StartTask(TWkAf);
	Kernel.Tk_StartTask(TEvAf);



	//	Kernel.Tk_CreateTask(Test_Tm_EvEvery, TEvev);
	Test_Tm_EvEvery();


	Serial.println(F("================= END ================="));
	Serial.flush();

	while (1)
	{
		delay(2000);
//		Kernel.Kn_PrintInternals();
	}
}

#endif



////////////////////// EOF