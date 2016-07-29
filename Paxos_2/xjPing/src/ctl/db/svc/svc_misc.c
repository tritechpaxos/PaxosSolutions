/*******************************************************************************
 * 
 *  svc_misc.c ---xjPing (On Memory DB) Misc programs for Server
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
/******************************************************************************
 *	svc_misc.c
 *
 *	説明	db_man server   共通関数
 *				その他部分
 ******************************************************************************/
#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	check_td_ud()
 *
 *  機能	テーブル、オープンユーザ記述子有効チェック
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
check_td_ud( td, ud )
	r_tbl_t	*td;
	r_usr_t	*ud;
{
	int	err;

	if( (err = check_td( td )) || (err = check_ud( ud)) )
		goto ret;
	
	if( td != ud->u_mytd )
		err = E_RDB_INVAL;

	td->t_access	= time( 0 );

ret:
	return( err );
}


/*****************************************************************************
 *
 *  関数名	check_td()
 *
 *  機能	テーブル記述子有効チェック
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
check_td( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	int	err = 0;

DBG_ENT(M_RDB,"check_td");

/*************************************************************
DBG_A(("td=0x%x(edata=0x%x,end=0x%x,sbrk(0)=0x%x)\n", 
		td, neo_edata, neo_end, sbrk(0)));
	if( CHK_ADDR( td ) ) {
		err = E_RDB_INVAL;
		goto ret;
	}
*************************************************************/

