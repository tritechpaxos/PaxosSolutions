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
 *	svc_find.c
 *
 *	説明		R_CMD_FIND
 *				R_CMD_REWIND	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"y.tab.h"

/*****************************************************************************
 *
 *  関数名	svc_find()
 *
 *  機能	レコードの検索
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_find( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	int	len;
	r_req_find_t	*reqp;
	Msg_t			*pMsg;
	r_res_find_t	*resp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		i;
	int		m, pn;
	r_value_t	v;
	int		cnt, n;
	char		*bufp;
	int		mode;
	r_expr_t	*check;
	int		first = 1;
DBG_ENT(M_RDB,"svc_find");

	/*
	 * リクエストレシーブ
	 */
	if( !(pMsg = p_recvByMsg( pd )) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}
	reqp	= (r_req_find_t*)MsgFirst( pMsg );

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	mode	= reqp->f_mode;

	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->f_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->f_name );

DBG_B(("td[0x%x]ud[0x%x]mode[0x%x]\n", td, ud, mode ));

	if( (neo_errno = check_td_ud( td, ud )) || 
	    (neo_errno = 
		check_lock( td, ud, ((mode&R_LOCK) ? R_WRITE : R_READ) )) ) {
		err_reply( pd, hp, neo_errno );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
						RDBCOMERR09, neo_errsym() );
		goto err2;
	}

	/*
	 * リプライ用のバッファのメモリアロック
	 * 取得されたメモリは R_PAGE_SIZEを最大値とする
	 */
	if( R_PAGE_SIZE >= td->t_rec_size ) {
		pn	= R_PAGE_SIZE/td->t_rec_size;
		m	= reqp->f_num < pn ? reqp->f_num : pn;
	} else {
		pn	= m = 1;
	}
	len = sizeof(r_res_find_t) + m*td->t_rec_size;
	if( !(resp = (r_res_find_t *)neo_malloc( len )) ) {
		
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() );
		goto err2;
	}
    resp->bytesflag = 0;
    check	= ud->u_search.s_expr;
	cnt = 0;	/* トータル取得レコード数の初期化 */
loop:

	/*
	 * 検索 ( for )
	 *      初期化:  コピー用バッファのポインタ(bufp)を
	 *		 レスポンス用バッファのレコード領域の先頭に設定
	 *		 取得レコード数(i)を0に初期化
	 *	終了条件:カーソル位置(i)がテーブルの最後に来るまで
	 *	動作:	 カーソルを一つ進める
	 *
	 *	*リプライの送信は R_PAGE_SIZEをレコードバッファの最大値
	 *	 とする、従って、1回で送信できない時は、分割して送られる
	 *	 その時、(n)は、バッファに設定されているレコードの数
	 *	 (f_num)で、まだ残りがあるとき、f_moreに1を設定する
	 *
	 */
	for( bufp = (char *)(resp+1), n = 0;
		(i = ud->u_cursor) < td->t_rec_num; ud->u_cursor++ ) {

		if( !(td->t_rec[i].r_stat & R_REC_CACHE) )	/* load */
			svc_load( td, i, pn );
		
		if( !(td->t_rec[i].r_stat & R_USED ) )
			continue;

		if( td->t_rec[i].r_head->r_Cksum64 ) {
			assert( !in_cksum64( td->t_rec[i].r_head, td->t_rec_size, 0 ) );
		}
		/*
		 * 検索条件が設定してあった場合は、条件を評価する
		 * 		( rdb_evaluate()は clnt/rdb_expr.c )
		 */
		if( check ) {
			if( rdb_evaluate( check, (char*)td->t_rec[i].r_head, &v ) 
					|| v.v_type != VBOOL ) {

				err_reply( pd, hp, E_RDB_SEARCH );

				goto err3;
			}
		}

		/*
		 * 検索条件が成立するレコードが見つかった
		 */
		if( !check || v.v_value.v_int ) {	/* found */

			/*
			 * 排他モードが設定されていれば、排他をかける
			 */
			if ( mode & R_LOCK )  {

				/* 
				 * レコード排他チェック
				 */
				if( (td->t_rec[i].r_stat & R_LOCK)
					&& td->t_rec[i].r_access != ud ) {

					goto	record_lock ;
				}  else {
					td->t_rec[i].r_stat &= ~(R_LOCK);
					td->t_rec[i].r_stat |= (mode & R_LOCK);
					td->t_rec[i].r_access = ud ;
					if( first ) {
						first = 0;
						if( !HashGet( &td->t_HashRecordLock, ud ) ) {
							HashPut( &td->t_HashRecordLock, ud, ud );
						}
					}
				}

			} else {

				/*
				 * レコード参照排他チェック
				 */
				if( ( td->t_rec[i].r_stat & R_EXCLUSIVE )
					&& td->t_rec[i].r_access != ud ) {

					goto record_lock;
				} 
			}

			/*
			 * レコードデータのコピー&取得レコード数のカウント
			 */
			bcopy( td->t_rec[i].r_head, bufp, td->t_rec_size );
			((r_head_t *)bufp)->r_stat	= td->t_rec[i].r_stat;
			bufp += td->t_rec_size;
			n++; cnt++;

			/*
			 * トータル取得レコード数が、要求レコード数に
			 * 達したらリプライを返し、追加(f_more)は無し
			 */
			if( cnt >= reqp->f_num ) {

				resp->f_more = 0;

				ud->u_cursor++;

				goto reply;
			}

			/*
			 * リプライ1回あたりの最大送信レコード数(m)よりも
			 * 取得レコード数(n)が小さい時は再検索
			 */
			if( n < m )
				continue;
			
			/*
			 * トータル取得レコード数が、要求レコード数に
			 * 達していないので、追加(f_more)を行う
			 */
			resp->f_more = 1;

			ud->u_cursor++;

			LOG(neo_log,LogDBG,"i=%d stat=0x%x", i, td->t_rec[i].r_stat );
			goto reply;
		} else
			continue;
	}

	/*
	 * カーソルがテーブルの最後に来た(もう該当するレコード無し)
	 */
	resp->f_more	= 0;

    
	/*
	 * リプライ
	 */
