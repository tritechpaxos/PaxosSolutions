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
STUnlink( Msg_t *pDel )
{
	PathEnt_t	*pPath;
	int	i;

	for( i = pDel->m_N - 1; i >= 0; i-- ) {
		pPath = (PathEnt_t*)pDel->m_aVec[i].v_pStart;
		printf("UNLINK[%s]\n", (char*)(pPath+1) );
	}
	return( 0 );
}

int
STFileList( char *pPath, Msg_t *pMsg )
{
	DIR	*pD;
	struct dirent	*pEnt;
	struct stat	Stat;
	MsgVec_t	Vec;
	StatusEnt_t	*pS;
	char	Path[FILENAME_MAX];

	pD	= opendir( pPath );
	if( !pD )	return( -1 );

	while( (pEnt = readdir( pD )) ) {
		if( !strcmp(".", pEnt->d_name ) || !strcmp("..", pEnt->d_name ) ) {
			continue;
		}
		strcpy( Path, pPath );
		if( Path[0] )	strcat( Path, "/" );
		strcat( Path, pEnt->d_name );
		stat( Path, &Stat );
		switch( pEnt->d_type ) {
			case DT_REG:
				pS	= (StatusEnt_t*)MsgMalloc( pMsg, sizeof(StatusEnt_t) );
				memset( pS, 0, sizeof(*pS) );
				strncpy( pS->e_Name, pEnt->d_name, sizeof(pS->e_Name) );
				pS->e_Time	= Stat.st_mtime;
				pS->e_Type	|= ST_REG;
				pS->e_Size	= Stat.st_size;
				MsgVecSet( &Vec, VEC_MINE, pS, sizeof(*pS), sizeof(*pS) );
				MsgAdd( pMsg, &Vec );
				break;

			case DT_DIR:
				pS	= (StatusEnt_t*)MsgMalloc( pMsg, sizeof(StatusEnt_t) );
				memset( pS, 0, sizeof(*pS) );
				strncpy( pS->e_Name, pEnt->d_name, sizeof(pS->e_Name) );
				pS->e_Time	= Stat.st_mtime;
				pS->e_Type	|= ST_DIR;
				MsgVecSet( &Vec, VEC_MINE, pS, sizeof(*pS), sizeof(*pS) );
				MsgAdd( pMsg, &Vec );

				strcpy( Path, pPath );
				strcat( Path, "/" );
				strcat( Path, pEnt->d_name );
				STFileList( Path, pMsg );

				pS	= (StatusEnt_t*)MsgMalloc( pMsg, sizeof(StatusEnt_t) );
				memset( pS, 0, sizeof(*pS) );
				strncpy( pS->e_Name, "..", sizeof(pS->e_Name) );
				pS->e_Type	|= ST_DIR;
				MsgVecSet( &Vec, VEC_MINE, pS, sizeof(*pS), sizeof(*pS) );
				MsgAdd( pMsg, &Vec );
				break;
			default:
				break;
		}
	}
	closedir( pD );
	return( 0 );
}

Hash_t*
STTreeCreate( Msg_t *pMsg )
{
	Hash_t	*pRoot, *pHash, *pChild;
	StatusEnt_t	*pS;

	pRoot	= (Hash_t*)(pMsg->m_fMalloc)( sizeof(Hash_t) );
	HashInit( pRoot, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL,
				pMsg->m_fMalloc, pMsg->m_fFree, NULL );

	for( pHash = pRoot, pS = (StatusEnt_t*)MsgFirst( pMsg ); pS;
				pS = (StatusEnt_t*)MsgNext( pMsg ) ) {

		if( pS->e_Type & ST_DIR ) {
			if( !strcmp("..", pS->e_Name ) ) {	// Up
				pHash = (Hash_t*)pHash->h_pVoid;
			} else {							// Down
				HashPut( pHash, pS->e_Name, pS );
				pChild	= (Hash_t*)(pMsg->m_fMalloc)( sizeof(Hash_t) );
				HashInit( pChild, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, 
						NULL, pMsg->m_fMalloc, pMsg->m_fFree, NULL );
				pChild->h_pVoid	= pHash;
				pS->e_pVoid	= pChild;
				pHash	= pChild;
			}
		} else {
			HashPut( pHash, pS->e_Name, pS );
		}
	}
	return( pRoot );
}

