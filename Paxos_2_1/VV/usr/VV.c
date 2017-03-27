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

#include	"VV.h"
#include	"bs_VV.h"

struct	Log	*pLog;
extern	struct	Log *neo_log;

static	struct	Session	*pRasSession;
static	struct	Session	*pCSSSession;
static	struct	Session	*pCSSEvent;

void			*pDBMgr;

static	int			WorkerMax;
static	pthread_t	*pThWorker;

static	Queue_t	WorkQueue;

VVCtrl_t	VVCtrl;

void VVDlgDestroy( Hash_t *pHash, void *pArg );
void DestroyVS( Hash_t *pHash, void *pArg );
void DestroyCOW( Hash_t *pHash, void *pArg );

typedef struct VSDlgLock {
	uint64_t	l_uDlg;
	VSDlg_t		*l_pDlg;
	int		l_iDlgNum;
	int		l_iDlgLast;
	VSDlg_t		**l_pDlgTab;
} VSDlgLock_t;

int VSDLockInit( VSDlgLock_t *pVSDL, VV_t *pVV, size_t Length, off_t Offset );
int VSDLockTerm( VSDlgLock_t *pVSDL );
int VSDLockR( VSDlgLock_t *pVSDL, VV_t *pVV, uint64_t uVS );
int VSDLockW( VSDlgLock_t *pVSDL, VV_t *pVV, uint64_t uVS );

/* Log */
static FILE	*pFile;

static void VVLogInitChild(void)
{
	if (pLog) {
		LogDestroy(pLog);
		pLog = NULL;
		neo_log = NULL;
	}
}

int
VVLogOpen(void)
{
	int	log_size;
//	pFile = stderr;
	int	log_level	= DEFAULT_LOG_LEVEL;

	if( !pFile ) {
		char *psz_log_pipe = getenv("LOG_PIPE");
		if( psz_log_pipe ) {
			pFile = fopen( psz_log_pipe, "w" );
			if( !pFile )	return( -1 );
		} else {
			pFile = stderr;
		}
	}
	if( !neo_log ) {
		char *psz_log_size = getenv("LOG_SIZE");
		if( psz_log_size ) {
			log_size	= atoi( psz_log_size );
			char *psz_log_level = getenv("LOG_LEVEL");
			if( psz_log_level ) log_level	= atoi( psz_log_level );
			pLog = LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
							pFile, log_level );
			neo_log	= pLog;
			pthread_atfork( NULL, NULL, VVLogInitChild );
			return( 0 );
		} else {
			return( -1 );
		}
	} else {
		pLog	= neo_log;
	}
	return( 0 );
}

void
VVLogClose(void)
{
}

void
VVLogDump(void)
{
	if( pLog ) LogDump( pLog );
}

int
_RasNotify( struct Session *pSession, PFSHead_t *pF )
{
	PFSEventDir_t	*pEvent;
//	PFSRasTag_t	*pRas;

//	pRas	= (PFSRasTag_t*)PaxosSessionGetTag( pSession );
	pEvent	= (PFSEventDir_t*)pF;

LOG( PaxosSessionGetLog(pSession), LogINF, "mod[%d] path[%s] entry[%s]",
pEvent->ed_Mode, pEvent->ed_Name, pEvent->ed_Entry );

	return( 0 );
}

typedef	struct RasArg {
	char	r_Name[PAXOS_CELL_NAME_MAX];
	int		r_Id;
} RasArg_t;

int
RecvPFS_RAS( int ad, PFSRASReq_t *pReqRAS, char *pMyName, int MyId )
{
	PFSRASRpl_t		RplRAS;
	struct Session *pSession;
	int	Ret;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
						Send, Recv, NULL, NULL, pReqRAS->r_Cell );

	PaxosSessionSetLog( pSession, pLog );

	Ret = PFSRasThreadCreate( pMyName, pSession, 
						pReqRAS->r_Path, MyId, _RasNotify);

   	SESSION_MSGINIT( &RplRAS, PFS_RAS, 0, 0, sizeof(RplRAS) );
	RplRAS.r_Head.h_Error	= Ret;
	RplRAS.r_Head.h_Head.h_Cksum64	= 0;
	RplRAS.r_Head.h_Head.h_Cksum64	= 
					in_cksum64( &RplRAS, RplRAS.r_Head.h_Head.h_Len, 0 );
	SendStream( ad, (char*)&RplRAS, sizeof(RplRAS) );

	return( Ret );
}

int
VV_LockR(void)	// Caled from bs_VV.c(request)
{
	RwLockR( &VVCtrl.c_Mtx );
	return( 0 );
}

int
VV_LockW(void)
{
	RwLockW( &VVCtrl.c_Mtx );
	return( 0 );
}

int
VV_Unlock(void)
{
	RwUnlock( &VVCtrl.c_Mtx );
	return( 0 );
}

int
VV_Ctrl(int Fd )
{
	VVCtrl_rpl_t	Rpl;

   	SESSION_MSGINIT( &Rpl, VV_CNTRL, 0, 0, sizeof(Rpl) );
	Rpl.c_Head.h_Error	= 0;
	Rpl.c_Head.h_Cksum64	= 0;

	Rpl.c_Ctrl	= VVCtrl;
	Rpl.c_pCtrl	= &VVCtrl;

	SendStream( Fd, (char*)&Rpl, sizeof(Rpl) );

	return( 0 );
}

int
VV_Dump(int Fd, VVDump_req_t *pReq )
{
	VVDump_rpl_t	Rpl;
	int	Ret;

   	SESSION_MSGINIT( &Rpl, VV_DUMP, 0, 0, sizeof(Rpl)+pReq->d_Len );
	Rpl.d_Head.h_Error	= 0;
	Rpl.d_Head.h_Cksum64	= 0;

	SendStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	Ret = SendStream( Fd, (char*)pReq->d_pAddr, pReq->d_Len );

	return( Ret );
}

