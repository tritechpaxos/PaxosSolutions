/******************************************************************************
*
*  PFSClient.c 	--- client part of PFS Library
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

/*
 *	作成			渡辺典孝
 *	試験			塩尻常春、久保正業
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

//#define	_PFS_SERVER_

#include	"PFS.h"
#include	<stdio.h>

File_t*
PFSOpen( struct Session* pSession, char *pathname, int Flags )
{
	File_t*	pFile;

	DBG_ENT();

	if( strstr(pathname, BLOCK_CKSUM_FILE_SUFFIX ) ) {
		errno	= ENOENT;
		goto err1;
	}
	pFile	= (File_t*)Malloc( sizeof(File_t) );
	if( pFile == NULL )	goto err1;
	memset( pFile, 0, sizeof(*pFile) );
	strncpy( pFile->f_Name, pathname, sizeof(pFile->f_Name) );
	pFile->f_pSession	= pSession;
	pFile->f_Flags		= Flags;

	RETURN( pFile );
err1:
	RETURN( NULL );
}

int
PFSClose( File_t* pF )
{
	Free( pF );
	return( 0 );
}

long
PFSWriteOutOfBand( File_t* pF, char* pBuff, size_t Len )
{
	PFSOutOfBandReq_t	Req;
	PFSOutOfBandRpl_t*	pRpl = NULL;
	long	Ret = 0;
	PFSWriteReq_t		ReqWrite;
	struct Session*	pSession = pF->f_pSession;
	PaxosSessionHead_t	ReqPush;
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	pMsgReq	= MsgCreate( 4, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &ReqWrite, PFS_WRITE, 0, 0 );
	strncpy( ReqWrite.w_Name, pF->f_Name, sizeof(ReqWrite.w_Name) );
	ReqWrite.w_Offset		= pF->f_Offset;
	ReqWrite.w_Len			= Len;
	ReqWrite.w_Flags		= pF->f_Flags;

	SESSION_MSGINIT( &ReqPush, PAXOS_SESSION_OB_PUSH, PFS_WRITE, 0,
				sizeof(ReqPush) + sizeof(ReqWrite) + Len );

	MSGINIT( &Req, PFS_OUTOFBAND, 0, sizeof(Req) );

	MsgVecSet( &Vec, VEC_HEAD, &ReqPush, sizeof(ReqPush), sizeof(ReqPush) );
	MsgAdd( pMsgReq, &Vec );

	MsgVecSet( &Vec, VEC_HEAD, &ReqWrite, sizeof(ReqWrite), sizeof(ReqWrite) );
	MsgAdd( pMsgReq, &Vec );

	MsgVecSet( &Vec, 0, pBuff, Len, Len );
	MsgAdd( pMsgReq, &Vec );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	ASSERT( pMsgReq->m_N == 4 );

	ReqWrite.w_Head.h_Head.h_Cksum64	= 0;
	ReqWrite.w_Head.h_Head.h_Cksum64	= MsgCksum64( pMsgReq, 1, 3 , NULL);

	ReqPush.h_Cksum64	= 0;
	if( sizeof(ReqPush) & 0x7 ) {
		ReqPush.h_Cksum64	= MsgCksum64( pMsgReq, 0, 3 , NULL);
	} else {
		ReqPush.h_Cksum64	= in_cksum64_update( 
									ReqWrite.w_Head.h_Head.h_Cksum64,
									NULL, 0, 0,
									(char*)&ReqPush, sizeof(ReqPush), 0 );
	}

	Req.o_Head.h_Head.h_Cksum64		= 0;
	Req.o_Head.h_Head.h_Cksum64		= MsgCksum64( pMsgReq, 3, 1, NULL );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE ); 
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PFSOutOfBandRpl_t*)MsgFirst( pMsgRpl );
		errno = pRpl->o_Head.h_Error;
		MsgDestroy( pMsgRpl );
		if( errno )	{
			Ret = -1;
			goto err1;
		}
	} else {
		Ret = -1;
		goto err1;
	}
	if( pF->f_Offset != PFS_APPEND )	pF->f_Offset	+= Len;
	Ret	= Len;

err1:
	RETURN( Ret );
}

long
PFSWriteInBand( File_t* pF, char* pBuff, size_t Len )
{
	PFSWriteReq_t	*pReq;
	PFSWriteRpl_t*	pRpl = NULL;
	struct Session	*pSession = pF->f_pSession;
	long	len, Total = 0;
	long	s;
	long	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	while( Len > 0 ) {

		len	= ( Len > DATA_SIZE_WRITE ? DATA_SIZE_WRITE : Len );

		pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
								PaxosSessionGetfFree(pSession) );

		s	= sizeof(*pReq) + len;
		pReq	= MsgMalloc( pMsgReq, s );
		MSGINIT( pReq, PFS_WRITE, 0, s );
		MsgVecSet( &Vec, VEC_MINE|VEC_HEAD|VEC_REPLY, pReq, s, s );
		MsgAdd( pMsgReq, &Vec );

		strncpy( pReq->w_Name, pF->f_Name, sizeof(pReq->w_Name) );
		pReq->w_Offset	= pF->f_Offset;
		memcpy( pReq->w_Data, pBuff, len );
		pReq->w_Len	= len;
		pReq->w_Flags		= pF->f_Flags;

		pReq->w_Head.h_Head.h_Cksum64	= 0;
		pReq->w_Head.h_Head.h_Cksum64	= in_cksum64( pReq, s, 0 );

		pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
		MsgDestroy( pMsgReq );
		if( pMsgRpl ) {
			pRpl	= (PFSWriteRpl_t*)MsgFirst( pMsgRpl );
			errno	= pRpl->w_Head.h_Error;
			MsgDestroy( pMsgRpl );
			if( errno )	 {
				Ret = -1;
				goto err1;
			}
		} else {
			Ret = -1;
			goto err1;
		}
		if( pF->f_Offset != PFS_APPEND )	pF->f_Offset	+= len;
		Total			+= len;
		pBuff			+= len;
		Len				-= len;
	}
	Ret	= Total;	
err1:
	RETURN( Ret );
}

long
PFSWrite( File_t* pF, char* pBuff, size_t Len )
{
	long	Ret;

	if( Len > DATA_SIZE_WRITE )	Ret = PFSWriteOutOfBand( pF, pBuff, Len );
	else						Ret = PFSWriteInBand( pF, pBuff, Len );
	return( Ret );
}

int
PFSCreate( struct Session* pSession, char *pathname, int Flag )
{
	PFSCreateReq_t	Req;
	PFSCreateRpl_t*	pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &Req, PFS_CREATE, 0, sizeof(Req) );
	strncpy( Req.c_Name, pathname, sizeof(Req.c_Name) );
	Req.c_Flags	= Flag;

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.c_Head.h_Head.h_Cksum64	= 0;
	Req.c_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSCreateRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->c_Head.h_Error;

		LOG( pLOG, LogINF, "h_Error=%d", pRpl->c_Head.h_Error );

		MsgDestroy( pMsgRpl );
		if( errno )	 {
			goto err1;
		}
	} else {
		goto err1;
	}
	RETURN( 0 );
err1:
	RETURN( -1 );
}

int
PFSDelete( struct Session* pSession, char *pathname )
{
	PFSDeleteReq_t	Req;
	PFSDeleteRpl_t*	pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &Req, PFS_DELETE, 0, sizeof(Req) );
	strncpy( Req.d_Name, pathname, sizeof(Req.d_Name) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.d_Head.h_Head.h_Cksum64	= 0;
	Req.d_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSDeleteRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->d_Head.h_Error;

		LOG( pLOG, LogINF, "h_Error=%d", pRpl->d_Head.h_Error );

		MsgDestroy( pMsgRpl );
		if( errno )	 {
			goto err1;
		}
	} else {
		goto err1;
	}
	RETURN( 0 );
err1:
	RETURN( -1 );
}

int
PFSTruncate( struct Session* pSession, char *pathname, off_t Len )
{
	PFSTruncateReq_t	Req;
	PFSTruncateRpl_t*	pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &Req, PFS_TRUNCATE, 0, sizeof(Req) );
	strncpy( Req.t_Name, pathname, sizeof(Req.t_Name) );
	Req.t_Len	= Len;

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.t_Head.h_Head.h_Cksum64	= 0;
	Req.t_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSTruncateRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->t_Head.h_Error;

		LOG( pLOG, LogINF, "h_Error=%d", pRpl->t_Head.h_Error );

		MsgDestroy( pMsgRpl );
		if( errno )	 {
			goto err1;
		}
	} else {
		goto err1;
	}
	RETURN( 0 );
err1:
	RETURN( -1 );
}

int
PFSStat( struct Session* pSession, char *pathname, struct stat *pStat )
{
	PFSStatReq_t	Req;
	PFSStatRpl_t*	pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	if( strstr(pathname, BLOCK_CKSUM_FILE_SUFFIX ) ) {
		errno	= ENOENT;
		goto err1;
	}
	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &Req, PFS_STAT, 0, sizeof(Req) );
	strncpy( Req.s_Name, pathname, sizeof(Req.s_Name) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.s_Head.h_Head.h_Cksum64	= 0;
	Req.s_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSStatRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->s_Head.h_Error;
		if( !errno ) {
			memcpy( pStat, &pRpl->s_Stat, sizeof(pRpl->s_Stat) );
		}

		LOG( pLOG, LogINF, "h_Error=%d", pRpl->s_Head.h_Error );

		MsgDestroy( pMsgRpl );
		if( errno )	 {
			goto err1;
		}
	} else {
		goto err1;
	}
	RETURN( 0 );
err1:
	RETURN( -1 );
}

int
PFSLseek( File_t* pF, off_t Offset )
{
	DBG_ENT();
	pF->f_Offset	= Offset;
	RETURN( Offset );
}

long
PFSRead( File_t* pF, char* pBuff, size_t Len )
{
	struct Session	*pSession = pF->f_pSession;
	PFSReadReq_t	Req;
	PFSReadRpl_t	*pRpl = NULL;
	long	Ret;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_READ, 0, sizeof(Req) );
	strncpy( Req.r_Name, pF->f_Name, sizeof(Req.r_Name) );
	Req.r_Offset	= pF->f_Offset;
	Req.r_Len		= Len;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.r_Head.h_Head.h_Cksum64	= 0;
	Req.r_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSReadRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->r_Head.h_Error;

		LOG( pLOG, LogINF, "h_Error=%d", pRpl->r_Head.h_Error );

		if( errno )	 {
			MsgDestroy( pMsgRpl );
			Ret = -1;
			goto err1;
		} else {
			Ret	= pRpl->r_Len;
			MsgTrim( pMsgRpl, sizeof(PFSReadRpl_t) );

			/* コピーがある　！！！ */
			MsgCopyToBuf( pMsgRpl, pBuff, (Ret > Len ? Len : Ret ) );
			MsgDestroy( pMsgRpl );
			pF->f_Offset	+= Ret;
		}
	} else {
		Ret = -1;
		goto err1;
	}
