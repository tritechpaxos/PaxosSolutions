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
 *	svc_sort.c
 *
 *	説明	R_CMD_SORT   procedure
 *				
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_sort()
 *
 *  機能	クライアント側指定したテーブルをソートし、応答を返す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_sort( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子アドレス */
	p_head_t	*hp;		/* 要求パケットヘッダ	    */
{
	int	len;
	r_req_sort_t	*reqp;
	r_res_sort_t	res;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i, j;
	r_sort_index_t	*abs, *rel;
	int		num;

DBG_ENT(M_RDB,"svc_sort");

	/*
	 * 要求パケット受信用バッファエリアを生成する
	 */
	len = hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_req_sort_t*)neo_malloc(len) ) ) {

		len = sizeof(p_head_t);
		if( p_recv( pd, hp, &len ) )
			goto err1;
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

	/*
	 * 指定したテーブルとオープンユーザの有効チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->s_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->s_name );
	HashClear( &td->t_HashEqual );

	if( (neo_errno = check_td_ud( td, ud ))
			|| (neo_errno = check_lock( td, ud, R_WRITE ) ) ) {
		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err2;
	}

	/*
	 * 指定したテーブルをアクセスしている他ユーザが
	 * 存在すれば、ソートできない
	 */
	if( td->t_usr_cnt > 1 ) {
		err_reply( pd, hp, E_RDB_TBL_BUSY );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err2;
	}

	/*
	 * ソート用レコード分の Indexテーブルエリア二つを生成する
	 */
	if( !(abs = (r_sort_index_t*)
		neo_malloc( td->t_rec_num*sizeof(r_sort_index_t))) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err2;
	}

	if( !(rel = (r_sort_index_t*)
		neo_malloc( td->t_rec_num*sizeof(r_sort_index_t))) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err3;
	}

	/*
	 * レコード情報を上記 Index テーブルエリアに設定する
	 */
	for( i = 0, num = 0; i < td->t_rec_num; i++ ) {

		/*
		 * キャッシュ化されていないレコードをファイル
		 * からロードする
		 */
		if( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 );

		if( td->t_rec[i].r_stat & R_USED ) {
			abs[num].i_head 
				= rel[num].i_head = td->t_rec[i].r_head;
			rel[num].i_id	= num;
			num++;
		}
	}

	/*
	 * テーブルをソートする
	 * ソートは、まずソート分析器を作成して、Index テーブルに対して
	 * 行う。ソート後 ソート済みIndex テーブルによって、実際
	 * メモリ上のレコードを入れ替えて、昇順(降順)に並べる
	 */
	if( num ) {
		if( (neo_errno = _rdb_sort_open( td,  reqp->s_n, (r_sort_t*)(reqp+1) ) )
			|| (neo_errno = _rdb_sort_start( rel, num ))
			|| (neo_errno = _rdb_sort_close() )
			|| (neo_errno = 
				_rdb_arrange_data(abs,rel,num,td->t_rec_size))){

			err_reply( pd, hp, neo_errno );

			goto err4;
		}
		/*
		 * レコードの状態を更新する
		 */
		for( i = 0; i < num; i++ ) {

DBG_A(("<%i->%i,%d[%s]>\n", i, abs[i].i_id, abs[i].i_head->r_abs, 
		abs[i].i_head+1 ));

			if( abs[i].i_id == i )
				continue;

			j = abs[i].i_head->r_abs;
			td->t_rec[j].r_stat 	|= R_UPDATE;
			abs[i].i_head->r_stat	|= R_UPDATE;

			td->t_rec[j].r_stat	|= R_BUSY;
			td->t_rec[i].r_access	= ud;
		}
	}
	td->t_version++;

	/*
	 * 応答パケットを作成し、クライアント側へ送る
	 */
	reply_head( (p_head_t*)reqp, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err4;
	}

	/*
	 * 要求パケット受信用エリア、ソート用臨時
	 * Index エリアを解放する
	 */
	neo_free( rel );
	neo_free( abs );
	neo_free( reqp );

DBG_EXIT(0);
	return( 0 );

err4: DBG_T("err4");
	neo_free( rel );
err3: DBG_T("err3");
	neo_free( abs );
err2: DBG_T("err2");
	neo_free( reqp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
