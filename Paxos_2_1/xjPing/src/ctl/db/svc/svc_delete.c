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
 *	説明	R_CMD_DELETE	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"./hash.h"
#include	<inttypes.h>

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
svc_delete( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int	len;
	r_req_delete_t	*pReq;
	r_res_delete_t	res;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i, j, k, n;
	r_head_t	*rp;

DBG_ENT(M_RDB,"svc_delete");

	/*
	 * 要求パケット受信用バッファを生成する
	 */
	len = hp->h_len + sizeof(p_head_t);
	if( !(pReq = (r_req_delete_t*)neo_malloc(len) ) ) {

		len = sizeof(p_head_t);
		if( p_recv( pd, hp, &len ) )	/* discard */
			goto err1;
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * リクエストレシーブ
	 */
	if( p_recv( pd, (p_head_t*)pReq, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err2;
	}

	/*
	 *  テーブル/オープンユーザ記述子有効性チェック
	 */

	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, pReq->d_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, pReq->d_name );

	if( (neo_errno = check_td_ud( td, ud ))
			||(neo_errno = check_lock( td, ud, R_WRITE ) ) ) {
		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err2;
	}

	n = ( len - sizeof(r_req_delete_t))/td->t_rec_size;
	for( k = 0, rp = (r_head_t*)(pReq+1); k < n;
			k++, rp = (r_head_t*)((char*)rp + td->t_rec_size) ) {
		/*
		 * レコード範囲/レコードバッファチェック
		 */
//LOG(neo_log,"rel=%"PRIi64" abls=%"PRIi64"", rp->r_rel, rp->r_abs );
		if( ( i = rp->r_abs ) < 0 || td->t_rec_num <= i ) {
			err_reply( pd, hp, E_RDB_INVAL );
			neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						DELETEERR01, neo_errsym() );
			goto err2;
		}

		if ( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 ) ;

		if( !(td->t_rec[i].r_stat & R_USED) ) {
			err_reply( pd, hp, E_RDB_NOEXIST );
			neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						DELETEERR01, neo_errsym() );
			goto err2;
		}

		/*
		 * 排他チェック(自分以外から排他がかけられていない事)
		 */
		if( td->t_rec[i].r_access != ud
			&& td->t_rec[i].r_stat & R_LOCK ) {
			err_reply( pd, hp, E_RDB_REC_BUSY );
			neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
							DELETEERR02, neo_errsym() );
LOG(neo_log,LogDBG,"i=%d r_access=%p ud=%p", i, td->t_rec[i].r_access, ud ); 
			goto err2;
		}

		/*
		 * レコードの削除
		 */
		r_item_t	*ip;
   		char        path[128];
		for(j = 0 ; j < td->t_scm->s_n; j++){
        	ip = &td->t_scm->s_item[j];

			hash_del( td, ip, i );

   	     if(ip->i_attr == R_BYTES){
   	         bcopy((char *)(td->t_rec[i].r_head) + ip->i_off + 4, 
							path, ip->i_len);
   	         if(file_delete(path) != 0){
   	             goto err2;
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
		if( i < td->t_unused_index )
			td->t_unused_index = i;

		td->t_stat	|= R_UPDATE;
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
	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)pReq, (p_head_t*)&res, sizeof(res), 0 );
	res.d_rec_used	= td->t_rec_used;

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}

	neo_free( pReq );
DBG_EXIT(0);
	return( 0 );
err2:
	neo_free( pReq );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
