/******************************************************************************
*
*  HTTPd.c 	--- test program of HTTP daemon
*
*  Copyright (C) 2011 triTech Inc. All Rights Reserved.
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
 *	試験
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

#include	"Converter.h"
#include	<stdio.h>


extern	Msg_t* HTTPRequestByMsg( int fd, bool_t *pUpdate, 
				void*(*fMalloc)(size_t), void(*fFree)(void*) );

char	*Response[] = {
"HTTP/1.0 200 OK\r\n\r\n"
};

void*
Thread( void* pArg )
{
	int	fd	= PNT_INT32(pArg);
	Msg_t	*pReq;
	bool_t	Update;
	char	*pBuf;

DBG_PRT("START\n");
	while( (pReq = HTTPRequestByMsg( fd, &Update, Malloc, Free ) ) ) {
		if( pReq->m_Err != 0 ) {
DBG_PRT("Err=%d\n", pReq->m_Err );
			MsgDestroy( pReq );
			break;
		}	
DBG_PRT("MsgTotalLen=%d\n", MsgTotalLen( pReq ) );
		for( pBuf = MsgFirst( pReq ); pBuf; pBuf = MsgNext( pReq ) ) {
pBuf[MsgLen(pReq)] = 0;
DBG_PRT("[%s]\n", pBuf );
		}
		MsgDestroy( pReq );
		send( fd, Response[0], strlen(Response[0]), 0 );
	}
DBG_PRT("END\n");
	pthread_exit( 0 );
	return( NULL );
}

void
usage()
{
	printf("HTTPd client_port\n");
}

int
main( int ac, char* av[] )
{
	char	host_name[128];

    struct addrinfo hints;
    struct addrinfo *result;
    int 	s;
	struct in_addr	pin_addr;
	struct sockaddr_in	Port, From;
	pthread_attr_t	attr;
	int	ListenFd, len, fd;
	pthread_t	th;


	if( ac < 2 ) {
		usage();
		exit( -1 );
	}
/*
 *	Client Accept Address
 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family 	= AF_INET;
    hints.ai_socktype 	= SOCK_STREAM;
    hints.ai_flags 		= 0;
    hints.ai_protocol 	= 0;
    hints.ai_canonname 	= NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

	gethostname( host_name, sizeof(host_name) );
   	s = getaddrinfo( host_name, NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	memset( &Port, 0, sizeof(Port) );
	Port.sin_family	= AF_INET;
	Port.sin_addr	= pin_addr;
	Port.sin_port	= htons( atoi(av[1]) );

	if( (ListenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror("socket");
		exit( -1 );
	}
	if( bind( ListenFd, (struct sockaddr*)&Port, sizeof(Port) ) < 0 ) {
		perror("bind");
		exit( -1 );
	}
	listen( ListenFd, 5 );

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	while( TRUE ) {
		len	= sizeof(From);
		fd	= accept( ListenFd, (struct sockaddr*)&From, &len );
		if( fd < 0 ) {
			perror("accept");
			exit( -1 );
		}
		pthread_create( &th, &attr, Thread, INT32_PNT(fd) );
	}
}

