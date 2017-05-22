////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTarduinoSysDep.cpp
//	AUTHOR: Antonio Pastore - April 2017
//	Program originally written by Antonio Pastore, Torino, ITALY.
//	UPDATED: 28 April 2017
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


#include <Arduino.h>

#include "uMT.h"

#include "uMTdebug.h"


#if defined(ARDUINO_ARCH_AVR)

#include "uMTarduinoAVR.h"

///////////////////////////////////////////////////////////////////////////////////
//
// This file contains CPU dependendent routines (Stack and ask Switches, Interrupts disabling/enabling, ...
// 
///////////////////////////////////////////////////////////////////////////////////

#ifdef ZAPPED
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iMTSwitchStack - ARDUINO_UNO
//
// We need to swith to a private, temporay stack for MT, usually when a task commit suicide...
// MTConfig.MTStack points to the first available, empty location
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::SwitchStack()
{

	static uint8_t *StackPtr;
	static uint8_t *newStack = MTStack;

	cli();	// Disable interrupts

	// Save Old SP value
	StackPtr =  (uint8_t *)(SP);

	// Restore old return adress
	*newStack-- = StackPtr[2];
	*newStack-- = StackPtr[1];
#if defined(__AVR_ATmega2560__)
	*TaskStack-- = 0;
#endif
	// Create a new Stack
	SP = newStack;

	sei(); // Renable interrupts

}
#endif




/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	NewTask - ARDUINO_UNO
//
// Setup new Stack for this stack, ready to be restarted
//
//	TaskStack points to the first available, empty location
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t __attribute__ ((noinline)) uMT::NewTask(
	StackPtr_t	TaskStack,
	StackSize_t	StackSize, 
	void		(*TaskStartAddr)(),
	void		(*BadExit)()
	)
{
	// Stack is growing down...
	// Arduino UNO is using a byte aligned Stack, so new SP is...
	TaskStack += (StackSize - 1);

	uint8_t	*pStack = (uint8_t	*)TaskStack;

	uint16_t Addr = (uint16_t)BadExit;	// Last return is BadExit()...
	*pStack-- = Addr & 0xff;
	*pStack-- = Addr >> 8;

#if defined(__AVR_ATmega2560__)
	*pStack-- = 0;
#endif

	Addr = (uint16_t)TaskStartAddr;	// Start address of the task
	*pStack-- = Addr & 0xff;
	*pStack-- = Addr >> 8;

#if defined(__AVR_ATmega2560__)
	*pStack-- = 0;
#endif


	// Store registers in the stack + SREG
	for (int a = 0; a < 33; a++)
	{
		*pStack-- = (uint8_t)0;
	}

	return((StackPtr_t)pStack);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	ResumeTask
//
// This never returns!!!!!
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::ResumeTask(StackPtr_t StackPtr)
{
	static StackPtr_t	_StackPtr;

	// Clear Timesharing, but only if we have done a task switch...
	if (Running != LastRunning)
	{
		TimeSlice = (Running == IdleTaskPtr ? uMT_IDLE_TIMEOUTVALUE : uMT_TICKS_TIMESHARING);
	}

#if uMT_USE_TIMERS==1
	// Clear Alarm expired, just in case
	AlarmExpired = FALSE;
#endif

	_StackPtr = StackPtr;		// Save it in a NOT Stack Pointer dependent location

	cli();		/* No interrupts now! */

	// Set the new Stack Pointer
	SP = _StackPtr;
 
	iMT_ISR_Exit();
	
	// The above will reload all the registers, including the SREG
	//
	// After resume, INTS enabled!!!!
	//
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Suspend
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::Suspend()
{
	// Create a STACK like an ISR
	iMT_ISR_Entry();

//	iMT_ISR_Exit();				// For testing purpose only...

	cli();		/* No interrupts now! */

//	SavedStackPtr = SP;		// Save Task's Stack Pointer, testing version

	Running->SavedSP = SP;	// Save Task's Stack Pointer

	Suspend2();

	// Never returns...
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Suspend2
//
// This is called both from Suspend() and from TimerTicks ISR routine if a reschedule is needed
// 
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::Suspend2()
{

	// On entry:
	// iMT_ISR_Entry performed
	// SP saved
	// Interrupts disabled

	NoResched = 1;		// Prevent further rescheduling.... until next 

	Reschedule();

	// Never returns...
}

		

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_IntLock - ARDUINO_UNO/MEGA
//
// It returns the status register & disables INTERRUPT (GLOBAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CpuStatusReg_t uMT::isrKn_IntLock()
{
	uint8_t oldSREG = SREG;

	cli();		/* No interrupts now! */


//	CheckInterrupts(F("isrKn_IntLock"));

	return((CpuStatusReg_t)oldSREG);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_IntUnlock - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::isrKn_IntUnlock(CpuStatusReg_t Param)
{

//	sei();

	SREG = Param;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeSRAM - ARDUINO_UNO/MEGA
//
/////////////////////////////////////////////////////////////////////////////////////////////////
extern unsigned int __bss_start;
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

StackPtr_t uMT::Kn_GetFreeRAM()
{
	// Save Old SP value
	uint16_t StackPtr = (uint16_t)(SP);
	StackPtr_t FreeSRAM;

	if ((uint16_t)__brkval == 0)	// heap is empty, use bss as start memory address
		FreeSRAM = (StackPtr_t)((uint16_t)StackPtr - (uint16_t)&__bss_end);
		
	else							// use heap end as the start of the memory address
		FreeSRAM = (StackPtr_t)((uint16_t)StackPtr - (uint16_t)__brkval);

	return(FreeSRAM);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetSPbase - ARDUINO_UNO/MEGA
//
// Only for static STACK allocation
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetSPbase()
{
	if ((uint16_t)__brkval == 0)
		return ((uint16_t)&__bss_end);		// heap is empty, use bss as start memory address
	else 		
		return ((uint16_t)__brkval);		// Use heap end as the start of the memory address
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetRAMend - ARDUINO_UNO/MEGA
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetRAMend()
{
	return(RAMEND);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeRAMend - ARDUINO_UNO/MEGA
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetFreeRAMend()
{
	StackPtr_t FreeRAM;
	
	if ((uint16_t)__brkval == 0)	// heap is empty, use bss as start memory address
		FreeRAM = (StackPtr_t)((uint16_t)RAMEND - (uint16_t)&__bss_end);
		
	else							// use heap end as the start of the memory address
		FreeRAM = (StackPtr_t)((uint16_t)RAMEND - (uint16_t)__brkval);

	return(FreeRAM);
}




#if uMT_SAFERUN==1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckInterrupts - ARDUINO_UNO
//
// CheckInterrupts
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::CheckInterrupts(const __FlashStringHelper *String)
{
#define Global_Interrupt_Enable	0x80

	uint8_t oldSREG = SREG;

	if (oldSREG & Global_Interrupt_Enable)
	{
		Serial.print(F("uMT: CheckInterrupts(): INTERRUPTS incorrectly enabled, SREG=0X"));
		Serial.print(oldSREG, HEX);
		Serial.print(F(" Func="));
		Serial.println(String);
		Serial.flush();

		isrKn_FatalError();
	}
}

#endif


#endif


////////////////////// EOF