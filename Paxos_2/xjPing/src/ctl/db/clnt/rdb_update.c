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
 *	rdb_update.c
 *
 *	説明		レコードを更新する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_update()
 *
 *  機能	レコードを更新する
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_update( td, n, rec, mode )
	r_tbl_t		*td;		/* テーブル記述子	*/
	int		n;		/* 更新するレコード数	*/
	r_head_t	*rec;		/* 更新用新レコード	*/
	int		mode;		/* モード		*/
{
	r_req_update_t	*reqp;
	r_res_update_t	res;
	int		len, len1;

DBG_ENT(M_RDB,"rdb_update");

	/*
	 * テーブル有効性チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	if ( ( n <= 0 )  || ( mode & ~(R_LOCK|R_WAIT|R_NOWAIT) ) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	/*
	 * 要求パケット用バッファエリアを生成する
	 */
	len = sizeof(r_req_update_t) + n * td->t_rec_size ;
	if( !(reqp = (r_req_update_t*)neo_malloc(len) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * 要求パケットを作成する
	 */
	_rdb_req_head( (p_head_t*)reqp, R_CMD_UPDATE, len );
	
	strncpy( reqp->u_name, td->t_name, sizeof(reqp->u_name) );
	reqp->u_mode	= mode;
	reqp->u_numbers	= n;

	bcopy( rec, reqp+1, n * td->t_rec_size );

	/*
	 * 要求パケットを送信する
	 */
again:
	if( p_request( TDTOPD(td), (p_head_t*)reqp, len ) )
		goto	err2;

	/*
	 * 応答パケットを受信まつ
	 */
	len1 = sizeof(r_res_update_t);
	if( p_response( TDTOPD(td), (p_head_t*)&res, &len1 ) )
		goto	err2;
	
	/*
	 * 応答パケットのヘッダ部エラー番号のチェック
	 */
	neo_errno = res.u_head.h_err ;

	if( neo_errno ) {

		/*
	  	 * 更新対象レコードがロック中かつモード引数で
	  	 * NOWAITを指定した場合には、エラーで戻る
		 */
		if( neo_errno != E_RDB_REC_BUSY
			|| mode & R_NOWAIT ) {

			goto err2;

		/*
		 * 再試行処理 
		 */
		} else {
			if( rdb_event( td, R_EVENT_REC, res.u_rec.r_abs ) )
				goto err2;

			goto again;
		}
	}

	/*
	 *	record to user
	 */

	/***
	bcopy(&res.u_rec, rec, sizeof(r_head_t) );
	***/
	neo_free( reqp );
DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_free( reqp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
