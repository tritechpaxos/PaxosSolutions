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
 *	TBL_DBOPEN.c
 *
 *	説明	TBL_DBOPEN : DBオープン
 *			TBL_DBCLOSE: DBクローズ
 * 
 ******************************************************************************/

#include	"TBL_RDB.h"

BlockCacheCtrl_t	BCNTRL;

/*****************************************************************************
 *
 *  関数名	TBL_DBOPEN()
 *
 *  機能	DB をオープンする
 * 
 *  関数値
 *     	正常: 0
 *	異常: !0
 *
 ******************************************************************************/
void*
TBL_DBOPEN( path )
	char	*path;	/* パス名	*/
{
	return( &BCNTRL );
}

/*****************************************************************************
 *
 *  関数名	TBL_DBCLOSE()
 *
 *  機能	DB をクローズする
 * 
 *  関数値
 *     	無し
 *
 ******************************************************************************/
int
TBL_DBCLOSE( path )
	char	*path;
{
	return( 0 );
}
