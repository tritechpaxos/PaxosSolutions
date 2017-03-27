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
 *	説明	デバッグ出力用　ライブラリー   (lib_dbg.c)
 * 
 ******************************************************************************/

#include <stdio.h>
#include <time.h>
/***
#include <varargs.h>
***/
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "lib_dbg.h"

/**********************/
/* グローバル変数定義 */
/**********************/

char	*_Dbg_filename;			/* ファイル名			*/
char	*_Dbg_func;			/* 関数名			*/
char	 _Dbg_level;			/* 出力レベル			*/
int	 _Dbg_line;			/* 出力ライン			*/
int	 _Dbg_class;			/* 出力クラス			*/

#define	D_MAX_TBL	256		/* 最大テーブルサイズ		*/
static	char	 _Dbg_tbl[D_MAX_TBL];	/* デバッグレベル　テーブル	*/
static	FILE	*_Dbg_fp = (FILE*)NULL;	/* 出力ファイルポインター	*/

/************/
/* 関数宣言 */
/************/

char	*strtok();
char	*getenv();
/***
int	printf();
char	*neo_malloc();
***/
char	*neo_malloc();
void	neo_free();

static	int	dbg_chk_arg();		/* 引数のチェック		*/
static	int	dbg_set_level();	/* デバッグレベルの設定		*/
static	int	dbg_set_tbl();		/* テーブルのセット		*/
static	int	dbg_set_tbl_all();	/* テーブルのセット		*/
static	int	dbg_get_class();	/* クラスインデックスの取得	*/
static	char	dbg_get_level();	/* レベルの取得			*/
static	int	dbg_open_file();	/* 出力ファイルのオープン	*/
static	void	dbg_write_header();	/* ヘッダーの書き込み		*/
static	char	*dbg_get_time();	/* 現在時刻の取得		*/

/**************************/
/* デバッグ用　マクロ定義 */
/**************************/

#ifdef	DEBUG

#define	PRINTF				printf
#define	FUNC1				dbg_table
#define	FUNC2				dbg_find_classname
#define	FUNC3				dbg_find_levelname
#define	_printf(a) 			PRINTF a
#define	_dbg_table()			FUNC1()
#define	_dbg_find_classname(a,b)	FUNC2(a,b)
#define	_dbg_find_levelname(a,b)	FUNC3(a,b)

#else	// DEBUG

#define	_printf(a)
#define	_dbg_table()
#define	_dbg_find_classname(a,b)
#define	_dbg_find_levelname(a,b)

#endif	// DEBUG

