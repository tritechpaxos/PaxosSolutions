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
 *	svc_update.c
 *
 *	説明	R_CMD_UPDATE	procedure
 * 
 ******************************************************************************/


#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	svc_update()
 *
 *  機能	クライアント側要求したレコードを更新して、
 *		応答を返す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_update( pd, hp )
	p_id_t		*pd;	/* 通信ポート記述子	*/
	p_head_t	*hp;	/* 要求パケットヘッダ	*/
{
	int	len;
	r_req_update_t	*reqp;
	r_res_update_t	res;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i,j;
	r_head_t	*rp, *rp1;
	int		mode;
	int		n;		/* numbers of record update */

DBG_ENT(M_RDB,"svc_update");

	/*
	 * 要求パケット受信用バッファを生成する
	 */
	len = hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_req_update_t*)neo_malloc(len) ) ) {

		len = sizeof(p_head_t);
		if( p_recv( pd, hp, &len ) )	/* discard */
			goto err1;
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * 要求パケットを読み出す
	 */
	if( p_recv( pd, (p_head_t*)reqp, &len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err2;
	}

	/*
	 * 指定したテーブルとオープンユーザの有効チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->u_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->u_name );
	HashClear( &td->t_HashEqual );

	mode	= reqp->u_mode;
	n	= reqp->u_numbers;

	if( (neo_errno = check_td_ud( td, ud ) )
		||( (neo_errno = check_lock( td, ud, R_WRITE ) ) ) ) {
		err_reply( pd, hp, neo_errno );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR09, neo_errsym() );
		goto err2;
	}

	/*
	 * 各更新レコードの絶対番号、更新対象レコード
	 * のロック、存在チェック
	 */
	for( j = 0, rp = (r_head_t*)(reqp+1); j < n;
			j++, rp = (r_head_t*)((char*)rp + td->t_rec_size) ) {

		i = rp->r_abs;

		 if ( !(td->t_rec[i].r_stat & R_REC_CACHE ) )
			svc_load( td, i, R_PAGE_SIZE/td->t_rec_size + 1 ) ;

		/*
		 * 更新用新レコードの絶対番号チェック
		 */
		if( i < 0 || td->t_rec_num <= i ) {

			neo_errno = E_RDB_INVAL;

			goto err_reply;
		}

		/*
		 * 更新対象レコードが存在しているかどうかをチェックする
		 */
		if( !(td->t_rec[i].r_stat & R_USED ) ) {

			neo_errno = E_RDB_NOEXIST ;

			goto err_reply;
		}

		/*
		 * 更新対象レコードがロック中(他ユーザにより)
		 * かどうかをチェックする
		 */
		if( td->t_rec[i].r_stat & R_LOCK
			&& td->t_rec[i].r_access != ud ) {

			neo_errno = E_RDB_REC_BUSY ;

			goto err_reply;
		}
	}

	/*
	 * 更新対象レコードをクライアント側から転送された
	 * 更新用新レコードで更新する
	 */
	for( j = 0, rp = (r_head_t*)(reqp+1); j < n;
			j++, rp = (r_head_t*)((char*)rp + td->t_rec_size) ) {

		i = rp->r_abs;

		bcopy( rp + 1, td->t_rec[i].r_head + 1, 
				td->t_rec_size - sizeof(r_head_t)  );

		td->t_rec[i].r_head->r_stat |= R_UPDATE ;
		td->t_rec[i].r_stat |= R_UPDATE | R_BUSY;
		td->t_rec[i].r_access	= ud;

		if( (rp1 = td->t_rec[i].r_head)->r_Cksum64 ) {
			rp1->r_Cksum64 = 0;
			rp1->r_Cksum64 = in_cksum64( rp1, td->t_rec_size, 0 );
		}
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
	td->t_version++;
	
	/*
	 * 応答パケットを作成し、クライアントに返す
	 */
err_reply:
	reply_head( (p_head_t*)reqp, (p_head_t*)&res, sizeof(res), 0 );
	bcopy( rp, &res.u_rec, sizeof(res.u_rec) );
	res.u_head.h_err = neo_errno;

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}

	neo_free( reqp );

DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_free( reqp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
