/******************************************************************************
*
*  p_user_cell_adm.c 	--- 
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
//#define	_LOCK_

#include	"../Paxos/xjPingPaxos.h"
//#include	"PaxosSession.h"
//#include	"neo_db.h"
//#include	<netinet/tcp.h>

p_id_t*
p_open( p_port_t* pLaddr )
{
	p_id_t	*pd;
	int		Fd;

	pd = (p_id_t*)neo_malloc( sizeof(p_id_t) );
	if( !pd )	goto err;

	memset( pd, 0, sizeof(*pd) );

	bcopy( pLaddr, &pd->i_remote, sizeof(p_port_t) );
	pd->i_local.p_addr.a_pd	= pd;

	pd->i_spacket.s_min_bytes = pd->i_spacket.s_min_time = SOCK_BUF_MAX;
	pd->i_rpacket.s_min_bytes = pd->i_rpacket.s_min_time = SOCK_BUF_MAX;

	_dl_init( &pd->i_lnk );
	_dl_insert( &pd->i_lnk, &_neo_port );

	Fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( Fd < 0 )	goto err1;

	if( connect( Fd, (struct sockaddr*)&pd->i_remote.p_addr.a_un,
						sizeof(pd->i_remote.p_addr.a_un) ) < 0 ) goto err1;
	pd->i_sfd = pd->i_rfd = Fd;

	return( pd );
err1:
	neo_free( pd );
err:
	return( NULL );
}

int
p_close( p_id_t *pd )
{
	_dl_remove( &pd->i_lnk );

	shutdown( pd->i_sfd, 2 );
	close( pd->i_sfd );

	neo_free( pd );
	return( 0 );
}

int
p_peek( p_id_t* pd, p_head_t *pHead, int Len, int flag )
{
	
	int		Ret = 0;

	Ret = PeekStream( pd->i_rfd, (char*)pHead, Len );
	if( Ret )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
p_request( p_id_t *pd, p_head_t *pH, int Len )
{
	int	Ret;

	Ret = SendStream( pd->i_sfd, (char*)pH, Len );
	return(0);
}

Msg_t*
p_responseByMsg( p_id_t *pd )
{
	p_head_t	Head;
	MsgVec_t	Vec;
	Msg_t		*pMsg;
	char		*pBuf;
	int			size, len;
	int		Ret;

	Ret = p_peek( pd, &Head, sizeof(Head), 0 );
	if( Ret < 0 )	goto err;

	size = len	= sizeof(p_head_t) + Head.h_len;
	pBuf = neo_malloc( len );
	
	Ret = p_response( pd, (p_head_t*)pBuf, &len );
	if( Ret < 0 )	goto err1;

	pMsg	= MsgCreate( 1, neo_malloc, neo_free );
	MsgVecSet(&Vec, VEC_MINE, pBuf, size, len );
	MsgAdd( pMsg, &Vec );

	return( pMsg );
err1:
	neo_free( pBuf );
err:
	return( NULL );
}

int
p_response( p_id_t *pd, p_head_t *pH, int *pL )
{
	int		Ret;
	int		L;

	L	= *pL;

	Ret = RecvStream( pd->i_rfd, (char*)pH, L );
	if( Ret )	goto err;

	return( 0 );
err:
	return( -1 );
}

_dlnk_t		_neo_r_man;
static	int	_r_init;

extern char *_neo_procbname, _neo_hostname[];
extern int  _neo_pid;

/*
 *	CellId = Cell + ":" + Id
 */
r_man_t*
rdb_link( p_name_t MyName, r_man_name_t CellId )
{
	r_man_t		*md;
	p_id_t		*pd;
	r_req_link_t	req;
	r_res_link_t	res;
	p_port_t	myport;
	int		len;
	char	*pCell, *pId;
	int	i;

	if( !_r_init ) {
		_dl_init( &_neo_r_man );
		_r_init	= 1;
	}
	pId = pCell = CellId;
	for( i = 0; i < sizeof(CellId); i++, pId++ ) {	
		if( *pId == ':' ) {
			*pId++	= 0;
			break;
		}
	}
	if( i >= sizeof(CellId) )	goto err1;

	memset( &myport, 0, sizeof(myport) );

	myport.p_addr.a_un.sun_family	= AF_UNIX;
	sprintf( myport.p_addr.a_un.sun_path, 
			"/tmp/"XJPING_ADMIN_PORT"_%s_%s", pCell, pId );

	myport.p_role = P_CLIENT;
	if( !( pd = p_open( &myport ) ) ) {
		goto err1;
	}
	/*
	 * DB_MANサーバへ接続する(データベース接続）
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_LINK, sizeof(r_req_link_t) );

	neo_setusradm( &req.l_usr, 
		_neo_procbname, _neo_pid, MyName, _neo_hostname, PNT_INT32(pd) );

	if( p_request( pd, (p_head_t*)&req, sizeof(req) ) ) {
		goto err2;
	}
	
	/*
	 * サーバからの接続応答を待つ
	 */
	len = sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) ) {
		goto err2;
	}
	
	if( (neo_errno = res.l_head.h_err) )
		goto err2;

	/*
	 * DB_MAN記述子を生成する
	 */
	if( !(md = (r_man_t *)neo_malloc( sizeof(r_man_t) )) ) {
		neo_errno	= E_RDB_NOMEM;
		goto err2;
	}
	memset( md, 0, sizeof(*md) );

	/*
	 * 生成したDB_MAN記述子初期化して、_neo_r_manへ繋ぐ
	 */
	md->r_ident	= R_MAN_IDENT;
	strncpy( md->r_name, pCell, sizeof(md->r_name) );

	_dl_insert( md, &_neo_r_man );
	_dl_init( &md->r_tbl );
	_dl_init( &md->r_result );
	md->r_pd  = pd;
	pd->i_tag = md;

	neo_setusradm( &md->r_MyId, 
		_neo_procbname, _neo_pid, MyName, _neo_hostname, PNT_INT32(pd) );

	neo_rpc_log_msg( "LINK", M_RDB, MyName, &res.l_usr, "", "" );

	return( md );
err2:	p_close( pd );		
err1:
		return( NULL );
}

int
rdb_unlink( r_man_t *md )
{
	p_id_t	*pd;
	r_req_unlink_t	req;
	r_res_unlink_t	res;
	int		len;
	
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

	p_close( pd );

	_dl_remove( md );

	neo_proc_msg( "UNLINK", M_RDB, md->r_name, "", "" );

	neo_free( md );
	return( 0 );

err1: return( -1 );
}


