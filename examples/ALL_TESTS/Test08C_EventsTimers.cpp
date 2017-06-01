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


#if TEST_EVENTS_TIMERS==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	EVENTS_TIMERS_setup()
#define LOOP()	EVENTS_TIMERS_loop()
#endif

#include <Arduino.h>

#include <uMT.h>

#if uMT_USE_EVENTS==1

#if uMT_USE_TIMERS==1

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

	Serial.println(F("================= Test08B_EventsTimers test ================="));

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start();

}

#define EVENT_A		0x0001
#define EVENT_B		0x0002
#define EVENT_C		0x0004
#define EVENT_ALL	0x0007

#define TIMEOUT_A	1300		// NUmero primo
#define TIMEOUT_B	1700		// NUmero primo
#define TIMEOUT_C	1900		// NUmero primo

static void CheckErrno(Errno_t error)
{
	if (error != E_SUCCESS)
	{
		Serial.print(F(" EvEvery(): Tm_EvEvery() Failure! - returned "));
		Serial.println((unsigned)error);
		Serial.flush();

		delay(10000);
	}
}

static void Test_Tm_EvEvery()
{
	Errno_t error;
	TimerId_t TmId;
	unsigned counter = 0;

	// Tm_WakeupAfter
	Serial.println(F("================= Tm_EvEvery() - BEGIN test ================="));
	Serial.flush();

	CheckErrno(Kernel.Tm_EvEvery(TIMEOUT_A, EVENT_A, TmId)); 	// Send event after ... seconds
	CheckErrno(Kernel.Tm_EvEvery(TIMEOUT_B, EVENT_B, TmId)); 	// Send event after ... seconds
	CheckErrno(Kernel.Tm_EvEvery(TIMEOUT_C, EVENT_C, TmId)); 	// Send event after ... seconds


	while (1)
	{
		Serial.print(F(" EvEvery(): => "));
		Serial.print(counter++);
		Serial.print(F("  KernelTickCounter => "));
		Serial.println(Kernel.isr_Kn_GetKernelTick());


		Event_t	eventout;
		error = Kernel.Ev_Receive(EVENT_ALL, uMT_ANY, &eventout);

		if (error != E_SUCCESS)
		{
			Serial.print(F(" EvEvery(): Ev_Receive() Failure! - returned "));
			Serial.println((unsigned)error);
			Serial.flush();

			delay(10000);

		}
		else
		{
			if (eventout & EVENT_A)
			{
				Serial.print(F(" EvEvery(): EVENT_A received"));
				Serial.print(F("  KernelTickCounter => "));
				Serial.println(Kernel.isr_Kn_GetKernelTick());
				Serial.flush();
			}
			if (eventout & EVENT_B)
			{
				Serial.print(F(" EvEvery(): EVENT_B received"));
				Serial.print(F("  KernelTickCounter => "));
				Serial.println(Kernel.isr_Kn_GetKernelTick());
				Serial.flush();
			}
			if (eventout & EVENT_C)
			{
				Serial.print(F(" EvEvery(): EVENT_C received"));
				Serial.print(F("  KernelTickCounter => "));
				Serial.println(Kernel.isr_Kn_GetKernelTick());
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

	Test_Tm_EvEvery();
  
}
#endif
#endif


////////////////////// EOF