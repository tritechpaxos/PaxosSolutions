/*******************************************************************************
 * 
 *  prc_open.c --- xjPing (On Memory DB)  programs
 * 
 *  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
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
#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"join.h"

r_tbl_t*
prc_get_td( namep )
	char	*namep;
{
	r_tbl_t	*td;

	DBG_ENT(M_RDB,"prc_get_td");

	/*
	 * テーブル記述子領域を生成する
	 */
	if( !(td = svc_tbl_alloc()) ) {
		neo_errno = E_RDB_NOMEM;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * テーブルをオープンする
	 */
	if( TBL_OPEN( td, namep ) ) {
		if( prc_view_file( namep ) ) {
			goto err2;
		}
		if( TBL_OPEN( td, namep ) ) {
			goto err2;
		}
	}

	/*
	 * テーブル用バッファエリアを確報する
	 */
	if( td->t_rec_num > 0 ) {
		if( (neo_errno = rec_cntl_alloc( td )) ) {
			goto err2;
		}

		if( (neo_errno = rec_buf_alloc( td )) ) {
			/*
			 *	Swap out and retry
			 */
			if( xj_swap_schedule( td ) ) {
				goto err3;
			}
			if( (neo_errno = rec_buf_alloc( td )) ) {
				goto err3;
			}
		}
	}

	/*
	 * 該当テーブル記述子を_opn_tblに繋がる
	 */
	_dl_insert( td, &(_svc_inf.f_opn_tbl ));

	HashPut( &_svc_inf.f_HashOpen, td->t_name, td );

	DBG_EXIT( td );
	return( td );

err3:	rec_cntl_free( td );
err2:	_r_tbl_free( td );
err1:
	DBG_EXIT( 0 );
	return( 0 );
}

int
prc_open( reqp, respp, res_len, drop )
	r_req_open_t	*reqp;
	r_res_open_t	**respp;
	int		*res_len;
	int		drop;
{
	r_res_open_t	*resp;
	int		len;
	r_usr_t		*ud;
	r_tbl_t		*td;
	int		new = 0;
	int		ret;
	int		err;

DBG_ENT(M_RDB,"prc_open");
DBG_A(("o_name[%s]\n", reqp->o_name ));

	if ( _svc_inf.f_stat == R_SERVER_STOP ) {
		err = E_RDB_STOP;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR10, neo_errsym() );
		goto err1;
	}

	/*
	 * オープンユーザ領域を生成する
	 */
	if( !(ud = _r_usr_alloc()) ) {
		err = E_RDB_NOMEM;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err1;
	}

	/*
	 * 要求されたテーブルが既にオープンされているかどうか
	 * をチェックする
	 */
	if( (td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->o_name)) ) {
		goto find;
	}

	/*
	 * オープンされていない場合には、まずテーブルドロップを行う
	 */
	if ( drop && (neo_errno = drop_table() ) ) {
		err = E_RDB_NOMEM;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto	err2;
	}

	/*
	 * テーブル記述子領域を生成する
	 */
	if( !(td = prc_get_td( reqp->o_name ) ) ) {
		err = E_RDB_NOEXIST;
		goto err2;
	}
	new = 1;
find:
	if( td->t_type == R_TBL_VIEW ) {
		if( (ret = prc_view_update( td ) ) < 0 ) {
			err = neo_errno;
			goto err3;
		}
		if( ret == 1 ) {	/* updated */
			if( td->t_usr_cnt == 0 ) {

				drop_td( td );

				TBL_DROP( reqp->o_name );

				new = 0;

				if( !(td = prc_get_td( reqp->o_name ) ) ) {
					goto err3;
				}
				new = 1;
			} 
		}
	}

	/*
	 * アクセスタイム更新
	 */
	td->t_access	= time( 0 );

	/*
	 * 該当テーブルのオープンユーザ情報を更新する
	 */
	td->t_usr_cnt++;
	_dl_insert( ud, &td->t_usr );


	/*
	 * 該当オープンユーザの情報を設定する
	 */
	ud->u_mytd	= td;
	ud->u_td	= reqp->o_td;

	/*
	 * 応答パケットを作成
	 */
