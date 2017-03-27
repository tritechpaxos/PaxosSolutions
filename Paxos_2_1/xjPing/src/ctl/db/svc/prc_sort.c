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
#include	"../sql/sql.h"

r_expr_t	*sql_getnode();

int
prc_view_sort( td, orderp )
	r_tbl_t		*td;
	r_expr_t	*orderp;
{
	int		i, j;
	r_sort_index_t	*abs, *rel;
	int		num;
	int		snum;
	r_expr_t	*wp;
	r_sort_t	*sp;

DBG_ENT(M_RDB,"prc_view_sort");

	if( td->t_rec_num == 0 ) {
		DBG_EXIT( 0 );
		return( 0 );
	}
	HashClear( &td->t_HashEqual );

	if( sql_getnode( orderp, 0, '.' ) ) {
		DBG_EXIT( 0 );
		return( 0 );
	}
	for( snum = 0, wp = 0; (wp = sql_getnode( orderp, wp, 'O' )); snum++ );
	if( snum <= 0 ) {
		DBG_EXIT( 0 );
		return( 0 );
	}
	if( !(sp = (r_sort_t*)neo_malloc( snum*sizeof(r_sort_t))) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}
	wp = 0;
	i  = 0;
	while( (wp = sql_getnode( orderp, wp, 'O' )) ) {
		j = wp->e_tree.t_binary.b_l->e_tree.t_unary.v_value.v_int;
		strcpy( (char*)&sp[i].s_name, 
				(char*)&td->t_scm->s_item[j].i_name );
		if( wp->e_tree.t_binary.b_r->e_tree.t_unary.v_value.v_int
			== SASC ) {
			sp[i].s_order = R_ASCENDANT;
		} else {
			sp[i].s_order = R_DESCENDANT;
		}
		i++;
	}
	/*
	 * ソート用レコード分の Indexテーブルエリア二つを生成する
	 */
	if( !(abs = (r_sort_index_t*)
		neo_malloc( td->t_rec_num*sizeof(r_sort_index_t))) ) {
		neo_errno = E_RDB_NOMEM;
		goto err2;
	}

	if( !(rel = (r_sort_index_t*)
		neo_malloc( td->t_rec_num*sizeof(r_sort_index_t))) ) {
		neo_errno = E_RDB_NOMEM;
		goto err3;
	}

	/*
	 * レコード情報を上記 Index テーブルエリアに設定する
	 */
	for( i = 0, num = 0; i < td->t_rec_num; i++ ) {

#ifdef XXX
		/*
		 * キャッシュ化されていないレコードをファイル
		 * からロードする
		 */
		if( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 );
#endif

		if( td->t_rec[i].r_stat & R_USED ) {
			abs[num].i_head 
				= rel[num].i_head = td->t_rec[i].r_head;
			rel[num].i_id	= num;
			num++;
		}
	}

	/*
	 * テーブルをソートする
	 * ソートは、まずソート分析器を作成して、Index テーブルに対して
	 * 行う。ソート後 ソート済みIndex テーブルによって、実際
	 * メモリ上のレコードを入れ替えて、昇順(降順)に並べる
	 */
	if( num ) {
		if( (neo_errno = _rdb_sort_open( td,  snum, sp ) )
			|| (neo_errno = _rdb_sort_start( rel, num ))
			|| (neo_errno = _rdb_sort_close() )
			|| (neo_errno = 
				_rdb_arrange_data(abs,rel,num,td->t_rec_size))){

			goto err4;
		}
		/*
		 * レコードの状態を更新する
		 */
		for( i = 0; i < num; i++ ) {

DBG_A(("<%i->%i,%d[%s]>\n", i, abs[i].i_id, abs[i].i_head->r_abs, 
		abs[i].i_head+1 ));

			if( abs[i].i_id == i )
				continue;

			j = abs[i].i_head->r_abs;
			td->t_rec[j].r_stat 	|= R_UPDATE;
			abs[i].i_head->r_stat	|= R_UPDATE;

			td->t_rec[j].r_stat	|= R_BUSY;
		}
	}

	/*
	 * ソート用臨時
	 * Index エリアを解放する
	 */
	neo_free( rel );
	neo_free( abs );
	neo_free( sp );

DBG_EXIT(0);
	return( 0 );

err4: DBG_T("err4");
	neo_free( rel );
err3: DBG_T("err3");
	neo_free( abs );
err2: DBG_T("err2");
	neo_free( sp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