err1:
	RETURN( Ret );
}

int
_PFSLockW( struct Session* pSession, char *pathname, int *pVer, bool_t bTry )
{
	PFSLockReq_t	Req;
	PFSLockRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_LOCK, 0, sizeof(Req) );
	strncpy( Req.l_Name, pathname, sizeof(Req.l_Name) );

	Req.l_bRead	= FALSE;
	Req.l_bTry	= bTry;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.l_Head.h_Head.h_Cksum64	= 0;
	Req.l_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

//	LOG( pLOG, "pathname[%s]", pathname );
	/* ロックは永遠に待つ ただし、TRY_LOCKは待たない　*/
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSLockRpl_t*)MsgFirst( pMsgRpl );
		*pVer	= pRpl->l_Ver;
		errno	= pRpl->l_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSLockW( struct Session* pSession, char *pathname )
{
	int	Ver;
	int	Ret;

	Ret	= _PFSLockW( pSession, pathname, &Ver, FALSE );
	return( Ret );
}

int
PFSTryLockW( struct Session* pSession, char *pathname )
{
	int	Ver;
	int	Ret;

	Ret	= _PFSLockW( pSession, pathname, &Ver, TRUE );
	return( Ret );
}

int
_PFSLockR( struct Session* pSession, char *pathname, int *pVer, bool_t bTry )
{
	PFSLockReq_t	Req;
	PFSLockRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_LOCK, 0, sizeof(Req) );
	strncpy( Req.l_Name, pathname, sizeof(Req.l_Name) );
	Req.l_bRead	= TRUE;
	Req.l_bTry	= bTry;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.l_Head.h_Head.h_Cksum64	= 0;
	Req.l_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

