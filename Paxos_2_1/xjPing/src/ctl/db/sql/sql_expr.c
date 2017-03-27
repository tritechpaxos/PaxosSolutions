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

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"neo_db.h"
#include	"../svc/join.h"
#include	"../sql/sql.h"

extern r_expr_t	*sql_getnode();
extern r_expr_t	*sql_tree();
extern r_expr_t	*sql_sname();

int sql_item_value( r_item_t*, char*, r_value_t* );
int sql_evaluate( r_expr_t*, r_head_t*[], r_value_t* );
int sql_create_expr( r_expr_t*, r_from_tables_t* );


int
sql_create_select( ep, tblp, op )
	r_expr_t	*ep;
	r_from_tables_t	*tblp;
	r_expr_t	*op;
{
	int		nTable;
	r_expr_t	*wp;
	r_expr_t	*vp;
	r_expr_t	*pp;
	r_expr_t	*cp;
	r_item_t	*ip, *vip;
	int		i, j;
	int		len;
	r_expr_t	*np;
	r_expr_t	*nodep;
	r_expr_t	*sasp;
	r_expr_t	*tp;
	r_expr_t	*wop;
	r_tbl_t		*tdp;
	char		*tname, *aname, *cname;
	int		pos;

	DBG_ENT(M_SQL,"sql_create_select");

	nTable = tblp->f_n;

	if(!(wp = sql_getnode( ep, 0, SAS ))) {	/* All items of tdpp */
		wp = sql_getnode( ep, 0, '*' );

		sql_free1( wp );

		len = sizeof(r_head_t);

		for( i = 0, np = wp, pos = 0; i < nTable; i++ ) {

			tdp = tblp->f_t[i].f_tdp;
			aname = tblp->f_t[i].f_name;

			for( j = 0; j < tdp->t_scm->s_n; j++ ) {
				if( !(nodep = sql_tree( 0, SNODE, 0 ) ) ) {
					goto err1;
				}
				np->e_tree.t_binary.b_r = nodep;
				np = nodep;
				if(!(sasp = sql_tree( 0, SAS, 0 ) ) ) {
					goto err1;
				}
				nodep->e_tree.t_binary.b_l = sasp;
				if(!(pp = sql_tree( 0, '.', 0 ) ) ) {
					goto err1;
				}
				sasp->e_tree.t_binary.b_l = pp;
				if(!(tp = (r_expr_t*)sql_mem_zalloc(
						sizeof(r_expr_t)))){
					neo_errno = E_SQL_NOMEM;
					goto err1;
				}
				pp->e_tree.t_binary.b_l = tp;

				tp->e_op = TLEAF;
				tp->e_tree.t_unary.v_type	= VBIN;
				tp->e_tree.t_unary.v_value.v_int= i;

				if(!(vp = (r_expr_t*)sql_mem_zalloc(
						sizeof(r_expr_t)))){
					neo_errno = E_SQL_NOMEM;
					goto err1;
				}
				pp->e_tree.t_binary.b_r = vp;
				vp->e_op	= TLEAF;

				if(!(ip = (r_item_t*)sql_mem_malloc(
						sizeof(r_item_t)))){
					neo_errno = E_SQL_NOMEM;
					goto err1;
				}
				vp->e_tree.t_unary.v_type= VITEM_ALLOC;
				vp->e_tree.t_unary.v_value.v_item= ip;

				bcopy( &tdp->t_scm->s_item[j], 
							ip, sizeof(r_item_t));

				if(!(vp = (r_expr_t*)sql_mem_zalloc(
						sizeof(r_expr_t)))){
					neo_errno = E_SQL_NOMEM;
					goto err1;
				}
				sasp->e_tree.t_binary.b_r = vp;
				vp->e_op	= TLEAF;

				if(!(ip = (r_item_t*)sql_mem_malloc(
						sizeof(r_item_t)))){
					neo_errno = E_SQL_NOMEM;
					goto err1;
				}
				vp->e_tree.t_unary.v_type= VITEM_ALLOC;
				vp->e_tree.t_unary.v_value.v_item= ip;

				bcopy( &tdp->t_scm->s_item[j], ip, 
							sizeof(r_item_t));

				ip->i_off	= len;
				len += ip->i_size;
				/*
				 *	Adjust order_by
				 */
				if( op ) {
					wop = 0;
					while( (wop = sql_getnode(op,wop,'.')) ){

						if( wop->e_tree.t_binary.b_l )
						  tname =
							wop->e_tree.t_binary.b_l
							->e_tree.t_unary.
							v_value.v_str.s_str;
						else
						  tname = (char*)&tdp->t_name;

						cname =wop->e_tree.t_binary.b_r
							->e_tree.t_unary.
							v_value.v_str.s_str;

						if( !strcmp( tname, aname )
						&& !strcmp( cname,
							(char*)&ip->i_name )){

							sql_free1( wop );

							wop->e_op = TLEAF;
							wop->e_tree.t_unary.
								v_type = VBIN;
							wop->e_tree.t_unary.
							v_value.v_int = pos;
							
						}
					}
				}
			pos++;
			}
		}
		goto ret;	/* EXIT */
	}


	if( sql_create_expr( ep, tblp ) ) {
		neo_errno = E_SQL_SELECT;
		goto err1;
	}
	len = sizeof(r_head_t);
	pos = 0;
	wp = ep;
	while( (wp = sql_getnode( ep, wp, SAS )) ) {	/* Assemble items */

		pp = sql_getnode( wp, 0, '.' );		/* '.' */

		tdp = tblp->
			f_t[pp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_int].
			f_tdp;
		aname = tblp->
			f_t[pp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_int].f_name;

		vip = pp->e_tree.t_binary.b_r->e_tree.t_unary.v_value.v_item;
		/*
		 *	Adjust order_by
		 */
		if( op ) {
			wop = 0;
			while( (wop = sql_getnode(op,wop,'.')) ){

				if( wop->e_tree.t_binary.b_l )
				 	 tname =
							wop->e_tree.t_binary.b_l
							->e_tree.t_unary.
							v_value.v_str.s_str;
				else
					  tname = (char*)&tdp->t_name;

				cname =wop->e_tree.t_binary.b_r
					->e_tree.t_unary.
					v_value.v_str.s_str;

				if( !strcmp( tname, aname )
				&& !strcmp( cname,
					(char*)&vip->i_name )){

					sql_free1( wop );

					wop->e_op = TLEAF;
					wop->e_tree.t_unary.
						v_type = VBIN;
					wop->e_tree.t_unary.
					v_value.v_int = pos;
							
				}
			}
		}

		cp = wp->e_tree.t_binary.b_r;	/* colum name */
		if(!(ip = (r_item_t*)sql_mem_malloc(sizeof(r_item_t)))){
			neo_errno = E_SQL_NOMEM;
			goto err1;
		}
		bcopy( vip, ip, sizeof(r_item_t));

		strcpy( (char*)&ip->i_name, 
			cp->e_tree.t_unary.v_value.v_str.s_str );

		ip->i_off	= len;
		len += ip->i_size;

		sql_free1( cp );

		cp->e_op	= TLEAF;
		cp->e_tree.t_unary.v_type		= VITEM_ALLOC;
		cp->e_tree.t_unary.v_value.v_item	= ip;

		pos++;
	}
ret:
	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

/*
 *	Adjust expression-tree for evaluate
 */
int
sql_create_expr( ep, tblp )
	r_expr_t	*ep;
	r_from_tables_t	*tblp;
{
	int		nTable;
	r_expr_t	*np;
	r_expr_t	*wp;
	r_value_t	*lp;
	r_value_t	*rp;
	r_item_t	*ip;
	int		i, j;

	DBG_ENT(M_SQL,"sql_create_expr");

	nTable = tblp->f_n;
	np = ep;
	while( (np = sql_getnode( ep, np, '.' )) ) {
		if( np->e_tree.t_binary.b_l ) { /* table index */
			if( np->e_tree.t_binary.b_l->e_op != TLEAF ) {
				neo_errno = E_SQL_EXPR;
				goto err1;
			}
			lp = &np->e_tree.t_binary.b_l->e_tree.t_unary;

			if( lp->v_type == VBIN )	/* already set */
				continue;

			if( lp->v_type != VSTRING 
				&& lp->v_type != VSTRING_ALLOC ) {
				neo_errno = E_SQL_EXPR;
				goto err1;
			}
			for( i = 0; i < nTable; i++ ) {
				if( !strcmp( lp->v_value.v_str.s_str,
					(char*)&tblp->f_t[i].f_name ) ) {
					if( lp->v_type == VSTRING_ALLOC )
						sql_mem_free( lp->v_value.
								v_str.s_str );
					lp->v_type = VBIN;
					lp->v_value.v_int	= i;

					goto ok_left;
				}
			}
			neo_errno = E_SQL_EXPR;
			goto err1;
		} else {
			if( !(wp = (r_expr_t*)sql_mem_malloc( sizeof(r_expr_t)))){
				goto err1;
			}
			wp->e_op = TLEAF;
			wp->e_tree.t_unary.v_type = VBIN;
			wp->e_tree.t_unary.v_value.v_int = 0;
			np->e_tree.t_binary.b_l	= wp;

			i = 0;
		}
	ok_left:
		if( np->e_tree.t_binary.b_r ) { 
			if( np->e_tree.t_binary.b_r->e_op != TLEAF ) {
				neo_errno = E_SQL_EXPR;
				goto err1;
			}
			rp = &np->e_tree.t_binary.b_r->e_tree.t_unary;
			if( rp->v_type != VSTRING 
				&& rp->v_type != VSTRING_ALLOC ) {
				neo_errno = E_SQL_EXPR;
				goto err1;
			}
			for( j = 0; j < tblp->f_t[i].f_tdp->t_scm->s_n; j++ ) {
				if( !strcmp( rp->v_value.v_str.s_str,
				(char*)&tblp->f_t[i].f_tdp
					->t_scm->s_item[j].i_name ) ) {

					if( !(ip = (r_item_t*)
						sql_mem_malloc( sizeof(r_item_t)))){
						goto err1;
					}
					bcopy( &tblp->f_t[i].f_tdp
							->t_scm->s_item[j], 
							ip, sizeof(*ip) );
					if( rp->v_type == VSTRING_ALLOC )
						sql_mem_free( rp->v_value.
								v_str.s_str );
					rp->v_type = VITEM_ALLOC;
					rp->v_value.v_item	= ip;

					goto ok_right;
				}
			}
			neo_errno = E_SQL_EXPR;
			goto err1;
		} else {
			neo_errno = E_SQL_EXPR;
			goto err1;
		}
	ok_right:;
	}

	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

/*
 *	Must be  called after adjust
 */
int
sql_eval_expr( ep, rpp )
	r_expr_t	*ep;
	r_head_t	*rpp[];
{
	r_value_t	v;

	DBG_ENT(M_SQL,"sql_eval_expr");

/***
sqlprint(ep);
***/
	if( /*neo_errno = */sql_evaluate( ep, rpp, &v ) ) {
		goto err;
	}

	DBG_EXIT( v.v_value.v_int );
	return( v.v_value.v_int );
err:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_evaluate( ep, rpp, vp )
	register r_expr_t	*ep;
	register r_head_t	*rpp[];
	register r_value_t	*vp;
{
	r_value_t	vl, vr, vs;
	r_expr_t	*epl, *epr;
	int		bool;

	DBG_ENT(M_SQL,"sql_evaluate");

	/*
	 *	ノードのタイプをチェック
	 */
	switch( ep->e_op ) {
		/*
		 *	ノードが 葉(TLEAF)
		 *		vp にデータを設定する
		 */
		case '.':	/* item variable */
			sql_item_value( ep->e_tree.t_binary.b_r->
					e_tree.t_unary.v_value.v_item,
			 		(char*)rpp[ep->e_tree.t_binary.b_l->
					e_tree.t_unary.v_value.v_int], vp );
			DBG_EXIT(0);
			return( 0 );
		case TLEAF:	/* terminal and cast */
			switch( ep->e_tree.t_unary.v_type ) {
				case VSTRING:
				case VSTRING_ALLOC:
					bcopy( &ep->e_tree.t_unary, vp,
								sizeof(*vp) );
					break;
				case VBIN:
				case VLONG:
				case VULONG:
				case VINT:
				case VUINT:
				case VFLOAT:
					bcopy( &ep->e_tree.t_unary, vp,
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
			 *	LIKE/NOT LIKE ESCAPE clause
			 */
			if( ep->e_op == SLIKE || ep->e_op == SNOTLIKE ) {

				/* column */
				if( !(epl=ep->e_tree.t_binary.b_l)) goto err1;
				if(sql_evaluate( epl, rpp, &vl ) < 0 ) {
						goto err1;
				}
				/* pattern and ecsape */
				if( !(epr=ep->e_tree.t_binary.b_r) ) goto err1;
				if( epr->e_op != SESCAPE )	goto err1;
				epl = epr->e_tree.t_binary.b_l;
				epr = epr->e_tree.t_binary.b_r;
				if( !epl )	goto  err1;
				if(sql_evaluate( epl, rpp, &vr ) < 0 ) {
						goto err1;
				}
				if( epr ) {
					if(sql_evaluate(epr, rpp, &vs ) < 0 ) {
							goto err1;
					}
				}
				if( sql_like( 
					vl.v_value.v_str.s_str,
					vl.v_value.v_str.s_len,
					vr.v_value.v_str.s_str,
					vr.v_value.v_str.s_len,
					epr == 0 ? 0: vs.v_value.v_str.s_str,
					&bool ) )
						goto err1;
					
			  	vp->v_type	= VBOOL;
				switch( ep->e_op ) {
					case SLIKE:
						if( bool )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					case SNOTLIKE:
						if( bool )
							vp->v_value.v_int = 0;
						else
							vp->v_value.v_int = 1;
						break;
				}
				DBG_EXIT( 0 );
				return( 0 );
			}
			/*
			 *	左再帰 ( vl 設定 )
			 */
			if( ep->e_tree.t_binary.b_l ) {

				if( sql_evaluate( ep->e_tree.t_binary.b_l, 
							rpp, &vl ) < 0 )
					goto err1;
			}
			/*
			 *	評価1-logical operator
			 */
			switch( ep->e_op ) {

				case SOR:	
					if( vl.v_type != VBOOL )  {
						goto err1;
					}
					if( vl.v_value.v_int == 1 ) { 

						vp->v_type	= VBOOL;
						vp->v_value.v_int = 1;

						DBG_EXIT(0);
						return( 0 );
					}
					break;
				case SAND:
					if( vl.v_type != VBOOL )  {
						goto err1;
					}
					if( vl.v_value.v_int == 0 ) { 

						vp->v_type	= VBOOL;
						vp->v_value.v_int = 0;

						DBG_EXIT(0);
						return( 0 );
					}
					break;
			}
			/*
			 *	右再帰 ( vr 設定 )
			 */
			if( ep->e_tree.t_binary.b_r ) {

				if( sql_evaluate( ep->e_tree.t_binary.b_r, 
							rpp, &vr ) < 0 )
					goto err1;
			}
			/*
			 *	Unary --- Through
			 */
			if( !ep->e_tree.t_binary.b_l 
				|| !ep->e_tree.t_binary.b_r ) {	/* through */

				if( ep->e_tree.t_binary.b_l )
					bcopy( &vl, vp, sizeof(*vp) );
				if( ep->e_tree.t_binary.b_r )
					bcopy( &vr, vp, sizeof(*vp) );

				switch( ep->e_op ) {
				  case '-':
					switch( vp->v_type ) {
					  case VINT:
						vp->v_value.v_int =	
							- (vp->v_value.v_int);
					  	break;
					  case VLONG:
						vp->v_value.v_long =	
							- (vp->v_value.v_long);
					  	break;
					  case VFLOAT:
						vp->v_value.v_float =	
							- (vp->v_value.v_float);
					  	break;
					}
				  	break;
				}
			}
			/*
			 *	評価
			 */
			switch( ep->e_op ) {

				/* +, -, *, / */

				case '+':
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					switch( vl.v_type ) {
					  case VINT:
						vp->v_type = VINT;
						vp->v_value.v_int =
							vl.v_value.v_int +
							vr.v_value.v_int;
						break;
					  case VUINT:
						vp->v_type = VUINT;
						vp->v_value.v_uint =
							vl.v_value.v_uint +
							vr.v_value.v_uint;
						break;
					  case VLONG:
						vp->v_type = VLONG;
						vp->v_value.v_long =
							vl.v_value.v_long +
							vr.v_value.v_long;
						break;
					  case VULONG:
						vp->v_type = VULONG;
						vp->v_value.v_ulong =
							vl.v_value.v_ulong +
							vr.v_value.v_ulong;
						break;
					  case VFLOAT:
						vp->v_type = VFLOAT;
						vp->v_value.v_float =
							vl.v_value.v_float +
							vr.v_value.v_float;
						break;
					}
					break;

				case '-':
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					switch( vl.v_type ) {
					  case VINT:
						vp->v_type = VINT;
						vp->v_value.v_int =
							vl.v_value.v_int -
							vr.v_value.v_int;
						break;
					  case VUINT:
						vp->v_type = VUINT;
						vp->v_value.v_uint =
							vl.v_value.v_uint -
							vr.v_value.v_uint;
						break;
					  case VLONG:
						vp->v_type = VLONG;
						vp->v_value.v_long =
							vl.v_value.v_long -
							vr.v_value.v_long;
						break;
					  case VULONG:
						vp->v_type = VULONG;
						vp->v_value.v_ulong =
							vl.v_value.v_ulong -
							vr.v_value.v_ulong;
						break;
					  case VFLOAT:
						vp->v_type = VFLOAT;
						vp->v_value.v_float =
							vl.v_value.v_float -
							vr.v_value.v_float;
						break;
					}
					break;

				case '*':
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					switch( vl.v_type ) {
					  case VINT:
						vp->v_type = VINT;
						vp->v_value.v_int =
							vl.v_value.v_int *
							vr.v_value.v_int;
						break;
					  case VUINT:
						vp->v_type = VUINT;
						vp->v_value.v_uint =
							vl.v_value.v_uint *
							vr.v_value.v_uint;
						break;
					  case VLONG:
						vp->v_type = VLONG;
						vp->v_value.v_long =
							vl.v_value.v_long *
							vr.v_value.v_long;
						break;
					  case VULONG:
						vp->v_type = VULONG;
						vp->v_value.v_ulong =
							vl.v_value.v_ulong *
							vr.v_value.v_ulong;
						break;
					  case VFLOAT:
						vp->v_type = VFLOAT;
						vp->v_value.v_float =
							vl.v_value.v_float *
							vr.v_value.v_float;
						break;
					}
					break;

				case '/':
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					switch( vl.v_type ) {
					  case VINT:
						vp->v_type = VINT;
						vp->v_value.v_int =
							vl.v_value.v_int /
							vr.v_value.v_int;
						break;
					  case VUINT:
						vp->v_type = VUINT;
						vp->v_value.v_uint =
							vl.v_value.v_uint /
							vr.v_value.v_uint;
						break;
					  case VLONG:
						vp->v_type = VLONG;
						vp->v_value.v_long =
							vl.v_value.v_long /
							vr.v_value.v_long;
						break;
					  case VULONG:
						vp->v_type = VULONG;
						vp->v_value.v_ulong =
							vl.v_value.v_ulong /
							vr.v_value.v_ulong;
						break;
					  case VFLOAT:
						vp->v_type = VFLOAT;
						vp->v_value.v_float =
							vl.v_value.v_float /
							vr.v_value.v_float;
						break;
					}
					break;

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
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VINT ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VUINT)  {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint <
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VLONG) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long <
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VULONG ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong <
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float <
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

						vp->v_type	= VBOOL;

						if( strncmp( 
						vl.v_value.v_str.s_str,
					  	vr.v_value.v_str.s_str,
					  	vl.v_value.v_str.s_len) < 0)

							vp->v_value.v_int = 1;
					 	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
				case SGT:
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VINT ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VUINT)  {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint >
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VLONG) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long >
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VULONG ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong >
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float >
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

						  vp->v_type	= VBOOL;
						  if( strncmp( 
							vl.v_value.v_str.s_str,
					  	   	vr.v_value.v_str.s_str,
					  	   	vl.v_value.v_str.s_len)
								 > 0)
							vp->v_value.v_int = 1;
					  	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
				case SLE:
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VINT ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int <=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VUINT)  {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint <=
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VLONG) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long <=
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VULONG ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong <=
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float <=
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

					  	vp->v_type	= VBOOL;
					  	if( strncmp( 
						vl.v_value.v_str.s_str,
					  	vr.v_value.v_str.s_str,
					  	vl.v_value.v_str.s_len) <= 0)
							vp->v_value.v_int = 1;
					  	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
				case SGE:
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( (vl.v_type == VINT) 
						&& (vr.v_type == VINT)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int >=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( (vl.v_type == VUINT) 
						&& (vr.v_type == VUINT)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint >=
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( (vl.v_type == VLONG) 
						&& (vr.v_type == VLONG)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long >=
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( (vl.v_type == VULONG) 
						&& (vr.v_type == VULONG)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong >=
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float >=
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

					 	vp->v_type	= VBOOL;
						if( strncmp( 
						vl.v_value.v_str.s_str,
					  	vr.v_value.v_str.s_str,
					  	vl.v_value.v_str.s_len) >= 0)
							vp->v_value.v_int = 1;
					  	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
				case SEQ:
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int ==
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VINT ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int ==
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VUINT)  {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint ==
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VLONG) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long ==
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VULONG ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong ==
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float ==
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

					  	vp->v_type	= VBOOL;
					  	if(!strncmp( vl.v_value.
								v_str.s_str,
					  	       vr.v_value.v_str.s_str,
					  	       vl.v_value.v_str.s_len))
							vp->v_value.v_int = 1;
					  	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
				case SNE:
					if( vl.v_type != vr.v_type )
						sql_cast( &vl, &vr );

					if( (vl.v_type == VBIN) 
						&& (vr.v_type == VBIN)) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int !=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VINT ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_int !=
							vr.v_value.v_int )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VUINT)  {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_uint !=
							vr.v_value.v_uint )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VLONG) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_long !=
							vr.v_value.v_long )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VULONG ) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_ulong !=
							vr.v_value.v_ulong )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}
					if( vl.v_type == VFLOAT) {

						vp->v_type	= VBOOL;
						if( vl.v_value.v_float !=
							vr.v_value.v_float )
							vp->v_value.v_int = 1;
						else
							vp->v_value.v_int = 0;
						break;
					}

					if( (vl.v_type == VSTRING
						|| vl.v_type == VSTRING_ALLOC) 
					&& (vr.v_type == VSTRING
						|| vr.v_type == VSTRING_ALLOC)){

					  	vp->v_type	= VBOOL;
					  	if( strncmp( 
							vl.v_value.v_str.s_str,
					  	       	vr.v_value.v_str.s_str,
					  	       	vl.v_value.v_str.s_len))
							vp->v_value.v_int = 1;
					  	else
							vp->v_value.v_int = 0;
						break;
					}
					goto err1;
			}
			break;
	}
	DBG_EXIT(0);
	return( 0 );
