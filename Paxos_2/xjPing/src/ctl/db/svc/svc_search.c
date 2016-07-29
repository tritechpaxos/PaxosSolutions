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
 *	svc_search.c
 *
 *	説明	R_CMD_SEARCH	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

r_tbl_t		*_search_tbl;
r_usr_t		*_search_usr;


/*****************************************************************************
 *
 *  関数名	svc_search()
 *
 *  機能	検索条件の設定
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_search( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int	len;
	r_req_search_t	*reqp;
	r_res_search_t	res;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i;
	int		n;

	/* 解析木のルートへのポインタ(in clnt/rdb_gram.c) */
	extern r_expr_t	*_Rdb_yyreturn; 

DBG_ENT(M_RDB,"svc_search");

	/*
	 * リクエストレシーブ
	 *	( R_CMD_SEARCH の要求パケットが可変長)
	 */
	len = hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_req_search_t*)neo_malloc(len) ) ) {
		len = sizeof(p_head_t);
		if( p_recv( pd, hp, &len ) ) {	/* discard */
			if( neo_errno != E_LINK_BUFSIZE )
				goto err1;
		}
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;

	}

	if( p_recv( pd, (p_head_t*)reqp, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err2;
	}

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->s_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->s_name );

	n = reqp->s_n;

	if( (neo_errno = check_td_ud( td, ud )) ) {
		err_reply( pd, hp, neo_errno );
		goto err2;
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

			err_reply( pd, (p_head_t*)reqp, E_RDB_SEARCH );
			goto err2;
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
	reply_head( (p_head_t*)reqp, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}

	/*
	 * 通信バッファのメモリ解放
	 */
	neo_free( reqp );
DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_free( reqp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
