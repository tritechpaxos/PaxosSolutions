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
 *	TBL_LOAD.c
 *
 *	説明		データロードとストア 
 *
 ******************************************************************************/

#include	"TBL_RDB.h"

/*****************************************************************************
 *
 *  関数名	TBL_LOAD()
 *
 *  機能	レコードをファイルからロードする 
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_LOAD( td, s, n, bufp )
	r_tbl_t		*td;		/* テーブル記述子		*/
	int		s;		/* ロードするレコードの開始番号 */
	int		n;		/* ロードするレコードの数	*/
	char		*bufp;		/* レコード格納用エリア		*/
{

DBG_ENT(M_RDB,"TBL_LOAD");
DBG_A(("s=%d,n=%d\n", s, n ));

	if( (neo_errno = my_tbl_load( TDTOMY( td ), s, n, bufp )) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	TBL_STORE()
 *
 *  機能	レコードをファイルへストアする 
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
TBL_STORE( td, s, n, bufp )
	r_tbl_t		*td;
	int		s, n;
	char		*bufp;
{
DBG_ENT(M_RDB,"TBL_STORE");
DBG_A(("s=%d,n=%d\n", s, n ));

	if( (neo_errno = my_tbl_store( TDTOMY( td ), s, n, bufp )) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( -1 );
}
