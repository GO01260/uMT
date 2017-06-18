////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTextendedTime.h
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
////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////
// class uMTextendedTime
//
// This class is used to manage:
//		- the TICKS counter, that is the counter of the number of milliseconds since Arduino boot.
//		- CPU running times.
//
// TICKS counter
//	Standard Arduino counter is able to manage up to 50 days without overflow.
//	With uint8_t extension (40 bits), the counter is able to manage almost 34 years.
//	With uint16_t extension (48 bits), the counter is able to manage over 8900 years.
//
// CPU running times
//	With uint8_t extension (40 bits), the counter is able to manage almost 13 days.
//	With uint16_t extension (48 bits), the counter is able to manage almost 9 years.
//
/////////////////////////////////////////////////////////////////////////////////////

typedef uint16_t TimerHigh_t;

class uMTextendedTime
{
public:
	Timer_t	Low;
	TimerHigh_t	High;


public:
	uMTextendedTime(TimerHigh_t _high, Timer_t _low) { High = _high; Low = _low;};
	uMTextendedTime(Timer_t Value) { High = 0; Low = Value;};
	uMTextendedTime() { High = 0; Low = 0;};

	uMTextendedTime operator+ (const Timer_t Value)
	{
		uMTextendedTime tmp;

		tmp.Low = Low + Value;
		tmp.High = 0;

		if (tmp.Low < Value)
			tmp.High++;

		return(tmp);
	};

	Timer_t operator% (const Timer_t Value)
	{
		return(Low % Value); 
	};


	uMTextendedTime operator+ (const uMTextendedTime Value)
	{
		uMTextendedTime tmp;

		tmp.Low = Low + Value.Low;
		tmp.High = High + Value.High;

		if (tmp.Low < Value.Low)
			tmp.High++;

		return(tmp);
	};

	uMTextendedTime operator- (const uMTextendedTime Value)
	{
		uMTextendedTime tmp;

		tmp.High = High - Value.High;
		tmp.Low = Low - Value.Low;

		if (Low < Value.Low)
			tmp.High--;

		return(tmp);
	};

	uMTextendedTime operator++ ()
	{
		Timer_t oldValue = Low;

		Low++;

		if (Low < oldValue) // RollOver..
			High++;

		return uMTextendedTime(High, Low);
	};

	uMTextendedTime operator++ (int)
	{
		uMTextendedTime tmp(High, Low);

		Timer_t oldValue = Low;

		Low++;

		if (Low < oldValue) // RollOver..
			High++;

		return tmp;
	};

	bool operator< (const uMTextendedTime& t)
	{
		if (High < t.High)
			return true;

		if (High == t.High && Low < t.Low)
			return true;

		return false;
	};

	bool operator<= (const uMTextendedTime& t)
	{
		if (High < t.High)
			return true;

		if (High == t.High && Low <= t.Low)
			return true;

		return false;
	};

	bool operator> (const uMTextendedTime& t)
	{
		if (High > t.High)
			return true;

		if (High == t.High && Low > t.Low)
			return true;

		return false;
	};

	bool operator== (const uMTextendedTime& t)
	{
		if (High == t.High && Low == t.Low)
			return true;

		return false;
	};

	void Set(const uint8_t	_High, const Timer_t _Low)
	{
		High = _High;
		Low = _Low;
	};

	void Clear()
	{
		High = 0;
		Low = 0;
	};

		
	
	uMTextendedTime DivideBy(uint32_t divisor)		// Max divisor is 32 bits
	{
		uMTextendedTime tmp(0,0);

		// Take the current 48bits value and split in 3 16bits variables of 32bits size
		// Divide each variable and add the remainder to the lower level
		// Combine again in 48 bits value

		if (divisor == 0)
		{
			// oops
			return(tmp);		// What else to do?
		}

		if (High == 0 && Low == 0)
		{
			return(tmp);
		}

		uint32_t _top = High;
		uint32_t _mid = Low >> 16;
		uint32_t _low = Low & 0xffff;

//		uint32_t result1 = _top / divisor;
//		tmp.High = result1;

		tmp.High = _top / divisor;
		_mid += (_top % divisor) << 16;			// Add remainder to the midle word, it cannot be bigger than 32 bits

//		uint32_t result2 = _mid / divisor;
		_low += (_mid % divisor) << 16;			// Add remainder to the top word, it cannot be bigger than 32 bits

//		uint32_t result3 = _low / divisor;
//		tmp.Low = result3 | (result2 << 16);

		tmp.Low = (_low / divisor) | ((_mid / divisor) << 16);

		return(tmp);
	};

	
#ifdef ZAPPED
	uMTextendedTime in1000th()
	{
		uMTextendedTime tmp;

		// Take the current 48bits value and split in 3 16bits variables of 32bits size
		// Divide each variable and add the remainder to the lower level
		// Combine again in 48 bits value

		uint32_t _top = High;
		uint32_t _mid = Low >> 16;
		uint32_t _low = Low & 0xffff;

//		uint32_t result1 = _top / 1000;
//		tmp.High = result1;

		tmp.High = _top / 1000;
		_mid += (_top % 1000) << 16;			// Add remainder to the midle word, it cannot be bigger than 32 bits

//		uint32_t result2 = _mid / 1000;
		_low += (_mid % 1000) << 16;			// Add remainder to the top word, it cannot be bigger than 32 bits

//		uint32_t result3 = _low / 1000;
//		tmp.Low = result3 | (result2 << 16);

		tmp.Low = (_low / 1000) | ((_mid / 1000) << 16);

		return(tmp);
	};
#endif

};





#ifdef ZAPPED
/////////////////////////////////////////////////////////////////////////////////////
// class uMTrunningTime
//
// This class is used to manage CPU running times with some simplifications and optimization
// Maximum is 50 days
/////////////////////////////////////////////////////////////////////////////////////


typedef uint16_t	uMTrt_micro_t;
typedef uint32_t	uMTrt_milli_t;

class uMTrunningTime
{
public:
	uMTrt_micro_t	micro;		// Keep time in microseconds (remainder)
	uMTrt_milli_t	milli;		// Keep time in milliseconds


public:
	uMTrunningTime() { micro = 0; micro = 0;};


	uMTrunningTime operator+ (const Timer_t Value)
	{
		uMTrunningTime tmp;

		tmp.micro = micro + (Value % 1000);
		tmp.milli = milli + (Value / 1000);

		if (tmp.micro > 1000)
		{
			tmp.micro = tmp.micro % 1000;
			tmp.milli = tmp.milli + 1;
		}

		return(tmp);
	};

	uMTrunningTime operator+ (const uMTrunningTime Value)
	{
		uMTrunningTime tmp;

		tmp.micro = micro + Value.micro;
		tmp.milli = milli + Value.milli;

		if (tmp.micro > 1000)
		{
			tmp.micro = tmp.micro % 1000;
			tmp.milli = tmp.milli + 1;
		}

		return(tmp);
	};

#ifdef ZAPPED
	inline void Add(const Timer_t Value)
	{
		micro = micro + (Value % 1000);
		milli = milli + (Value / 1000);

		if (micro > 1000)
		{
			micro = micro % 1000;
			milli = milli + 1;
		}
	};
#endif


	void Clear()
	{
		micro = 0;
		milli = 0;
	};

	void Set(const uMTrt_micro_t _micro, const uMTrt_milli_t _milli)
	{
		micro = _micro;
		milli = _milli;
	};

};

#endif



/////////////////////////// EOF