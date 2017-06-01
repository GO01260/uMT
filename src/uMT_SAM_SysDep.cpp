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

///////////////////////////////////////////////////////////////////////////////////
//
// This file contains CPU dependendent routines (Stack and ask Switches, Interrupts disabling/enabling, ...
// 
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUSH stores registers on the stack in order of decreasing the register numbers, with the highest numbered
// register using the highest memory address and the lowest numbered register using the lowest memory address.
//
// POP loads registers from the stack in order of increasing register numbers, with the lowest numbered register
// using the lowest memory address and the highest numbered register using the highest memory address.
//
// LR is register #14
//
// The saved/restored STACK frame is emulating the EXCEPTION processing + additional registers.
//
/////////////////////////////////////////////////////////
//	STACK after HARDWARE EXCEPTION (HW_EXCP) - 8 registers
//
//	TOP:	PSR
//			saved PC	== LR when NOT ISR
//			LR
//			R12
//			R3
//			R2
//			R1
//	SP:		R0
//
/////////////////////////////////////////////////////////
//	Addtional registers saved (9 registers)
//
//	TOP:	LR		(SAM_EXCEPTION_LR value, always)
//			R11
//			R10
//			R09
//			R08
//			R07
//			R06
//			R05
//	SP:		R04
//
/////////////////////////////////////////////////////////
//
// The initial EXC_RETURN is the bitmask value 0xFFFFFFF9 refecting the fact that we do
// not implement a separate stack pointer register for "system" and "user" mode (c.f. SAM3X8E
// datasheet section 16.6.7.6 page 86).
//
// The initial PSR must (PSR) 0x01000000
//
// SAM_EXCEPTION_LR = b1001 (0x9) = 
//		Return to Thread mode.
//		Exception return gets state from MSP.
//		Execution uses MSP after return.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



#define SAM_INITIAL_PSR		(0x01000000)
#define SAM_EXCEPTION_LR	(0xFFFFFFF9)

// Generate a "pendSVHook" exception
#define GeneratePendSVHook_int()	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk

#define EnableInterrupts() asm volatile ("cpsie i")
#define DisableInterrupts() asm volatile ("cpsid i")


