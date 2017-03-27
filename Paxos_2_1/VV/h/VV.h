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

#ifndef	_VV_H_
#define	_VV_H_

#include	"PFS.h"
#include	"LV.h"
#include	"DlgCache.h"
#include	"BlockCache.h"
#include	<inttypes.h>

extern	struct	Log *pLog;
extern	struct	Log *neo_log;

#define	PAXOS_VV_SHUTDOWN_FILE	"SHUTDOWN"

#define	PAXOS_VV_ROOT			"VV"
#define	PAXOS_VV_RAS_FMT		"RAS/%s/%s"		// RAS,root,cluster 
#define	PAXOS_VV_EPHEMERAL_FMT	"RAS/%s/%s/%d"	// RAS,root,cluster,Id 

#define	CSS_CELL		"CSS"
#define	DB_CELL			"DB"
#define	VV_NAME			"VV_NAME"
#define	VV_ID			"VV_ID"

#define	CLIENT_READ_CACHE_SIZE	"CRC_SIZE"
#define	CLIENT_READ_CACHE_MAX	"CRC_MAX"

#define	VV_MAX_CACHE	"VV"
#define	MC_MAX_CACHE	"MC"

#define	DEFAULT_VV_MAX	(1000)
#define	DEFAULT_MC_MAX	(100)
#define	DEFAULT_DC_MAX	(1000)
#define	DEFAULT_RC_MAX	(2000)

#define	RC_BLOCK_SIZE	(1024*128)	// 128KiB(recommended)
#define	RC_CHECK_MSEC	(10000L)	// 10sec

#define	SNAP_UNIT_SIZE	(1024)		// 1KiB

#define	VV_UNIT_SIZE	(SNAP_UNIT_SIZE*32)	// 32KiB(32768)	
#define	VV_UNITS		(128)	// 128
#define	VV_LS_SIZE		(VV_UNIT_SIZE*VV_UNITS)	// 4MiB(4194304)
#define	VV_STRIPE_MAX	(8)

#define	vv_vs_size(pVV)	(VV_LS_SIZE*((pVV)->vv_Stripes)) // 8MiB(2stripe)

#define	VV_VS_DLG_SIZE		(16)	// 16VS
#define	VV_B_DLG_SIZE( pVV )	(VV_VS_DLG_SIZE*vv_vs_size(pVV)) 
									//128MiB(2stripes)

#define	B2U( B )		((B)/VV_UNIT_SIZE)	// Unit in VL
#define	B2UO( B )		((B)%VV_UNIT_SIZE)	// Off in Unit
#define	U2VU( U )		((U)%VV_UNITS)		// Unit in VS

#define	U2SI( pVV, VU )	((VU)%(pVV)->vv_Stripes)	// SI
#define	U2LU( pVV, VU )	((VU)/(pVV)->vv_Stripes)	// Unit in LS


#define	B2VS( pVV, B )	((B)/vv_vs_size(pVV))
#define	B2SI( pVV, B )	(B2U(B)%((pVV)->vv_Stripes))
#define	B2VO( pVV, B )	((B)%vv_vs_size(pVV))

#define	B2LU( pVV, B )	(B2VO(pVV,B)/VV_UNIT_SIZE/(pVV)->vv_Stripes)
#define	B2LO( pVV, B )	(B2LU(pVV,B)*VV_UNIT_SIZE+B2UO(B)) // off in LS	

//#define	B2DLG( pVV, B )		((B)/VV_B_DLG_SIZE(pVV))
#define	DLG2B( pVV, D )		((D)*VV_B_DLG_SIZE(pVV))

#define	VS2DLG( VS )	((VS)/VV_VS_DLG_SIZE)
#define	DLG2VS( D )		((D)*VV_VS_DLG_SIZE)

#define	SU2DLG( pVV, U )	(B2DLG(pVV,(U)*SNAP_UNIT_SIZE))
#define	SU2SI( pVV, U )		(B2SI(pVV,(U)*SNAP_UNIT_SIZE))
#define	DLG2SU( pVV, D )	(DLG2B(pVV,(D))/SNAP_UNIT_SIZE)

