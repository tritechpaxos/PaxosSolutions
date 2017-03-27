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
 *	svc_delete.c
 *
 *	説明	server   R_CMD_DELETE	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"join.h"
#include	"hash.h"
#include	"../sql/sql.h"

/*****************************************************************************
 *
 *  関数名	svc_delete()
 *
 *  機能	レコードを削除する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
prc_delete( fp, wp )
	r_from_tables_t	*fp;
	r_expr_t	*wp;
{
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		    i,j;
	int		    deleted = 0;
    r_item_t	*ip;
    char        path[128];

DBG_ENT(M_RDB,"prc_delete");
	/*
	 *  テーブル/オープンユーザ記述子有効性チェック
	 */
	td = fp->f_t[0].f_tdp;
	ud = fp->f_t[0].f_udp;

	if( (neo_errno = check_td_ud( td, ud ))
			||(neo_errno = check_lock( td, ud, R_WRITE ) ) ) {
		goto err1;
	}

	if( wp && sql_create_expr( wp, fp ) ) {
		goto err1;
	}
	fp->f_wherep	= wp;
	if( xj_join_by_hash( fp ) ) {
		goto err1;
	}
	/*
	 * レコード範囲/レコードバッファチェック
	 */
	int	k;
	for( k = 0 ; k < fp->f_num; k++ ) {

		i = fp->f_resultp[k];

	if ( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
		svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 ) ;

	if( !(td->t_rec[i].r_stat & R_USED) ) {
		continue;
	}
	/*
	 * 排他チェック(自分以外から排他がかけられていない事)
	 */
	if( td->t_rec[i].r_access != ud
		&& td->t_rec[i].r_stat & R_LOCK ) {
		neo_errno = E_RDB_REC_BUSY;
		goto err1;
	}

	/*
	 * レコードの削除
	 */
	//if there are bytes data files,delete them first
	for(j = 0 ; j < td->t_scm->s_n; j++){
        ip = &td->t_scm->s_item[j];

		hash_del( td, ip, i );

        if(ip->i_attr == R_BYTES){
            bcopy((char *)(td->t_rec[i].r_head) + ip->i_off + 4, path, ip->i_len);
            if(file_delete(path) != 0){
                goto err1;
            }
            
        }
    } 
	 
	td->t_rec[i].r_stat 	&= ~R_USED;
	td->t_rec[i].r_stat 	|= R_UPDATE;

	td->t_rec[i].r_stat	|= R_BUSY;
	td->t_rec[i].r_access	= ud;

	td->t_rec[i].r_head->r_stat = 0;

	td->t_rec_used--;
	ud->u_added--;

	ud->u_delete++;

	deleted++;

	if( i < td->t_unused_index )
		td->t_unused_index = i;

	td->t_stat	|= R_UPDATE;
	td->t_modified	= time(0);
	td->t_version++;
	/*
	 * レコードロックしていたならば解除する
	 */
	if( td->t_rec[i].r_access == ud 
		&& td->t_rec[i].r_stat & R_LOCK ) {
		td->t_rec[i].r_stat	&= ~R_LOCK;
		svc_rec_post( td, i );
	}
}

DBG_EXIT(0);
	return( 0 );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