reply:
	//set bytes flag
    if(n > 0){
        for(i = 0 ; i < td->t_scm->s_n; i++){
            if(td->t_scm->s_item[i].i_attr == R_BYTES){
                resp->bytesflag = 2;//2 means have bytes data to send to JDBC
                break;
            }
        }
    }
    
	len = sizeof(r_res_find_t) + n*td->t_rec_size;

	reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

	resp->f_num	= n;

	if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}
	
	/*
	 * 追加(f_more)が必要な時は、再検索
	 */
	if( resp->f_more )
		goto loop;

DBG_B(("ret(%d)\n", 0 ));

	MsgDestroy( pMsg );
DBG_EXIT(0);
	return( 0 );

record_lock:

	/*
	 * 排他がかかっていて、参照や排他ができない時、エラーとなる
	 * 既にレコードを取得したら、そこまでを送信する、追加(f_more)
	 * を行わせる
	 */
	if ( n ) {
		resp->f_more	= 1;
		goto	reply;
	} else 
		err_reply( pd, hp, E_RDB_REC_BUSY ) ;

err3:	neo_free( resp );
err2:	MsgDestroy( pMsg );
err1: 
DBG_B(("ret(%s)\n", neo_errsym()));
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_rewind()
 *
 *  機能	カーソルを先頭に戻す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_rewind( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	r_req_rewind_t	*reqp;
	Msg_t			*pMsg;
	r_res_rewind_t	*resp;
	r_tbl_t		*td;
	r_usr_t		*ud;
DBG_ENT(M_RDB,"svc_rewind");

	/*
	 * リクエストレシーブ
	 */
	if( !(pMsg = p_recvByMsg( pd ) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}
	reqp = (r_req_rewind_t*)MsgFirst( pMsg );

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->r_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->r_name );

	if( (neo_errno = check_td_ud( td, ud )) ) {
		err_reply( pd, hp, neo_errno );
		goto err2;
	}

	/*
	 * カーソルを0に設定
	 */
DBG_A(("cursor=%d\n", ud->u_cursor ));
	ud->u_cursor	= 0;

	/*
	 * リプライ
	 */
	resp = (r_res_rewind_t*)neo_malloc( sizeof(*resp) );
	reply_head( (p_head_t*)reqp, (p_head_t*)resp, sizeof(*resp), 0 );
	if( p_reply_free( pd, (p_head_t*)resp, sizeof(*resp) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err2;
	}

	MsgDestroy( pMsg );
DBG_EXIT(0);
	return( 0 );

err2:
	MsgDestroy( pMsg );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

/*
 *	seek to absolue record which is active.
 */
int
svc_seek( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子     */
	p_head_t	*hp;		/* 要求パケットヘッダ   */
{
	r_req_seek_t	*reqp;
	Msg_t			*pMsg;
	r_res_seek_t	*resp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	int		seek;
	r_expr_t	*check;
	int		i, cnt;
	int		pn;
	r_value_t	v;
DBG_ENT(M_RDB,"svc_seek");

	/*
	 * リクエストレシーブ
	 */
	if( !(pMsg = p_recvByMsg( pd ) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}
	reqp = (r_req_seek_t*)MsgFirst( pMsg );

	/*
	 * テーブル/オープンユーザ記述子有効性チェック
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, reqp->r_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, reqp->r_name );

	if( (neo_errno = check_td_ud( td, ud )) ) {
		err_reply( pd, hp, neo_errno );
		goto err2;
	}

	/*
	 * カーソルを設定
	 */
	seek = reqp->r_seek;
DBG_A(("seek=%d\n", seek ));
	if( seek < 0 || td->t_rec_num < seek ) {
		neo_errno = E_RDB_INVAL;
		err_reply( pd, hp, neo_errno );
		goto err1;
	}
	check	= ud->u_search.s_expr;

	for( i = 0, cnt = 0; i < td->t_rec_num && cnt < seek; i++ ) {

		if( !(td->t_rec[i].r_stat & R_REC_CACHE) )	{ /* load */
			pn	= R_PAGE_SIZE/td->t_rec_size;
			svc_load( td, i, pn );
		}
		
		if( !(td->t_rec[i].r_stat & R_USED ) )
			continue;

		/*
		 * 検索条件が設定してあった場合は、条件を評価する
		 * 		( rdb_evaluate()は clnt/rdb_expr.c )
		 */
		if( check ) {
			if( rdb_evaluate( check, (char*)td->t_rec[i].r_head, &v ) 
					|| v.v_type != VBOOL ) {

				err_reply( pd, hp, E_RDB_SEARCH );

				goto err1;
			}
		}

		/*
		 * 検索条件が成立するレコードが見つかった
		 */
		if( !check || v.v_value.v_int ) {	/* found */
			cnt++;
		}
	}
DBG_A(("cursor=%d\n", ud->u_cursor ));
	ud->u_cursor	= i;

	/*
	 * リプライ
	 */
	resp	= (r_res_seek_t*)neo_malloc( sizeof(*resp) );
	reply_head( (p_head_t*)reqp, (p_head_t*)resp, sizeof(*resp), 0 );
	if( p_reply_free( pd, (p_head_t*)resp, sizeof(*resp) ) ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() );
		goto err1;
	}

	MsgDestroy( pMsg );
DBG_EXIT(0);
	return( 0 );

err2:
	MsgDestroy( pMsg );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}
