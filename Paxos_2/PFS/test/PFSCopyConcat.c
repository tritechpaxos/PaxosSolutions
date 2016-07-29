/******************************************************************************
*
*  PFSCopyConcat.c 	--- Active program
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

/*
 *	作成			渡辺典孝
 *	試験
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_


#include	"PFS.h"
#include	<stdio.h>

int
usage()
{
	printf("PFSCopyConcat cellname {copy|concat} from to\n");
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	Ret;

	if( ac < 5 ) {
		usage();
		exit( -1 );
	}
	struct Session*	pSession;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr, 
					Send, Recv, NULL, NULL, av[1] );
	if( !pSession ) {
		printf("Can't get cell[%s] addr.\n", av[1] );
		exit( -1 );
	}

	Ret	= PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
	if( Ret ) goto err;

	if( !strcmp( "copy", av[2] ) ) {
		Ret = PFSCopy( pSession, av[3], av[4] );
	} else if( !strcmp( "concat", av[2] ) ) {
		Ret = PFSConcat( pSession, av[3], av[4] );
	}
	printf("Ret[%d] errno=%d\n", Ret, errno );

	PaxosSessionClose( pSession );

	return( Ret );
//err1:
//	PaxosSessionClose( pSession );
err:
	return( -1 );
}