#define	SU2B( S )		((S)*SNAP_UNIT_SIZE)
#define	SU2VU( S )		((S)*SNAP_UNIT_SIZE/VV_UNIT_SIZE)
#define	SU2VS( pVV, S )	((S)*SNAP_UNIT_SIZE/vv_vs_size(pVV))
#define	SU2LB( pVV, s )	B2LB(pVV,SU2B(s))
#define	VS2SU( pVV, VS)	(vv_vs_size(pVV)*(VS)/SNAP_UNIT_SIZE)

#define	VV_VS_MIRRORS	(PAXOS_SERVER_MAX)		// 3

#define	VV_NAME_MAX		(128)


#define	VV_DB_DLG	"VV_DB_DLG"

//#define	WORKER_MAX_TGTD	 2// 16 -> bs_paxos.h
#define	WORKER_MAX		 4// 4
#define	WORKER_MAX_WB	 2

/*
 *	Mirror cell
 */
#define	MIRROR_CELL_CACHE_MAX	10

typedef	struct MCCtrl {
	GElmCtrl_t	mcc_Ctrl;
} MCCtrl_t;

typedef struct MC {
	GElm_t				mc_GE;
	PaxosSessionCell_t	mc_Cell;
	struct Session		*mc_pSession;
} MC_t;

/*
 *	VV,VS
 */
struct	VV;

typedef struct LSUnit {
	LS_t		u_LS;
	uint32_t	u_Unit;
} LSUnit_t;

typedef struct VS {
	list_t			vs_Lnk;
	struct VV		*vs_pVV;
	uint64_t		vs_VS;	
	LS_t			vs_aLS[VV_STRIPE_MAX];
	Mtx_t			vs_Mtx;
} VS_t;

typedef	struct VVCtrl {
	GElmCtrl_t			c_VV;
	RwLock_t			c_Mtx;
	DlgCacheCtrl_t		c_VlDlg;
	DlgCacheCtrl_t		c_VVDlg;
	DlgCacheCtrl_t		c_VSDlg;
	DlgCacheRecall_t	c_Recall;
	bool_t				c_bCache;
	C_BCtrl_t			c_Cache;
	MCCtrl_t			c_MC;
} VVCtrl_t;

typedef struct VV_DB {
	char		db_Name[VV_NAME_MAX];	
	char		db_NameWrTBL[VV_NAME_MAX];	
	char		db_NameCOW[VV_NAME_MAX];	
	char		db_NameRdVV[VV_NAME_MAX];	
	int64_t		db_LSs;		// only for refer
	int64_t		db_COWs;	// only for refer
} VV_DB_t;

typedef struct VV_DB_List {
	int		l_Cnt;
	VV_DB_t	l_aVV[0];
} VV_DB_List_t;

typedef struct VV {
	DlgCache_t	vv_Dlg;
	Mtx_t		vv_Mtx;
	Cnd_t		vv_Cnd;
	VV_DB_t		vv_DB;
	int			vv_Stripes;	// <= VV_STRIPES
	int			vv_Ref;
	struct VV	*vv_pChild;
} VV_t;

typedef	struct	VN_DB {
	char		db_Name[VV_NAME_MAX];
	char		db_Target[PATH_NAME_MAX];
	char		db_Pool[PATH_NAME_MAX];
	int64_t		db_Size;
	int			db_Stripes;	// <= VV_STRIPES
	int			db_Ref;
	int			db_Ver;
	time_t		db_Time;
} VN_DB_t;

typedef struct VN_DB_List {
	int		l_Cnt;
	VN_DB_t	l_aVN[0];
} VN_DB_List_t;

typedef struct Vol {
	DlgCache_t	vl_Dlg;
	VN_DB_t		vl_DB;
	
	VV_DB_t		vl_VV;
//	int64_t		vl_COWs;
	int			vl_Parents;	// parent
	VV_DB_t		*vl_aParent;
	int			vl_Childs;
	VV_DB_t		*vl_aChild;
	int			vl_Err;
	int			vl_Status;
} Vol_t;

typedef	struct VN_VV_List {
	VN_DB_List_t	*l_pVN_List;
	VV_DB_List_t	*l_pVV_List;
} VN_VV_List_t;

