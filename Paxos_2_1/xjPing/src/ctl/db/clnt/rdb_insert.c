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
 *	rdb_insert.c
 *
 *	説明                 レコードを挿入する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_insert()
 *
 *  機能	レコードをテーブルに挿入する
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 *  注意
 *		X 1. 本版では挿入するレコードを一個に限定する
 *		2. m items指定した場合には、レコードの重複
 *		 登録のチェックを行う
 *
 ******************************************************************************/
int
rdb_insert( td, n, rec, m, items, mode )
	r_tbl_t		*td;		/* 挿入先のテーブル記述子	*/
	int		n;		/* 挿入したいレコード数  	*/
	r_head_t	*rec;		/* 挿入するレコードの格納領域	*/
	int		m;		/* チェックするアイテム個数 	*/
	r_item_name_t	items[];	/* チェックするアイテム名	*/
	int		mode;		/* モード:R_SHARE,  R_EXCLUSIVE */
{
	r_req_insert_t	*reqp;
	r_res_insert_t	res;
	int		len;
	int		i, j;

DBG_ENT(M_RDB,"rdb_insert");

	/*
	 * テーブル記述子有効性チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * 引数 m, n のチェック
	 * ( 本版では、一回挿入レコード数nを一に限定する
	 */
	if( n < 0 || m < 0 || td->t_scm->s_n < m ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	if ( mode & ~R_LOCK ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	/*
	 * アイテム名のチェック(mを指定した場合)
	 */
	if( m ) {
	  for( j = 0; j < m; j++ ) {
		for( i = 0; i < td->t_scm->s_n; i++ ) {
			if( !strcmp( items[j], td->t_scm->s_item[i].i_name ) ) {
				goto next;
			}
		}
		neo_errno = E_RDB_NOITEM;
		goto err1;
	  next:;
	  }
	}

	/*
	 * 要求パケット用バッファを生成する
	 */
	len = sizeof(r_req_insert_t) 
		+ n*td->t_rec_size 
		+ m*sizeof(r_item_name_t);
	if( !(reqp = (r_req_insert_t*)neo_malloc(len) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * 要求パケットを作成し、サーバへ送る
	 */
	_rdb_req_head( (p_head_t*)reqp, R_CMD_INSERT, len );
	
	strncpy( reqp->i_name, td->t_name, sizeof(reqp->i_name) );
	reqp->i_mode	= mode;
	reqp->i_rec	= n;
	reqp->i_item	= m;

	bcopy( rec, reqp+1, n*td->t_rec_size );
	if( m )
		bcopy( items, (char*)(reqp+1) + n*td->t_rec_size, 
						m*sizeof(r_item_name_t) );
	if( p_request( TDTOPD(td), (p_head_t*)reqp, len ) )
		goto	err2;

	/*
	 * 応答パケットを受信まつ
	 */
	len = sizeof(r_res_insert_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len ) )
		goto	err2;
	
	if( (neo_errno = res.i_head.h_err) )
		goto err2;

	/*
	 * ローカルのテーブルのレコード数情報を更新する
	 */
	td->t_rec_num	= res.i_rec_num;
	td->t_rec_used	= res.i_rec_used;
	bcopy( &res.i_rec, rec, sizeof(r_head_t) );

	neo_free( reqp );
DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_free( reqp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
