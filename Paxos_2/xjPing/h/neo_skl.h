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
 *	neo_skl.h
 *
 *	説明	スケルトン
 *
 ******************************************************************************/
#ifndef _NEO_SKL_
#define _NEO_SKL_

#include "neo_dlnk.h"
#include "neo_system.h"

/*
 *	ＮＥＯ シグナル
 */
#define	NEO_SIGFAULT	1
#define	NEO_SIGINT	2
#define	NEO_SIGPIPE	3
#define	NEO_SIGTERM	4
#define	NEO_SIGCLD	5
#define	NEO_SIGUSR1	6
#define	NEO_SIGUSR2	7

#define	NEO_SIG_IGN	(void (*)())0


/*
 *	ＮＥＯ インタバルタイマ モード
 */
#define	NEO_ONESHOT	0
#define	NEO_CYCLE	1


/*
 *	ＮＥＯ 排他制御 待ちモード
 */
#define	NEO_NOWAIT	0
#define	NEO_WAIT	1

#define NEO_LOCKDIR	"lock"

/*
 *	ＮＥＯ 排他制御 管理構造体
 */
typedef struct lk_id {
	_dlnk_t	lnk;				/* 双方向リンク	*/
	char	resource[NEO_MAXPATHLEN+NEO_NAME_MAX];	/* ファイル名	*/
	int	fd;				/* ファイルディスクリプタ */
} lk_id_t;

extern	int		_neo_prologue( int *pAc, char *Av[] );
extern	void	_neo_epilogue( int Status );

extern void	neo_exit(int);
extern void	neo_term(void);
extern int	neo_isterm(void);
extern void	(*neo_signal(int,void(*)(int)))(void);
//extern void	*neo_setitimer();
extern lk_id_t	*neo_create_lock(char*);

extern void	*Malloc(size_t);
extern void	*Calloc(size_t);
extern void	Free(void*);

extern	void * 	neo_setitimer( void(*)(void*), void*, long, int );
extern	int		neo_delitimer( void* );
extern	int		neo_sleep( unsigned int );

extern	struct Log	*neo_log;

extern	void	_neo_log_open(char*);
extern	void	_neo_log_close(void);

#ifdef	ZZZ

#define	neo_proc_err( M, SVC, ERR, SYM )		LOG( neo_log, "%s", SYM )
#define	neo_rpc_log_msg( N, M, SVC, CLNT, F, S ) \
								LOG( neo_log, "%s %s %s", N, CLNT, S )

#define	neo_rpc_log_err( M, SVC, CLNT, ERR, SYM )	LOG( neo_log, "%s", SYM )
#define	neo_log_msg( M, fmt, SYM )					LOG( neo_log, fmt, SYM )
#define	neo_proc_msg( N, M, SVC, CLNT, SYM ) \
									LOG( neo_log, "%s %s %s", N, SVC, CLNT )

#define	sm_proc_start( PID, AV )	LOG( neo_log, "START pid=%d", PID ) 
#define	sm_proc_term( PID )			LOG( neo_log, "TERM pid=%d", PID ) 

#else

#define	neo_proc_err( M, SVC, ERR, SYM )	LOG( neo_log, LogERR, ERR, SYM )
#define	neo_rpc_log_msg( N, M, SVC, CLNT, F, S ) \
				LOG( neo_log, LogINF, "%s %s " F, N, CLNT, S )

#define	neo_rpc_log_err( M, SVC, CLNT, ERR, SYM ) \
							LOG( neo_log, LogERR, ERR, SYM )
#define	neo_log_msg( M, fmt, SYM )			LOG( neo_log, LogINF, fmt, SYM )
#define	neo_proc_msg( N, M, SVC, CLNT, SYM ) \
				LOG( neo_log, LogINF, "%s %s %s", N, SVC, CLNT )

#define	sm_proc_start( PID, AV )	LOG( neo_log, LogSYS, "START pid=%d", PID ) 
#define	sm_proc_term( PID )			LOG( neo_log, LogSYS, "TERM pid=%d", PID ) 

#endif
 

#endif  /* _NEO_SKL_ */
