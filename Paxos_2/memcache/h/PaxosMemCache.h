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

#ifndef	_PaxosMemCache_
#define	_PaxosMemCache_

#include	"NWGadget.h"
#include	"DlgCache.h"

#define	DEFAULT_WORKERS		5	// must be >= 2
#define	DEFAULT_ITEM_MAX	2000
#define	DEFAULT_CKSUM_MSEC	(1000*5)
#define	DEFAULT_EXPTIME_SEC	(60*5)
#define	DEFAULT_LEN_WB		100

#define	MEMCACHED_MAX	20

#define	SESSION_RAS_NO	3

/* Admin Port */
#define	PAXOS_MEMCACHE_ADM_FMT	"/tmp/PAXOS_MEMCACHE_ADM_%d"

/* Key namespace */
#define	PAXOS_MEMCACHE_ROOT			"PaxosMemcache"
#define	PAXOS_MEMCACHE_SPACE_FMT	"%s/%s"			// root, space -> namespace
#define	PAXOS_MEMCACHE_KEY_FMT		"%s/%s"			// namespace, key

#define	PAXOS_MEMCACHE_RAS_FMT		"RAS/%s/%s"		// RAS,root,space 
#define	PAXOS_MEMCACHE_EPHEMERAL_FMT	"RAS/%s/%s/%d"	// RAS,root,space,Id 

#define	PAXOS_MEM_SHUTDOWN_FILE		"SHUTDOWN"

struct	MemItem;
struct	FdEvCon;

/* Connection */
/* Client-Server pair(1:n) */
struct FdEvCon;
typedef struct AcceptMemcached {
	struct sockaddr	am_Accept;
	struct sockaddr am_Memcached;
	list_t			am_Lnk;
	struct FdEvCon	*am_pAcceptFd;
} AcceptMemcached_t;

typedef struct MemCachedCon {
	GElm_t				m_Elm;
	int					m_Fd;
	int					m_FdBin;
	struct sockaddr_in	m_Addr;
	Mtx_t				m_Mtx;
	Cnd_t				m_Cnd;
#define	MEMCACHEDCON_ABORT		0x8000
	int				m_Flags;
} MemCachedCon_t;

/* code of update */
typedef enum RetCode {
	RET_TOO_LARGE		= -8,
	RET_NO_MEM			= -7,
	RET_BAD_VALUE		= -6,
	RET_AUTH_ERROR		= -5,
	RET_ERROR			= -4,
	RET_NOT_STORED		= -3,
	RET_NOENT			= -2,
	RET_EXISTS			= -1,

	RET_OK				= 0,
	RET_STORED			= 1,
	RET_WRITE_THROUGH	= 2,

	RET_DUP				= 3,	// Duprication
	RET_YES				= 4,	// Replication
	RET_NEW				= 5,
	RET_UPDATE			= 6
} RetCode_t;

#define	TOKEN_BUF_SIZE	1024
typedef	struct CmdToken {
	char		*c_pCmd;
	char		*c_pKey;
	char		*c_pFlags;
	char		*c_pExptime;
	char		*c_pBytes;
	char		*c_pNoreply;
	char		*c_pCAS;
	char		*c_pValue;
	char		*c_pSave;
	uint64_t	c_MetaCAS;
	uint64_t	c_Epoch;
	char	c_Buf[TOKEN_BUF_SIZE];
} CmdToken_t;

/* Item cache */
#define	CAS_INC( cas )	( ++(cas) == 0 ? ++(cas) : (cas) )
#define	CAS_DEC( cas )	( --(cas) == 0 ? --(cas) : (cas) )
#define	CAS_CMP( x, y ) ((x) == 0 ? \
			((y) == 0 ? 0 : -1) : \
			((y) == 0 ? 1 : (int64_t)((x)-(y))))

