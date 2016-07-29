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

#include	"VV.h"

Msg_t*
LVCompoundReadByMsg( struct Session *pSession, LS_t *pLS, list_t *pEnt )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	LV_IO_t		*pIO;
	LV_read_req_t	*pReq;
	int	Len = 0;
	PaxosSessionHead_t	Head;
	int	Flag;

	pMsgReq	= MsgCreate( 20, Malloc, Free );

	SESSION_MSGINIT( &Head, LV_COMPOUND_READ, 0, 0, sizeof(Head) );
	MsgVecSet( &Vec, VEC_HEAD, &Head, sizeof(Head), sizeof(Head) );
	MsgAdd( pMsgReq, &Vec );

	Flag = VEC_HEAD|VEC_MINE|VEC_REPLY;

	LOG( PaxosSessionGetLog(pSession), LogDBG, "### READ [%s][%s:%u] ###", 
		pLS->ls_Cell, pLS->ls_LV, pLS->ls_LS );

	list_for_each_entry( pIO, pEnt, io_Lnk ) {

		pReq	= (LV_read_req_t*)Malloc( sizeof(*pReq) );
		memset( pReq, 0, sizeof(*pReq) );

		SESSION_MSGINIT( &pReq->r_Head, LV_READ, 0, 0, sizeof(*pReq) );
		pReq->r_LS	= *pLS;
		pReq->r_Off	= pIO->io_Off + (off_t)pLS->ls_LS*VV_LS_SIZE;
		pReq->r_Len	= pIO->io_Len;

		LOG( PaxosSessionGetLog(pSession), LogDBG,
			"Off=%"PRIu64" Len=%"PRIu64" buf=%p",
			pIO->io_Off, pIO->io_Len, pIO->io_pBuf );

		MsgVecSet( &Vec, Flag, pReq, sizeof(*pReq), sizeof(*pReq) );
		MsgAdd( pMsgReq, &Vec );

		Flag	&= ~VEC_REPLY;	// only the first

		Len	+= sizeof(*pReq);
	}
	Head.h_Len	+= Len;

	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
	MsgDestroy( pMsgReq );
	if( !pMsgRpl )	goto err1;

	return( pMsgRpl );
err1:
	return( NULL );
}

int
LVCompoundRead( struct Session *pSession, LS_t *pLS, list_t *pEnt )
{
	Msg_t	*pMsgRpl;
	LV_read_rpl_t	*pRpl;
	LV_IO_t		*pIO;

	pMsgRpl = LVCompoundReadByMsg( pSession, pLS, pEnt );
	if( !pMsgRpl )	goto err;

	pRpl	= (LV_read_rpl_t*)MsgFirst( pMsgRpl );
	if( !pRpl ) {
		errno	= EIO;
		goto err1;
	}
	errno	= pRpl->r_Head.h_Error;
	if( errno )	goto err1;

	while( (pIO = list_first_entry( pEnt, LV_IO_t, io_Lnk ))) {

		list_del( &pIO->io_Lnk );

ASSERT( pIO->io_Len <= pRpl->r_Head.h_Len - sizeof(LV_read_rpl_t) );
		ASSERT( pIO->io_Len == pRpl->r_Len );

		memcpy( pIO->io_pBuf, (char*)(pRpl+1), pRpl->r_Len );  

		pRpl	= (LV_read_rpl_t*)
					((char*)pRpl + LONG_BYTE(sizeof(*pRpl)+pRpl->r_Len));

#ifdef	ZZZ
		Free( pIO );
#endif
	}
	MsgDestroy( pMsgRpl );
	return( 0 );
err1:
	MsgDestroy( pMsgRpl );
err:
	return( -1 );
}

int
LVCompoundWrite( struct Session *pSession, LS_t *pLS, list_t *pEnt )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	LV_IO_t	*pIO;
	LV_write_req_t	*pReq;
	LV_write_rpl_t	*pRpl;
	char	*pR;
	int		Len, len, r;
	long	padd = 0;
	PaxosSessionHead_t	Head;
	PaxosSessionHead_t	Push, Out;
	int	Flag;

	pMsgReq	= MsgCreate( 20, Malloc, Free );

	SESSION_MSGINIT( &Head, LV_COMPOUND_WRITE, 0, 0, sizeof(Head) );
	MsgVecSet( &Vec, VEC_HEAD, &Head, sizeof(Head), sizeof(Head) );
	MsgAdd( pMsgReq, &Vec );

	Len = 0;
	Flag = VEC_HEAD|VEC_MINE|VEC_REPLY;

	LOG( PaxosSessionGetLog(pSession), LogINF, "### WRITE [%s][%s:%u] ###", 
		pLS->ls_Cell, pLS->ls_LV, pLS->ls_LS );

	list_for_each_entry( pIO, pEnt, io_Lnk ) {

		pReq	= (LV_write_req_t*)Malloc( sizeof(*pReq) );
		memset( pReq, 0, sizeof(*pReq) );

		len	= LONG_BYTE( sizeof(*pReq) + pIO->io_Len );

		SESSION_MSGINIT( &pReq->w_Head, LV_WRITE, 0, 0, len );
		pReq->w_LS	= *pLS;
		pReq->w_Off	= pIO->io_Off + (off_t)pLS->ls_LS*VV_LS_SIZE;
		pReq->w_Len	= pIO->io_Len;

		LOG( PaxosSessionGetLog(pSession), LogDBG,
			"Off=%"PRIu64" Len=%"PRIu64" buf=%p Flag=0x%x",
			pIO->io_Off, pIO->io_Len, pIO->io_pBuf, pIO->io_Flag );

		MsgVecSet( &Vec, Flag, pReq, sizeof(*pReq), sizeof(*pReq) );
		MsgAdd( pMsgReq, &Vec );

		Flag	&= ~VEC_REPLY;	// only the first

		MsgVecSet( &Vec, 0, pIO->io_pBuf, pIO->io_Len, pIO->io_Len );
		MsgAdd( pMsgReq, &Vec );

		/* Boundary alignement */
		if( (r = pIO->io_Len % sizeof(long)) > 0 ) {
			MsgVecSet( &Vec, 0, &padd, sizeof(long) - r, sizeof(long) - r );
			MsgAdd( pMsgReq, &Vec );
		}
		Len	+= len;
	}
	Head.h_Len	+= Len;
