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
 *	rdb_flush.c
 *
 *	説明		更新データをディスクに書き出す
 *			       R_CMD_CANCEL
 *				更新データを元のデータに戻す
 * 
 ******************************************************************************/

#include	"neo_db.h"

/******************************************************************************
 * 関数名
 *		rdb_flush()
 * 機能        
 *		更新データをディスクに書き出す
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_flush( td )
	r_tbl_t	*td;	/* テーブル記述子	*/
{
	r_req_flush_t	req;
	r_res_flush_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_flush");

	/*
	 *	引数チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 *	リクエスト
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_FLUSH, sizeof(req) );

	strncpy( req.f_name, td->t_name, sizeof(req.f_name) );

	if( p_request( TDTOPD( td ), (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	/*
	 *	レスポンス
	 */
	len = sizeof(res);
	if( p_response( TDTOPD( td ), (p_head_t*)&res, &len )
			|| (neo_errno = res.f_head.h_err) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}

/******************************************************************************
 * 関数名
 *		rdb_cancel()
 * 機能        
 *		更新データを元のデータに戻す
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_cancel( td )
	r_tbl_t	*td;	/* テーブル記述子	*/
{
	r_req_cancel_t	req;
	r_res_cancel_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_cancel");

	/*
	 *	引数チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	/*
	 *	リクエスト
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_CANCEL, sizeof(req) );

	strncpy( req.c_name, td->t_name, sizeof(req.c_name) );

	if( p_request( TDTOPD( td ), (p_head_t*)&req, sizeof(req) ) )
		goto err1;

	/*
	 *	レスポンス
	 */
	len = sizeof(res);
	if( p_response( TDTOPD( td ), (p_head_t*)&res, &len )
			|| (neo_errno = res.c_head.h_err) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}

