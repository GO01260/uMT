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
#if TEST_COMPLEX_1==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	COMPLEX1_setup()
#define LOOP()	COMPLEX1_loop()
#endif

#include "uMT.h"

#include <Arduino.h>

#if uMT_USE_TIMERS==1

//////////////////////////////////////////////////
// Configuration
//////////////////////////////////////////////////

#define SUPPLIER_TIMEOUT		0			// 1/100 sec
#define SUPPLIER_SEM			15
#define ALL_EVENTS			0x7FFF		// 15 events (except 16) 

#define MAX_PRODUCERS		6			// How many producers
#define	SIMPLE_MONITOR		0


#define VERBOSE_PRODUCER	0
#define VERBOSE_SUPPLIER	0
#define VERBOSE_CONSUMER	0

///////////////////////////////////////////////////////


void SETUP() 
{
	// put your setup code here, to run once:

//	Serial.begin(9600);
	Serial.begin(57600);
//	Serial.begin(115200);

	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.


	Serial.print(F("MySetup(): Free memory = "));
	Serial.println(Kernel.Kn_GetFreeSRAM());

	Serial.println(F("================= Farmer/Producer/Consumer test ================="));

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));


	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start();

}


void CheckError(const __FlashStringHelper *Msg, Errno_t error)
{
	if (error != E_SUCCESS)
	{
		Serial.print(F("FATAL ERROR: "));
		Serial.print(Msg);
		Serial.println((unsigned)error);
		Serial.flush();

		Kernel.iKn_FatalError();
	}
}



TaskId_t TidConsumer;

unsigned int	SupplierProduced = 0;
unsigned int	Consumed[MAX_PRODUCERS];


#if VERBOSE_SUPPLIER==1
#define SupplierPrint(x)		Serial.print(x), Serial.flush();
#define SupplierPrintln(x)		Serial.println(x), Serial.flush();
#else
#define SupplierPrint(x)
#define SupplierPrintln(x)
#endif

void Supplier()
{
	TaskId_t myTid;
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("Supplier: [Tid="));
	Serial.print(myTid);
	Serial.print(F("] timeout="));
	Serial.println(SUPPLIER_TIMEOUT);
	Serial.flush();

	while (1)
	{
		SupplierProduced++;

		SupplierPrint(F("Supplier: producing item = "));
		SupplierPrintln(SupplierProduced);
		CheckError(F("Supplier"), Kernel.Sm_Release(SUPPLIER_SEM));


		SupplierPrintln(F("Supplier: sleeping..."));
		CheckError(F("Supplier"), Kernel.Tm_WakeupAfter(SUPPLIER_TIMEOUT)); 	// Wake up after ... seconds

	}

}


#if VERBOSE_PRODUCER==1
#define ProducerPrintln(x)	Serial.println(x), Serial.flush();
#define ProducerPrint(x)	Serial.print(x), Serial.flush();
#else
#define ProducerPrintln(x)
#define ProducerPrint(x)
#endif

void PrintLead(int count)
{
	while (count-- > 0)
		ProducerPrint(F(" "));
}


