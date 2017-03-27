/******************************************************************************
*
*  p_user_cell.c 	--- 
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
//#define	_LOCK_

#include	"xjPingPaxos.h"
#include	<netinet/tcp.h>

p_id_t*
p_open( p_port_t* pLaddr )
{
	p_id_t	*pd;

	pd = (p_id_t*)neo_malloc( sizeof(p_id_t) );
	memset( pd, 0, sizeof(*pd) );
	bcopy( pLaddr, &pd->i_local, sizeof(p_port_t) );
	pd->i_local.p_addr.a_pd	= pd;
	pd->i_local.p_cur_time	= pd->i_local.p_min_time;

	pd->i_spacket.s_min_bytes = pd->i_spacket.s_min_time = SOCK_BUF_MAX;
	pd->i_rpacket.s_min_bytes = pd->i_rpacket.s_min_time = SOCK_BUF_MAX;

	_dl_init( &pd->i_lnk );
	_dl_insert( &pd->i_lnk, &_neo_port );

	return( pd );
}

int
p_close( p_id_t *pd )
{
	_dl_remove( &pd->i_lnk );

	neo_free( pd );
	return( 0 );
}

int
p_peek( p_id_t* pd, p_head_t *pHead, int Len, int flag )
{
	p_Tag_t	*pT = (p_Tag_t*)pd->i_pTag;
	Msg_t	*pMsg;
	int		Ret = 0;
	int		n;

	if( pT ) {
		Ret = -1;
		pMsg	= list_first_entry( &pT->t_Msgs, Msg_t, m_Lnk );
		if (pMsg != NULL) {
			n = MsgCopyToBuf( pMsg, (char*)pHead, Len );
			if( n == Len )	Ret = 0;
		}
	} else {
		Ret = PeekStream( pd->i_rfd, (char*)pHead, Len );
	}
	return( Ret );
}

Msg_t*
p_recvByMsg( p_id_t* pd )
{
	p_Tag_t	*pT = (p_Tag_t*)pd->i_pTag;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	int		Ret;
	p_head_t	head;
	char	*pBuf;
	int		Len;

	if( pT ) {
		pMsg	= list_first_entry( &pT->t_Msgs, Msg_t, m_Lnk );
		list_del_init( &pMsg->m_Lnk );
	} else {
		PeekStream( pd->i_rfd, (char*)&head, sizeof(head) );
		Len		= head.h_len + sizeof(head);
		pBuf	=  neo_malloc( Len );
		Ret = RecvStream( pd->i_rfd, pBuf, Len );
		if( Ret < 0 ) {
			neo_free( pBuf );
			return( NULL );
		}
		pMsg	= MsgCreate( 1, neo_malloc, neo_free );
		MsgVecSet( &Vec, VEC_MINE, pBuf, Len, Len );
		MsgAdd( pMsg, &Vec );
	}
	return( pMsg );
}

int
p_recv( p_id_t* pd, p_head_t *pHead, int *pLen )
{
	p_Tag_t	*pT = (p_Tag_t*)pd->i_pTag;
	Msg_t	*pMsg;
	int		Ret;
	int		n;

	if( pT ) {
/* コピーがある */
		pMsg	= list_first_entry( &pT->t_Msgs, Msg_t, m_Lnk );
		n = MsgCopyToBuf( pMsg, (char*)pHead, *pLen );
		list_del_init( &pMsg->m_Lnk );
		MsgDestroy( pMsg );
		*pLen	= n;
	} else {
		Ret = RecvStream( pd->i_rfd, (char*)pHead, *pLen );
		*pLen	= Ret;
	}
	return( 0 );
}

int
PingAcceptHold( p_id_t *pd )
{
	p_Tag_t		*pT = (p_Tag_t*)pd->i_pTag;

	pT->t_HoldCount	= 0;
	pT->t_bHold		= TRUE;

	PaxosAcceptHold( pT->t_pAccept );
	return( 0 );
}

