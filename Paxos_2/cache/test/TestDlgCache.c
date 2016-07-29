/******************************************************************************
*  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
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

#include	"DlgCache.h"

#define	PRINTF( fmt, ... ) \
			printf("[%u:%u]"fmt, \
			(uint32_t)pthread_self(), (uint32_t)time(0), ##__VA_ARGS__)

int
Init0( DlgCache_t *pDlg, void *pArg )
{
//	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );

	PRINTF("Init0:[%s] pTag=%p pArg=%p\n", 
			pDlg->d_Dlg.d_Name, pDlg->d_Dlg.d_pTag, pArg );

	return( 0 );
}

int
Term0( DlgCache_t *pDlg, void *pArg )
{
//	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );

	PRINTF("Term0:[%s] pTag=%p pArg=%p\n", 
			pDlg->d_Dlg.d_Name, pDlg->d_Dlg.d_pTag, pArg );

	return( 0 );
}

int
Init1( DlgCache_t *pDlg, void *pArg )
{
//	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );

	PRINTF("Init1:[%s] pTag=%p pArg=%p\n", 
			pDlg->d_Dlg.d_Name, pDlg->d_Dlg.d_pTag, pArg );

	return( 0 );
}

int
Term1( DlgCache_t *pDlg, void *pArg )
{
//	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );

	PRINTF("Term1:[%s] pTag=%p pArg=%p\n", 
			pDlg->d_Dlg.d_Name, pDlg->d_Dlg.d_pTag, pArg );

	return( 0 );
}

DlgCacheCtrl_t		CTRL0;
DlgCacheCtrl_t		CTRL1;

DlgCacheRecall_t	RECALL;

void
usage(void)
{
	printf("TestDlgCache cell LockW name type count\n");
	printf("TestDlgCache cell LockR name type count\n");
	printf("TestDlgCache cell LockW/count name type count\n");
	printf("TestDlgCache cell LockR/count name type count\n");
}

int
main( int ac, char *av[] )
{
	struct Session *pSession, *pEvent;
	int	Ret;
	DlgCacheCtrl_t	*pCTRL;
	DlgCache_t	*pD;
	int	Type;
	int	count;

	if( ac < 6 ) {
		usage();
		exit( -1 );
	}
	Type = atoi(av[4]);
	count	= atoi(av[5]);

	if( Type )	pCTRL	= &CTRL1;
	else		pCTRL	= &CTRL0;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	pEvent = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession || !pEvent ) {
		printf("Can't get address [%s].\n", av[1] );
		goto err;
	}
	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, stderr, 5 );
	}

	PaxosSessionSetLog( pEvent, PaxosSessionGetLog( pSession ) );

	Ret = PaxosSessionOpen( pEvent, PFS_SESSION_EVENT, FALSE );
	if( Ret != 0 ) {
		Printf("EventSessionOpen0:Ret[%d]\n", Ret );
		goto err;
	}
	Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
	if( Ret != 0 ) {
		Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret,errno );
		PaxosSessionClose( pSession );
		goto err;
	}
	DlgCacheRecallStart( &RECALL, pEvent );

	DlgCacheInit( pSession, &RECALL, 
				&CTRL0, 2,
				NULL, NULL, NULL, NULL,
				Init0, Term0, NULL,
				PaxosSessionGetLog(pSession) );

	DlgCacheInit( pSession, &RECALL, 
				&CTRL1, 2,
				NULL, NULL, NULL, NULL,
				Init1, Term1, NULL,
				PaxosSessionGetLog(pSession) );

	if( !strcmp("LockW", av[2]) ) {

		while( count-- ) {
			PRINTF( "### LockW:[%s] ###\n", av[3] );

			pD = DlgCacheLockW( pCTRL, av[3], TRUE,
						(void*)(uintptr_t)Type, (void*)(uintptr_t)count );
			PRINTF("AFTER LockW:sleep 30\n");sleep(30);
			DlgCacheUnlock( pD, FALSE );
			PRINTF("=== AFTER Unlock:sleep 30 ===\n");sleep(30);
		}
	}
	else if( !strcmp("LockR", av[2]) ) {

		while( count-- ) {
			PRINTF( "### LockR:[%s] ###\n", av[3] );

			pD = DlgCacheLockR( pCTRL, av[3], TRUE,
						(void*)(uintptr_t)Type, (void*)(uintptr_t)count );
			PRINTF("AFTER LockR:sleep 30\n");sleep(30);
			DlgCacheUnlock( pD, FALSE );
			PRINTF("=== AFTER Unlock:sleep 30 ===\n");sleep(30);
		}
	}
	else if( !strcmp("LockW/count", av[2]) ) {
		char	Name[128];
		while( count ) {
			sprintf( Name, "%s/%d", av[3], count-- );
			PRINTF( "### LockW/count:[%s] ###\n", Name );

			pD = DlgCacheLockW( pCTRL, Name, TRUE,
						(void*)(uintptr_t)Type, (void*)(uintptr_t)count );
			PRINTF("AFTER LockW:sleep 30\n");sleep(30);
			DlgCacheUnlock( pD, FALSE );
			PRINTF("=== AFTER Unlock:sleep 30 ===\n");sleep(30);
		}
	}
	else if( !strcmp("LockR/count", av[2]) ) {
		char	Name[128];
		while( count ) {
			sprintf( Name, "%s/%d", av[3], count-- );
			PRINTF( "### LockR/count:[%s] ###\n", Name );

			pD = DlgCacheLockR( pCTRL, Name, TRUE,
						(void*)(uintptr_t)Type, (void*)(uintptr_t)count );
			PRINTF("AFTER LockR:sleep 30\n");sleep(30);
			DlgCacheUnlock( pD, FALSE );
			PRINTF("=== AFTER Unlock:sleep 30 ===\n");sleep(30);
		}
	}
	PRINTF("BEFORE Destroy\n");
	DlgCacheDestroy( &CTRL0 );
	DlgCacheDestroy( &CTRL1 );
	DlgCacheRecallStop( &RECALL );
	PRINTF("After Destroy\n");

	PaxosSessionClose( pSession );
//	PaxosSessionClose( pEvent );
	PRINTF("After SessionClose\n");

	PaxosSessionFree( pSession );
	PaxosSessionFree( pEvent );
	PRINTF("After SessionFree\n");
	return( 0 );
err:
	return( -1 );
}
