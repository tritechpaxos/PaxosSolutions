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
 *	rdb_compress.c
 *
 *	説明	db_man library   R_CMD_COMPRESS
 *				 テーブルを圧縮する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_compress()
 *
 *  機能	指定したテーブルを圧縮する
 * 
 *  関数値
 *     		正常 	0
 *		異常	-1	
 *
 ******************************************************************************/
int
rdb_compress( td )
	r_tbl_t		*td;		/* 圧縮するテーブルの記述子 */
{
	r_req_compress_t	req;
	r_res_compress_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_compress");

	/*
	 * テーブル記述子有効性のチェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * 要求パケットを作成し、送信する
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_COMPRESS, sizeof(req) );

	strncpy( req.c_name, td->t_name, sizeof(req.c_name) );

	if( p_request( TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	/*
	 * 応答パケットを受信まつ
	 */
	len = sizeof(r_res_compress_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len )
				||( neo_errno = res.c_head.h_err) )
		goto err1;

	/*
	 * テーブル記述子の相手情報とレコード数情報を更新する
	 */
	td->t_rec_num	= res.c_rec_num;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}
