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
 *	svc_create.c
 *
 *	説明		R_CMD_CREATE
 *				R_CMD_DROP	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_create()
 *
 *  機能	テーブルを作成する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_create( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	r_req_create_t	*reqp;
	r_res_create_t	*resp;
	int		len;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i;

DBG_ENT(M_RDB,"svc_create");

	/*
	 * 要求パケット受信バッファをアロック
	 * 	[ R_CMD_CREATE要求パケットサイズが可変長だから]   
	 */
	len	= hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_req_create_t *)neo_malloc( len ) ) ) {
		len	= sizeof(*hp);
		if( p_recv( pd, hp, &len ) ) {	/* discard */
			if( neo_errno != E_LINK_BUFSIZE )
				goto err1;
		}
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}
	
	/*
	 * 要求パケットを読み出す
	 */
	if( p_recv( pd, (p_head_t*)reqp, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err2;
	}

	if ( _svc_inf.f_stat == R_SERVER_STOP ) {
		err_reply( pd, hp, E_RDB_STOP );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR10, neo_errsym() );
		goto err2;
	}

	/*
	 * リプライ構造体メモリアロック
	 * 	( R_CMD_CREATE応答パケットサイズが可変長だから)
	 */
	len	= sizeof(r_res_create_t) + reqp->c_item_num*sizeof(r_item_t);
	if( !(resp = (r_res_create_t*)neo_malloc( len ) ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err2;
	}

	/*
	 * テーブル名チェック
	 */
	if( HashGet( &_svc_inf.f_HashOpen, reqp->c_name ) ) {
			err_reply( pd, hp, E_RDB_EXIST );
			neo_rpc_log_err( M_RDB, svc_myname, 
					&( PDTOENT(pd)->p_clnt ), 
						CREATEERR01, neo_errsym() );
			goto err3;
	}

	/*
	 * 古いテーブルをメモリから消去
	 */
	if ( (neo_errno = drop_table()) ) {
		err_reply( pd, hp, neo_errno ) ;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR05, neo_errsym() );
		goto	err3;
	}

	/*
	 * オープンユーザ/テーブル記述子メモリアロック
	 */
	if( !(ud = _r_usr_alloc() ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err3;
	}
	if( !(td = svc_tbl_alloc() ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err4;
	}

	/*
	 * テーブル(ファイル)クリエイト
	 *  既にファイルが存在した場合は、新規に作り直される
	 */
	if( TBL_CREATE( reqp->c_name, R_TBL_NORMAL, (long)time(0), 
			reqp->c_rec_num, reqp->c_item_num, reqp->c_item,
			0, 0 ) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR06, neo_errsym() );
		goto err5;
	}

	/*
	 * テーブル(ファイル)オープン
	 */
	if( TBL_OPEN( td, reqp->c_name ) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR07, neo_errsym() );
		goto err5;
	}

	/*
	 * レコード管理構造体/レコードバッファメモリアロック
	 */
	if( (neo_errno = rec_cntl_alloc( td )) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err6;
	}
	if( (neo_errno = rec_buf_alloc( td )) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err7;
	}

	/*
	 *	r_rec_tフラグ設定
	 */
DBG_A(("rec_num=%d\n", td->t_rec_num ));
	for( i = 0; i < td->t_rec_num; i++ ) {
		td->t_rec[i].r_stat |= R_REC_CACHE;
	}

	/*
	 * アクセスタイム設定
	 */
	td->t_access	= time( 0 );

	/*
	 * オープンユーザ構造体情報設定
	 */
	r_port_t	*pe = PDTOENT(pd);

	_dl_insert( &ud->u_port, &(PDTOENT(pd)->p_usr_ent) );
	ud->u_pd	= pd;
	ud->u_mytd	= td;
	ud->u_td	= reqp->c_td;	// client td

	_dl_insert( ud, &td->t_usr );
	td->t_usr_cnt = 1;

	HashPut( &pe->p_HashUser, td->t_name, ud );
#ifdef	ZZZ
#else
	ud->u_refer = 1;
#endif

	_dl_insert( td, &(_svc_inf.f_opn_tbl ) );

	HashPut( &_svc_inf.f_HashOpen, td->t_name, td );


	/*
	 * リプライ情報設定
	 */
	reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

	resp->c_td		= td;
	resp->c_ud		= ud;
	resp->c_rec_num		= td->t_rec_num;
	resp->c_rec_size	= td->t_rec_size;
	resp->c_item_num	= td->t_scm->s_n;
	bcopy( td->t_scm->s_item, resp+1, td->t_scm->s_n*sizeof(r_item_t) );

	neo_rpc_log_msg( "RDB_CREATE", M_RDB, svc_myname,
			    &( PDTOENT(pd)->p_clnt), CREATELOG01, td->t_name );
	/*
	 * リプライ
	 */
	if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err3;
	}

	/*
	 * 通信バッファの解放
	 */
	neo_free( reqp );

DBG_EXIT(0);
	return( 0 );

err7: DBG_T("err7");
	rec_cntl_free( td );
err6: DBG_T("err6");
	TBL_CLOSE( td );
	TBL_DROP( reqp->c_name );
err5: DBG_T("err5");
	_r_tbl_free( td );
err4: DBG_T("err4");
	_r_usr_free( ud );
err3: DBG_T("err3");
	neo_free( resp );
err2: DBG_T("err2");
	neo_free( reqp );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_drop()
 *
 *  機能	テーブルを削除する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_drop( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	r_tbl_t		*td;
	r_req_drop_t	req;
	r_res_drop_t	res;
	int		len;
DBG_ENT(M_RDB,"svc_drop");

	/*
	 * 要求パケットを読み出す
	 */
	len	= sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	if ( _svc_inf.f_stat == R_SERVER_STOP ) {
		err_reply( pd, hp, E_RDB_STOP );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR10, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブルチェック
	 */
	if( (td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.d_name)) ) {

			if( td->t_usr_cnt == 0 ) {

				if ( (neo_errno = drop_td( td )) ) {
					err_reply( pd, hp, neo_errno );
					goto	err1;
				}
			} else {
				err_reply( pd, hp, E_RDB_TBL_BUSY );
				neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ),
						CREATEERR02, neo_errsym() );
				goto err1;
			}
	}

	/*
	 * テーブル(ファイル)削除
	 */
	if( TBL_DROP( req.d_name ) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR08, neo_errsym() );
		goto err1;
	}

	neo_rpc_log_msg( "RDB_DROP", M_RDB, svc_myname,
			    &( PDTOENT(pd)->p_clnt), CREATELOG02, req.d_name );

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err1;
	}
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

