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
	printf("STDiff new_path old_list delete_file update_file\n");
}

int
main( int ac, char *av[] )
{
	Msg_t	*pMsgOld, *pMsgNew, *pMsgDel, *pMsgUpd;
	StatusEnt_t	*pS, S;
	IOFile_t	*pF;
	int		Ret = 0;
	MsgVec_t	Vec, *pVec;
	int	i;
	PathEnt_t	*pP;
	char		Path[FILENAME_MAX];

	if( ac < 5 ) {
		usage();
		Ret = -1;
		goto err;
	}
	pMsgOld	= MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgNew	= MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgDel	= MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgUpd	= MsgCreate( ST_MSG_SIZE, Malloc, Free );

	/*
	 * Old
	 */
	pF = IOFileCreate( av[2], O_RDONLY, 0, Malloc, Free );
	if( !pF )	goto err1;

	while( IORead( (IOReadWrite_t*)pF, &S, sizeof(S) ) > 0 ) {
		pS = (StatusEnt_t*)MsgMalloc( pMsgOld, sizeof(*pS) );
		*pS	= S;
		MsgVecSet( &Vec, VEC_MINE, pS, sizeof(*pS), sizeof(*pS) );
		MsgAdd( pMsgOld, &Vec );
	}
	IOFileDestroy( pF, TRUE );

	/*
	 * New
	 */
	Ret = STFileList( av[1], pMsgNew );
	if( Ret < 0 )	goto err1;

	/*
	 * Diff
	 */
	STDiffList( pMsgOld, pMsgNew, 0, pMsgDel, pMsgUpd );

	/*
	 * Delete
	 */
	pF = IOFileCreate( av[3], O_WRONLY|O_TRUNC|O_CREAT, 0, Malloc, Free );
	if( !pF )	goto err1;

	for( i = 0; i < pMsgDel->m_N; i++ ) {
		pVec	= &pMsgDel->m_aVec[i];
		IOWrite( (IOReadWrite_t*)pF, &pVec->v_Len, sizeof(pVec->v_Len) );
		IOWrite( (IOReadWrite_t*)pF, pVec->v_pStart, pVec->v_Len );
	}
	IOFileDestroy( pF, TRUE );

	/*
	 * Update
	 */
	pF = IOFileCreate( av[4], O_WRONLY|O_TRUNC|O_CREAT, 0, Malloc, Free );
	if( !pF )	goto err1;

	for( pP = (PathEnt_t*)MsgFirst(pMsgUpd); pP;
				pP = ( PathEnt_t*)MsgNext(pMsgUpd) ) {

		sprintf( Path, "%s/%s\n", av[1], (char*)(pP+1) );
		IOWrite( (IOReadWrite_t*)pF, Path, strlen(Path) );
	}
	IOFileDestroy( pF, TRUE );
err1:
	MsgDestroy( pMsgOld );
	MsgDestroy( pMsgNew );
	MsgDestroy( pMsgDel );
	MsgDestroy( pMsgUpd );
err:
	return( Ret );
}

