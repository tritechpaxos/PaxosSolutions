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
 *		rdb_expr.c
 *
 *	説明	構文解析木用 サブルーチン
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"y.tab.h"

r_expr_t	*_e_alloc();
int			_e_free();
int			rdb_item_value();

/******************************************************************************
 * 関数名
 *		rdb_string_op()
 * 機能        
 *		文字列データ用の 解析木LEAF 作成
 * 関数値
 *      正常: 解析木のノード
 *      異常: 0
 * 注意
 ******************************************************************************/
r_expr_t	*
rdb_string_op( item, op, str )
	char	*item;	/* アイテム名	*/
	long long	op;	/* 比較演算子	*/
	char	*str;	/* 条件文字列	*/
{
	r_item_t	*ip;
	int		i;
	r_expr_t	*lv, *rv, *e;

DBG_ENT( M_RDB, "rdb_string_op" );
DBG_A(("item=[%s],op=%d,str%s=\n", item, op, str ));

	/*
	 *	アイテム検索
	 */
	for( ip = &_search_tbl->t_scm->s_item[0], i = 0;
			i < _search_tbl->t_scm->s_n; ip++, i++ ) {
		if( !strcmp( ip->i_name, item ) )
			goto find;
	}
	goto err1;

find:
	/* 
	 *	解析木用構造体のメモリアロック 
	 */
	lv	= _e_alloc( sizeof(r_expr_t) );
	if( !lv )
		goto err1;
	rv	= _e_alloc( sizeof(r_expr_t) );
	if( !rv ) {
		_e_free( lv );
		goto err1;
	}
	e	= _e_alloc( sizeof(r_expr_t) );
	if( !e ) {
		_e_free( lv );
		_e_free( rv );
		goto err1;
	}

	/* 
	 *	情報設定 
	 */
	lv->e_op				= TLEAF;
	lv->e_tree.t_unary.v_type		= VITEM;
	lv->e_tree.t_unary.v_value.v_item	= ip;

	rv->e_op				= TLEAF;
	rv->e_tree.t_unary.v_type		= VSTRING;
	rv->e_tree.t_unary.v_value.v_str.s_len	= strlen( str );
	rv->e_tree.t_unary.v_value.v_str.s_str	= str;

	e->e_op				= op;
	e->e_tree.t_binary.b_l		= lv;
	e->e_tree.t_binary.b_r		= rv;

DBG_EXIT(e);
	return( e );
err1:
DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		rdb_numerical_op()
 * 機能        
 *		数字データ用の 解析木LEAF 作成
 * 関数値
 *      正常: 解析木のノード
 *      異常: 0
 * 注意
 ******************************************************************************/
r_expr_t	*
rdb_numerical_op( item, op, num )
	char	*item;	/* アイテム名	*/
	long long	op;	/* 比較演算子	*/
	long	num;	/* 条件数	*/
{
	r_item_t	*ip;
	int		i;
	r_expr_t	*lv, *rv, *e;

DBG_ENT( M_RDB, "rdb_numerical_op" );
DBG_A(("item=[%s],op=%d,num=0x%xL\n", item, op, num ));

	/*
	 *	アイテム検索
	 */
	for( ip = &_search_tbl->t_scm->s_item[0], i = 0;
			i < _search_tbl->t_scm->s_n; ip++, i++ ) {
		if( !strcmp( ip->i_name, item ) )
			goto find;
	}
	goto err1;

find:
	/* 
	 *	解析木用構造体のメモリアロック 
	 */
	lv	= _e_alloc( sizeof(r_expr_t) );
	if( !lv )
		goto err1;
	rv	= _e_alloc( sizeof(r_expr_t) );
	if( !rv ) {
		_e_free( lv );
		goto err1;
	}
	e	= _e_alloc( sizeof(r_expr_t) );
	if( !e ) {
		_e_free( lv );
		_e_free( rv );
		goto err1;
	}

	/* 
	 *	情報設定 
	 */
	lv->e_op				= TLEAF;
	lv->e_tree.t_unary.v_type		= VITEM;
	lv->e_tree.t_unary.v_value.v_item	= ip;

	rv->e_op				= TLEAF;
	rv->e_tree.t_unary.v_type		= VBIN;
	rv->e_tree.t_unary.v_value.v_int	= num;

	e->e_op				= op;
	e->e_tree.t_binary.b_l		= lv;
	e->e_tree.t_binary.b_r		= rv;

DBG_EXIT(e);
	return( e );
err1:
DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		rdb_logical_op()
 * 機能        
 *		論理演算用の 解析木LEAF 作成
 * 関数値
 *      正常: 解析木のノード
 *      異常: 0
 * 注意
 ******************************************************************************/
r_expr_t	*
rdb_logical_op( epl, op, epr )
	r_expr_t	*epl;	/* 解析木のノード	*/
	long long		op;	/* 論理演算子		*/
	r_expr_t	*epr;	/* 解析木のノード	*/
{
	r_expr_t	*e;

DBG_ENT( M_RDB, "rdb_logical_op" );

	/* 
	 *	解析木用構造体のメモリアロック 
	 */
	if( (e = _e_alloc(sizeof(r_expr_t) )) ) {
		e->e_op			= op;
		e->e_tree.t_binary.b_l	= epl;
		e->e_tree.t_binary.b_r	= epr;
	} else {
		goto err1;
	}

DBG_EXIT(e);
	return( e );
err1:
DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		rdb_evaluate()
 * 機能        
 *		設定された検索条件で、レコードを評価する
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 *	結果が 真ならば 1,  偽ならば 0 が vp->v_value.v_int に設定
 *	また、 vp->v_type に VBOOL(論理値) が設定されていない時はエラー
 ******************************************************************************/
int
rdb_evaluate( tp, rec, vp )
	r_expr_t	*tp;	/* I 解析木のルート	*/
	char		*rec;	/* I レコードデータ	*/
	r_value_t	*vp;	/* O 評価結果		*/
{
	r_value_t	vl, vr;

DBG_ENT( M_RDB, "rdb_evaluate" );

	/*
	 *	ノードのタイプをチェック
	 */
	switch( tp->e_op ) {
		/*
		 *	ノードが 葉(TLEAF)
		 *		vp にデータを設定する
		 */
		case TLEAF:	/* terminal */
			switch( tp->e_tree.t_unary.v_type ) {
				case VITEM:
					rdb_item_value(
					  tp->e_tree.t_unary.v_value.v_item,
					  rec,
					  vp );
					break;
				case VSTRING:
DBG_A(("VSTRING(%s)\n", tp->e_tree.t_unary.v_value.v_str.s_str ));
					bcopy( &tp->e_tree.t_unary, vp,
								sizeof(*vp) );
					break;
				case VBIN:
DBG_A(("VBIN(0x%x)\n", tp->e_tree.t_unary.v_value.v_int ));
					bcopy( &tp->e_tree.t_unary, vp,
								sizeof(*vp) );
					break;
					
			}
DBG_EXIT(0);
			return( 0 );

		/*
		 *	ノードが 演算子
		 *		再帰を繰り返して 葉(TLEAF)の演算になったら評価
		 *		結果が 真ならば 1,  偽ならば 0 を v_int に設定
		 *		その際に v_type に VBOOL(論理値) を設定する
		 */
		default:
			/*
			 *	左再帰 ( vl 設定 )
			 */
			if( rdb_evaluate( tp->e_tree.t_binary.b_l, 
							rec, &vl ) < 0 )
				goto err1;
			/*
			 *	右再帰 ( vr 設定 )
			 */
			if( rdb_evaluate( tp->e_tree.t_binary.b_r, 
							rec, &vr ) < 0 )
				goto err1;

			/*
			 *	評価
			 */
			switch( tp->e_op ) {

				/* logical operator */
				case SOR:	
					if( (vl.v_type != VBOOL) 
						|| (vr.v_type != VBOOL)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int 
							|| vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SAND:
					if( (vl.v_type != VBOOL) 
						|| (vr.v_type != VBOOL)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int 
							&& vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;

				/* arithmatic operator */
				case SLT:	
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SGT:
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SLE:
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SGE:
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SEQ:
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int ==
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case SNE:
					if( (vl.v_type != VBIN) 
						|| (vr.v_type != VBIN)) {
							goto err1;
					} else {
						vp->v_type	= VBOOL;
						if( vl.v_value.v_int !=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
					}
					break;
				case STR_EQ:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if(!strncmp( vl.v_value.v_str.s_str,
					  	       vr.v_value.v_str.s_str,
					  	       vl.v_value.v_str.s_len))
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;
				case STR_NE:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if( strncmp( vl.v_value.v_str.s_str,
					  	       vr.v_value.v_str.s_str,
					  	       vl.v_value.v_str.s_len))
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;
				case STR_GT:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if( strncmp( vl.v_value.v_str.s_str,
					  	   vr.v_value.v_str.s_str,
					  	   vl.v_value.v_str.s_len) > 0)
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;
				case STR_LT:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if( strncmp( vl.v_value.v_str.s_str,
					  	   vr.v_value.v_str.s_str,
					  	   vl.v_value.v_str.s_len) < 0)
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;
				case STR_GE:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if( strncmp( vl.v_value.v_str.s_str,
					  	   vr.v_value.v_str.s_str,
					  	   vl.v_value.v_str.s_len) >= 0)
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;
				case STR_LE:
					if( (vl.v_type != VSTRING) 
						|| (vr.v_type != VSTRING)) {
							goto err1;
					} else {
					  vp->v_type	= VBOOL;
					  if( strncmp( vl.v_value.v_str.s_str,
					  	   vr.v_value.v_str.s_str,
					  	   vl.v_value.v_str.s_len) <= 0)
						vp->v_value.v_int = 1;
					  else
						vp->v_value.v_int = 0;
					}
					break;

			}
			break;
	}

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}

/******************************************************************************
 * 関数名
 *		rdb_item_value()
 * 機能        
 *		解析木の 葉(VITEM) に、レコードのデータを設定する
 * 関数値
 *      ０のみで、意味がない
 * 注意
 ******************************************************************************/
int
rdb_item_value( ip, rec, vp )
	r_item_t	*ip;	/* I アイテム構造体	*/
	char		*rec;	/* I レコードデータ	*/
	r_value_t	*vp;	/* O 解析木の 葉(VITEM)	*/
{
	int	len;

DBG_ENT( M_RDB, "rdb_item_value" );

	switch( ip->i_attr ) {
		case R_HEX:
		case R_INT:
		case R_UINT:
		case R_FLOAT:
		case R_LONG:
		case R_ULONG:
			vp->v_type	= VBIN;
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
			break;
		case R_STR:
			vp->v_type		= VSTRING;
			vp->v_value.v_str.s_len	= ip->i_len;
			vp->v_value.v_str.s_str	= (char *)(rec+ip->i_off);
			break;
		case R_BYTES:
		    vp->v_type		= VSTRING;
			vp->v_value.v_str.s_len	= ip->i_len;
			vp->v_value.v_str.s_str	= (char *)(rec+ip->i_off);
			break;

	}

DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		_e_alloc()
 * 機能        
 *		解析木用構造体のメモリアロック
 * 関数値
 *      正常: ノードへのポインタ
 *      異常: 0
 * 注意
 *	現在は、固定領域から順番に取得しているだけ
 ******************************************************************************/
r_expr_t	*
_e_alloc( n )
	int	n;	/* 現在、未使用	*/
{
	int	i;

DBG_ENT(M_RDB,"e_alloc");

#ifdef NODEF
	if( !_search_usr->u_search.s_ebuff ) {
		if( !(_search_usr->u_search.s_ebuff = 
			(r_expr_t *)neo_malloc(R_EXPR_MAX*sizeof(r_expr_t))) ) {
			goto err1;
		} else {
			_search_usr->u_search.s_max	= R_EXPR_MAX;
			_search_usr->u_search.s_cnt	= 0;
		}
		if( _search_usr->u_search.s_cnt 
				>= _search_usr->u_search.s_max )
			goto err1;
	}
	DBG_EXIT(_search_usr->u_search.s_ebuff+_search_usr->u_search.s_cnt++ );
	return( _search_usr->u_search.s_ebuff+_search_usr->u_search.s_cnt++ );
#endif

	if( _search_usr->u_search.s_cnt 
			>= _search_usr->u_search.s_max )
		goto err1;

	i = _search_usr->u_search.s_cnt++;

DBG_EXIT(i);
	return( &_search_usr->u_search.s_elm[i] );
err1:
DBG_EXIT(0);
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		_e_free()
 * 機能        
 *		解析木用構造体のメモリ解放
 * 関数値
 *      ０のみで、意味がない
 * 注意
 *	現在は、何もしない
 ******************************************************************************/
int
_e_free( ep )
	r_expr_t	*ep;	/* 現在、未使用 */
{
	return( 0 );
}
