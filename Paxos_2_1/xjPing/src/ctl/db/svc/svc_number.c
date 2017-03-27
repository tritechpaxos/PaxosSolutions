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
 *	svc_number.c
 *
 *	説明	R_CMD_REC_NUMS	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_number()
 *
 *  機能	レコード総数とUSED数情報を取得する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_number( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int			len;
	r_req_rec_nums_t	req;
	r_res_rec_nums_t	res;
	r_tbl_t			*td;
	r_usr_t			*ud;

DBG_ENT(M_RDB,"svc_recnums");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.r_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.r_name );

	if( (neo_errno = check_td_ud( td, ud )) ) {
		err_reply( pd, hp, neo_errno );
		goto err1;
	}

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );
	res.r_total	= td->t_rec_num;
	res.r_used	= td->t_rec_used;

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err1;
	}
	
DBG_EXIT(0);
	return( 0 );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
