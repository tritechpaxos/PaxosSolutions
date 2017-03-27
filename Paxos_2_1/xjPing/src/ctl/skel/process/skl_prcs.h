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
 *	skl_prcs.h
 *
 *	説明	スケルトン プロセス管理
 *
 ******************************************************************************/
#include	<signal.h>
#include	<ctype.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	"neo_dlnk.h"
#include	"neo_skl.h"
#include	"neo_system.h"

#define         ARR_SIZE(a)     (sizeof(a)/sizeof(a[0]))

/*
 *	プロセス管理構造体
 */
typedef struct _neo_child {
	_dlnk_t	lnk;	/* リンク構造体	*/
	int	pid;	/* プロセスid	*/
} _neo_child_t;


void		_neo_init_prcs();
void		_neo_child_remove();

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
extern	int	_neo_ArgMake(int,char**,char*,int,int);
