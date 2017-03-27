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
 *	TBL_OPEN.c
 *
 *	説明	TBL_OPEN : テーブルオープン
 *			TBL_CLOSE: テーブルクローズ
 * 
 ******************************************************************************/

#include	"TBL_RDB.h"

/*****************************************************************************
 *
 *  関数名	TBL_OPEN()
 *
 *  機能	テーブルをオープンする
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_OPEN( td, name )
	r_tbl_t		*td;		/* テーブル記述子	 */
	r_tbl_name_t	name;		/* オープン対象テーブル名*/
{
	my_tbl_t	*md;
	int		err;
	uint64_t	Off;

DBG_ENT(M_RDB,"TBL_OPEN");
DBG_A(("name[%s]\n", name ));

	strncpy( td->t_name, name, sizeof(td->t_name) );

	/*
	 * DBテーブル記述子エリアを生成する
	 */
	if( !( md = my_tbl_alloc() ) ) {
		err	= E_RDB_NOMEM;
		goto err1;
	}
	md->m_td	= td;
	td->t_tag	= (void *)md;

	/*
	 * DBテーブルをオープンする
	 */

/*****
	if( (err = fd_open( md )) )
		goto err2;
****/

	/*
	 * レコード情報、アイテム情報などのファイル
	 * ヘッダ部を取得する
	 */
	if( my_head_read( md, &Off ) )
		goto err2;
	if( my_item_read( md, &Off ) )
		goto err2;
	if( my_depend_read( md, &Off ) )
		goto err2;

	/*
	 * データ部のオフセットを算出する
	 */
	md->m_data_off	= my_data_off( md );

DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	my_tbl_free( md );
err1: DBG_T("err1");
	neo_errno = err;
DBG_EXIT(-1);
	return( -1 );
}

#ifdef	ZZZ
#else
int
TBL_OPEN_SET( r_tbl_t *td )
{
	my_tbl_t	*md;
	int		err;

	/*
	 * DBテーブル記述子エリアを生成する
	 */
	if( !( md = my_tbl_alloc() ) ) {
		err	= E_RDB_NOMEM;
		goto err1;
	}
	md->m_td	= td;
	td->t_tag	= (void *)md;

	md->m_data_off	= my_data_off( md );

	return( 0 );
err1:
	neo_errno = err;
	return( -1 );
}
#endif


/*****************************************************************************
 *
 *  関数名	TBL_CLOSE()
 *
 *  機能	テーブルをクローズする
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_CLOSE( td )
	r_tbl_t	*td;		/* テーブル記述子	*/
{
	my_tbl_t	*md;
	int		err;

DBG_ENT(M_RDB,"TBL_CLOSE");

	md	= TDTOMY( td );

	if( !md )
		goto ret;

	/*
	 * ファイルヘッダ部のUSEDレコード情報を更新する
	 */
	if( (err = my_head_update( md )) )
		goto err1;

	/*
	 * DBテーブルをクローズする
	 */
/***
	if( (err = fd_close( md )) )
		goto err1;
***/

	/*
	 * DBテーブル記述子エリアを解放する
	 */
	my_tbl_free( md );

ret:
DBG_EXIT(0);
	return( 0 );
err1:
	neo_errno = err;
DBG_EXIT(-1);
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	TBL_CNTL()
 *
 *  機能	
 * 
 *  関数値
 *     		正常	0
 *		異常	-1	
 *
 ******************************************************************************/
int
TBL_CNTL( td, cmd, arg )
	r_tbl_t	*td;		/* テーブル記述子	*/
	int	cmd;		
	char	*arg;
{
	my_tbl_t	*md;
	uint64_t	Off;

DBG_ENT(M_RDB,"TBL_CNTL");

	md	= TDTOMY( td );

	if( !md )
		goto ret;

	switch( PNT_INT32(arg) ) {
		case 0:
			/*
			 * ファイルヘッダ部の情報を更新する
			 * (modified, used)
			 */
			if( (neo_errno = my_head_update( md )) )
				goto err1;
			break;
		case 1:	/* reload may not be used ? */
			if( (neo_errno = my_head_read( md, &Off )) )
				goto err1;
			break;
	}

ret:
DBG_EXIT( 0 );
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}
