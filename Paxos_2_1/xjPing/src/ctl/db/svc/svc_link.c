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
 *	svc_link.c
 *
 *	説明		R_CMD_LINK
 *				R_CMD_UNLINK	 procedure
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"hash.h"

/*****************************************************************************
 *
 *  関数名	svc_link()
 *
 *  機能	クライアントの接続要求を応じて、ポートエントリを生成
 *		して、応答を返す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_link( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	r_req_link_t	req;
	r_res_link_t	res;
	r_port_t	*pe;
	int		len;
	extern char *_neo_procbname, _neo_hostname[];
	extern int  _neo_pid;

DBG_ENT(M_RDB,"svc_link");

	/*
	 * 接続要求を読みだす
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() ) ;
		goto err1;
	}

	/*
	 * ポートエントリを生成する
	 */
	if( !(pe = (r_port_t *)neo_zalloc( sizeof(r_port_t) ) ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR03, neo_errsym() ) ;
		goto err1;
	}
	
	/*
	 * ポートエントリを通信ポートへの繋げ、初期化をする
	 */
	_dl_init( &pe->p_usr_ent );
	pd->i_tag	= (void*)pe;

	HashInit( &pe->p_HashUser, PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR,
			NULL, neo_malloc, neo_free, NULL );

	/*
	 * usr_adm_t from client
	 */
	bcopy( &req.l_usr, &pe->p_clnt, sizeof(usr_adm_t) );

	/*
	 * 応答パケット作成、応答を返す
	 */
DBG_A(("reply(pe=0x%x)\n", pe ));
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	neo_setusradm( &res.l_usr, 
		_neo_procbname, _neo_pid, svc_myname, _neo_hostname, PNT_INT32(pd) );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() ) ;
		goto err1;
	}

	neo_rpc_log_msg( "LINK", M_RDB, svc_myname, &pe->p_clnt, "", "" );

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_unlink()
 *
 *  機能	クライアントの切断要求を処理して、応答を返す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_unlink( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子アドレス */
	p_head_t	*hp;		/* 切断要求パケットヘッダ   */
{
	r_req_unlink_t	req;
	r_res_unlink_t	res;
	r_port_t	*pe;
	int		len;

DBG_ENT(M_RDB,"svc_unlink");

	/*
	 * 切断要求を読みだす
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() ) ;
		goto err1;
	}
//DBG_PRT("UNLINK RECV\n");

	/*
	 * ポートエントリがなければ、エラー応答を返す
	 */
	pe = PDTOENT( pd );
	if( !pe ) {
		neo_proc_err( M_RDB, svc_myname, LINKERR01, neo_errsym() ) ;
		err_reply( pd, hp, E_RDB_CONFUSED );
		goto err1;
	}

	neo_rpc_log_msg( "UNLINK", M_RDB, svc_myname, &pe->p_clnt, "", "" );

	/*
	 * ポートエントリ解放前の後処理をする
	 */
	svc_garbage( pd );

	/*
	 * 応答パケットを作成して、応答を返す
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );
	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR02, neo_errsym() ) ;
		goto err1;
	}
//DBG_PRT("UNLINK REPLY sizeof(res)=%d\n", sizeof(res) );

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_garbage()
 *
 *  機能	切断前の後処理
 * 
 *  関数値 	0
 *
 ******************************************************************************/
int
svc_rollback_td( r_tbl_t *td, r_usr_t *ud )
{
	int	i;
	int	inc;
			/*
			 * レコードについての後処理
			 */
			for( i = 0; i < td->t_rec_num; i++ ) {

				if( td->t_rec[i].r_access != ud )
					continue;

				/*
				 * レコードロック解除
				 */
				if( td->t_rec[i].r_stat & R_LOCK )
					svc_rec_post( td, i );
					
				/* 
				 * 自分からの変更を廃棄する
				 */
				if( td->t_rec[i].r_stat & R_DIRTY ) {

					if( td->t_rec[i].r_stat & R_INSERT ) {

						inc = -1;
						if ( td->t_unused_index > i )
							td->t_unused_index = i;

					}  else if ( td->t_rec[i].r_stat 
								& R_UPDATE ) {
						if( td->t_rec[i].r_stat
								& R_USED )	
							inc = 0;
						else 
							inc = 1;
					}

					if( !svc_load( td, i, 1 ) ) {
						td->t_rec_used	+= inc;
						td->t_stat	|= R_UPDATE;
					}
				}

				td->t_rec[i].r_access	= 0;
				td->t_rec[i].r_stat	&= ~R_LOCK;
			}
			if( td->t_stat & R_UPDATE ) {
				td->t_version++;
				HashClear( &td->t_HashEqual );
			}
			/*
			 * テーブルのロックは解除しない
			 */
//			if( td->t_lock_usr == ud && td->t_stat & R_LOCK ) {
//
//				svc_tbl_post( td );
//				
//			}
	return( 0 );
}

int
svc_garbage( pd )
	p_id_t	*pd;		/* 通信ポート記述子アドレス */
{
	r_port_t	*pe;
	r_twait_t	*twaitp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	_dlnk_t		*dp;

DBG_ENT(M_RDB,"svc_garbage");

	/*
	 * ポートエントリがある場合には後処理をする
	 */
	if( (pe = PDTOENT( pd )) ) {

		/*
		 * 待ちテーブルがあれば、該当テーブルの
		 * 待ちユーザリストから該当ユーザを外して、領域を解放する
		 */
		if( (twaitp = pe->p_twait) ) {

			/* remove twait from td */
			td = twaitp->w_td;
			td->t_wait_cnt--;

			_dl_remove( twaitp );

			/*  unlink usr from twaitp */
			while( (dp = _dl_head(&twaitp->w_usr)) )
				_dl_remove( dp );

DBG_A(("free twaitp=0x%x\n", twaitp ));
			neo_free( twaitp );
		}

		/*
		 * オープンユーザをクローズする
		 */
		while( (ud = PDTOUSR(_dl_head( &pe->p_usr_ent ))) ) {

			td = ud->u_mytd;

			svc_rollback_td( td, ud );

#ifdef	ZZZ
#else
			/*
			 * テーブルのロックを解除する
			 */
			if( td->t_lock_usr == ud && td->t_stat & R_LOCK ) {

				svc_tbl_post( td );
			
			}
#endif
DBG_A(("close_user(ud=0x%x)\n", ud ));
			close_user( ud );

			if( td->t_stat & T_TMP ) {
DBG_B(("drop_td:t_usr_cnt=%d\n", td->t_usr_cnt ));
				drop_td( td );
			}
		}
DBG_A(("free pe=0x%x\n", pe ));
		
		/*
		 * ポートエントリ解放
		 */
		ASSERT( HashCount(&pe->p_HashUser) == 0 );
		HashDestroy( &pe->p_HashUser );

		neo_free( pe );
		pd->i_tag	= 0;
	}
DBG_EXIT(0);
	return( 0 );
}