int
PingAcceptRelease( p_id_t *pd )
{
	p_Tag_t		*pT = (p_Tag_t*)pd->i_pTag;

	pT->t_HoldCount	= 0;
	pT->t_bHold		= FALSE;

	PaxosAcceptRelease( pT->t_pAccept );
	return( 0 );
}
int
PingReplyByMsg( p_id_t* pd, p_head_t *pHead, int Len, bool_t bFree )
{
	struct PaxosAccept *pAccept;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	PaxosSessionHead_t	*pH;
	int			L;
	int			Ret = 0;
	p_Tag_t		*pT = (p_Tag_t*)pd->i_pTag;
	int		n;

	if( pT ) {
		pAccept	= ((p_Tag_t*)pd->i_pTag)->t_pAccept;
		pMsg	= MsgCreate( 2, neo_malloc, neo_free );
	
		if( pT->t_HoldCount == 0 ) {
			L	= sizeof(*pH) + Len;
			pH	= (PaxosSessionHead_t*)MsgMalloc( pMsg, sizeof(*pH) );
			SESSION_MSGINIT( pH, pT->t_Cmd, 0, 0, L );

			MsgVecSet( &Vec, VEC_MINE, pH, sizeof(*pH), sizeof(*pH) );
			MsgAdd( pMsg, &Vec );
		}

		if( bFree ) {MsgVecSet( &Vec, VEC_MINE, pHead, Len, Len );}
		else		{MsgVecSet( &Vec, 0, pHead, Len, Len );}
		MsgAdd( pMsg, &Vec );

		if( pT->t_bHold )	pT->t_HoldCount++;

		switch( pHead->h_cmd ) {
			case R_CMD_FIND:
			case R_CMD_LIST:
				n = PaxosAcceptReplyReadByMsg( pAccept, pMsg );	
				break;
			default:
				n = PaxosAcceptReplyByMsg( pAccept, pMsg );	
				break;
		}
		if( n >= 0 )	Ret = 0;
		else			Ret = -1;
	} else {
		Ret = SendStream( pd->i_sfd, (char*)pHead, Len );
		if( bFree )	neo_free( pHead );
	}
	return( Ret );
}

int
p_reply( p_id_t* pd, p_head_t *pHead, int Len )
{
	return( PingReplyByMsg( pd, pHead, Len, FALSE ) );
}

int
p_reply_free( p_id_t* pd, p_head_t *pHead, int Len )
{
	return( PingReplyByMsg( pd, pHead, Len, TRUE ) );
}

Msg_t*
RequestAndReplyByMsg( 
	struct Session*			pSession,
	Msg_t*					pReq,
	bool_t					Update )
{
	PaxosSessionHead_t	Push, Head, Out;
	int	Len;
	MsgVec_t	Vec;
	Msg_t		*pRpl = NULL;

	Len	= MsgTotalLen( pReq );
	Len	+= sizeof(Head);

	if( Update )	SESSION_MSGINIT( &Head, XJPING_UPDATE, 0, 0, Len );
	else			SESSION_MSGINIT( &Head, XJPING_REFER, 0, 0, Len );

	MsgVecSet(&Vec, VEC_HEAD|VEC_REPLY, &Head, sizeof(Head), sizeof(Head));
	MsgInsert( pReq, &Vec );

	if( Len > PAXOS_COMMAND_SIZE ) {
LOG( neo_log, LogDBG, "Len=%d PAXOS_COMMAND_SIZE=%d sizeof(Head)=%d",
Len, PAXOS_COMMAND_SIZE, sizeof(Head) );

		SESSION_MSGINIT( &Push, PAXOS_SESSION_OB_PUSH, 0, 0, 
							sizeof(Push) + Len );

		MsgVecSet( &Vec, VEC_HEAD, &Push, sizeof(Push), sizeof(Push) );
		MsgInsert( pReq, &Vec );

		SESSION_MSGINIT( &Out, XJPING_OUTOFBAND, 0, 0, sizeof(Out) );
		MsgVecSet(&Vec, VEC_HEAD, &Out, sizeof(Out), sizeof(Out));
		MsgAdd( pReq, &Vec );
/*	
 *	PUSH + ( XJPING_UPDATE/REFER + Body ) + XJPING_OUTOFBAND 
 */
	}

	pRpl = PaxosSessionReqAndRplByMsg( pSession, pReq, Update, FALSE );

	MsgFirst( pRpl );
	MsgTrim( pRpl, sizeof(Head) );

	return( pRpl );
}

