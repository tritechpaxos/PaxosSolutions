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
 *	svc_hold.c
 *
 *	説明		R_CMD_HOLD
 *				R_CMD_RELEASE	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_hold()
 *
 *  機能	テーブルの排他を宣言する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_hold( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int	len;
	r_req_hold_t	*reqp;
	r_res_hold_t	res;
	r_tbl_t		**tds = NULL;
	r_usr_t		**uds = NULL;
	int		i, j;
	r_hold_t	*hds;
	r_twait_t	*twaitp;

DBG_ENT(M_RDB,"svc_hold");

	/*
	 * リクエストレシーブ
	 */
	len = hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_req_hold_t*)neo_malloc(len) ) ) {

		len = sizeof(p_head_t);
		if( p_recv( pd, hp, &len ) ) {	/* discard */
			if( neo_errno != E_LINK_BUFSIZE )
				goto err1;
		}

		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	if( p_recv( pd, (p_head_t*)reqp, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err2;
	}

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	if( !(tds = (r_tbl_t**)neo_malloc(sizeof(void*)*reqp->h_n)) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err2;
	}
	if( !(uds = (r_usr_t**)neo_malloc(sizeof(void*)*reqp->h_n)) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err2;
	}
	r_port_t	*pe = PDTOENT(pd);

	for( hds = (r_hold_t *)(reqp+1), i = 0; i < reqp->h_n; i++ ) {

		tds[i] = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, hds[i].h_name);
		uds[i] = (r_usr_t*)HashGet( &pe->p_HashUser, hds[i].h_name );

		if( (neo_errno = check_td_ud( tds[i], uds[i] )) ) {
			err_reply( pd, hp, neo_errno );
			goto err2;
		}
	}

	/*
	 * テーブルロック
	 */
	for( i = 0; i < reqp->h_n; i++ ) {

		/*
		 * 排他可能であることをチェックする
		 */
		if( !(tds[i]->t_stat & R_LOCK) ) {

			/*
			 * 全てのレコードが他から排他されていない事をチェック
			 */
			if( (j = _check_rec_lock( tds[i], uds[i] )) != -1 ) {

				reply_head( (p_head_t*)reqp, (p_head_t*)&res,
					sizeof(res), E_RDB_REC_BUSY );
				res.h_td  = uds[i]->u_td;	// Client td
				res.h_abs = j;

				p_reply( pd, (p_head_t*)&res, sizeof(res) );
				neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ), 
						HOLDERR01, neo_errsym() );
				goto err2;
			}

			continue;

		} else {
			if( tds[i]->t_lock_usr == uds[i] ) {
				err_reply( pd, hp, E_RDB_HOLD );
				neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ), 
						HOLDERR02, neo_errsym() );
				goto err2;
			}
			if( reqp->h_wait == R_NOWAIT ) {
				err_reply( pd, hp, E_RDB_TBL_BUSY );
				goto err2;
			}
		}

		/*
		 * テーブル待ち構造体作成
		 */
		if( !(twaitp = (r_twait_t *)neo_malloc( sizeof(r_twait_t))) ) {
			err_reply( pd, hp, E_RDB_NOMEM );
			neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, 
							neo_errsym() );
			goto err2;
		}

		PDTOENT( pd )->p_twait	= twaitp;
		twaitp->w_pd		= pd;
		bcopy( hp, &twaitp->w_head, sizeof(p_head_t) );

		_dl_init( &twaitp->w_usr );
		twaitp->w_Id	= twaitp;

		for( j = 0; j < reqp->h_n; j++ ) {
			uds[j]->u_mode	= hds[j].h_mode;
			_dl_insert( &uds[j]->u_twait, &twaitp->w_usr );
		}

		_dl_insert( twaitp, &tds[i]->t_wait );
		tds[i]->t_wait_cnt++;
		twaitp->w_td	= tds[i];

		/*
		 * テーブルリリースのイベント待ちへ
		 */
		goto ret;
	}

	/* 
	 * 全てのテーブルがロック可能なので、テーブルをロックする
	 */
	for( i = 0; i < reqp->h_n; i++ ) {
		tds[i]->t_stat 	|= hds[i].h_mode;;
		tds[i]->t_lock_usr	= uds[i];
	}

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)reqp, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}

	/*
	 * 通信バッファの解放
	 */