//	LOG( pLOG, "pathname[%s]", pathname );
	/* ロックは永遠に待つ ただし、TRY_LOCKは待たない　*/
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSLockRpl_t*)MsgFirst( pMsgRpl );
		*pVer	= pRpl->l_Ver;
		errno	= pRpl->l_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSLockR( struct Session* pSession, char *pathname )
{
	int	Ver;
	int	Ret;

	Ret	= _PFSLockR( pSession, pathname, &Ver, FALSE );
	return( Ret );
}

int
PFSTryLockR( struct Session* pSession, char *pathname )
{
	int	Ver;
	int	Ret;

	Ret	= _PFSLockR( pSession, pathname, &Ver, TRUE );
	return( Ret );
}

int
PFSUnlock( struct Session* pSession, char *pathname )
{
	PFSUnlockReq_t	Req;
	PFSUnlockRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	LOG( pLOG, LogINF, "pathname[%s]", pathname );

	MSGINIT( &Req, PFS_UNLOCK, 0, sizeof(Req) );
	strncpy( Req.l_Name, pathname, sizeof(Req.l_Name) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.l_Head.h_Head.h_Cksum64	= 0;
	Req.l_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSUnlockRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->l_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSUnlockSuper( struct Session* pSession, struct Session *pOwner, char *path )
{
	PFSUnlockSuperReq_t	Req;
	PFSUnlockSuperRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	LOG( pLOG, LogINF, "pOwner=%p path[%s]", pOwner, path );

	MSGINIT( &Req, PFS_UNLOCK_SUPER, 0, sizeof(Req) );
	strncpy( Req.l_Name, path, sizeof(Req.l_Name) );
	Req.l_Id	= *PaxosSessionGetClientId( pOwner );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.l_Head.h_Head.h_Cksum64	= 0;
	Req.l_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSUnlockSuperRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->l_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSMkdir( struct Session* pSession, char *pathname )
{
	PFSMkdirReq_t	Req;
	PFSMkdirRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_MKDIR, 0, sizeof(Req) );
	strncpy( Req.m_Name, pathname, sizeof(Req.m_Name) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.m_Head.h_Head.h_Cksum64	= 0;
	Req.m_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSMkdirRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->m_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSRmdir( struct Session* pSession, char *pathname )
{
	PFSRmdirReq_t	Req;
	PFSRmdirRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_RMDIR, 0, sizeof(Req) );
	strncpy( Req.r_Name, pathname, sizeof(Req.r_Name) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.r_Head.h_Head.h_Cksum64	= 0;
	Req.r_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSRmdirRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->r_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSReadDir( struct Session* pSession, char *pathname, 
		PFSDirent_t* pE, int32_t Index, int32_t* pNumber )
{
	PFSReadDirReq_t	Req;
	PFSReadDirRpl_t*	pRpl = NULL;
	int	i;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	MSGINIT( &Req, PFS_READDIR, 0, sizeof(Req) );
	strncpy( Req.r_Name, pathname, sizeof(Req.r_Name) );
	Req.r_Index		= Index;
	Req.r_Number	= *pNumber;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.r_Head.h_Head.h_Cksum64	= 0;
	Req.r_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSReadDirRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->r_Head.h_Error;
		if( errno )	{
			Ret = -1;
		} else {
			*pNumber	= pRpl->r_Number;
			PFSDirent_t	*pDirent = (PFSDirent_t*)(pRpl+1);
			for( i = 0; i < *pNumber; i++ ) {
				pE[i]	= pDirent[i];
			}
		}
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	RETURN( Ret );
}

int
PFSEventDirSet( struct Session* pSession, char* pName, struct Session *pE )
{
	PFSEventDirReq_t	Req;
	PFSEventDirRpl_t*	pRpl = NULL;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	PaxosClientId_t	*pId;

	pId	= PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_DIR, 0, sizeof(Req) );
	strncpy( Req.ed_Name, pName, sizeof(Req.ed_Name) );
	Req.ed_Id	= *pId;
	Req.ed_Set	= TRUE;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.ed_Head.h_Head.h_Cksum64	= 0;
	Req.ed_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

LOG( pLOG, LogDBG, "BEFORE:ReqAndRpl pSession=%p", pSession );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSEventDirRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->ed_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
LOG( pLOG, LogDBG, "AFTER:ReqAndRpl pSession=%p Ret=%d errno=%d", 
		pSession, Ret, errno );

err:
	RETURN( Ret );
}

int
PFSEventDirCancel( struct Session* pSession, char* pName, struct Session *pE )
{
	PFSEventDirReq_t	Req;
	PFSEventDirRpl_t*	pRpl = NULL;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	PaxosClientId_t	*pId;

	pId	= PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_DIR, 0, sizeof(Req) );
	strncpy( Req.ed_Name, pName, sizeof(Req.ed_Name) );
	Req.ed_Id	= *pId;
	Req.ed_Set	= FALSE;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	Req.ed_Head.h_Head.h_Cksum64	= 0;
	Req.ed_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	if( pMsgRpl ) {
		pRpl	= (PFSEventDirRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->ed_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
err:
	RETURN( Ret );
}

int
_PFSEventLockSet( struct Session* pSession, 
						char* pName, struct Session *pE, bool_t bRecall )
{
	PFSEventLockReq_t	Req;
	PFSEventLockRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	PaxosClientId_t	*pId;

	DBG_ENT();

	pId	= PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_LOCK, 0, sizeof(Req) );
	strncpy( Req.el_Name, pName, sizeof(Req.el_Name) );
	Req.el_Id	= *pId;
	Req.el_Set	= TRUE;
	Req.el_bRecall	= bRecall;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.el_Head.h_Head.h_Cksum64	= 0;
	Req.el_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSEventLockRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->el_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
err:
	RETURN( Ret );
}

int
PFSEventLockCancel( struct Session* pSession, char* pName, struct Session *pE )
{
	PFSEventLockReq_t	Req;
	PFSEventLockRpl_t*	pRpl;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	PaxosClientId_t	*pId;

	DBG_ENT();

	pId	= PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_LOCK, 0, sizeof(Req) );
	strncpy( Req.el_Name, pName, sizeof(Req.el_Name) );
	Req.el_Id	= *pId;
	Req.el_Set	= FALSE;

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	Req.el_Head.h_Head.h_Cksum64	= 0;
	Req.el_Head.h_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	if( pMsgRpl ) {
		pRpl	= (PFSEventLockRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->el_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
err:
	RETURN( Ret );
}

int
PFSEventGetByCallback( struct Session* pSession, 
						int32_t* pCnt, int32_t* pOmitted,
						int(*fCallback)(struct Session*,PFSHead_t*) )
{
	PaxosSessionHead_t		Req;
	PaxosSessionHead_t*	pRpl = NULL;
	PFSHead_t*	pF;
	int	i;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	SESSION_MSGINIT( &Req, PAXOS_SESSION_EVENT, 0, 0, sizeof(Req) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.h_Cksum64	= 0;
	Req.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

	/* 永遠に待つ */
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PaxosSessionHead_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->h_Error;
		if( errno )	 {
			Ret = -1;
			MsgDestroy( pMsgRpl );
			goto err1;
		}
	} else {
		Ret = -1;
		goto err1;
	}
	*pCnt		= pRpl->h_EventCnt;
	*pOmitted	= pRpl->h_EventOmitted;
	pF = (PFSHead_t*)(pRpl+1);
	for( i = 0; i < pRpl->h_EventCnt; 
			i++, pF = (PFSHead_t*)((char*)pF + pF->h_Head.h_Len) ) {

		(*fCallback)( pSession, pF );

	}
	MsgDestroy( pMsgRpl );
err1:
	return( Ret );	
}

PFSReadRpl_t*
PFSReadDirect( void* pH, File_t* pF, size_t Len )
{
	PFSReadReq_t	Req;
	PFSReadRpl_t	*pRpl;

	DBG_ENT();

	MSGINIT( &Req, PFS_READ, 0, sizeof(Req) );
	strncpy( Req.r_Name, pF->f_Name, sizeof(Req.r_Name) );
	Req.r_Offset	= pF->f_Offset;
	Req.r_Len		= Len;

	pRpl = (PFSReadRpl_t*)
			PaxosSessionAny( pF->f_pSession, pH, (PaxosSessionHead_t*)&Req );
	if( !pRpl ) goto err1;

	if( pRpl->r_Head.h_Error ) {
		errno	= pRpl->r_Head.h_Error;
		Free( pRpl );
		pRpl = NULL;
		goto err1;
	}
	if( pRpl->r_Len ) {
		if( pF->f_Offset != PFS_APPEND )	pF->f_Offset	+= pRpl->r_Len;
	} else {
		Free( pRpl );
		pRpl = NULL;
	}
err1:
	RETURN( pRpl );
}

int
PFSPostOutOfBand( struct Session *pSession, 
			char *pQueue, char* pBuff, size_t Len )
{
	PFSOutOfBandReq_t	Req;
	PFSPostMsgReq_t		ReqPost;
	PaxosSessionHead_t	ReqPush;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	int	Ret;

	pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	SESSION_MSGINIT( &ReqPush, PAXOS_SESSION_OB_PUSH, PFS_POST_MSG, 0,
				sizeof(ReqPush) + sizeof(ReqPost) + Len );
	MsgVecSet( &Vec, VEC_HEAD, &ReqPush, sizeof(ReqPush), sizeof(ReqPush) );
	MsgAdd( pMsgReq, &Vec );

	MSGINIT( &ReqPost, PFS_POST_MSG, 0, sizeof(ReqPost) + Len );
	strncpy( ReqPost.p_Name, pQueue, sizeof(ReqPost.p_Name) );
	ReqPost.p_Len			= Len;

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, 
					&ReqPost, sizeof(ReqPost), sizeof(ReqPost) );
	MsgAdd( pMsgReq, &Vec );

	MsgVecSet( &Vec, 0, pBuff, Len, Len );
	MsgAdd( pMsgReq, &Vec );

	MSGINIT( &Req, PFS_OUTOFBAND, 0, sizeof(Req) );

	MsgVecSet( &Vec, VEC_HEAD, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	ASSERT( pMsgReq->m_N == 4 );

	ReqPost.p_Head.h_Head.h_Cksum64	= 0;
	ReqPost.p_Head.h_Head.h_Cksum64	= MsgCksum64( pMsgReq, 1, 3, NULL );

	ReqPush.h_Cksum64	= 0;
	if( sizeof(ReqPush) & 0x7 ) {
		ReqPush.h_Cksum64	= MsgCksum64( pMsgReq, 0, 3, NULL );
	} else {
		ReqPush.h_Cksum64	= in_cksum64_update( 
									ReqPost.p_Head.h_Head.h_Cksum64,
									NULL, 0, 0,
									(char*)&ReqPush, sizeof(ReqPush), 0 );
	}

	Req.o_Head.h_Head.h_Cksum64		= 0;
	Req.o_Head.h_Head.h_Cksum64		= MsgCksum64( pMsgReq, 3, 1, NULL );

	pMsgRpl	= PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		Ret = pMsgRpl->m_Err;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}

	return( Ret );
}

int
PFSPost( struct Session *pSession, char *pQueue, char *pBuff, size_t Len )
{
	PFSPostMsgReq_t	ReqPost;
	PFSPostMsgRpl_t	*pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	int	Ret = 0;

	if( Len > DATA_SIZE_POST ) {
		return( PFSPostOutOfBand( pSession, pQueue, pBuff, Len ) );
	} else {
		pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

		MSGINIT( &ReqPost, PFS_POST_MSG, 0, sizeof(ReqPost) + Len );
		strncpy( ReqPost.p_Name, pQueue, sizeof(ReqPost.p_Name) );
		ReqPost.p_Len			= Len;

		MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, 
						&ReqPost, sizeof(ReqPost), sizeof(ReqPost) );
		MsgAdd( pMsgReq, &Vec );

		MsgVecSet( &Vec, 0, pBuff, Len, Len );
		MsgAdd( pMsgReq, &Vec );

		ReqPost.p_Head.h_Head.h_Cksum64 = 0;
		ReqPost.p_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

		pMsgRpl	= PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
		MsgDestroy( pMsgReq );
		if( pMsgRpl ) {
			pRpl	= (PFSPostMsgRpl_t*)MsgFirst( pMsgRpl );

			errno = pRpl->p_Head.h_Error;
			MsgDestroy( pMsgRpl );
			if( errno )	Ret = -1;
		} else {
			Ret = -1;
		}
		return( Ret );
	}
}

Msg_t*
PFSPeekByMsg( struct Session *pSession, char *pQueue )
{
	PFSPeekMsgReq_t		ReqPeek;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &ReqPeek, PFS_PEEK_MSG, 0, sizeof(ReqPeek) );
	strncpy( ReqPeek.p_Name, pQueue, sizeof(ReqPeek.p_Name) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, 
						&ReqPeek, sizeof(ReqPeek), sizeof(ReqPeek) );
	MsgAdd( pMsgReq, &Vec );

	ReqPeek.p_Head.h_Head.h_Cksum64 = 0;
	ReqPeek.p_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl	= PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );

	MsgDestroy( pMsgReq );

	return( pMsgRpl );
}

Msg_t*
PFSPeekAckByMsg( struct Session *pSession, char *pQueue )
{
	PFSPeekAckReq_t		ReqAck;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MSGINIT( &ReqAck, PFS_PEEK_ACK, 0, sizeof(ReqAck) );
	strncpy( ReqAck.a_Name, pQueue, sizeof(ReqAck.a_Name) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, 
						&ReqAck, sizeof(ReqAck), sizeof(ReqAck) );
	MsgAdd( pMsgReq, &Vec );

	ReqAck.a_Head.h_Head.h_Cksum64 = 0;
	ReqAck.a_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl	= PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );

	MsgDestroy( pMsgReq );

	return( pMsgRpl );
}

