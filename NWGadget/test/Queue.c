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

#include	"NWGadget.h"

Queue_t	Queue;

typedef struct {
	list_t	m_Lnk;
	int		m_Data;
} M_t;

M_t*
M_Create( int Data )
{
	M_t	*pM;

	pM	= (M_t*)Malloc( sizeof(*pM) );
	list_init( &pM->m_Lnk );
	pM->m_Data	= Data;

	return( pM );
}

void
M_Destroy( M_t *pM )
{
	Free( pM );
}

void*
ThreadWait( void *pArg )
{
	M_t	*pM;
	pthread_t	th;
	int	ms	= (int)(uintptr_t)pArg;

	th	= pthread_self();

	while( 1 ) {
		if( ms == FOREVER ) {QueueWaitEntry( &Queue, pM, m_Lnk );}	
		else				{QueueTimedWaitEntryWithMs(&Queue,pM,m_Lnk,ms);}

		printf("[%lu] ms=%d pM=%p\n", *(long*)&th, ms, pM );
		if( !pM )	break;
		if( pM ) {
			printf("\tm_Data=%d\n", pM->m_Data );
			M_Destroy( pM );
		}
	}
	printf("pM=%p ERROR EXIT errno=%d\n", pM, errno );
	pthread_exit( (void*)(uintptr_t)errno );
	return( NULL );
}

int
main( int ac, char *av[] )
{
	pthread_t	th[5];
	int	i;
	M_t	*pM;
	int	Ret;
	void	*pRet;

	QueueInit( &Queue );

	/* 1. timeout */
	printf("### 1. 0s timed_wait timeout ###\n");
	pthread_create( &th[0], NULL, ThreadWait, (void*)0 );

	pRet = NULL;
	pthread_join( th[0], &pRet );
	ASSERT( (int)(intptr_t)(pRet) == ETIMEDOUT );

	/* 2. timeout */
	printf("### 2. 10s timed_wait timeout ###\n");
	pthread_create( &th[0], NULL, ThreadWait, (void*)(10*1000) );

	pRet = NULL;
	pthread_join( th[0], &pRet );
	ASSERT( (int)(intptr_t)(pRet) == ETIMEDOUT );

	/* 3. timeout */
	printf("### 3. 10s timed_wait receive and timeout###\n");
	pthread_create( &th[0], NULL, ThreadWait, (void*)(10*1000) );

	pM = M_Create( 3 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	if( Ret < 0 )	goto err;

	pRet = NULL;
	pthread_join( th[0], &pRet );
	ASSERT( (int)(intptr_t)(pRet) == ETIMEDOUT );

	/* 4. one for-ever-wait thread */
	printf("### 4. one for_ever_wait thread ###\n");
	pthread_create( &th[0], NULL, ThreadWait, (void*)FOREVER );

	pM = M_Create( 4 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	if( Ret < 0 )	goto err;

	sleep( 1 );

	/* 5. multi for-ever-wait threads */
	printf("### 5. multi for_ever_wait threads ###\n");
	pthread_create( &th[1], NULL, ThreadWait, (void*)FOREVER );
	pthread_create( &th[2], NULL, ThreadWait, (void*)FOREVER );
	pthread_create( &th[3], NULL, ThreadWait, (void*)FOREVER );
	pthread_create( &th[4], NULL, ThreadWait, (void*)FOREVER );

	pM = M_Create( 5 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	if( Ret < 0 )	goto err;

	sleep( 1 );

	/* 6. suspend/resume */
	printf("### 6. suspend / count / resume###\n");
	QueueSuspend( &Queue );

	QueueMax(&Queue, 1 );
	pM = M_Create( 6 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	printf("Posted to suspended Queue Ret=%d Cnt=%d Omitted=%d\n", 
				Ret, QueueCnt(&Queue), QueueOmitted(&Queue));
	pM = M_Create( 61 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	printf("Posted to suspended Queue Ret=%d Cnt=%d Omitted=%d\n", 
				Ret, QueueCnt(&Queue), QueueOmitted(&Queue));
	pM = M_Create( 62 );
	Ret = QueuePostEntry( &Queue, pM, m_Lnk );
	printf("Posted to suspended Queue Ret=%d Cnt=%d Omitted=%d\n", 
				Ret, QueueCnt(&Queue), QueueOmitted(&Queue));
	sleep( 1 );
	printf("q_cnt=%d\n", QueueCnt( &Queue ) );

	printf("Resume Queue\n");
	QueueResume( &Queue );
	sleep( 1 );

	/* 7. Abort */
	printf("### 7. Abort ###\n");
	QueueAbort( &Queue, 9 );
	sleep( 10 );

	for( i = 0; i < 5; i++ ) {
		pRet = NULL;
		pthread_join( th[i], &pRet );
		ASSERT( (int)(intptr_t)(pRet) == SIGKILL );
	}

	QueueDestroy( &Queue );

	printf("=== OK ===\n");
	return( 0 );
err:
	printf("=== NG ===\n");
	return( -1 );
}
