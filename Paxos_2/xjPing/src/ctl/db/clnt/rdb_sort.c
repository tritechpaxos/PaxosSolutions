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
 *	rdb_sort.c
 *
 *	説明	 	テーブルをソートする
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_sort()
 *
 *  機能	テーブルのレコードをソートする
 * 
 *  関数値
 *     		正常	 0
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_sort( td, n, sort )
	r_tbl_t		*td;		/* ソートするテーブル記述子 */
	int		n;		/* ソート用キーの数	    */
	r_sort_t	sort[];		/* ソート用キー		    */
{
	r_item_t	*ip;
	int		i, j;
	r_req_sort_t	*reqp;
	r_res_sort_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_sort");

	/*
	 * テーブル記述子有効性チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * ソート用キーの数を100以内に限定する
	 */
	if( n < 0 || td->t_scm->s_n < n ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	/*
	 * 各ソート用キーのアイテム名とソート順序指定をチェックする
	 */
	for( j = 0; j < n ; j++ ) {
		if( sort[j].s_order != R_ASCENDANT 
			&& sort[j].s_order != R_DESCENDANT ) {

			neo_errno = E_RDB_INVAL;
			goto err1;
		}
		for( i = 0; i < td->t_scm->s_n; i++ ) {
			ip = &td->t_scm->s_item[i];
			if( !strcmp( sort[j].s_name, ip->i_name ) )
				goto find;
		}
		neo_errno = E_RDB_NOITEM;

		goto err1;
	find:
		continue;
	}

	/*
	 * 要求パケット用エリアを生成する
	 */
	len = sizeof(r_req_sort_t) + n*sizeof(r_sort_t);
	if( !(reqp = (r_req_sort_t *)neo_malloc(len) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * 要求パケットを作成し、サーバへ送る
	 */
	_rdb_req_head( (p_head_t*)reqp, R_CMD_SORT, len );

	strncpy( reqp->s_name, td->t_name, sizeof(reqp->s_name) );
	reqp->s_n 	= n;
	bcopy( sort, reqp+1, n*sizeof(r_sort_t) );

	if( p_request( TDTOPD(td), (p_head_t*)reqp, len ) )
		goto err2;

	/*
	 * 応答パケット受信待ち
	 */
	len = sizeof(r_res_sort_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len )
				||( neo_errno = res.s_head.h_err) )
		goto err2;

	/*
	 * 要求パケット用エリアを解放する
	 */
	neo_free( reqp );

DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( reqp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
