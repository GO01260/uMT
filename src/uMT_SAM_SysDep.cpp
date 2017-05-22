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



#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)

#if defined(ARDUINO_ARCH_SAM) 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUSH stores registers on the stack in order of decreasing the register numbers, with the highest numbered
// register using the highest memory address and the lowest numbered register using the lowest memory address.
//
// POP loads registers from the stack in order of increasing register numbers, with the lowest numbered register
// using the lowest memory address and the highest numbered register using the highest memory address.
//
// LR is register #14
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// It saves all the General Purposes registers (13) on the STACK, plus the LR register (14 registers total)
#define iMT_ISR_Entry()	asm volatile ("push   {r0-r12, lr};")

// It restores all the General Purposes registers (13) from the STACK, plus the LR register (14 registers total),
// renable INTS and thend jump back to the caller (in LR)
#define iMT_ISR_Exit()	asm volatile ("pop   {r0-r12, lr};" "cpsie i;" "bx    lr;")

#else		// ARDUINO_ARCH_SAMD: note this code NEVER tested!!!!!!

#define iMT_ISR_Entry()	\
	asm (						\
	"mov   r12, sp;"			\
#if defined(ARDUINO_ARCH_SAMD)	\
	/* store low registers */	\
	"stmia sp, {r0-r7};"		\
	/* store high registers */	\
	/* move them to low registers first. */	\
	"mov   r1, r8;"				\
	"mov   r2, r9;"				\
	"mov   r3, r10;"			\
	"mov   r4, r11;"			\
	"mov   r5, r12;"			\
	"mov   r6, lr;"				\
	"stmia sp, {r1-r6};"		\
	);
#endif




///////////////////////////////////////////////////////////////////////////////////
//
// This file contains CPU dependendent routines (Stack and ask Switches, Interrupts disabling/enabling, ...
// 
///////////////////////////////////////////////////////////////////////////////////



StackPtr_t __attribute__((noinline)) uMT::Kn_GetSP()
{
	asm volatile ("mov   r0, sp;");
}