typedef struct MemItemAttr {
	struct sockaddr	a_MemCached;

	uint32_t		a_Flags;
	uint64_t		a_Bytes;
	uint64_t		a_MetaCAS;
	uint64_t		a_CAS;
	uint64_t		a_Epoch;
	timespec_t		a_ExpTime;
	timespec_t		a_ReloadTime;
	timespec_t		a_CksumTime;
	uint64_t		a_Cksum64;
	Msg_t			*a_pData;
	Msg_t			*a_pDiff;
} MemItemAttr_t;

typedef struct MemItemAttrs {
	uint64_t		a_MetaCAS;
	uint64_t		a_Epoch;
	int				a_I;	// current
	int				a_N;	// array size
	MemItemAttr_t	a_aAttr[1];
} MemItemAttrs_t;

typedef enum MetaCmd {
	META_CMD_SET		= 1,
	META_CMD_ADD		= 2,
	META_CMD_REPLACE	= 3,
	META_CMD_CAS		= 4,
	META_CMD_APPEND		= 5,
	META_CMD_PREPEND	= 6,
	META_CMD_OTHERS		= 0x7f
} MetaCmd_t;

#define	MSG_TYPE_MASK	0x00000ff
#define	MSG_RC_MASK		0x000ff00
#define	MSG_CMD_MASK	0x0ff0000	// At now, only storage command
#define	MSG_NOREPLY		0x1000000
#define	MSG_CAS			0x2000000

#define	MSG_TYPE_TEXT	1
#define	MSG_TYPE_BINARY	2

#define	MsgSetText(pMsg) \
	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
	(((uintptr_t)(pMsg)->m_pTag1 & ~MSG_TYPE_MASK) + MSG_TYPE_TEXT)
#define	MsgIsText(pMsg) (((uintptr_t)(pMsg)->m_pTag1 & MSG_TYPE_MASK) \
										== MSG_TYPE_TEXT)
#define	MsgSetBinary(pMsg) \
	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
	(((uintptr_t)(pMsg)->m_pTag1 & ~MSG_TYPE_MASK) + MSG_TYPE_BINARY)
#define	MsgIsBinary(pMsg) (((uintptr_t)(pMsg)->m_pTag1 & MSG_TYPE_MASK) \
										== MSG_TYPE_BINARY)

#define	MsgSetNoreply(pMsg)	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
									((uintptr_t)(pMsg)->m_pTag1 | MSG_NOREPLY)
#define	MsgIsNoreply(pMsg) (((uintptr_t)(pMsg)->m_pTag1 & MSG_NOREPLY)!=0)

#define	MsgSetRC( pMsg, Rpl ) \
	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
	(((uintptr_t)(pMsg)->m_pTag1 & ~MSG_RC_MASK) + ((Rpl)<<8 & MSG_RC_MASK)) 

#define	MsgGetRC( pMsg ) \
	(int8_t)(((uintptr_t)(pMsg)->m_pTag1 & MSG_RC_MASK)>>8 )

#define	MsgSetCAS(pMsg)	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
									((uintptr_t)(pMsg)->m_pTag1 | MSG_CAS)
#define	MsgIsCAS(pMsg) ((uintptr_t)(pMsg)->m_pTag1 & MSG_CAS)

#define	MsgSetCmd( pMsg, Cmd ) \
	(pMsg)->m_pTag1 = (void*)(uintptr_t) \
	(((uintptr_t)(pMsg)->m_pTag1 & ~MSG_CMD_MASK) + ((Cmd)<<16) )

#define	MsgGetCmd( pMsg ) \
	(((int)(uintptr_t)(pMsg)->m_pTag1 & MSG_CMD_MASK)>>16 )

