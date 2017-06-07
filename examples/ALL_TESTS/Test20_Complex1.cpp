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
#if TEST_COMPLEX_1HUGE==1
#define SETUP()	setup()
#define LOOP()	loop()

#define MAX_SEM		102
#define MAX_TASKS	100

#elif TEST_COMPLEX_1==1
#define SETUP()	setup()
#define LOOP()	loop()

#define MAX_SEM		uMT_DEFAULT_SEM_NUM
#define MAX_TASKS	uMT_DEFAULT_TASK_NUM

#else

#define SETUP()	COMPLEX1HUGE_setup()
#define LOOP()	COMPLEX1HUGE_loop()

#define MAX_SEM		uMT_DEFAULT_SEM_NUM
#define MAX_TASKS	uMT_DEFAULT_TASK_NUM

#endif

#include "uMT.h"

#include <Arduino.h>


#if uMT_USE_TIMERS==1

//////////////////////////////////////////////////
// Configuration
//////////////////////////////////////////////////

#define SUPPLIER_TIMEOUT		0			// 1/100 sec
#define ALL_EVENTS				uMT_ALL_EVENT_MASK		// 32 events if SAM or 16 if AVR

#define MAX_PRODUCERS			(MAX_TASKS - 4)			// How many producers: N - (IDLE + 1 Monitor + 1 Suplier + 1 Consumer)

#define VERBOSE_PRODUCER		0
#define VERBOSE_SUPPLIER		0
#define VERBOSE_CONSUMER		0

#define SEM_ID_OFFSET			4		// See below

///////////////////////////////////////////
// TID Mapping
// 0: IDLE
// 1: Arduino loop() => Monitor()
// 2: Supplier()
// 3: Consumer()
// 4-xx: Produced
//
// Note:each task is using a Semaphore EQUAL to its Task_id
// SemId = MyOwnTid or TaskIndex (0--n) + 4
/////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////

static void CheckError(const __FlashStringHelper *Msg, Errno_t error)
{
	if (error != E_SUCCESS)
	{
		Serial.print(F("FATAL ERROR: "));
		Serial.print(Msg);
		Serial.print(F(" - ERRNO = "));
		Serial.println((unsigned)error);
		Serial.flush();

		Kernel.isr_Kn_FatalError();
	}
}

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

	Serial.println(F("================= Supplier/Producer/Consumer test ================="));

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));


	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	

	uMTcfg Cfg;

	Kernel.Kn_GetConfiguration(Cfg);

	Cfg.rw.Tasks_Num = MAX_TASKS;
	Cfg.rw.Semaphores_Num = MAX_SEM;


#if TEST_COMPLEX_1HUGE==1
	Cfg.rw.Task1_Stack_Size = 512;
	Cfg.rw.AppTasks_Stack_Size = 512;
#else
	Cfg.rw.Task1_Stack_Size = uMT_DEFAULT_TID1_STACK_SIZE;
	Cfg.rw.AppTasks_Stack_Size = uMT_DEFAULT_STACK_SIZE;
#endif

	CheckError(F("Setup"), Kernel.Kn_Start(Cfg));
//	CheckError(F("Setup"), Kernel.Kn_Start());

	// Print works only AFTER Start
	Kernel.Kn_PrintConfiguration(Cfg);


	Kernel.Kn_PrintInternals();

	delay(2000);

}





static	TaskId_t		TidConsumer;
static	SemId_t			SidSupplier;

static unsigned int		SupplierProduced = 0;
static unsigned int		Consumed[MAX_PRODUCERS];
static unsigned int		OldConsumed[MAX_PRODUCERS];


#if VERBOSE_SUPPLIER==1
#define SupplierPrint(x)		Serial.print(x), Serial.flush();
#define SupplierPrintln(x)		Serial.println(x), Serial.flush();
#else
#define SupplierPrint(x)
#define SupplierPrintln(x)
#endif

static void Supplier()
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
		CheckError(F("Supplier"), Kernel.Sm_Release(SidSupplier));

		SupplierPrintln(F("Supplier: sleeping..."));

		// Wake up after ... seconds. If 0, roundrobin
		CheckError(F("Supplier"), Kernel.Tm_WakeupAfter(SUPPLIER_TIMEOUT)); 	

	}

}