int
TreePrint( Hash_t *pHash )
{
	int	i;
	HashItem_t	*pItem;
	StatusEnt_t	*pS;

	for( i = 0; i < pHash->h_N; i++ ) {
		if( !list_empty( &pHash->h_aHeads[i] ) ) {
			list_for_each_entry( pItem, &pHash->h_aHeads[i], i_Link ) {
				pS = (StatusEnt_t*)pItem->i_pValue;
				Printf("[%s]\n", pS->e_Name );

				if( pS->e_Type & ST_DIR ) {
				Printf("->\n");
					TreePrint( (Hash_t*)pS->e_pVoid );
				Printf("<-\n");
				}
			}
		}
	}
	return( 0 );
}

int
STTreeDestroy( Hash_t *pHash )
{
	int	i;
	HashItem_t	*pItem;
	StatusEnt_t	*pS;

	for( i = 0; i < pHash->h_N; i++ ) {
		if( !list_empty( &pHash->h_aHeads[i] ) ) {
			list_for_each_entry( pItem, &pHash->h_aHeads[i], i_Link ) {
				pS = (StatusEnt_t*)pItem->i_pValue;

				if( pS->e_Type & ST_DIR ) {
					STTreeDestroy( (Hash_t*)pS->e_pVoid );
				}
			}
		}
	}
	HashDestroy( pHash );
	(pHash->h_fFree)( pHash );
	return( 0 );
}

StatusEnt_t*
STTreeFind( Msg_t* pDir, StatusEnt_t *pOldS, Hash_t *pTree )
{
	StatusEnt_t	*pOld, *pNew = NULL;
	
	for( pOld = (StatusEnt_t*)MsgFirst( pDir ); pOld;
				pOld = (StatusEnt_t*)MsgNext( pDir ) ) {
		if( !pTree )	return( NULL );
		pNew = (StatusEnt_t*)HashGet( pTree, pOld->e_Name );
		if( !pNew )	return( NULL );
		if( pNew->e_Type & ST_DIR ) {
		/*
		 * 旧にもディレクトリがある
		 */
			pNew->e_Type	|= ST_NON;

			pTree	= (Hash_t*)pNew->e_pVoid;
		}
	}
	if( pOldS )	pNew = (StatusEnt_t*)HashGet( pTree, pOldS->e_Name );
	return( pNew );	
}

StatusEnt_t*
STTreeFindByPath( char *pPath, Hash_t *pTree )	// pPath be changed
{
	StatusEnt_t	*pNew = NULL;
	char	*pT;
	
	for( pT = strtok( pPath, "/"); pT; pT = strtok( NULL, "/") ) {
		
		if( !pTree )	return( NULL );
		pNew = (StatusEnt_t*)HashGet( pTree, pT );
		if( !pNew )	return( NULL );
		if( pNew->e_Type & ST_DIR ) {
		/*
		 * 旧にもディレクトリがある
		 */
			pNew->e_Type	|= ST_NON;

			pTree	= (Hash_t*)pNew->e_pVoid;
		}
	}
	return( pNew );	
}