typedef struct MemItem {
	DlgCache_t		i_Dlg;
	Mtx_t			i_Mtx;
	char			*i_pKey;
#define	MEM_ITEM_CAS	0x1
#define	MEM_ITEM_ACIVE	0x2
	int				i_Status;
	char			i_Key[PATH_NAME_MAX];	// original key -> MD5
	MemItemAttrs_t	*i_pAttrs;

#define	i_pData		i_pAttrs->a_aAttr[0].a_pData	
#define	i_Flags		i_pAttrs->a_aAttr[0].a_Flags
#define	i_Bytes		i_pAttrs->a_aAttr[0].a_Bytes
#define	i_CAS		i_pAttrs->a_aAttr[0].a_CAS
#define	i_MetaCAS	i_pAttrs->a_aAttr[0].a_MetaCAS
#define	i_ExpTime	i_pAttrs->a_aAttr[0].a_ExpTime
#define	i_ReloadTime	i_pAttrs->a_aAttr[0].a_ReloadTime
#define	i_CksumTime	i_pAttrs->a_aAttr[0].a_CksumTime
#define	i_Cksum64	i_pAttrs->a_aAttr[0].a_Cksum64

} MemItem_t;


/* Cotrol */
#define	MAX_WORKERS	6

typedef struct MemStatistics {
	uint64_t	s_new;
	uint64_t	s_delete;
	uint64_t	s_update;
	uint64_t	s_duprication;
	uint64_t	s_replication;
	uint64_t	s_refer;
	uint64_t	s_hit;
	uint64_t	s_error;
	uint64_t	s_cas;
	uint64_t	s_load;
	uint64_t	s_write_through;
	uint64_t	s_write_back;
	uint64_t	s_write_back_omitted;
} MemStatistics_t;

typedef struct MemCacheCtrl {
	RwLock_t			mc_RwLock;
	pthread_t			mc_ThreadAdm;
	struct sockaddr_un	mc_AdmAddr;
	DlgCacheRecall_t	mc_Recall;
	DlgCacheCtrl_t		mc_MemItem;
	DlgCacheCtrl_t		mc_MemSpace;
	DlgCacheCtrl_t		mc_SeqMetaCAS;
	Queue_t				mc_Worker;
	pthread_t			mc_WorkerThread[MAX_WORKERS];
	Queue_t				mc_WorkerWB;
	pthread_t			mc_WorkerThreadWB[MAX_WORKERS];
	HashList_t			mc_Pair;
	Mtx_t				mc_Mtx;
	FdEventCtrl_t		mc_FdEvClient;
	bool_t				mc_bWB;
	int					mc_LenWB;
	bool_t				mc_bCksum;
	uint32_t			mc_UsecCksum;
	uint32_t			mc_SecExp;
	int					mc_Id;
	char				mc_Space[PATH_NAME_MAX];
	char				mc_NameSpace[PATH_NAME_MAX];
	char				mc_Ephemeral[PATH_NAME_MAX];
	struct Session		*mc_pSession;
	struct Session		*mc_pEvent;
	GElmCtrl_t			mc_MemCacheds;
	MemStatistics_t		mc_Statistics;

	struct Session		*mc_pSessionRAS;

	Timer_t				mc_Timer;
	Mtx_t				mc_MtxCritical;
	struct {
		char	m_Version[FILE_NAME_MAX];
		size_t	m_item_size_max;
	}		mc_Memcached;
#define	mc_Version			mc_Memcached.m_Version
#define	mc_item_size_max	mc_Memcached.m_item_size_max

} MemCacheCtrl_t;

/*
 *	Protocol
 */
#define	MEM_FAMILY	(7<<16)

#define	MEM_CLOSE					(MEM_FAMILY|0)

#define	MEM_LOCK					(MEM_FAMILY|1)
#define	MEM_UNLOCK					(MEM_FAMILY|2)

#define	MEM_CTRL					(MEM_FAMILY|3)
#define	MEM_DUMP					(MEM_FAMILY|4)

#define	MEM_ADD_ACCEPT_MEMCACHED	(MEM_FAMILY|5)
#define	MEM_DEL_ACCEPT_MEMCACHED	(MEM_FAMILY|6)

