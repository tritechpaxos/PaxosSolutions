/******************************************************************************
*
*  Shutdown.c 	--- Shutdown command of Paxos-Session Library
*
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

//#define	_SERVER_
//#define	_LOCK_

#include	"PaxosSession.h"

int
main( int ac, char* av[] )
{
	int	Ret;

	if( ac < 3 ) {
		printf("PaxosSessionShutdown cell id [ABORT|comment]\n");
		exit( -1 );
	}
	struct Session*	pSession;

	pSession = PaxosSessionAlloc(
				ConnectAdm, Shutdown, CloseFd, GetMyAddr,
				Send, Recv,
				NULL, NULL, av[1] );

	if( getenv("LOG_SIZE") ) {
		int     log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, 
							stderr, DEFAULT_LOG_LEVEL );
	}

	IOSocket_t	IO;
	IOReadWrite_t	*pIO = (IOReadWrite_t*)&IO;
	void	*pH = NULL;
	PaxosSessionShutdownReq_t	Req;

	int	id = atoi(av[2]);

	PaxosSessionLock( pSession, id, &pH );
	if( !pH ) {
		Printf("SessionLock error id=%d\n", id );
		return( -1 );
	}

	IOSocketBind( &IO, PNT_INT32(pH) );

	memset( &Req, 0, sizeof(Req) );
	SESSION_MSGINIT( &Req.s_Head, PAXOS_SESSION_SHUTDOWN, 0, 0, sizeof(Req) );
	if( ac >= 4 ) {
		strncpy( Req.s_Comment, av[3], sizeof(Req.s_Comment) );
	}
	IOWrite( pIO, &Req, sizeof(Req) );

	IORead( pIO, &Ret, sizeof(Ret) );

	PaxosSessionFree( pSession );
	return( 0 );
}

