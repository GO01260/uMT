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
#if TEST_KERNEL_AVR_LOW_LEVEL==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	KERNEL_LOW_LEVEL_setup()
#define LOOP()	KERNEL_LOW_LEVEL_loop()
#endif

#include <Arduino.h>

#include <uMT.h>

#if defined(ARDUINO_ARCH_AVR)

#define LED_PIN					(13)	// Arduino board LED pin

extern unsigned int __bss_start;
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

void Print_FreeRam()
{
	// Save Old SP value
	uint16_t *StackPtr = (uint16_t *)(SP);

	Serial.println(F(""));
    Serial.print(F("Free memory = "));
	Serial.println(Kernel.Kn_GetFreeRAM());

    Serial.print(F("  __bss_end = 0x"));
	Serial.println((uint16_t)&__bss_end, HEX);

    Serial.print(F("_heap_start = 0x"));
	Serial.println((uint16_t)&__heap_start, HEX);

	Serial.print(F("   __brkval = 0x"));
	Serial.println((uint16_t)__brkval, HEX);

	Serial.print(F("         SP = 0x"));
	Serial.println((uint16_t)StackPtr, HEX);

	Serial.print(F("         SP = "));
	Serial.println((uint16_t)StackPtr);

	Serial.print(F("    RAM END = "));
	Serial.println((uint16_t)Kernel.Kn_GetRAMend());

	Serial.println("");

}





void SETUP()
{
  // put your setup code here, to run once:

    Serial.begin(57600);
    Serial.println(F("Initialising..."));
    delay(100); //Allow for serial print to complete.

    Serial.println(F("Initialisation complete."));
    delay(100); //Allow for serial print to complete.


	Print_FreeRam();

	Kernel.Kn_Start(FALSE);		// No timesharing, no LED blinking

}



void KLL_TaskLoop()
{
	int counter = 0;

	Serial.println(F("============== TaskLoop =============="));
	Serial.flush();

	Serial.println(F("Testing INTS with Suspend()"));
	Serial.flush();

	#define Global_Interrupt_Enable	0x80

	uint8_t oldSREG = SREG;

	if (oldSREG & Global_Interrupt_Enable)
	{
		Serial.println(F("uMT: CheckInterrupts(1): INTERRUPTS enabled, SREG=0X"));
		Serial.flush();
	}

	cli();
 
	oldSREG = SREG;

	if (oldSREG & Global_Interrupt_Enable)
	{
		Serial.println(F("uMT: CheckInterrupts(2): INTERRUPTS enabled, SREG=0X"));
		Serial.flush();
 	}
	else
	{
		Serial.println(F("uMT: CheckInterrupts(2): INTERRUPTS DISabled, SREG=0X"));
		Serial.flush();
	}

	Kernel.Suspend();

	oldSREG = SREG;

	if (oldSREG & Global_Interrupt_Enable)
	{
		Serial.println(F("uMT: CheckInterrupts(3): INTERRUPTS enabled, SREG=0X"));
		Serial.flush();
 	}

	while (1)
	{
		Serial.println(F("Suspending..."));
		Serial.flush();
 
		Kernel.Suspend();

		digitalWrite(LED_BUILTIN, LOW);

		Serial.println(F("Restarting..."));
		Serial.flush();
 
		counter++;
		Serial.print(F("TaskLoop = "));
		Serial.println(counter);
		Serial.flush();

		delay(1000);
	}

	Serial.println(F("Stopping..."));
	delay(1000);


	while (1)
	{
	}

}




void KLL_MainLoop()
{
	int counter = 0;

	Serial.println(F("MyLoop() begin"));


 	Serial.print(F("sizeof(void *)"));
 	Serial.println(sizeof(void *));
	Serial.flush();
  
	Serial.println(F("NewTask()"));
	Serial.flush();
 
	Kernel.TaskList[2].SavedSP = Kernel.NewTask(Kernel.TaskList[2].StackBaseAddr, Kernel.TaskList[2].StackSize, KLL_TaskLoop, Kernel.BadExit);
	
	Serial.println(F("ResumeTask()"));
	Serial.flush();

  /* Now run task */
	Kernel.Running = &Kernel.TaskList[2];
	Kernel.Running->TaskStatus = S_RUNNING;
	
	Kernel.ResumeTask(Kernel.TaskList[2].SavedSP);


	while (1)
	{
		counter++;

		if (counter < 20)
		{
			Serial.print(F(" => "));
			Serial.println(counter);
			Serial.flush();
		}
  
		delay(1000);
	}
  
}



void LOOP()
{
	KLL_MainLoop();
}

#endif

////////////////////// EOF