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
#include	"join.h"
#include	"hash.h"
#include	"../sql/sql.h"

extern r_expr_t	*sql_read_tree();
extern r_expr_t	*sql_getnode();

#define	MAX_INC	2000

int
prc_init_from_tables( r_from_tables_t *tblp, int n )
{
	int	i;

	memset( tblp, 0, sizeof(r_from_tables_t) + sizeof(r_from_table_t)*(n-1) );
	if( !(tblp->f_recpp = (r_head_t**)neo_malloc(sizeof(r_head_t*)*n))) {
		goto err1;
	}
	tblp->f_n	= n;
	if( !(tblp->f_joinp = (int*)neo_malloc(sizeof(int)*n) ) ) {
		goto err2;
	}
	for( i = 0; i < n; i++ ) {
		tblp->f_joinp[i]	= -1;
	}
	return( 0 );
err2:
	neo_free( tblp->f_recpp );	tblp->f_recpp	= NULL;
err1:
	neo_errno	= E_RDB_NOMEM;
	return( -1 );
}

void
prc_destroy_from_tables( r_from_tables_t *tblp )
{
	if( tblp->f_recpp ) {
		neo_free( tblp->f_recpp );
		tblp->f_recpp	= NULL;
	}
	if( tblp->f_joinp ) {
		neo_free( tblp->f_joinp );
		tblp->f_joinp	= NULL;
	}
	if( tblp->f_resultp ) {
		neo_free( tblp->f_resultp );
		tblp->f_resultp	= NULL;
	}
}

r_from_tables_t*
prc_open_from_tables( fromp )
	r_expr_t	*fromp;
{
	int		n;
	r_from_tables_t	*tblp;
	r_expr_t	*wp;
	int		i;

	DBG_ENT(M_SQL,"prc_open_from_tables");

	for(n=0, wp=0; (wp=sql_getnode(fromp,wp,'T')); n++ );
	/*
	 *	Open from-tables
 	 */
	if( !(tblp = (r_from_tables_t*)neo_malloc
		( sizeof(r_from_tables_t)+sizeof(r_from_table_t)*(n-1)))) {
		goto err1;
	}
	if( prc_init_from_tables( tblp, n ) ) {
		goto err2;
	}
	for( i = 0, wp = 0; i < n; i++ ) {

		wp = sql_getnode( fromp, wp, 'T' );
		
		/* table name length check */
		if( strlen( wp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_str.s_str ) 
				>= R_TABLE_NAME_MAX  ) {

			neo_errno = E_RDB_INVAL;
			goto err3;
		}
		if( wp->e_tree.t_binary.b_r
			&&  strlen( wp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_str.s_str ) 
				>= R_TABLE_NAME_MAX  ) {

			neo_errno = E_RDB_INVAL;
			goto err3;
		}

		/* alias table name */
		if( wp->e_tree.t_binary.b_r ) {
			strcpy( tblp->f_t[i].f_name,
				wp->e_tree.t_binary.b_r->
				e_tree.t_unary.v_value.v_str.s_str );
		} else {
			strcpy( tblp->f_t[i].f_name, 
				wp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_str.s_str );
		}

DBG_A(("FROM %d[%s-%s]\n", i, wp->e_tree.t_binary.b_l->
			e_tree.t_unary.v_value.v_str.s_str,
			tblp->f_t[i].f_name ));

		if( sql_open( wp->e_tree.t_binary.b_l->
				e_tree.t_unary.v_value.v_str.s_str,
					&tblp->f_t[i] ) ) {
			goto err3;
		}
	}

	DBG_EXIT( tblp );
	return( tblp );

err3:	
	for( i = 0; i < n; i++ ) {
		if( tblp->f_t[i].f_tdp ) {
			sql_close( &tblp->f_t[i] );
		}
	}
	prc_destroy_from_tables( tblp );
err2: 	neo_free( tblp );
err1:
	DBG_EXIT( 0 );
	return( 0 );
}

int
prc_close_from_tables( tblp, flg )
	r_from_tables_t	*tblp;
	int	flg;
{
	int		i;

	DBG_ENT(M_SQL,"prc_close_from_tables");

	for( i = 0; i < tblp->f_n; i++ ) {
		if( tblp->f_t[i].f_tdp ) {

			sql_close( &tblp->f_t[i] );
		}
	}
	prc_destroy_from_tables( tblp );
	if( flg )	neo_free( tblp );

	DBG_EXIT( 0 );
	return( 0 );
}