typedef struct LV_DB {
	char		db_Name[VV_NAME_MAX];	
	uint32_t	db_Flag;
	uint32_t	db_Total;
	uint32_t	db_Usable;
	char		db_Pool[VV_NAME_MAX];	
	char		db_Cell[VV_NAME_MAX];	
} LV_DB_t;

typedef struct LV_DB_List {
	int		l_Cnt;
	LV_DB_t	l_aLV[0];
} LV_DB_List_t;

//#define	VV_FLUSH_COW	1

typedef	struct VSDlg {
	DlgCache_t		vd_Dlg;	// vd_Dlg.d_pTag <- VV_t*
	Mtx_t			vd_Mtx;
	HashList_t		vd_HashListVS;
	DHash_t			vd_DHashCOW;
	int				vd_nCOW;
	int				vd_sCOW;
	uint64_t		*vd_pCOW;
} VSDlg_t;

typedef struct VSKey {
	char		vsk_Name[VV_NAME_MAX];
	uint64_t	vsk_VS;
} VSKey_t;
	
extern	VSDlg_t* VLockR( VV_t *pVV, uint64_t uVS );
extern	VSDlg_t* VLockW( VV_t *pVV, uint64_t uVS );
extern	int VUnlock( VSDlg_t *pVSDlg );

typedef struct CopyLog {
	uint64_t	cl_Seq;
	char		cl_Fr[VV_NAME_MAX];	
	char		cl_To[VV_NAME_MAX];	
	char		cl_FrSnap[VV_NAME_MAX];	
	char		cl_ToSnap[VV_NAME_MAX];	
	uint64_t	cl_FrLSs;
	uint64_t	cl_ToLSs;
	uint64_t	cl_ToCopied;
	time_t		cl_Start;
	time_t		cl_End;
	uint32_t	cl_Stage;
	uint32_t	cl_Status;
} CopyLog_t;

extern	int	DBCopyLogSeqGet( void *pD, uint64_t * );
extern	int	DBCopyLogAdd( void *pD, CopyLog_t *pCL );
extern	int	DBCopyLogUpdate( void *pD, CopyLog_t *pCL );
extern	int	DBCopyLogGetBySeq( void *pD, CopyLog_t *pCL );
extern	int DBListCopy( void *pD, int *pN, CopyLog_t **paCL );
extern	int DBCopyAbort( void *pD, uint64_t Seq, CopyLog_t *pCL );

#define	COPY_STAGE_CREATE_SNAP	1
#define	COPY_STAGE_LINK			2
#define	COPY_STAGE_COPY1		3
#define	COPY_STAGE_COPY2		4
#define	COPY_STAGE_UNLINK		5
#define	COPY_STAGE_DELETE_SNAP	6
#define	COPY_STAGE_COMPLETE		7

#define	COPY_STATUS_ABORT		0x1000

extern	int DBCommit( void *pD );
extern	int DBRollback( void *pD );

extern	int DBCreateSnap( void *pD, Vol_t *pVl, char *pSnapName );
extern	int DBRestoreSnap( void *pD, VV_t *pVV, char *pSnapName );
extern	int DBUpdateRdVV( void *pD, char *pName, char *pRdName );
extern	int DBLink( void *pD, char *pName, char *pRdName );
//extern	int _DBLink( void *pD, char *pName, char *pRdName );
extern	int DBCopyCreateSnapLink( void *pD, 
			Vol_t *pVlFr, char *pSnapFr, Vol_t *pVlTo, char *pSnapTo );

extern	int DBDeleteCommitComplete( void *pD, VV_t *pVV );
extern	int DBDeleteCommitBottom( void *pD, VV_t *pVV );
extern	int DBDeleteCommitTop( void *pD, VV_t *pVV );
extern	int DBDeleteCommitMerge( void *pD, VV_t *pVV, VV_t *pVVParent );

extern	int DBCreateVV( void *pDBMgr, char *pVolName, 
			char *pTarget, uint64_t Size, int Stripes, char *pPool );

extern	int	DBSetVl( void *pDBMgr, Vol_t *pVl );
extern	Vol_t*	DBGetVl( void *pDBMgr, char *pVolName );
extern	int	DBSetVV( void *pDBMgr, VV_t *pVV );
extern	int	_DBGetUpVV( void *pDBMgr, VV_t *pVV );