void Producer()
{
	Param_t Param;
	int	HowManyComponents;
	TaskId_t myTid;
	SemId_t	SemId;
	Event_t Event;
	unsigned int Produced = 0;

	// Get how many
	Kernel.Tk_GetParam(Param);

	int ProdIndex = (int)Param;
	HowManyComponents = ProdIndex + 1;
	SemId = ProdIndex;
	Event = 0x01 << ProdIndex;


	Kernel.Tk_GetMyTid(myTid);

//	PrintLead(ProdIndex);
	Serial.print(F("Producer("));
	Serial.print(ProdIndex);
	Serial.print(F(") [Tid="));
	Serial.print(myTid);
	Serial.print(F("]: items required for each part = "));
	Serial.println(HowManyComponents);
	Serial.flush();

	while (1)
	{
		// Collecting farmer output
		for (int idx = 0; idx < HowManyComponents; idx++)
		{
			CheckError(F("Producer"), Kernel.Sm_Claim(SUPPLIER_SEM, uMT_WAIT));

			PrintLead(ProdIndex);
			ProducerPrint(F("Producer("));
			ProducerPrint(ProdIndex);
			ProducerPrint(F("): collected "));
			ProducerPrint(idx);
			ProducerPrint(F(" items of "));
			ProducerPrintln(HowManyComponents);

		}

		Produced++;

		PrintLead(ProdIndex);
		ProducerPrint(F("Producer("));
		ProducerPrint(ProdIndex);
		ProducerPrint(F("): produced  "));
		ProducerPrint(Produced);
		ProducerPrintln(F(" parts"));

		// Producing output
		CheckError(F("Producer"), Kernel.Sm_Release(SemId));

		// Signaling Consumer
		CheckError(F("Producer"), Kernel.iEv_Send(TidConsumer, Event));
	}

}

#if VERBOSE_CONSUMER==1
#define ConsumerPrint(x)		Serial.print(x), Serial.flush();
#define ConsumerPrintln(x)		Serial.println(x), Serial.flush();
#else
#define ConsumerPrint(x)
#define ConsumerPrintln(x)
#endif

void ConsumerCollect(int producer, unsigned int	&Consumed)
{
	unsigned int OldConsumed = Consumed;

	while (Kernel.Sm_Claim(producer, uMT_NOWAIT) == E_SUCCESS)
	{
		Consumed++;
	}

	if (OldConsumed == Consumed)
	{
		ConsumerPrint(F("********* Consumer(t="));
		ConsumerPrint(millis());
		ConsumerPrint(F("): from producer = "));
		ConsumerPrint(producer);
		ConsumerPrintln(F(", ERROR!!! No new parts!"));
	}
	else
	{
		for (int idx = 0; idx < producer; idx++)
			ConsumerPrint(F("="));

		ConsumerPrint(F("> Consumer(t="));
		ConsumerPrint(millis());
		ConsumerPrint(F("): from producer = "));
		ConsumerPrint(producer);
		ConsumerPrint(F(", total consumed parts = "));
		ConsumerPrintln(Consumed);
	}

}


void Consumer()
{
	TaskId_t myTid;
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("Consumer: [Tid="));
	Serial.print(myTid);
	Serial.print(F("] ALL_EVENTS=0x"));
	Serial.println(ALL_EVENTS, HEX);
	Serial.flush();


	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		Consumed[idx] = 0;


	while (1)
	{
		Event_t	eventout = 0;

		CheckError(F("Consumer"), Kernel.Ev_Receive(ALL_EVENTS, uMT_ANY, &eventout));

		for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		{
			Bool_t EventSet = (eventout & 0x01);
			
			eventout >>= 1;

			if (EventSet)
			{
				ConsumerCollect(idx, Consumed[idx]);
			}
		}
	}
}


#define MONITOR_TIMEOUT			5000			// 5 Seconds
#define MONITOR_PRINT_KERNEL	1

#ifdef SIMPLE_MONITOR
void Monitor()
{
	unsigned int	counter = 0;
	TaskId_t myTid;

	Kernel.Tk_GetMyTid(myTid);

	while (1)
	{

		CheckError(F("Kernel.Tm_WakeupAfter(MONITOR_TIMEOUT)"), Kernel.Tm_WakeupAfter(MONITOR_TIMEOUT)); 	// Wake up after ... seconds

		Serial.print(F("*** [Tid="));
		Serial.print(myTid);
		Serial.print(F("] Monitor(t="));
		Serial.print(millis());
		Serial.print(F("): "));

		for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		{
			Serial.print(idx);
			Serial.print(F("=>"));
			Serial.print(Consumed[idx]);
			Serial.print(F(" "));
		}
		Serial.println(F("***"));
		Serial.flush();


		counter++;
		
		if ((counter % MONITOR_PRINT_KERNEL) == 0)
		{
			Kernel.Kn_PrintInternals();
		}

	}
}
#else