r_tbl_t*
prc_join( tablep, wherep, selectp, orderp )
register r_expr_t	*tablep;
	r_expr_t	*wherep;
	r_expr_t	*selectp;
	r_expr_t	*orderp;
{
	register int	i;
	int		nTable;
	r_tbl_t		*tdp;
	r_expr_t	*wp;
	r_from_tables_t	*tblp;
	r_tbl_t		*tdView;
	int		n, rec_size;
	r_scm_t		*scmp;
	r_tbl_list_t	*dependp;
	int		werr;

	DBG_ENT( M_SQL, "prc_join" );

	/*
	 *	Open from-tables
 	 */
	if( !(tblp = prc_open_from_tables( tablep ) )) {
		goto err1;
	}
	nTable = tblp->f_n;

	/*
	 *	Adjust where conditions
	 */
DBG_AF((sqlprint( wherep )));

	if( wherep && sql_create_expr( wherep, tblp ) ) {
		goto err4;
	}
DBG_AF((sqlprint( wherep )));
	tblp->f_wherep = wherep;
	/*
	 *	Adjust view schema and order_by
	 */
DBG_AF((sqlprint( selectp )));

	if( sql_create_select( selectp, tblp, orderp ) ) {
		goto err4;
	}
DBG_AF((sqlprint( selectp )));
	tblp->f_selectp = selectp;

	n = 0;
	rec_size = sizeof(r_head_t);
	wp = 0;
	while( (wp = sql_getnode( selectp, wp, SAS )) ) {
		n++;
	}
	if( !(scmp = (r_scm_t*)neo_malloc
			(sizeof(r_scm_t)+sizeof(r_item_t)*(n-1)) )) {
		goto err4;
	}
	/*
	 *	Create to items
	 */
	scmp->s_n = n;
	for( i = 0, wp = 0; i < n; i++ ) {
		wp = sql_getnode(selectp, wp, SAS ) ;
		bcopy( wp->e_tree.t_binary.b_r->e_tree.t_unary.v_value.v_item,
			&scmp->s_item[i], sizeof(r_item_t) );
		rec_size += scmp->s_item[i].i_size;
	}
		
	/*
	 *	Allocate r_tbl_t for select view
	 *		and table dependency
	 */
	if( !(tdView = svc_tbl_alloc() ) ) {
		goto err5;
	}
	if( !(dependp = (r_tbl_list_t*)neo_zalloc(
		sizeof(r_tbl_list_t) + sizeof(r_tbl_mod_t)*(nTable-1) ) ) ) {
		goto err6;
	}
	dependp->l_n	= nTable;
	for( i = 0; i < nTable; i++ ) {
		tdp = tblp->f_t[i].f_tdp;
		bcopy( tdp->t_name, dependp->l_tbl[i].m_name,
				sizeof(dependp->l_tbl[i].m_name ) );
		dependp->l_tbl[i].m_time = tdp->t_modified;
		dependp->l_tbl[i].m_version = tdp->t_version;
	}
	tdView->t_depend	= dependp;
	tdView->t_rec_size	= rec_size;
	tdView->t_scm		= scmp;

	tblp->f_tdp = tdView;
	tblp->f_num = 0;
	/*
	 *	Main loop for set 
	 */
#ifndef	PERFORMANCE
	if( xj_join_by_hash( tblp ) ) {
		goto err6;
	}
#else
int	nn = 100000;
clock_t	s, e;
s= clock();
while( nn-- ) {
	for( i = 0; i < nTable; i++ ) tblp->f_joinp[i] = -1;
	if( tblp->f_resultp ) {
		neo_free( tblp->f_resultp );
		tblp->f_resultp = NULL;
		tblp->f_result_size = 0;
	}
	tblp->f_num = 0;
	if( xj_join_by_hash( tblp ) ) {
		goto err6;
	}
}
e = clock();
//DBG_PRT("loop=%d\n", e-s );
#endif
	r_rec_t	*rp;
	int		j, k;
	r_item_t	*f_itemp, *t_itemp;
	int		*resultp;

	tdView->t_rec_num	= tblp->f_num;

	/* control */
	if( rec_cntl_alloc( tdView ) ) {
		goto err6;
	}
	/* data buffer */
	if( rec_buf_alloc( tdView ) ) {
		goto err7;
	}
	for( i = 0, resultp = tblp->f_resultp;
			i < tblp->f_num; i++, resultp += tblp->f_n ) {

			rp	= &tdView->t_rec[i];
	/* move data */
			for(j = 0,wp = selectp; j < tdView->t_scm->s_n; j++ ) {

				wp = sql_getnode( selectp, wp, '.' );

				k = wp->e_tree.t_binary.b_l->
						e_tree.t_unary.v_value.v_int;
				tdp	= tblp->f_t[k].f_tdp;

				f_itemp = wp->e_tree.t_binary.b_r->
					e_tree.t_unary.v_value.v_item;
				t_itemp = &tdView->t_scm->s_item[j];

				bcopy( (char*)(tdp->t_rec[resultp[k]].r_head) 
						+ f_itemp->i_off,
						(char*)rp->r_head + t_itemp->i_off,
						t_itemp->i_len
				);
			} 
		rp->r_stat	|= R_REC_CACHE|R_USED|R_BUSY|R_INSERT;
		rp->r_head->r_stat	|= R_USED|R_BUSY|R_INSERT;
		rp->r_head->r_abs	= i;
	}
	

	tdView->t_rec_used	= tblp->f_num;;
	tdView->t_stat 	|= R_UPDATE;

	tdView->t_access = time( 0 );
	/*
	 *	Sort
	 */
	if( orderp ) {
		if( prc_view_sort( tdView, orderp ) ) {
			goto err8;
		}
	}

	/*
	 *	Close from tables
	 */
	prc_close_from_tables( tblp, 1 );

DBG_AF((print_join( tdView )));

	DBG_EXIT( tdView );
	return( tdView );

err8:	rec_buf_free( tdView );
err7:	rec_cntl_free( tdView );
err6:	svc_tbl_free( tdView ); scmp = 0;
err5:	if(scmp) neo_free( scmp );
err4:
//err3:
/*err2:*/	werr=neo_errno;prc_close_from_tables( tblp, 1 );neo_errno=werr;
err1:
	DBG_EXIT( 0 );
	return( 0 );
}

