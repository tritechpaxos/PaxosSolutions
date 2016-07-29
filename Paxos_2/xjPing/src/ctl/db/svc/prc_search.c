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


#include	"neo_db.h"
#include	"svc_logmsg.h"

r_tbl_t		*_search_tbl;
r_usr_t		*_search_usr;


int
prc_search( reqp, resp )
	r_req_search_t	*reqp;
	r_res_search_t	*resp;
{
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i;
	int		n;

	/* 解析木のルートへのポインタ(in clnt/rdb_gram.c) */
	extern r_expr_t	*_Rdb_yyreturn; 

DBG_ENT(M_RDB,"prc_search");

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	td = reqp->s_td;
	ud = reqp->s_ud;

	n = reqp->s_n;

	if( (neo_errno = check_td_ud( td, ud )) ) {
		goto err1;
	}

	/*
	 * 検索条件の設定
	 *  ( lex_start(), yyparse()は clnt/rdb_expr.c, clnt/rdb_gram.c)
	 */
	if( n ) {
		_search_tbl	= td;
		_search_usr	= ud;

		_search_usr->u_search.s_cnt	= 0;

DBG_A(("str=[%s]\n", reqp+1 ));
		
		/*
		 * 字句解析処理前準備
		 */
		lex_start( (char*)(reqp+1), _search_usr->u_search.s_text,
				 sizeof(_search_usr->u_search.s_text) );


		/*
		 * 構文解析木作成
		 *   yyparse() のリターン値は、正常時は 0 、異常時は 1
		 *   _Rdb_yyreturn は、正常に yyparse() が終了した時に、
		 *   解析木のルートへのポインタが設定されます。
		 */
		if( yyparse() ) {

			neo_errno = E_RDB_SEARCH;
			goto err1;
		}

		_search_usr->u_search.s_expr	= _Rdb_yyreturn;
		_search_usr->u_cursor		= 0;

	} else {
		ud->u_search.s_expr	= 0;
		ud->u_cursor		= 0;
	}

	/*
	 * 排他をかけていたレコードのロックを解除する
	 * その際に、待ちユーザがあれば通知する
	 */
	for( i = 0; i < td->t_rec_num; i++ ) {
		if( (td->t_rec[i].r_access == ud)
			&& (td->t_rec[i].r_stat & R_LOCK) ) {

			td->t_rec[i].r_stat	&= ~R_LOCK;

			svc_rec_post( td, i );
		}
	}

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)reqp, (p_head_t*)resp, sizeof(*resp), 0 );

DBG_EXIT(0);
	return( 0 );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
