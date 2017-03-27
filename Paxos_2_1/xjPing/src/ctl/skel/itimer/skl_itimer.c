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
 *	skl_itimer.c
 *
 *	説明	スケルトン インタバルタイマ
 * 
 ******************************************************************************/

#include	"skl_itimer.h"


/* 
 *	インタバルタイマ管理リスト エントリ
 */
static _dlnk_t	_itimer_entry;


/******************************************************************************
 * 関数名
 *		_neo_init_itimer()
 * 機能        
 *		インタバルタイマ 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_init_itimer()
{
DBG_ENT( M_SKL, "_neo_init_itimer" );

	_dl_init( &_itimer_entry );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_setitimer()
 * 機能        
 *		インタバルタイマ 登録
 * 関数値
 *      正常: !NULL (インタバルタイマ番号)
 *      異常: NULL
 * 注意
 ******************************************************************************/
void *
neo_setitimer( func, arg, msec, mode )
void	(*func)();	/* I ユーザ登録関数		*/
void	*arg;		/* I ユーザ登録関数実行時の引数	*/
long	msec;		/* I インタバル値		*/
int	mode;		/* I 起動モード			*/
{
	_neo_itimer_t	*newt;
	long		sec, usec;

DBG_ENT( M_SKL, "neo_setitimer" );
DBG_C(( "func(%#x) arg(%#x) msec(%d) mode(%d)\n", func, arg, msec, mode ));

	/*
	 *	引数チェック
	 */
	if( (msec <= 0) || ((mode != NEO_ONESHOT) && (mode != NEO_CYCLE)) ) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	if( !(newt = (_neo_itimer_t *)neo_malloc( sizeof(_neo_itimer_t) )) ) {
		goto err;
	}

	/*
	 *	発行中のタイマ停止
	 */
	if( _dl_head(&_itimer_entry) ) {
		_it_timer( 0, 0, 0, 0 );
	}

	/*
	 *	登録情報設定
	 */
	msec = ( (msec + 99)/100*100 );
	sec  = msec/1000;
	usec = msec - sec*1000;

	newt->id		= (void *)newt;
	newt->mode		= mode;
	newt->itime.tv_sec	= sec;
	newt->itime.tv_usec	= usec;
	newt->func		= func;
	newt->arg		= arg;

	/*
	 *	タイマ管理構造体の登録
	 */
	_it_entlist( newt );

	/*
	 *	タイマ発行
	 */
	_it_alarm();

DBG_EXIT(newt);
	return( (void *)newt );
err:;
DBG_EXIT(0);
	return( (void *)0 );
}


/******************************************************************************
 * 関数名
 *		neo_delitimer()
 * 機能        
 *		インタバルタイマ 削除
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_delitimer( id )
void	*id;	/* I インタバルタイマ番号	*/
{
	int	ret = -1;

DBG_ENT( M_SKL, "neo_delitimer" );
DBG_C(( "id(%#x) \n", id ));

	/*
	 *	引数チェック
	 */
	if( _dl_head(&_itimer_entry) ) {
		/*
		 *	発行中のタイマ停止
		 */
		_it_timer( 0, 0, 0, 0 );
	} else {
		neo_errno = E_SKL_INVARG;
		goto end;
	}

	/*
	 *	タイマ管理構造体の削除
	 */
	ret = _it_delist( (_neo_itimer_t *)id );

	/*
	 *	タイマ発行
	 */
	_it_alarm();

end:;
DBG_EXIT(ret);
	return( ret );
}

