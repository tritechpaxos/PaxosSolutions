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
 *	rdb_hold.c
 *
 *	説明		テーブルの排他を宣言する
 *			       R_CMD_RELEASE
 *				テーブルの排他を解除する
 * 
 ******************************************************************************/

#include	"neo_db.h"

#define	RETRY	1	/* 1000 */
#define	EVER		/* i++  */

/******************************************************************************
 * 関数名
 *		rdb_hold()
 * 機能        
 *		テーブルの排他を宣言する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_hold( n, hds, wait )
	int		n;	/* 排他を宣言するテーブルの数	*/
	r_hold_t	*hds;	/* 排他を宣言するテーブル	*/
	int		wait;	/* 待ちモード			*/
{
	int			i;
	r_req_hold_t		*reqp;
	r_res_hold_t		res;
	int			len, lens, mode;
	r_hold_t		*hd;
	r_req_hold_cancel_t	creq;
	r_res_hold_cancel_t	cres;
	r_man_t			*dbman;

DBG_ENT(M_RDB,"rdb_hold");

	/*
	 *	引数チェック
	 */
	if( n <= 0 || R_TBL_HOLD_MAX < n ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( wait & ~(R_WAIT|R_NOWAIT) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	dbman = hds[0].h_td->t_md;
	for( i = 0; i < n; i++ ) {
		if( !((mode = hds[i].h_mode) & R_LOCK) || 
		     ((mode & R_SHARE) && (mode & R_EXCLUSIVE)) ) {
			neo_errno = E_RDB_INVAL;
			goto err1;
		}
		if( hds[i].h_td->t_ident != R_TBL_IDENT ) {
			neo_errno = E_RDB_TD;
			goto err1;
		}
		if( hds[i].h_td->t_stat & R_LOCK ) {
			neo_errno = E_RDB_HOLD;
			goto err1;
		}
		if( hds[i].h_td->t_md != dbman ) {
			neo_errno = E_RDB_INVAL;
			goto err1;
		}
	}

	/*
	 *	リクエスト
	 *		( R_CMD_HOLD はパケットサイズが可変 )
	 */
	len = sizeof(r_req_hold_t) + n*(sizeof(r_hold_t));
	if( !(reqp = (r_req_hold_t *)neo_malloc(len) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}
	_rdb_req_head( (p_head_t*)reqp, R_CMD_HOLD, len );

	reqp->h_wait	= wait;
	reqp->h_n	= n;
	for( hd = (r_hold_t *)(reqp+1), i = 0; i < n; i++ ) {
		strncpy( hd[i].h_name, hds[i].h_td->t_name, sizeof(hd[i].h_name) );
		hd[i].h_mode	= hds[i].h_mode;
	}
again:
	if( p_request( TDTOPD(hds[0].h_td), (p_head_t*)reqp, len ) )
		goto err2;

	/*
	 *	レスポンス
	 *	(RETRY * p_max_time)時間 応答を待つ
	 */
	for( i = 0; i < RETRY; EVER ) {

		lens = sizeof(r_res_hold_t);
		if( p_response( TDTOPD(hds[0].h_td), (p_head_t*)&res, &lens ) == 0 ) {

			if( !(neo_errno = res.h_head.h_err) )
				goto ok;

			if( neo_errno != E_RDB_REC_BUSY || (wait & R_NOWAIT) ) {
				goto err2;

			} else {
				if(rdb_event(res.h_td, R_EVENT_REC, res.h_abs))
					goto err2;
				goto again;
			}
		} else {
			switch( neo_errno ) {
			case E_LINK_TIMEOUT:	continue;
			case E_SKL_TERM:	goto cancel;
			default:		goto err2;
			}
		}
	}

	neo_errno = E_RDB_RETRY;

cancel:;
	/*
	 *	HOLD_CANCEL プロトコル  (タイムアウト)
	 *	時間内に応答がなかったか、NEO_SIGTERM で割り込まれた
	 */
	_rdb_req_head( (p_head_t*)&creq, R_CMD_HOLD_CANCEL, sizeof(creq) );

	if( p_request( TDTOPD(hds[0].h_td), (p_head_t*)&creq, sizeof(creq) ) )
		goto err2;
	
	do {
		/*
		 *	HOLD_CANCEL プロトコルのレスポンスパケットは、
		 *	HOLD プロトコルのレスポンスパケットと同じ長さ
		 */
		len = sizeof(cres);

		if( p_response( TDTOPD(hds[0].h_td), (p_head_t*)&cres, &len ) )
			goto err2;

DBG_B( ("p_response h_cmd == %d\n", cres.h_head.h_cmd) );
	} while( cres.h_head.h_cmd != R_CMD_HOLD_CANCEL );

	goto	err2;

ok:
	for( i = 0; i < n; i++ ) {
		hds[i].h_td->t_stat = hds[i].h_mode;
	}

	neo_free( reqp );
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( reqp );
err1:
DBG_EXIT(-1);
	return( -1 );
}

/******************************************************************************
 * 関数名
 *		rdb_release()
 * 機能        
 *		テーブルの排他を解除する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_release( td )
	r_tbl_t	*td;	/* テーブル記述子	*/
{
	r_req_release_t	req;
	r_res_release_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_release");

	/*
	 *	引数チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	if( !(td->t_stat & R_LOCK ) ) {
		neo_errno = E_RDB_HOLD;
		goto err1;
	}

	/*
	 *	リクエスト
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_RELEASE, sizeof(req) );

	strncpy( req.r_name, td->t_name, sizeof(req.r_name) );

	if( p_request( TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	/*
	 *	レスポンス
	 */
	len = sizeof(r_res_release_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len )
				||( neo_errno = res.r_head.h_err) )
		goto err1;

	td->t_stat &= ~R_LOCK;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT( -1 );
	return( -1 );
}