#if VERBOSE_PRODUCER==1
#define ProducerPrintln(x)	Serial.println(x), Serial.flush();
#define ProducerPrint(x)	Serial.print(x), Serial.flush();
#else
#define ProducerPrintln(x)
#define ProducerPrint(x)
#endif

static void PrintLead(int count)
{
	while (count-- > 0)
		ProducerPrint(F(" "));
}


static void Producer()
{
	Param_t Param;
	int	HowManyComponents;
	TaskId_t myTid;
	SemId_t	SemId;
	Event_t Event;
	unsigned int Produced = 0;

	// Get how many
	Kernel.Tk_GetParam(Param);

	int ProdIndex = (int)Param;			// Task index: 0..n
	HowManyComponents = ProdIndex + 1;
	SemId = Param + SEM_ID_OFFSET;

	// There are at most 32 events per task, so we need to map Taskid to Event
	// To do that we take the remainder of the integer division
	Event =  0x01 << (Param % uMT_DEFAULT_SEM_NUM);

//	Event = 0x01 << ProdIndex;


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
			// Collect part from the SUPPLIER using the SUPPLIER_SEM
			CheckError(F("Producer"), Kernel.Sm_Claim(SidSupplier, uMT_WAIT));

			PrintLead(ProdIndex);
			ProducerPrint(F("Producer("));
			ProducerPrint(ProdIndex);
			ProducerPrint(F("): collected "));
			ProducerPrint(idx);
			ProducerPrint(F(" items of "));
			ProducerPrintln(HowManyComponents);

		}
		// All components collected, assembly the part...

		Produced++;

		// One more assembled part produced

		PrintLead(ProdIndex);
		ProducerPrint(F("Producer("));
		ProducerPrint(ProdIndex);
		ProducerPrint(F("): produced  "));
		ProducerPrint(Produced);
		ProducerPrintln(F(" parts"));

		// Producing output for the Consumer on our own Semaphore (not shared)
		CheckError(F("Producer"), Kernel.Sm_Release(SemId));

		// Signaling Consumer (one consumer only), possibly shared among multiple Producer (Events are limited to 32)
		CheckError(F("Producer"), Kernel.Ev_Send(TidConsumer, Event));
	}

}

#if VERBOSE_CONSUMER==1
#define ConsumerPrint(x)		Serial.print(x), Serial.flush();
#define ConsumerPrintln(x)		Serial.println(x), Serial.flush();
#define ConsumerPrintValueln(x, y)		Serial.println(x, y), Serial.flush();
#else
#define ConsumerPrint(x)
#define ConsumerPrintln(x)
#define ConsumerPrintValueln(x, y)
#endif

