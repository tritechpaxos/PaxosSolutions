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


#include	"Status.h"

void
usage()
{
	printf("STFileList path file\n");
}

int
main( int ac, char *av[] )
{
	Msg_t	*pMsgList;
	StatusEnt_t	*pS;
	IOFile_t	*pF;
	int		Ret = 0;

	if( ac < 3 ) {
		usage();
		Ret = -1;
		goto err;
	}
	pMsgList	= MsgCreate( ST_MSG_SIZE, Malloc, Free );

	Ret = STFileList( av[1], pMsgList );
	if( Ret < 0 )	goto err1;

	pF = IOFileCreate( av[2], O_WRONLY|O_TRUNC|O_CREAT, 0, Malloc, Free );
	if( !pF )	goto err1;

	for( pS = (StatusEnt_t*)MsgFirst(pMsgList); pS;
			pS = (StatusEnt_t*)MsgNext(pMsgList) ) {
		IOWrite( (IOReadWrite_t*)pF, pS, sizeof(*pS) );
	}
	IOFileDestroy( pF, FALSE );
err1:
	MsgDestroy( pMsgList );
err:
	return( Ret );
}

