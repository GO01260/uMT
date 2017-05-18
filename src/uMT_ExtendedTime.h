////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMT_ExtTime.h
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
// class uMT_ExtTime
//
// This class is used to manage the TICKS counter, that is the counter of the number of milliseconds since Arduino boot.
// Standard Arduino counter is able to manage up to 50 days without overflow.
// With this extension, the counter is able to manage almost 34 years.
//
/////////////////////////////////////////////////////////////////////////////////////

class uMT_ExtTime
{
public:
	Timer_t	Low;
	uint8_t	High;


public:
	uMT_ExtTime(uint8_t _high, Timer_t _low) { High = _high; Low = _low;};
	uMT_ExtTime(Timer_t Value) { High = 0; Low = Value;};
	uMT_ExtTime() { High = 0; Low = 0;};

	uMT_ExtTime operator+ (const Timer_t Value)
	{
		uMT_ExtTime tmp;

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

	uMT_ExtTime operator+ (const uMT_ExtTime Value)
	{
		uMT_ExtTime tmp;

		tmp.Low = Low + Value.Low;
		tmp.High = High + Value.High;

		if (tmp.Low < Value.Low)
			tmp.High++;

		return(tmp);
	};

	uMT_ExtTime operator++ ()
	{
		Timer_t oldValue = Low;

		Low++;

		if (Low < oldValue) // RollOver..
			High++;

		return uMT_ExtTime(High, Low);
	};

	uMT_ExtTime operator++ (int)
	{
		uMT_ExtTime tmp(High, Low);

		Timer_t oldValue = Low;

		Low++;

		if (Low < oldValue) // RollOver..
			High++;

		return tmp;
	};

	bool operator< (const uMT_ExtTime& t)
	{
		if (High < t.High)
			return true;

		if (High == t.High && Low < t.Low)
			return true;

		return false;
	};

	bool operator<= (const uMT_ExtTime& t)
	{
		if (High < t.High)
			return true;

		if (High == t.High && Low <= t.Low)
			return true;

		return false;
	};

	bool operator> (const uMT_ExtTime& t)
	{
		if (High > t.High)
			return true;

		if (High == t.High && Low > t.Low)
			return true;

		return false;
	};

	bool operator== (const uMT_ExtTime& t)
	{
		if (High == t.High && Low == t.Low)
			return true;

		return false;
	};

	void Set(const uint8_t	_High, Timer_t _Low)
	{
		High = _High;
		Low = _Low;
	};

};


/////////////////////////// EOF