////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTarduinoSysTick.cpp
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


extern void uMT_SystemTicks();
extern unsigned uMTdoTicksWork();


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

extern uint32_t GetTickCount();



// Generate a "pendSVHook" exception
#define GeneratePendSVHook_int()	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk
#define EnableInterrupts() asm volatile ("cpsie i")
#define DisableInterrupts() asm volatile ("cpsid i")



extern "C" {

#define USE_TICK_HOOD_SAM	1	// 0 not to use pendSVHook() in sysTickHook(): of course no task switching....


////////////////////////////////////////////////////////////////////////////////////
//
//	sysTickHook
//
// This is called by the ARM SysTick interrupt routine.
// It must return 0 so any other step is performed in the original SysTick_Handler
////////////////////////////////////////////////////////////////////////////////////
unsigned int  __attribute__((noinline)) sysTickHook()
{
#if USE_TICK_HOOD_SAM==1

	// Generate a "pendSVHook" exception
	if (uMT::Inited == TRUE)
		GeneratePendSVHook_int();

#endif

	return (0);
}


////////////////////////////////////////////////////////////////////////////////////
//
//	pendSVHook
//
// This is executed when "pendSVHook" exception is generated.
//
////////////////////////////////////////////////////////////////////////////////////
void __attribute__ ((naked)) __attribute__((noinline)) pendSVHook(void)
{
	//  On entry, HW_EXCP is already saved 

	// Save the remaining registers
	asm volatile ("push {r4-r11,lr}");


	uint32_t CurrentTick = GetTickCount();


	if (CurrentTick < Kernel.TickCounter.Low) // RollOver..
				Kernel.TickCounter.High++;


	Kernel.TickCounter.Low = CurrentTick;		// Simply copy ticks counter...


	if (Kernel.kernelCfg.BlinkingLED)
	{
		if ((Kernel.TickCounter % uMT_TICKS_SECONDS) == 0)
		{
			volatile static Bool_t f_led = TRUE;

			if (f_led == TRUE)
			{
				f_led = FALSE;
				digitalWrite(LED_BUILTIN, HIGH);
			}
			else
			{
				f_led = TRUE;
				digitalWrite(LED_BUILTIN, LOW);
			}
		}
	}

	if (uMTdoTicksWork() == 1)
	{
		///////////////////////////////////////////
		// Suspend task and force a reschedule
		///////////////////////////////////////////

		// Disable INTS
		DisableInterrupts();

		register StackPtr_t stack_ptr asm ("sp");

		if (Kernel.KernelStackMode == FALSE)			// to support Restart()
			Kernel.Running->SavedSP = stack_ptr;	// Save Task's Stack Pointer

		Kernel.NoPreempt = TRUE;		// Prevent further rescheduling.... until next one

		// Switch to private stack and call Reschedule();
		Kernel.NewStackReschedule();

		// Returning to the caller only when this task is again the RUNNING task
	}
	else
	{
		// Restore registers
		asm volatile ("pop {r4-r11,lr}");

		// Return from EXCEPTION
		asm volatile ("bx lr");
	}
}


};		// extern "C" linkage


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupSysTicks
//
////////////////////////////////////////////////////////////////////////////////////
void uMT::SetupSysTicks()
{
	// Switch LED off
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	// Align SystemTick
	Kernel.TickCounter.Low = GetTickCount();		// Simply copy ticks counter...

	// Force the loading of the sysTickHook() routine to overwrite Arduino "weak" version
	if (Kernel.TickCounter.Low == 0xFFFFFFFF)
	{
		sysTickHook();
		pendSVHook();
	}


	// Set interrupts to be preemptive. Change the grouping to set no sub-priority.
	// See SAM3x8E datasheet 12.6.6 page 84 and 12.21.6.1 page 177.
 	NVIC_SetPriorityGrouping (0b011);

	// Configure the system tick frequency to adjust the time quantum allocated to processes.
	// Not needed, done already in Arduino
	//  SysTick_Config (SystemCoreClock / SYSTICK_FREQUENCY_HZ) ;

	// Set the base priority register to 0 to allow any exception to be handled.
	// See SAM3x8E datasheet 12.4.3.14 page 62.
	__set_BASEPRI (0) ;

	// Force the PendSV exception to have the lowest priority to avoid killing other interrupts.
	// See SAM3x8E datasheet 12.20.10.1 page 168. */
	NVIC_SetPriority (PendSV_IRQn, 0xFF) ;

	// Enable SVCall_IRQn
	// NVIC_EnableIRQ (SVCall_IRQn) ;	// It seems useless
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isr_Kn_Reboot - ARDUINO_UNO/MEGA
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::isr_Kn_Reboot()
{
	NoPreempt = TRUE;		// Prevent further rescheduling.... until next one

	NVIC_SystemReset();			// Reset processor and internal peripherals

	while (1) 
	{ 
		// do nothing and wait for the eventual...
	} 
}



#endif

///////////// EOF