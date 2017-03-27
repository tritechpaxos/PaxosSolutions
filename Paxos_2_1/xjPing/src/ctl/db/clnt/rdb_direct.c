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
 *	rdb_direct.c
 *
 *	説明	DB_MAN library R_CMD_DIRECT
 *				レコード絶対番号でレコードを取得する
 * 
 ******************************************************************************/

#include	"neo_db.h"

/******************************************************************************
 * 関数名
 *		rdb_direct()
 * 機能        
 *		レコード絶対番号でレコードを取得する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
rdb_direct( td, abs, bufp, mode )
	r_tbl_t		*td;	/* I テーブル記述子	*/
	int		abs;	/* I レコード絶対番号	*/
	char		*bufp;	/* O レコードバッファ	*/
	int		mode;	/* I 排他モード		*/
{
	p_id_t		*pd;
	r_req_direct_t	req;
	r_res_direct_t	*resp;
	r_res_direct_t	head;
	int		len;

DBG_ENT(M_RDB,"rdb_direct");

	/*
	 *	引数チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	if( abs < 0 ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( mode & ~(R_EXCLUSIVE|R_SHARE|R_WAIT|R_NOWAIT) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	if( (mode & R_EXCLUSIVE) && (mode & R_SHARE) ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	/*
	 *	レスポンス用バッファのメモリアロック
	 *	(R_CMD_DIRECT パケットは可変長)
	 */
	len = sizeof(r_res_direct_t) + td->t_rec_size;
	if( !(resp = (r_res_direct_t *)neo_malloc( len )) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 *	リクエストパケットの情報設定
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DIRECT, sizeof(req) );
	
	strncpy( req.d_name, td->t_name, sizeof(req.d_name) );
	req.d_abs	= abs;
	req.d_mode	= mode;

	/*
	 *	リクエスト
	 */
again:
	if( p_request( pd = TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err2;

	/*
	 *	レスポンス(R_CMD_DIRECT パケットは可変長)
	 */
	if( p_peek( pd, (p_head_t*)&head, sizeof(head), P_WAIT ) < 0 )
		goto err1;
	if( (neo_errno = head.d_head.h_err) ) {

		len = sizeof(head);
		p_response( pd, (p_head_t*)&head, &len );

		if( (neo_errno != E_RDB_REC_BUSY) || (mode&R_NOWAIT)  )
			goto err2;

		/*
		 *	排他できない時は、レコードイベントを待つ
		 */
		if( rdb_event( td, R_EVENT_REC, abs ) )
			goto err2;
		goto again;
	}

	len = sizeof(r_res_direct_t) + td->t_rec_size;
	if( p_response( pd, (p_head_t*)resp, &len ) )
		goto	err2;

	/*
	 *	データコピー
	 */
	bcopy( resp+1, bufp, td->t_rec_size );

	neo_free( resp );
	
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