int
PFSPeek( struct Session *pSession, char *pQueue, char *pBuff, size_t Len )
{
	PFSPeekMsgRpl_t	*pRpl;
	Msg_t	*pMsgRpl;
	int		Ret = 0;

	pMsgRpl = PFSPeekByMsg( pSession, pQueue );
	if( !pMsgRpl )	goto err;

	pRpl	= (PFSPeekMsgRpl_t*)MsgFirst( pMsgRpl );
	if( pRpl->p_Head.h_Error ) {
		errno	= pRpl->p_Head.h_Error;
		goto err;
	} else {
		MsgTrim( pMsgRpl, sizeof(PFSPeekMsgRpl_t) );
		Ret = MsgCopyToBuf( pMsgRpl, pBuff, Len );
	}
	MsgDestroy( pMsgRpl );
	return( Ret );
err:
	return( -1 );
}

int
PFSPeekAck( struct Session *pSession, char *pQueue )
{
	PFSPeekAckRpl_t	*pRplAck;
	Msg_t	*pMsgRpl;

	pMsgRpl	= PFSPeekAckByMsg( pSession, pQueue );
	pRplAck	= (PFSPeekAckRpl_t*)MsgFirst( pMsgRpl );
	if( pRplAck->a_Head.h_Error ) {
		errno	= pRplAck->a_Head.h_Error;
		goto err;
	}
	MsgDestroy( pMsgRpl );
	return( 0 );
err:
	return( -1 );
}