#define MONITOR_EVENT		0x8000		// #15 bit
#define MONITOR_PK_AFTER	0

void Monitor()
{
	unsigned int	counter = 0;
	TaskId_t myTid;
	TimerId_t TmId;


	Kernel.Tk_GetMyTid(myTid);

	CheckError(F("Kernel.Tm_EvEvery(MONITOR_TIMEOUT, MONITOR_EVENT, TmId)"), Kernel.Tm_EvEvery(MONITOR_TIMEOUT, MONITOR_EVENT, TmId)); 	// Wake up after ... seconds

	while (1)
	{
		Event_t	eventout;
		
		CheckError(F("Kernel.Ev_Receive(MONITOR_EVENT, uMT_ANY, &eventout)"), Kernel.Ev_Receive(MONITOR_EVENT, uMT_ANY, &eventout));

		if (eventout != MONITOR_EVENT)
		{
			Serial.print(F(" EvEvery(): INVALID EVENT received = "));
			Serial.println((unsigned)eventout);
			Serial.flush();

			Kernel.iKn_FatalError();
		}
		else
		{

#if MONITOR_PK_AFTER==0
			if ((counter % MONITOR_PRINT_KERNEL) == 0)
			{
				Kernel.Kn_PrintInternals();
			}
#endif
			Serial.print(F("*** [Tid="));
			Serial.print(myTid);
			Serial.print(F("] Monitor(t="));
			Serial.print(millis());
			Serial.print(F("): "));

			for (int idx = 0; idx < MAX_PRODUCERS; idx++)
			{
				Serial.print(idx);
				Serial.print(F("=>"));
				Serial.print(Consumed[idx]);
				Serial.print(F(" "));
			}
			Serial.println(F("***"));
			Serial.flush();


			counter++;
	
#if MONITOR_PK_AFTER==1
			if ((counter % MONITOR_PRINT_KERNEL) == 0)
			{
				Kernel.Kn_PrintInternals();
			}
#endif
		}

	}
}

#endif


void LOOP()		// TASK TID=1
{
	Errno_t error;
	
#ifdef ZAPPED
	Kernel.Tk_GetMyTid(TidConsumer);

	Serial.print(F("LOOP(): TidConsumer = "));
	Serial.println(TidConsumer);
	Serial.flush();
#endif

	TaskId_t TidSupplier;
	TaskId_t TidProducer[MAX_PRODUCERS];


	Serial.println(F("LOOP(): Kernel.Tk_CreateTask()"));

	CheckError(F("Kernel.Tk_CreateTask(Supplier, TidSupplier)"), Kernel.Tk_CreateTask(Supplier, TidSupplier));
	CheckError(F("Kernel.Tk_CreateTask(Consumer, TidConsumer)"), Kernel.Tk_CreateTask(Consumer, TidConsumer));

	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
	{
		CheckError(F("Kernel.Tk_CreateTask(Farmer, TidProducer)"), Kernel.Tk_CreateTask(Producer, TidProducer[idx]));
//		Kernel.Kn_PrintInternals();
	}


	Serial.print(F("LOOP(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();


	// Set parameters for Producers
	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		Kernel.Tk_SetParam(TidProducer[idx], idx);

	Serial.println(F("LOOP(): StartTask()")); Serial.flush();

	CheckError(F("Kernel.Tk_StartTask(TidSupplier)"), Kernel.Tk_StartTask(TidSupplier));
	CheckError(F("Kernel.Tk_StartTask(TidConsumer)"), Kernel.Tk_StartTask(TidConsumer));

	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		CheckError(F("Kernel.Tk_StartTask(TidProducer)"), Kernel.Tk_StartTask(TidProducer[idx]));

	// Run Monitor
	Monitor();


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