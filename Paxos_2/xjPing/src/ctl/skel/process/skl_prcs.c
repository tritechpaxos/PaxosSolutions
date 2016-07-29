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
 *	skl_prcs.c
 *
 *	説明	スケルトン プロセス管理
 * 
 ******************************************************************************/

#include	"skl_prcs.h"

/*
 *	プロセス管理リスト エントリ
 */
static _dlnk_t	_child_entry;

/*
 *	終了通知フラグ
 */
static int	_isterm_flag;

static	void	_neo_child_clean();

/******************************************************************************
 * 関数名
 *		_neo_init_prcs()
 * 機能        
 *		プロセス管理 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_init_prcs()
{
DBG_ENT( M_SKL, "_neo_init_prcs" );

	_dl_init( &_child_entry );
	_isterm_flag = 0;

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_fork()
 * 機能        
 *		子プロセスの生成
 * 関数値
 *      正常: 親: 0< (プロセスid)
 *          : 子: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_fork()
{
	_neo_child_t	*pt;
	int		pid = -1, before;

DBG_ENT( M_SKL, "neo_fork" );

    	if( !(pt = (_neo_child_t *)neo_malloc(sizeof(_neo_child_t))) ) {
		goto end;
	}

	switch( pid = fork() ) {

	/*
	 *	Child process
	 */
	case 0:
		neo_free( pt );
		_neo_child_clean();
		break;

	/*
	 *	Error
	 */
	case -1:
		neo_free( pt );
		/* 
		 * UNIX ERR 
		 */
		neo_errno = unixerr2neo();
		break;
		
	/*
	 *	Parent process
	 */
	default:
		before = sigblock( sigmask( SIGCLD ) );

		pt->pid = pid;

		_dl_insert( pt, &_child_entry );

		sigsetmask( before );

		break;
	}

end:;
DBG_EXIT(pid);
	return( pid );
}


/*******************************************************************************
 * 関数名
 *		neo_exec
 * 機能
 * 		プログラムの起動
 * 関数値
 *	無し: 正常
 *	!0:   異常
 * 注意
 ******************************************************************************/
int
neo_exec(path, execstr, envstr)
char	*path;		/* ロードモジュールパス名 */
char	*execstr;	/* 実行形式 */
char	*envstr;	/* 環境変数 */
{
	int	ac, nenv, i;
	char	*av[20], *env[20];
	struct	stat	st;

DBG_ENT( M_SKL, "neo_exec" );
DBG_C(( "path = (%s) execstr = (%s) envstr = (%s) \n", path, execstr, envstr ));

	/*
	 *	ファイル検査
	 */
	if (stat(path, &st) < 0) {	 /* ### ファイルがない */
		DBG_A(( "%s のファイルがありません", path ));
		neo_errno = E_SKL_INVARG;
		goto err;
	}
	ac = _neo_ArgMake(ARR_SIZE(av), av, execstr, 0, 0);

	/*
	 *	環境変数
	 */
	if (envstr) {
		nenv = _neo_ArgMake(ARR_SIZE(env), env, envstr, 0, 0);
		if (nenv > 0) {
			for (i=0; i < nenv; i++) {
				putenv(env[i]);
			}
		}
	}

	/*
	 *	fdクローズ
	 */
	for (i = 3; i < NOFILE; i++)
		close(i);

	/*
	 *	起動
	 */
	execv(path, av);

	/* 
	 * UNIX ERR 
	 */
	neo_errno = unixerr2neo();

	/* ### 起動エラー(exec) */
	DBG_A(( "%s 起動エラーです", path ));

err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		neo_term()
 * 機能        
 *		子プロセスへの終了要求
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
neo_term()
{
	_neo_child_t	*pt;
	int		before;

DBG_ENT( M_SKL, "neo_term" );

	/*
	 *	SIGCLD ブロック
	 */
	before = sigblock( sigmask( SIGCLD ) );

	/*
	 *	終了シグナル送信
	 */
        for( pt = (_neo_child_t *)_dl_head( &_child_entry );
			_dl_valid( &_child_entry, pt );
			pt = (_neo_child_t *)_dl_next( pt ) ) {

		kill( pt->pid, SIGTERM );
	}

	/*
	 *	子プロセス終了判定
	 */
	while( _dl_head( &_child_entry ) )
		sigpause( 0 );
	
	/*
	 *	SIGCLD ブロック解除
	 */
	sigsetmask( before );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_isterm()
 * 機能        
 *		プロセス終了要求の監視
 * 関数値
 *	要求有り: !0
 *	要求無し: 0
 * 注意
 ******************************************************************************/
int
neo_isterm()
{
DBG_ENT( M_SKL, "neo_isterm" );

	if( _isterm_flag )
		neo_errno = E_SKL_TERM;

DBG_EXIT(_isterm_flag);
	return( _isterm_flag );
}


/******************************************************************************
 * 関数名
 *		_neo_isterm_set()
 * 機能        
 *		プロセス終了要求のセット
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_isterm_set()
{
DBG_ENT( M_SKL, "_neo_isterm_set" );

	_isterm_flag = -1;

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_neo_child_remove()
 * 機能        
 *		子プロセスのリストからの削除
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_child_remove( pid )
int	pid;		/* I プロセスid	*/
{
	_neo_child_t	*pt;

DBG_ENT( M_SKL, "_neo_child_remove" );
DBG_C(( "pid=%d\n", pid ));

	for( pt = (_neo_child_t *)_dl_head( &_child_entry );
			_dl_valid( &_child_entry, pt );
			pt = (_neo_child_t *)_dl_next( pt ) ) {

		if( pt->pid == pid ) {
			_dl_remove( pt );
			neo_free( pt );
			break;
		}
	}

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_neo_child_clean()
 * 機能        
 *		子プロセスのリストからの全削除
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_neo_child_clean()
{
	_neo_child_t	*pt, *tpt;

DBG_ENT( M_SKL, "_neo_child_clean" );

	for( pt = (_neo_child_t *)_dl_head( &_child_entry );
			_dl_valid( &_child_entry, pt ); ) {

DBG_C(( "Clean struct (pid=%d)\n", pt->pid ));

		tpt = pt;
		pt = (_neo_child_t *)_dl_next( pt );

		_dl_remove( tpt );
		neo_free( tpt );
	}

DBG_EXIT(0);
}