err1:
	DBG_EXIT( E_SQL_EVAL );
	return( E_SQL_EVAL );
}

/******************************************************************************
 * 関数名
 *		sql_item_value()
 * 機能        
 *		r_value_tに、レコードのデータを設定する
 * 関数値
 *      ０のみで、意味がない
 * 注意
 ******************************************************************************/
int
sql_item_value( ip, rec, vp )
	r_item_t	*ip;	/* I アイテム構造体	*/
	char		*rec;	/* I レコードデータ	*/
	r_value_t	*vp;	/* O 解析木の 葉(VITEM)	*/
{
	int	len;

DBG_ENT( M_SQL, "rdb_item_value" );

	switch( ip->i_attr ) {	/* cast */
		case R_HEX:
			vp->v_type	= VBIN;
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
			if( len == sizeof(int) )	vp->v_type = VUINT;
			break;
		case R_INT:
			vp->v_type	= VINT;
			vp->v_value.v_int = *((int*)(rec+ip->i_off));
/***
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
***/
			break;
		case R_UINT:
			vp->v_type	= VUINT;
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
			break;
		case R_FLOAT:
			vp->v_type	= VFLOAT;
			vp->v_value.v_float = *((float*)(rec+ip->i_off));
/***
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
***/
			break;
		case R_LONG:
			vp->v_type	= VLONG;
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
			break;
		case R_ULONG:
			vp->v_type	= VULONG;
			len = ip->i_len < sizeof(vp->v_value) ?
					ip->i_len : sizeof(vp->v_value);
			bcopy( rec + ip->i_off, &vp->v_value, len );
			break;
		case R_STR:
			vp->v_type		= VSTRING;
			vp->v_value.v_str.s_len	= ip->i_len;
			vp->v_value.v_str.s_str	= (char *)(rec+ip->i_off);
			break;

	}

DBG_EXIT(0);
	return( 0 );
}