int
prc_join_loop( nt, tblp )
	int		nt;
	r_from_tables_t	*tblp;
{
	int		N_INC = 100;
	int		r, pn;
	r_tbl_t		*tdp;
	int		save;

	DBG_ENT( M_SQL, "prc_join_loop" );

	if( nt >= tblp->f_n ) { /* process join terminate */

		if( prc_join_evaluate( tblp, tblp->f_joinp, tblp->f_wherep ) ) {

			int	*workp;

			if( tblp->f_num >= tblp->f_result_size ) {
				tblp->f_result_size	+= N_INC;
				workp	= (int*)neo_malloc( sizeof(int)*tblp->f_n
								*tblp->f_result_size );
				if( tblp->f_resultp ) {
					if( tblp->f_num > 0 ) {
						memcpy( workp, tblp->f_resultp, 
								sizeof(int)*tblp->f_n*tblp->f_num );
					}
					neo_free( tblp->f_resultp );
				}
				tblp->f_resultp	= workp;
			}
			memcpy( tblp->f_resultp + tblp->f_n*tblp->f_num++,
						tblp->f_joinp, sizeof(int)*tblp->f_n );
//DBG_PRT("f_num=%d\n", tblp->f_num );
		}
		DBG_EXIT( 0 );
		return( 0 );
	}

	save = tblp->f_joinp[nt];
//DBG_PRT("nt=%d save=%d\n", nt, save );

	if( save >= 0 ) {

		if( prc_join_loop( nt+1, tblp ) )
			goto err1;

	} else {

		tdp = tblp->f_t[nt].f_tdp;

		for( r = 0; r < tdp->t_rec_num; r++ ) {

               		if( !(tdp->t_rec[r].r_stat & R_REC_CACHE) ) {/* load */
                		pn      = R_PAGE_SIZE/tdp->t_rec_size;
                        	svc_load( tdp, r, pn );
			}
			if( !(tdp->t_rec[r].r_stat & R_USED ) ) {
				continue;	/* skip evaluate */
			}
			tblp->f_joinp[nt] = r;

			if( prc_join_loop( nt+1, tblp ) )
				goto err1;

		}
		tblp->f_joinp[nt] = save;	/* loop again */
	}
	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );	
}

int
prc_join_evaluate( tblp, join_rec, wherep )
	r_from_tables_t	*tblp;
	int		join_rec[];
	r_expr_t	*wherep;
{
	int		nTable;
	r_tbl_t		*tdp;
	int		pn;
	int		i, r;
	int		ret;
	r_head_t	*rp;

	DBG_ENT( M_SQL, "prc_join_evaluate" );

	nTable = tblp->f_n;

	for( i = 0; i < nTable; i++ ) {

		tdp = tblp->f_t[i].f_tdp;
		r = join_rec[i];
//DBG_PRT("[%s-%d]\n", tdp->t_name, r );

		ASSERT( r >= 0 );

        if( !(tdp->t_rec[r].r_stat & R_REC_CACHE) ) {      /* load */
        	pn      = R_PAGE_SIZE/tdp->t_rec_size;
            svc_load( tdp, r, pn );
		}
		if( !(tdp->t_rec[r].r_stat & R_USED ) ) {
			goto false;
		}
		tblp->f_recpp[i]	= tdp->t_rec[r].r_head;
		if( (rp = tdp->t_rec[r].r_head)->r_Cksum64 ) {
			assert( !in_cksum64( rp, tdp->t_rec_size, 0 ) );
		}
	}

	/*
	 *	Now check
	 */
	if( wherep )
		ret = sql_eval_expr( wherep, tblp->f_recpp );
	else
		ret = 1;

//DBG_PRT("ret=%d\n", ret );
	DBG_EXIT( ret );
	return( ret );
false:
	DBG_EXIT( 0 );
	return( 0 );
}


#ifdef DEBUG

print_join( tdView )
	r_tbl_t	*tdView;
{
	int	i;

	for( i = 0; i < tdView->t_rec_num; i++ ) {
		print_record( i, &tdView->t_rec[i], 
			tdView->t_scm->s_n, tdView->t_scm->s_item );
	}
}

#endif
