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
 *	svc_direct.c
 *
 *	説明	R_CMD_DIRECT	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_direct()
 *
 *  機能	レコード絶対番号でレコードを取得する
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_direct( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int		len;
	r_req_direct_t	req;
	r_res_direct_t	*resp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		abs;
	int		mode;

DBG_ENT(M_RDB,"svc_direct");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 * レコード絶対番号チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.d_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.d_name );

	mode	= req.d_mode;
	abs	= req.d_abs;

	if( (neo_errno = check_td_ud( td, ud )) ||
	    (neo_errno = 
		check_lock( td, ud, ((mode&R_LOCK) ? R_WRITE : R_READ) )) ) {
		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err1;
	}

	if( abs < 0 || td->t_rec_num <= abs ) {
		err_reply( pd, hp, E_RDB_INVAL );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						DIRECTERR01, neo_errsym() );
		goto err1;
	}

	/*
	 * リプライバッファのメモリアロック
	 * 	( R_CMD_DIRECT応答パケットが可変長)
	 */
	len = sizeof(r_res_direct_t) + td->t_rec_size;
	if( !(resp = (r_res_direct_t *)neo_malloc( len )) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * レコードバッファのチェック
	 */
	if( !(td->t_rec[abs].r_stat & R_REC_CACHE) )	/* load */
			svc_load( td, abs, R_PAGE_SIZE/td->t_rec_size+1 );
		
	if( !(td->t_rec[abs].r_stat & R_USED ) ) {
		err_reply( pd, hp, E_RDB_NOEXIST );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						DIRECTERR01, neo_errsym() );
		goto err2;
	}

	/*
	 * 排他モードが設定されていれば、排他をかける
	 */
	if ( mode & R_LOCK )  {

		/* 
		 * レコード排他チェック
		 */
		if( (td->t_rec[abs].r_stat & R_LOCK)
			&& td->t_rec[abs].r_access != ud ) {

			err_reply( pd, hp, E_RDB_REC_BUSY );
			goto err2;
		}  else {
			td->t_rec[abs].r_stat &= ~(R_LOCK);
			td->t_rec[abs].r_stat |= (mode & R_LOCK);
			td->t_rec[abs].r_access = ud ;
		}
	} else {

		/*
		 * レコード参照排他チェック
		 */
		if( ( td->t_rec[abs].r_stat & R_EXCLUSIVE )
			&& td->t_rec[abs].r_access != ud ) {
			err_reply( pd, hp, E_RDB_REC_BUSY );
			neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ), 
						DIRECTERR02, neo_errsym() );
			goto err2;
		} 

		if( ( td->t_rec[abs].r_stat & R_LOCK )
			&& td->t_rec[abs].r_access == ud ) {
			
			td->t_rec[abs].r_stat &= ~R_LOCK;
			svc_rec_post( td, abs );
		}
	}

	bcopy( td->t_rec[abs].r_head, resp+1, td->t_rec_size );

	/*
	 * リプライ
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)resp, len, 0 );

	if( p_reply( pd, (p_head_t*)resp, len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}
	
	neo_free( resp );

DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
