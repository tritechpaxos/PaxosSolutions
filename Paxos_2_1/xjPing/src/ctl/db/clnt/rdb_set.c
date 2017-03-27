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
 *	rdb_set.c
 *
 *	説明	  アイテム値の設定と参照
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_set()
 *
 *  機能	レコードの指定アイテム値の設定
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_set( td, rp, item, data )
	r_tbl_t		*td;		/* テーブル記述子		*/
	r_head_t	*rp;		/* 設定先レコードバッファ	*/ 
	char		*item;		/* 設定先アイテム名		*/
	char		*data;		/* 設定用データ値		*/
{
	r_item_t	*ip;
	int		i;
	char	*rec = (char*)rp;

DBG_ENT(M_RDB,"rdb_set");

	/*
	 * テーブル有効性チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * 指定アイテムをスキーマの中に捜し出し、
	 * レコード内のオフセット値を取得する
	 */
	for( i = 0; i < td->t_scm->s_n; i++ ) {
		ip = &td->t_scm->s_item[i];
		if( !strcmp( item, ip->i_name ) )
			goto find;
	}
	neo_errno = E_RDB_NOITEM;

	goto err1;
find:
	/*
	 * データをレコードに設定する
	 */
	bcopy( data, rec + ip->i_off, ip->i_len ); 

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	rdb_get()
 *
 *  機能	レコードのあるスキーマ値を取得する
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_get( td, rp, item, data, lenp )
	r_tbl_t		*td;		/* テーブル記述子	*/
	r_head_t	*rp;		/* レコードバッファ	*/
	char		*item;		/* アイテム名		*/
	char		*data;		/* アイテム地格納エリア	*/
	int		*lenp;		/* 取得長		*/
{
	r_item_t	*ip;
	int		i;
	char	*rec = (char*)rp;

DBG_ENT(M_RDB,"rdb_get");

	/*
	 * テーブル有効性チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * 指定したアイテムをスキーマの中から捜し出し、
	 * レコード内のオフセット値を取得する
	 */
	for( i = 0; i < td->t_scm->s_n; i++ ) {
		ip = &td->t_scm->s_item[i];
		if( !strcmp( item, ip->i_name ) )
			goto find;
	}
	neo_errno = E_RDB_NOITEM;

	goto err1;
find:
	/*
	 * レコードの該当スキーマ値を取り出す
	 */
	bcopy( rec + ip->i_off, data, *lenp = ip->i_len ); 

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}
