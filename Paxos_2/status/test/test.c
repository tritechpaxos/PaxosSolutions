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

int
main( int ac, char *av[] )
{
	Msg_t	*pMsgNew, *pMsgOld, *pMsgDel, *pMsgUpd;
	StatusEnt_t	*pS;
	char	Path[1024];
	PathEnt_t	*pPath;

	memset( Path, 0, sizeof(Path) );

printf("MallocCount=%d\n", MallocCount() );

	pMsgNew	= MsgCreate( 10, Malloc, Free );
	pMsgOld	= MsgCreate( 10, Malloc, Free );
	pMsgDel	= MsgCreate( 10, Malloc, Free );
	pMsgUpd	= MsgCreate( 10, Malloc, Free );

	STFileList( "New", pMsgNew );
	for( pS = (StatusEnt_t*)MsgFirst(pMsgNew); pS;
			pS = (StatusEnt_t*)MsgNext(pMsgNew) ) {
		printf("New:0x%x [%s] time=%s", 
				pS->e_Type, pS->e_Name, ctime(&pS->e_Time) );
	}
	STFileList( "Old", pMsgOld );
	for( pS = (StatusEnt_t*)MsgFirst(pMsgOld); pS;
			pS = (StatusEnt_t*)MsgNext(pMsgOld) ) {
		printf("Old:0x%x [%s] time=%s", 
				pS->e_Type, pS->e_Name, ctime(&pS->e_Time) );
	}
	STDiffList( pMsgOld, pMsgNew, 0, pMsgDel, pMsgUpd );
	for( pPath = (PathEnt_t*)MsgFirst(pMsgDel); pPath;
			pPath = (PathEnt_t*)MsgNext(pMsgDel) ) {
		printf("Del:p_Len=%d [%s]\n", pPath->p_Len, (char*)(pPath+1) );
	}
	for( pPath = (PathEnt_t*)MsgFirst(pMsgUpd); pPath;
			pPath = (PathEnt_t*)MsgNext(pMsgUpd) ) {
		printf("Upd:p_Len=%d [%s]\n", pPath->p_Len, (char*)(pPath+1) );
	}
	STUnlink( pMsgDel );
	MsgDestroy( pMsgOld );
	MsgDestroy( pMsgNew );
	MsgDestroy( pMsgDel );
	MsgDestroy( pMsgUpd );

printf("MallocCount=%d\n", MallocCount() );
	return( 0 );
}