int
PFSEventQueueSet( struct Session* pSession, char* pName, struct Session *pE )
{
	PFSEventQueueReq_t	Req;
	PFSEventQueueRpl_t*	pRpl = NULL;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	PaxosClientId_t	*pId;

	pId = PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_QUEUE, 0, sizeof(Req) );
	strncpy( Req.eq_Name, pName, sizeof(Req.eq_Name) );
	Req.eq_Id	= *pId;
	Req.eq_Set	= TRUE;

	pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.eq_Head.h_Head.h_Cksum64 = 0;
	Req.eq_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PFSEventQueueRpl_t*)MsgFirst( pMsgRpl );
		errno = pRpl->eq_Head.h_Error;
		MsgDestroy( pMsgRpl );
		if( errno )	Ret = -1;
	} else {
		Ret = -1;
	}
err:
	return( Ret );	
}

int
PFSEventQueueCancel( struct Session* pSession, char* pName, struct Session *pE )
{
	PFSEventQueueReq_t	Req;
	PFSEventQueueRpl_t*	pRpl = NULL;
	int	Ret = 0;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	PaxosClientId_t	*pId;

	pId = PaxosSessionGetClientId( pE );
	if( !pId ) {
		errno = EINVAL;
		Ret = -1;
		goto err;
	}
	MSGINIT( &Req, PFS_EVENT_QUEUE, 0, sizeof(Req) );
	strncpy( Req.eq_Name, pName, sizeof(Req.eq_Name) );
	Req.eq_Id	= *pId;
	Req.eq_Set	= FALSE;

	pMsgReq	= MsgCreate( 5, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.eq_Head.h_Head.h_Cksum64 = 0;
	Req.eq_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PFSEventQueueRpl_t*)MsgFirst( pMsgRpl );
		errno = pRpl->eq_Head.h_Error;
		MsgDestroy( pMsgRpl );
		if( errno )	Ret = -1;
	} else {
		Ret = -1;
	}
err:
	return( Ret );	
}

int
PFSDlgInit( struct Session *pSession, struct Session *pEvent,
		PFSDlg_t *pD, char *pName,
		int (*fInit)(PFSDlg_t*,void*), int (*fTerm)(PFSDlg_t*,void*), 
		void *pTag )
{
	LOG( pLOG, LogINF, "[%s]", pName );

	list_init( &pD->d_Lnk );
	strncpy( pD->d_Name, pName, sizeof(pD->d_Name) );

	pD->d_Own	= 0;
	pD->d_Ver	= 0;
	pD->d_fInit	= fInit;
	pD->d_fTerm	= fTerm;
	pD->d_pTag	= pTag;

	HashInit( &pD->d_Hold, PRIMARY_5, HASH_CODE_U64, HASH_CMP_U64,
				NULL, Malloc, Free, NULL );
	CndInit( &pD->d_Cnd );

	pD->d_pS	= pSession;
	pD->d_pE	= pEvent;
	MtxInit( &pD->d_Mtx );
	return( 0 );
}

