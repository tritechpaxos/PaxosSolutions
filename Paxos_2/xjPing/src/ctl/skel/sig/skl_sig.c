/*******************************************************************************
 * 
 *  skl_sig.c --- xjPing (On Memory DB) skelton (signal Handring) Library programs
 * 
 *  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

#include	"skl_sig.h"

#ifdef DEBUG
extern void neo_exit();
#endif

/*
 *	シグナル処理 管理テーブル
 */
static _neo_signal_t	_sig_tbl[] = {
{SIGHUP,    0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
#ifdef DEBUG
{SIGINT,    NEO_SIGINT,	  1, _hndl_dfl,(int(*)())neo_exit,_pthndl_dfl, 0},
#else
{SIGINT,    NEO_SIGINT,	  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
#endif
{SIGQUIT,   NEO_SIGINT,	  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGILL,    NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},
{SIGTRAP,   0,            0, 0,		0,           0,		  0},
{SIGABRT,   NEO_SIGINT,	  0, _hndl_dfl, 0,           _pthndl_dfl, 0},
/*{SIGEMT,    NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},*/
{SIGFPE,    NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},
{SIGKILL,   0,            0, 0,		0,           0,		  0},
{SIGBUS,    NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},
{SIGSEGV,   0,            0, 0,         0,           0,           0},
{SIGSYS,    NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},
{SIGPIPE,   NEO_SIGPIPE,  1, _hndl_dfl,	0,           _pthndl_dfl, 0},
{SIGALRM,   0,            1, _hndl_itm,	0,           0,		  0},
{SIGTERM,   NEO_SIGTERM,  1, _hndl_dfl,	_prhndl_trm, _pthndl_dfl, 0},
{SIGURG,    0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGSTOP,   0,            0, 0,		0,           0,		  0},

{SIGTSTP,   0,         	  0, 0,         0,           0,           0},
{SIGCONT,   0,         	  0, 0,         0,           0,           0},

{SIGCLD,    NEO_SIGCLD,   1, _hndl_cld,	_prhndl_cld, _pthndl_dfl, 0},
{SIGTTIN,   0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGTTOU,   0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGIO,     0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGXCPU,   0,            0, 0,		0,           0,		  0},
{SIGXFSZ,   0,            0, 0,		0,           0,		  0},
{SIGVTALRM, 0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGPROF,   0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGWINCH,  0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGUSR1,   NEO_SIGUSR1,  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
{SIGUSR2,   NEO_SIGUSR2,  1, _hndl_dfl, 0,           _pthndl_dfl, 0},

#ifdef SIGLOST
{SIGLOST,   NEO_SIGFAULT, 1, _hndl_dfl, 0,           _pthndl_flt, 0},
#endif
#ifdef SIGPWR
{SIGPWR,    0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
#endif
#ifdef SIGWAITING
{SIGWAITING,0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
#endif
#ifdef SIGLWP
{SIGLWP,    0,		  1, _hndl_dfl, 0,           _pthndl_dfl, 0},
#endif

{0, 0, 0, 0, 0, 0, 0},
};


/*
 *	シグナルマスク 管理テーブル
 */
static _neo_sigmask_t	_sig_mask[] = {
{0, 0},
{NEO_SIGFAULT, 0},
{NEO_SIGINT,   0},
{NEO_SIGPIPE,  0},
{NEO_SIGTERM,  0},
{NEO_SIGCLD,   0},
{NEO_SIGUSR1,  0},
{NEO_SIGUSR2,  0},
{0, 0},
};

/*
 *	シグナルフラグ
 */
static int	_signal_flag;

/*
 *	シグナルマスク
 */
static int	_signal_mask;
static int	_signal_pending;

#undef sigmask
/********* undef sigmask は下記の事情による
	/usr/include/signal.h の中で
	#ifdef __USE_BSD
	/ * Compute mask for signal SIG.  * /
	# define sigmask(sig)   __sigmask(sig)
    ......................................
	#endif / * Use BSD.  * /
と定義されているため
************************* ********************/

int
sigmask( sig )
	int	sig;
{
DBG_ENT( M_SKL, "sigmask" );
	return( 1<<sig );
}

int
sigblock( mask )
	int	mask;
{
	int 	prev;

DBG_ENT( M_SKL, "sigblock" );
	prev = _signal_mask;

	_signal_mask |= mask;

DBG_EXIT( prev );
	return( prev );
}

int
sigsetmask( mask )
	int	mask;
{
	int 	prev;
	int pended;
	_neo_signal_t	*ps;

DBG_ENT( M_SKL, "sigsetmask" );

	prev = _signal_mask;
	_signal_mask = mask;

	pended = _signal_pending & ~mask;
	if( pended ) {
		for( ps = _sig_tbl; ps->neo_sig; ps++ ) {
			if( pended & _sig_mask[ps->neo_sig].mask ) {

				(*ps->_neo_hndl)( ps->sig );

				_signal_pending 
					&= ~_sig_mask[ps->neo_sig].mask;
			}
		}
	}
DBG_EXIT( prev );
	return( prev );
}


/******************************************************************************
 * 関数名
 *		_neo_init_signal()
 * 機能        
 *		シグナル処理 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_init_signal()
{
	_neo_signal_t	*ps;

DBG_ENT( M_SKL, "_neo_init_signal" );

	for( ps = _sig_tbl; ps->sig; ps++ ) {

		/*
		 *	シグナルハンドラ 登録
		 */
		if( ps->set_flag )
			signal( ps->sig, ps->_neo_hndl );

		/*
		 *	シグナルマスク 登録
		 */
		if( ps->neo_sig )
			_sig_mask[ps->neo_sig].mask |= sigmask(ps->sig);
	}

	/*
	 *	シグナルフラグ 初期化
	 */
	_neo_sigflag_init();

	/*
	 *	シグナルマスク 初期化
	 */
	_neo_sigmask_init();

/***
	siginterrupt( SIGTERM, 1 );
***/

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_sighold()
 * 機能        
 *		シグナルのホールド
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_sighold( neo_sig )
int	neo_sig;	/* I シグナル番号(NEO)	*/
{
	int	ret = 0;

DBG_ENT( M_SKL, "neo_sighold" );

	/*
	 *	引数チェック
	 */
	if( neo_sig <= 0 || neo_sig > NEO_SIGUSR2 ) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	/*
	 *	シグナルブロック
	 */
	sigblock( _sig_mask[neo_sig].mask );

end:;
DBG_EXIT(ret);
	return( ret );
err:;
	ret = -1;
	goto end;
}

/******************************************************************************
 * 関数名
 *		neo_sigrelse()
 * 機能        
 *		シグナルのリリース
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_sigrelse( neo_sig )
int	neo_sig;	/* I シグナル番号(NEO)	*/
{
	int	ret = 0;

DBG_ENT( M_SKL, "neo_sigrelse" );

	/*
	 *	引数チェック
	 */
	if( neo_sig <= 0 || neo_sig > NEO_SIGUSR2 ) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	_signal_mask &= ~_sig_mask[neo_sig].mask;

end:;
DBG_EXIT(ret);
	return( ret );
err:;
	ret = -1;
	goto end;
}


/******************************************************************************
 * 関数名
 *		neo_signal()
 * 機能        
 *		シグナル処理 登録／削除
 * 関数値
 *      正常: !-1 (以前の登録関数)
 *      異常: -1
 * 注意
 ******************************************************************************/
void
(*neo_signal( neo_sig, func ))()
int	neo_sig;	/* I シグナル番号(NEO)	*/
void	(*func)();	/* I ユーザ定義関数 アドレス	*/
{
	int		before;
	_neo_signal_t	*ps;
	void		(*prefunc)() = (void (*)())-1;

DBG_ENT( M_SKL, "neo_signal" );
DBG_C(( "neo_sig=%d, func=%#x\n", neo_sig, func ));

	/*
	 *	引数チェック
	 */
	if( neo_sig <= 0 || neo_sig > NEO_SIGUSR2 ) {
		neo_errno = E_SKL_INVARG;
		goto end;
	}

	/*
	 *	シグナルブロック
	 */
	before = sigblock( _sig_mask[neo_sig].mask );

	/*
	 *	登録
	 */
	for( ps = _sig_tbl; ps->sig; ps++ ) {

		if( (ps->neo_sig) && (ps->neo_sig == neo_sig) ) {
			prefunc = ps->_usr_hndl;
			ps->_usr_hndl = func;
		}
	}

	/*
	 *	ブロック解除
	 */
	sigsetmask( before );

end:;
DBG_EXIT(prefunc);
	return( prefunc );
}


/******************************************************************************
 * 関数名
 *		_hndl_dfl()
 * 機能        
 *		シグナルハンドラ （デフォルト）
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_hndl_dfl( sig, code, scp, addr )
int	sig;		/* I シグナル番号	*/
int	code;		/* I パラメータ	*/
struct sigcontext *scp;	/* I コンテキストを復元するために用いる構造体 */
char	*addr;		/* I アドレスに関する付加情報	*/
{
	_neo_signal_t	*ps;

DBG_ENT( M_SKL, "_hndl_dfl" );
DBG_C(( "signal no=%d \n", sig ));

	/*
	 *	構造体の検索
	 */
	ps = _tbl_search( sig );
	
	if( _sig_mask[ps->neo_sig].mask & _signal_mask ) {

		_signal_pending |= _sig_mask[ps->neo_sig].mask;

		return;
	}
	/*
	 *	前処理
	 */
	if( ps->_neo_prhndl )
		(*ps->_neo_prhndl)();

	/*
	 *	ユーザ定義処理
	 */
	if( ps->_usr_hndl )
		(*ps->_usr_hndl)( ps->neo_sig );

	/*
	 *	後処理
	 */
	if( ps->_neo_pthndl )
		(*ps->_neo_pthndl)( sig );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_hndl_cld()
 * 機能        
 *		シグナルハンドラ （SIGCLD）
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_hndl_cld( sig, code, scp, addr )
int	sig;		/* I シグナル番号	*/
int	code;		/* I パラメータ	*/
struct sigcontext *scp;	/* I コンテキストを復元するために用いる構造体 */
char	*addr;		/* I アドレスに関する付加情報	*/
{
	_neo_signal_t	*ps;
	int		ret, pid, status;

DBG_ENT( M_SKL, "_hndl_cld" );
DBG_C(( "signal no=%d \n", sig ));

	/*
	 *	構造体の検索
	 */
	ps = _tbl_search( sig );
	
	if( _sig_mask[ps->neo_sig].mask & _signal_mask ) {

		_signal_pending |= _sig_mask[ps->neo_sig].mask;

		return;
	}

	while( 1 ) {
		/*
		 *	前処理
		 */
		ret = (*ps->_neo_prhndl)( &pid, &status );

		switch( ret ) {
		case -1:
		case  0:
			goto post;

		default:
DBG_C(( "Die child process pid=%d, status=%#x \n", pid, status ));
			/*
			 *	ユーザ定義処理
			 */
			if( ps->_usr_hndl ) {
				(*ps->_usr_hndl)( ps->neo_sig, pid, status );
			}
			continue;
		}
	}
post:;
	/*
	 *	後処理
	 */
	if( ps->_neo_pthndl )
		(*ps->_neo_pthndl)( sig );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_prhndl_trm()
 * 機能        
 *		シグナルハンドラ 前処理（SIGTERM)
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static int
_prhndl_trm()
{
DBG_ENT( M_SKL, "_prhndl_trm" );

	_neo_isterm_set();

DBG_EXIT(0);
	return( 0 );
}


/******************************************************************************
 * 関数名
 *		_prhndl_cld()
 * 機能        
 *		シグナルハンドラ 前処理（SIGCLD)
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static int
_prhndl_cld( pid, status )
int	*pid;		/* O プロセスｉｄ	*/
int	*status;	/* O 終了ステータス	*/
{
DBG_ENT( M_SKL, "_prhndl_cld" );

	while( 1 ) {
		*pid = wait3( status, WNOHANG, 0 );

		switch( *pid ) {
		/*
		 *	エラー
		 */
		case -1:
			if( errno == EINTR )
				continue;
			goto end;

		/*
		 *	終了子プロセス無し
		 */
		case  0:
			goto end;

		/*
		 *	終了子プロセス有り
		 */
		default:
			_neo_child_remove( *pid );
			goto end;
		}
	}
end:;
DBG_EXIT(*pid);
	return( *pid );
}


/******************************************************************************
 * 関数名
 *		_pthndl_dfl()
 * 機能        
 *		シグナルハンドラ 後処理（デフォルト)
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static int
_pthndl_dfl( sig )
int	sig;	/* I シグナル番号	*/
{
DBG_ENT( M_SKL, "_pthndl_dfl" );

	_neo_sigflag_set( sig );

DBG_EXIT(sig);
	return( 0 );
}


/******************************************************************************
 * 関数名
 *		_pthndl_flt()
 * 機能        
 *		シグナルハンドラ 後処理（NEO_SIGFAULT)
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static int
_pthndl_flt( sig )
int	sig;	/* I シグナル番号	*/
{
DBG_ENT( M_SKL, "_pthndl_flt" );
DBG_C(( "signal no=%d \n", sig ));

	_neo_epilogue( 1 );

	/*
	 *	コアダンプ
	 */
	signal( sig, SIG_DFL );
	kill( getpid(), sig );

DBG_EXIT(1);
	return( 1 );
}


/******************************************************************************
 * 関数名
 *		_tbl_search()
 * 機能        
 *		シグナル管理構造体 検索
 * 関数値
 *	シグナル管理構造体へのポインタ
 * 注意
 ******************************************************************************/
static	_neo_signal_t *
_tbl_search( sig )
int	sig;	/* I シグナル番号	*/
{
	_neo_signal_t	*ps;

DBG_ENT( M_SKL, "_tbl_search" );
DBG_C(( "signal no=%d \n", sig ));

	for( ps = _sig_tbl; ps->sig; ps++ ) {
		if( sig == ps->sig )
			break;
	}

DBG_EXIT(ps);
	return( ps );
}


/******************************************************************************
 * 関数名
 *		_neo_sigflag_set()
 * 機能        
 *		シグナルフラグ 設定
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_sigflag_set( sig )
int	sig;	/* I シグナル番号	*/
{
DBG_ENT( M_SKL, "_neo_sigflag_set" );
DBG_C(( "signal no=%d \n", sig ));

	_signal_flag = sig;

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_neo_sigflag_init()
 * 機能        
 *		シグナルフラグ 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_sigflag_init()
{
DBG_ENT( M_SKL, "_neo_sigflag_init" );

	_signal_flag = 0;

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_neo_sigflag_get()
 * 機能        
 *		シグナルフラグ 取得
 * 関数値
 *	シグナルフラグ 番号
 * 注意
 ******************************************************************************/
int
_neo_sigflag_get()
{
DBG_ENT( M_SKL, "_neo_sigflag_get" );
DBG_EXIT(_signal_flag);

	return( _signal_flag );
}


/******************************************************************************
 * 関数名
 *		_neo_sigmask_init()
 * 機能        
 *		シグナルマスク 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_neo_sigmask_init()
{
DBG_ENT( M_SKL, "_neo_sigmask_init" );

	_signal_mask = 0;

DBG_EXIT(0);
}
