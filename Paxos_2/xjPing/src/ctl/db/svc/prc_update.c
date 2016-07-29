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
#include	"join.h"
#include	"hash.h"
#include	"../sql/sql.h"

r_expr_t	*sql_getnode();

int
prc_update( tblp, sp, wp, mode, updatedp, buffpp )
	r_from_tables_t	*tblp;
	r_expr_t	*sp;
	r_expr_t	*wp;
	int		mode;
	int		*updatedp;
	char		**buffpp;
{
	int	len;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		    i;
	r_head_t	*rp;
	r_expr_t	*sp1;
	char		*buffp;
	r_res_sql_update_t *up;
	char		*firstp = 0;
	int         *bytesflag;
    
DBG_ENT(M_SQL,"prc_update");

	*updatedp = 0;
	/*
	 * 指定したテーブルとオープンユーザの有効チェック
	 */
	td = tblp->f_t[0].f_tdp;
	ud = tblp->f_t[0].f_udp;

	if( (neo_errno = check_td_ud( td, ud ) )
		||( (neo_errno = check_lock( td, ud, R_WRITE ) ) ) ) {
		goto err1;
	}
	tblp->f_wherep	= wp;
	if( xj_join_by_hash( tblp ) ) {
		goto err1;
	}
	/*
	 *	Alloc return area
	 */
	if( !(buffp = (char*)sql_mem_malloc(
			len = (
			sizeof(r_res_sql_t)	/* response head */
			+ sizeof(int) /* whether have bytes field: 1 means have, 0 means no */
			+ sizeof(r_res_sql_update_t)
			+ sizeof(r_scm_t)
			+ sizeof(r_item_t)*(td->t_scm->s_n-1)
			+ td->t_rec_size )	/* the first found record */
		) ) ) {
		goto err1;
	}
	((p_head_t*)buffp)->h_len = len - sizeof(p_head_t);
	bytesflag = (int *)(buffp+sizeof(r_res_sql_t));
	*bytesflag = 0; //0 means no bytes field
	
	up	= (r_res_sql_update_t*)(buffp+sizeof(r_res_sql_t)+sizeof(int));
	bcopy( td->t_scm, (char*)(up+1),
		(len = sizeof(r_scm_t)+sizeof(r_item_t)*(td->t_scm->s_n-1) ) );

	firstp = (char*)(up+1) + len; 

	/*
	 *	SSET expression
	 */
	sp1 = sp;
	while( (sp1 = sql_getnode( sp, sp1, SSET )) ) {
		if( sql_create_expr( sp1->e_tree.t_binary.b_r, tblp ) ) {
			goto err2;
		}
	}

	int	j;
	for( j = 0 ; j < tblp->f_num; j++ ) {

		i = tblp->f_resultp[j];

		 if ( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 ) ;

		if( !(td->t_rec[i].r_stat & R_USED ) ) {
			continue;
		}

		/*
		 * 更新対象レコードがロック中(他ユーザにより)
		 * かどうかをチェックする
		 */
		if( td->t_rec[i].r_stat & R_LOCK
			&& td->t_rec[i].r_access != ud ) {

			neo_errno = E_RDB_REC_BUSY ;

			goto err2;
		}
		rp = td->t_rec[i].r_head;

		if( *updatedp == 0 && firstp ) {
			bcopy( rp, firstp, td->t_rec_size );
		}

		sp1 = sp;
		while( (sp1 = sql_getnode( sp, sp1, SSET )) ) {

			hash_del( td, sp1->e_left->e_value.v_value.v_item, i );

			if( sql_set_item( sp1->e_tree.t_binary.b_l->
						e_tree.t_unary.v_value.v_item,
					sp1->e_tree.t_binary.b_r,
					(char*)rp ) ) {
				hash_add( td, sp1->e_left->e_value.v_value.v_item, i );
				goto err2;
			}
			hash_add( td, sp1->e_left->e_value.v_value.v_item, i );
		}

		td->t_rec[i].r_head->r_stat |= R_UPDATE ;
		td->t_rec[i].r_stat |= R_UPDATE | R_BUSY;
		td->t_rec[i].r_access	= ud;
		if( rp->r_Cksum64 ) {
			rp->r_Cksum64	= 0;
			rp->r_Cksum64	= in_cksum64( rp, td->t_rec_size, 0 ); 
		}
		(*updatedp)++;

		/*
		 * モードでロックを指定した場合には、該当レコード
		 * の状態に設定する。そうでない場合には、該当レコード
		 * のロック状態(もしあれば)を自動的に解除して、
		 * 待ちユーザに通知する
		 */
		if( mode & R_LOCK ) {
			td->t_rec[i].r_stat 	&= ~R_LOCK ;
			td->t_rec[i].r_stat 	|= (mode & R_LOCK );

		} else if( td->t_rec[i].r_stat & R_LOCK ) {

			td->t_rec[i].r_stat &= ~R_LOCK;

			svc_rec_post( td, i );
		}
	}
	td->t_modified	= time(0);
	td->t_version++;

	ud->u_update	= *updatedp;
	up->u_updated	= *updatedp;
	up->u_num	= 1;
	up->u_rec_size	= td->t_rec_size;
    if(*updatedp != 0){
        for(i = 0 ; i < td->t_scm->s_n; i++){
            if(td->t_scm->s_item[i].i_attr == R_BYTES){
                *bytesflag = 1;//1 means have bytes data
                break;
            }
        }
    }

	*buffpp	 = buffp;

DBG_EXIT(0);
	return( 0 );

err2:	sql_mem_free( buffp );	
		prc_close_from_tables( tblp, 0 );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
