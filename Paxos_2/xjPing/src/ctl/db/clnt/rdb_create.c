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
 *	rdb_create.c
 *
 *	説明	db_man library R_CMD_CREATE
 *				テーブルを作成する 
 *			       R_CMD_DROP
 *				テーブルを削除する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/******************************************************************************
 * 関数名
 *		rdb_create()
 * 機能        
 *		テーブルを作成する
 * 関数値
 *      正常: テーブル記述子
 *      異常: 0
 * 注意
 ******************************************************************************/
r_tbl_t	*
rdb_create( md, name, total, n, items )
	r_man_t		*md;		/* DB_MAN 記述子	*/
	r_tbl_name_t	name;		/* テーブル名		*/
	int		total;		/* 総レコード数		*/
	int		n;		/* 総アイテム数		*/
	r_item_t	items[];	/* スキーマ		*/
{
	r_tbl_t		*td;
	r_req_create_t	*reqp;
	r_res_create_t	*resp;
	int		lenq, lens;
	r_scm_t		*sp;
	int		i, j;

DBG_ENT(M_RDB,"rdb_create");

	/*
	 * 	引数 チェック
	 */
	if( (total < R_TBL_REC_MIN) /* 1 */ || (n <= 0) || 
			(strlen(name) >= R_TABLE_NAME_MAX) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	/* アイテム名の重複	*/
	for( i = 0; i < n; i++ ) {
		for( j = i + 1; j < n; j++ ) {
			if( !strncmp( items[i].i_name, items[j].i_name,
						sizeof(r_item_name_t) ) ) {
				neo_errno = E_RDB_EXIST;
				goto err1;
			}
		}
	}
	for( i = 0; i < n; i++ ) {
		if( (items[i].i_len <= 0)
			|| ( strlen(items[i].i_name) >= R_ITEM_NAME_MAX )
			|| ( items[i].i_attr < R_HEX )
			|| ( R_ULONG < items[i].i_attr ) ) {

			neo_errno = E_RDB_INVAL;

			goto err1;
		}
		items[i].i_size	= 
		    ((items[i].i_len + sizeof(int)-1)/sizeof(int))*sizeof(int);
	}
	/* テーブル名の重複	*/
	for( td = (r_tbl_t *)_dl_head(&md->r_tbl);
		_dl_valid(&md->r_tbl,td); td = (r_tbl_t *)_dl_next(td) ) {
		if( !strncmp( name, td->t_name, sizeof(r_tbl_name_t) ) ) {
			neo_errno = E_RDB_EXIST;
			goto err1;
		}
	}

	/*
	 *	テーブル構造体 メモリアロック
	 */
	if( !(td = _r_tbl_alloc() ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}
	/*
	 *	スキーマ構造体 メモリアロック
	 */
	if( !(sp = (r_scm_t*)neo_malloc(sizeof(r_scm_t)
				+ (n - 1)*sizeof(r_item_t))) ) {
		neo_errno = E_RDB_NOMEM;
		goto err2;
	}

	/*
	 *	リクエスト 構造体 メモリアロック
	 *		( R_CMD_CREATE はパケットサイズが可変長だから )
	 */
	lenq	= sizeof(r_req_create_t) + (n-1)*sizeof(r_item_t);
	if( !(reqp = (r_req_create_t *)neo_malloc( lenq )) ) {
		neo_errno = E_RDB_NOMEM;
		goto err3;
	}
	/*
	 *	レスポンス構造体 メモリアロック
	 *		( R_CMD_CREATE はパケットサイズが可変長だから )
	 */
	lens	= sizeof(r_res_create_t) + sizeof(r_item_t)*n;
	if( !(resp = (r_res_create_t*)neo_malloc( lens )) ) {
		neo_errno = E_RDB_NOMEM;
		goto err4;
	}

	/*
	 *	リクエスト内容の設定
	 */
	_rdb_req_head( (p_head_t*)reqp, R_CMD_CREATE, lenq );
	bzero( reqp->c_name, sizeof(reqp->c_name) );
	strncpy( reqp->c_name, name, sizeof(reqp->c_name) );
	reqp->c_td		= td;
	reqp->c_rec_num		= total;
	reqp->c_item_num	= n;
	bcopy( items, reqp->c_item, n*sizeof(r_item_t) );

	/*
	 *	リクエスト
	 */
	if( p_request( md->r_pd, (p_head_t*)reqp, lenq ) )
		goto err5;
	/*
	 *	レスポンス
	 */
	if( p_response( md->r_pd, (p_head_t*)resp, &lens )
			||(neo_errno = resp->c_head.h_err) )
		goto err5;

	/*
	 *	クライアント側のテーブル情報の設定
	 */
	sp->s_n	= resp->c_item_num;
	bcopy( resp+1, sp->s_item, resp->c_item_num*sizeof(r_item_t) );

	strncpy( td->t_name, name, sizeof(td->t_name) );
	td->t_md	= md;
	td->t_rtd	= resp->c_td;
	td->t_rud	= resp->c_ud;
	td->t_rec_size	= resp->c_rec_size;
	td->t_rec_num	= resp->c_rec_num;
	td->t_scm	= sp;

	/*
	 *	DB_MAN 記述子へのリンク
	 */
	_dl_insert( td, &md->r_tbl );

	/*
	 *	通信バッファの解放
	 */
	neo_free( reqp );
	neo_free( resp );

DBG_EXIT(td);
	return( td );

err5: DBG_T("err5");
	neo_free( resp );
err4: DBG_T("err4");
	neo_free( reqp );
err3: DBG_T("err3");
	neo_free( sp );
err2: DBG_T("err2");
	neo_free( td );
err1:
DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		rdb_drop()
 * 機能        
 *		テーブルを削除する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_drop( md, name )
	r_man_t		*md;		/* DB_MAN 記述子	*/
	r_tbl_name_t	name;		/* テーブル名		*/
{
	r_tbl_t		*td;
	r_req_drop_t	req;
	r_res_drop_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_drop");

	/*
	 * 	引数チェック
	 */
	if( (strlen(name) >= R_TABLE_NAME_MAX) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	for( td = (r_tbl_t *)_dl_head(&md->r_tbl);
		_dl_valid(&md->r_tbl,td); td = (r_tbl_t *)_dl_next(td) ) {
		if( !strncmp( name, td->t_name, sizeof(r_tbl_name_t) ) ) {
			neo_errno = E_RDB_TBL_BUSY;
			goto err1;
		}
	}
	/*
	 *	リクエスト内容の設定
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DROP, sizeof(req) );
	bzero( req.d_name, sizeof(req.d_name) );
	strncpy( req.d_name, name, sizeof(req.d_name) );

	/*
	 *	リクエスト
	 */
	if( p_request( md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	/*
	 *	レスポンス
	 */
	len	= sizeof(res);
	if( p_response( md->r_pd, (p_head_t*)&res, &len )
						||(neo_errno = res.d_head.h_err) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}