ret:
	neo_free( reqp );
	if( tds )	neo_free( tds );
	if( uds )	neo_free( uds );
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( reqp );
err1: 
	if( tds )	neo_free( tds );
	if( uds )	neo_free( uds );
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_release()
 *
 *  機能	テーブルの排他を解除する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_release( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	r_req_release_t	req;
	r_res_release_t	res;
	int		len;
	r_tbl_t		*td;
	r_usr_t		*ud;

DBG_ENT(M_RDB,"svc_release");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof( req );
	if( p_recv( pd, (p_head_t*)&req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);
	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.r_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.r_name );

	if( (neo_errno = check_td_ud( td, ud )) || 
		(neo_errno = check_lock( td, ud, R_WRITE )) ) {
		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err1;
	}

	/*
	 * 自分で排他を宣言していなければ、エラーとなる
	 */
	if( !(td->t_stat & R_LOCK) || (td->t_lock_usr != ud) ) {
		err_reply( pd, hp, E_RDB_HOLD );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						HOLDERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * リリース(リリース待ちユーザがいた場合は通知する)
	 */
	svc_tbl_post( td );

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
DBG_EXIT( -1 );
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	svc_hold_cancel()
 *
 *  機能	テーブルの排他宣言をキャンセルする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_hold_cancel( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	r_req_hold_cancel_t	req;
	r_res_hold_cancel_t	res;
	int			len;
	_dlnk_t			*dp;
	r_port_t		*pe;
	r_twait_t		*twaitp;

DBG_ENT(M_RDB,"svc_hold_cancel");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;

	/*
	 * テーブル待ち構造体の解除
	 */
	if( (pe = PDTOENT(pd)) ) {
		if( (twaitp = pe->p_twait) ) {

			twaitp->w_td->t_wait_cnt--;

			_dl_remove( twaitp );

			while( (dp = _dl_head( &twaitp->w_usr )) )
				_dl_remove( dp );

			neo_free( twaitp );

			pe->p_twait = 0;
		} else {
			err_reply( pd, hp, E_RDB_NOEXIST );

			goto err1;
		}
	}

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) < 0 )
		goto err1;
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_tbl_post()
 *
 *  機能	テーブルリリース(リリース待ちユーザがいた場合は通知する)
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_tbl_post( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	r_usr_t		*ud;
	r_res_hold_t	hres;
	r_twait_t	*twaitp;
	_dlnk_t		*dp;
	int		i;

DBG_ENT(M_RDB,"svc_tbl_post");

	/*
	 * テーブル構造体のロック状態の解除
	 */
	td->t_stat	&= ~R_LOCK;
	td->t_lock_usr	= 0;

DBG_A(("t_wait_cnt=%d\n", td->t_wait_cnt));

	/*
	 * リリース待ちユーザがいた場合
	 */
	if( td->t_wait_cnt ) {

		/*
		 * テーブル待ち構造体のリストを順番に検査する
		 */
		while( (twaitp = (r_twait_t*)_dl_head( &td->t_wait )) ) {
DBG_A(("twaitp=0x%x\n", twaitp ));

			_dl_remove( twaitp );
			td->t_wait_cnt--;

			for( dp = _dl_head(&twaitp->w_usr); 
					_dl_valid(&twaitp->w_usr,dp);
							dp = _dl_next(dp) ) {
				ud = TWTOUSR( dp );
DBG_A(("ud=0x%x,mytd=0x%x(dp=0x%x)\n", ud, ud->u_mytd, dp ));

				if( ud->u_mytd->t_stat & R_LOCK ) {
DBG_A(("wait again\n"));
				  _dl_insert( twaitp, &ud->u_mytd->t_wait);
				  ud->u_mytd->t_wait_cnt++;
				  twaitp->w_td	= ud->u_mytd;
				  goto next;

				} else {
				  /*
				   * 全てのレコードが他から排他されていない事
				   */
				  if( (i = _check_rec_lock( ud->u_mytd, ud )) 
								!= -1 ) {
					reply_head( (p_head_t*)&twaitp->w_head, (p_head_t*)&hres,
						sizeof(hres), E_RDB_REC_BUSY );
					hres.h_td  = ud->u_td;
					hres.h_abs = i;

					if( p_reply( twaitp->w_pd, 
							(p_head_t*)&hres, sizeof(hres) ) )
						DBG_A(("ERR tbl_post reply\n"));

					goto next;
				  }

				}
			}

			/*
			 * ロック可能なテーブル待ち構造体が存在した
			 */
			while( (dp = _dl_head(&twaitp->w_usr)) ) {

				ud = TWTOUSR( dp );
				ud->u_mytd->t_stat 	|= ud->u_mode;
				ud->u_mytd->t_lock_usr	= ud;

				_dl_remove( dp );
			}

			/*
			 * リプライ
			 */
			reply_head( (p_head_t*)&twaitp->w_head, (p_head_t*)&hres, 
									sizeof(hres), 0 );

			if( p_reply( twaitp->w_pd, (p_head_t*)&hres, sizeof(hres) ) ) {

				DBG_A(("LOG error reply to hold\n"));
			}
			PDTOENT(twaitp->w_pd)->p_twait	= 0;
			neo_free( twaitp );
			goto end;
		next:;
		}
	}
end:

DBG_EXIT( 0 );
	return( 0 );
}

/*****************************************************************************
 *
 *  関数名	_check_rec_lock()
 *
 *  機能	全てのレコードが他から排他されていない事をチェック
 * 
 *  関数値
 *     		正常	-1
 *		異常	排他されているレコードの絶対番号( >= 0 )
 *
 ******************************************************************************/
int
_check_rec_lock( td, ud )
	r_tbl_t	*td;	/* テーブル記述子	*/
	r_usr_t	*ud;	/* オープンユーザ構造体	*/
{
	int	i;

DBG_ENT( M_RDB, "_check_rec_lock" );

	/*
	 * 全てのレコードが他から排他されていない事をチェック
	 */
	for( i = 0; i < td->t_rec_num; i++ ) {

		if( !(td->t_rec[i].r_stat & R_REC_CACHE) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 );

		if( td->t_rec[i].r_stat & R_USED ) {

			if( (td->t_rec[i].r_stat & R_LOCK) &&
			    (td->t_rec[i].r_access != ud) ) {

				goto err1;
			}
		}
	}

DBG_EXIT(-1);
	return( -1 );
err1:;
DBG_EXIT(i);
	return( i );
}
