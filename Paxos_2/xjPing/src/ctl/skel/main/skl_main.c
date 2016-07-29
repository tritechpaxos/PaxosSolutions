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
 *	skl_main.c
 *
 *	説明	スケルトン メイン処理
 * 
 ******************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	"neo_dlnk.h"
#include	"neo_skl.h"
#include	"neo_system.h"
#include	"neo_usradm.h"

/**
extern void	_neo_init_mem();
**/
extern void	_neo_init_signal();
extern void	_neo_init_lock();
extern void	_neo_init_itimer();
extern void	_neo_init_prcs();
extern void	_neo_init_epilog();
extern void	_neo_exec_epilog();

extern	int	_dbg_init();
extern	int	_dbg_term();

extern	int	neo_main();
/*
 * Process Name
 */
__attribute__((weak))
char	*_neo_procname;		/* full path name */
__attribute__((weak))
char	*_neo_procbname;	/* base name 	*/

/*
 * Host Name
 */
__attribute__((weak))
char	_neo_hostname[NEO_NAME_MAX];

/*
 * process id
 */
__attribute__((weak))
int	_neo_pid;

/*
 * ネオ．エラーＮＯ．
 */
__attribute__((weak))
int	neo_errno;

/*
 * ポートエントリ
 */
__attribute__((weak))
_dlnk_t	_neo_port;


/*
 *	Log
 */
__attribute__((weak))
struct Log	*neo_log;

/******************************************************************************
 * 関数名
 *		_neo_prologue()
 * 機能        
 *		スケルトン プロローグ処理
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
_neo_prologue( ac, av )
int	*ac;		/* IO 引数の数	*/
char	*av[];		/* IO 引数文字列	*/
{
	char	*c, *p;
	char	buf[512];
	int	i;

DBG_ENT( M_SKL, "_neo_prologue" );

	/***
	_neo_init_mem();
	***/

	if( _dbg_init(ac, av) < 0 ) {
		printf("Can not open Debug file.\n");
		goto err;
	}

	_neo_log_open( av[0] );

	_dl_init( &_neo_port );
	_neo_init_signal();
	_neo_init_lock();
	_neo_init_itimer();
	_neo_init_epilog();
	_neo_init_prcs();
	neo_errno = 0;

	sm_proc_start( getpid(), av );

	_neo_procname	= *av;
	for( c = (av[0] + strlen(av[0])); *c != '/' && c >= av[0]; c-- ) p = c;
	_neo_procbname	= p;
	_neo_pid	= getpid();
	gethostname( _neo_hostname, NEO_NAME_MAX );

	for( i = 0, c = buf; i < *ac; i++, c = buf + strlen(buf) ) 
		sprintf( c, "%s ", av[i] );
	neo_log_msg( M_SKL, "PROG_START: %s", buf );

//ok:;
DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		_neo_epilogue()
 * 機能        
 *		スケルトン エピローグ処理
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_epilogue( status )
int	status;		/* I 終了ステータス	*/
{
DBG_ENT( M_SKL, "_neo_epilogue" );

	neo_term();
	_neo_exec_epilog();
	neo_log_msg( M_SKL, "PROG_END", "" );

	sm_proc_term( getpid() );

	_neo_log_close();

	_dbg_term();

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_exit()
 * 機能        
 *		スケルトン 終了
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
neo_exit( status )
int	status;		/* I 終了ステータス	*/
{
DBG_ENT( M_SKL, "neo_exit" );

	_neo_epilogue( status );

DBG_EXIT(status);
	exit( status );
}


/******************************************************************************
 * 関数名
 *		main()
 * 機能        
 *		スケルトン 開始
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
/***
#pragma weak main
#pragma weak neo_main
***/
__attribute__((weak))
int
neo_main( ac, av )
{
	return( -1 );
}

__attribute__((weak))
int
main( ac, av )
int	ac;		/* IO 引数の数	*/
char	*av[];		/* IO 引数文字列	*/
{
	int	status = 1;

DBG_ENT( M_SKL, "main" );

	/*
	 *	プロローグ処理
	 */
	if( _neo_prologue( &ac, av ) )
		goto epilogue;

	/*
	 *	ユーザ メイン処理
	 */
	status = neo_main( ac, av );

epilogue:;
DBG_EXIT(status);
	/*
	 *	エピローグ処理
	 */
	neo_exit( status );
	return( 0 );
}

