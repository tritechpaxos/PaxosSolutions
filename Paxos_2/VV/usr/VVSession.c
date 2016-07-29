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

#include	"PaxosSession.h"

struct	Log	*pLog = NULL;

/*	Session */
struct Session*
VVSessionOpen( char *pCellName, int SessionNo, bool_t IsSync )
{
	struct Session *pSession = NULL; 
	int	Ret;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, Malloc, Free, pCellName );
	if( !pSession )	goto err;

	if( pLog )	PaxosSessionSetLog( pSession, pLog );

	Ret	= PaxosSessionOpen( pSession, SessionNo, IsSync );
	if( Ret != 0 ) {
		errno = EACCES;
		goto err1;
	}
	return( pSession );
err1:
	PaxosSessionFree( pSession );
err:
	return( NULL );
}

void
VVSessionClose( void * pSession )
{
	PaxosSessionClose( (struct Session*)pSession );
	PaxosSessionFree( pSession );
}

void
VVSessionAbort( void * pSession )
{
	PaxosSessionAbort( (struct Session*)pSession );
	PaxosSessionFree( pSession );
}