#ifdef ZZZ
	if( td->t_ident != R_TBL_IDENT ) {
#else
	if( !td || td->t_ident != R_TBL_IDENT ) {
#endif
		err = E_RDB_TD;
		goto ret;
	}
ret:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	check_ud()
 *
 *  機能	オープンユーザ記述子有効チェック
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
check_ud( ud )
	r_usr_t	*ud;		/* オープンユーザ記述子	*/
{
	int	err = 0;

DBG_ENT(M_RDB,"check_ud");

/**********************************************
	if( CHK_ADDR( ud ) ) {
		err = E_RDB_INVAL;
		goto ret;
	}
**********************************************/

#ifdef	ZZZ
	if( ud->u_ident != R_USR_IDENT ) {
#else
	if( !ud || ud->u_ident != R_USR_IDENT ) {
#endif
		err = E_RDB_UD;
		goto ret;
	}
ret:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	check_lock()
 *
 *  機能	テーブル操作可能性チェック
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
check_lock( td, ud, mode )
	r_tbl_t	*td;		/* テーブル記述子	*/
	r_usr_t	*ud; 		/* オープンユーザ記述子 */
	int	mode;		/* 操作タイプ		*/
{
	int	err = 0;

DBG_ENT(M_RDB,"check_access");

	if( (td->t_stat & R_LOCK ) && (ud != td->t_lock_usr) ) {

		if( td->t_stat & R_EXCLUSIVE ) {
			err = E_RDB_TBL_BUSY;
		} else if( mode & R_WRITE ) {
			err = E_RDB_TBL_BUSY;
		}
	}
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	reply_head()
 *
 *  機能	応答パケットヘッダ部作成
 * 
 *  関数値
 *     		無し
 *
 *****************************************************************************/
void
reply_head( reqp, resp, len, err )
	p_head_t	*reqp;		/* 要求パケットヘッダ	*/
	p_head_t	*resp;		/* 応答パケットヘッダ	*/
	int		len;		/* 応答パケットサイズ	*/
	int		err;		/* エラー番号		*/
{
	bcopy( reqp, resp, sizeof(p_head_t) );
	resp->h_len	= len - sizeof(p_head_t);
	resp->h_err	= err;
}


/*****************************************************************************
 *
 *  関数名	err_reply()
 *
 *  機能	エラー時応答パケットの送信
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
err_reply( pd, hp, err )
	p_id_t		*pd;		/* 通信ポートアドレス	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
	int		err;		/* エラー番号		*/
{
	p_head_t	*pH;

	pH	= (p_head_t*)neo_malloc( sizeof(p_head_t) );
	memcpy( pH, hp, sizeof(*pH) );
	neo_errno	= pH->h_err	= err;
	pH->h_len	= 0;

	p_reply_free( pd, pH, sizeof(*pH) );

DBG_EXIT(neo_errno);
	return( neo_errno );
}

#define	OPEN_IDLE_MAX	50

void	hash_destroy();

r_tbl_t*
svc_tbl_alloc()
{
	r_tbl_t	*tdp;

	tdp = _r_tbl_alloc();
	HashInit( &tdp->t_HashEqual, PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR,
				NULL, neo_malloc, neo_free, hash_destroy );	
	HashInit( &tdp->t_HashRecordLock, PRIMARY_11, HASH_CODE_PNT, HASH_CMP_PNT,
				NULL, neo_malloc, neo_free, NULL );	

	return( tdp );
}

void
svc_tbl_free( r_tbl_t *tdp )
{
	HashDestroy( &tdp->t_HashRecordLock );
	HashDestroy( &tdp->t_HashEqual );
	_r_tbl_free( tdp );
}

/*****************************************************************************
 *
 *  関数名	drop_table()
 *
 *  機能	テーブルドロップ
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
drop_table()
{
	r_tbl_t	*td;
	r_tbl_t	*nd;
	long	t;
	int	cnt;
	int	err;

DBG_ENT(M_RDB,"drop_table");

	t = time( 0 );

	for( td = (r_tbl_t*)_dl_head(&(_svc_inf.f_opn_tbl)), cnt = 0;
			_dl_valid(&(_svc_inf.f_opn_tbl),td); td = nd ) {

		nd	= (r_tbl_t *)_dl_next( td );

		if( td->t_usr_cnt == 0 ) {

			cnt++;

			if( (t - td->t_access) > R_DROP_TABLE_TIME 
				|| cnt > OPEN_IDLE_MAX /* older ones */ ) {

				/* moved from prc_close */
				if( td->t_md ) {
					if( (err = svc_store
						(td, 0, td->t_rec_num, 0)) ){
						goto err1;
					}
				}

				if ( (err = drop_td( td )) )
					goto	err1;
			}
		}
	}
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	drop_td()
 *
 *  機能	テーブルをメモリから消去する
 * 
 *  関数値
 *     		正常  	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
drop_td( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	int	err;

DBG_ENT(M_RDB,"drop_td");

	/*
	 * バッファフリー
	 */
	if ( (err = rec_buf_free( td )) )
		goto	err1 ;

	if ( (err = rec_cntl_free( td )) )
		goto	err1 ;
	/*
	 *	remove td from list
	 */
	_dl_remove( td );
	td->t_ident	= 0;

	HashRemove( &_svc_inf.f_HashOpen, td->t_name );

	if ( (err = TBL_CLOSE( td )) )
		goto	err1;

	/*
	 *  Free tbl
	 */
	 svc_tbl_free( td );

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(err);
	return( err );

}


/*****************************************************************************
 *
 *  関数名	svc_switch()
 *
 *  機能	テーブルサイズの変更
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_switch( td, store_rec_num, new_rec_num )
	r_tbl_t	*td;			/* テーブル記述子	*/
	int	store_rec_num;		/* 書き込むレコード数	*/
	int	new_rec_num;		/* 更新後レコード数	*/
{
	int	i;
	r_tbl_t		*tdwp;
	r_rec_t		*recwp;

DBG_ENT(M_RDB,"svc_switch");
DBG_A(("rec_num=%d,store_rec_num=%d,new_rec_num=%d\n",
		td->t_rec_num, store_rec_num, new_rec_num ));

	if( !(tdwp = svc_tbl_alloc() ) ) {
		goto err1;
	}

	tdwp->t_rec_num		= new_rec_num;
	tdwp->t_rec_size	= td->t_rec_size;

	if( rec_cntl_alloc( tdwp ) ) {
		goto err2;
	}
	if( rec_buf_alloc( tdwp ) ) {
		goto err3;
	}
	for( i = 0; i < store_rec_num; i++ ) {

		tdwp->t_rec[i].r_stat	= td->t_rec[i].r_stat;
		tdwp->t_rec[i].r_access	= td->t_rec[i].r_access;
		tdwp->t_rec[i].r_wait	= td->t_rec[i].r_wait;

		bcopy( td->t_rec[i].r_head, tdwp->t_rec[i].r_head,
					td->t_rec_size );
	}
	recwp		= tdwp->t_rec;
	tdwp->t_rec	= td->t_rec;
	td->t_rec	= recwp;

	td->t_rec_num	= new_rec_num;
	tdwp->t_rec_num	= store_rec_num;

	if( TBL_CNTL( td, 0, 0 ) ) {
		goto err4;
	}
	if( drop_td( tdwp ) ) {
		goto err4;
	}
	td->t_version++;

	DBG_EXIT( 0 );
	return( 0 );

err4:	rec_buf_free( tdwp );
err3:	rec_cntl_free( tdwp );
err2:	svc_tbl_free( tdwp );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
#ifdef XXX
	/*
	 *	save
	 */
	item_num = td->t_scm->s_n;
	if( !(items = (r_item_t*)neo_malloc(item_num*sizeof(r_item_t)) ) ) {
		err = E_RDB_NOMEM;
		goto err1;
	}
	bcopy( td->t_scm->s_item, items, item_num*(sizeof(r_item_t)) );

	/*
	 *	file delete/create
	 */
	old_rec_num	= td->t_rec_num;
	used		= td->t_rec_used;

	if( TBL_CLOSE( td ) 
		|| TBL_DROP( td->t_name ) 
		|| TBL_CREATE( td->t_name, td->t_type, td->t_modified,
						new_rec_num, item_num, items )
		|| TBL_OPEN( td, td->t_name ) ) {

		err = E_RDB_CONFUSED;
		goto err2;
	}
	neo_free( items );

	for ( i = 0; i < store_rec_num; i++ ) {
		td->t_rec[i].r_stat	|= R_UPDATE;
	}
	if( err = svc_store( td, 0, store_rec_num, 0 ) ) {
		goto err2;
	}
	/*
	 *	free old memories
	 */
	td->t_rec_num	= old_rec_num;

	if ( err = rec_buf_free( td ) )
		goto	err1;
	if ( err = rec_cntl_free( td ) )
		goto	err1;
	/*
	 *	attach new control
	 */
	td->t_rec_num	= new_rec_num;
	td->t_rec_used	= used;

	if( err = rec_cntl_alloc( td ) ) {
		goto err1;
	}
	if( err = rec_buf_alloc( td ) ) {
		rec_cntl_free( td );
		goto err1;
	}

	if ( err = TBL_CNTL( td, 0, 0 ) )
		goto	err1;

	if( err = svc_load( td, 0, old_rec_num ) )
		goto err1;
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( items );
err1:
DBG_EXIT(err);
	return( err );
#endif
}

