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

/************************************************************************/
/*									*/
/*									*/
/*	機能	デバッグ用の出力を行なう。				*/
/*	処理	クラスとレベルにより出力を制御する。			*/
/*									*/
/*	関数								*/
/*									*/
/************************************************************************/
#ifndef	_LIB_DBG_
#define	_LIB_DBG_

#include "NWGadget.h"

#include "neo_class.h"
#include "neo_debug.h"

/**************/
/* 出力レベル */
/**************/

typedef	struct	neo_level_sym		/* レベル・シンボル　構造体	*/
{
	char	c_level;		/* 出力レベル番号		*/
	char	c_sym;			/* 出力レベル文字列		*/
} neo_level_sym_t;

neo_level_sym_t	_neo_level_tbl[] = {	/* レベル・シンボル　テーブル	*/

	{ _LEV_A,	'A'  },		/* 出力レベル　Ａ		*/
	{ _LEV_B,	'B'  },		/* 出力レベル　Ｂ		*/
	{ _LEV_C,	'C'  },		/* 出力レベル　Ｃ		*/
	{ _LEV_T,	'T'  },		/* 出力レベル　トレース		*/
	{ '\0',		'\0' }
};

/**************/
/* オプション */
/**************/

#define	D_PREFIX	"DBG"		/* 出力ファイルのプリフィックス	*/
#define	D_OPT_DIR	"NEO_DBGDIR"	/* ﾃﾞﾊﾞｯｸﾞﾌｧｲﾙ用ディレクトリー	*/
#define	D_OPT_ENV	"NEO_DBGOPT"	/* 環境変数からのオプション指定	*/
#define	D_OPT_ARG	"-d" 		/* 引数からのオプション指定	*/
#define	D_OPT_ALL	"ALL"		/* 全クラス指定オプション	*/
#define	D_OPT_DEL	","		/* オプション用デリミッター	*/
#define	D_OPT_LEV	'='		/* レベル用デリミッター		*/

#define	OK		0
#define	ERR		-1

#endif	// _LIB_DBG_
/************************************************************************/
/*			End of the Header				*/
/************************************************************************/
