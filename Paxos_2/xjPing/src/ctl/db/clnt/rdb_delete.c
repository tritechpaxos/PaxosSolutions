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
 *	rdb_delete.c
 *
 *	説明	R_CMD_DELETE
 *				レコードを削除する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/******************************************************************************
 * 関数名
 *		rdb_delete()
 * 機能        
 *		レコードを削除する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 *		現版では、一度に削除できるレコード数は 1 レコードのみである
 ******************************************************************************/
int
rdb_delete( td, n, rec, mode )
	r_tbl_t		*td;	/* テーブル記述子	*/
	int		n;	/* レコード数		*/
	r_head_t	*rec;	/* レコード		*/
	int		mode;	/* 待ちモード		*/
{
	r_req_delete_t	*pReq;
	r_res_delete_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_delete");

	/*
	 *	引数チェック
	 */
	if( n <= 0 || (mode & ~(R_WAIT|R_NOWAIT)) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	/*
	 * 要求パケット用バッファエリアを生成する
	 */
	len = sizeof(r_req_delete_t) + n * td->t_rec_size ;
	if( !(pReq = (r_req_delete_t*)neo_malloc(len) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 *	リクエスト情報の設定
	 */
	_rdb_req_head( (p_head_t*)pReq, R_CMD_DELETE, len );
	
	strncpy( pReq->d_name, td->t_name, sizeof(pReq->d_name) );
	bcopy( rec, pReq+1, n * td->t_rec_size );

	/*
	 *	リクエスト
	 */
again:
	if( p_request( TDTOPD(td), (p_head_t*)pReq, len ) )
		goto	err2;

	/*
	 *	レスポンス
	 */
	len = sizeof(r_res_delete_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len ) )
		goto	err2;

	neo_errno = res.d_head.h_err;
	if( neo_errno ) {
		if( neo_errno != E_RDB_REC_BUSY
			|| mode & R_NOWAIT ) {

			goto err2;

		} else {
			/*
			 *	R_WAIT で削除できなかったので、
			 *	レコードイベント待ち
			 */
			if( rdb_event( td, R_EVENT_REC, rec->r_abs ) )
				goto err2;

			goto again;
		}
	}
	td->t_rec_used	= res.d_rec_used;

	neo_free( pReq );
DBG_EXIT(0);
	return( 0 );
err2:
	neo_free( pReq );
err1:
DBG_EXIT(-1);
	return( -1 );
}