/////////////////////////////////////////////////////////////////////////////////////////////////
// Kernel PRIVATE STACK 
/////////////////////////////////////////////////////////////////////////////////////////////////
static 	StackPtr_t KernelStack[uMT_KERNEL_STACK_SIZE];


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::NewStackReschedule - ARDUINO_SAM
//
// Switch to private stack and call Reschedule();
// Entered with INTS disabled!
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__((noinline)) uMT::NewStackReschedule()
{
	static StackPtr_t	TaskStackBase = (StackPtr_t)&KernelStack[uMT_KERNEL_STACK_SIZE - 4];		// Load PRIVATE Kernel STACK

	// Arduino DUE is using a 4 bytes aligned Stack, so new SP is...
	TaskStackBase &= 0xFFFFFFFC;	// Align to 32 bits boundary.

	register StackPtr_t stack_ptr asm ("sp");

	stack_ptr = (StackPtr_t)TaskStackBase;		// Load PRIVATE Kernel STACK

	// We are using Kernel Private STACK
	KernelStackMode = TRUE;

	// Call Reschedule
	Reschedule();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::NewStackReborn - ARDUINO_SAM
//
// Switch to private stack and call Reborn();
// Entered with INTS disabled!
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__((noinline)) uMT::NewStackReborn()
{
	static StackPtr_t	TaskStackBase = (StackPtr_t)&KernelStack[uMT_KERNEL_STACK_SIZE - 4];		// Load PRIVATE Kernel STACK

	// Arduino DUE is using a 4 bytes aligned Stack, so new SP is...
	TaskStackBase &= 0xFFFFFFFC;	// Align to 32 bits boundary.

	register StackPtr_t stack_ptr asm ("sp");

	stack_ptr = (StackPtr_t)TaskStackBase;		// Load PRIVATE Kernel STACK

	// We are using Kernel Private STACK
	KernelStackMode = TRUE;

	// Call Reborn
	Kernel.Reborn();
}


#ifdef ZAPPED
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::NewStackReschedule - ARDUINO_UNO
//
// Switch to private stack and call Reschedule();
// Entered with INTS disabled!
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__((naked)) __attribute__ ((noinline))  uMT::Switch2PrivateStack()
{
	static StackPtr_t	TaskStackBase = (StackPtr_t)&KernelStack[uMT_KERNEL_STACK_SIZE];		// Load PRIVATE Kernel STACK

	// Arduino DUE is using a 4 bytes aligned Stack, so new SP is...
	TaskStackBase &= 0xFFFFFFFC;	// Align to 32 bits boundary.

	// Save Old SP value
	register StackPtr_t stack_ptr asm ("sp");

	stack_ptr = (StackPtr_t)TaskStackBase;		// Load PRIVATE Kernel STACK

	// And return to caller....
	asm volatile ("bx	lr;");

}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::Kn_GetSP - ARDUINO_SAM
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t __attribute__((noinline)) uMT::Kn_GetSP()
{
	asm volatile ("mov r0, sp;");
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	NewTask - ARDUINO_SAM
//
//	Setup new Stack for this stack, ready to be restarted
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
	// Stack is growing down... and SP points to the LAST used position

	TaskStackBase += (StackSize);
//	TaskStackBase += (StackSize - 4);

	// Arduino DUE is using a 4 bytes aligned Stack, so new SP is...
	TaskStackBase &= 0xFFFFFFFC;	// Align to 32 bits boundary.

	uint32_t *StackPointer = (uint32_t *)TaskStackBase;


	// Load Initial PSR
	*--StackPointer = SAM_INITIAL_PSR;
	
	// Load Initial PC
	*--StackPointer = (uint32_t)TaskStartAddr;

	// Last return is BadExit()...
	*--StackPointer = (uint32_t)BadExit;

	// Store registers r0-r3 + r12 in the stack
	for (int a = 0; a < 5; a++)
	{
		*--StackPointer = 0;
	}

	// Load SAM_EXCEPTION_LR
	*--StackPointer = SAM_EXCEPTION_LR;

	// Store registers r4-r11 in the stack
	for (int a = 0; a < 8; a++)
	{
		*--StackPointer = 0;
	}

	return((StackPtr_t)StackPointer);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	ResumeTask
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__((naked)) __attribute__ ((noinline)) uMT::ResumeTask(StackPtr_t StackPtr)
{
	asm volatile ("mov sp, r1;");		// Reload SP (StackPtr in register r1, r0 is class pointer)

	// We are using application STACK
	KernelStackMode = FALSE;

	// Re-enable INTS in case not enabled!!!!
	EnableInterrupts();

	// Restore all the General Purposes registers (13) from the STACK, 
	// plus the LR register (14 registers total) and thend jump back to the caller (in LR)
	asm volatile ("pop   {r4-r11, lr};" "bx	lr;");
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Suspend
//
// This is called from KENEL routine if a reschedule is needed
// Usually entered with INTS ENABLED
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::Suspend()
{
	// Disable INTS
	DisableInterrupts();

	// Force a rescheduling at the next pendSVHook()
	NeedResched = TRUE;
	NoPreempt = FALSE;

	// Enable INTS in case they are disabled
	EnableInterrupts();

	// The following will trigger an INTERRUPT and a reschedule will occur
	GeneratePendSVHook_int();

	// Returning to the caller only when this task is again the RUNNING task
}

	

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isr_Kn_IntLock - ARDUINO_SAM
//
// It returns the status register & disables INTERRUPT (GLOBAL)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CpuStatusReg_t uMT::isr_Kn_IntLock()
{
	// This function disables IRQ interrupts by setting the I-bit in the CPSR.
	// It can only be executed in Privileged modes.
	asm volatile ("mrs r0, PRIMASK; cpsid i");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isr_Kn_IntUnlock - ARDUINO_SAM
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void uMT::isr_Kn_IntUnlock(CpuStatusReg_t Param)
{
	// This function enables IRQ interrupts by clearing the I-bit in the CPSR.
	// It can only be executed in Privileged modes.
	asm volatile ("msr PRIMASK, r0;");
}


extern int  _end ;

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeSRAM - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetFreeRAM()
{
	register uint32_t stack_ptr asm ("sp");		// variable stack_ptr is associated to sp
	
	return(stack_ptr - (StackPtr_t)&_end);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetFreeRAMend - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetFreeRAMend()
{
	return(Kn_GetRAMend() - (StackPtr_t)&_end);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetSPbase - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetSPbase()
{
	return((StackPtr_t)&_end);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Kn_GetRAMend - ARDUINO_DUE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
StackPtr_t uMT::Kn_GetRAMend()
{
	return((StackPtr_t)0x20088000);
}


#if uMT_SAFERUN==1

#ifdef ZAPPED
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	ReadPRIMASK - ARDUINO_SAM
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t __attribute__ ((noinline)) ReadPRIMASK()
{
	asm volatile ("mrs r0, PRIMASK");
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CheckInterrupts - ARDUINO_SAM
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((noinline)) uMT::CheckInterrupts(const __FlashStringHelper *String)
{
#ifdef ZAPPED
	uint32_t oldSREG = ReadPRIMASK();
#else
	// R0 == THIS
	// R1 == String
	register uint32_t oldSREG asm ("r2");		// variable oldSREG is associated to r2
	asm volatile ("mrs r2, PRIMASK");
#endif

#define Global_Interrupt_Enable	0x00000001

	if ((oldSREG & Global_Interrupt_Enable) == 0)
	{
		Serial.print(F("!uMT: CheckInterrupts(): INTERRUPTS incorrectly enabled, PRIMASK=0X"));
		Serial.print(oldSREG, HEX);
		Serial.flush();
		Serial.print(F(" Func="));
		Serial.println(F(String));
		Serial.flush();

		isr_Kn_FatalError();
	}
}

#endif


#endif


////////////////////// EOF