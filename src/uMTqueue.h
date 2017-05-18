////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTqueue.h
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

#ifndef uMT_QUEUE_H
#define uMT_QUEUE_H


#include "uMT.h"


#define QUEUE_NOPRIO	0x00		// Queued in append mode
#define QUEUE_PRIO		0x01		// Queued for priority

typedef uint8_t	QueueMode_t;

class uMTtaskQueue
{

public:
	uTask		*Head;		// Head of the queue
	QueueMode_t		Mode;	// Prio or no PRIO


	void		Insert(uTask *pTask);
	void		Remove(uTask *pTask);
	uTask		*GetFirst();
inline	void	SetQueueMode(QueueMode_t _Mode) { Mode = _Mode; };

	void Init(QueueMode_t _Mode=QUEUE_PRIO) {
		Head = NULL;
		Mode = _Mode;
	};
};


#endif

/////////////////////////////////////// EOF

