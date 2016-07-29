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
 *	neo_class.c
 *
 *	説明	対応するシンボルテーブル
 * 
 ******************************************************************************/
#include "neo_class.h"

/************/
/* テーブル */
/************/

neo_class_sym_t	_neo_class_tbl[] = {

	{  M_UNIX,	"M_UNIX"  },	/* システムコール,libc */
	{  M_LINK,	"M_LINK"  },	/* 通信管理 */
	{  M_RDB,	"M_RDB"  },	/* ＤＢ管理 */
	{  M_SKL,	"M_SKL"  },	/* スケルトン */
	{  M_SQL,	"M_SQL"  },	/* SQL */
	{  M_SWAP,	"M_SWAP"  },	  
	{  M_HASH,	"M_HASH"  },	  
	{  M_APL,	"M_APL"  },	/* ＡＰ */
	{  0,		(char*)0  }
};
