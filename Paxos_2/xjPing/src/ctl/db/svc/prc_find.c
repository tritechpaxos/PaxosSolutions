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


#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"y.tab.h"

/*****************************************************************************
 *
 *  関数名	prc_find()
 *
 *  機能	レコードの検索
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
prc_find( reqp, respp, res_lenp )
	r_req_find_t	*reqp;
	r_res_find_t	**respp;
	int		*res_lenp;
{
	r_res_find_t	*resp;
	int	len;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i;
	int		m, pn;
	r_value_t	v;
	int		cnt, n;
	char		*bufp;
	int		mode;
	r_expr_t	*check;
DBG_ENT(M_RDB,"prc_find");

#ifdef XXX
	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, &req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}
#endif

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	td	 = reqp->f_td;
	ud	 = reqp->f_ud;
	mode	= reqp->f_mode;

	if( (neo_errno = check_td_ud( td, ud )) || 
	    (neo_errno = 
                check_lock( td, ud, ((mode&R_LOCK) ? R_WRITE : R_READ) )) ) {
		goto err1;
	}

	/*
	 * リプライ用のバッファのメモリアロック
	 * 取得されたメモリは R_PAGE_SIZEを最大値とする
	 */
	if( R_PAGE_SIZE >= td->t_rec_size ) {
		pn	= R_PAGE_SIZE/td->t_rec_size;
		m	= reqp->f_num < pn ? reqp->f_num : pn;
	} else {
		pn	= m = 1;
	}
	len = sizeof(r_res_find_t) + m*td->t_rec_size;
	if( !(resp = (r_res_find_t *)neo_malloc( len )) ) {
		
		neo_errno = E_RDB_NOMEM;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	check	= ud->u_search.s_expr;
	cnt = 0;	/* トータル取得レコード数の初期化 */

	/*
	 * 検索 ( for )
	 *      初期化:  コピー用バッファのポインタ(bufp)を
	 *		 レスポンス用バッファのレコード領域の先頭に設定
	 *		 取得レコード数(i)を0に初期化
	 *	終了条件:カーソル位置(i)がテーブルの最後に来るまで
	 *	動作:	 カーソルを一つ進める
	 *
	 *	*リプライの送信は R_PAGE_SIZEをレコードバッファの最大値
	 *	 とする、従って、1回で送信できない時は、分割して送られる
	 *	 その時、(n)は、バッファに設定されているレコードの数
	 *	 (f_num)で、まだ残りがあるとき、f_moreに1を設定する
	 *
	 */
	for( bufp = (char *)(resp+1), n = 0;
		(i = ud->u_cursor) < td->t_rec_num; ud->u_cursor++ ) {

		if( !(td->t_rec[i].r_stat & R_REC_CACHE) )	/* load */
			svc_load( td, i, pn );
		
		if( !(td->t_rec[i].r_stat & R_USED ) )
			continue;

		if( td->t_rec[i].r_head->r_Cksum64 ) {
			assert( !in_cksum64( td->t_rec[i].r_head, td->t_rec_size, 0 ) );
		}
		/*
		 * 検索条件が設定してあった場合は、条件を評価する
		 * 		( rdb_evaluate()は clnt/rdb_expr.c )
		 */
		if( check ) {
			if( rdb_evaluate( check, (char*)td->t_rec[i].r_head, &v ) 
					|| v.v_type != VBOOL ) {

				neo_errno = E_RDB_SEARCH;

				goto err2;
			}
		}

		/*
		 * 検索条件が成立するレコードが見つかった
		 */
		if( !check || v.v_value.v_int ) {	/* found */

			/*
			 * 排他モードが設定されていれば、排他をかける
			 */
			if ( mode & R_LOCK )  {

				/* 
				 * レコード排他チェック
				 */
				if( (td->t_rec[i].r_stat & R_LOCK)
					&& td->t_rec[i].r_access != ud ) {

					goto	record_lock ;
				}  else {
					td->t_rec[i].r_stat &= ~(R_LOCK);
					td->t_rec[i].r_stat |= (mode & R_LOCK);
					td->t_rec[i].r_access = ud ;
				}

			} else {

				/*
				 * レコード参照排他チェック
				 */
				if( ( td->t_rec[i].r_stat & R_EXCLUSIVE )
					&& td->t_rec[i].r_access != ud ) {

					goto record_lock;
				} 
			}

			/*
			 * レコードデータのコピー&取得レコード数のカウント
			 */
			bcopy( td->t_rec[i].r_head, bufp, td->t_rec_size );
			((r_head_t *)bufp)->r_stat	= td->t_rec[i].r_stat;
			bufp += td->t_rec_size;
			n++; cnt++;

			/*
			 * トータル取得レコード数が、要求レコード数に
			 * 達したらリプライを返し、追加(f_more)は無し
			 */
			if( cnt >= reqp->f_num ) {

				resp->f_more = 0;

				ud->u_cursor++;

				goto reply;
			}

			/*
			 * リプライ1回あたりの最大送信レコード数(m)よりも
			 * 取得レコード数(n)が小さい時は再検索
			 */
			if( n < m )
				continue;
			
			/*
			 * トータル取得レコード数が、要求レコード数に
			 * 達していないので、追加(f_more)を行う
			 */
			resp->f_more = 1;

			ud->u_cursor++;

			goto reply;
		} else
			continue;
	}

	/*
	 * カーソルがテーブルの最後に来た(もう該当するレコード無し)
	 */
	resp->f_more	= 0;


	/*
	 * リプライ
	 */
reply:
	len = sizeof(r_res_find_t) + n*td->t_rec_size;

	reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

	resp->f_num	= n;

	*respp		= resp;
	*res_lenp	= len;
DBG_EXIT(0);
	return( 0 );

record_lock:

	/*
	 * 排他がかかっていて、参照や排他ができない時、エラーとなる
	 * 既にレコードを取得したら、そこまでを送信する、追加(f_more)
	 * を行わせる
	 */
	if ( n ) {
		resp->f_more	= 1;
		goto	reply;
	} else 
		neo_errno = E_RDB_REC_BUSY;

err2: DBG_T("err2");
	neo_free( resp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	prc_rewind()
 *
 *  機能	カーソルを先頭に戻す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
prc_rewind( reqp, resp )
	r_req_rewind_t	*reqp;
	r_res_rewind_t	*resp;
{
	r_res_rewind_t	res;
	r_tbl_t		*td;
	r_usr_t		*ud;
DBG_ENT(M_RDB,"prc_rewind");

#ifdef XXX
	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, &req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}
#endif

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	td = reqp->r_td;
	ud = reqp->r_ud;

	if( (neo_errno = check_td_ud( td, ud )) ) {
		goto err1;
	}

	/*
	 * カーソルを0に設定
	 */
DBG_A(("cursor=%d\n", ud->u_cursor ));
	ud->u_cursor	= 0;

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)reqp, (p_head_t*)resp, sizeof(res), 0 );

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}