#ifdef ZAPPED		// Alternative version
static StackPtr_t __attribute__((noinline)) uMT::GetSP()
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
	StackPtr_t	TaskStackBase,
	StackSize_t	StackSize, 
	void		(*TaskStartAddr)(),
	void		(*BadExit)()
	)
{
	// Stack is growing down...

	// Arduino DUE is using a 4 bytes aligned Stack, so new SP is...
	TaskStackBase += (StackSize - 4);

	TaskStackBase &= 0xFFFFFFFC;	// Align to 32 bits boundary.

	uint32_t *StackPointer = (uint32_t *)TaskStackBase;


	// Last return is BadExit()...
	*--StackPointer = (uint32_t)BadExit;

	// It will be loaded in LR register
	*--StackPointer = (uint32_t)TaskStartAddr;


	// Store registers in the stack
	for (int a = 0; a < 13; a++)
	{
		*--StackPointer = 0;
	}

	return((StackPtr_t)StackPointer);

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	doResumeTask
//
// This never returns!!!!!
/////////////////////////////////////////////////////////////////////////////////////////////////
static void __attribute__((naked)) __attribute__ ((noinline)) doResumeTask(StackPtr_t StackPtr)
{
	asm volatile ("mov   sp, r0;");		// Reload SP (StackPtr in register 0)

	iMT_ISR_Exit();
	
	// The above will reload all the registers, including the SREG
	//
	// After resume, INTS enabled!!!!
	//
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	ResumeTask
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::ResumeTask(StackPtr_t StackPtr)
// void  uMT::ResumeTask(StackPtr_t StackPtr)
{
	// Clear Timesharing, but only if we have done a task switch...
	if (Running != LastRunning)
	{
		TimeSlice = (Running == IdleTaskPtr ? uMT_IDLE_TIMEOUTVALUE : uMT_TICKS_TIMESHARING);
	}

#if uMT_USE_TIMERS==1
	// Clear Alarm expired, just in case
	AlarmExpired = FALSE;
#endif

	doResumeTask(StackPtr);
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
	
	register StackPtr_t stack_ptr asm ("sp");

	Running->SavedSP = stack_ptr;	// Save Task's Stack Pointer

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

	// Disable INTS
	asm volatile ("cpsid i");

	// Interrupts disabled

	NoResched = 1;		// Prevent further rescheduling.... until next 

	Reschedule();

	// Never returns...
}

		

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_IntLock - ARDUINO_SAM
//
// It returns the status register & disables INTERRUPT (GLOBAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CpuStatusReg_t uMT::isrKn_IntLock()
{
	// This function disables IRQ interrupts by setting the I-bit in the CPSR.
	// It can only be executed in Privileged modes.
	asm volatile ("mrs r0, PRIMASK; cpsid i");

//	asm ("cpsid i");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_IntUnlock - ARDUINO_SAM
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::isrKn_IntUnlock(CpuStatusReg_t Param)
{
	// This function enables IRQ interrupts by clearing the I-bit in the CPSR.
	// It can only be executed in Privileged modes.
	asm volatile ("msr PRIMASK, r0;");


#ifdef ZAPPED
	if (Param == 0)		// Re-enable interrupts
	{
		asm volatile ("cpsie i");
	}
#endif
}




#ifdef ZAPPED
extern char _end;
char *ramstart=(char *)0x20070000;
char *ramend=(char *)0x20088000;
#endif

extern "C" char *sbrk(int i);


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeSRAM - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetFreeRAM()
{
 	char *heapend=sbrk(0);
	register uint32_t stack_ptr asm ("sp");		// variable PRIMASK is associated to sp
	
	return(stack_ptr - (uint32_t)heapend);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeRAMend - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetFreeRAMend()
{
	char *heapend=sbrk(0);
	
	return(Kn_GetRAMend() - (uint32_t)heapend);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetSPbase - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetSPbase()
{
	return((StackPtr_t)sbrk(0));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetRAMend - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetRAMend()
{
#if defined(ARDUINO_ARCH_SAMD)
	return((StackPtr_t)0x20088000);		?????? // Incorrect, only 32KB RAM
#else
	return((StackPtr_t)0x20088000);
#endif
}


#if uMT_SAFERUN==1

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	ReadPRIMASK - ARDUINO_SAM
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t __attribute__ ((noinline)) ReadPRIMASK()
{
	asm volatile ("mrs r0, PRIMASK");
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckInterrupts - ARDUINO_SAM
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::CheckInterrupts(const __FlashStringHelper *String)
{

	uint32_t oldSREG = ReadPRIMASK();

#define Global_Interrupt_Enable	0x00000001

	if ((oldSREG & Global_Interrupt_Enable) == 0)
	{
		Serial.print(F("!uMT: CheckInterrupts(): INTERRUPTS incorrectly enabled, PRIMASK=0X"));
		Serial.print(oldSREG, HEX);
		Serial.flush();
		Serial.print(F(" Func="));
		Serial.println(F(String));
		Serial.flush();

		isrKn_FatalError();
	}
}

#endif

#ifdef ZAPPED
volatile uint8_t SR_reg;               /* Current value of the FAULTMASK register */
volatile uint8_t SR_lock = 0x00U;      /* Lock */
 
/* Save status register and disable interrupts */
#define EnterCritical() \
  do {\
    if (++SR_lock == 1u) {\
      /*lint -save  -e586 -e950 Disable MISRA rule (2.1,1.1) checking. */\
      asm ( \
      "MRS R0, PRIMASK\n\t" \
      "CPSID i\n\t"            \
      "STRB R0, %[output]"  \
      : [output] "=m" (SR_reg)\
      :: "r0");\
      /*lint -restore Enable MISRA rule (2.1,1.1) checking. */\
  }\
} while(0)
 
/* Restore status register  */
#define ExitCritical() \
  do {\
    if (--SR_lock == 0u) { \
      /*lint -save  -e586 -e950 Disable MISRA rule (2.1,1.1) checking. */\
      asm (                 \
      "ldrb r0, %[input]\n\t"\
      "msr PRIMASK,r0;\n\t" \
      ::[input] "m" (SR_reg)  \
      : "r0");                \
      /*lint -restore Enable MISRA rule (2.1,1.1) checking. */\
    }\
  } while(0)
#endif


#endif


////////////////////// EOF