void*
ThreadAdm( void *pArg )
{
	RasArg_t		*pRasArg	= pArg;
	int	Ret;
	sigset_t set;
    int s;
static bool_t	bLocked;

	pthread_attr_t      thread_attr;
    size_t              stack_size = 0;
    size_t              guard_size = 0;

    pthread_attr_init( &thread_attr );
    pthread_attr_getstacksize( &thread_attr, &stack_size );
    pthread_attr_getguardsize( &thread_attr, &guard_size );
    LOG( pLog, LogINF, "Default stack size %zd[%zdM]; guard size %zd "
        "minimum is %u[%uK]",
        stack_size, stack_size >> 20, guard_size,
        PTHREAD_STACK_MIN, PTHREAD_STACK_MIN >> 10 );


    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s != 0) {
        LOG( pLog, LogINF, "pthread_sigmask s=%d", s );
		goto err;
	}
	/* 	Admin Port */
	struct sockaddr_un	AdmAddr;
	char	PATH[PAXOS_CELL_NAME_MAX];
	int		Id;
	int	Fd, ad;

	Id	= pRasArg->r_Id;
	sprintf( PATH, PAXOS_SESSION_ADMIN_PORT_FMT, 
				pRasArg->r_Name, pRasArg->r_Id );

	unlink( PATH );

	memset( &AdmAddr, 0, sizeof(AdmAddr) );
	AdmAddr.sun_family	= AF_UNIX;
	strcpy( AdmAddr.sun_path, PATH );

	if( (Fd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err;
	}
	if( bind( Fd, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		goto err1;
	}
	listen( Fd, 1 );

	PaxosSessionHead_t	*pM, M, Rpl, *pReq;

	while( 1 ) {

		errno = 0;

		ad	= accept( Fd, NULL, NULL );
		if( ad < 0 ) {
			LOG( pLog, LogERR, "accept ERROR" );
			continue;
		}

		while( 1 ) {
			Ret = PeekStream( ad, (char*)&M, sizeof(M) );
			if( Ret ) {
				LOG( pLog, LogERR, "peek ERROR", Ret, M.h_Len );
				break;
			}

			pM	= Malloc( M.h_Len );
			ASSERT( pM );

			if( (Ret = RecvStream( ad, (char*)pM, M.h_Len ) ) ) {
				LOG( pLog, LogERR, "recv ERROR Ret=%d h_Len=%d", Ret, M.h_Len );
				Free( pM );
				break;
			}
			switch( (int)pM->h_Cmd ) {

			case VV_LOG_DUMP:
				VV_LockW();

				LogDump( pLog );

   				SESSION_MSGINIT( &Rpl, VV_LOG_DUMP, 0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				VV_Unlock();
				break;

			case VV_ACTIVE:
   				SESSION_MSGINIT( &Rpl, VV_ACTIVE, 0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;

			case VV_STOP:
				errno = 0;
				Ret =	creat( PAXOS_VV_SHUTDOWN_FILE, 0666 );
				if( Ret < 0 ) {
					LOG( pLog, LogERR, "creat error(errno=%d)", errno );
				}
				else
					LOG( pLog, LogERR, "creat(Ret=%d)", Ret );
				write( Ret, "TEST", 4 );
				close( Ret );
				LogDump( pLog );
   				SESSION_MSGINIT( &Rpl, VV_STOP, 0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;


			case VV_RAS_REGIST:
				{VVRasReg_req_t *pReqReg = (VVRasReg_req_t*)pM;
				VVRasReg_rpl_t RplReg;
				char	Path[PATH_NAME_MAX];
				char	Ephemeral[FILE_NAME_MAX];

				if( pRasSession ) {
					Ret	= EEXIST;
					goto regist_skip;
				}
				pRasSession	= VVSessionOpen( pReqReg->r_RasCell, 10, TRUE ); 
				if( !pRasSession ) {
					Ret = ENOENT;
					LOG( pLog, LogERR, "Reg:ras_cell[%s]", pReqReg->r_RasCell );
					goto regist_skip;
				}

				snprintf( Path, sizeof(Path), PAXOS_VV_RAS_FMT,
							PAXOS_VV_ROOT, pRasArg->r_Name );
				sprintf( Ephemeral, "%d", pRasArg->r_Id );

				Ret = PFSRasRegister( pRasSession, Path, Ephemeral );
				LOG( pLog, LogSYS, "Reg[%s] Ret=%d", Ephemeral, Ret );
regist_skip:
   				SESSION_MSGINIT( &RplReg, VV_RAS_REGIST, 0, Ret, 
											sizeof(RplReg) );
				SendStream( ad, (char*)&RplReg, sizeof(RplReg) );

				}
				break;
			case VV_RAS_UNREGIST:
				{VVRasReg_req_t *pReqReg = (VVRasReg_req_t*)pM;
				VVRasReg_rpl_t RplReg;
				char	Path[PATH_NAME_MAX];
				char	Ephemeral[FILE_NAME_MAX];

				if( !pRasSession ) {
					Ret = ENOENT;
					LOG(pLog, LogERR,"Unreg:ras_cell[%s]", pReqReg->r_RasCell);
					goto unregist_skip;
				}
				snprintf( Path, sizeof(Path), PAXOS_VV_RAS_FMT,
						PAXOS_VV_ROOT, pRasArg->r_Name );
				sprintf( Ephemeral, "%d", pRasArg->r_Id );
	
				Ret = PFSRasUnregister( pRasSession, Path, Ephemeral );

				LOG( pLog, LogSYS, "Unreg[%s] Ret=%d", Ephemeral, Ret );
				VVSessionClose( pRasSession );
				pRasSession = NULL;
unregist_skip:
   				SESSION_MSGINIT( &RplReg, VV_RAS_UNREGIST, 0, Ret, 
						sizeof(RplReg) );
				SendStream( ad, (char*)&RplReg, sizeof(RplReg) );
				}
				break;

			case PAXOS_SESSION_ANY:
				pReq	= pM+1;
				switch( (int)pReq->h_Cmd ) {

				case PFS_RAS:
					RecvPFS_RAS( ad, (PFSRASReq_t*)pReq, pRasArg->r_Name, Id );
					break;

				case VV_LOCK:

					VV_LockW();
					bLocked	= TRUE;

   					SESSION_MSGINIT( &Rpl, VV_LOCK, 0, 0, sizeof(Rpl) );
					Rpl.h_Error	= 0;
					Rpl.h_Cksum64	= 0;

					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					break;

				case VV_UNLOCK:
					VV_Unlock();
					bLocked	= FALSE;

   					SESSION_MSGINIT( &Rpl, VV_UNLOCK, 0, 0, sizeof(Rpl) );
					Rpl.h_Error	= 0;
					Rpl.h_Cksum64	= 0;

					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					break;
				case VV_CNTRL:
					VV_Ctrl( ad );
					break;
				case VV_DUMP:
					VV_Dump( ad, (VVDump_req_t*)pReq );
					break;
				default:
					LOG( pLog, LogERR, 
					"Not defined pReq->h_Cmd=0x%x", pReq->h_Cmd );
					break;
				}
				break;

			default:
				LOG( pLog, LogERR, "Not defined pM->h_Cmd=0x%x", pM->h_Cmd );
				break;
			}
			Free( pM );
		}
		if( bLocked ) {
			VV_Unlock();
			bLocked	= FALSE;
		}
		close( ad );
	}
	close( Fd );
	Free( pArg );
	pthread_exit( NULL );
	return( NULL );
err1:
	close( Fd );
err:
	pthread_exit( NULL );
	return( NULL );
}

void*
ThreadWorkerIO( void *pArg )
{
	WorkMsg_t	*pWM = NULL;	
	VV_IO_t	*pVV_IO;
	LS_IO_t	*pLS_IO;
	int	Ret;
	LV_IO_t	*pLV_IO;
	list_t	LnkLV_IO;
	BTCursor_t	Cursor;

	while(1) {

		QueueWaitEntry( &WorkQueue, pWM, wm_Lnk );
LOG( pLog, LogINF, "Ret=%d pWM=%p Cmd=%d", 
	Ret, pWM, (pWM != NULL ? pWM->wm_Cmd : 9 ) ); 
		if( !pWM )	break;

		pVV_IO	= pWM->wm_pVV_IO;
		pLS_IO	= pWM->wm_pLS_IO;

		/* make list */
		list_init( &LnkLV_IO );
		BTCursorOpen( &Cursor, &pLS_IO->l_ReqLV_IO );
		BTCHead( &Cursor );
		while( (pLV_IO = BTCGetKey( LV_IO_t*, &Cursor)) ) {
			list_init( &pLV_IO->io_Lnk );
			list_add_tail( &pLV_IO->io_Lnk, &LnkLV_IO );
			if( BTCNext( &Cursor ) )	break;
		}
		BTCursorClose( &Cursor );

		switch( pWM->wm_Cmd ) {
			case WM_READ:
				Ret = MCReadIO( &pLS_IO->l_LS, &LnkLV_IO );
				break;
			case WM_WRITE:
				Ret = MCWriteIO( &pLS_IO->l_LS, &LnkLV_IO );
				break;
			case WM_COPY:
				Ret = MCCopyIO( pLS_IO );
				break;
			default:
				LOG( pLog, LogERR, "START:Cmd=%d LV[%s] LS=%u", 
					pWM->wm_Cmd, pLS_IO->l_LS.ls_LV, pLS_IO->l_LS.ls_LS );
				goto exit;
		}
		MtxLock( &pVV_IO->io_Mtx );

		LS_IODestroy( pVV_IO, pLS_IO );

		CndSignal( &pVV_IO->io_Cnd );

		MtxUnlock( &pVV_IO->io_Mtx );

		Free( pWM );
	}
exit:
	LOG(pLog, LogSYS, "EXIT(0)");
	pthread_exit( NULL );
	return( NULL );
}

DlgCache_t*
_Vl_alloc( DlgCacheCtrl_t *pDC )
{
	Vol_t *pVl;

	pVl	= Malloc( sizeof(*pVl) );
	ASSERT( pVl );
	memset( pVl, 0, sizeof(*pVl) );

	return( &pVl->vl_Dlg );
}

int
_Vl_free( DlgCacheCtrl_t *pDC, DlgCache_t *pD )
{
	Vol_t		*pVl = container_of( pD, Vol_t, vl_Dlg );

	ASSERT( list_empty( &pD->d_GE.e_Lnk ) );
	ASSERT( pD->d_GE.e_Cnt == 0  );

	Free( pVl );

	return( 0 );
}

int
_Vl_set( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	Vol_t		*pVl = container_of( pD, Vol_t, vl_Dlg );

	strncpy( pVl->vl_DB.db_Name, pD->d_Dlg.d_Name+3, sizeof(pVl->vl_DB.db_Name) );

	LOG(pLog, LogINF, "%p:VVName[%s]", pVl, pVl->vl_DB.db_Name );

	return( 0 );
}

int
_Vl_unset( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	Vol_t	*pVl = container_of( pD, Vol_t, vl_Dlg );

	LOG(pLog, LogINF, "%p:VVName[%s]", pVl, pVl->vl_DB.db_Name );

	return( 0 );
}
int
_Vl_load( DlgCache_t *pD, void *pArg )
{
	Vol_t	*pVl = container_of( pD, Vol_t, vl_Dlg );
	int	Ret;

	LOG(pLog, LogINF, "%p:VVName[%s]", pVl, pVl->vl_DB.db_Name );

	Ret = DBSetVl( pDBMgr, pVl );

	return( Ret );
}

int
_Vl_unload( DlgCache_t *pD, void *pArg )
{
	Vol_t	*pVl = container_of( pD, Vol_t, vl_Dlg );

	LOG(pLog, LogINF, "%p:VVName[%s]", pVl, pVl->vl_DB.db_Name );

	if( pVl->vl_Parents > 0 ) {
		pVl->vl_Parents	= 0;
		Free( pVl->vl_aParent );
		pVl->vl_aParent	= NULL;
	}
	if( pVl->vl_Childs > 0 ) {
		pVl->vl_Childs	= 0;
		Free( pVl->vl_aChild );
		pVl->vl_aChild	= NULL;
	}
	return( 0 );
}

DlgCache_t*
_VV_alloc( DlgCacheCtrl_t *pDC )
{
	VV_t *pVV;

	pVV	= Malloc( sizeof(*pVV) );
	ASSERT( pVV );
	memset( pVV, 0, sizeof(*pVV) );

	return( &pVV->vv_Dlg );
}

int
_VV_free( DlgCacheCtrl_t *pDC, DlgCache_t *pD )
{
	VV_t		*pVV = container_of( pD, VV_t, vv_Dlg );

	LOG(pLog, LogINF, "%p:FREE", pVV );

	ASSERT( list_empty( &pD->d_GE.e_Lnk ) );
	ASSERT( pD->d_GE.e_Cnt == 0  );

	return( 0 );
}

int
_VV_set( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	VV_t		*pVV = container_of( pD, VV_t, vv_Dlg );
	Vol_t		*pVl = (Vol_t*)pD->d_Dlg.d_pTag;

	strncpy( pVV->vv_DB.db_Name, pD->d_Dlg.d_Name, sizeof(pVV->vv_DB.db_Name) );

	if( pVl )	pVV->vv_Stripes	= pVl->vl_DB.db_Stripes;

	LOG(pLog, LogINF, "%p:VVName[%s]", pVV, pVV->vv_DB.db_Name );

	return( 0 );
}

int
_VV_unset( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	VV_t	*pVV = container_of( pD, VV_t, vv_Dlg );

	LOG(pLog, LogINF, "%p:VVName[%s]", pVV, pVV->vv_DB.db_Name );

	return( 0 );
}

int
_VV_load( DlgCache_t *pD, void *pArg )
{
	VV_t	*pVV = container_of( pD, VV_t, vv_Dlg );
	int	Ret;

	Ret = DBSetVV( pDBMgr, pVV );
	if( Ret < 0 ) {
		errno = ENOENT;
		LOG(pLog, LogERR, "NOENT %p:VVName[%s]", pVV, pVV->vv_DB.db_Name );
		goto err;
	}
	Vol_t	*pVl = (Vol_t*)pArg;

	if( pVl )	pVV->vv_Stripes	= pVl->vl_DB.db_Stripes;

	LOG(pLog, LogINF, "Name[%s]WrTBL[%s]COW[%s]RdVV[%s]",
		pVV->vv_DB.db_Name, pVV->vv_DB.db_NameWrTBL, pVV->vv_DB.db_NameCOW, pVV->vv_DB.db_NameRdVV );
	return( 0 );
err:
	return( -1 );
}

int
_VV_unload( DlgCache_t *pD, void *pArg )
{
	VV_t	*pVV = container_of( pD, VV_t, vv_Dlg );

	LOG(pLog, LogINF, "Name[%s]WrTBL[%s]COW[%s]RdVV[%s]",
		pVV->vv_DB.db_Name, pVV->vv_DB.db_NameWrTBL, pVV->vv_DB.db_NameCOW, pVV->vv_DB.db_NameRdVV );

	return( 0 );
}

DlgCache_t*
_VSDlgAlloc( DlgCacheCtrl_t *pCtrl )
{
	VSDlg_t	*pVSDlg;

	pVSDlg	= (VSDlg_t*)Malloc( sizeof(*pVSDlg) );

	memset( pVSDlg, 0, sizeof(*pVSDlg) );

	LOG(pLog, LogINF, "%p", pVSDlg );

	return( &pVSDlg->vd_Dlg );
}

int
_VSDlgFree( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	VSDlg_t	*pVSDlg = container_of(pD, VSDlg_t, vd_Dlg);

	LOG(pLog, LogINF, "%p[%s]", pVSDlg, pVSDlg->vd_Dlg.d_Dlg.d_Name );

	Free( pVSDlg );

	return( 0 );
}

int
_VSDlgSet( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	VSDlg_t	*pVSDlg = container_of(pD, VSDlg_t, vd_Dlg);

	LOG(pLog, LogINF, "%p[%s]", pVSDlg, pVSDlg->vd_Dlg.d_Dlg.d_Name );

	MtxInit( &pVSDlg->vd_Mtx );

	HashListInit( &pVSDlg->vd_HashListVS, PRIMARY_101, 
							HASH_CODE_U64, HASH_CMP_U64, 
							NULL, Malloc, Free, DestroyVS );

	DHashInit( &pVSDlg->vd_DHashCOW, PRIMARY_1013, 200, PRIMARY_1009,
				HASH_CODE_U64, HASH_CMP_U64, NULL, Malloc, Free, NULL );
	return( 0 );
}

int
_VSDlgUnset( DlgCacheCtrl_t *pCtrl, DlgCache_t *pD )
{
	VSDlg_t	*pVSDlg = container_of(pD, VSDlg_t, vd_Dlg);
	
	LOG(pLog, LogINF, "%p[%s]", pVSDlg, pVSDlg->vd_Dlg.d_Dlg.d_Name );

	HashDestroy( &pVSDlg->vd_HashListVS.hl_Hash );
	DHashDestroy( &pVSDlg->vd_DHashCOW );

	return( 0 );
}

int
_VSDlgInit( DlgCache_t *pD, void *pArg )
{
	VSDlg_t *pDlg = container_of( pD, VSDlg_t, vd_Dlg );
	VV_t	*pVV = pDlg->vd_Dlg.d_Dlg.d_pTag;
	uint64_t	uDlg = (uint64_t)(uintptr_t)pArg;
	int	Ret;

	LOG( pLog, LogINF, "d_Name[%s]Own[%d]vv_NameWrTBL[%s]vv_NameCOW[%s]", 
		pDlg->vd_Dlg.d_Dlg.d_Name, pDlg->vd_Dlg.d_Dlg.d_Own,
		pVV->vv_DB.db_NameWrTBL, pVV->vv_DB.db_NameCOW );

	// Set VS into hash
	Ret = DBSetHashVS( pDBMgr, pVV, &pDlg->vd_HashListVS, 
						uDlg*VV_VS_DLG_SIZE, VV_VS_DLG_SIZE );
	if( Ret <0 )	goto err;

	// Set COW into hash
	if( pVV->vv_DB.db_NameCOW[0] != ' ' && pVV->vv_DB.db_NameCOW[0] != 0 ) {
		DBSetHashCOW( pDBMgr, pVV, &pDlg->vd_DHashCOW, 
				DLG2SU(pVV,uDlg), DLG2SU(pVV,1) );
	}
	return( 0 );
err:
	return( -1 );
}

int
_VSDlgAddCOW( VSDlg_t *pDlg, uint64_t iCOW )
{
	uint64_t	*p;

	if( pDlg->vd_sCOW == 0 ) {
		pDlg->vd_sCOW	= 1/*10*/;
		pDlg->vd_pCOW	= (uint64_t*)Malloc(sizeof(uint64_t)*pDlg->vd_sCOW);
	} else if( pDlg->vd_nCOW == pDlg->vd_sCOW ) {
		pDlg->vd_sCOW *= 2;
		p	= (uint64_t*)Malloc(sizeof(uint64_t)*pDlg->vd_sCOW);
		memcpy( p, pDlg->vd_pCOW, sizeof(uint64_t)*pDlg->vd_nCOW );
		Free( pDlg->vd_pCOW );
		pDlg->vd_pCOW	= p;
	}
	pDlg->vd_pCOW[pDlg->vd_nCOW++]	= iCOW;
	return( 0 );
}

int
_VSDlgPutCOW( VSDlg_t *pDlg )
{
	VV_t	*pVV = pDlg->vd_Dlg.d_Dlg.d_pTag;
	int	Ret;

	LOG( pLog, LogINF, "COW[%s] n=%d", pVV->vv_DB.db_NameCOW, pDlg->vd_nCOW );

	if( pDlg->vd_nCOW > 0 ) {
		Ret = DBPutHashCOW( pDBMgr, pVV->vv_DB.db_NameCOW, 
							pDlg->vd_nCOW, pDlg->vd_pCOW );
		if( Ret < 0 )	goto err;

		pDlg->vd_nCOW = 0;
		if( pDlg->vd_pCOW ) {
			Free( pDlg->vd_pCOW );
			pDlg->vd_pCOW 	= NULL;
			pDlg->vd_sCOW	= 0;
		}
	}
	return( 0 );
err:
	return( -1 );
}

int
_VSDlgLoop( DlgCache_t *pD, void *pVoid )
{
	return( 0 );
}

int
_VSDlgTerm( DlgCache_t *pD, void *pVoid )
{
	VSDlg_t *pDlg = container_of( pD, VSDlg_t, vd_Dlg );
	VV_t	*pVV = pDlg->vd_Dlg.d_Dlg.d_pTag;

	LOG( pLog, LogINF,
		"pVV=%p d_Name[%s] %d vv_NameWrTBL[%s] vv_NameCOW[%s] nCOW=%d", 
		pVV, pDlg->vd_Dlg.d_Dlg.d_Name, pDlg->vd_Dlg.d_Dlg.d_Own,
		pVV->vv_DB.db_NameWrTBL, pVV->vv_DB.db_NameCOW, pDlg->vd_nCOW );

	if( pDlg->vd_nCOW > 0 ) {
		_VSDlgPutCOW( pDlg );
	}
	HashListClear( &pDlg->vd_HashListVS );
	DHashClear( &pDlg->vd_DHashCOW );

	return( 0 );
}

void
_VSDlgDestroy( Hash_t *pHash, void *pArg )
{
/***
	VSDlg_t	*pDlg = (VSDlg_t*)pArg;

	PFSDlgEventCancel( &pDlg->vd_Dlg );

	PFSDlgReturn( &pDlg->vd_Dlg, NULL );
***/
}

void
DestroyVS( Hash_t *pHash, void *pArg )
{
	VS_t	*pVS = (VS_t*)pArg;

	list_del( &pVS->vs_Lnk );
	VS_free( pVS );
}

VSDlg_t*
VLockR( VV_t *pVV, uint64_t uVS )
{
	uint64_t	uDlg = VS2DLG( uVS );
	DlgCache_t	*pD;
	VSDlg_t		*pVSDlg;
	char		Name[PATH_NAME_MAX];

	sprintf( Name, "%s/%"PRIu64"", pVV->vv_DB.db_Name, uDlg );

	LOG(pLog, LogINF, "Name[%s] VS=%"PRIu64"", Name, uVS );

	pD = DlgCacheLockR( &VVCtrl.c_VSDlg, Name, TRUE, 
										pVV, (void*)(uintptr_t)uDlg );

	if( pD )	pVSDlg	= container_of( pD, VSDlg_t, vd_Dlg );
	else {
		pVSDlg	= NULL;
		errno = EAGAIN;
	}

	return( pVSDlg );
}

VSDlg_t*
VLockW( VV_t *pVV, uint64_t uVS )
{
	uint64_t	uDlg = VS2DLG( uVS );
	DlgCache_t	*pD;
	VSDlg_t		*pVSDlg;
	char		Name[PATH_NAME_MAX];

	sprintf( Name, "%s/%"PRIu64"", pVV->vv_DB.db_Name, uDlg );

	LOG(pLog, LogINF, "Name[%s] VS=%"PRIu64"", Name, uVS );

	pD = DlgCacheLockW( &VVCtrl.c_VSDlg, Name, TRUE, pVV, (void*)uDlg );

	if( pD )	pVSDlg	= container_of( pD, VSDlg_t, vd_Dlg );
	else {
		pVSDlg	= NULL;
		errno = EAGAIN;
	}
	return( pVSDlg );
}

int
VUnlock( VSDlg_t *pVSDlg )
{
	LOG(pLog, LogINF, "d_Name[%s]", pVSDlg->vd_Dlg.d_Dlg.d_Name );

	DlgCacheUnlock( &pVSDlg->vd_Dlg, FALSE );

	return( 0 );
}

static bool_t	b_VV_init = FALSE;

static pthread_t	ThAdm;

int
VV_init(void)
{
	unlink( PAXOS_VV_SHUTDOWN_FILE );

	VVLogOpen();

	// Global Lock
	RwLockInit( &VVCtrl.c_Mtx );

	// Admin command including RAS
	char *psz_vv_name = getenv(VV_NAME);
	char *psz_vv_id = getenv(VV_ID);
	if( psz_vv_name && psz_vv_id ) {
		RasArg_t	*pArg;

		LOG( pLog, LogINF, "VV_NAME[%s] VV_ID[%s]", 
				psz_vv_name, psz_vv_id);

		pArg	= (RasArg_t*)Malloc( sizeof(*pArg) );
		strncpy( pArg->r_Name, psz_vv_name, sizeof(pArg->r_Name) );
		pArg->r_Id	= atoi( psz_vv_id );		
		pthread_create( &ThAdm, NULL, ThreadAdm, pArg );
	}
	return( 0 );
}

int
VV_exit()
{
	VVLogClose();
	return( 0 );
}

int
VL_init( char *pTarget, int Worker )
{
	int	MCMaxCache	= DEFAULT_MC_MAX;
	int	DCMaxCache	= DEFAULT_DC_MAX; 
	int	RCMaxCache	= DEFAULT_RC_MAX;
	int	RCBlockSize	= RC_BLOCK_SIZE;
	int	Ret;

	int	i;
#define	DLG_CACHE_MAX_TYPE	2

	if( Worker )	WorkerMax	= Worker;

	if( b_VV_init == TRUE ) return( 0 );

	// CSS Mgr
	char *psz_css_cell = getenv(CSS_CELL);
	if( psz_css_cell ) {
		pCSSSession	= VVSessionOpen( psz_css_cell, 2, 0 );
		if( !pCSSSession ) {
			LOG(pLog, LogERR, "NO CSS session[%s]", psz_css_cell );
			goto err1;
		}
		if( !pCSSEvent ) pCSSEvent	= VVSessionOpen( psz_css_cell, 3, 0 );
		if( !pCSSEvent ) {
			LOG(pLog, LogERR, "NO CSS Event session[%s]", psz_css_cell );
			goto err1;
		}
	} else	goto err1;

	// Recall init and start
	Ret = DlgCacheRecallStart( &VVCtrl.c_Recall, pCSSEvent );
	if( Ret < 0 )	goto err1;

	// DB
	char *psz_db_cell = getenv(DB_CELL);
	if( psz_db_cell ) {
		pDBMgr = DBOpen( psz_db_cell, pCSSSession, &VVCtrl.c_Recall );
		if( !pDBMgr ) {
			LOG(pLog, LogERR, "NO DB[%s]", psz_db_cell );
			goto err2;
		}
	} else	goto err2;

	// VS(VDlg) Delegation
	Ret = DlgCacheInit( pCSSSession, 
		&VVCtrl.c_Recall, &VVCtrl.c_VSDlg, DCMaxCache,
		_VSDlgAlloc, _VSDlgFree, _VSDlgSet, _VSDlgUnset, _VSDlgInit, _VSDlgTerm,
		_VSDlgLoop, pLog );
	if( Ret < 0 )	goto err3;

	// VV
	Ret = DlgCacheInit( pCSSSession, &VVCtrl.c_Recall, &VVCtrl.c_VVDlg, 100,
		_VV_alloc, _VV_free, _VV_set, _VV_unset, _VV_load, _VV_unload, NULL,
		pLog );	
	if( Ret < 0 )	goto err4;

	// Vol
	Ret = DlgCacheInit( pCSSSession, &VVCtrl.c_Recall, &VVCtrl.c_VlDlg, 10,
		_Vl_alloc, _Vl_free, _Vl_set, _Vl_unset, _Vl_load, _Vl_unload, NULL,
		pLog );	
	if( Ret < 0 )	goto err5;

	// Read cache
#if 1
	VVCtrl.c_bCache	= FALSE;
#else
	VVCtrl.c_bCache	= TRUE;
#endif
	char *psz_crcm = getenv(CLIENT_READ_CACHE_MAX);
	if( psz_crcm ) {
		RCMaxCache	= atoi(psz_crcm); 
	}
	char *psz_crcs = getenv(CLIENT_READ_CACHE_SIZE);
	if( psz_crcs ) {
		RCBlockSize	= atoi(psz_crcs); 
	}

	if( RCMaxCache && RCBlockSize ) {
		Ret = BCInit( &VVCtrl.c_Cache, pCSSSession, &VVCtrl.c_Recall,
				RCMaxCache, RCBlockSize, TRUE, RC_CHECK_MSEC, 
				Malloc, Free, pLog );
		if( Ret < 0 )	goto err6;
	}
	else {
		VVCtrl.c_bCache	= FALSE;
	}

	// Mirror
	MCInit( MCMaxCache, &VVCtrl.c_MC );

	// Worker thread
	QueueInit( &WorkQueue );

	if( !WorkerMax )	WorkerMax	= WORKER_MAX;
	pThWorker	= (pthread_t*)Malloc( sizeof(pthread_t)*WorkerMax );

	for( i = 0; i < WorkerMax; i++ ) {
		pthread_create( &pThWorker[i], NULL, ThreadWorkerIO, NULL );
	}

	b_VV_init	= TRUE;
	return( 0 );
err6:
	DlgCacheDestroy( &VVCtrl.c_VlDlg );
err5:
	DlgCacheDestroy( &VVCtrl.c_VVDlg );
err4:
	DlgCacheDestroy( &VVCtrl.c_VSDlg );
err3:
	DBClose( pDBMgr );
err2:
	DlgCacheRecallStop( &VVCtrl.c_Recall );
err1:
	if( pCSSEvent )		VVSessionClose( pCSSEvent );
	if( pCSSSession )	VVSessionClose( pCSSSession );
	if( pRasSession )	VVSessionClose( pRasSession );
	pCSSEvent	= NULL;
	pCSSSession	= NULL;
	pRasSession	= NULL;

	return( -1 );
}

int
VL_exit(void)
{
	int	i;

	LOG(pLog, LogSYS, "ENT:" );

	DlgCacheRecallStop( &VVCtrl.c_Recall );

	QueueAbort( &WorkQueue, SIGKILL );
	LOG(pLog,LogINF,"WAIT workers" );
	for( i = 0; i < WorkerMax; i++ ) {
		pthread_join( pThWorker[i], NULL );
	}

	if( pDBMgr )		DBClose( pDBMgr );

	if( pCSSSession )	VVSessionClose( pCSSSession );
//???	if( pCSSEvent )		VVSessionClose( pCSSEvent );
	if( pRasSession )	VVSessionClose( pRasSession );
	pCSSEvent	= NULL;
	pCSSSession	= NULL;
	pRasSession	= NULL;

	return( 0 );
}

VV_t*
VV_getR( char *pName, bool_t bCreate, Vol_t *pVl )
{
	VV_t	*pVV;
	DlgCache_t	*pD;

//	LOG(pLog, LogINF, "pName[%s]->LockR", pName );

	/* set and get configuration */
	pD = DlgCacheLockR( &VVCtrl.c_VVDlg, pName, bCreate, pVl, pVl );
	if( !pD ) {
		errno = ENOENT;
		goto err;
	}
	pVV = container_of( pD, VV_t, vv_Dlg );

	return( pVV );
err:
	return( NULL );
}

VV_t*
VV_getW( char *pName, bool_t bCreate, Vol_t *pVl )
{
	VV_t	*pVV;
	DlgCache_t	*pD;

	LOG(pLog, LogINF, "pName[%s]->LockW", pName );

	/* set and get configuration */
	pD = DlgCacheLockW( &VVCtrl.c_VVDlg, pName, bCreate, pVl, pVl );
	if( !pD ) {
		errno = ENOENT;
		goto err;
	}
	pVV = container_of( pD, VV_t, vv_Dlg );

	return( pVV );
err:
	return( NULL );
}

int
VV_put( VV_t *pVV, bool_t bFree )
{
//	LOG(pLog, LogINF, "vv_Name[VV_%s]->Unlock", pVV->vv_Name );

	DlgCacheUnlock( &pVV->vv_Dlg, bFree );

//if( bFree )
//DlgCacheReturn( &pVV->vv_Dlg, NULL );

	return( 0 );
}

int
VL_open( char *pTarget, char *pVolName, void **pxxc_p, uint64_t *pSize )
{
	Vol_t	*pVl;
	DlgCache_t	*pD;
	pD = VL_LockR( pVolName );
	if( !pD ) {
		LOG(pLog, LogERR, "[%s]", pVolName );
		goto err;
	}
	pVl = container_of( pD, Vol_t, vl_Dlg );

	*pxxc_p	= pVl;
	*pSize	= pVl->vl_DB.db_Size;

	VL_Unlock( pD, FALSE );

	return( 0 );
err:
	return( -1 );
}

int
VL_close( void *pVoid )
{
	return( 0 );
}

DlgCache_t*
VL_LockR( char *pVolName )
{
	DlgCache_t	*pD;
	char	Name[128];

	sprintf( Name, "VL_%s", pVolName );

	pD = DlgCacheLockR( &VVCtrl.c_VlDlg, Name, TRUE, NULL, NULL );

	LOG(pLog, LogINF, "[%s] pD=%p", Name, pD );

	return( pD );
}

DlgCache_t*
VL_LockW( char *pVolName )
{
	DlgCache_t	*pD;
	char	Name[128];

	sprintf( Name, "VL_%s", pVolName );

	pD = DlgCacheLockW( &VVCtrl.c_VlDlg, Name, TRUE, NULL, NULL );
	LOG(pLog, LogINF, "[%s] pD=%p", Name, pD );

	return( pD );
}

int
VL_Unlock( DlgCache_t *pD, bool_t bFree )
{
	LOG(pLog, LogINF, "[%s] pD=%p", pD->d_Dlg.d_Name, pD );

	DlgCacheUnlock( pD, bFree );
	return( 0 );
}

long
LV_IOCompare( void *pA, void *pB )
{
	LV_IO_t	*pLeft	= (LV_IO_t*)pB;
	LV_IO_t	*pRight	= (LV_IO_t*)pA;

	return( pRight->io_Off - pLeft->io_Off );
}

void
LV_IOPrint( void *p )
{
	LV_IO_t	*pReq	= (LV_IO_t*)p;

	printf("[%p:Off=%"PRIu64" Len=%zu pBuf=%p Flag=0x%x]", 
		p, pReq->io_Off, pReq->io_Len, pReq->io_pBuf, pReq->io_Flag );
}

static int
LV_IODestroy(BTree_t *pBTree,void *pArg)
{
	LV_IO_t		*pIO = (LV_IO_t *)pArg;

	if( pIO->io_Flag & LV_IO_FREE ) {
		LOG(pLog, LogINF, "Free(%p)", pIO->io_pBuf);
		Free( pIO->io_pBuf );
	}
	Free(pIO);

	return 0;
}

LS_IO_t*
LS_IOCreate( VV_IO_t* pVVIO, VS_Id_t *pVS_Id, LS_t *pLS )
{
	LS_IO_t	*pLS_IO;

	pLS_IO = (LS_IO_t*)HashListGet( &pVVIO->io_LS_IO, pVS_Id );
	if( !pLS_IO ) {

		pLS_IO	= (LS_IO_t*)Malloc( sizeof(*pLS_IO) );
LOG(pLog, LogDBG, "Malloc pLS_IO:%p", pLS_IO );
		memset( pLS_IO, 0, sizeof(*pLS_IO) );
		list_init( &pLS_IO->l_Lnk );
		pLS_IO->l_LS	= *pLS;
		pLS_IO->l_VS	= *pVS_Id;

		HashListPut( &pVVIO->io_LS_IO, &pLS_IO->l_VS, pLS_IO, l_Lnk );

		BTreeInit( &pLS_IO->l_ReqLV_IO, 10, 
			LV_IOCompare,  LV_IOPrint, Malloc, Free, LV_IODestroy );
	}
	return( pLS_IO );
}

int
LS_IODestroy( VV_IO_t *pVVIO, LS_IO_t *pLS_IO )
{
LOG(pLog, LogDBG, "Free pLS_IO:%p pVVIO=%p", pLS_IO, pVVIO );
	list_del( &pLS_IO->l_Lnk );
	HashListDel( &pVVIO->io_LS_IO, &pLS_IO->l_VS, LS_IO_t*, l_Lnk );
	BTreeDestroy( &pLS_IO->l_ReqLV_IO );
	Free( pLS_IO );
	return( 0 );
}

/*
 *	No overwrap suposed
 *		LSの領域がストライプ内で連続していれば連結する
 *			LS内の接続、バッファの接続
 */
int
LS_IOInsertReqIO( LS_IO_t *pLS_IO, LV_IO_t *pReq )
{
	int	Ret;
	LV_IO_t	*pL, *pR;
	BTCursor_t	Cursor;
	bool_t	bConcat	= FALSE;

	BTCursorOpen( &Cursor, &pLS_IO->l_ReqLV_IO );

	if( pReq->io_Flag & LV_IO_FREE 	)	goto insert;

	if( !BTCFindLower(&Cursor, pReq ) ) {
		pR = BTCGetKey( LV_IO_t*, &Cursor );

		if( !(pR->io_Flag & LV_IO_FREE)
			&& pReq->io_Off + pReq->io_Len == pR->io_Off
			&& pReq->io_pBuf + pReq->io_Len == pR->io_pBuf ) {// right concat

		 	pReq->io_Len	+= pR->io_Len;
			BTDelete( &pLS_IO->l_ReqLV_IO, pR );
		}
	}
	if( !BTCFindUpper( &Cursor, pReq ) ) {
		pL = BTCGetKey( LV_IO_t*, &Cursor );

		if( !(pL->io_Flag & LV_IO_FREE)
			&& pL->io_Off + pL->io_Len == pReq->io_Off 
			&& pL->io_pBuf + pL->io_Len == pReq->io_pBuf ) {// left concat

		 	pL->io_Len	+= pReq->io_Len;

			bConcat = TRUE;
			*pReq = *pL;
		}
	}
insert:
	if( !bConcat ) {
		pR	= (LV_IO_t*)Malloc( sizeof(*pR) );		// insert
		*pR	= *pReq;
		Ret = BTInsert( &pLS_IO->l_ReqLV_IO, pR );
	}
ret:
	BTCursorClose( &Cursor );
	return( Ret );
}

int
LS_IODeleteReqIO( LS_IO_t *pLS_IO, LV_IO_t *pReq )
{
	int	Ret;

	Ret = BTDelete( &pLS_IO->l_ReqLV_IO, pReq );
	return( Ret );
}


/* 
 *	Offset in iUnit
 */
int
VVReadRequestVS( VV_t *pVV, VS_t *pVS, char *pBuff, 
		size_t Length, off_t Offset, int64_t SnapUnit, VV_IO_t *pVVIO )
{
	uint64_t	Off;
	int			Len;
	LS_IO_t	*pLS_IO;
	LV_IO_t	ReqIO;
	off_t		iSI;
	LS_t	*pLS;
	VS_Id_t	VS_Id;

//LOG( pLog, LogDBG, 
//"VV[%s] L=%"PRIu64" Offset=%"PRIu64" COWUnit=%"PRIi64" VS=%"PRIu64"", 
//	pVV->vv_Name, Offset, Length, SnapUnit, pVS->vs_VS );

	memset( &VS_Id, 0, sizeof(VS_Id) );
	strncpy( VS_Id.vs_VV, pVV->vv_DB.db_Name, sizeof(VS_Id.vs_VV) );

	iSI	= B2SI( pVV, Offset );
	Off	= B2UO( Offset );

	Len = (VV_UNIT_SIZE < Off + Length ? VV_UNIT_SIZE - Off : Length); 

ASSERT( 0 < Len );
ASSERT( Len <= VV_UNIT_SIZE );

	MtxLock( &pVVIO->io_Mtx );

	pLS	= &pVS->vs_aLS[iSI];

	VS_Id.vs_VS	= pVS->vs_VS;
	VS_Id.vs_SI	= iSI;

LOG( pLog, LogDBG, "LV[%s] LS=%d Off=%"PRIu64" SI=%d pBuff=%p", 
	pLS->ls_LV, pLS->ls_LS, Off, iSI, pBuff );

	pLS_IO	= LS_IOCreate( pVVIO, &VS_Id, pLS );

	memset( &ReqIO, 0, sizeof(ReqIO) );
	list_init( &ReqIO.io_Lnk );

	ReqIO.io_Flag	= 0;
	ReqIO.io_pBuf	= pBuff;
	ReqIO.io_Len	= Len;
	ReqIO.io_Off	= B2LO( pVV, Offset );	// in LS

	LS_IOInsertReqIO( pLS_IO, &ReqIO );

	MtxUnlock( &pVVIO->io_Mtx );

	return( 0 );
}

typedef struct {
	Vol_t		*s_pVl;
	VV_t		*s_pVV;
	char		*s_pBuf;
	size_t		s_Length;
	off_t		s_Offset;
	uint64_t	s_SnapUnit;
	VV_IO_t		*s_pIO;
	struct VSDlgLock	*s_pVSDL;
} SnapArg_t;

void*
VVRequestUnlock( Msg_t *pMsg )
{
//	VSDlg_t	*pDlg;	

//	pDlg	= MSG_FUNC_ARG( pMsg );	

//	LOG( pLog, "VUnlock:[%s]", pDlg->vd_Dlg.d_Dlg.d_Name );

//	VUnlock( pDlg );

	return( MsgPopFunc(pMsg) );
}

void*
VVReadRequestSnapUnitLatter1( Msg_t *pMsg )
{
	VV_t	*pVV;	
	SnapArg_t	*pArg;

	pArg	= (SnapArg_t*)MsgFirst( pMsg );

//	LOG( pLog, "[%s]", pArg->s_pVV->vv_Name );

	pVV	= MSG_FUNC_ARG( pMsg );	

	VV_put( pArg->s_pVV, FALSE );

	pArg->s_pVV	= pVV;

	return( MsgPopFunc(pMsg) );
}

void*
VVReadRequestSnapUnitFormer( Msg_t *pMsg )
{
	SnapArg_t	*pArg;
	VV_t		*pVV1;
	uint64_t	Off, Len;
	VS_t		*pVS;
	VSDlg_t		*pDlg;
	uint64_t	uVS;
	int	Ret;

	pArg	= (SnapArg_t*)MsgFirst( pMsg );

	uVS	= B2VS( pArg->s_pVV, pArg->s_Offset ); // To get pDlg

	LOG( pLog, LogDBG,
		"VLockR:VV[%s] Offset[%"PRIu64"] COW[%"PRIu64"] VS[%"PRIu64"]", 
			pArg->s_pVV->vv_DB.db_Name, 
			pArg->s_Offset,
			pArg->s_SnapUnit,
			uVS );

	Ret	= VSDLockR( pArg->s_pVSDL, pArg->s_pVV, uVS );
	ASSERT( Ret == 0 );
	pDlg	= pArg->s_pVSDL->l_pDlg;

	MsgPushFunc( pMsg, VVRequestUnlock, (void*)pDlg );

	if( DHashGet( &pDlg->vd_DHashCOW, &pArg->s_SnapUnit ) ) {

		pVS	= HashGet( &pDlg->vd_HashListVS.hl_Hash, &uVS );

		ASSERT( pVS );

		VVReadRequestVS( pArg->s_pVV, 
						pVS, 
						pArg->s_pBuf, 
						pArg->s_Length, 
						pArg->s_Offset, 
						pArg->s_SnapUnit, 
						pArg->s_pIO );

	} else if( pArg->s_pVV->vv_DB.db_NameRdVV[0] 
			&& pArg->s_pVV->vv_DB.db_NameRdVV[0] != ' ' ) {

		pVV1 = VV_getR( pArg->s_pVV->vv_DB.db_NameRdVV, TRUE, pArg->s_pVl );

		MsgPushFunc( pMsg,VVReadRequestSnapUnitLatter1, (void*)pArg->s_pVV );

		pArg->s_pVV	= pVV1;

		return( VVReadRequestSnapUnitFormer );

	} else {

		uVS	= B2VS( pArg->s_pVV, pArg->s_Offset );
		pVS = HashGet( &pDlg->vd_HashListVS.hl_Hash, &uVS );

		if( pVS ) {

			VVReadRequestVS( pArg->s_pVV, 
								pVS, 
								pArg->s_pBuf, 
								pArg->s_Length, pArg->s_Offset, 
								pArg->s_SnapUnit, 
								pArg->s_pIO );
		}else {

		/* Set ZERO */

			Off	= pArg->s_Offset % SNAP_UNIT_SIZE;
			Len = (SNAP_UNIT_SIZE < Off + pArg->s_Length ? 
							SNAP_UNIT_SIZE - Off : pArg->s_Length); 

LOG( pLog, LogDBG, 
"NOT EXISTS VS=%"PRIu64" Offset=%"PRIu64" Len=%"PRIu64""
" pBuf=%p SnapUnit=%"PRIu64" set ZERO", 
uVS, pArg->s_Offset, Len, pArg->s_pBuf, pArg->s_SnapUnit );

			memset( pArg->s_pBuf, 0, Len );
		}
	}
	return( MsgPopFunc(pMsg) );
}

int
_VVReadRequest(Vol_t *pVl, VV_t *pVV, char *pBuff, 
				size_t Length, off_t Offset, VV_IO_t *pVVIO, VSDlgLock_t *pVSDL )
{
	uint64_t	iUnit;
	uint64_t	Off, End, Len, end;
	int	Ret;

	LOG( pLog, LogINF, "### READ [%s] Offset=%jd Length=%zu ###", 
		pVV->vv_DB.db_Name, (intmax_t)Offset, Length );

	Off = Offset; 
	End	= Offset + Length;

	ASSERT( SNAP_UNIT_SIZE <= VV_UNIT_SIZE );

	for( ; Off < End; Off += Len, pBuff += Len ) {
		iUnit	= Off/SNAP_UNIT_SIZE;
		end		= (iUnit+1)*SNAP_UNIT_SIZE;
		Len	= ( End < end ? End - Off : end - Off );

		Msg_t		*pMsg;
		SnapArg_t	*pArg;
		MsgVec_t	Vec;

		pMsg = MsgCreate( 1, Malloc, Free );

		pArg				= MsgMalloc( pMsg, sizeof(*pArg) );
		pArg->s_pVl			= pVl;
		pArg->s_pVV			= pVV;
		pArg->s_pBuf		= pBuff;
		pArg->s_Length		= Len;
		pArg->s_Offset		= Off;
		pArg->s_SnapUnit	= iUnit;
		pArg->s_pIO			= pVVIO;
		pArg->s_pVSDL		= pVSDL;

		MsgVecSet( &Vec, 0, pArg, sizeof(*pArg), sizeof(*pArg) );
		MsgAdd( pMsg, &Vec );

		Ret = MsgEngine( pMsg, VVReadRequestSnapUnitFormer );
		if( Ret < 0 )	goto err;

		MsgFree( pMsg, pArg );

		MsgDestroy( pMsg );
	}
	return( 0 );
err:
	return( -1 );
}

int
VVReadRequest(Vol_t *pVl, VV_t *pVV, char *pBuff, size_t Length, off_t Offset)
{
	VSDlgLock_t	VSDL;
	VV_IO_t		VVIO;
	int	Ret;

	VSDLockInit( &VSDL, pVV, Length, Offset );

	VV_IO_init( &VVIO );

	Ret = _VVReadRequest(pVl, pVV, pBuff, Length, Offset, &VVIO, &VSDL );
	if( Ret < 0 )	goto err;

	Ret = VVKickReqIO( &VVIO, WM_READ ); // Kick and Wait

	VV_IO_destroy( &VVIO );
	VSDLockTerm( &VSDL );
	return( Ret );
err:
	VV_IO_destroy( &VVIO );
	VSDLockTerm( &VSDL );
	return( -1 );
}

int
VVRead(Vol_t *pVl, VV_t *pVV, char *pBuff, size_t Length, off_t Offset)
{
	int	Ret;
	BC_IO_t		BC_IO;
	BC_Req_t	*pReq;
	VV_IO_t		VVIO;
	VSDlgLock_t	VSDL;

	if( VVCtrl.c_bCache ) {	// Read cache

		BC_IO_init( &BC_IO );

		Ret = BCReadIO( &VVCtrl.c_Cache, &BC_IO, 
					pVl->vl_DB.db_Name, Offset, Length, pBuff );

		if( !list_empty( &BC_IO.io_Lnk ) ) {

			VSDLockInit( &VSDL, pVV, Length, Offset );

			VV_IO_init( &VVIO );

			list_for_each_entry( pReq, &BC_IO.io_Lnk, rq_Lnk ) {

				Ret = _VVReadRequest(pVl, pVV, pReq->rq_pData, 
						pReq->rq_Size, pReq->rq_Offset, &VVIO, &VSDL );
			}
			Ret = VVKickReqIO( &VVIO, WM_READ ); // Kick and Wait

			VV_IO_destroy( &VVIO );

			VSDLockTerm( &VSDL );
		}
		Ret = BCReadDone( &VVCtrl.c_Cache, &BC_IO, 
					pVl->vl_DB.db_Name, Offset, Length, pBuff );

		BC_IO_destroy( &BC_IO );
		
	} else {
		Ret = VVReadRequest( pVl, pVV, pBuff, Length, Offset );
	}
	return( Ret );
}

int
VVWriteRequestVS( VV_t *pVV, VS_t *pVS, char *pBuff, 
	size_t Length, off_t Offset, int64_t SnapUnit, VV_IO_t *pVVIO, int Flag )
{
	uint64_t	Off;
	int			Len;
	LS_IO_t	*pLS_IO;
	LV_IO_t	ReqIO;
	int		iSI;
	LS_t	*pLS;
	VS_Id_t	VS_Id;

	memset( &VS_Id, 0, sizeof(VS_Id) );
	strncpy( VS_Id.vs_VV, pVV->vv_DB.db_Name, sizeof(VS_Id.vs_VV) );

	iSI	= B2SI( pVV, Offset );
	Off	= B2UO( Offset );

	Len = (VV_UNIT_SIZE < Off + Length ? VV_UNIT_SIZE - Off : Length); 

ASSERT( 0 < Len );
ASSERT( Len <= VV_UNIT_SIZE );

	MtxLock( &pVVIO->io_Mtx );

	pLS	= &pVS->vs_aLS[iSI];

	VS_Id.vs_VS	= pVS->vs_VS;
	VS_Id.vs_SI	= iSI;
	pLS_IO	= LS_IOCreate( pVVIO, &VS_Id, pLS );

	memset( &ReqIO, 0, sizeof(ReqIO) );
	list_init( &ReqIO.io_Lnk );

	ReqIO.io_Flag	= Flag;
	ReqIO.io_pBuf	= pBuff;
	ReqIO.io_Len	= Len;
	ReqIO.io_Off	= B2LO( pVV, Offset );	// in LS
//	ReqIO.io_Off	= iLU*VV_UNIT_SIZE + Off; // in LS
//	ReqIO.io_Off	+= VV_LS_SIZE*pLS->ls_LS; // in LV

	LS_IOInsertReqIO( pLS_IO, &ReqIO );

	MtxUnlock( &pVVIO->io_Mtx );

	return( 0 );
//err:
//	MtxUnlock( &pVVIO->io_Mtx );
//	return( -1 );
}

int
VSDLockInit( VSDlgLock_t *pVSDL, VV_t *pVV, size_t Length, off_t Offset )
{
	memset( pVSDL, 0, sizeof(*pVSDL) );

	uint64_t uVSStart	= B2VS( pVV, Offset );
	uint64_t uVSEnd		= B2VS( pVV, Offset + Length );
	uint64_t uDlgStart	= VS2DLG( uVSStart );
	uint64_t uDlgEnd	= VS2DLG( uVSEnd );

	pVSDL->l_iDlgNum = ( uDlgEnd - uDlgStart + 1 );
	pVSDL->l_iDlgLast = -1;

	size_t len = sizeof( pVSDL->l_pDlgTab[0] ) * ( pVSDL->l_iDlgNum );
	pVSDL->l_pDlgTab = Malloc(len);
	ASSERT( pVSDL->l_pDlgTab != NULL );
	memset( pVSDL->l_pDlgTab, 0, len );

	return( 0 );
}

int
VSDLockTerm( VSDlgLock_t *pVSDL )
{
	VSDlg_t		*pVSDlg;
	int i;

	for ( i = 0; i <= pVSDL->l_iDlgLast; i++ ) {
		pVSDlg = pVSDL->l_pDlgTab[i];

		LOG( pLog, LogINF, 
		"VUnlock:Dlg[%s] uDlg=%"PRIu64"", pVSDlg->vd_Dlg.d_Dlg.d_Name, pVSDL->l_uDlg );

		VUnlock( pVSDlg );
	}
	Free( pVSDL->l_pDlgTab );

	return( 0 );
}

int
VSDLockW( VSDlgLock_t *pVSDL, VV_t *pVV, uint64_t uVS )
{
	uint64_t	uDlg = VS2DLG( uVS );
	VSDlg_t		*pVSDlg;

	if( (pVSDlg = pVSDL->l_pDlg) ) {

		if( pVSDL->l_uDlg == uDlg ) {

			if( pVSDlg->vd_Dlg.d_Dlg.d_Own == PFS_DLG_W )	return( 0 );

			else if( pVSDlg->vd_Dlg.d_Dlg.d_Own == PFS_DLG_R ) {
				LOG( pLog, LogINF,
				"R-VLockW:VV[%s] Dlg[%"PRIu64"] VS[%"PRIu64"]", 
					pVV->vv_DB.db_Name, uDlg, uVS );

				VUnlock( pVSDlg );
			}
		}
	}
	LOG( pLog, LogINF, "VLockW:VV[%s] Dlg[%"PRIu64"] VS[%"PRIu64"]", 
					pVV->vv_DB.db_Name, uDlg, uVS );

	pVSDlg	= VLockW( pVV, uVS );
	pVSDL->l_pDlg = pVSDlg;
	pVSDL->l_uDlg	= uDlg;
	pVSDL->l_pDlgTab[++pVSDL->l_iDlgLast] = pVSDlg;
	ASSERT( pVSDL->l_iDlgLast < pVSDL->l_iDlgNum );

	return( 0 );
}

int
VSDLockR( VSDlgLock_t *pVSDL, VV_t *pVV, uint64_t uVS )
{
	uint64_t	uDlg = VS2DLG( uVS );
	VSDlg_t		*pVSDlg;

	if( (pVSDlg = pVSDL->l_pDlg) ) {

		if( pVSDL->l_uDlg == uDlg )	return( 0 );
	}
	LOG( pLog, LogINF, "VLockR:VV[%s] Dlg[%"PRIu64"] VS[%"PRIu64"]", 
					pVV->vv_DB.db_Name, uDlg, uVS );

	pVSDlg	= VLockR( pVV, uVS );
	pVSDL->l_pDlg = pVSDlg;
	pVSDL->l_uDlg	= uDlg;
	pVSDL->l_pDlgTab[++pVSDL->l_iDlgLast] = pVSDlg;
	ASSERT( pVSDL->l_iDlgLast < pVSDL->l_iDlgNum );

	return( 0 );
}

int
VVWriteRequestSnapUnit( Vol_t *pVl, VV_t *pVV, char *pBuff, 
	size_t Length, off_t Offset, uint64_t SnapUnit, 
	VV_IO_t *pIO, VSDlgLock_t *pVSDL )
{
	VV_t		*pVV1;
	VS_t		*pVS;
	uint64_t	uVS;
	int64_t		sr, su;
	uint64_t	Off;
	char		*pBUFF = NULL;
	int	Ret;

	ASSERT( Length <= SNAP_UNIT_SIZE );

	uVS	= B2VS( pVV, Offset );

	Ret = VSDLockW( pVSDL, pVV, uVS );
	if( Ret < 0 )	goto err;

	/* Allocate VS */
	pVS = HashListGet( &pVSDL->l_pDlg->vd_HashListVS, &uVS );
	if( !pVS ) {

		pVS	= VS_alloc( pVV, uVS );
		if( !pVS ) {
			goto err1;
		}
		HashListPut( &pVSDL->l_pDlg->vd_HashListVS, 
							&pVS->vs_VS, pVS, vs_Lnk );

		Ret = DBAllocVS( pDBMgr, pVS, pVl->vl_DB.db_Pool );

		if( Ret < 0 ) {
			LOG( pLog, LogERR, 
			"### DBAllocVS ERROR:VS=%d ###", pVS->vs_VS );

			goto err1;
		}
	}

	if( pVV->vv_DB.db_NameRdVV[0] 
		&& pVV->vv_DB.db_NameRdVV[0] != ' ' ) {	// node

LOG( pLog, LogINF, "Node" );
		/* exists in COW */
		if( !DHashGet( &pVSDL->l_pDlg->vd_DHashCOW, &SnapUnit ) ) {

			DHashPut( &pVSDL->l_pDlg->vd_DHashCOW, &SnapUnit, (void*)TRUE );
			_VSDlgAddCOW( pVSDL->l_pDlg, SnapUnit );

LOG( pLog, LogINF, "ADD COW[%"PRIu64"]", SnapUnit );
			/* if  0 < Offset%SNAP_UNIT_SIZE || Length < SNAP_UNIT_SIZE,
			 *	read SNAP_UNIT_SIZE (Copy On write)
			 */   
			sr	= Offset % SNAP_UNIT_SIZE;
			su	= Offset / SNAP_UNIT_SIZE;
			Off	= SNAP_UNIT_SIZE*su;
			if( 0 < sr || Offset + Length < Off + SNAP_UNIT_SIZE ) {

				pBUFF	= Malloc( SNAP_UNIT_SIZE );

				pVV1 = VV_getR( pVV->vv_DB.db_NameRdVV, TRUE, pVl );

				Ret = VVReadRequest( pVl, pVV1, pBUFF, SNAP_UNIT_SIZE, Off );

				VV_put( pVV1, FALSE );
			}
		}
	}
	/* write */
	if( pBUFF ) {	// Copy On Write
		LOG( pLog, LogINF, "CopyOnWrite" );
		ASSERT( Length <= SNAP_UNIT_SIZE );
		memcpy( pBUFF+sr, pBuff, Length );
		VVWriteRequestVS( pVV, pVS, pBUFF, SNAP_UNIT_SIZE, Off, 
						SnapUnit, pIO, LV_IO_FREE );
	} else {
//LOG( pLog, "NO CopyOnWrite" );
		VVWriteRequestVS( pVV, pVS, pBuff, Length, Offset, 
						SnapUnit, pIO, 0 );
	}

	return( 0 );
err1:
err:
	return( -1 );
}

int
VVWriteRequest( Vol_t *pVl, VV_t *pVV, 
			char *pBuff, size_t Length, off_t Offset )
{
	uint64_t	iUnit;
	uint64_t	Off, End, Len, end;
	int	Ret;
	VSDlgLock_t	VSDL;
	VV_IO_t		VVIO;

	LOG( pLog, LogINF, "### WRITE [%s] Offset=%jd Length=%zu ###", 
		pVV->vv_DB.db_Name, (intmax_t)Offset, Length );

	Off = Offset; 
	End	= Offset + Length;

	ASSERT( SNAP_UNIT_SIZE <= VV_UNIT_SIZE );

	VSDLockInit( &VSDL, pVV, Length, Offset );

	VV_IO_init( &VVIO );

	for( ; Off < End; Off += Len, pBuff += Len ) {
		iUnit	= Off/SNAP_UNIT_SIZE;
		end		= (iUnit+1)*SNAP_UNIT_SIZE;
		Len	= ( End < end ? End - Off : end - Off );

		Ret = VVWriteRequestSnapUnit( pVl, pVV, pBuff, Len, Off, iUnit, 
										&VVIO, &VSDL );
		if( Ret < 0 ) {
			LOG( pLog, LogERR, 
			"### VVWriteRequestSnapUnit ERROR: iUnit=%"PRIu64" ###", iUnit ); 
			goto err;
		}
	}
	Ret = VVKickReqIO( &VVIO, WM_WRITE ); // Kick and Wait
	ASSERT( Ret == 0 );

	VV_IO_destroy( &VVIO );

	VSDLockTerm( &VSDL );
	return( 0 );
err:
	DBRollback( pDBMgr );
	VV_IO_destroy( &VVIO );
	VSDLockTerm( &VSDL );
	return( -1 );
}

int
VVWrite(Vol_t *pVl, VV_t *pVV, char *pBuff, size_t Length, off_t Offset)
{
	int	Ret;
	BC_IO_t		BC_IO;
	BC_Req_t	*pReq;
	VV_IO_t		VVIO;
	VSDlgLock_t	VSDL;

	if( VVCtrl.c_bCache ) {	// Read cache

		BC_IO_init( &BC_IO );

		Ret = BCWriteIO( &VVCtrl.c_Cache, &BC_IO, 
					pVl->vl_DB.db_Name, Offset, Length, pBuff );

		if( !list_empty( &BC_IO.io_Lnk ) ) {

			VSDLockInit( &VSDL, pVV, Length, Offset );

			VV_IO_init( &VVIO );

			// Read Both ends
			list_for_each_entry( pReq, &BC_IO.io_Lnk, rq_Lnk ) {

				Ret = _VVReadRequest(pVl, pVV, pReq->rq_pData, 
						pReq->rq_Size, pReq->rq_Offset, &VVIO, &VSDL );
			}
			Ret = VVKickReqIO( &VVIO, WM_READ ); // Kick and Wait

			VV_IO_destroy( &VVIO );

			VSDLockTerm( &VSDL );
		}
		// write through
		Ret = VVWriteRequest( pVl, pVV, pBuff, Length, Offset );

		// write cache
		if( Ret == 0 ) {
			Ret = BCWriteDone( &VVCtrl.c_Cache, &BC_IO, 
					pVl->vl_DB.db_Name, Offset, Length, pBuff );
		}
		BC_IO_destroy( &BC_IO );
		
	} else {
		Ret = VVWriteRequest( pVl, pVV, pBuff, Length, Offset );
	}
	return( Ret );
}

int
VL_read( void *pVoid, char *pBuff, size_t Length, off_t Offset )
{
	int Ret;
	Vol_t	*pVl = (Vol_t*)pVoid;
	VV_t	*pVV;
	DlgCache_t	*pD;

LOG( pLog, LogINF, "<<<<< [%s:%jd-%zd] >>>>>", pVl->vl_DB.db_Name, (intmax_t)Offset, Length );

	pD = VL_LockR( pVl->vl_DB.db_Name );

	pVV = VV_getR( pVl->vl_DB.db_Name, TRUE, pVl );

	Ret = VVRead( pVl, pVV, pBuff, Length, Offset );

	VV_put( pVV, FALSE );

	VL_Unlock( pD, FALSE );

	if( Ret == 0 )	return( Length );
	else {
		LOG( pLog, LogERR, 
		"ERR:<<< [%s:%jd-%zd] >>>>>", pVl->vl_DB.db_Name, (intmax_t)Offset, Length );
		return( -1 );		// set_medium_error
	}
}

int
VL_write( void *pVoid,  char *pBuff, size_t Length, off_t Offset )
{
	Vol_t	*pVl = (Vol_t*)pVoid;
	VV_t	*pVV;
	int Ret;
	DlgCache_t	*pD;

LOG( pLog, LogINF, "<<<<< [%s:%jd-%zd] >>>>>", pVl->vl_DB.db_Name, (intmax_t)Offset, Length );
//	if( pVl->vl_Err )	goto err;

	pD = VL_LockR( pVl->vl_DB.db_Name );

	pVV = VV_getR( pVl->vl_DB.db_Name, TRUE, pVl );

	Ret = VVWrite( pVl, pVV, pBuff, Length, Offset );

	VV_put( pVV, FALSE );

	VL_Unlock( pD, FALSE );

	if( Ret ) goto err1;
	return( Length );
err1:
	pVl->vl_Err	= EIO;
//err:
	errno	= pVl->vl_Err;	
	LOG( pLog, LogERR, 
	"!!! ERR:<<< [%s:%jd-%zd] >>>>>", pVl->vl_DB.db_Name, (intmax_t)Offset, Length );
	return( -1 );		// set_VV_write_error
}

int
LS_IOCode( void *pArg )
{
	VS_Id_t	*pVS_Id = (VS_Id_t*)pArg;
	long	code;

	code 	= HashStrCode( pVS_Id->vs_VV );
	code	+= pVS_Id->vs_VS;
	code	+= pVS_Id->vs_SI;
	return( code );
}

int
LS_IOCmp( void *pA, void *pB )
{
	VS_Id_t	*pVS_IdA = (VS_Id_t*)pA;
	VS_Id_t	*pVS_IdB = (VS_Id_t*)pB;
	int	cmp;

	cmp	= pVS_IdB->vs_SI - pVS_IdA->vs_SI;
	if( cmp )	return( cmp );

	cmp	= pVS_IdB->vs_VS - pVS_IdA->vs_VS;
	if( cmp )	return( cmp );

	cmp	= strncmp(	pVS_IdB->vs_VV, pVS_IdA->vs_VV,
					sizeof(pVS_IdB->vs_VV) );
	return( cmp );
}

int
VV_IO_init( VV_IO_t *pVVIO )
{
	LOG( pLog, LogDBG, "" );

	memset( pVVIO, 0, sizeof(*pVVIO) );

	MtxInit( &pVVIO->io_Mtx );
	CndInit( &pVVIO->io_Cnd );
	HashListInit( &pVVIO->io_LS_IO, PRIMARY_101, 
					LS_IOCode, LS_IOCmp, NULL, Malloc, Free, NULL );
	DHashInit( &pVVIO->io_DHashCOW, PRIMARY_1013, 200, PRIMARY_1009,
				HASH_CODE_U64, HASH_CMP_U64, NULL, Malloc, Free, NULL );
	return( 0 );
}


int
VV_IO_destroy( VV_IO_t *pVVIO )
{
	LOG( pLog, LogDBG, "" );

	MtxDestroy( &pVVIO->io_Mtx );
	CndDestroy( &pVVIO->io_Cnd );
	HashListDestroy( &pVVIO->io_LS_IO );
	DHashDestroy( &pVVIO->io_DHashCOW );
	return( 0 );
}

int
VV_IO_print( VV_IO_t *pVVIO )
{
	LS_IO_t	*pLS_IO;
	void	*p;

	list_for_each_entry( pLS_IO, &pVVIO->io_LS_IO.hl_Lnk, l_Lnk ) {
		p = HashGet( &pVVIO->io_LS_IO.hl_Hash, &pLS_IO->l_VS );
		printf("### [%s,%u]->[%s,%u] [%s,%"PRIu64",%d] %p ###\n", 
			pLS_IO->l_LS.ls_LV, pLS_IO->l_LS.ls_LS, 
			pLS_IO->l_LSTo.ls_LV, pLS_IO->l_LSTo.ls_LS, 
			pLS_IO->l_VS.vs_VV, pLS_IO->l_VS.vs_VS, 
			pLS_IO->l_VS.vs_SI, p );

		BTreePrint( &pLS_IO->l_ReqLV_IO );
	}
	return( 0 );
}

int
VVKickReqIO( VV_IO_t *pVV_IO, WMCmd_t Cmd )
{
	LS_IO_t		*pLS_IO;
	WorkMsg_t	*pWM;
	int			Ret = 0;

	MtxLock( &pVV_IO->io_Mtx );

//VV_IO_print( pVV_IO );
	list_for_each_entry( pLS_IO, &pVV_IO->io_LS_IO.hl_Lnk, l_Lnk ) {

		pWM = (WorkMsg_t*)Malloc( sizeof(*pWM) );
		pWM->wm_Cmd		= Cmd;
		pWM->wm_pVV_IO 	= pVV_IO;
		pWM->wm_pLS_IO	= pLS_IO;

		LOG( pLog, LogINF, "pWM=%p Cmd=%d [%s] LS=%d:[%s] VS=%"PRIu64" SI=%d", 
			pWM, Cmd, pLS_IO->l_LS.ls_LV, pLS_IO->l_LS.ls_LS,
			pLS_IO->l_VS.vs_VV, pLS_IO->l_VS.vs_VS, pLS_IO->l_VS.vs_SI );

		QueuePostEntry( &WorkQueue, pWM, wm_Lnk );
	}
	while( HashCount( &pVV_IO->io_LS_IO.hl_Hash ) ) {
		Ret = CndWait( &pVV_IO->io_Cnd, &pVV_IO->io_Mtx );
	}
	MtxUnlock( &pVV_IO->io_Mtx );
	return( Ret );
}

int
VVKickReqIOCopy( VV_IO_t *pVV_IO, void* pDBMgr, CopyLog_t *pCL )
{
	LS_IO_t		*pLS_IO;
	WorkMsg_t	*pWM;
	int			Ret = 0;
	uint64_t	Total;
	int			n;

	MtxLock( &pVV_IO->io_Mtx );

//VV_IO_print( pVV_IO );
	Total			= HashCount( &pVV_IO->io_LS_IO.hl_Hash );
	pCL->cl_ToLSs	+= Total;
	pCL->cl_End		= time(0);

	Ret = DBCopyLogUpdate( pDBMgr, pCL );
	if( Ret < 0 )	goto err1;
	Ret = DBCopyLogGetBySeq( pDBMgr, pCL );
	if( Ret < 0 )	goto err1;
	if( pCL->cl_Status & COPY_STATUS_ABORT ) {
		LOG( pLog, LogSYS, "ABORT:" );
		goto err1;
	}

	while( Total > 0 ) {
		n = 0;
		list_for_each_entry( pLS_IO, &pVV_IO->io_LS_IO.hl_Lnk, l_Lnk ) {

			pWM = (WorkMsg_t*)Malloc( sizeof(*pWM) );
			pWM->wm_Cmd		= WM_COPY;
			pWM->wm_pVV_IO 	= pVV_IO;
			pWM->wm_pLS_IO	= pLS_IO;

			LOG( pLog, LogINF, 
			"pWM=%p [%s] LS=%d:[%s] VS=%"PRIu64" SI=%d", 
			pWM, pLS_IO->l_LS.ls_LV, pLS_IO->l_LS.ls_LS,
			pLS_IO->l_VS.vs_VV, pLS_IO->l_VS.vs_VS, pLS_IO->l_VS.vs_SI );

			QueuePostEntry( &WorkQueue, pWM, wm_Lnk );

			pCL->cl_ToCopied++;
			Total--;

			if( n++ > (2*WorkerMax > 6 ? 2*WorkerMax:6) )	break;
		}
		while( HashCount( &pVV_IO->io_LS_IO.hl_Hash ) > Total ) {
			Ret = CndWait( &pVV_IO->io_Cnd, &pVV_IO->io_Mtx );
		}
		pCL->cl_End	= time(0);
		Ret = DBCopyLogUpdate( pDBMgr, pCL );
		if( Ret < 0 )	goto err1;
		Ret = DBCopyLogGetBySeq( pDBMgr, pCL );
		if( Ret < 0 )	goto err1;
		if( pCL->cl_Status & COPY_STATUS_ABORT ) {
			LOG( pLog, LogSYS, "ABORT:Stage=%d", pCL->cl_Stage );
			goto err1;
		}
	}
	MtxUnlock( &pVV_IO->io_Mtx );
	return( Ret );
err1:
	MtxUnlock( &pVV_IO->io_Mtx );
	return( -1 );
}

