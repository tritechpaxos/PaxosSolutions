/******************************************************************************
*
*  xjPingPaxos.h 	--- 
*
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

/*
 *	作成			渡辺典孝
 *	試験		
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

//#define	_LOCK_

#include	"PaxosSession.h"
#include	"PFS.h"
#include	"Status.h"
#include	"../../cache/h/FileCache.h"
#include	"neo_db.h"
#include	"../src/ctl/db/svc/svc_logmsg.h"

#define	XJPING_ADMIN_PORT	"PAXOS_CMDB_ADMIN_PORT"

#define	XJPING_FAMILY		(3<<16)
#define	XJPING_OUTOFBAND	(XJPING_FAMILY|0)
#define	XJPING_UPDATE		(XJPING_FAMILY|1)
#define	XJPING_REFER		(XJPING_FAMILY|2)
#define	XJPING_ADMIN		(XJPING_FAMILY|3)

typedef struct p_Tag {
	struct PaxosAccept*	t_pAccept;
	int					t_Cmd;
	struct Session		*t_pSession;
	list_t				t_Msgs;
	int					t_HoldCount;
	bool_t				t_bHold;
} p_Tag_t;

extern	void*	Connect( struct Session* pSession, int j );
extern	int		Shutdown( void* pVoid, int how );
extern	int		CloseFd( void* pVoid );
extern	int		GetMyAddr( void* pVoid, struct sockaddr_in* pMyAddr );
extern	int		Send( void* pVoid, char* pBuf, size_t Len, int Flags );
extern	int		Recv( void* pVoid, char* pBuf, size_t Len, int Flags );

extern	int	PingAcceptHold( p_id_t *pd );
extern	int	PingAcceptRelease( p_id_t *pd );

typedef struct {
	RwLock_t				c_RwLock;
	struct PaxosAcceptor	*c_pAcceptor;
	BlockCacheCtrl_t		*c_pBlockCache;
	bool_t					c_bCksum;
} PingCNTRL_t;

extern	PingCNTRL_t	CNTRL;

#define	pBCC	(CNTRL.c_pBlockCache)