/******************************************************************************
 * 関数名
 *		neo_chgitimer()
 * 機能        
 *		インタバルタイマ インタバル値の変更
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_chgitimer( pt, msec )
_neo_itimer_t	*pt;	/* I インタバルタイマ番号	*/
long		msec;	/* I インタバル値		*/
{
	int	before;
	long	sec, usec;

	DBG_ENT( M_SKL, "neo_chgitimer" );
	DBG_B(( "### pt(%#x),msec(%ld)\n", pt,msec ));

	before = sigblock( sigmask(SIGALRM) );

	/*
	 *	引数チェック
	 */
	if( (pt->mode != NEO_CYCLE) || (msec <= 0) || (pt->id != (void*)pt)) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	/*
	 *	登録情報設定
	 */
	msec = ( (msec + 99)/100*100 );
	sec  = msec/1000;
	usec = msec - sec*1000;

	pt->itime.tv_sec	= sec;
	pt->itime.tv_usec	= usec;

	sigsetmask(before);
	DBG_EXIT(0);
	return( 0 );

err:;
	sigsetmask(before);
	DBG_EXIT(-1);
	return( -1 );
}

/******************************************************************************
 * 関数名
 *		neo_sleep()
 * 機能        
 *		スリープ
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_sleep( sec )
unsigned int	sec;	/* I スリープ秒数	*/
{
DBG_ENT( M_SKL, "neo_sleep" );
DBG_C(( "sec(%d) \n", sec ));

	/*
	 *	シグナルフラグの初期化
	 */
	_neo_sigflag_init();

	/*
	 *	タイマ発行
	 */
	if( neo_setitimer( _skl_sleep, 0, (sec * 1000), NEO_ONESHOT ) == 0 ) {
		goto err;
	}

	while( 1 ) {
		pause();
		if( _neo_sigflag_get() )
			break;
	}


DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		_skl_sleep()
 * 機能        
 *		シグナルフラグ(SIGALRM)の設定
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_skl_sleep()
{
DBG_ENT( M_SKL, "_it_sleep" );

	_neo_sigflag_set( SIGALRM );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_hndl_itm()
 * 機能        
 *		シグナルハンドラ(SIGALRM)
 * 関数値
 *	無し
 * 注意
 *	  ユーザ定義処理関数の中で, インタバルタイマの削除を行なわれた時,
 *	リストの先頭が変わっている可能性があるため、ユーザ定義処理関数の
 *	実行後、もう一度リストの先頭を取り出す。
 *	  その値を見て、登録メンバの削除／再登録を行なう
 ******************************************************************************/
void
_hndl_itm()
{
	_neo_itimer_t	*pt, *pt2;
#ifdef CHKTEST
static int prev = 0;
int s,t;
#endif
DBG_ENT( M_SKL, "_hndl_itm" );
#ifdef CHKTEST
t = time(0);
if( (s = t - prev) != 72 )
	DBG_A(("**************** HANDLER in(%d) from(%d) = (%d)\n",t,prev,s));
prev = t;
#endif

	if( !(pt = (_neo_itimer_t *)_dl_head(&_itimer_entry)) ) {
		goto end;
	}

	/*
	 *	ユーザ定義処理関数
	 */
	(*pt->func)(pt->arg);

	if( !(pt2 = (_neo_itimer_t *)_dl_head(&_itimer_entry)) ) {
		goto end;
	}

	/*
	 *	一回だけ実行
	 */
	if( pt == pt2 && pt->mode == NEO_ONESHOT ) {
		_it_delist( pt );

	/*
	 *	繰り返し実行
	 */
	} else if( pt == pt2 && pt->mode == NEO_CYCLE ) {
		_dl_remove( pt );
		_it_entlist( pt );
	}

	/*
	 *	タイマ発行
	 */
	_it_alarm();

end:;
DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		_it_entlist()
 * 機能        
 *		シグナル構造体のリストへの登録
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
static int
_it_entlist( newt )
_neo_itimer_t	*newt;	/* I シグナル構造体	*/
{
	struct timeval	tp;
	struct timezone	tzp;
	long		sec, usec;
	_neo_itimer_t	*pt;

DBG_ENT( M_SKL, "_it_entlist" );

	/*
	 *	現在時刻の取得
	 */
	gettimeofday(&tp, &tzp);


	/*
	 *	起動時刻の計算
	 */
	usec = tp.tv_usec + newt->itime.tv_usec;

	if( usec/1000000 ) {
		usec = usec - 1000000;
		sec = tp.tv_sec + newt->itime.tv_sec + 1;
	} else {
		sec = tp.tv_sec + newt->itime.tv_sec;
	}

	newt->stime.tv_sec	= sec;
	newt->stime.tv_usec	= usec;


	/*
	 *	リストへの登録
	 */
	for( pt = (_neo_itimer_t *)_dl_head( &_itimer_entry ); 
			_dl_valid( &_itimer_entry, pt );
			pt = (_neo_itimer_t *)_dl_next( pt ) ) {

		if( (pt->stime.tv_sec > sec) || 
		    ((pt->stime.tv_sec == sec) && (pt->stime.tv_usec > usec)) ){
			_dl_insert( newt, pt );
			goto ok;
		}
	}

	_dl_insert( newt, &_itimer_entry );

ok:;
DBG_EXIT(0);
	return( 0 );
}


/******************************************************************************
 * 関数名
 *		_it_dellist()
 * 機能        
 *		シグナル構造体のリストからの削除
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
static int	
_it_delist( del )
_neo_itimer_t	*del;	/* I シグナル構造体	*/
{
	_neo_itimer_t		*pt;

DBG_ENT( M_SKL, "_it_delist" );

	for( pt = (_neo_itimer_t *)_dl_head( &_itimer_entry ); 
			_dl_valid( &_itimer_entry, pt ); 
			pt = (_neo_itimer_t *)_dl_next( pt ) ) {

		if( pt == del ) {
			_dl_remove( pt );
			pt->mode = -1;
			neo_free( pt );
			goto ok;
		}
	}

	neo_errno = E_SKL_INVARG;
	goto err;

ok:;
DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		_it_alarm()
 * 機能        
 *		インタバルタイマの発行
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
static int	
_it_alarm()
{
	_neo_itimer_t		*pt;
	struct timeval		tp;
	struct timezone		tzp;
	long			ssec, susec;

DBG_ENT( M_SKL, "_it_alarm" );

	if( !(pt = (_neo_itimer_t *)_dl_head( &_itimer_entry )) ) {
		goto err;
	}

	/*
	 *	現在時刻の取得
	 */
	gettimeofday(&tp, &tzp);


	/*
	 *	起動時刻の計算
	 */
	ssec  = pt->stime.tv_sec - tp.tv_sec;
	susec = pt->stime.tv_usec - tp.tv_usec;


	/*
	 *	 現在時刻が 起動時刻を 過ぎている時
	 */
	if( ssec < 0 || ( ssec == 0 && susec <= 0 ) ) {
		DBG_A(("next handler time-over(%d.%d)(%d)\n",ssec,susec,time(0)));
		_hndl_itm();

	/*
	 *	 起動時刻が 現在時刻より 遅い時
	 */
	} else {
		if( susec < 0 ) {
			susec += 1000000;
			ssec--;
		}
		DBG_A(("next handler (%d.%d)(%d)\n",ssec,susec,time(0)));
		_it_timer( 0, 0, ssec, susec );
	}


DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		_it_timer()
 * 機能        
 *		setitimer() の発行
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
static int
_it_timer( its, itus, vls, vlus )
long	its;	/* タイマの期限が来たときの再ロードに使用する値(秒)	*/
long	itus;	/* タイマの期限が来たときの再ロードに使用する値(マイクロ秒) */
long	vls;	/* 次のタイマ期限までの時間(秒)		*/
long	vlus;	/* 次のタイマ期限までの時間(マイクロ秒)	*/
{
	struct itimerval	vl, ovl;

DBG_ENT( M_SKL, "_it_timer" );
DBG_C(( "interval_sec(%d) interval_usec(%d)\n", vls, vlus ));

	vl.it_interval.tv_sec   = its;
	vl.it_interval.tv_usec  = itus;
	vl.it_value.tv_sec      = vls;
	vl.it_value.tv_usec     = vlus;

	setitimer( ITIMER_REAL, &vl, &ovl );

DBG_EXIT(0);
	return( 0 );
}
