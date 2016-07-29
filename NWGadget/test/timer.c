/******************************************************************************
*
*  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
*
*  See GPL-LICENSE.txt for copyright and license details.
*
*  This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version. 
*
*  This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with
* this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/** @file
 * @brief timer ‚ÌŠÖ”ŒQ
 * @author nw
 */

#include	"NWGadget.h"

int
eventFired( TimerEvent_t* pEvent )
{
	DBG_ENT();

Printf("pEvent=%p e_pArg=%p Registered[%d-%d] Timeout[%d-%d] Fired[%d-%d] %d\n", 
				pEvent,
				pEvent->e_pArg, 
				(int)pEvent->e_Registered.tv_sec,
				(int)pEvent->e_Registered.tv_nsec,
				(int)pEvent->e_Timeout.tv_sec,
				(int)pEvent->e_Timeout.tv_nsec,
				(int)pEvent->e_Fired.tv_sec,
				(int)pEvent->e_Fired.tv_nsec,
				(int)TIMESPECCOMPARE( pEvent->e_Timeout, pEvent->e_Fired ) );
	DBG_EXT(0);
	return( 0 );
}

void
eventPrint( void* pArg )
{
	eventFired( (TimerEvent_t*) pArg );
}

void*
ThreadTest( void *pArg )
{
	printf("pThread OK(pArg=%p)\n", pArg );
	return( NULL );
}

int
main( int ac, char* av[] )
{
	Timer_t	Timer;
	TimerEvent_t* pEvent, *pEvent1/*, *pEvent2*/;
	void	*pArg;

pthread_t th;
pthread_create( &th, NULL, ThreadTest, (void*)0 );
sleep( 10 );

	TimerInit( &Timer, TIMER_HEAP|TIMER_BTREE,
				1024*64, 2, 0,
				Malloc, Free, 
				5/*2*/, eventPrint );
	TimerEnable( &Timer );
	TimerStart( &Timer );

	Printf("=== TimerSet 1000ms\n");	
	pEvent = TimerSet( &Timer, 999/*1000*/, eventFired, (void*)1 );
	HashDump( &Timer.t_Hash );
	Printf("sleep(10)\n");	
	sleep( 10 );
	Printf("=== TimerCancel NOT EXIST %p\n", (pArg = TimerCancel( &Timer, pEvent )) );
	ASSERT( !pArg );

	Printf("=== TimerSet 20000ms\n");	
	pEvent = TimerSet( &Timer, 20000, eventFired, (void*)1 );
//	HashDump( &Timer.t_Hash );
	Printf("sleep(10)\n");	
	sleep( 10 );
	Printf("=== TimerCancel HEAD %p\n", (pArg = TimerCancel( &Timer, pEvent ) ) );
	ASSERT( pArg == (void*)1 );

	pEvent = TimerSet( &Timer, 3000, eventFired, (void*)2 );
	pEvent = TimerSet( &Timer, 2000, eventFired, (void*)3 );
	sleep( 20 );

	pEvent = TimerSet( &Timer, 3000, eventFired, (void*)2 );
	pEvent = TimerSet( &Timer, 2000, eventFired, (void*)3 );
//	BTreePrint( &Timer.t_BTree );
	pArg = TimerCancel( &Timer, pEvent );
	ASSERT( pArg == (void*)3 );
//	BTreePrint( &Timer.t_BTree );
	sleep( 20 );

	pEvent1 = TimerSet( &Timer, 3000, eventFired, (void*)2 );
	/*pEvent2 = */TimerSet( &Timer, 2000, eventFired, (void*)3 );
	pArg = TimerCancel( &Timer, pEvent1 );
	ASSERT( pArg == (void*)2 );
	sleep( 20 );

	BTreePrint( &Timer.t_BTree );
	HashDump( &Timer.t_Hash );

	pEvent1 = TimerSet( &Timer, 3000, eventFired, (void*)9 );
	HashDump( &Timer.t_Hash );

	TimerStop( &Timer );
	HashDump( &Timer.t_Hash );
	TimerDestroy( &Timer );

	printf("=== END ===\n");

	return( 0 );
}
