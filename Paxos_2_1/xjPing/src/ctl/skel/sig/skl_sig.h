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
 *	skl_sig.h
 *
 *	説明	スケルトン シグナル処理
 *
 ******************************************************************************/
#include	<signal.h>
#include	<sys/wait.h>
#include	<sys/time.h>
#include	<sys/resource.h>
#include	"neo_skl.h"
#include	"neo_system.h"


/*
 *	シグナル処理 管理構造体
 */
typedef struct _neo_signal {
	int	sig;			/* シグナル番号(UNIX)		*/
	int	neo_sig;		/* シグナル番号(NEO)		*/
	int	set_flag;		/* シグナルハンドラ登録フラグ	*/
	void	(*_neo_hndl)();		/* シグナルハンドラ		*/
	int	(*_neo_prhndl)();	/* シグナルハンドラ(前処理)	*/
	int	(*_neo_pthndl)();	/* シグナルハンドラ(後処理)	*/
	void	(*_usr_hndl)();		/* シグナルハンドラ(ユーザ定義)	*/
} _neo_signal_t;


/*
 *	シグナルマスク 管理構造体
 */
typedef struct _neo_sigmask {
	int	neo_sig;		/* シグナル番号(NEO)	*/
	int	mask;			/* シグナルマスク	*/
} _neo_sigmask_t;


extern void	_neo_epilogue();
extern void	_neo_child_remove();
extern void	_neo_isterm_set();
extern void	_hndl_itm();

static void	_hndl_dfl();
static void	_hndl_cld();
static int	_prhndl_trm();
static int	_prhndl_cld();
static int	_pthndl_dfl();
static int	_pthndl_flt();
static _neo_signal_t	*_tbl_search();
static void	_neo_sigmask_init();

void		_neo_init_signal();
void		_neo_sigflag_init();
void		_neo_sigflag_set();
int		_neo_sigflag_get();
