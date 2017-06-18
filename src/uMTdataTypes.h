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


#ifdef WIN32		// AVR look-alike

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef  short			int16_t;
typedef unsigned int	uint32_t;
#define __FlashStringHelper char
#define F(x)				x

//typedef uint16_t		SRAMsize_t;			// 16 bits

typedef	uint16_t		StackPtr_t;			// Stack
typedef uint16_t		StackSize_t;		// 16 bits, system dependent. In MEGA the number of bits in an address is 22 but 16 seems to be sufficient
typedef uint8_t			CpuStatusReg_t;		// 8 bits - Lock/Unlock
typedef uint16_t		Param_t;			// 16 bits
typedef uint16_t		SemValue_t;			// 16 bits
typedef uint32_t		RunValue_t;			// 32 bits

#define uMT_MAX_SEM_VALUE	(0xffff)		// 16 bits value
#define uMT_ALL_EVENT_MASK	(0xffff)		// 16 bits value

#else

// The followingis required for uintXX_t definitions
#include "Arduino.h"


#if defined(ARDUINO_ARCH_AVR) // AVR-specific code

typedef	uint16_t		StackPtr_t;			// Stack 
typedef uint16_t		StackSize_t;		// 16 bits, system dependent. In MEGA the number of bits in an address is 22 but 16 seems to be sufficient
typedef uint8_t			CpuStatusReg_t;		// 8 bits - Lock/Unlock
typedef uint16_t		Param_t;			// 16 bits
typedef uint16_t		SemValue_t;			// 16 bits
typedef uint32_t		RunValue_t;			// 32 bits

#define uMT_MAX_SEM_VALUE	(0xffff)		// 16 bits value
#define uMT_ALL_EVENT_MASK	(0xffff)		// 16 bits value


#elif defined(ARDUINO_ARCH_SAM)  || defined(ARDUINO_ARCH_SAMD) // SAM-specific code

typedef	uint32_t		StackPtr_t;			// Stack 
typedef uint32_t		StackSize_t;		// 32 bits, system dependent.
typedef uint32_t		CpuStatusReg_t;		// 8 bits - Lock/Unlock
typedef uint32_t		Param_t;			// 32 bits
typedef uint32_t		SemValue_t;			// 32 bits
typedef uint32_t		RunValue_t;			// 32 bits

#define uMT_MAX_SEM_VALUE	(0xffffffff)	// 32 bits value
#define uMT_ALL_EVENT_MASK	(0xffffffff)	// 32 bits value


#else
  #error “This library only supports boards with AVR or SAM processor.”
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



////////////////////////////////////////////////////////////////////////////////////
//
//	uMT OBJECT class
//
////////////////////////////////////////////////////////////////////////////////////

#define OBJ_DELETED		0xFFFF	/* Means OBJECT deleted */

class uMTobject_id {
	friend class uTask;
	friend class uMT;
	friend class uTimer;
	friend	class uMTtaskQueue;

private:

static uint16_t ObjectNumber;		// Object counter

	inline uint16_t	NextObjectNumber()
	{
		++ObjectNumber;
		if (ObjectNumber == 0 || ObjectNumber == OBJ_DELETED)
			ObjectNumber = 1;
		return(ObjectNumber);
	};


	uint16_t	Timestamp;
	uint16_t	Index;

public:

	inline	void Init(unsigned int index)
	{
		Timestamp = 0;		// Se it invalid
		Index = index;
	};


inline	void NewObject(unsigned int index)
	{
		Timestamp = NextObjectNumber();
		Index = index;
	};

inline	void NewTimestamp()
	{
		// Keep existing Index
		Timestamp = NextObjectNumber();
	};

inline	void ClearTimestamp()
	{
		// Keep existing Index
		Timestamp = OBJ_DELETED;
	};

inline	uint16_t GetID()
	{
		return(Index);
	};

};

typedef uMTobject_id	TimerId_t;			// Timer ID type

typedef uint8_t			Bool_t;				// 8 bits
typedef uint32_t		Timer_t;			// 32 bits
typedef uint8_t			SemId_t;			// 8 bits, max 255
typedef uint16_t		Cfg_data_t;			// Used in uMTcfg class


#if uMT_MAX_EVENTS_NUM == 16
typedef uint16_t		Event_t;			// 16 bits
#else
typedef uint32_t		Event_t;			// 32 bits
#endif

typedef void		(*FuncAddress_t)();		// Placehoder for a variable keeping an address of a function()


#endif


////////////////////////// EOF