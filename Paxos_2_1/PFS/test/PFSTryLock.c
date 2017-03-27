/******************************************************************************
*
*  PFSTryLock.c 	--- client test program of PFS Library
*
*  Copyright (C) 2014-2016 triTech Inc. All Rights Reserved.
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

/*
 *	作成			渡辺典孝
 *	試験			塩尻常春
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

#include	"PFS.h"
#include	<stdio.h>
#include	<netinet/tcp.h>
#include	<inttypes.h>

void
usage()
{
	printf("cmd cell lockW name\n");
	printf("cmd cell lockR name\n");
	printf("cmd cell trylockW name\n");
	printf("cmd cell trylockR name\n");
}

int
main( int ac, char* av[] )
{
	int	Ret;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
/*
 *	Initialize
 */
	struct Session	*pSession = NULL, *pSessionEvent = NULL;
	char	Buff[DATA_SIZE_WRITE];
	// ファイル毎にLOCK名を生成するため


	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession ) {
		printf("Can't get address [%s].\n", av[1] );
		exit( -1 );
	}

	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, 
							stderr, LogDBG );
	}

	pSessionEvent = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );

	PaxosSessionSetLog( pSessionEvent, PaxosSessionGetLog( pSession ) );

	memset( Buff, 0, sizeof(Buff) );

	if ( !strcmp( av[2], "lockW" ) ) {

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;
		Ret = PFSLockW( pSession, av[3] );
		Printf("PFSLockW EXT: Ret[%d] errno[%d]\n", Ret, errno );

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "lockR" ) ) {

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;

		Ret = PFSLockR( pSession, av[3] );
		Printf("PFSLockR EXT: Ret[%d] errno[%d]\n", Ret, errno );

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose EXT:Ret[%d] errno[%d]\n", Ret, errno );

	} else if ( !strcmp( av[2], "trylockW" ) ) {
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;
		Ret = PFSTryLockW( pSession, av[3] );
		Printf("PFSTryLockW EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 )	goto end;

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "trylockR" ) ) {
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;
		Ret = PFSTryLockR( pSession, av[3] );
		Printf("PFSTryLockR EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 )	goto end;

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 ) goto end;

	} else {
		goto err1;
	}
	if( pSession )	PaxosSessionFree( pSession );
	return( 0 );
err1:
	usage();
end:
	return( -1 );
}

