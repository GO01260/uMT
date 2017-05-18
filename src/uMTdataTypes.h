////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTdataTypes.h
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

#ifndef uMT_DATATYPES_H
#define uMT_DATATYPES_H


#ifdef WIN32

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef  short	int16_t;
typedef unsigned int	uint32_t;
#define __FlashStringHelper char
#define F(x)			x

typedef uint16_t		SRAMsize_t;			// 16 bits

#else

// The followingis required for uintXX_t definitions
#include "Arduino.h"


#if defined(ARDUINO_ARCH_AVR) // AVR-specific code
typedef uint16_t		SRAMsize_t;			// 16 bits


#elif defined(ARDUINO_ARCH_SAM)  // SAM-specific code

typedef uint32_t		SRAMsize_t;			// 16 bits

#else
  #error “This library only supports boards with AVR or SAM.”
#endif

#endif




////////////////////////////////////////////////////////////////////////////////////
//
//	uMT EXTERNAL DEFINEs, TYPEDEFs, ETCs
//
////////////////////////////////////////////////////////////////////////////////////
#define EOS			'\0'	/* NULL character value */
#ifndef NULL
#define NULL		0	/* NULL pointer */
#endif


#define FALSE		0
#define TRUE		1

typedef uint8_t			Bool_t;				// 8 bits
typedef uint32_t		Timer_t;			// 32 bits
typedef uint8_t			TimerId_t;			// 8 bits
typedef uint8_t			SemId_t;			// 8 bits, max 255
typedef uint16_t		Event_t;			// 16 bits
typedef uint16_t		Param_t;			// 16 bits

typedef void		(*FuncAddress_t)();		// Placehoder for a variable keeping an address of a function()


#endif


////////////////////////// EOF