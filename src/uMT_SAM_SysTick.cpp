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


#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)


extern void uMT_SystemTicks();
extern unsigned uMTdoTicksWork();


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

extern uint32_t GetTickCount();
extern void tickReset();
extern void	TimeTick_Increment();


////////////////////////////////////////////////////////////////////////////////////
//
//	sysTickHook
//
// This is called by the ARM SysTick interrupt routine.
// It must return 0
//
////////////////////////////////////////////////////////////////////////////////////
extern "C" {

unsigned int sysTickHook()
{
	uint32_t CurrentTick = GetTickCount();

	if (CurrentTick < Kernel.TickCounter.Low) // RollOver..
			Kernel.TickCounter.High++;

	Kernel.TickCounter.Low = CurrentTick;		// Simply copy ticks counter...


	if (Kernel.BlinkingLED)
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


#ifdef ZAPPED		// Not working yet

	if (uMTdoTicksWork() == 1)
	{
		// Suspend task and force a reschedule
		Kernel.Suspend();
	}
#endif

	return(0);
}
};



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupSysTicks
//
////////////////////////////////////////////////////////////////////////////////////
void uMT::SetupSysTicks()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	// Align SystemTick
	Kernel.TickCounter.Low = GetTickCount();		// Simply copy ticks counter...

	// Force the loading of the sysTickHook() routine to overwrite Arduino "weak" version
	if (Kernel.TickCounter.Low == 0xFFFFFFFF)
	{
		sysTickHook();
	}

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_Reboot - ARDUINO_UNO/MEGA
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::isrKn_Reboot()
{
	NoResched++;		// Prevent rescheduling....

//	delay(5000);

	RSTC->RSTC_CR = 0xA5000005; // Reset processor and internal peripherals

	while (1) 
	{ 
		// do nothing and wait for the eventual...
	} 
}




#endif

///////////// EOF