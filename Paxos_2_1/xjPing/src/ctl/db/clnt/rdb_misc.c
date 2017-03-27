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
 *	rdb_misc.c
 *
 *	説明	その他部分 : レコードサイズ、レコード数の参照及び
 *				     テーブル記述子、オープンユーザ構造体
 *				     の生成の返却
 * 
 ******************************************************************************/

#include	"neo_db.h"

/*****************************************************************************
 *
 *  関数名	rdb_size()
 *
 *  機能	レコードのサイズを問い合わせる
 * 
 *  関数値
 *     		正常	レコードのサイズ
 *		異常	-1
 *
 ******************************************************************************/
int
rdb_size( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	return( td->t_rec_size );
err1:
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	rdb_number()
 *
 *  機能	レコード数を問い合わせる
 * 
 *  関数値
 *     		正常	レコード数
 *		異常	-1
 *
 *   注	 	引数のモード
 *			0:	レコード総数
 *			1:	USEDレコード数
 *			2:	未使用レコード数
 *
 ******************************************************************************/
int
rdb_number( td, mode )
	r_tbl_t	*td;		/* テーブル記述子	*/
	int	mode;		/* モード: 0, 1, 2 	*/
{
	int			ret;
	p_id_t			*pd;
	r_req_rec_nums_t	req;
	r_res_rec_nums_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_number");

	/*
	 *	引数チェック
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}

	/*
	 *	リクエストパケットの情報設定
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_REC_NUMS, sizeof(req) );
	
	strncpy( req.r_name, td->t_name, sizeof(req.r_name) );
	/*
	 *	リクエスト
	 */
	if( p_request( pd = TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err1;

	/*
	 *	レスポンス
	 */
	len = sizeof( res ) ;
	if( p_response( pd, (p_head_t*)&res, &len ) )
		goto	err1;
	td->t_rec_num  = res.r_total;
	td->t_rec_used = res.r_used;

	switch( mode ) {
		case R_ALL_REC:
			ret = td->t_rec_num;
			break;
		case R_USED_REC:
			ret = td->t_rec_used;
			break;
		case R_UNUSED_REC:
			ret = td->t_rec_num - td->t_rec_used;
			break;
		default:
			neo_errno = E_RDB_INVAL;
			goto err1;
	}

DBG_EXIT(ret);
	return(ret);
err1:
DBG_EXIT(-1);
	return( -1 );

}


/*****************************************************************************
 *
 *  関数名	_r_usr_alloc()
 *
 *  機能	オープンユーザ構造体の生成と初期化
 * 
 *  関数値
 *     		正常	構造体のアドレス
 *		異常	NULL
 *
 ******************************************************************************/
r_usr_t	*
_r_usr_alloc()
{
	r_usr_t	*up;
DBG_ENT(M_RDB,"r_usr_alloc");

	if( (up = (r_usr_t *)neo_zalloc( sizeof(r_usr_t) )) ) {

		up->u_Id	= up;	// for Cell 
		_dl_init( &up->u_tbl );	
		_dl_init( &up->u_port );	
		_dl_init( &up->u_twait );	
		_dl_init( &up->u_rwait );	

		up->u_ident	= R_USR_IDENT;

		up->u_search.s_max	= R_EXPR_MAX;
	}
DBG_A(("up=0x%x\n", up ));
DBG_EXIT(up);
	return( up );
}


/*****************************************************************************
 *
 *  関数名	_r_usr_free()
 *
 *  機能	オープンユーザ構造体の解放
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
void
_r_usr_free( up )
	r_usr_t	*up;		/* 解放するオープンユーザ構造体のアドレス*/
{
DBG_ENT(M_RDB,"r_usr_free");
DBG_A(("up=0x%x\n", up ));
	up->u_ident	= 0;
	neo_free( up );
DBG_EXIT(0);
}

/*****************************************************************************
 *
 *  関数名	_r_tbl_alloc()
 *
 *  機能	テーブル記述子の生成と初期化
 * 
 *  関数値
 *     		正常	記述子のアドレス
 *		異常	NULL
 *
 ******************************************************************************/
r_tbl_t	*
_r_tbl_alloc()
{
	r_tbl_t	*td;

	if( (td = (r_tbl_t *)neo_zalloc(sizeof(r_tbl_t))) ) {
		td->t_ident	= R_TBL_IDENT;
		_dl_init( &td->t_usr );
		_dl_init( &td->t_wait );
	}
	return( td );
}


/*****************************************************************************
 *
 *  関数名	_r_tbl_free()
 *
 *  機能	テーブル記述子の返却
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
void
_r_tbl_free( td )
	r_tbl_t	*td;		/* 返却するテーブル記述子*/
{
	td->t_ident	= 0;
	if( td->t_scm ) {
		neo_free( td->t_scm );
		td->t_scm	= 0;
	}
	if( td->t_depend ) {
		neo_free( td->t_depend );
		td->t_depend	= 0;
	}
	neo_free( td );
}

/*****************************************************************************
 *
 *  関数名	_rdb_req_head()
 *
 *  機能	RDBファミリプロトコル要求パケットヘッダ部作成
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
void
_rdb_req_head( hp, cmd, len )
	p_head_t	*hp;
	int		cmd;
	int		len;
{
	static	int	idcnt = 0;

	hp->h_tag	= P_HEAD_TAG;
	hp->h_mode	= P_PKT_REQUEST;
	hp->h_id	= idcnt++;
	hp->h_err	= 0;
	hp->h_len	= len - sizeof(p_head_t);
	hp->h_family	= P_F_RDB;
	hp->h_vers	= R_VER0;
	hp->h_cmd	= cmd;
}