void
sql_cast( vl, vr )
	r_value_t	*vl;
	r_value_t	*vr;
{
	if( vl->v_type == VFLOAT || vr->v_type == VFLOAT ) {
		if( vl->v_type != VFLOAT )	sql_cast_float( vl );
		if( vr->v_type != VFLOAT )	sql_cast_float( vr );
	}
	if( vl->v_type == VULONG || vr->v_type == VULONG ) {
		if( vl->v_type != VULONG )	sql_cast_ulong( vl );
		if( vr->v_type != VULONG )	sql_cast_ulong( vr );
	}
	if( vl->v_type == VLONG || vr->v_type == VLONG ) {
		if( vl->v_type != VULONG )	sql_cast_long( vl );
		if( vr->v_type != VULONG )	sql_cast_long( vr );
	}
	if( vl->v_type == VUINT || vr->v_type == VUINT ) {
		if( vl->v_type != VUINT )	sql_cast_uint( vl );
		if( vr->v_type != VUINT )	sql_cast_uint( vr );
	}
}

void
sql_cast_float( v )
	r_value_t	*v;
{
	switch( v->v_type ) {
		case VINT:	
			v->v_value.v_float	= (double)v->v_value.v_int;
			v->v_type = VFLOAT;
			break;
		case VUINT:	
			v->v_value.v_float	= (double)v->v_value.v_uint;
			v->v_type = VFLOAT;
			break;
		case VLONG:	
			v->v_value.v_float	= (double)v->v_value.v_long;
			v->v_type = VFLOAT;
			break;
		case VULONG:	
			v->v_value.v_float	= (double)v->v_value.v_ulong;
			v->v_type = VFLOAT;
			break;
	}
}

