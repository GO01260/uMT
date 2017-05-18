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



#if defined(ARDUINO_ARCH_SAM)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	iKn_Reboot - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::iKn_Reboot()
{
	NoResched++;		// Prevent rescheduling....

	while (1)
		;
}


///////////////////////////////////////////////////////////////////////////////////
//
// This file contains CPU dependendent routines (Stack and ask Switches, Interrupts disabling/enabling, ...
// 
///////////////////////////////////////////////////////////////////////////////////



static StackPtr_t __attribute__((naked)) __attribute__((noinline)) uMT::GetSP()
{
	asm ("mov   r0, sp;");
}

#ifdef ZAPPED		// Alternative version
static StackPtr_t __attribute__((naked)) __attribute__((noinline)) uMT::GetSP()
{
	register unsigned char * stack_ptr asm ("sp");

	return(stack_ptr);
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
	void		(*TaskStartAddr)() = 

	// Stack is growing down...

	// Arduino UNO is using a byte aligned Stack, so new SP is...
	TaskStack += (StackSize - 1);

	uint16_t Addr = (uint16_t)BadExit;	// Last return is BadExit()...
	*TaskStack-- = Addr & 0xff;
	*TaskStack-- = Addr >> 8;

#if defined(__AVR_ATmega2560__)
	*TaskStack-- = 0;
#endif

	Addr = (uint16_t)TaskStartAddr;	// Start address of the task
	*TaskStack-- = Addr & 0xff;
	*TaskStack-- = Addr >> 8;

#if defined(__AVR_ATmega2560__)
	*TaskStack-- = 0;
#endif


	// Store registers in the stack + SREG
	for (int a = 0; a < 33; a++)
	{
		*TaskStack-- = (uint8_t)0;
	}

	return(TaskStack);
}

#ifdef ZAPPED
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
//	IntLock - ARDUINO_UNO/MEGA
//
// It returns the status register & disables INTERRUPT (GLOBAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CpuStatusReg_t uMT::IntLock()
{
	uint8_t oldSREG = SREG;

	cli();		/* No interrupts now! */


//	CheckInterrupts(F("IntLock"));

	return((CpuStatusReg_t)oldSREG);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	IntUnlock - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::IntUnlock(CpuStatusReg_t Param)
{

//	sei();

	SREG = Param;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeSRAM - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <malloc.h>

extern char _end;
extern "C" char *sbrk(int i);

uint16_t uMT::Kn_GetFreeSRAM()
{
  char *ramstart = (char *) 0x20070000;
  char *ramend = (char *) 0x20088000;
  char *heapend = sbrk(0);
  register char * stack_ptr asm( "sp" );
  struct mallinfo mi = mallinfo();

#ifdef ZAPPED
  Serial << "Ram used (bytes): " << endl
    << "  dynamic: " << mi.uordblks << endl
    << "  static:  " << &_end - ramstart << endl
    << "  stack:   " << ramend - stack_ptr << endl;
  Serial << "Estimation free Ram: " << stack_ptr - heapend + mi.fordblks << endl;
#endif

}




/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetSPbase - ARDUINO_UNO/MEGA
//
/////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t uMT::Kn_GetSPbase()
{

}

#endif
#endif


////////////////////// EOF