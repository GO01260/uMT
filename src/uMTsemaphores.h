////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTsemaphores.h
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

#ifndef uMT_SEM_H
#define uMT_SEM_H

#if uMT_USE_SEMAPHORES==1

///////////////////////////////////////////////////////////////////////////////////
//
//	uMT SEMAPHORE
//
////////////////////////////////////////////////////////////////////////////////////


class uMTsem
{
	friend class uTask;
	friend class uMT;

	SemValue_t		SemValue;	// Semaphore value
	uMTtaskQueue	SemQueue;	// Pointer to the task list waiting for this Semaphore

	void Init()
	{
		SemValue = 0;			// Set it LOCKED
		SemQueue.Init();
	};
};


#endif

#endif


/////////////////////////////////////// EOF

