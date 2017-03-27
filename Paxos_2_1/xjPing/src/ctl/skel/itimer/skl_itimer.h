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
 *	skl_itimer.h
 *
 *	説明	スケルトン インタバルタイマ
 *
 ******************************************************************************/
#include	<signal.h>
#include	<sys/time.h>
#include	"neo_dlnk.h"
#include	"neo_skl.h"
#include	"neo_system.h"


/*
 *	インタバルタイマ管理構造体
 */
typedef struct _neo_itimer {
	_dlnk_t		lnk;		/* 双方向リンク			*/
	void		*id;		/* インタバルタイマ番号		*/
	int		mode;		/* 起動モード			*/
	struct timeval	itime;		/* インタバル値(/100msec)	*/
	struct timeval	stime;		/* 実行開始時刻			*/
	void		(*func)();	/* ユーザ登録関数		*/
	void		*arg;		/* ユーザ登録関数実行時の引数	*/
} _neo_itimer_t;


extern void	_neo_sigflag_init();
extern void	_neo_sigflag_set();
extern int	_neo_sigflag_get();

static int	_it_alarm();
static int	_it_delist();
static int	_it_entlist();
static int	_it_timer();
static void	_skl_sleep();

void		_hndl_itm();
void		_neo_init_itimer();

extern	int	sigblock(int);
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
extern	int	sigmask(int);
extern	int	sigsetmask(int);