int
PFSDlgInitByPNT( struct Session *pSession, struct Session *pEvent,
		PFSDlg_t *pD, char *pName,
		int (*fInit)(PFSDlg_t*,void*), int (*fTerm)(PFSDlg_t*,void*), 
		void *pTag )
{
	LOG( pLOG, LogINF, "[%s]", pName );

	list_init( &pD->d_Lnk );
	strncpy( pD->d_Name, pName, sizeof(pD->d_Name) );

	pD->d_Own	= 0;
	pD->d_Ver	= 0;
	pD->d_fInit	= fInit;
	pD->d_fTerm	= fTerm;
	pD->d_pTag	= pTag;

	HashInit( &pD->d_Hold, PRIMARY_5, HASH_CODE_PNT, HASH_CMP_PNT,
				NULL, Malloc, Free, NULL );
	CndInit( &pD->d_Cnd );

	pD->d_pS	= pSession;
	pD->d_pE	= pEvent;
	MtxInit( &pD->d_Mtx );
	return( 0 );
}

int
PFSDlgDestroy( PFSDlg_t *pD )
{
	struct Session *pSession = pD->d_pS;

	LOG( pLOG, LogINF, "[%s]", pD->d_Name );

	HashDestroy( &pD->d_Hold );
	CndDestroy( &pD->d_Cnd );
	MtxDestroy( &pD->d_Mtx );
	return( 0 );
}

int
_PFSDlgReturn(  struct Session *pSession, PFSDlg_t *pD,
						 void *pArg, void *pHolder )
{
	int	Ret;
	pthread_t	Th = pthread_self();
	int	Cnt;

	if( pHolder )	Th = (uintptr_t)pHolder;
	LOG( pLOG, LogINF, "pD=[%s] Own=%d Ver=%d Cnt=%d, Th=%p", 
		pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold), Th );

	while( (Cnt = HashCount( &pD->d_Hold )) ) {

		if( Cnt == 1 && HashGet( &pD->d_Hold, (void*)Th ) ) {// Only Myself
			break;
		}
		pD->d_Wait++;
		CndWait( &pD->d_Cnd, &pD->d_Mtx );
		pD->d_Wait--;
	}
	if( pD->d_Own ) {
		if( pD->d_fTerm ) {
			Ret = (*pD->d_fTerm)( pD, pArg );
			if( Ret )	goto err;
		}
		Ret = PFSUnlockSuper( pSession, pD->d_pS, pD->d_Name );
		if( Ret < 0 )	goto err;

		pD->d_Own	= 0;
		pD->d_Ver	= 0;
	}

	LOG( pLOG, LogINF, "OK" );
	return( 0 );
err:
	LOG( pLOG, LogERR, "NG errno=%d", errno );
	return( -1 );
}

int
PFSDlgReturn(  PFSDlg_t *pD, void *pArg )
{
	int Ret;

	MtxLock( &pD->d_Mtx );

	Ret	= _PFSDlgReturn( pD->d_pS, pD, pArg, NULL );

	MtxUnlock( &pD->d_Mtx );

	return( Ret );
}

int
_PFSDlgRecall(  struct Session *pSession, PFSDlg_t *pD, void *pArg )
{
	int	Ret;
	int	Cnt;

	LOG( pLOG, LogINF, "pD=[%s] Own=%d Ver=%d Cnt=%d", 
		pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold) );

	while( (Cnt = HashCount( &pD->d_Hold )) ) {

		pD->d_WaitRecall++;
		CndWait( &pD->d_Cnd, &pD->d_Mtx );
		pD->d_WaitRecall--;
	}
	if( pD->d_Own ) {
		if( pD->d_fTerm ) {
			Ret = (*pD->d_fTerm)( pD, pArg );
			if( Ret )	goto err;
		}
		Ret = PFSUnlockSuper( pSession, pD->d_pS, pD->d_Name );
		if( Ret < 0 )	goto err;

		pD->d_Own	= 0;
		pD->d_Ver	= 0;
	}
	if( pD->d_WaitRecall == 0 )	CndBroadcast( &pD->d_Cnd );

	LOG( pLOG, LogINF, "OK" );
	return( 0 );
err:
	LOG( pLOG, LogERR, "NG errno=%d", errno );
	return( -1 );
}

int
PFSDlgRecall( PFSEventLock_t *pEv, PFSDlg_t *pD, void *pArg )
{
	struct Session *pSession = pD->d_pS;
	int	Ret = -1;

	LOG( pLOG, LogINF, "pD=[%s] d_Ver=%d l_Ver=%d", 
			pD->d_Name, pD->d_Ver, pEv->el_Ver );

	MtxLock( &pD->d_Mtx );

	LOG( pLOG, LogDBG, "AGAIN:pD=[%s] d_Ver=%d l_Ver=%d", 
			pD->d_Name, pD->d_Ver, pEv->el_Ver );

	if( pD->d_Own && SEQ_CMP( pD->d_Ver, pEv->el_Ver ) == 0 ) {
		Ret = _PFSDlgRecall( pD->d_pE, pD, pArg );
	}
//	pD->d_Ver	= pEv->el_Ver;

	MtxUnlock( &pD->d_Mtx );
	return( Ret );
}

