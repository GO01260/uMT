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
#if TEST_INT_LATENCY_01==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	TEST_INT_LATENCY_01_setup()
#define LOOP()	TEST_INT_LATENCY_01_loop()
#endif

#include <Arduino.h>

#include <uMT.h>


#define TRACE_FLOW				0
#define TRACE_WITH_LED			0



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



#if defined(ARDUINO_ARCH_SAM)
#include "DueTimer.h"

#define SAM_USE_SYSTICK_TIMER	1

#if SAM_USE_SYSTICK_TIMER==1



// Using SysTick counter directly
// If you are guaranteed that you will always be measuring strictly less than a millisecond,
// one can read the SysTick counter directly. The counter counts down from 83999 to 0 with each clock tick.
// One can use it a bit like micros(), except that you have to handle the wraparound explicitly,
// and because it counts down the subtraction is the other way around.
// You could divide the result by 84 to get microseconds, but if you are after a specific delay 
// it would be better to multiply the delay you require by 84 to get it in clock ticks.


#ifdef ZAPPED
// SysTick example by stimmer

void setup() {
  Serial.begin(115200);
}

void loop() {
 
  int r=random(200);
  int v=SysTick->VAL;
  delayMicroseconds(r);
  v=v-SysTick->VAL;
  if(v<0)v+=84000;
  Serial.print("delayMicroseconds(");
  Serial.print(r);
  Serial.print(") took ");
  Serial.print(v);
  Serial.print(" clock ticks, which when divided by 84 equals ");
  Serial.println(v/84);
  delay(500);
 
}

#endif


#define MICROS()	SysTick->VAL
typedef uint32_t	SystemTick_t;

#else		// SAM_USE_SYSTICK_TIMER==0

#define MICROS()	micros()
typedef unsigned long	SystemTick_t;

#endif


#else	// ARDUINO_ARCH_AVR

#define SAM_USE_SYSTICK_TIMER	0

#define MICROS()	micros()
typedef unsigned long	SystemTick_t;

#endif

const uint8_t interruptPin = 2;
volatile 	SystemTick_t time0;
static TaskId_t Tid;

static volatile uint8_t	ISR_mode = 0;

#define ARDUINO_TID	1

#define EVENT_A	0x0001

static void interruptHandler()
{
	time0 = MICROS();

#if TRACE_WITH_LED==1
	digitalWrite(LED_BUILTIN, HIGH);
#endif

//	noInterrupts();

	digitalWrite(interruptPin, LOW);

//	interrupts();

	if (ISR_mode == 0)
		return;

	if (ISR_mode == 1)
		Kernel.isr_Ev_Send(ARDUINO_TID, EVENT_A);
	else
		Kernel.isr_p_Ev_Send(ARDUINO_TID, EVENT_A);
} 

static void InterruptSetup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(interruptPin, OUTPUT);

	digitalWrite(LED_BUILTIN, LOW);
	digitalWrite(interruptPin, LOW);

	attachInterrupt(digitalPinToInterrupt(interruptPin), interruptHandler, CHANGE);
//	attachInterrupt(digitalPinToInterrupt(interruptPin), interruptHandler, RISING);

} 


static void Producer()
{
	unsigned counter = 0;
	Serial.println(F("*** Producer(): started (using isr_Ev_Send()) ***"));
	Serial.flush();

	ISR_mode = 1;


	while (1)
	{
#if TRACE_FLOW==1
		Kernel.Kn_PrintInternals();

		delay(2000);
#endif

		Kernel.Tm_WakeupAfter(2000);

#if TRACE_FLOW==1
		Serial.println(F("*** Producer(): sending interrupts! ***"));
		Serial.flush();
#endif

		digitalWrite(interruptPin, HIGH);

		if (counter++ > 10  && ISR_mode == 1)
		{
			Serial.println(F("*** Producer(): switching to: isr_p_Ev_Send()) ***"));
			Serial.flush();
			ISR_mode = 2;
		}
	}
}

