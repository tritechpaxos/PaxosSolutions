/*******************************************************************************
 * 
 *  prc_view.c --- xjPing (On Memory DB) table view Library programs
 * 
 *  Copyright (C) 2011,2015-2016 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"neo_db.h"
#include	"join.h"
#include	"../sql/sql.h"

extern r_expr_t	*sql_read_tree();
extern r_expr_t	*sql_getnode();

r_tbl_t	*prc_view_concatinate();


/*
 *	view_table.scm
 */	
int
sql_create_view( root, statement )
	r_expr_t	*root;
	char		*statement;
{
	int		fd;
	int		n;
	r_expr_t	*view_name_ep;
	r_tbl_name_t	name;

	DBG_ENT(M_SQL,"sql_create_view");

	sql_drop_view( root );
	/*
	 *	View table
	 */
	if(!(view_name_ep = sql_getnode( root, 0, SVIEW ))) {
		goto err1;
	}
	if(!(view_name_ep = sql_getnode( view_name_ep, 0, TLEAF ))) {
		goto err1;
	}

	if( strlen( view_name_ep->e_tree.t_unary.v_value.v_str.s_str)
		>= R_TABLE_NAME_MAX ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	bzero( name, sizeof(name) );
	strncpy( name, view_name_ep->e_tree.t_unary.v_value.v_str.s_str,
			sizeof(name)-sizeof(R_VIEW_SCHEME_SUFFIX)-1 );
	strcat( name, R_VIEW_SCHEME_SUFFIX );

	if( !(fd = f_create( name )) ) {
		goto err2;
	}
	n = strlen( statement );
	if( write( fd, &n, sizeof(n) ) < 0 ) {
		goto err2;
	}
	if( write( fd, statement, n ) < 0 ) {
		goto err2;
	}

	sql_write_tree( fd, root );

	close( fd );

	DBG_EXIT( 0 );
	return( 0 );

err2:	close( fd );
err1:
	DBG_EXIT(neo_errno);
	return(neo_errno);
}

int
sql_drop_view( root )
	r_expr_t	*root;
{
	r_expr_t	*tablep;
	char		*table;
	r_tbl_name_t	name;

	DBG_ENT(M_RDB, "sql_drop_view");

	tablep = sql_getnode( root, 0, TLEAF );
	table = tablep->e_tree.t_unary.v_value.v_str.s_str;

	if( sql_drop_table( root ) ) {
		goto err1;
	}
	strcpy( name, table );
	strcat( name, R_VIEW_SCHEME_SUFFIX );

	if( f_unlink( name ) ) {
		goto err1;
	}

	DBG_EXIT( 0 );
	return( 0 );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

r_view_scm_t*
prc_view_scm_alloc( viewTable )
	r_tbl_name_t	viewTable;
{
	r_view_scm_t	*scmp;
	int		fd;
	r_tbl_name_t	viewScm;
	int		n;

	DBG_ENT(M_SQL,"prc_view_scm_alloc");

	strcpy( viewScm, (char*)viewTable );
	strcat( viewScm, R_VIEW_SCHEME_SUFFIX );

	if( (fd = f_open( viewScm ) ) < 0 ) {
		goto err1;
	}

	if( read( fd, &n, sizeof(n) ) < 0 ) {
		goto err2;
	}
	if( lseek( fd, (off_t)n, SEEK_CUR ) < 0 ) {
		goto err2;
	}
	
	if( !(scmp = (r_view_scm_t*)neo_zalloc( sizeof(r_view_scm_t) ) ) ) {
		goto err2;
	}
	if( !(scmp->v_root = sql_read_tree( fd ) ) ) {
		goto err3;
	}

	close( fd );

	DBG_EXIT( scmp );
	return( scmp );

err3:	neo_free( scmp );
err2:	close( fd );
err1:
	DBG_EXIT(0);
	return( 0 );
}

int
prc_view_scm_free( scmp )
	r_view_scm_t	*scmp;
{
	DBG_ENT(M_SQL,"prc_view_scm_free");

	sql_free( scmp->v_root );

	neo_free( scmp );

	DBG_EXIT(0);
	return( 0 );
}

int
prc_view_update( tdp )
	r_tbl_t	*tdp;
{
	int		i, j;
	r_view_scm_t	*vscmp;
	r_from_tables_t	*tblp;
	int		ret;
	r_expr_t	*fp;

	DBG_ENT(M_SQL,"prc_view_update");

	if( !(vscmp = prc_view_scm_alloc( tdp->t_name ) ) ) {
		goto err1;
	}

	tdp->t_usr_cnt++;	/* Guard tdp */

	fp = vscmp->v_root;
	j = 0;

	while( (fp = sql_getnode( vscmp->v_root, fp, SFROM )) ) {

		if( !(tblp = prc_open_from_tables( fp ) ) ) {
			goto err3;
		}
		for( i = 0; i < tblp->f_n; i++ ) {
			if( tdp->t_depend->l_tbl[j].m_time 
				!= tblp->f_t[i].f_tdp->t_modified 
			|| tdp->t_depend->l_tbl[j].m_version 
				!= tblp->f_t[i].f_tdp->t_version ) {

				prc_close_from_tables( tblp, 1 );
				ret = 1;
				goto update;
			}
			j++;
		}
		prc_close_from_tables( tblp, 1 );
	}
	ret = 0;
update:
	tdp->t_usr_cnt--;

	prc_view_scm_free( vscmp );

	DBG_EXIT( ret );
	return( ret );

err3:	tdp->t_usr_cnt--;
		prc_view_scm_free( vscmp );
err1:	DBG_EXIT( -1 );
	return( -1 );
}

int
prc_view_file( name )
	char	*name;
{
	r_view_scm_t	*vscmp;
	int		ret;

	DBG_ENT( M_SQL, "prc_view_file" );

	if( !(vscmp = prc_view_scm_alloc( name ) ) ) {
		goto err;
	}

	ret = prc_view_create( name, vscmp->v_root );
	if( ret )
		goto err1;

	prc_view_scm_free( vscmp );

	DBG_EXIT( 0 );
	return( 0 );

err1:	prc_view_scm_free( vscmp );
err:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
prc_view_create( viewTable, root )
	r_tbl_name_t	viewTable;
	r_expr_t	*root;
{
	r_tbl_t		*tdView, *tdp;
	int		rec_used;
	r_expr_t	*selectp, *fp, *sp, *wp, *op;

	DBG_ENT( M_SQL, "prc_view_create" );

	tdView	= 0;
	selectp	= root;
	while( (selectp = sql_getnode( root, selectp, SSELECT )) ) {

		if( !(sp = sql_getnode( selectp, 0, '*' ) ) ) {
			goto err1;
		}
		if( !(fp = sql_getnode( selectp, 0, SFROM ) ) ) {
			goto err1;
		}
		wp = sql_getnode( selectp, 0, SWHERE );
		op = sql_getnode( selectp, 0, SORDER );

		if( !(tdp = prc_join( fp, wp, sp, op ) ) ) {
			goto err1;
		}
		if( tdView ) {
			if( !(tdView = prc_view_concatinate(tdView,tdp) ) ) {
				rec_buf_free( tdp );
				rec_cntl_free( tdp );
				svc_tbl_free( tdp );
				goto err2;
			}
		} else {
			tdView = tdp;
		}
	}
	rec_used = tdView->t_rec_used;

	/*
	 *	Create View Table
	 */
	TBL_DROP( viewTable );

	if( TBL_CREATE( viewTable, R_TBL_VIEW, (long)time(0),
		tdView->t_rec_num, tdView->t_scm->s_n, tdView->t_scm->s_item,
		tdView->t_depend->l_n, tdView->t_depend->l_tbl ) ) {
		goto err2;
	}

	neo_free( tdView->t_scm );
	tdView->t_scm = 0;
	neo_free( tdView->t_depend );
	tdView->t_depend = 0;

	if( TBL_OPEN( tdView, viewTable ) ) {
		goto err2;
	}
	/*
	 *	store data
	 */
	if( svc_store( tdView, 0, tdView->t_rec_num, 0 ) ) {
		goto err2;
	}

/***
	tdView->t_rec_used	= tdView->t_rec_num;
	tdView->t_rec_ori_used	= tdView->t_rec_num;
***/
	tdView->t_rec_used	= rec_used;
	tdView->t_rec_ori_used	= tdView->t_rec_used;

	if( TBL_CLOSE( tdView ) ) {
		goto err2;
	}

	DBG_A(("View[%s] is created.\n", viewTable ));

	/***
	drop_td( tdView );
	***/
	rec_buf_free( tdView );
	rec_cntl_free( tdView );
	svc_tbl_free( tdView );

	DBG_EXIT( 0 );
	return( 0 );

err2:	/*drop_td( tdView );*/
	if( tdView->t_scm )	neo_free( tdView->t_scm );
	rec_buf_free( tdView );
	rec_cntl_free( tdView );
	svc_tbl_free( tdView );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

r_tbl_t*
prc_view_concatinate( tdp1, tdp2 )
	r_tbl_t	*tdp1;
	r_tbl_t	*tdp2;
{
	r_scm_t		*scmp1;
	r_scm_t		*scmp2;
	int		i;
	r_rec_t		*rp;
	r_tbl_list_t	*dp;

	DBG_ENT( M_SQL, "prc_view_concatinate" );

	/*
	 *	Schema check
	 */
	scmp1 = tdp1->t_scm;
	scmp2 = tdp2->t_scm;
	if( scmp1->s_n != scmp2->s_n ) {
		neo_errno = E_SQL_SCHEMA;
		goto err1;
	}
	for( i = 0; i < scmp1->s_n; i++ ) {
		if( scmp1->s_item[i].i_attr != scmp2->s_item[i].i_attr
		|| scmp1->s_item[i].i_size != scmp2->s_item[i].i_size ) {
			neo_errno = E_SQL_SCHEMA;
			goto err1;
		}
	}
	/*
	 *	Concatinate
	 */
	if( !(rp = (r_rec_t*)neo_zalloc( sizeof(r_rec_t)
			*(tdp1->t_rec_num + tdp2->t_rec_num) ) ) ) {
			neo_errno = E_RDB_NOMEM;
			goto err1;
	}
	if( !(dp = (r_tbl_list_t*)neo_zalloc(sizeof(r_tbl_list_t)
		+ sizeof(r_tbl_mod_t)
			*(tdp1->t_depend->l_n + tdp2->t_depend->l_n) ) ) ) {
			neo_errno = E_RDB_NOMEM;
			goto err2;
	}
	/* record cntl */
	bcopy( tdp1->t_rec, &rp[0], sizeof(r_rec_t)*tdp1->t_rec_num );
	bcopy( tdp2->t_rec, &rp[tdp1->t_rec_num], 
				sizeof(r_rec_t)*tdp2->t_rec_num );

	neo_free( tdp1->t_rec );
	tdp1->t_rec = rp;
	tdp1->t_rec_num		+= tdp2->t_rec_num;
	tdp1->t_rec_used	+= tdp2->t_rec_used;

	neo_free( tdp2->t_rec );

	/* depend tables */
	bcopy( &tdp1->t_depend->l_tbl[0], &dp->l_tbl[0], 
			sizeof(r_tbl_mod_t)*tdp1->t_depend->l_n );
	bcopy( &tdp2->t_depend->l_tbl[0], &dp->l_tbl[tdp1->t_depend->l_n], 
			sizeof(r_tbl_mod_t)*tdp2->t_depend->l_n );
	dp->l_n = tdp1->t_depend->l_n + tdp2->t_depend->l_n;

	neo_free( tdp1->t_depend );
	tdp1->t_depend = dp;
	
	/* free tdp2 */
	svc_tbl_free( tdp2 );

	DBG_EXIT( tdp1 );
	return( tdp1 );

err2:	neo_free( rp );
err1:
	DBG_EXIT( 0 );
	return( 0 );
}