int
p_request( p_id_t *pd, p_head_t *pH, int Len )
{
	struct Session	*pSession;
	p_Tag_t			*pTag;
	bool_t	Update = TRUE;
	Msg_t	*pReq, *pRpl, *pRpl1;
	MsgVec_t	Vec;
	int	l;
	p_head_t	*pH1;

	pTag	= (p_Tag_t*)pd->i_pTag;
	pSession	= pTag->t_pSession;

	switch( pH->h_cmd ) {
		case R_CMD_DUMP_PROC:
		case R_CMD_DUMP_INF:
		case R_CMD_DUMP_RECORD:
		case R_CMD_DUMP_TBL:
		case R_CMD_DUMP_USR:
		case R_CMD_DUMP_MEM:
		case R_CMD_DUMP_CLIENT:
			neo_errno = E_RDB_NOSUPPORT;
			return( -1 );
		case R_CMD_FIND:
		case R_CMD_LIST:
			Update = FALSE;
			break;
	}
	pReq	= MsgCreate( 1, neo_malloc, neo_free );

	l = pH->h_len + sizeof(*pH);

	ASSERT( l == Len );

	MsgVecSet( &Vec, 0, pH, l, l );
	MsgAdd( pReq, &Vec );

	pRpl	= RequestAndReplyByMsg( pSession, pReq, Update );

	/* multi reply */
	while( (pH1	= (p_head_t*)MsgFirst( pRpl ) ) ) {
		l	= pH1->h_len + sizeof(p_head_t);
		if( MsgTotalLen( pRpl ) == l )	break;

		pRpl1	= MsgClone( pRpl );
		list_add_tail( &pRpl1->m_Lnk, &pTag->t_Msgs );
		MsgTrim( pRpl, l );
	}
	list_add_tail( &pRpl->m_Lnk, &pTag->t_Msgs );

	MsgDestroy( pReq );

	return(0);
}

Msg_t*
p_responseByMsg( p_id_t *pd )
{
	p_Tag_t			*pTag;
	Msg_t			*pMsg;

	pTag	= (p_Tag_t*)pd->i_pTag;
	pMsg	= list_first_entry( &pTag->t_Msgs, Msg_t, m_Lnk );
	if( pMsg )	list_del_init( &pMsg->m_Lnk );
	return( pMsg );
}

int
p_response( p_id_t *pd, p_head_t *pH, int *pL )
{
	p_Tag_t			*pTag;
	Msg_t			*pMsg;
	int	l;

	pTag	= (p_Tag_t*)pd->i_pTag;

/* コピーがある */
	pMsg	= list_first_entry( &pTag->t_Msgs, Msg_t, m_Lnk );
	if( !pMsg ) {
		return( -1 );
	}
	l = MsgCopyToBuf( pMsg, (char*)pH, *pL );
	list_del_init( &pMsg->m_Lnk);
	MsgDestroy( pMsg );

	*pL	= l;

	return(0);
}

_dlnk_t		_neo_r_man;
static	int	_r_init;

extern char *_neo_procbname, _neo_hostname[];
extern int  _neo_pid;

