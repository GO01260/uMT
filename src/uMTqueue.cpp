////////////////////////////////////////////////////////////////////////////////////
//
//	FILE: uMTqueue.cpp
//	AUTHOR: Antonio Pastore - March 2017
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



//#include "uMT.h"

#include "uMT.h"

#define uMT_DEBUG 0
#include "uMTdebug.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//				CLASS uMTtaskQueue
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//
//	uMTtaskQueue::Insert
//
// Entered with INTS disabled
//
////////////////////////////////////////////////////////////////////////////////////
void uMTtaskQueue::Insert(uTask *pTask)
{
	CHECK_INTS("uMTtaskQueue::Insert");		// Verify if INTS are disabled...

//	QueueCounter++;		// Increment queue counter

	if (Head == NULL)
	{
		Head = pTask;
		pTask->Next = NULL;

		return;
	}

	uTask *pScan = Head;
	uTask *pLast = NULL;

	do
	{
		CHECK_TASK_MAGIC(pScan, "uMTtaskQueue::Insert");

		if (Mode == QUEUE_PRIO && pTask->Priority > pScan->Priority)	// Use PRIO to drive insertion
		{		
			if (pScan == Head)	// Insert as first element?
			{
				DgbStringPrint("uMTtaskQueue::Insert(): inserting as first task, TID => ");
				DgbValuePrintLN(pTask->myTid);

				pTask->Next = Head;
				Head = pTask;
			}
			else
			{
				DgbStringPrint("uMTtaskQueue::Insert(): inserting in the middle, TID => ");
				DgbValuePrint(pTask->myTid);
				DgbStringPrint("pLast TID => ");
				DgbValuePrint(pLast->myTid);
				DgbStringPrint("pNext TID => ");
				DgbValuePrintLN(pScan->myTid);

				// pLast garanteed NOT to be NULL
				pTask->Next = pScan;
				pLast->Next = pTask;
			}

			return;
		}

		pLast = pScan;
		pScan = pScan->Next;

	} while (pScan != NULL);

	
	// Add at the end
	pLast->Next = pTask;
	pTask->Next = NULL;
}



////////////////////////////////////////////////////////////////////////////////////
//
//	uMTtaskQueue::Remove
//
// Entered with INTS disabled
//
////////////////////////////////////////////////////////////////////////////////////
void uMTtaskQueue::Remove(uTask *pTask)
{
	CHECK_INTS("TaskQ_Remove");		// Verify if INTS are disabled...

	uTask *pScan = Head;
	uTask *pPrev = NULL;

	while (pScan != NULL)
	{
		if (pScan == pTask)
		{
			// Found!

			if (pPrev != NULL)		// pQueue was NOT NULL!
			{
				pPrev->Next = pScan->Next;
			}
			else	// pQueueHead was NULL!
			{
				Head = pScan->Next;		// It must be the first element...
			}

			return;
		}
		pScan = pScan->Next;		// Pick up next
	}

}


////////////////////////////////////////////////////////////////////////////////////
//
//	uMTtaskQueue::TaskQ_GetFirst
//
// Entered with INTS disabled
//
////////////////////////////////////////////////////////////////////////////////////
uTask * uMTtaskQueue::GetFirst()
{
	CHECK_INTS("TaskQ_GetFirst");		// Verify if INTS are disabled...

	uTask *pTask = Head;

	if (pTask == NULL)		// No tasks in the READY queue
	{
		return(NULL);
	}

	// Now pQueue CANNOT be NULL

	Head = pTask->Next;	// if ->Next is NULL, Queue is then empty

	CHECK_TASK_MAGIC(pTask, "uMTtaskQueue::GetFirst");

	return(pTask);
}




///////////////////// EOF