#define	MEM_STOP					(MEM_FAMILY|7)

#define	MEM_LOG_DUMP				(MEM_FAMILY|8)

#define	MEM_RAS_REGIST				(MEM_FAMILY|9)
#define	MEM_RAS_UNREGIST			(MEM_FAMILY|10)

#define	MEM_ACTIVE					(MEM_FAMILY|11)

typedef	PaxosSessionHead_t	MemActive_req_t;
typedef	PaxosSessionHead_t	MemActive_rpl_t;

typedef struct MemCacheClose_req {
	PaxosSessionHead_t	c_Head;
} MemCacheClose_req_t;

typedef struct MemCacheClose_rpl {
	PaxosSessionHead_t	c_Head;
} MemCacheClose_rpl_t;

typedef struct MemCacheLock_req {
	PaxosSessionHead_t	l_Head;
} MemCacheLock_req_t;

typedef struct MemCacheLock_rpl {
	PaxosSessionHead_t	l_Head;
} MemCacheLock_rpl_t;

typedef struct MemCacheUnlock_req {
	PaxosSessionHead_t	u_Head;
} MemCacheUnlock_req_t;

typedef struct MemCacheUnlock_rpl {
	PaxosSessionHead_t	u_Head;
} MemCacheUnlock_rpl_t;

typedef struct MemCacheCtrl_req {
	PaxosSessionHead_t	c_Head;
} MemCacheCtrl_req_t;

typedef struct MemCacheCtrl_rpl {
	PaxosSessionHead_t	c_Head;
	MemCacheCtrl_t		*c_pCtrl;
} MemCacheCtrl_rpl_t;

typedef struct MemCacheDump_req {
	PaxosSessionHead_t	d_Head;
	void				*d_pAddr;
	int					d_Len;
} MemCacheDump_req_t;

typedef struct MemCacheDump_rpl {
	PaxosSessionHead_t	d_Head;
	// follow data
} MemCacheDump_rpl_t;

typedef struct MemAddAcceptMemcached_req {
	PaxosSessionHead_t	a_Head;
	AcceptMemcached_t	a_AcceptMemcached;
} MemAddAcceptMemcached_req_t;

typedef struct MemAddAcceptMemcached_rpl {
	PaxosSessionHead_t	a_Head;
} MemAddAcceptMemcached_rpl_t;

typedef struct MemDelAcceptMemcached_req {
	PaxosSessionHead_t	d_Head;
	AcceptMemcached_t	d_AcceptMemcached;
} MemDelAcceptMemcached_req_t;

typedef struct MemDelAcceptMemcached_rpl {
	PaxosSessionHead_t	a_Head;
} MemDelAcceptMemcached_rpl_t;

typedef struct MemStop_req {
	PaxosSessionHead_t	s_Head;
} MemStop_req_t;

typedef struct MemStop_rpl {
	PaxosSessionHead_t	s_Head;
} MemStop_rpl_t;

typedef struct MemLog_req {
	PaxosSessionHead_t	l_Head;
} MemLog_req_t;

typedef struct MemLog_rpl {
	PaxosSessionHead_t	l_Head;
} MemLog_rpl_t;

typedef struct MemRASRegist_req {
	PaxosSessionHead_t	r_Head;
	char				r_CellName[PAXOS_CELL_NAME_MAX];
} MemRASRegist_req_t;

typedef struct MemRASRegist_rpl {
	PaxosSessionHead_t	r_Head;
} MemRASRegist_rpl_t;

typedef struct MemRASUnregist_req {
	PaxosSessionHead_t	u_Head;
	char				u_CellName[PAXOS_CELL_NAME_MAX];
} MemRASUnregist_req_t;

typedef struct MemRASUregist_rpl {
	PaxosSessionHead_t	u_Head;
} MemRASUnregist_rpl_t;