r_man_t*
rdb_link( p_name_t MyName, r_man_name_t CellName )
{
	r_man_t		*md;
	p_id_t		*pd;
	r_req_link_t	req;
	r_res_link_t	res;
	p_port_t	myport;
	int		len;
	struct Session	*pSession = NULL;
	int	Ret;
static int No;
	p_Tag_t	*pTag;

	if( !_r_init ) {
		_dl_init( &_neo_r_man );
		_r_init	= 1;
	}
	memset( &myport, 0, sizeof(myport) );
	if( p_getaddrbyname( MyName, &myport ) ) {
		goto err1;
	}
	myport.p_role = P_CLIENT;
	if( !( pd = p_open( &myport ) ) ) {
		goto err1;
	}

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, neo_malloc, neo_free, CellName );
	if( !pSession ) {
		printf("Can't get address [%s].\n", CellName );
		goto err2;
	}
	PaxosSessionSetLog( pSession, neo_log );

	Ret = PaxosSessionOpen( pSession, No++, FALSE );
	if( Ret )	goto err3;

	pTag	= (p_Tag_t*)neo_malloc( sizeof(*pTag) );
	if( !pTag ) {
		neo_errno	= E_RDB_NOMEM;
		goto err3;
	}
	memset( pTag, 0, sizeof(*pTag) );
	list_init( &pTag->t_Msgs );
	pTag->t_pSession	= pSession;
	pd->i_pTag	= pTag;

	/*
	 * DB_MANサーバへ接続する(データベース接続）
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_LINK, sizeof(r_req_link_t) );

	neo_setusradm( &req.l_usr, 
		_neo_procbname, _neo_pid, MyName, _neo_hostname, PNT_INT32(pd) );

	if( p_request( pd, (p_head_t*)&req, sizeof(req) ) ) {
		goto err3;
	}
	
	/*
	 * サーバからの接続応答を待つ
	 */
	len = sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) ) {
		goto err3;
	}
	
	if( (neo_errno = res.l_head.h_err) )
		goto err3;

	/*
	 * DB_MAN記述子を生成する
	 */
	if( !(md = (r_man_t *)neo_malloc( sizeof(r_man_t) )) ) {
		neo_errno	= E_RDB_NOMEM;
		goto err4;
	}
	memset( md, 0, sizeof(*md) );

	/*
	 * 生成したDB_MAN記述子初期化して、_neo_r_manへ繋ぐ
	 */
	md->r_ident	= R_MAN_IDENT;
	strncpy( md->r_name, CellName, sizeof(md->r_name) );

	_dl_insert( md, &_neo_r_man );
	_dl_init( &md->r_tbl );
	_dl_init( &md->r_result );
	md->r_pd  = pd;
	pd->i_tag = md;

	neo_setusradm( &md->r_MyId, 
		_neo_procbname, _neo_pid, MyName, _neo_hostname, PNT_INT32(pd) );

	neo_rpc_log_msg( "LINK", M_RDB, MyName, &res.l_usr, "", "" );

	return( md );

err4:	neo_free( pTag );
err3:	PaxosSessionFree( pSession );
err2:	p_close( pd );		
err1:	return( NULL );
}

int
rdb_unlink( r_man_t *md )
{
	p_id_t	*pd;
	r_req_unlink_t	req;
	r_res_unlink_t	res;
	int		len;
	struct Session	*pSession;
	p_Tag_t	*pTag;
	Msg_t	*pMsg;
	
	/*
	 * DB_MAN記述子有効フラグをチェックする
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno	= E_RDB_MD;
		goto err1;
	}
	/*
	 * データベースとの切断をする
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_UNLINK, sizeof(req) );

	if( p_request( md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	len = sizeof(res);
	if( p_response( md->r_pd, (p_head_t*)&res, &len ) )
		goto err1;
	
	if( (neo_errno = res.u_head.h_err) )
		goto err1;

	pd	= md->r_pd;
	pTag	= (p_Tag_t*)pd->i_pTag;
	pSession	= pTag->t_pSession;

	PaxosSessionClose( pSession );

	neo_free( pSession );
	while( (pMsg = list_first_entry( &pTag->t_Msgs, Msg_t, m_Lnk ) ) ) {
		list_del_init( &pMsg->m_Lnk );
		MsgDestroy( pMsg );
	}
	neo_free( pTag );
	pd->i_pTag	= NULL;

	_dl_remove( md );

	neo_proc_msg( "UNLINK", M_RDB, md->r_name, "", "" );

	neo_free( md );
	return( 0 );

err1: return( -1 );
}


