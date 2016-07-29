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
 *	svc_stop.c
 *
 *	説明		R_CMD_STOP
 *				R_CMD_RESTART	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

int
svc_shutdown( void )
{
	r_tbl_t		*td, *ttd;
	/*
	 * 既に停止中の時
	 */
	if( _svc_inf.f_stat == R_SERVER_STOP ) {
		goto err1;
	}
	
	/*
	 * テーブルチェック
	 */
	for ( td = ( r_tbl_t *)_dl_head(&(_svc_inf.f_opn_tbl));
		_dl_valid(&(_svc_inf.f_opn_tbl), td); ) {

		ttd = td ;
		td = (r_tbl_t *)_dl_next(td);

		/*
		 * アクセスユーザがいなければ削除
		 */
		if ( ttd->t_usr_cnt == 0 ) {
			
			drop_td( ttd ) ;

		} else {
			goto	err1;
		}
	}

	/*
	 * サーバ停止
	 */
	_svc_inf.f_stat = R_SERVER_STOP;

	TBL_DBCLOSE( _svc_inf.f_path ) ;

	chdir( _svc_inf.f_root );

	return( 0 );
err1:
	return( -1 );
}
/*****************************************************************************
 *
 *  関数名	svc_stop()
 *
 *  機能	サーバを停止させる
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_stop( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	r_req_stop_t	req;
	r_res_stop_t	res;
	int		len;
	r_tbl_t		*td, *ttd;

DBG_ENT(M_RDB,"svc_stop");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * 既に停止中の時
	 */
	if( _svc_inf.f_stat == R_SERVER_STOP ) {
		err_reply( pd, hp, E_RDB_STOP );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						STOPERR01, neo_errsym() );
		goto err1;
	}
	
	/*
	 * テーブルチェック
	 */
	for ( td = ( r_tbl_t *)_dl_head(&(_svc_inf.f_opn_tbl));
		_dl_valid(&(_svc_inf.f_opn_tbl), td); ) {

		ttd = td ;
		td = (r_tbl_t *)_dl_next(td);

		/*
		 * アクセスユーザがいなければ削除
		 */
		if ( ttd->t_usr_cnt == 0 ) {
			
			drop_td( ttd ) ;

		} else {
			err_reply( pd, hp, E_RDB_TBL_BUSY ) ;
			neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ), 
						STOPERR02, neo_errsym() );
			goto	err1;
		}
	}

	/*
	 * サーバ停止
	 */
	_svc_inf.f_stat = R_SERVER_STOP;

	TBL_DBCLOSE( _svc_inf.f_path ) ;

	chdir( _svc_inf.f_root );

	neo_rpc_log_msg( "RDB_STOP", M_RDB, svc_myname, 
					&( PDTOENT(pd)->p_clnt ), STOPLOG01, "" );

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
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
 *  関数名	svc_restart()
 *
 *  機能	DB_MANサーバを再スタートさせる
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_restart( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	 */
	p_head_t	*hp;		/* 要求パケットヘッダ    */
{
	r_req_restart_t	req;
	r_res_restart_t  res;
	int		len;

DBG_ENT(M_RDB,"svc_restart");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * 既に活動中の時
	 */
	if( _svc_inf.f_stat == R_SERVER_ACTIVE ) {
		err_reply( pd, hp, E_RDB_STOP );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						STOPERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * サーバ再スタート
	 */
	if ( *(req.s_path) ) {

		if ( TBL_DBOPEN( req.s_path ) ) {
			err_reply( pd, hp, neo_errno );
			goto	err1;
		}

		strcpy(_svc_inf.f_path, req.s_path );
	} else {
		if ( TBL_DBOPEN( _svc_inf.f_path ) ) {
			err_reply( pd, hp, neo_errno );
			goto	err1;
		}
	}

	_svc_inf.f_stat = R_SERVER_ACTIVE;

	neo_rpc_log_msg( "RDB_RESTART", M_RDB, svc_myname, 
					&( PDTOENT(pd)->p_clnt ), STOPLOG02, "" );

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err1;
	}

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}