/* text reply */
#define	TXT_RPL_DELETED		"DELETED\r\n"
#define	TXT_RPL_NOT_FOUND	"NOT_FOUND\r\n"

/* Binary */
typedef enum BinMagic {
	BIN_MAGIC_REQ = 0x80,
	BIN_MAGIC_RPL = 0x81,
} BinMagic_t;

typedef enum BinRplStatus {
	BIN_RPL_OK 				= 0x0000,
	BIN_RPL_KEY_NOT_FOUND	= 0x0001,
	BIN_RPL_KEY_EXISTS		= 0x0002,
	BIN_RPL_VALUE_TOO_LARGE	= 0x0003,
	BIN_RPL_INVALID_ARG		= 0x0004,
	BIN_RPL_ITEM_NOT_STORED	= 0x0005,
	BIN_RPL_NON_NUMERIC		= 0x0006,
	BIN_RPL_NOT_MY_VBUCKET	= 0x0007,
	BIN_RPL_AUTH_ERROR		= 0x0008,
	BIN_RPL_AUTH_CONTINUE	= 0x0009,
	BIN_RPL_UNKNOWN_COMMAND	= 0x0081,
	BIN_RPL_OUT_OF_MEMORY	= 0x0082,
	BIN_RPL_NOT_SUPPORTED	= 0x0083,
	BIN_RPL_INTERNAL_ERROR	= 0x0084,
	BIN_RPL_BUSY			= 0x0085,
	BIN_RPL_TMP_FAILURE		= 0x0086,
} BinRplStatus_t;

typedef enum BinCmd {
	BIN_CMD_GET			= 0x00,
	BIN_CMD_SET			= 0x01,
	BIN_CMD_ADD			= 0x02,
	BIN_CMD_REPLACE		= 0x03,
	BIN_CMD_DELETE		= 0x04,
	BIN_CMD_INCR		= 0x05,
	BIN_CMD_DECR		= 0x06,
	BIN_CMD_QUIT		= 0x07,
	BIN_CMD_FLUSH		= 0x08,
	BIN_CMD_GETQ		= 0x09,
	BIN_CMD_NOOP		= 0x0a,
	BIN_CMD_VERSION		= 0x0b,
	BIN_CMD_GETK		= 0x0c,
	BIN_CMD_GETKQ		= 0x0d,
	BIN_CMD_APPEND		= 0x0e,
	BIN_CMD_PREPEND		= 0x0f,
	BIN_CMD_STAT		= 0x10,
	BIN_CMD_SETQ		= 0x11,
	BIN_CMD_ADDQ		= 0x12,
	BIN_CMD_REPLACEQ	= 0x13,
	BIN_CMD_DELETEQ		= 0x14,
	BIN_CMD_INCRQ		= 0x15,
	BIN_CMD_DECRQ		= 0x16,
	BIN_CMD_QUITQ		= 0x17,
	BIN_CMD_FLUSHQ		= 0x18,
	BIN_CMD_APPENDQ		= 0x19,
	BIN_CMD_PREPENDQ	= 0x1a,
	BIN_CMD_VERBOSITY	= 0x1b,
	BIN_CMD_TOUCH		= 0x1c,
	BIN_CMD_GAT			= 0x1d,
	BIN_CMD_GATQ		= 0x1e,

	BIN_CMD_SALS_LIST	= 0x20,
	BIN_CMD_SALS_AUTH	= 0x21,
	BIN_CMD_SALS_STEP	= 0x22,

	BIN_CMD_RGET		= 0x30,
	BIN_CMD_RSET		= 0x31,
	BIN_CMD_RSETQ		= 0x32,
	BIN_CMD_RAPPEND		= 0x33,
	BIN_CMD_RAPPENDQ	= 0x34,
	BIN_CMD_RPREPEND	= 0x35,
	BIN_CMD_RPREPENDQ	= 0x36,
	BIN_CMD_RDELETE		= 0x37,
	BIN_CMD_RDELETEQ	= 0x38,
	BIN_CMD_RINCR		= 0x39,
	BIN_CMD_RINCRQ		= 0x3a,
	BIN_CMD_RDECR		= 0x3b,
	BIN_CMD_RDECRQ		= 0x3c,
	BIN_CMD_SET_VBACKET	= 0x3d,
	BIN_CMD_GET_VBACKET	= 0x3e,
	BIN_CMD_DEL_VBACKET	= 0x3f,
	BIN_CMD_TAP_CONNECT				= 0x40,
	BIN_CMD_TAP_MUTATION			= 0x41,
	BIN_CMD_TAP_DELETE				= 0x42,
	BIN_CMD_TAP_FLUSH				= 0x43,
	BIN_CMD_TAP_OPAQUE				= 0x44,
	BIN_CMD_TAP_VBACKET_SET			= 0x45,
	BIN_CMD_TAP_CHECKPOINT_START	= 0x46,
	BIN_CMD_TAP_CHECKPOINT_END		= 0x47,
} BinCmd_t;