int
STDiffList( Msg_t *pMsgOld, Msg_t *pMsgNew, time_t Alive,
				Msg_t *pMsgDel, Msg_t *pMsgUpd )
{
	Hash_t		*pHashNew;
	StatusEnt_t	*pOldS, *pNewS;
	Msg_t		*pDir;
	MsgVec_t	Vec, *pVec;
	int			i;
	char		Dir[FILENAME_MAX], *p;
	PathEnt_t	*pPath, *pPathPrev;
	int			len;

	pHashNew	= STTreeCreate( pMsgNew );
	pDir = MsgCreate( ST_MSG_SIZE, pMsgNew->m_fMalloc, pMsgNew->m_fFree );
	Dir[0]	= 0;

	for( pOldS = (StatusEnt_t*)MsgFirst( pMsgOld ); pOldS;
			pOldS = (StatusEnt_t*)MsgNext( pMsgOld ) ) {

		if( pOldS->e_Type & ST_DIR ) {
			if( strcmp( pOldS->e_Name, ".." ) ) {

				if( Dir[0] )	strcat( Dir, "/" );
				strcat( Dir, pOldS->e_Name );

				MsgVecSet( &Vec, 0, pOldS, sizeof(*pOldS), sizeof(*pOldS) );
				if( pDir->m_N >= pDir->m_Size ) {
					pVec	= (pDir->m_fMalloc)(pDir->m_Size*2);
					for( i = 0; i < pDir->m_N; i++ ) {
						*pVec	= pDir->m_aVec[i];
					}
					(pDir->m_fFree)( pDir->m_aVec );
					pDir->m_aVec	= pVec;
				}
				pDir->m_aVec[pDir->m_N++]	= Vec;
				pNewS	= STTreeFind( pDir, NULL, pHashNew );
			} else {
				len	= strlen( Dir );
				while( len > 0 && Dir[len] != '/' )	len--;
				Dir[len]	= 0;

				pDir->m_N--;
				continue;
			}
		} else {
			pNewS	= STTreeFind( pDir, pOldS, pHashNew );
		}
		if( pNewS ) {
			/* バックアップ時にはキャッシュアウトされている
			 * 旧のダウン時(Alive)より充分な過去について変更なしの判断をする
			 * 但し、Alive=0は全員がダウンし再起動されたデバッグモードとし
			 *	特別扱いとする
			 *	pOldのST_NONはインクリメンタルで反映済みを意味する
			 */
#ifdef	ZZZ
			if( !Alive || pNewS->e_Time < Alive - ST_DT ) {
				if( pNewS->e_Time - ST_DT < pOldS->e_Time ) {
					pNewS->e_Type	|= ST_NON;
				}
			}
#else
#define	ABS( x, y )	( (x)>(y) ? (x)-(y) : (y)-(x) )

			if( pOldS->e_Type & ST_NON ) {
				if( ABS( pNewS->e_Time, pOldS->e_Time ) < 1 ) {
					pNewS->e_Type	|= ST_NON;
				}
			} else {
				if( !Alive || pNewS->e_Time < Alive - ST_DT ) {
					if( pNewS->e_Time - ST_DT < pOldS->e_Time ) {
						pNewS->e_Type	|= ST_NON;
					}
				}
			}
#endif
		} else {
			len	= sizeof(PathEnt_t) + strlen( Dir ) + 1;
			if( !(pOldS->e_Type & ST_DIR ) )	len += strlen(pOldS->e_Name)+1;

			pPath	= (PathEnt_t*)MsgMalloc( pMsgDel, len );
			pPath->p_Type	= pOldS->e_Type;
			pPath->p_Time	= pOldS->e_Time;
			p	= (char*)(pPath+1);
			strcpy( p, Dir );
			if( !(pOldS->e_Type & ST_DIR ) ) {
				if( *p )	strcat( p, "/" );
				strcat( p, pOldS->e_Name );
			}
			pPath->p_Len	= strlen( (char*)(pPath+1) );
			MsgVecSet( &Vec, VEC_MINE, pPath, len, len );
			MsgAdd( pMsgDel, &Vec );
		}
	}
	Dir[0]	= 0;
	for( pNewS = (StatusEnt_t*)MsgFirst( pMsgNew ); pNewS;
			pNewS = (StatusEnt_t*)MsgNext( pMsgNew ) ) {

		if( pNewS->e_Type & ST_DIR ) {
			if( strcmp( pNewS->e_Name, ".." ) ) {

				strcat( Dir, pNewS->e_Name );
				strcat( Dir, "/" );
			} else {
				len	= strlen( Dir );
				len	-= 2;
				while( len > 0 && Dir[len] != '/' )	len--;
				if( len > 0 ) 	Dir[len+1]	= 0;
				else			Dir[0] = 0;
				continue;
			}
		}
		if( pNewS->e_Type & ST_NON )	continue;

		len	= sizeof(PathEnt_t) + strlen( Dir ) + 1;
		if( !(pNewS->e_Type & ST_DIR ) )	len += strlen(pNewS->e_Name)+1;

		pPath	= (PathEnt_t*)MsgMalloc( pMsgUpd, len );
		pPath->p_Type	= pNewS->e_Type;
		pPath->p_Time	= pNewS->e_Time;
		pPath->p_Size	= pNewS->e_Size;
		p	= (char*)(pPath+1);
		strcpy( p, Dir );
		if( !(pNewS->e_Type & ST_DIR ) ) {
			strcat( p, pNewS->e_Name );
		}
		pPath->p_Len	= strlen( (char*)(pPath+1) );
		MsgVecSet( &Vec, VEC_MINE, pPath, len, len );
		/*
		 * 旧にディレクトリがない場合、新のパスリストが連続するので
		 * 前のディレクトリをディセーブルする
		 */
		if( pMsgUpd->m_N > 0 ) {
			pVec		= &pMsgUpd->m_aVec[pMsgUpd->m_N-1];
			pPathPrev	= (PathEnt_t*)pVec->v_pStart;
			if( pPathPrev->p_Type & ST_DIR 
					&& pPathPrev->p_Len < pPath->p_Len ) {
				if( !strncmp( (char*)(pPathPrev+1), (char*)(pPath+1), 
									pPathPrev->p_Len ) ) {
					pVec->v_Flags	|= VEC_DISABLED;
				}
			}
		}
		MsgAdd( pMsgUpd, &Vec );
	}
	STTreeDestroy( pHashNew );
	MsgDestroy( pDir );

	return( 0 );
}

