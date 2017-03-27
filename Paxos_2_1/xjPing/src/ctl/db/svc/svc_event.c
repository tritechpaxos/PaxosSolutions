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

/******************************************************************************
 *
 *         
 *	svc_event.c
 *
 *	説明		R_CMD_EVENT
 *				R_CMD_EVENT_CANCEL	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_event()
 *
 *  機能	イベント同期処理
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_event( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	int	len;
	r_req_event_t	req;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		err;

DBG_ENT(M_RDB,"svc_event");

	/*
	 * 要求パケットを読み出す
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブル、オープンユーザ記述子有効チェック
	 * 及びテーブル操作可能性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.e_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.e_name );

	if( (neo_errno = check_td_ud( td, ud ))
			||(neo_errno = check_lock( td, ud, R_WRITE ) ) ) {

		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err1;
	}

	/*
	 * イベント同期及び応答返答
	 */
	bcopy( &req, &ud->u_head, sizeof(ud->u_head) );

	switch( req.e_class ) {

		case R_EVENT_REC:
			if( (err = svc_rec_wait( td, ud, req.e_code )) ) {
				err_reply( pd, hp, err );
				goto err1;
			}
			break;

		default:
			err_reply( pd, hp, E_RDB_INVAL );
			neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ), 
						EVENTERR01, neo_errsym() );
			goto err1;
	}

DBG_EXIT(0);
	return( 0 );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_rec_wait()
 *
 *  機能	
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_rec_wait( td, ud, index )
	r_tbl_t	*td;		/* テーブル記述子	*/
	r_usr_t	*ud;		/* オープンユーザ記述子	*/
	int	index;		/* レコード絶対番号	*/
{
	r_res_event_t	res;
	int		err;

DBG_ENT(M_RDB,"svc_rec_wait");

	/*
	 * レコード絶対番号チェック
	 */
	if( index == -1 )
		index	= ud->u_cursor;

	if( index < 0 || td->t_rec_num <= index ) {
		err = E_RDB_INVAL;
		goto err1;
	}

	/*
	 * レコードが非ロック中だったら、イベント同期発生として、
	 * 応答を返す。そうでない場合は、該当オープンユーザを
	 * レコードの待ちユーザリストに登録する
	 */
	if( !(td->t_rec[index].r_stat & R_LOCK ) ) {	/* ok */
DBG_A(("rec_nowait:index=%d\n", index ));
		
		reply_head( &ud->u_head, (p_head_t*)&res, sizeof(res), 0 );

		if( p_reply( UDTOPD(ud), (p_head_t*)&res, sizeof(res) ) ) {

			err = neo_errno;

			goto err1;
		}
	} else {	/* wait */
DBG_A(("rec_wait:index=%d\n", index ));
		
		if( td->t_rec[index].r_wait )
			_dl_insert( &ud->u_rwait, 
					&((td->t_rec[index].r_wait)->u_rwait) );
		else
			td->t_rec[index].r_wait	= ud;

		ud->u_stat	|= R_USR_RWAIT;
		ud->u_index	= index;
	}
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	svc_rec_post()
 *
 *  機能	レコードロック解除を全ての待ちユーザに
 *		通知する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_rec_post( td, index )
	r_tbl_t	*td;		/* テーブル記述子	*/
	int	index;		/* レコード絶対番号	*/
{
	r_res_event_t	res;
	_dlnk_t		*dp;
	r_usr_t		*ud;
	int		err;
DBG_ENT(M_RDB,"svc_rec_post");

	/*
	 * 該当レコードの全ての待ちユーザに対して、
	 * ロック解除を通知する
	 */
	while( (ud = td->t_rec[index].r_wait) ) {

		dp = _dl_head( &ud->u_rwait );

		_dl_remove( &ud->u_rwait );
		_dl_init( &ud->u_rwait );

		ud->u_stat	&= ~R_USR_RWAIT;

		td->t_rec[index].r_wait	= RWTOUSR( dp );

		reply_head( (p_head_t*)&ud->u_head, (p_head_t*)&res, sizeof(res), 0 );

		if( p_reply( UDTOPD(ud), (p_head_t*)&res, sizeof(res) ) ) {

			err = neo_errno;

			goto err1;
		}
	}
DBG_EXIT(0);
	return( 0 );

err1:
	return( err );
}


/*****************************************************************************
 *
 *  関数名	svc_event_cancel()
 *
 *  機能	イベント同期要求をキャンセルする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_event_cancel( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* キャンセル要求ヘッダ	*/
{
	r_req_event_cancel_t	req;
	r_res_event_cancel_t	res;
	int			len;
	r_tbl_t			*td;
	r_usr_t			*ud;
	int			i;

DBG_ENT(M_RDB,"svc_event_cancel");

	/*
	 * キャンセル要求を読み出す
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;

	/*
	 * テーブル、オープンユーザ記述子有効チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.e_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.e_name );
	i  = req.e_code;

	if( (neo_errno = check_td_ud( td, ud )) ) {

		err_reply( pd, hp, neo_errno );

		goto err1;
	}
DBG_A(("code=%d,rec_num=%d\n", i, td->t_rec_num ));

	/*
	 * レコード絶対番号チェック
	 */
	if( i == -1 )
		i	= ud->u_cursor;

	if( i < 0 || td->t_rec_num <= i ) {

		err_reply( pd, hp, E_RDB_INVAL );

		goto err1;
	}
	/*
	 * レコード同期のキャンセル処理
	 */

	svc_cancel_rwait( td, ud, i );

	/*
	 * 応答を返す
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) )
		goto err1;
	
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_cancel_rwait()
 *
 *  機能	レコードのイベント同期をキャンセルする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_cancel_rwait( td, ud, i )
	r_tbl_t	*td;		/* テーブル記述子	*/
	r_usr_t	*ud;		/* オープンユーザ記述子	*/
	int	i;		/* レコードの絶対番号	*/
{
DBG_ENT(M_RDB,"svc_cancel_rwait");
DBG_A(("td=0x%x,ud=0x%x,i=%d\n", td, ud, i ));

	/*
	 * レコードの待ちユーザリストから
	 * 該当ユーザを外す
	 */
	if( ud->u_stat & R_USR_RWAIT ) {

		if( td->t_rec[i].r_wait == ud ) {

			td->t_rec[i].r_wait = RWTOUSR( _dl_head(&ud->u_rwait) );
		}
		if( _dl_head( &ud->u_rwait) ) {

			_dl_remove( &ud->u_rwait );
			_dl_init( &ud->u_rwait );

		}
		ud->u_stat	&= ~R_USR_RWAIT;
	}
DBG_EXIT( 0 );
	return( 0 );
}