typedef enum {
	BIN_RAW_BYTES	= 0x00,
} BinDataType_t;

typedef struct {
	uint8_t		h_Magic;
	uint8_t		h_Opcode;
	uint16_t	h_KeyLen;
	uint8_t		h_ExtLen;
	uint8_t		h_DataType;
	union {
		uint16_t	req_VbucketId;
		uint16_t	rpl_Status;
	} h_Vbucket_Status;
#define	h_VbucketId	h_Vbucket_Status.req_VbucketId
#define	h_Status	h_Vbucket_Status.rpl_Status
	uint32_t	h_BodyLen;
	uint32_t	h_Opaque;
	uint64_t	h_CAS;
} BinHead_t;

typedef struct {
	BinHead_t	t_Head;
	char		*t_pExt;
	char		*t_pKey;
	char		*t_pValue;
	uint64_t	t_MetaCAS;
	uint64_t	t_Epoch;
	// follow Ext and Key
} BinHeadTag_t;

typedef	struct {
	uint32_t	get_Flags;
} BinExtGet_t;

typedef	struct {
	uint32_t	set_Flags;
	uint32_t	set_Expiration;
} BinExtSet_t;

typedef	struct {
	uint64_t	id_Amount;
	uint64_t	id_InitialValue;
	uint32_t	id_Expiration;
} BinExtIncrDecr_t;

typedef struct {
	uint32_t	fl_Expiration;
} BinExtFlush_t;

typedef struct {
	uint32_t	v_Verbosity;
} BinExtVerbosity_t;

typedef struct {
	uint32_t	t_Expiration;
} BinExtTouch_t;

#define	FD_TYPE_ACCEPT	0
#define	FD_TYPE_CLIENT	1
#define	FD_TYPE_SERVER	2
#define	FD_TYPE_ADM		3
#define	FD_TYPE_CONNECT	4

typedef struct FdEvCon {
	list_t			c_LnkQ;
	Mtx_t			c_Mtx;
	Cnd_t			c_Cnd;

	int				c_CntWB;
	TimerEvent_t	*c_pTimerEvent;

#define	FDEVCON_WORKER		0x1
#define	FDEVCON_WAIT		0x2
#define	FDEVCON_REPLY		0x4
#define	FDEVCON_ACK			0x8
#define	FDEVCON_WB			0x10

#define	FDEVCON_SYNC_END	0x1000
#define	FDEVCON_SYNC		0x2000
#define	FDEVCON_ABORT		0x8000
	int				c_Flags;
	void			*c_pVoid;

	FdEvent_t		c_FdEvent;
	struct sockaddr c_PeerAddr;
	MemCachedCon_t	*c_pCon;
	list_t			c_MsgList;
	Msg_t			*c_pMsg;	// current Msg
	list_t			c_MI;
} FdEvCon_t;

#endif