int
_PFSDlgHoldW( PFSDlg_t *pD, void *pArg, bool_t bTry, void *pHolder )
{
	struct Session *pSession = pD->d_pS;
	int	Ver, VerOld;
	int	r;
	int	Ret;
	pthread_t	Th = pthread_self();
	int	Cnt;
	HashItem_t	*pItem;

	if( pHolder )	Th = (uintptr_t)pHolder;
	MtxLock( &pD->d_Mtx );

	Ret = PaxosSessionGetErr( pD->d_pS );	// Active?
	pD->d_Err	= Ret;
	if( Ret ) {
		if( pD->d_Own && pD->d_fTerm ) {
			(*pD->d_fTerm)( pD, pArg );
		}
		pD->d_Own	= 0;
		pD->d_Ver	= 0;
		goto err;
	}

//	LOG( pLOG, LogINF, "pD=[%s] Own=%d Ver=%d Cnt=%d, Th=%p", 
//		pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold), Th );

	Ret		= pD->d_Own;
	VerOld	= pD->d_Ver;

again:
	while( pD->d_WaitRecall ) {
		CndWait( &pD->d_Cnd, &pD->d_Mtx );
	}
	switch( pD->d_Own ) {
	default:	abort();

	case PFS_DLG_W:

		if( (Cnt = HashCount( &pD->d_Hold )) > 0 
				&& !HashGet( &pD->d_Hold, (void*)Th ) ) {

			pItem=HashIsExist(&pD->d_Hold);

			LOG( pLOG, LogINF, 
			"PFS_DLG_W([%s]Cnt=%d,pKey=%p,pValue=%u) is not mine/WAIT(Th=%p)", 
			pD->d_Name, Cnt,
			pItem->i_pKey, PNT_UINT32(pItem->i_pValue), Th );

			pD->d_Wait++;
			CndWait( &pD->d_Cnd, &pD->d_Mtx );
			pD->d_Wait--;

			goto again;
		}
		pD->d_OwnOld	= pD->d_Own;
		break;

	case PFS_DLG_R:

		if( (Cnt = HashCount( &pD->d_Hold ) ) > 1 
			|| (Cnt == 1 && !HashGet(&pD->d_Hold,(void*)Th)) ) {

			pItem=HashIsExist(&pD->d_Hold);
			LOG( pLOG, LogINF, 
			"PFS_DLG_R([%s]Cnt=%d,pKey=%p,pValue=%u) is not mine/WAIT", 
			pD->d_Name, Cnt, 
			pItem->i_pKey, PNT_UINT32(pItem->i_pValue) );

			pD->d_Wait++;
			CndWait( &pD->d_Cnd, &pD->d_Mtx );
			pD->d_Wait--;
			goto again;
		}
		LOG( pLOG, LogINF, "[%s]:R->W(fTerm-PFSUnlock-_PFSLockW-fInit)", 
			pD->d_Name );

		if( pD->d_fTerm ) {
			Ret = (*pD->d_fTerm)( pD, pArg );
			if( Ret < 0 )	goto err;
		}
		r	= PFSUnlock( pSession, pD->d_Name );

//		LOG( pLOG, "[%s]:R->W PFSUnlock added", pD->d_Name );
//		r	= PFSUnlock( pSession, pD->d_Name );
//		if( r )	goto err;

		r	= _PFSLockW( pSession, pD->d_Name, &Ver, bTry );
		if( r ) goto err;

	
		if( SEQ_CMP( Ver, VerOld ) <= 0 ) {
			r	= PFSUnlock( pSession, pD->d_Name );
			pD->d_Own	= 0;
			goto err;
		}
		pD->d_OwnOld	= pD->d_Own;
		pD->d_Own	= PFS_DLG_W;
		pD->d_Ver	= Ver;
		if( pD->d_fInit && SEQ_CMP(Ver, VerOld) > 0 ) {
			Ret = (*pD->d_fInit)( pD, pArg );
			if( Ret < 0 ) {
				r	= PFSUnlock( pSession, pD->d_Name );
				pD->d_Own	= 0;
				goto err;
			}
		}
		break;
	case 0:
		LOG( pLOG, LogINF, "[%s]:0->W _PFSLockW", pD->d_Name );

		r	= _PFSLockW( pSession, pD->d_Name, &Ver, bTry );
		if( r )	goto err;
		if( SEQ_CMP( Ver, VerOld ) <= 0 ) {
			r	= PFSUnlock( pSession, pD->d_Name );
			goto err;
		}
		pD->d_OwnOld	= pD->d_Own;
		pD->d_Own	= PFS_DLG_W;
		pD->d_Ver	= Ver;
		if( pD->d_fInit ) {
			Ret = (*pD->d_fInit)( pD, pArg );
			if( Ret < 0 ) {
				r	= PFSUnlock( pSession, pD->d_Name );
				pD->d_Own	= 0;
				goto err;
			}
		}

		break;
	}
	HashPut( &pD->d_Hold, (void*)Th, (void*)Th ); // recursive OK
	MtxUnlock( &pD->d_Mtx );
	return( 0 );
err:
	MtxUnlock( &pD->d_Mtx );
	return( -1 );
}

int
PFSDlgHoldW( PFSDlg_t *pD, void *pArg )
{
	int	Ret;

	Ret = _PFSDlgHoldW( pD, pArg, FALSE, NULL );

	return( Ret );
}

int
PFSDlgTryHoldW( PFSDlg_t *pD, void *pArg )
{
	int	Ret;

	Ret = _PFSDlgHoldW( pD, pArg, TRUE, NULL );

	return( Ret );
}