extern	int	DBGetVS( void *pDBMgr, char *pTBL, int64_t iVS, VS_t *pVS );
extern	int DBAllocVS( void *pDBMgr, VS_t *pVS, char *pPool );
extern	int DBFreeVS( void *pDBMgr, VS_t *pVS );

extern	int DBCreateLV( void *pDBMgr, char *pLVName, 
						char *pLVS, char *pPool, int Segs );
extern	int DBDeleteLV( void *pDBMgr, char *pLVName );
extern	int DBGetLV( void *pDBMgr, char *pPool, int N, LS_t *pLS );
extern	int DBIncLV( void *pDBMgr, char *pLVName, int Inc );
extern	int DBDecLV( void *pDBMgr, char *pLVName, int Dec );
extern	void*	DBOpen(char *pDEBCell, struct Session *pS, DlgCacheRecall_t*);
extern	int		DBClose( void *pDBMgr );
extern	int	DBSetHashVS( void *pDBMgr, VV_t *pVV, HashList_t *pHashList,
									uint64_t VSStart, uint64_t Size ); 
extern	int	DBSetHashCOW( void *pDBMgr, VV_t *pVV, DHash_t *pDHash, 
									uint64_t VSStart, uint64_t Size ); 
extern	int	DBPutHashCOW( void *pDBMgr, char *pTable, int n, uint64_t *pUnit );

extern	DlgCache_t* DBLockR( void *pDBMgr, char *pTable );
extern	DlgCache_t* DBLockW( void *pDBMgr, char *pTable );
extern	int DBUnlock( DlgCache_t *pD );

extern	int _DBCreateSnap( void *pD, Vol_t *pVl, char *pSnapName );
extern	int DBDeleteVolume( void *pDBMgr, char *pVolName );
extern	int _DBDeleteTop( void *pDBMgr, char *pVolName );

extern	int DBMoveLV( void *pD, char *pFrLV, char *pToLV );
extern	int DBRenameVolume( void *pD, char *pFr, char *pTo );
extern	int DBResizeVolume( void *pD, char *pVol, char *pSize );

extern	int DBGetSeq( void *pD, char *pName, uint64_t *pUint64 );
extern	int DBGetCopySeq( void *pD, uint64_t *pUint64 );

extern	VV_t* VV_getR( char *pName, bool_t bCreate, Vol_t *pVl );
extern	VV_t* VV_getW( char *pName, bool_t bCreate, Vol_t *pVl );
extern	int VV_put( VV_t *pVV, bool_t bFree );

extern	int VV_init( void );	// also defined in bs_vv.h
extern	int VV_exit( void );	// also defined in bs_vv.h

extern	int VL_init( char *pTarget, int Worker );	// also defined in bs_vv.h
extern	int VL_exit(void);

extern	int VL_open( char *pTarget, char *pVolName, 
							void **pxxc_p, uint64_t *pSize );
extern	int	VL_close( void *pVoid );
extern	DlgCache_t* VL_LockR( char *pVolName );
extern	DlgCache_t* VL_LockW( char *pVolName );
extern	int VL_Unlock( DlgCache_t *pD, bool_t bFree );

extern	VS_t*	VS_alloc( VV_t *pVV, uint64_t iVS );
extern	void	VS_free( VS_t* );

struct Session* VVSessionOpen( char *CellName, int SessionNo, int IsSync );
void	VVSessionClose( void * pSession );
void	VVSessionAbort( void * pSession );

int		VVLogOpen(void);
void	VVLogClose(void);
void	VVLogDump(void);

typedef	struct LVWBQ {
	list_t		q_Lnk;
	LSUnit_t	q_LSU;
	struct timeval q_Timeval;
} LVWBQ_t;

typedef struct Worker {
	list_t		w_Lnk;
	pthread_t	w_th;
} Worker_t;

typedef	struct VVRange {
	uint64_t	r_Off;
	uint64_t	r_Len;
} VVRange_t;

typedef	struct VS_Id {
	char		vs_VV[VV_NAME_MAX];
	uint64_t	vs_VS;
	int			vs_SI;
} VS_Id_t;

typedef struct LS_IO {
	list_t	l_Lnk;
	VS_Id_t	l_VS;
	LS_t	l_LS;
	LS_t	l_LSTo;
	BTree_t	l_ReqLV_IO;
} LS_IO_t;

