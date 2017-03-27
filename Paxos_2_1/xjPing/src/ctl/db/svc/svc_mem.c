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
 *	svc_mem.c
 *
 *	説明		レコードについてのメモリ管理
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"

/*****************************************************************************
 *
 *  関数名	rec_cntl_alloc()
 *
 *  機能	レコード管理構造体のアロケット
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
rec_cntl_alloc( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	int	err;

DBG_ENT(M_RDB,"rec_cntl_alloc");

	/*
	 * レコード数分のレコード管理構造体用
	 * メモリを生成し、テーブル記述子に繋がる
	 */
	if( td->t_rec_num > 0 && !(td->t_rec = 
		(r_rec_t *)neo_zalloc(sizeof(r_rec_t)*td->t_rec_num)) ) {

		err = E_RDB_NOMEM;
		goto err1;
	}

DBG_A(("rec=0x%x(num=%d)\n", td->t_rec, td->t_rec_num ));
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	rec_cntl_free()
 *
 *  機能	レコード管理構造体のフリー
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
rec_cntl_free( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
DBG_ENT(M_RDB,"rec_cntl_free");
DBG_A(("rec=0x%x\n", td->t_rec));
	if( td->t_rec )
		neo_free( td->t_rec );
DBG_EXIT(0);
	return( 0 );
}


/*****************************************************************************
 *
 *  関数名	rec_buf_alloc()
 *
 *  機能	レコードバッファメモリのアロケット
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
rec_buf_alloc( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	r_head_t	*dp;
	r_rec_t		*rp;
	int		i;
	int		err;

DBG_ENT(M_RDB,"rec_buf_alloc");

	if( td->t_rec_num <= 0 )	return( 0 );
	/*
	 * レコード数分のレコード格納用バッファを生成する
	 */
	if( !(dp =
		(r_head_t *)neo_zalloc(((size_t)td->t_rec_size)*td->t_rec_num)) ) {

		err = E_RDB_NOMEM;
		goto err1;
	}
DBG_A(("rec_num=%d,size=%d,bufp=0x%x\n", td->t_rec_num, td->t_rec_size, dp));

	/*
	 * 各レコード領域のヘッダ部をクリアし、領域のアドレス
	 * を対応するレコードの￥管理構造体に設定し、管理構造体
	 * メモリアタッチ中フラグをたつ
	 */
	for( i = 0, rp = td->t_rec; i < td->t_rec_num;
			i++, dp = (r_head_t*)((char*)dp + td->t_rec_size) ) {

		bzero( dp, sizeof(r_head_t) );
		rp[i].r_head	= dp;
		rp[i].r_stat	|= R_REC_BUF;
	}

	/*
	 * 先頭レコード管理構造体だけのバッファフリーフラグをたつ
	 */
	rp[0].r_stat	|= R_REC_FREE;
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(err);
	return( err );
}

/*****************************************************************************
 *
 *  関数名	rec_buf_free()
 *
 *  機能	レコード格納エリアのフリー
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
rec_buf_free( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	int	err;
	int	i;
	r_rec_t	*rp;

DBG_ENT(M_RDB,"rec_buf_free");

	rp = td->t_rec;
	if( !rp ) {
		DBG_EXIT(0);
		return( 0 );
	}

	/*
	 * 先頭レコードバッファフリー指示がなければ
	 * エラーとなる
	 */
	if( !(rp[0].r_stat & R_REC_FREE) ) {
		err = E_RDB_CONFUSED;
		goto err1;
	}

	for( i = 0; i < td->t_rec_num; i++ ) {
		/*
		 * レコード格納エリアを解放する
		 */
		if( rp[i].r_stat & R_REC_FREE )
			neo_free( rp[i].r_head );

		/*
		 * レコード管理構造体のメモリアタッチ中フラグ
		 * とバッファフリーフラグをクリアする
		 */
		rp[i].r_stat &= ~(R_REC_FREE|R_REC_BUF);
	}
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(err);
	return( err );
}