static void ConsumerCollect(int producer, unsigned int	&Consumed)
{
	unsigned int OldConsumed = Consumed;
	SemId_t	SemId = producer + SEM_ID_OFFSET;

	while (Kernel.Sm_Claim(SemId, uMT_NOWAIT) == E_SUCCESS)
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


static void Consumer()
{
	TaskId_t myTid;
	Kernel.Tk_GetMyTid(myTid);

	Serial.print(F("Consumer: [Tid="));
	Serial.print(myTid);
	Serial.print(F("] ALL_EVENTS=0x"));
	Serial.println((Event_t)ALL_EVENTS, HEX);
	Serial.flush();


	// Clear consumed parts
	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		Consumed[idx] = 0;


	while (1)
	{
		Event_t	eventout = 0;

		// Wait for some event signalling the availability of a produced assembly part
		CheckError(F("Consumer"), Kernel.Ev_Receive((Event_t)ALL_EVENTS, uMT_ANY, &eventout));

		// At least an Event has been set

		for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		{
			Event_t	eventwrk;

			// // load/reload/update event flags
			if ((idx % uMT_DEFAULT_SEM_NUM) == 0)
				eventwrk = eventout;			// load/reload event flags
			else
				eventwrk >>= 1;

	
			Bool_t EventSet = (eventwrk & 0x01);

			ConsumerPrint(F("> Consumer(t="));
			ConsumerPrint(millis());
			ConsumerPrint(F("): producer = "));
			ConsumerPrint(idx);
			ConsumerPrint(F(", eventwrk = 0x"));
			ConsumerPrintValueln(eventwrk, HEX);

			if (EventSet)
			{
				ConsumerCollect(idx, Consumed[idx]);
			}

		}
	}
}


#define MONITOR_TIMEOUT			3000			// 2 Seconds
#define MONITOR_PRINT_KERNEL	5
#define MONITOR_PK_AFTER	0

static void Monitor()
{
	unsigned int	counter = 0;
	TaskId_t myTid;

	Kernel.Tk_GetMyTid(myTid);

	while (1)
	{

		CheckError(F("Kernel.Tm_WakeupAfter(MONITOR_TIMEOUT)"), Kernel.Tm_WakeupAfter(MONITOR_TIMEOUT)); 	// Wake up after ... seconds

#if MONITOR_PK_AFTER==0
		if ((counter % MONITOR_PRINT_KERNEL) == 0)
		{
			Kernel.Kn_PrintInternals(TRUE);
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

		// Compare new with old
		for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		{
			if (Consumed[idx] <= OldConsumed[idx])
			{
				Serial.print(F("Consumer["));
				Serial.print(idx);
				Serial.println(F("] not consuming!!!!"));
			}
		}
		Serial.flush();

		// Copy new on Old
		CpuStatusReg_t	CpuFlags = Kernel.isr_Kn_IntLock();	/* Enter critical region */

		memcpy(OldConsumed, Consumed, sizeof(OldConsumed));

		Kernel.isr_Kn_IntUnlock(CpuFlags);


		counter++;
		
#if MONITOR_PK_AFTER==1
		if ((counter % MONITOR_PRINT_KERNEL) == 0)
		{
			Kernel.Kn_PrintInternals(TRUE);
		}
#endif

	}
}


void LOOP()		// TASK TID=1
{
	Errno_t error;
	TaskId_t TidSupplier;
	TaskId_t TidProducer[MAX_PRODUCERS];

	
#ifdef ZAPPED
	Kernel.Tk_GetMyTid(TidConsumer);

	Serial.print(F("LOOP(): TidConsumer = "));
	Serial.println(TidConsumer);
	Serial.flush();
#endif

	Serial.println(F("LOOP(): Kernel.Tk_CreateTask()"));

	CheckError(F("Kernel.Tk_CreateTask(Supplier, TidSupplier)"), Kernel.Tk_CreateTask(Supplier, TidSupplier));

	SidSupplier = (SemId_t)TidSupplier;

	CheckError(F("Kernel.Tk_CreateTask(Consumer, TidConsumer)"), Kernel.Tk_CreateTask(Consumer, TidConsumer));

	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
	{
		CheckError(F("Kernel.Tk_CreateTask(Farmer, TidProducer)"), Kernel.Tk_CreateTask(Producer, TidProducer[idx]));
	}


	Serial.print(F("LOOP(): Active TASKS = "));
	Serial.println(Kernel.Tk_GetActiveTaskNo());
	Serial.flush();


	// Set parameters for Producers
	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		Kernel.Tk_SetParam(TidProducer[idx], idx);		// In Param, the task index (not the Tid)

	Serial.println(F("LOOP(): StartTask()")); Serial.flush();

	CheckError(F("Kernel.Tk_StartTask(TidSupplier)"), Kernel.Tk_StartTask(TidSupplier));
	CheckError(F("Kernel.Tk_StartTask(TidConsumer)"), Kernel.Tk_StartTask(TidConsumer));

	for (int idx = 0; idx < MAX_PRODUCERS; idx++)
		CheckError(F("Kernel.Tk_StartTask(TidProducer)"), Kernel.Tk_StartTask(TidProducer[idx]));

	Serial.println(F("LOOP(): StartTask() completed")); Serial.flush();


	Kernel.Kn_PrintInternals();

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