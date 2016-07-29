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

#ifndef	_LV_H_
#define	_LV_H_

#include	"Paxos.h"
#include	"PaxosSession.h"
#include	"FileCache.h"
#include	"Status.h"
#include	<inttypes.h>

extern	struct Log	*pLog;

#define	LV_SESSION_NORMAL	0

#define	LV_NAME_MAX		(128)
#ifdef	ZZZ
#define	LV_ROOT			"."
#else
#define	LV_ROOT			"./LVS"
#endif
/* Snapshot */
#define	LV_SNAP_TAR			"SNAP_LV_TAR"
#define	LV_SNAP_UPD_LIST	"SNAP_LV_UPD"
#define	LV_SNAP_DEL_LIST	"SNAP_LV_DEL"

#ifdef KKK
#define TAR_SIZE_MAX    1000000000LL    /* 1GB */
#else
#define TAR_SIZE_MAX    500000000LL     /* 500M */
#endif

#define	LONG_SIZE( l )		(((l) + sizeof(long) - 1 )/sizeof(long))
#define	LONG_BYTE( l )		(LONG_SIZE(l)*sizeof(long))
#define	LONG_ZERO( p, l )	*((long*)(p) + LONG_SIZE(l) -1 ) = 0

typedef struct LS {
#define	LS_DEFINED		0x1
#define	LS_CELL_DEFINED	0x2
#define	LS_PATH_DEFINED	0x4
	int			ls_Flags;
	char		ls_LV[LV_NAME_MAX];
	uint32_t	ls_LS;
	char		ls_Path[PATH_NAME_MAX];
	char		ls_Cell[PAXOS_CELL_NAME_MAX];
} LS_t;

#define	EqLS( pA, pB ) (((pA)->ls_LS == (pB)->ls_LS) \
  						&& !strncmp((pA)->ls_LV,(pB)->ls_LV,LV_NAME_MAX))
/*
 *	Protocol
 */
#define	LV_FAMILY		(5<<16)

#define	LV_READ				(LV_FAMILY|1)
#define	LV_WRITE			(LV_FAMILY|2)
#define	LV_CACHEOUT			(LV_FAMILY|3)
#define	LV_PREFETCH			(LV_FAMILY|4)
#define	LV_DELETE			(LV_FAMILY|5)

#define	LV_OUTOFBAND		(LV_FAMILY|8)
#define	LV_DATA_RPL			(LV_FAMILY|9)
#define	LV_DATA_REQ			(LV_FAMILY|10)
#define	LV_DATA_APPLY		(LV_FAMILY|11)
#define	LV_COMPOUND_WRITE	(LV_FAMILY|12)
#define	LV_COMPOUND_READ	(LV_FAMILY|13)

#define	LV_CNTRL			(LV_FAMILY|20)
#define	LV_DUMP				(LV_FAMILY|21)

typedef struct LV_write_req {
	PaxosSessionHead_t	w_Head;
	LS_t				w_LS;
	off_t				w_Off;
	size_t				w_Len;
	// follow data
} LV_write_req_t;

typedef struct LV_write_rpl {
	PaxosSessionHead_t	w_Head;
} LV_write_rpl_t;

typedef struct LV_read_req {
	PaxosSessionHead_t	r_Head;
	LS_t				r_LS;
	off_t				r_Off;
	size_t				r_Len;
} LV_read_req_t;

typedef struct LV_read_rpl {
	PaxosSessionHead_t	r_Head;
	size_t				r_Len;
	// follow data
} LV_read_rpl_t;

typedef struct LV_cacheout_req {
	PaxosSessionHead_t	c_Head;
} LV_cacheout_req_t;

typedef struct LV_cacheout_rpl {
	PaxosSessionHead_t	c_Head;
} LV_cacheout_rpl_t;

typedef struct LV_delete_req {
	PaxosSessionHead_t	d_Head;
	LS_t				d_LS;
} LV_delete_req_t;

typedef struct LV_delete_rpl {
	PaxosSessionHead_t	d_Head;
} LV_delete_rpl_t;

typedef struct LVCntrl {
	struct PaxosAcceptor	*c_pAcceptor;
	RwLock_t				c_RwLock;
	int						c_Max;
	int						c_Maximum;
	BlockCacheCtrl_t		c_BlockCache;
	bool_t					c_bCksum;
} LVCntrl_t;

typedef	struct LVSessionAdaptor {
	struct PaxosAccept*	a_pAccept;
} LVSessionAdaptor_t;

typedef struct LVDataReq {
	PaxosSessionHead_t	d_Head;
	int					d_Server;
	uint32_t			d_Epoch;
} LVDataReq_t;

typedef struct LVDataRpl {
	PaxosSessionHead_t	d_Head;
	int					d_From;
} LVDataRpl_t;

typedef struct LVDataApply {
	PaxosSessionHead_t	a_Head;
	int					a_From;
} LVDataApply_t;

typedef struct LVCntrl_req {
	PaxosSessionHead_t	c_Head;
} LVCntrl_req_t;

typedef struct LVCntrl_rpl {
	PaxosSessionHead_t	c_Head;
	LVCntrl_t			c_Cntrl;
	LVCntrl_t			*c_pCntrl;
} LVCntrl_rpl_t;

typedef struct LVDump_req {
	PaxosSessionHead_t	d_Head;
	void				*d_pAddr;
	int					d_Len;
} LVDump_req_t;

typedef struct LVDump_rpl {
	PaxosSessionHead_t	d_Head;
	// follow data
} LVDump_rpl_t;

/*
 *		Compound I/O
 */
typedef struct LV_IO {
	list_t	io_Lnk;
	char	*io_pBuf;
	off_t	io_Off;
	size_t	io_Len;
#define	LV_IO_FREE	0x1
	int		io_Flag;
} LV_IO_t;

extern	int LVCompoundRead(struct Session *pSession, LS_t *pLS, list_t *pEnt);
extern	int LVCompoundWrite(struct Session *pSession, LS_t *pLS, list_t *pEnt);
extern	int LVFlushAll( struct Session *pSession );
extern	int LVDelete( struct Session *pSession, LS_t *pLS );

extern	int LVCompoundCopy( struct Session *pFr, LS_t *pLSFr, list_t *pEnt,
					struct Session *pTo, LS_t *pLSTo  );

#endif	// _LV_H_