int
_PFSDlgHoldR( PFSDlg_t *pD, void* pArg, bool_t bTry, void *pHolder )
{
struct Session *pSession = pD->d_pS;
	int	Ver, VerOld;
	int	r;
	int	Ret;
	pthread_t	Th = pthread_self();
	if( pHolder )	Th = (uintptr_t)pHolder;

	MtxLock( &pD->d_Mtx );

	Ret = PaxosSessionGetErr( pD->d_pS );	// Active?
	pD->d_Err	= Ret;
	if( Ret ) {
		if( pD->d_Own && pD->d_fTerm ) {
			(*pD->d_fTerm)( pD, pArg );
		}
		pD->d_Own	= 0;
		pD->d_Ver	= 0;
		goto err;
	}
//	LOG( pLOG, "pD=[%s] Own=%d Ver=%d Cnt=%d", 
//		pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold) );

	Ret		= pD->d_Own;
	VerOld	= pD->d_Ver;

again:
	while( pD->d_WaitRecall ) {
		CndWait( &pD->d_Cnd, &pD->d_Mtx );
	}
	switch( pD->d_Own ) {
	default: abort();

	case PFS_DLG_R:
		pD->d_OwnOld	= pD->d_Own;
		break;

	case PFS_DLG_W:

		if( !HashGet( &pD->d_Hold, (void*)Th ) ) {
			if( HashCount( &pD->d_Hold ) == 1 ) {
				LOG( pLOG, LogINF, "PFS_DLG_W not mine/Wait TH=%p", (void*)Th );
				pD->d_Wait++;
				CndWait( &pD->d_Cnd, &pD->d_Mtx );
				pD->d_Wait--;
				goto again;
			}
		}
		pD->d_OwnOld	= pD->d_Own;
		LOG( pLOG, LogINF, "[%s]:W->W", pD->d_Name );
		break;
	case 0:
		LOG( pLOG, LogINF, "[%s]:0->R _PFSLockR", pD->d_Name );

		r	= _PFSLockR( pSession, pD->d_Name, &Ver, bTry );
		if( r )	goto err;

		if( SEQ_CMP( Ver, VerOld ) <= 0 ) {
			r	= PFSUnlock( pSession, pD->d_Name );
			goto err;
		}
		pD->d_OwnOld	= pD->d_Own;
		pD->d_Own	= PFS_DLG_R;
		pD->d_Ver	= Ver;
		if( pD->d_fInit ) {
			Ret = (*pD->d_fInit)( pD, pArg );
			if( Ret < 0 ) {
				r	= PFSUnlock( pSession, pD->d_Name );
				pD->d_Own	= 0;
				goto err;
			}
		}
		break;
	}
	HashPut( &pD->d_Hold, (void*)Th, (void*)Th ); // recursive  OK
	MtxUnlock( &pD->d_Mtx );
	return( 0 );
err:
	MtxUnlock( &pD->d_Mtx );
	return( -1 );
}

int
PFSDlgHoldR( PFSDlg_t *pD, void *pArg )
{
	int	Ret;

	Ret = _PFSDlgHoldR( pD, pArg, FALSE, NULL );

	return( Ret );
}

int
PFSDlgTryHoldR( PFSDlg_t *pD, void *pArg )
{
	int	Ret;

	Ret = _PFSDlgHoldR( pD, pArg, TRUE, NULL );

	return( Ret );
}

int
_PFSDlgRelease( PFSDlg_t *pD, void *pHolder )
{
	pthread_t	Th = pthread_self();
	struct Session *pSession = pD->d_pS;
	bool_t	bSched;

	if( pHolder )	Th = (uintptr_t)pHolder;
	MtxLock( &pD->d_Mtx );

//	LOG( pLOG, LogINF, "pD=[%s] Own=%d Ver=%d Cnt=%d Th=%p", 
//		pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold), Th );

	if( !HashRemove( &pD->d_Hold, (void*)Th ) )	goto err;

	if( pD->d_WaitRecall > 0 ) {
		bSched	= TRUE;
		CndBroadcast( &pD->d_Cnd );
	} else if( pD->d_Wait > 0 ) {
		bSched	= TRUE;
		CndBroadcast( &pD->d_Cnd );
	} else {
		bSched	= FALSE;
	}

	MtxUnlock( &pD->d_Mtx );
	if( bSched )	sched_yield();
	return( 0 );
err:
	LOG( pLOG, LogERR, "pD=[%s] Own=%d Ver=%d Cnt=%d Th=%p", 
	pD->d_Name, pD->d_Own, pD->d_Ver, HashCount(&pD->d_Hold), Th );

	MtxUnlock( &pD->d_Mtx );
	return( -1 );
}

int
PFSDlgRelease( PFSDlg_t *pD )
{
	int	Ret;

	Ret = _PFSDlgRelease( pD, NULL );

	return( Ret );
}

int
PFSDlgHoldWBy( PFSDlg_t *pD, void *pArg, void *pHolder )
{
	int	Ret;

	Ret = _PFSDlgHoldW( pD, pArg, FALSE, pHolder );

	return( Ret );
}

int
PFSDlgHoldRBy( PFSDlg_t *pD, void *pArg, void *pHolder )
{
	int	Ret;

	Ret = _PFSDlgHoldR( pD, pArg, FALSE, pHolder );

	return( Ret );
}

int
PFSDlgReleaseBy( PFSDlg_t *pD, void *pHolder )
{
	int	Ret;

	Ret = _PFSDlgRelease( pD, pHolder );

	return( Ret );
}

int
PFSDlgReturnBy(  PFSDlg_t *pD, void *pArg, void *pHolder )
{
	int Ret;

	MtxLock( &pD->d_Mtx );

	Ret	= _PFSDlgReturn( pD->d_pS, pD, pArg, pHolder );

	MtxUnlock( &pD->d_Mtx );

	return( Ret );
}

int
PFSCopy( struct Session* pSession, char *pFrom, char *pTo )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	PFSCopyReq_t	Req;
	PFSCopyRpl_t	*pRpl;
	int	Ret = 0;

	memset( &Req, 0, sizeof(Req) );

	MSGINIT( &Req, PFS_COPY, 0, sizeof(Req) );
	strncpy( Req.c_From, pFrom, sizeof(Req.c_From) );
	strncpy( Req.c_To, pTo, sizeof(Req.c_To) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.c_Head.h_Head.h_Cksum64 = 0;
	Req.c_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSCopyRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->c_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	return( Ret );
}

int
PFSConcat( struct Session* pSession, char *pFrom, char *pTo )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	PFSConcatReq_t	Req;
	PFSConcatRpl_t	*pRpl;
	int	Ret = 0;

	memset( &Req, 0, sizeof(Req) );

	MSGINIT( &Req, PFS_CONCAT, 0, sizeof(Req) );
	strncpy( Req.c_From, pFrom, sizeof(Req.c_From) );
	strncpy( Req.c_To, pTo, sizeof(Req.c_To) );

	pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
							PaxosSessionGetfFree(pSession) );

	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	Req.c_Head.h_Head.h_Cksum64 = 0;
	Req.c_Head.h_Head.h_Cksum64 = MsgCksum64( pMsgReq, 0, 0, NULL );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );

	if( pMsgRpl ) {
		pRpl	= (PFSConcatRpl_t*)MsgFirst( pMsgRpl );
		errno	= pRpl->c_Head.h_Error;
		if( errno )	Ret = -1;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	return( Ret );
}