void
sql_cast_ulong( v )
	r_value_t	*v;
{
	switch( v->v_type ) {
		case VINT:	
			v->v_value.v_ulong	= 
				(unsigned long long)v->v_value.v_int;
			v->v_type = VULONG;
			break;
		case VUINT:	
			v->v_value.v_ulong	= 
				(unsigned long long)v->v_value.v_uint;
			v->v_type = VULONG;
			break;
		case VLONG:	
			v->v_value.v_ulong	= 
				(unsigned long long)v->v_value.v_long;
			v->v_type = VULONG;
			break;
	}
}

void
sql_cast_long( v )
	r_value_t	*v;
{
	switch( v->v_type ) {
		case VULONG:	
			v->v_value.v_long	= v->v_value.v_ulong;
			v->v_type = VLONG;
			break;
		case VINT:	
			v->v_value.v_long	= (long long)v->v_value.v_int;
			v->v_type = VLONG;
			break;
		case VUINT:	
			v->v_value.v_long	= (long long)v->v_value.v_uint;
			v->v_type = VLONG;
			break;
	}
}

void
sql_cast_uint( v )
	r_value_t	*v;
{
	switch( v->v_type ) {
		case VULONG:	
			v->v_value.v_uint	= 
				(unsigned int)v->v_value.v_ulong;
			v->v_type = VUINT;
			break;
		case VLONG:	
			v->v_value.v_uint	= 
				(unsigned int)v->v_value.v_long;
			v->v_type = VUINT;
			break;
		case VINT:	
			v->v_value.v_uint	= 
				(unsigned int)v->v_value.v_int;
			v->v_type = VUINT;
			break;
	}
}

void
sql_cast_int( v )
	r_value_t	*v;
{
	switch( v->v_type ) {
		case VULONG:	
			v->v_value.v_int	= v->v_value.v_ulong;
			v->v_type = VINT;
			break;
		case VLONG:	
			v->v_value.v_int	= v->v_value.v_long;
			v->v_type = VINT;
			break;
		case VUINT:	
			v->v_value.v_int	= v->v_value.v_uint;
			v->v_type = VINT;
			break;
	}
}