/******************************************************************************
 * 関数名	_dbg_init
 * 機能		デバッグの初期化
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

extern	int
_dbg_init( ac, av )
int	*ac;
char	*av[];
{
	char	*dbg_level = (char*)NULL;	/* デバッグレベル文字列	*/
	char	*procname  = (char*)NULL;	/* プロセス名称		*/
	char	save_av[1024];			/* 引数のセーブ		*/
	int	ret_cd  = OK;			/* リターン　コード	*/
	int	dbg_flg = 0;			/* デバッグ　フラグ	*/

	_printf(( "Enter %s(ac=%d,av[0]=%s)\n", __FUNCTION__, *ac, av[0] ));

	/******************/
	/* 引数のチェック */
	/******************/

	if ( dbg_chk_arg(ac, av, &dbg_level, &procname, save_av) == ERR ) {
		_printf(( "      %s : dbg_chk_arg ERR !\n", func ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : dbg_chk_arg OK !\n", func ));

	/************************/
	/* デバッグテーブル作成 */
	/************************/

	if ( dbg_set_level(dbg_level, &dbg_flg) == ERR ) {
		_printf(( "      %s : dbg_set_level ERR !\n", func ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : dbg_set_level OK !\n", func ));

	if ( dbg_flg == 0 ) {
		_printf(( "      %s : dbg_flg == 0 : goto ret !\n", func ));
		goto 	ret;
	}

	/**************************/
	/* 出力ファイルのオープン */
	/**************************/

	if ( dbg_open_file(procname) == ERR ) {
		_printf(( "      %s : dbg_open_file(%s) ERR !\n",
			func, procname ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : dbg_open_file(%s) OK !\n", func, procname ));

	/**********************/
	/* ヘッダーの書き込み */
	/**********************/

	dbg_write_header( save_av );
	_printf(( "      %s : dbg_write_header OK !\n", func ));

	/********/
	/* 終了 */
	/********/
ret:
	if ( procname  ) {
		neo_free( procname );
		_printf(( "      %s : neo_free procname OK !\n", func ));
	}

	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	_dbg_term
 * 機能		デバッグの終了処理
 * 関数値	なし
 * 注意
 ******************************************************************************/

extern	void
_dbg_term()
{
	_printf(( "Enter %s\n", __FUNCTION__ ));

	/**********************/
	/* 終了時刻の書き込み */
	/**********************/

	if ( _Dbg_fp  ) {
		_printf(( "      %s : write dbg_get_time OK !\n", func ));
		fprintf( _Dbg_fp, "\n%s\n", dbg_get_time() );
	}

	/********/
	/* 終了 */
	/********/

	if ( _Dbg_fp  ) {
		fclose( _Dbg_fp );
		_Dbg_fp = (FILE*)NULL;
		_printf(( "      %s : fclose OK !\n", func ));
	}

	_printf(( "Exit  (void)%s\n", func ));
}

/* VARARGS2 */
/******************************************************************************
 * 関数名	_dbg_printf
 * 機能		デバッグ表示
 * 関数値	なし
 * 注意
 ******************************************************************************/

extern	int
_dbg_print_check()
{
#ifdef	DEBUG
	va_list ap;
	char	classname[32];
	char	levelname[32];
#endif
	int	check = 0;

	_dbg_find_classname( _Dbg_class, classname );
	_dbg_find_levelname( _Dbg_level, levelname );
	_printf(( "Enter %s( %s, %s )\n", __FUNCTION__, classname, levelname ));

	/******************************************************/
	/* ファイルがオープンされていない時は、何も出力しない */
	/******************************************************/

	if ( _Dbg_fp == (FILE*)NULL ) {
		_printf(( "      %s : _Dbg_fp == NULL!\n", func ));
		goto	ret;
	}

	/********************/
	/* クラス  チェック */
	/********************/

	if ( _Dbg_class < 0 || _Dbg_class > D_MAX_TBL ) {
		_printf(( "      %s : class check ERR ! : class = %d\n",
			func, _Dbg_class ));
		goto	ret;
	}

	_printf(( "      %s : class check OK ! : class = %d\n",
			func, _Dbg_class ));

	/************/
	/* 出力判定 */
	/************/

	if ( (_Dbg_level & _Dbg_tbl[_Dbg_class]) == 0 ) {
		_printf(( "      %s : 出力なし\n", func ));
		goto	ret;
	}
	check = 1;
	fprintf(  _Dbg_fp, "%-12s %4d  ", _Dbg_filename, _Dbg_line );
ret:
	return( check );
}

#ifdef XXX
extern	void
_dbg_printf1( va_alist )
va_dcl
{
	va_list ap;
	char	*format;
	static	char	buf[BUFSIZ];

	if ( _Dbg_fp == (FILE*)NULL ) {
		return;
	}
	va_start(ap);
	format = va_arg( ap, char* );
	vsprintf( buf, format, ap );
	fprintf( _Dbg_fp, buf );
}

#else

extern	void
_dbg_printf1( char *format, ... )
{
	va_list ap;
	static	char	buf[BUFSIZ];

	if ( _Dbg_fp == (FILE*)NULL ) {
		return;
	}
	va_start(ap, format );
	vsprintf( buf, format, ap );
	fprintf( _Dbg_fp, "%s", buf );
	va_end(ap);
}
#endif

#ifdef XXX
extern	void
_dbg_printf( va_alist )
va_dcl
{
	va_list ap;
	neo_level_sym_t	*ptr;
	char	*func = "_dbg_printf";		/* 関数名		*/
	char	*format;
	int	length;
	int	index;
	int	idx;
	int	err    = errno;
	int	lv_flg = 0;
	char	classname[32];
	char	levelname[32];
	static	char	buf[BUFSIZ];

	_dbg_find_classname( _Dbg_class, classname );
	_dbg_find_levelname( _Dbg_level, levelname );
	_printf(( "Enter %s( %s, %s )\n", func, classname, levelname ));

	/******************************************************/
	/* ファイルがオープンされていない時は、何も出力しない */
	/******************************************************/

	if ( _Dbg_fp == (FILE*)NULL ) {
		_printf(( "      %s : _Dbg_fp == NULL!\n", func ));
		goto	ret;
	}

	/********************/
	/* クラス  チェック */
	/********************/

	if ( _Dbg_class < 0 || _Dbg_class > D_MAX_TBL ) {
		_printf(( "      %s : class check ERR ! : class = %d\n",
			func, _Dbg_class ));
		goto	ret;
	}

	_printf(( "      %s : class check OK ! : class = %d\n",
			func, _Dbg_class ));

	/************/
	/* 出力判定 */
	/************/

	if ( (_Dbg_level & _Dbg_tbl[_Dbg_class]) == 0 ) {
		_printf(( "      %s : 出力なし\n", func ));
		goto	ret;
	}

	_printf(( "      %s : 出力\n", func ));

	/**************************/
	/* フォーマット　チェック */
	/**************************/

	va_start(ap);
	format = va_arg( ap, char* );

	if ( format == NULL ) {
		_printf(( "      %s : format ERR !\n", func ));
		goto	ret;
	}

	_printf(( "      %s : format OK !\n", func ));

	/****************/
	/* ファイル出力 */
	/****************/

	fprintf(  _Dbg_fp, "%-12s %4d  ", _Dbg_filename, _Dbg_line );

	if ( !(_Dbg_level & _LEV_T) ) {
		fprintf(  _Dbg_fp, "   " );
	}

	vsprintf( buf, format, ap );
	fprintf( _Dbg_fp, buf );
	_printf(( "      %s : fprintf OK ! : buf = \"%s\"\n", func, buf ));

	errno = err;

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  (void)%s\n", func ));
	return;
}
#else
extern	void
_dbg_printf( char *format, ... )
{
	va_list ap;
#ifdef	DEBUG
	neo_level_sym_t	*ptr;
	int	length;
	int	index;
	int	idx;
	int	lv_flg = 0;
	char	classname[32];
	char	levelname[32];
#endif
	int	err    = errno;
	static	char	buf[BUFSIZ];

	_dbg_find_classname( _Dbg_class, classname );
	_dbg_find_levelname( _Dbg_level, levelname );
	_printf(( "Enter %s( %s, %s )\n", __FUNCTION__, classname, levelname ));

	/******************************************************/
	/* ファイルがオープンされていない時は、何も出力しない */
	/******************************************************/

	if ( _Dbg_fp == (FILE*)NULL ) {
		_printf(( "      %s : _Dbg_fp == NULL!\n", func ));
		goto	ret;
	}

	/********************/
	/* クラス  チェック */
	/********************/

	if ( _Dbg_class < 0 || _Dbg_class > D_MAX_TBL ) {
		_printf(( "      %s : class check ERR ! : class = %d\n",
			func, _Dbg_class ));
		goto	ret;
	}

	_printf(( "      %s : class check OK ! : class = %d\n",
			func, _Dbg_class ));

	/************/
	/* 出力判定 */
	/************/

	if ( (_Dbg_level & _Dbg_tbl[_Dbg_class]) == 0 ) {
		_printf(( "      %s : 出力なし\n", __FUNCTION__ ));
		goto	ret;
	}

	_printf(( "      %s : 出力\n", __FUNCTION__ ));

	/**************************/
	/* フォーマット　チェック */
	/**************************/

	va_start(ap,format);

	if ( format == NULL ) {
		_printf(( "      %s : format ERR !\n", __FUNCTION__ ));
		goto	ret;
	}

	_printf(( "      %s : format OK !\n", __FUNCTION__ ));

	/****************/
	/* ファイル出力 */
	/****************/

	fprintf(  _Dbg_fp, "%-12s %4d  ", _Dbg_filename, _Dbg_line );

	if ( !(_Dbg_level & _LEV_T) ) {
		fprintf(  _Dbg_fp, "   " );
	}

	vsprintf( buf, format, ap );
	fprintf( _Dbg_fp, "%s", buf );
	_printf(( "      %s : fprintf OK ! : buf = \"%s\"\n", __FUNCTION__, buf ));

	errno = err;

	/********/
	/* 終了 */
	/********/
ret:
	va_end( ap );
	_printf(( "Exit  (void)%s\n", __FUNCTION__ ));
	return;
}
#endif

/******************************************************************************
 * 関数名	dbg_chk_arg
 * 機能		引数のチェック
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_chk_arg( ac, av, dbg_level, procname, save_av )
int	*ac;
char	*av[];
char	**dbg_level;
char	**procname;
char	*save_av;
{
	int	ret_cd = OK;			/* リターン　コード	*/
	int	size;
	int	idx, i, j;
	char	*cp;

	_printf(( "Enter %s(ac=%d,av[0]=%s)\n", __FUNCTION__, *ac, av[0] ));

	/******************/
	/* エラーチェック */
	/******************/

	if ( ac == NULL || *ac <= 0 ) {
		_printf(( "      %s : ac ERR ! : ac = %d\n", __FUNCTION__, ac ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : ac, av OK !\n", func ));

	/********************/
	/* プロセス名　取得 */
	/********************/

	size = strlen( av[0] );

	if ( (*procname = neo_malloc(size+1)) == NULL ) {
		_printf(( "      %s : neo_malloc(%d) ERR !\n", func, size+1 ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : neo_malloc(%d) OK !\n", func, size + 1 ));

	if ( (cp = strrchr(av[0], '/')) == NULL ) {
		strcpy( *procname, av[0] );
	} else {
		strcpy( *procname, (cp+1) );
	}

	_printf(( "      %s : procname = %s\n", func, *procname ));

	/****************/
	/* 引数のセーブ */
	/****************/

	strcpy( save_av, *procname );
	strcat( save_av, " " );

	for (idx = 1; idx < *ac; idx++) {

		strcat( save_av, av[idx] );
		strcat( save_av, " " );
	}

	_printf(( "      %s : save av OK !\n", func ));

	/******************************/
	/* デバッグ　オプション　検索 */
	/******************************/

	for (idx = 1; idx < *ac; idx++) {

		if ( strcmp(av[idx], D_OPT_ARG) == 0 ) {

			if ( idx + 1 == *ac || av[idx+1] == NULL ) {
				_printf(( "      %s : av ERR !\n", func ));
				ret_cd = ERR;
				goto	ret;
			}

			size = strlen( av[idx+1] );
			if ( (*dbg_level = neo_malloc(size+1)) == NULL ) {
				_printf(( "      %s : neo_malloc ERR !\n",
					func ));
				ret_cd = ERR;
				goto	ret;
			}
			strcpy( *dbg_level, av[idx+1] );
			av[idx]   = NULL;
			av[idx+1] = NULL;
			_printf(( "      %s : dbg_level = \"%s\" !\n",
				func, *dbg_level ));
			break;
		}
	}

	/****************************/
	/* デバッグオプションの削除 */
	/****************************/

	for (i = j = 1; i < *ac; i++) {
		if (av[i])
			av[j++] = av[i];
	}
	av[j] = NULL;
	*ac = j;

	_printf(( "      %s : delete options OK !\n", func ));

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	dbg_set_level
 * 機能		デバッグレベルの設定
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_set_level( dbg_level, dbg_flg )
char	*dbg_level;
int	*dbg_flg;
{
	int	 ret_cd = OK;			/* リターン　コード	*/
	char	*class, *level;
	char	*cp;
	int	length;
	int	idx;

	if ( dbg_level ) {
		_printf(( "Enter %s(lev=%s,flg=%d)\n",
			__FUNCTION__, dbg_level, *dbg_flg ));
	} else {
		_printf(( "Enter %s(lev=null,flg=%d)\n", __FUNCTION__, *dbg_flg ));
	}

	/**************************/
	/* デバッグフラグ　初期化 */
	/**************************/

	*dbg_flg = 0;

	/********************************/
	/* デバッグレベル環境変数　取得 */
	/********************************/

	if ( dbg_level == NULL ) {	/* 1993年10月07日 (木) */
					/* 1993年10月27日 (水) */

		/******************/
		/* 環境変数　取得 */
		/******************/

		if ( (cp = getenv(D_OPT_ENV)) == NULL ) {
			_printf(( "      %s : getenv == NULL\n", func ));
			goto	ret;
		}

		_printf(( "      %s : getenv(%s) OK !\n", func, D_OPT_ENV ));

		/****************************/
		/* メモリー　アロケーション */
		/****************************/

		length = strlen( cp ) + 1;

		if ( (dbg_level = neo_malloc(length)) == NULL ) {

			_printf(( "      %s : neo_malloc ERR !\n", func ));
			ret_cd = ERR;
			goto	ret;
		}

		_printf(( "      %s : neo_malloc(%d) OK !\n", func, length ));

		/********************/
		/* 環境変数　コピー */
		/********************/

		strcpy( dbg_level, cp );
		_printf(( "      %s : strcpy OK !\n", func ));
	}

	_printf(( "      %s : dbg_level = %s\n", func, dbg_level ));

	/**********************************************/
	/* ＡＬＬの時は、全クラスにレベルをセットする */
	/**********************************************/

	length = strlen( D_OPT_ALL );

	if ( strncmp(dbg_level, D_OPT_ALL, length) == 0 ) {

		if ( dbg_level[length] == '\0' ) {
			_printf(("      %s : dbg_level(%s) ERR !\n",
				func, dbg_level ));
			ret_cd = ERR;
			goto	ret;
		}

		level = &(dbg_level[length+1]);

		if ( dbg_set_tbl_all(level) == ERR ) {
			_printf(("      %s : dbg_set_tbl_all ERR !\n", func ));
			ret_cd = ERR;
			goto	ret;
		}

		_printf(("      %s : dbg_set_tbl_all(%s) OK !\n", func, level));

		/**************************/
		/* デバッグフラグ　セット */
		/**************************/

		*dbg_flg = 1;
		_dbg_table();
		goto	ret;
	}

	/********************/
	/* テーブルの初期化 */
	/********************/

	for ( idx = 0; idx < D_MAX_TBL; idx++ ) {
		_Dbg_tbl[idx] = 0;
	}

	_printf(( "      %s : init _Dbg_tbl OK !\n", func ));

	/****************************************/
	/* デバッグレベルをテーブルへセットする */
	/****************************************/

	for (class = dbg_level; (class=strtok(class, D_OPT_DEL)); class = NULL){

		/* 1993年10月07日 (木) */

		if ( (level = strchr(class, D_OPT_LEV)) == NULL ) {
			_printf(( "      %s : strchr ERR ! : class = %s\n",
				func, class ));
			ret_cd = ERR;
			goto	ret;
		}

		*level = '\0';

		if ( *(++level) == '\0' ) {
			_printf(( "      %s : level = NULL !\n", func ));
			ret_cd = ERR;
			goto	ret;
		}

		_printf(( "      %s : strchr OK ! : class = %s : level = %s\n",
			func, class, level ));

		if ( dbg_set_tbl(class, level) == ERR ) {
			_printf(( "      %s : dbg_set_tbl ERR !\n", func ));
			ret_cd = ERR;
			goto	ret;
		}
	}

	_printf(( "      %s : set _Dbg_tbl OK !\n", func ));

	/**************************/
	/* デバッグフラグ　セット */
	/**************************/

	*dbg_flg = 1;
	_dbg_table();

	/********/
	/* 終了 */
	/********/
ret:
	if ( *dbg_flg ) {
		_printf(( "      %s : デバッグ　ＯＮ\n", func ));
	} else {
		_printf(( "      %s : デバッグ　ＯＦＦ\n", func ));
	}

	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	dbg_set_tbl
 * 機能		デバッグレベルをテーブルへセットする
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_set_tbl( class, level_str )
char	*class;
char	*level_str;
{
	int	ret_cd = OK;			/* リターン　コード	*/
	char	level;
	int	index;

	_printf(( "Enter %s(class=%s,level=%s)\n", __FUNCTION__, class, level_str ));

	/********************/
	/* クラス　チェック */
	/********************/

	if ( (index = dbg_get_class(class)) == ERR ) {
		_printf(( "      %s : dbg_get_class ERR !\n", func ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(("      %s : dbg_get_class OK ! class = %d\n", func, class));

	/********************/
	/* レベル　チェック */
	/********************/

	if ( level_str == NULL ) {
		_printf(( "      %s : check level ERR !\n", func ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : check level(%s) OK !\n", func, level_str ));

	if ( (level = dbg_get_level(level_str)) == 0 ) {
		_printf(( "      %s : dbg_get_level ERR !\n", func ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : dbg_get_level OK !\n", func ));

	/********************************/
	/* レベルをテーブルへセットする */
	/********************************/

	_Dbg_tbl[index] = level;
	_printf(( "      %s : set level OK !\n", func ));

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	dbg_set_tbl_all
 * 機能		デバッグレベルを全てのクラスへセットする
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_set_tbl_all( level_str )
char	*level_str;
{
	int	ret_cd = OK;			/* リターン　コード	*/
	char	level;
	int	idx;

	_printf(( "Enter %s(level=%s)\n", __FUNCTION__, level_str ));

	/********************/
	/* レベル　チェック */
	/********************/

	if ( level_str == NULL || level_str[0] == '\0' ) {
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : level OK !\n", func ));

	if ( (level = dbg_get_level(level_str)) == 0 ) {
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "      %s : dbg_get_level OK ! : level = %s\n",
		func, level_str ));

	/********************************/
	/* レベルをテーブルへセットする */
	/********************************/

	for ( idx = 0; idx < D_MAX_TBL; idx++ ) {
		_Dbg_tbl[idx] = level;
	}

	_printf(( "      %s : set level OK !\n", func ));


	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	dbg_get_class
 * 機能		クラステーブルを検索する
 * 関数値	テーブルのインデックス
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_get_class( class )
char	*class;
{
	int	index = ERR;			/* リターン　コード	*/
	neo_class_sym_t	*ptr;			/* ｸﾗｽ  ﾃｰﾌﾞﾙ  ﾎﾟｲﾝﾀｰ	*/

	_printf(( "Enter %s(class=%s)\n", __FUNCTION__, class ));

	/******************/
	/* エラーチェック */
	/******************/

	if ( class == NULL ) {
		_printf(( "      %s : class ERR !\n", func ));
		goto	ret;
	}

	_printf(( "      %s : class OK !\n", func ));

	/************************/
	/* クラステーブル　検索 */
	/************************/

	for ( ptr = _neo_class_tbl; ptr->c_sym; ptr++ ) {

		if ( strcmp(ptr->c_sym, class) == 0 ) {
			index = ptr->c_class;
			goto	ret;
		}
	}

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(index=%d)\n", func, index ));
	return( index );
}

/******************************************************************************
 * 関数名	dbg_get_level
 * 機能		レベルテーブルを検索し、レベル値を作成する
 * 関数値	レベル値
 *		０　：　エラー
 * 注意
 ******************************************************************************/

static	char
dbg_get_level( level_str )
char	*level_str;
{
	char	level = 0;			/* リターン　レベル	*/
	neo_level_sym_t	*ptr;			/* ﾚﾍﾞﾙ  ﾃｰﾌﾞﾙ  ﾎﾟｲﾝﾀｰ	*/
	int	size;
	int	idx;

	_printf(( "Enter %s(level=%s)\n", __FUNCTION__, level_str ));

	/******************/
	/* エラーチェック */
	/******************/

	if ( level_str == NULL ) {
		goto	ret;
	}

	_printf(( "      %s : level_str OK !\n", func ));

	/************************/
	/* レベルテーブル　検索 */
	/************************/

	size = strlen( level_str );

	for ( level = idx = 0; idx < size; idx++ ) {

		for ( ptr = _neo_level_tbl; ptr->c_sym; ptr++ ) {

			if ( ptr->c_sym == level_str[idx] ) {
				level |= ptr->c_level;
				_printf(( "      %s : search OK !\n", func ));
				break;
			}
		}
	}

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(level=%d)\n", func, level ));
	return( level );
}

/******************************************************************************
 * 関数名	dbg_open_file
 * 機能		出力ファイルのオープン
 * 関数値	０　　：　正常終了
 *		－１　：　エラー
 * 注意
 ******************************************************************************/

static	int
dbg_open_file( procname )
char	*procname;
{
	int	ret_cd = OK;			/* リターン　コード	*/
	char	*dbg_dir;			/* ディクトリー		*/
	char	outfile[2048];			/* 出力ファイル名	*/

	_printf(( "Enter %s(procname=%s)\n", __FUNCTION__, procname ));

	/**********************************/
	/* デバッグ　ディレクトリー　取得 */
	/**********************************/

	dbg_dir = getenv( D_OPT_DIR );

	if ( dbg_dir == NULL ) {
		_printf(( "     %s : getenv(%s) = null\n", func, D_OPT_DIR ));
		dbg_dir = ".";
	}

	_printf(( "     %s : dbg_dir OK ! : dbg_dir = %s\n", func, dbg_dir ));

	/**************************/
	/* 出力ファイル　オープン */
	/**************************/

	/* 1993年06月11日(金) */
	sprintf(outfile, "%s/%s.%s", dbg_dir, D_PREFIX, procname );

	if ( ! (_Dbg_fp = fopen(outfile, "w")) ){
		_printf(( "     %s : fopen(%s) ERR !\n", func, outfile ));
		ret_cd = ERR;
		goto	ret;
	}

	_printf(( "     %s : fopen(%s) OK !\n", func, outfile ));
	setbuf( _Dbg_fp, NULL );

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(ret=%d)\n", func, ret_cd ));
	return( ret_cd );
}

/******************************************************************************
 * 関数名	dbg_write_header
 * 機能		ヘッダーの書き込み
 * 関数値	なし
 * 注意
 ******************************************************************************/

static	void
dbg_write_header( save_av )
char	*save_av;
{
	_printf(( "Enter %s(save_av=%s)\n", __FUNCTION__, save_av ));

	/**********************/
	/* ヘッダー　書き込み */
	/**********************/

	fprintf( _Dbg_fp, "%s : %s\n\n", dbg_get_time(), save_av );
	_printf(( "     %s : write header OK !\n", func ));

	/********/
	/* 終了 */
	/********/

	_printf(( "Exit  (void)%s\n", func ));
}

/******************************************************************************
 * 関数名	dbg_get_time
 * 機能		システム時刻を文字列で取得する。
 * 関数値	現在時刻 (char*)
 * 注意
 ******************************************************************************/

static	char*
dbg_get_time()
{
	time_t	time();
	char	*ctime();
	long	t;
	static char	buf[32];

	_printf(( "Enter %s(void)\n", __FUNCTION__ ));

	buf[0] = '\0';

	if ( time(&t) == -1 ) {
		_printf(( "     %s : time() ERR ! : err = %d\n", func, errno ));
		goto	ret;
	}

	strcpy(buf, ctime(&t));
	buf[strlen(buf) - 1] = 0;

	/********/
	/* 終了 */
	/********/
ret:
	_printf(( "Exit  %s(ret=%s)\n", func, buf ));
	return buf;
}

/******************************************************************************
 * 関数名	_dbg_err_printf
 * 機能		デバッグ表示
 * 関数値	なし
 * 注意
 ******************************************************************************/

extern	char*	neo_errsym();

extern	void
_dbg_err_printf( str )
char	*str;
{
	if (str && *str) {
		_dbg_printf( "%s : %s\n", str, neo_errsym() );
	} else {
		_dbg_printf( "%s\n", neo_errsym() );
	}
}

#ifdef	DEBUG

/******************************************************************************
 * 関数名	debug_level
 * 機能		レベルテーブル　デバッグ 
 * 関数値	なし
 * 注意
 ******************************************************************************/

static	void
dbg_table()
{
	int	idx;
	char	classname[16];
	char	levelname[16];

	_printf(( "\n" ));
	_printf(( "/********************/\n" ));
	_printf(( "/* レベル　テーブル */\n" ));
	_printf(( "/********************/\n" ));

	for ( idx = 0; idx < D_MAX_TBL; idx++ ) {

		if ( _Dbg_tbl[idx] != 0 ) {
			_dbg_find_classname( idx, classname );

			if ( classname[0] != '\0' ) {
				_dbg_find_levelname( _Dbg_tbl[idx], levelname );
				_printf(("%-16s  %s\n", classname, levelname));
			}
		}
	}
	_printf(( "/********************/\n\n" ));
}

static	void
dbg_find_classname( class, name )
int	class;
char	*name;
{
	neo_class_sym_t	*ptr;

	name[0] = '\0';

	for ( ptr = _neo_class_tbl; ptr->c_sym; ptr++ ) {

		if ( class == ptr->c_class ) {
			strcpy( name, ptr->c_sym );
			break;
		}
	}
}

static	void
dbg_find_levelname( level, name )
char	level;
char	*name;
{
	neo_level_sym_t	*ptr;
	char	*cp;

	cp = name;

	for ( ptr = _neo_level_tbl; ptr->c_sym; ptr++ ) {

		if ( level & ptr->c_level ) {
			*cp = ptr->c_sym;
			cp++;
		}
	}
	*cp = '\0';
}

#endif	// DEBUG
