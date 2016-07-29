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

FdEventCtrl_t	CTRL;

struct sockaddr_in ADDR;
#define	PORT	4989

void*
Thread( void *pArg )
{
	int	Fd;

	sleep( 10 );

	Fd = (int)socket( AF_INET, SOCK_STREAM, 0 );
	if( Fd < 0 ) {
		perror("Thread:socket:");
		goto err;
	}

	printf("After 10sec connect\n");
	if( connect( Fd, (struct sockaddr*)&ADDR, sizeof(ADDR) ) < 0 ) {
		perror("Thread:connect:");
		goto err;
	}

	sleep( 1 );
	printf("pthread_exit\n");
	pthread_exit( 0 );
err:
	perror("SendThread");
	pthread_exit( 0 );
	return( NULL );
}

int
AcceptHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	int	Fd;
	int	Ad;
	socklen_t	len;
	struct sockaddr Client;

	memset( &Client, 0, sizeof(Client) );
	len	= sizeof(Client);

	Fd = pEv->fd_Fd;
	Ad = (int)accept( Fd, &Client, &len );
	if( Ad < 0 ) {
		perror("accept:");
		exit( -1 );
	}

	printf("Handler[Fd=%d,Ad=%d]\n", Fd, Ad );
	return( -1 );
}

int
main( int ac, char *av[] )
{
	int	Ret;
	int	Fd;
	FdEvent_t	FdEvListen;
	uint64_t	U64;
	pthread_t	th;
	int			t;

#ifdef	VISUAL_CPP
	if( WinInit() < 0 ) return( -1 );
#endif

	if( ac < 2 ) {
		printf("usage:FdEvent sec[5|20|-1]\n");
		return( -1 );
	}
	t = atoi( av[1] );

	ADDR.sin_family				= AF_INET;
	ADDR.sin_addr.s_addr	= htonl(0x7f000001);	
	ADDR.sin_port				= htons( PORT );

	Fd = (int)socket( AF_INET, SOCK_STREAM, 0 );
	if( Fd < 0 ) {
		perror("socket:");
		goto err;
	}

	if( bind( Fd, (struct sockaddr*)&ADDR, sizeof(ADDR) ) < 0 ) {
		perror("bind:");
		goto err;
	}

	listen( Fd, 1 );

	Ret = FdEventCtrlCreate( &CTRL );

	FdEventInit( &FdEvListen, 0, Fd, FD_EVENT_READ, &CTRL, AcceptHandler );
	U64	= Fd;
	if( FdEventAdd( &CTRL, U64, &FdEvListen ) < 0 ) {
		perror("FdEventAdd:");
		goto err;
	}
	pthread_create( &th, NULL, Thread, NULL );

	if( t > 0 )	Ret = FdEventLoop( &CTRL, t*1000 );
	else		Ret = FdEventLoop( &CTRL, FOREVER );

	printf("FdEventLoop exit(Ret=%d)\n", Ret );
	perror("");

	pthread_join( th, NULL );

	printf("OK\n");
	return( 0 );
err:
	perror("");
	printf("NG\n");
	return( -1 );
}
