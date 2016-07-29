/******************************************************************************
*
*  PaxosSessionChangeMember.c 	--- client test program of Paxos-Session Library
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


#define	_SERVER_
//#define	_LOCK_

#include	"PaxosSession.h"

PaxosSessionCell_t	*pSessionCell;

int
main( int ac, char* av[] )
{
	struct sockaddr_in	Addr;
	int	Ret;

	if( ac < 5 ) {
		printf("command cell id in_addr port\n");
		exit( -1 );
	}
	struct Session*	pSession;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
				Send, Recv, Malloc, Free, av[1] );

	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, 
								stderr, DEFAULT_LOG_LEVEL );
	}
	Ret = PaxosSessionOpen( pSession, 0, TRUE );
	if( Ret ) {
		printf("Open ERROR\n");
		goto ret;
	}
	memset( &Addr, 0, sizeof(Addr) );
	Addr.sin_family		= AF_INET;
	Addr.sin_addr		= PaxosGetAddrInfo( av[3] );
	Addr.sin_port		= ntohs( atoi(av[4]) );

	Ret = PaxosSessionChangeMember( pSession, atoi(av[2]), &Addr );

	printf("Ret[%d]\n", Ret );

	PaxosSessionClose( pSession );
ret:
	PaxosSessionFree( pSession );
	return( 0 );
}

