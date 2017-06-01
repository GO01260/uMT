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
#if TEST_KERNEL_SAM_LOW_LEVEL==1
#define SETUP()	setup()
#define LOOP()	loop()
#else
#define SETUP()	KERNEL_LOW_LEVEL_setup()
#define LOOP()	KERNEL_LOW_LEVEL_loop()
#endif

#include <Arduino.h>

#include <uMT.h>

#if defined(ARDUINO_ARCH_SAM)

#define LED_PIN					(13)	// Arduino board LED pin


void SETUP()
{
    Serial.begin(57600);

	Serial.println(F(" "));
	Serial.println(F(" "));
	Serial.println(F(" "));
	Serial.println(F("================= Test00_SAM_KernelLowLevel test ================="));
	delay(100); //Allow for serial print to complete.

	Serial.print(F("Compile Date&Time = "));
	Serial.print(F(__DATE__));

	Serial.print(F(" "));
	Serial.println(F(__TIME__));
	Serial.flush();

//	Kernel.Kn_Start(FALSE, FALSE);		// No timesharing, no LED blinking
	Kernel.Kn_Start(FALSE);		// No timesharing

	Kernel.Kn_PrintInternals();
}


#define SAM_SF_SIZE	17			// 17 registers/words totals

extern uint32_t	SAM_StackFrame[];
extern Bool_t	SAM_StackFrame_inited;

const char *SF_names[] =
{
	"R04",
	"R05",
	"R06",
	"R07",
	"R08",
	"R09",
	"R10",
	"R11",
	"LR_EXC",
	"R0",
	"R1",
	"R2",
	"R3",
	"R12",
	"LR",
	"Saved PC",
	"PSR"
};

uint32_t __attribute__((noinline)) GetPC()
{
	asm volatile ("mov r0, pc;");
}


void PrintStackFrame(uint32_t StackFrame[])
{
	for (int idx = SAM_SF_SIZE - 1; idx >= 0; idx--)
	{
		Serial.print(F("SAM SF["));
		Serial.print(idx);
		Serial.print(F("] ("));
		Serial.print(SF_names[idx]);
		Serial.print(F(")=0x"));
		Serial.println(StackFrame[idx], HEX);
	}

}




#define TEST_INTS	0

static uint32_t __attribute__ ((noinline)) ReadPRIMASK()
{
	asm volatile ("mrs r0, PRIMASK");
}

#define Global_Interrupt_Enable	0x00000001

static void CheckInterrupts(const __FlashStringHelper *String)
{
	uint32_t oldSREG = ReadPRIMASK();

	if ((oldSREG & Global_Interrupt_Enable) == 0)
	{
		Serial.print(F("CheckInterrupts(): INTERRUPTS enabled, SREG=0X"));
		Serial.print(oldSREG, HEX);
		Serial.print(F(" Func="));
		Serial.println(F(String));
		Serial.flush();
	}
}


void KLL_TaskLoop()
{
	int counter = 0;

	Serial.println(F("============== TaskLoop =============="));
	Serial.flush();

#if TEST_INTS==1
	Serial.println(F("Testing INTS with Suspend()"));
	Serial.flush();

	CheckInterrupts(F("1 - MUST be ENABLED!"));

	Serial.println(F("Disabling INTS..."));
	Serial.flush();

	CpuStatusReg_t	CpuFlags = Kernel.isr_Kn_IntLock();	/* Enter critical region */


	uint32_t oldSREG = ReadPRIMASK();

	Kernel.isr_Kn_IntUnlock(CpuFlags);

	if (oldSREG == 1)
	{
		Serial.print(F("INTS correctly DISABLED => SREG=0X"));
	}
	else
	{
		Serial.print(F("INTS incorrectly ENABLED! => SREG=0X"));
	}

	Serial.print(oldSREG, HEX);
	Serial.println(F(" Func=2"));
	Serial.flush();


	Serial.println(F("Calling  Kernel.Suspend()..."));
	Serial.flush();


	Kernel.Suspend();

	CheckInterrupts(F("3 - MUST be ENABLED!"));
#endif


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
		Serial.print(counter);

		Serial.print(F(" - GetTickCount()="));
		Serial.print(GetTickCount());

		Serial.print(F("    Kernel.isr_Kn_GetKernelTick()="));
		Serial.println(Kernel.isr_Kn_GetKernelTick());

		Serial.flush();

		delay(1000);
	}

	Serial.println(F("Stopping..."));
	delay(1000);


	while (1)
	{
	}

}


static void reboot()
{
	Serial.println(F("reboot()"));
	Serial.flush();

	delay(2000);

	Kernel.isr_Kn_Reboot();
}



void KLL_MainLoop()
{
	int counter = 0;

	Serial.println(F("MyLoop() begin"));


	Serial.print(F("SP=0x"));
	Serial.println(Kernel.Kn_GetSP(), HEX);



	Serial.println(F("NewTask()"));
	Serial.flush();
 
	Kernel.TaskList[2].SavedSP = Kernel.NewTask(Kernel.TaskList[2].StackBaseAddr, Kernel.TaskList[2].StackSize, KLL_TaskLoop, Kernel.BadExit);


	PrintStackFrame((uint32_t *)Kernel.TaskList[2].SavedSP);

	Serial.print(F("KLL_TaskLoop=0x"));
	Serial.println((uint32_t)KLL_TaskLoop, HEX);

	Serial.print(F("Kernel.BadExit=0x"));
	Serial.println((uint32_t)Kernel.BadExit, HEX);
	Serial.flush();


//	Kernel.Kn_PrintInternals();



#ifdef ZAPPED
	for (idx = 0; idx < 5; idx++)
	{
		Serial.print(F("alive... GetTickCount()="));
		Serial.print(GetTickCount());

		Serial.print(F("    Kernel.isr_Kn_GetKernelTick()="));
		Serial.println(Kernel.isr_Kn_GetKernelTick());
		Serial.flush();

		delay(1000);
	}

//	reboot();
#endif

	delay(2000);




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




void KLL_SP_Frame()
{
	while (SAM_StackFrame_inited == FALSE)
	{
		Serial.println(F("SAM_StackFrame_inited => FALSE"));
		Serial.flush();
		delay(1000);
	}

	Serial.println(F("SAM_StackFrame_inited => TRUE"));
	Serial.flush();

	Serial.print(F("SAM SP=0x"));
	Serial.println(SAM_StackFrame[SAM_SF_SIZE], HEX);

	PrintStackFrame(SAM_StackFrame);

	register uint32_t stack_ptr asm ("sp");

	Serial.print(F("Current SP=0x"));
	Serial.println(stack_ptr, HEX);

	Serial.print(F("Current PC=0x"));
	Serial.println(GetPC(), HEX);

}


void LOOP()
{


	KLL_SP_Frame();

//	KLL_TaskLoop();

	KLL_MainLoop();
}

#endif

////////////////////// EOF