typedef struct VV_IO {
	Mtx_t		io_Mtx;
	Cnd_t		io_Cnd;
	HashList_t	io_LS_IO;
	DHash_t		io_DHashCOW;	// for admin
} VV_IO_t;

typedef enum {
	WM_READ,
	WM_WRITE,
	WM_COPY,
} WMCmd_t;
	
typedef struct WorkMsg {
	list_t	wm_Lnk;
	WMCmd_t	wm_Cmd;
	VV_IO_t	*wm_pVV_IO;
	LS_IO_t	*wm_pLS_IO;
} WorkMsg_t;

extern	LS_IO_t* LS_IOCreate(VV_IO_t* pVVIO, VS_Id_t *pVS, LS_t *pLS);
extern	int LS_IODestroy( VV_IO_t *pVVIO, LS_IO_t *pLS_IO );
extern	int LS_IOInsertReqIO( LS_IO_t *pLS_IO, LV_IO_t *pReq );
extern	int LS_IODeleteReqIO( LS_IO_t *pLS_IO, LV_IO_t *pReq );

extern	int	VV_IO_init( VV_IO_t* );
extern	int	VV_IO_destroy( VV_IO_t* );
extern	int	VV_IO_print( VV_IO_t* );

extern	int	VVKickReqIO( VV_IO_t*, WMCmd_t );
extern	int	VVKickReqIOCopy( VV_IO_t*, void *pD, CopyLog_t *pCL );

extern	int MCReadIO( LS_t *pLS, list_t *pEnt );
extern	int MCWriteIO( LS_t *pLS, list_t *pEnt );
extern	int MCCopyIO( LS_IO_t *pLS );
extern	int	MCFlushAll( char *pCell );
extern	int MCDelete( LS_t *pLS );
extern	int	MCInit( int MCMax, MCCtrl_t *pMCCtrl );

extern	int DBCopyFr( void *pD, VV_t *pVV, VV_IO_t *pCopy );
extern	int DBCopyTo( void *pD, Vol_t *pVl, VV_t *pVV, VV_IO_t *pCopy );

/*
 *	Protocol
 */
#define	VV_FAMILY	(6<<16)

#define	VV_CNTRL		(VV_FAMILY|0)
#define	VV_DUMP			(VV_FAMILY|1)

#define	VV_LOCK			(VV_FAMILY|2)
#define	VV_UNLOCK		(VV_FAMILY|3)

#define	VV_RAS_REGIST	(VV_FAMILY|4)
#define	VV_RAS_UNREGIST	(VV_FAMILY|5)

#define	VV_ACTIVE		(VV_FAMILY|6)
#define	VV_STOP			(VV_FAMILY|7)

#define	VV_LOG_DUMP		(VV_FAMILY|8)

typedef	PaxosSessionHead_t	VVActive_req_t;
typedef	PaxosSessionHead_t	VVActive_rpl_t;

typedef	PaxosSessionHead_t	VVStop_req_t;
typedef	PaxosSessionHead_t	VVStop_rpl_t;

typedef struct VVRasReg_req {
	PaxosSessionHead_t	r_Head;
	char				r_RasCell[PAXOS_CELL_NAME_MAX];
} VVRasReg_req_t;
typedef	PaxosSessionHead_t	VVRasReg_rpl_t;

typedef	PaxosSessionHead_t	VVLogDump_req_t;
typedef	PaxosSessionHead_t	VVLogDump_rpl_t;

typedef struct VVCtrl_req {
	PaxosSessionHead_t	c_Head;
} VVCtrl_req_t;

typedef struct VVCtrl_rpl {
	PaxosSessionHead_t	c_Head;
	VVCtrl_t			c_Ctrl;
	VVCtrl_t			*c_pCtrl;
} VVCtrl_rpl_t;

typedef struct VVDump_req {
	PaxosSessionHead_t	d_Head;
	void				*d_pAddr;
	int					d_Len;
} VVDump_req_t;

typedef struct VVDump_rpl {
	PaxosSessionHead_t	d_Head;
	// follow data
} VVDump_rpl_t;

#endif	// _VV_H_
