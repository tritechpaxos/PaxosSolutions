/******************************************************************************
*
*  PFSRasCluster.c 	--- Administration
*	ras	--- Register myself into RAS Cell
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
 *	試験			
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_


#include	<stdio.h>
#include	"PFS.h"

int
usage()
{
	printf("PFSRasCluster cell id ras_cell path\n");
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	j;
	int	Ret;
	char	*pCell, *pPath, *pRas;
	int	Fd;
	int	Len;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	pCell	= av[1];
	j 		= atoi( av[2] );
	pRas	= av[3];
	pPath	= av[4];

	Fd	= ConnectAdmPort( pCell, j );
	if( Fd < 0 ) {
		printf("No AdmPort[%s_%d]\n", pCell, j );
		goto err;
	}
/*
 *	send RAS command
 */
	PaxosSessionHead_t	Any;

	MSGINIT( &Any, PAXOS_SESSION_ANY, 0, sizeof(Any)+sizeof(PFSRASReq_t));
	Ret = SendStream( Fd, (char*)&Any, sizeof(Any) );
	if( Ret ) {
		printf("ERR:SendStream[%s_%d] ANY\n", pCell, j );
		goto err1;
	}

	PFSRASReq_t	Req;
	PFSRASRpl_t	Rpl;

	memset( &Req, 0, sizeof(Req));

	MSGINIT( &Req, PFS_RAS, 0, sizeof(Req) );
	strncpy( Req.r_Cell, pRas, sizeof(Req.r_Cell) );
	strncpy( Req.r_Path, pPath, sizeof(Req.r_Path) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret ) {
		printf("ERR:SendStream[%s_%d]\n", pCell, j );
		goto err1;
	}
	Len	= sizeof(Rpl);
	Ret = RecvStream( Fd, (char*)&Rpl, Len );
	if( Ret ) {
		printf("ERR:RecvStream[%s_%d]\n", pCell, j );
		goto err1;
	}
	if( Rpl.r_Head.h_Error ) {
		errno = Rpl.r_Head.h_Error;
		perror(" ");
	}
	close( Fd );
	exit( 0 );
err1:
	close( Fd );
err:
	exit( -1 );
}
