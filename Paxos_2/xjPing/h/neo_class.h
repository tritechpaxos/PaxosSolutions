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
 *	neo_class.h
 *
 *	説明	ＮＥＯ クラス定義
 *
 ******************************************************************************/
#ifndef _NEO_CLASS_
#define _NEO_CLASS_

#define M_UNIX		0	/*  システムコール,libc		*/
#define M_LINK		1	/*  通信管理	*/
#define M_RDB		2	/*  ＤＢ管理	*/
#define M_SKL		3	/*  スケルトン			*/
#define	M_SQL		4	/*  SQL */
#define	M_SWAP		5
#define	M_HASH		6
#define M_APL		100	/*  ＡＰ			*/

typedef	struct	neo_class_sym		/* クラス・シンボル　構造体	*/
{
	int	c_class;		/* 出力クラス番号		*/
	char	*c_sym;			/* 出力クラス文字列		*/
} neo_class_sym_t;

extern	neo_class_sym_t	_neo_class_tbl[];

#endif

