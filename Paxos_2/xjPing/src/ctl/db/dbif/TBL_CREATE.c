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
 *	TBL_CREATE.c
 *
 *	説明		ファイル生成と削除
 * 
 ******************************************************************************/

#include	"TBL_RDB.h"

/*****************************************************************************
 *
 *  関数名	TBL_CREATE()
 *
 *  機能	ファイルを生成し、初期化する
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_CREATE( name, type, modified, rec_num, item_num, items,
		depend_num, depends )
	r_tbl_name_t	name;		/* 生成するファイル名	 */
	int		type;		/* table type normal/view */
	long		modified;	/* modified time */
	int		rec_num;	/* レコード総数		 */
	int		item_num;	/* レコード毎のアイテム数*/
	r_item_t	items[];	/* アイテム情報エリア	 */
	int		depend_num;
	r_tbl_mod_t	depends[];
{
	int		i;
	uint64_t	Off;

DBG_ENT(M_RDB,"TBL_CREATE");
DBG_A(("rec_num=%d,item_num=%d\n", rec_num, item_num ));
	for( i = 0; i < item_num; i++ ) {
		items[i].i_ind	= i;
	}
	/*
	 * ファイルを生成する
	 */
/***
	if( (fd = fd_create( name )) < 0 )
		goto err1;
***/

	if( FileCacheCreate( pBC, name ) < 0 )
		goto err1;
	/*
	 * ファイルのヘッダ部分を設定する
	 */
/***
	if( (neo_errno = my_head_write( fd, type, modified,
					rec_num, item_num, items )) ) {
		goto err1;
	}
	if( (neo_errno = my_item_write( fd, item_num, items )) ) {
		goto err1;
	}
	if( (neo_errno = my_depend_write( fd, depend_num, depends )) ) {
		goto err1;
	}

	fd_delete( fd );
***/
	if( (neo_errno = my_head_write( name, type, modified,
					rec_num, item_num, items, &Off )) ) {
		goto err1;
	}
	if( (neo_errno = my_item_write( name, item_num, items, &Off )) ) {
		goto err1;
	}
	if( (neo_errno = my_depend_write( name, depend_num, depends, &Off )) ) {
		goto err1;
	}

DBG_EXIT(0);
	return( 0 );

err1: DBG_T("err1");
DBG_EXIT(0);
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	TBL_DROP()
 *
 *  機能	ファイルを削除する
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_DROP( name )
	r_tbl_name_t	name;		/* 削除するファイル名	*/
{

DBG_ENT(M_RDB,"TBL_DROP");

/***
	if( fd_unlink( name ) < 0 )
		goto err1;
***/
	if( FileCacheDelete( pBC, name ) < 0 )
		goto err1;
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	neo_errno = unixerr2neo();
	return( -1 );
}

