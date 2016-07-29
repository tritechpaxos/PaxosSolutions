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
 *	svc_load.c
 *
 *	説明		テーブルのファイルからのロード及び
 *				ファイルへのストア
 * 
 ******************************************************************************/

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"join.h"

/*****************************************************************************
 *
 *  関数名	svc_load()
 *
 *  機能	ファイルをメモリにロードする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_load( td, s, l )
	r_tbl_t	*td;		/* テーブル記述子		*/
	int	s;		/* ロード開始レコード番号	*/
	int	l;		/* ロードするレコードの数	*/
{
	int	e, i, n, j;
	r_rec_t	*cp;

DBG_ENT(M_RDB,"svc_load");
DBG_A(("s=%d,l=%d\n", s, l ));

	/*
	 * ロードするレコードの終了番号設定
	 */
	e = s + l -1;
	if( e >= td->t_rec_num )
		e = td->t_rec_num - 1;

	/*
	 *	Swap in
	*/
	for( i = s; i <= e; i++ ) {
		if( td->t_rec[i].r_stat & R_REC_SWAP ) {
			xj_swap_in_td( td, i, e );
		}
	}
	for( i = s; i <= e; i++ ) {
		if( !(td->t_rec[i].r_stat & R_REC_CACHE )
			|| td->t_rec[i].r_stat & R_DIRTY )
			goto loop;
	}
	DBG_EXIT( 0 );
	return( 0 );
loop:
	/*
	 * フリーセグメントの先頭を s とする
	 */
	i = s;
	n = 0;

	i++; n++;

	/*
	 * 該当フリーセグメントの最後を捜す
	 */
	while( i <= e ) {
		cp = &td->t_rec[i];
		if( cp->r_stat & R_REC_FREE ) {
			break;
		} else {
			n++;
			i++;
		}
	}

	/*
	 * 該当フリーセグメントに対応するレコード
	 * をロードする
	 */
DBG_A(("LOAD(s=%d,n=%d,i=%d,e=%d)\n", s, n, i, e));
LOG( neo_log, LogDBG, "[%s] s=%d n=%d i=%d e=%d", td->t_name, s, n, i, e );
	if( TBL_LOAD( td, s, n, (char*)td->t_rec[s].r_head ) < 0 )
		goto err1;

	for( j = s; j < s+n; j++ ) {

		td->t_rec[j].r_stat	&= ~(R_USED|R_BUSY|R_DIRTY);
		td->t_rec[j].r_stat	|= td->t_rec[j].r_head->r_stat&R_USED;

		td->t_rec[j].r_stat	|= R_REC_CACHE;
	}

	/*
	 * 未ロードレコード残っているならば、
	 * 次のフリーセグメントを捜す
	 */
	if( (s = i) <= e )
		goto loop;

LOG( neo_log, LogDBG, "EXT(0)" );
DBG_EXIT(0);
	return( 0 );
err1:
LOG( neo_log, LogDBG, "EXT(%d)", neo_errno );
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	svc_store()
 *
 *  機能	テーブルをファイルへ書き込む
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
svc_store( td, s, l, ud )
	r_tbl_t	*td;		/* テーブル記述子	*/
	int	s;		/* 開始レコード		*/
	int	l;		/* レコードの数		*/
	r_usr_t	*ud;		/* オープンユーザ記述子	*/
{
	int	is;
	r_rec_t	*cp;
	int	i, n;
	int	k;
	int	e;
DBG_ENT(M_RDB,"svc_store");

	e = s + l;
	i = s;
DBG_A(("s=%d,l=%d,e=%d\n", s, l, e ));
loop:
	n = 0;

	/*
	 * 復帰必要なレコードの開始番号を捜す
	 * [ 編集された(更新、挿入など)レコードだけを書き換える]
	 */
	while( i < e ) {
		cp = &td->t_rec[i];
		if( (cp->r_stat & R_DIRTY) && 
			( !ud || !cp->r_access || ud == cp->r_access ) ) {
			is = i;
			i++;
			n++;
			goto end;
		}
		i++;
	}
	goto exit;
end:
	/*
	 * 復帰必要なレコードの最後番号を捜す
	 */
	while( i < e ) {
		cp = &td->t_rec[i];
		if( (cp->r_stat & R_DIRTY)
			&& ( !ud || !cp->r_access || ud == cp->r_access )
			&& !(cp->r_stat & R_REC_FREE ) ) {

			i++;
			n++;

			continue;
		} else {
			break;
		}
	}

	/* 
	 * 開始番号～最後番号間のレコードセグメント
	 * をファイルに書き換える
	 */
	if( TBL_STORE( td, is, n, (char*)td->t_rec[is].r_head ) )
		goto err1;

	/* 
	 * 開始番号～最後番号間のレコードの編集された
	 * フラグをクリアする
	 */
	for( k = is; k < is + n ; k++ ) {
DBG_A(("k=%d\n", k ));
		td->t_rec[k].r_stat	&= ~(R_BUSY|R_DIRTY);
	}

	goto loop;
exit:

	/*
	 * update used information in the head of file
	 */
	if ( ud ) {

		td->t_rec_ori_used += ud->u_added;

		if ( TBL_CNTL(td, 0, 0 ) ) {
			td->t_rec_ori_used -= ud->u_added; 
			goto	err1;
		}

		ud->u_added = 0;
		ud->u_insert = 0;
		ud->u_update = 0;
		ud->u_delete = 0;
	}

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}
