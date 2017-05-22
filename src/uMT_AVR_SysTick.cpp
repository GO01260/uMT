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


#if defined(ARDUINO_ARCH_AVR)

#include "uMTarduinoAVR.h"

#include <avr/wdt.h>


extern void uMT_SystemTicks();
extern unsigned uMTdoTicksWork();



#if uMT_USE_WTD_4_TICKS==1

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

extern volatile unsigned long timer0_overflow_count;
extern volatile unsigned long timer0_millis;

static unsigned char timer0_fract = 0;


// NOTE:
// We need to keep the original code of WIRING.C here because we are linking the stripped down, local
// version of wiring.c
//

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_millis;
	unsigned char f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
	timer0_overflow_count++;


}


/***************************************************
    Name:        ISR(WDT_vect)
    Returns:     Nothing.
    Parameters:  None.
    Description: Watchdog Interrupt Service. This
                 is executed when watchdog timed out.
 ***************************************************/
ISR(WDT_vect, ISR_NAKED)
{
	iMT_ISR_Entry();

	/////////////////////////////////////////
	// uMT Specific
	/////////////////////////////////////////
	uMT_SystemTicks();		// Let the compiler to "inline" if it makes sense...

	iMT_ISR_Exit();
}



/*

  15mS  WDTO_15MS
  30mS  WDTO_30MS
  60mS  WDTO_60MS
  120mS WDTO_120MS
  250mS WDTO_250MS
  500mS WDTO_500MS
  1S  WDTO_1S
  2S  WDTO_2S
  4S  WDTO_4S
  8S  WDTO_8S

*/


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	WDT_SetTimer - ARDUINO_UNO
//
// Set the WatchDog Timer on ARDUINO UNO
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void WDT_SetTimer(int timer)
{
	/*** Setup the WDT ***/

	/* Clear the reset flag. */
	MCUSR &= ~(1 << WDRF);

	// In order to change WDE or the prescaler, we need to
	// set WDCE (This will allow updates for 4 clock cycles).
	WDTCSR |= (1 << WDCE) | (1 << WDE);

	// WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
	// WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
	WDTCSR = timer;

	/* Enable the WD interrupt (note no reset). */
	WDTCSR |= _BV(WDIE);

}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMT
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
//
//	uMT::SetupSysTicks
//
////////////////////////////////////////////////////////////////////////////////////
void uMT::SetupSysTicks()
{
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

/* set new watchdog timeout prescaler value */

	int	DelayValue;

	switch (uMT_TICKS_SECONDS)
	{
	case 2:
		DelayValue = WDTO_500MS;
		break;
	case 4:
		DelayValue = WDTO_250MS;
		break;
	case 8:
		DelayValue = WDTO_120MS;
		break;
	case 16:
		DelayValue = WDTO_60MS;
		break;
	case 32:
		DelayValue = WDTO_30MS;
		break;
	case 64:
		DelayValue = WDTO_15MS;
		break;

	default:
		DelayValue = WDTO_1S;
		break;
	}

	DgbStringPrint("SetupSysTicks(): WDT DelayValue => ");
	DgbValuePrint(DelayValue);


	WDT_SetTimer(DelayValue);

}

#endif


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////



#if uMT_USE_TIMER0_4_TICKS==1


// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

extern volatile unsigned long timer0_overflow_count;
extern volatile unsigned long timer0_millis;

static unsigned char timer0_fract = 0;


#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect, ISR_NAKED)
#else
ISR(TIMER0_OVF_vect, ISR_NAKED)
#endif
{
	iMT_ISR_Entry();


	///////////////////////////////////////////////////////////////////
	// ARDUINO original wiring.c
	///////////////////////////////////////////////////////////////////

	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_millis;
	unsigned char f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
	timer0_overflow_count++;


	/////////////////////////////////////////
	// uMT Specific
	/////////////////////////////////////////
	uMT_SystemTicks();		// Let the compiler to "inline" if it makes sense...


	iMT_ISR_Exit();
}



#endif



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT_SystemTicks
//
// This is shielded in a routine to allow to "FRIEND"ly access data in uMT C++ class
//
////////////////////////////////////////////////////////////////////////////////////
inline void uMT_SystemTicks()
{

#if uMT_USE_WTD_4_TICKS==1			// Using WDT
	Kernel.TickCounter++;
#endif

#if uMT_USE_TIMER0_4_TICKS==1		// Using TIMER0
	if (timer0_millis < Kernel.TickCounter.Low) // RollOver..
			Kernel.TickCounter.High++;

	Kernel.TickCounter.Low = timer0_millis;		// Simply copy ticks counter...
#endif

	if (Kernel.BlinkingLED)
	{
		if ((Kernel.TickCounter % uMT_TICKS_SECONDS) == 0)
		{
			volatile static Bool_t f_wdt = TRUE;

			if (f_wdt == TRUE)
			{
				f_wdt = FALSE;
				digitalWrite(LED_BUILTIN, HIGH);
			}
			else
			{
				f_wdt = TRUE;
				digitalWrite(LED_BUILTIN, LOW);
			}
		}
	}
	
	if (uMTdoTicksWork() == 1)
	{
		Kernel.Running->SavedSP = SP;	// Save Task's Stack Pointer

		// Suspend task and force a reschedule
		Kernel.Suspend2();
	}
}




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
	Kernel.TickCounter.Low = timer0_millis;		// Simply copy ticks counter...
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	isrKn_Reboot - ARDUINO_UNO/MEGA
//
// It restore the previous status register (enabling INTERRUPTs (GLOBAL) if previously enabled)
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void	uMT::isrKn_Reboot()
{
	NoResched++;		// Prevent rescheduling....


	cli();		// Disable interrupts


#ifdef ZAPPED
	__asm__ __volatile__ ("wdr");	// Reset WatchDog

	WDT_SetTimer(WDTO_4S);		// 4 seconds..
	WDTCSR |= _BV(WDE);			// RESET!

#else
	__asm__ __volatile__ ("wdr");	// Reset WatchDog
	wdt_enable(WDTO_4S); // turn on the WatchDog 
#endif
	
	sei();		// Enable interrupts


	while (1) 
	{ 
		// do nothing and wait for the eventual...
	} 
}




#endif

///////////// EOF