static void Receiver()
{
	unsigned counter = 0;

	while (1)
	{
		SystemTick_t elapsed;
		Event_t	eventout;

#if TRACE_FLOW==1
		Serial.println(F("Receiver(): Ev_Receive() ..."));
		Serial.flush();
#endif
		Errno_t error = Kernel.Ev_Receive(EVENT_A, uMT_ANY, &eventout);

		if (error != E_SUCCESS)
		{
			Serial.print(F("Receiver(): Ev_Receive() Failure! - returned "));
			Serial.println((unsigned)error);
			Serial.flush();

			delay(5000);

		}

#if SAM_USE_SYSTICK_TIMER==1
		// Read the timer: SysTick is counting down and we assume 1ms systick resolution
		elapsed = (time0 - MICROS()) / 84;
#else
		// Read the timer
		elapsed = MICROS() - time0;
#endif

		Serial.print(F("Receiver(): counter = "));
		Serial.print(counter++);
		Serial.print(F(" - Elapsed micros = "));
		Serial.println(elapsed);
		Serial.flush();

//		digitalWrite(interruptPin, LOW);

#if TRACE_WITH_LED==1
		digitalWrite(LED_BUILTIN, LOW);
#endif

	}
}



void SETUP() 
{
	// put your setup code here, to run once:

	Serial.begin(57600);

	Serial.println(F("MySetup(): Initialising..."));
	delay(100); //Allow for serial print to complete.

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));


	Serial.println(F("========== INTERRUPTS LATENCY TEST ============= "));


}

void LoopWithoutKernel(int idx)
{
	unsigned long elapsed;

#if TRACE_FLOW==1
	Serial.println(F("LoopWithoutKernel(): starting..."));
	Serial.flush();
#endif


#if defined(ARDUINO_ARCH_AVR)
	delay(1000);
	time0 = 0L;

	digitalWrite(interruptPin, HIGH);
#else
	time0 = 0L;

#endif

	while (time0 == 0)
		;

#if SAM_USE_SYSTICK_TIMER==1
		// Read the timer: SysTick is counting down and we assume 1ms systick resolution
		elapsed = (time0 - MICROS()) / 84;
#else
		// Read the timer
		elapsed = MICROS() - time0;
#endif
		

	Serial.print(F("No uMT: counter = "));
	Serial.print(idx);
	Serial.print(F(" - Elapsed micros = "));
	Serial.println(elapsed);
	Serial.flush();


#if TRACE_WITH_LED==1
	digitalWrite(LED_BUILTIN, LOW);
#endif
}


void LOOP()		// TASK TID=1
{
#if defined(ARDUINO_ARCH_SAM)
	
	Timer3.attachInterrupt(interruptHandler);
	Timer3.start(1000000); // Calls every 1000ms

#else		// AVR

	// Setup Interrupt handler
	InterruptSetup();
#endif

	for (int idx = 0; idx < 10; idx++)
		LoopWithoutKernel(idx);


	uMTcfg Cfg;

	Kernel.Kn_GetConfiguration(Cfg);
	
	Cfg.rw.BlinkingLED = FALSE;
//	Cfg.rw.TimeSharingEnabled = FALSE;

	CheckError(F("Setup"), Kernel.Kn_Start(Cfg));

	// Print works only AFTER Start
	Kernel.Kn_PrintConfiguration(Cfg);

	delay(2000);

	Serial.println(F("MySetup(): => Kernel.Kn_Start()"));
	Serial.flush();

	Kernel.Kn_Start(FALSE);		// No Timesharing


	// Create Interrupts Producer task
	Kernel.Tk_CreateTask(Producer, Tid);

	// Lower producer priority otherwise only Time Sharing will trigger a task switch...
	TaskPrio_t OldPrio;
	Kernel.Tk_SetPriority(Tid, PRIO_NORMAL - 1, OldPrio);

	// Start Producer
 	Kernel.Tk_StartTask(Tid);

	Receiver();

}


////////////////////// EOF