DBG_A(("s_n=%d\n", td->t_scm->s_n ));
	len = sizeof(r_res_open_t) + sizeof(r_item_t)*(td->t_scm->s_n - 1 );
	if( !(resp = (r_res_open_t*)neo_malloc( len ) ) ) {
		err = E_RDB_NOMEM;
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err4;
	}

	reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );
	resp->o_td		= td;
	resp->o_ud		= ud;
	strncpy( resp->o_name, td->t_name, sizeof(resp->o_name) );
	resp->o_rec_num		= td->t_rec_num;
	resp->o_rec_used	= td->t_rec_used;
	resp->o_rec_size	= td->t_rec_size;
	bcopy( &td->t_scm->s_n, &resp->o_n, 
		sizeof(td->t_scm->s_n) + sizeof(r_item_t)*td->t_scm->s_n );

	*respp		= resp;
	*res_len	= len;
DBG_EXIT(0);
	return( 0 );

err4: close_user( ud ); ud = 0;
err3: if( new ) drop_td( td );
err2: if(ud)	_r_usr_free( ud );
err1:
	neo_errno = err;
DBG_EXIT(neo_errno);
	return( neo_errno );
}


int
prc_close( reqp, resp, flush )
	r_req_close_t	*reqp;
	r_res_close_t	*resp;
	int		flush;
{
	r_tbl_t		*td;
	r_usr_t		*ud;
	int			i;

DBG_ENT(M_RDB,"prc_close");

	/*
	 * 要求されたテーブルの有効チェック
	 */
	td = reqp->c_td;
	ud = reqp->c_ud;

	if( (neo_errno = check_td( td ) )
		|| ( neo_errno = check_ud( ud ) ) ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	if( td->t_stat & T_TMP ) {

		td->t_stat = 0;

		if( close_user( ud ) 
		    || drop_td( td ) ) {
			goto err1;
		}
		goto ret;
	}
	/* moved to drop table */
	/*
	 * テーブルのメモリ映像をディスクに反映する
	 */
	if( flush && (neo_errno = svc_store( td, 0, td->t_rec_num, ud )) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR04, neo_errsym() );
		goto err1;
	}

	/*
	 * レコードのロック(もしあれば)を解除する
	 */
	if( HashGet( &td->t_HashRecordLock, ud ) ) {
//DBG_PRT("RECORD LOCK ud=%p\n", ud );
		HashRemove( &td->t_HashRecordLock, ud );
		for( i = 0; i < td->t_rec_num; i++ ) {
			if(  td->t_rec[i].r_access == ud ) {
				td->t_rec[i].r_access	= 0;

				if( td->t_rec[i].r_stat & R_LOCK  ) {
					td->t_rec[i].r_stat	&= ~R_LOCK;
					svc_rec_post( td, i );
				}
			}
		}
	}

	/*
	 * 自分がロックしたテーブルを解除する
	 */
	if ( td->t_stat & R_LOCK  && td->t_lock_usr == ud ) {
		td->t_stat &= ~R_LOCK;
		td->t_lock_usr = 0;
		svc_tbl_post( td );
	}

	/*
	 *  オープンユーザをクローズする
	 */
DBG_A(("td=0x%x,ud=0x%x\n", td, ud ));
	if( close_user( ud ) ) {
		neo_proc_err( M_RDB, svc_myname, OPENERR02, neo_errsym() );
		goto err1;
	}

ret:
	/*
	 * 応答パケットを作成し、クライアントに返す
	 */
	bcopy( reqp, resp, sizeof(p_head_t) );
	resp->c_head.h_len	= sizeof(*resp) - sizeof(p_head_t);

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

