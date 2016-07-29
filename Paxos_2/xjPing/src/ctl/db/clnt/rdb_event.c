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
 *	rdb_event.c
 *
 *	説明	イベント同期
 * 
 ******************************************************************************/

#include	"neo_db.h"

#define	RETRY	1	/*1000*/
#define	EVER		/* i++ */

/*****************************************************************************
 *
 *  関数名	rdb_event()
 *
 *  機能	イベント同期
 * 
 *  関数値
 *     		正常	 0
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_event( td, class, code )
	r_tbl_t	*td;		/* テーブル記述子		   */
	int	class;		/* イベントクラス(レコード待ちのみ)*/
	int	code;		/* イベントコード(レコード絶対番号)*/
{
	r_req_event_t		req;
	r_res_event_t		res;
	int			len;
	r_req_event_cancel_t	creq;
	r_res_event_cancel_t	cres;
	int			i;

DBG_ENT(M_RDB,"rdb_event");

	/*
	 * テーブル記述子有効チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 * 要求パケットを作成し、サーバ側に送る
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_EVENT, sizeof(req) );
	strncpy( req.e_name, td->t_name, sizeof(req.e_name) );
	req.e_class	= class;
	req.e_code	= code;
	if( p_request( TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err1;

	/*
	 * イベント同期発生するまで、あるいはある
	 * 決まった時間帯内(RETRY * p_max_time)、応答を待つ
	 */
	for( i = 0; i < RETRY; EVER ) {

		len = sizeof(r_res_event_t);
		if( p_response( TDTOPD(td), (p_head_t*)&res, &len ) == 0 ) {
DBG_A(("p_response(0),h_err=0x%x\n",res.e_head.h_err ));
			if( (neo_errno = res.e_head.h_err) )
				goto err1;
			else
				goto ok;
		} else {
DBG_A(("neo_errno(0x%x)\n", neo_errno ));
			if( neo_errno == E_LINK_TIMEOUT )
				continue;
			else if ( neo_errno == E_SKL_TERM )
				goto	cancel;
			else
				goto err1;
		}
	}

	neo_errno	= E_RDB_RETRY;
	/*
	 * 決まった時間帯内応答が来ていなかったならば、
	 * イベント同期要求をキャンセルする
	 */
DBG_A(("cancel event\n"));

cancel:
	/*
	 * キャンセル要求パケット作成、送信
	 */

	_rdb_req_head( (p_head_t*)&creq, R_CMD_EVENT_CANCEL, sizeof(creq) );
	strncpy( creq.e_name, td->t_name, sizeof(creq.e_name) );
	creq.e_class	= class;
	creq.e_code	= code;
	if( p_request( TDTOPD(td), (p_head_t*)&creq, sizeof(creq) ) )
		goto err1;

	/*
	 * キャンセル要求の応答を受信待つ
	 * ( キャンセルの時点で前のイベント同期要求の
	 *   応答が返されてくる場合を想定する)
	 */
	do {
		len = sizeof(cres);
		if( p_response( TDTOPD(td), (p_head_t*)&cres, &len ) )
			goto err1;

	} while( cres.e_head.h_cmd != R_CMD_EVENT_CANCEL );


	goto err1;
ok:
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}