/*
 *	OutOfBand
 *		PUSH + ( LV_COMPOUND_WRITE + (LV_WRITE + Body ) + ... ) + LV_OUTOFBAND
 */
	if( Head.h_Len > PAXOS_COMMAND_SIZE ) {

		SESSION_MSGINIT( &Push, PAXOS_SESSION_OB_PUSH, 0, 0, 
								sizeof(Push) + Head.h_Len );
		MsgVecSet( &Vec, VEC_HEAD, &Push, sizeof(Push), sizeof(Push) );
		MsgInsert( pMsgReq, &Vec );

		SESSION_MSGINIT( &Out, LV_OUTOFBAND, 0, 0, sizeof(Out) );
		MsgVecSet( &Vec, VEC_HEAD, &Out, sizeof(Out), sizeof(Out) );
		MsgAdd( pMsgReq, &Vec );
	}
#ifdef	ZZZ
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
#else
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
#endif
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (LV_write_rpl_t*)MsgFirst( pMsgRpl );
	}
	if( !pMsgRpl || !pRpl ) {
		errno = EIO;
		goto err1;
	}
	errno	= pRpl->w_Head.h_Error;
	if( errno )	goto err2;

	pR	= (char*)pRpl;
	while( (pIO = list_first_entry( pEnt, LV_IO_t, io_Lnk ))) {

		list_del( &pIO->io_Lnk );
		pR	+= sizeof(LV_write_rpl_t);
	}
	MsgDestroy( pMsgRpl );

	return( 0 );
err2:
	MsgDestroy( pMsgRpl );
err1:
	return( -1 );
}

int
LVCompoundCopy( struct Session *pFr, LS_t *pLSFr, list_t *pEnt,
						struct Session *pTo, LS_t *pLSTo  )
{
	Msg_t	*pMsgRpl;
	LV_read_rpl_t	*pRpl;
	LV_IO_t		*pIO;
	int	Ret;

	/* read */
	pMsgRpl = LVCompoundReadByMsg( pFr, pLSFr, pEnt );
	if( !pMsgRpl )	goto err;

	pRpl	= (LV_read_rpl_t*)MsgFirst( pMsgRpl );
	if( !pRpl ) {
		errno	= EIO;
		goto err1;
	}
	errno	= pRpl->r_Head.h_Error;
	if( errno )	goto err1;

	list_for_each_entry( pIO, pEnt, io_Lnk ) {

ASSERT( pIO->io_Len <= pRpl->r_Head.h_Len - sizeof(LV_read_rpl_t) );
		ASSERT( pIO->io_Len == pRpl->r_Len );

//		memcpy( pIO->io_pBuf, (char*)(pRpl+1), pRpl->r_Len );  
		pIO->io_pBuf	= (char*)(pRpl+1);

		pRpl	= (LV_read_rpl_t*)
					((char*)pRpl + LONG_BYTE(sizeof(*pRpl)+pRpl->r_Len));

	}
	/* write */
	Ret = LVCompoundWrite( pTo, pLSTo, pEnt );
	if( Ret < 0 )	goto err1;

	MsgDestroy( pMsgRpl );
	return( 0 );
err1:
	MsgDestroy( pMsgRpl );
err:
	return( -1 );
}

int
LVFlushAll( struct Session *pSession )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	LV_cacheout_req_t	Req;
	LV_cacheout_rpl_t	*pRpl;

	pMsgReq	= MsgCreate( 3, Malloc, Free );

	SESSION_MSGINIT( &Req.c_Head, LV_CACHEOUT, 0, 0, sizeof(Req) );
	
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

#ifdef	ZZZ
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
#else
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
#endif
	MsgDestroy( pMsgReq );
	pRpl	= (LV_cacheout_rpl_t*)MsgFirst( pMsgRpl );
	if( !pRpl ) {
		errno = EIO;
		goto err1;
	}
	errno	= pRpl->c_Head.h_Error;
	if( errno )	goto err2;

	MsgDestroy( pMsgRpl );

	return( 0 );
err2:
	MsgDestroy( pMsgRpl );
err1:
	return( -1 );
}

int
LVDelete( struct Session *pSession, LS_t *pLS )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	LV_delete_req_t	Req;
	LV_delete_rpl_t	*pRpl;

	pMsgReq	= MsgCreate( 3, Malloc, Free );

	memset( &Req, 0, sizeof(Req) );
	SESSION_MSGINIT( &Req.d_Head, LV_DELETE, 0, 0, sizeof(Req) );
	Req.d_LS = *pLS;
	
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

#ifdef	ZZZ
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, FALSE, FALSE );
#else
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
#endif
	MsgDestroy( pMsgReq );
	pRpl	= (LV_delete_rpl_t*)MsgFirst( pMsgRpl );
	if( !pRpl ) {
		errno = EIO;
		goto err1;
	}
	errno	= pRpl->d_Head.h_Error;
	if( errno )	goto err2;

	MsgDestroy( pMsgRpl );

	return( 0 );
err2:
	MsgDestroy( pMsgRpl );
err1:
	return( -1 );
}
