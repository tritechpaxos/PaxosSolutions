/******************************************************************************
*
*  PFSServer.c 	--- server part of PFS Library
*
*  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
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
 *	試験			塩尻常春、久保正業
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

//#define	_LOCK_

#define	_PFS_SERVER_
#define	_PAXOS_SESSION_SERVER_	// for DEBUG
#include	"PFS.h"
#include	<sys/wait.h>
#include	"Status.h"

/*
 *	Paxos File System
 */

int	MyId;
Timer_t	Timer;

char	*pPFS_Tar_Bin = "/bin/tar"; // Default: LINUX Machine.

PFSControl_t	CNTRL;

#define	PFSMalloc( s )	(PaxosAcceptorGetfMalloc(CNTRL.c_pAcceptor))( s )
#define	PFSFree( p )	(PaxosAcceptorGetfFree(CNTRL.c_pAcceptor))( p )

#define	JOINT_ALLOC( pA, pAdaptorLink, Type, pEventLink, fGarbage, pArg ) \
	({PFSJoint_t* _p = (PFSJoint_t*)PFSMalloc(sizeof(PFSJoint_t)); \
		if(_p) {JOINT_INIT(&_p->j_Head); \
				JOINT( pAdaptorLink, pEventLink, &_p->j_Head); \
				_p->j_Type = Type; _p->j_pAdaptor = pA; \
				_p->j_fGarbage = fGarbage;_p->j_pArg = pArg;} \
	 _p;})

#define	JOINT_FREE( _p ) \
	({int (*fGarbage)(void*); void* pArg; int Ret = 0; \
	fGarbage = (_p)->j_fGarbage; pArg = (_p)->j_pArg; \
	UNJOINT(&(_p)->j_Head); \
	PFSFree( _p ); \
	if( fGarbage )	Ret = (fGarbage)(pArg);Ret;})

#define	PATH_GET( pName ) \
	({PFSPath_t* _p; \
	_p = (PFSPath_t*)DHashGet( &CNTRL.c_Path.hl_Hash, RootOmitted(pName) ); \
	_p;})

#define	PATH_ALLOC( pName ) \
	({PFSPath_t* _p = (PFSPath_t*)PFSMalloc(sizeof(PFSPath_t)); \
DBG_TRC("PATH_ALLOC[%s]\n", pName ); \
		if( _p ) {	memset(_p,0,sizeof(*_p)); \
			list_init(&_p->p_Lnk); list_init(&_p->p_AdaptorEvent); \
			strncpy(_p->p_Name,RootOmitted(pName),sizeof(_p->p_Name) ); \
			DHashListPut(&CNTRL.c_Path,_p->p_Name,_p,p_Lnk); } _p;})

#define	PATH_FREE( pName ) \
	({PFSPath_t* _p; \
DBG_TRC("PATH_FREE[%s]\n", pName ); \
	_p = (PFSPath_t*)DHashListDel(&CNTRL.c_Path, RootOmitted(pName), \
		PFSPath_t*, p_Lnk ); if( _p ) PFSFree(_p);_p;})

#define	EPHEMERAL_GET( pA, pName )	\
	({Ephemeral_t* _p; \
	_p = (Ephemeral_t*)HashListGet(&(pA)->a_Ephemeral,RootOmitted(pName)); \
	_p;})

#define	EPHEMERAL_ALLOC( pA, pName ) \
	({Ephemeral_t* _p; \
		_p = (Ephemeral_t*)PFSMalloc(sizeof(Ephemeral_t)); \
		if( _p ) { \
			list_init(&_p->e_Lnk); \
			strncpy(_p->e_Name,RootOmitted(pName),sizeof(_p->e_Name) ); \
			HashListPut(&(pA)->a_Ephemeral, _p->e_Name, _p, e_Lnk );} _p;})

#define	EPHEMERAL_FREE( pA, pName ) \
	({Ephemeral_t* _p; \
	_p = (Ephemeral_t*)HashListDel(&(pA)->a_Ephemeral, RootOmitted(pName), \
			Ephemeral_t*, e_Lnk ); if( _p ) PFSFree(_p);_p;})



int pfsUnlock( struct PaxosAccept *pAccept, 
				PaxosClientId_t *pId, PFSJoint_t* pJoint, bool_t bAbort );

int
HoldHashCode( void* pArg )
{
	int	index;

	PaxosClientId_t*	pId = (PaxosClientId_t*)pArg;

	index	= (int)( pId->i_Addr.sin_addr.s_addr + pId->i_Pid );

	return( index );
}

int
HoldHashCmp( void* pA, void* pB )
{
	PaxosClientId_t	*pAId = (PaxosClientId_t*)pA;
	PaxosClientId_t	*pBId = (PaxosClientId_t*)pB;

	return( !(pAId->i_Addr.sin_addr.s_addr == pBId->i_Addr.sin_addr.s_addr
				&& pAId->i_Pid == pBId->i_Pid) ); 
}

#define	LOCK_ALLOC( name ) \
	({PFSLock_t* pLock	= (PFSLock_t*)PFSMalloc( sizeof(*pLock) ); \
	memset( pLock, 0, sizeof(*pLock) ); \
	list_init( &pLock->l_Lnk ); \
	list_init( &pLock->l_AdaptorWait ); \
	list_init( &pLock->l_AdaptorHold ); \
	list_init( &pLock->l_AdaptorEvent ); \
	pLock->l_Cnt = 0; pLock->l_Ver = 0/*1*/;\
	strncpy( pLock->l_Name, name, sizeof(pLock->l_Name) ); \
	HashInit( &pLock->l_HoldHash, PRIMARY_11, HoldHashCode, HoldHashCmp, NULL, \
		(void*(*)(size_t))PaxosAcceptorGetfMalloc( CNTRL.c_pAcceptor ), \
		(void(*)(void*))PaxosAcceptorGetfFree( CNTRL.c_pAcceptor ), \
		NULL ); \
	DHashListPut(&CNTRL.c_Lock,(void*)pLock->l_Name, pLock, l_Lnk ); \
	pLock;})

#define	LOCK_FREE( pLock ) \
	({DHashListDel( &CNTRL.c_Lock, (void*)pLock->l_Name, PFSLock_t*, l_Lnk ); \
	HashDestroy( &pLock->l_HoldHash ); \
	PFSFree( pLock );})

#define	QUEUE_ALLOC( name ) \
	({PFSQueue_t* pQueue	= (PFSQueue_t*)PFSMalloc( sizeof(*pQueue) ); \
	memset( pQueue, 0, sizeof(*pQueue) ); \
	list_init( &pQueue->q_Lnk ); \
	list_init( &pQueue->q_AdaptorMsg ); \
	list_init( &pQueue->q_AdaptorWait ); \
	list_init( &pQueue->q_AdaptorEvent ); \
	strncpy( pQueue->q_Name, (name), sizeof(pQueue->q_Name) ); \
	DHashListPut(&CNTRL.c_Queue,(void*)pQueue->q_Name,pQueue,q_Lnk ); \
	pQueue;})

#define	QUEUE_FREE( pQueue ) \
	({DHashListDel(&CNTRL.c_Queue,(void*)pQueue->q_Name,PFSQueue_t*,q_Lnk ); \
	PFSFree( pQueue );})


int
PFSInit( int Core, int Ext, int id, bool_t bCksum,
		int	FdMax, char *pRoot,
		int BlockMax, int SegSize, int SegNum, int UsecWB,
		struct PaxosAcceptor* pAcceptor )
{
	char	Root[PATH_NAME_MAX];
	int	Maximum;
	int	Ret;

	MyId	= id;
	CNTRL.c_Max	= Core;
	CNTRL.c_Maximum	= Maximum = Core + Ext;
	CNTRL.c_bCksum	= bCksum;

	CNTRL.c_pAcceptor	= pAcceptor;

	DHashListInit( &CNTRL.c_Lock, PRIMARY_1009, 200, PRIMARY_1013, 
				HASH_CODE_STR, HASH_CMP_STR,
				NULL, 
				(void*(*)(size_t))PaxosAcceptorGetfMalloc( pAcceptor ), 
				(void(*)(void*))PaxosAcceptorGetfFree( pAcceptor ), 
				NULL );
	DHashListInit( &CNTRL.c_Path, PRIMARY_1009, 200, PRIMARY_1013, 
				HASH_CODE_STR, HASH_CMP_STR,
				NULL, 
				(void*(*)(size_t))PaxosAcceptorGetfMalloc( pAcceptor ), 
				(void(*)(void*))PaxosAcceptorGetfFree( pAcceptor ), 
				NULL );
	DHashListInit( &CNTRL.c_Queue, PRIMARY_1009, 200, PRIMARY_1013, 
				HASH_CODE_STR, HASH_CMP_STR,
				NULL, 
				(void*(*)(size_t))PaxosAcceptorGetfMalloc( pAcceptor ), 
				(void(*)(void*))PaxosAcceptorGetfFree( pAcceptor ), 
				NULL );

	RwLockInit( &CNTRL.c_RwLock );

	sprintf( Root, "%s/%s", pRoot, PFS_PFS );
	Ret = FileCacheInit( &CNTRL.c_BlockCache, FdMax, Root, BlockMax,
							SegSize, SegNum, UsecWB, bCksum,
							PaxosAcceptorGetLog(pAcceptor) );
	
	if( Ret < 0 )	goto err;

	char *pEnv_ostype = 0;
	if( (pEnv_ostype = getenv( PFS_OSTYPE ) ) ) {
		if( 0 != strcmp( pEnv_ostype, "linux" ) ) {
			pPFS_Tar_Bin = "/usr/bin/tar";
		}
	}
	LOG( PaxosAcceptorGetLog(pAcceptor), LogINF, 
			"####pPFS_Tar_Bin=%s",pPFS_Tar_Bin);

	unlink( FILE_SHUTDOWN );
	return( 0 );
err:
	return( Ret );
}

#define	pBC	(&CNTRL.c_BlockCache)
#define	pFC	(&CNTRL.c_BlockCache.bc_FdCtrl)

int
_pfsUpdateDir( struct PaxosAccept* pAccept, char* pDirName, 
					PFSDirEntryMode_t Mode, char* pEntry )
{
	PFSPath_t*	pPath;
	SessionAdaptor_t	*pAdaptor;
	PFSJoint_t	*pWait, *pW;
	PFSEventDir_t*	pRpl;
	int	Ret;
	
	pPath = PATH_GET( pDirName );
	if( pPath ) {
		time_t	Time = time(0);
		SEQ_INC( pPath->p_EventSeq );
		list_for_each_entry_safe( pWait, pW, 
							&pPath->p_AdaptorEvent, j_Head.j_Event ) {

			pAdaptor	= pWait->j_pAdaptor;

			pRpl	= (PFSEventDir_t*)MSGALLOC( PFS_EVENT_DIR, 0, 
										sizeof(*pRpl), PFSMalloc );	
			pRpl->ed_Mode	= Mode;
			strncpy( pRpl->ed_Name, pDirName, sizeof(pRpl->ed_Name) );
			strncpy( pRpl->ed_Entry, pEntry, sizeof(pRpl->ed_Entry) );
			pRpl->ed_Time	= Time;
			pRpl->ed_EventSeq	= pPath->p_EventSeq;

			Ret = PaxosAcceptEventRaise( pAdaptor->a_pAccept,
												(PaxosSessionHead_t*)pRpl );
		} 
	}
	RETURN( Ret );
}

int
pfsUpdateDir( struct PaxosAccept* pAccept, char* pName, 
						PFSDirEntryMode_t Mode )
{
	char	DirPath[PATH_NAME_MAX];
	char	BaseName[FILE_NAME_MAX];
	char	*pDirName, *pBaseName;

	strncpy( DirPath, pName, sizeof(DirPath) );
	pDirName = dirname( DirPath );

	strncpy( BaseName, pName, sizeof(BaseName) );
	pBaseName = basename( BaseName );

	GElmCtrlLock( &pFC->fc_Ctrl );

	_pfsUpdateDir( pAccept, pDirName, Mode, pBaseName );

	GElmCtrlUnlock( &pFC->fc_Ctrl );

	return( 0 );
}

#define	MY_REAL_PATH( pName ) \
	char	PathName[PATH_NAME_MAX_REAL]; \
	if( *pName == '/' ) sprintf(PathName, "%s%s", PFS_PFS, pName ); \
	else				sprintf(PathName, "%s/%s", PFS_PFS, pName );

int
pfsMkdir( struct PaxosAccept* pAccept, char* pName )
{
	char	DirPath[PATH_NAME_MAX];
	char	BaseName[FILE_NAME_MAX];
	char	*pDirName, *pBaseName;

	MY_REAL_PATH( pName );

	sprintf( DirPath, "mkdir -p \"%s\"", PathName );
	if( system( DirPath ) ) {
		RETURN( -1 );
	}
	strncpy( DirPath, pName, sizeof(DirPath) );
	pDirName = RootOmitted(dirname( DirPath ));
	strncpy( BaseName, pName, sizeof(BaseName) );
	pBaseName = basename( BaseName );

	GElmCtrlLock( &pFC->fc_Ctrl );

	_pfsUpdateDir( pAccept, pDirName, DIR_ENTRY_CREATE, pBaseName );

	GElmCtrlUnlock( &pFC->fc_Ctrl );
	RETURN( 0 );
}

int
pfsRmdir( struct PaxosAccept* pAccept, char* pName )
{
	char	DirPath[PATH_NAME_MAX];
	char	BaseName[FILE_NAME_MAX];
	char	*pDirName, *pBaseName;
	DIR*	pDir;
	struct dirent*	pDirent;
	int		n;

	MY_REAL_PATH( pName );

	if( rmdir( PathName ) < 0 ) {
		if( errno == ENOTEMPTY ) {
			pDir	= opendir( PathName );
			if( !pDir )	return( -1 );
			n	= 0;
			while( (pDirent = readdir( pDir ) ) ) {
				if( !strcmp( ".", pDirent->d_name )
					|| !strcmp("..", pDirent->d_name ) ) {
					continue;
				}
				n++;
			}
			closedir( pDir );
			if( n )	{
				errno	= ENOTEMPTY;
				return( -1 );
			}
		} else {
			return( -1 );
		}
	}
	strncpy( DirPath, pName, sizeof(DirPath) );
	pDirName = RootOmitted(dirname( DirPath ));
	strncpy( BaseName, pName, sizeof(BaseName) );
	pBaseName = basename( BaseName );

	GElmCtrlLock( &pFC->fc_Ctrl );

	_pfsUpdateDir( pAccept, pDirName, DIR_ENTRY_DELETE, pBaseName );

	GElmCtrlUnlock( &pFC->fc_Ctrl );
	return( 0 );
}

int
PFSAdaptorOpen( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	SessionAdaptor_t*	pAdaptor;
	int	Ret = 0;

	DBG_ENT();

DBG_TRC("!!! FILE_SESSION_OPEN [pid=%u no=%d seq=%u] pAccept=%p pAdaptor=%p\n", 
pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq, pAccept, 
			PaxosAcceptGetTag( pAccept, PFS_FAMILY) );

	if ( (pAdaptor = (SessionAdaptor_t *)PaxosAcceptGetTag( pAccept, PFS_FAMILY )) ) {
LOG(pALOG, LogINF,
"REUSE ?:LockWait(empty?->%d),pLockWait(0x%p),LockHold(empty?->%d),Ephemeral.Lnk(empty?->%d),"
"Ephemeral.Hash(size(%d),Count(%d),CurIndex(%d),Heads(0x%p)),"
"Events(empty?->%d),QuWait(empty?->%d),pQueWait(0x%p),"
"pAccept(0x%p,(parent),0x%p)",
list_empty(&pAdaptor->a_LockWait),pAdaptor->a_pLockWait,list_empty(&pAdaptor->a_LockHold),
list_empty(&pAdaptor->a_Ephemeral.hl_Lnk),
pAdaptor->a_Ephemeral.hl_Hash.h_N,pAdaptor->a_Ephemeral.hl_Hash.h_Count,
pAdaptor->a_Ephemeral.hl_Hash.h_CurIndex,pAdaptor->a_Ephemeral.hl_Hash.h_aHeads,
list_empty(&pAdaptor->a_Events),list_empty(&pAdaptor->a_QueueWait),
pAdaptor->a_pQueueWait,pAdaptor->a_pAccept,pAccept);
	} else {

		pAdaptor = (SessionAdaptor_t*)PFSMalloc( sizeof(*pAdaptor) );

		list_init( &pAdaptor->a_LockWait );
		pAdaptor->a_pLockWait	= NULL;
		list_init( &pAdaptor->a_LockHold );

		HashListInit( &pAdaptor->a_Ephemeral, PRIMARY_101, 
				HASH_CODE_STR, HASH_CMP_STR, NULL, 
				PaxosAcceptGetfMalloc( pAccept ), 
				PaxosAcceptGetfFree( pAccept ), NULL );

		list_init( &pAdaptor->a_Events );

		list_init( &pAdaptor->a_QueueWait );
		pAdaptor->a_pQueueWait	= NULL;

		pAdaptor->a_pAccept	= pAccept;

		PaxosAcceptSetTag( pAccept, PFS_FAMILY, pAdaptor );

	}
	RETURN( Ret );
}

int queueGarbage( void* pVoid );
int pfsPeekAck( SessionAdaptor_t *, bool_t );
int pfsCopy( struct PaxosAccept *pAccept, char *pFrom, char *pTo );

int
PFSAdaptorClose( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	SessionAdaptor_t*	pAdaptor;
	Ephemeral_t		*pEphemeral, *pEW;
	PFSPath_t*	pPath;
	int	Ret = 0;
	PFSJoint_t	*pJoint, *pW;
	bool_t bAbort = FALSE;

	if( pM->h_Cmd == PAXOS_SESSION_ABORT )	bAbort = TRUE;

	DBG_ENT();

DBG_TRC("!!! FILE_SESSION_CLOSE pAccept=0x%x [pid=%d No=%d Seq=%d]\n", 
			pAccept, pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq );
	if( !pAccept ) {
		/* BINDだけでRecover時には存在しない */
		RETURN( -1 );
	}
	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, 
												PFS_FAMILY );
	if( !pAdaptor ) {
	/* BINDだけ */
		LOG( pALOG, LogERR, "#### NOT SESSION_OPEN pAccept=%p ###", pAccept);
		RETURN( -1 );
	}

	RwLockW( &CNTRL.c_RwLock );
	/*
	 *	Queue
	 */
	if( pAdaptor->a_pQueueWait ) {
		pfsPeekAck( pAdaptor, FALSE );
	}
	/*
	 * Lock
	 */
	list_del_init( &pAdaptor->a_LockWait );
	while( (pJoint = list_last_entry( &pAdaptor->a_LockHold,
								PFSJoint_t, j_Head.j_Wait ) ) ) {
		Ret = pfsUnlock( pAccept, &pAccept->a_Id, pJoint, bAbort );
	}
	/*
	 * Ephemeral File
	 */
	list_for_each_entry_safe( pEphemeral, pEW,
							&pAdaptor->a_Ephemeral.hl_Lnk, e_Lnk ) {

		pPath	= PATH_GET( pEphemeral->e_Name );

		ASSERT( pPath );

		if( !(--pPath->p_RefCnt) ) {
			char BackUp[PATH_NAME_MAX];
			off_t	Off = 0;
			char	Buff[4096];
			int		n;

			snprintf( BackUp, sizeof(BackUp), "%s_BAK", pPath->p_Name );

			FileCacheDelete( pBC, BackUp );
			FileCacheCreate( pBC, BackUp );
			while( (Ret = n = FileCacheRead( pBC, pPath->p_Name,
									Off, sizeof(Buff), Buff ) ) > 0 ) {
				Ret = FileCacheWrite( pBC, BackUp, Off, n, Buff );
				if( Ret < 0 )	break;
				Off	+= n;
			}
			Ret = FileCacheDelete( pBC, pPath->p_Name );
			if( Ret == 0 ) {
				pfsUpdateDir( pAccept, pPath->p_Name, DIR_ENTRY_DELETE );
			}
			PATH_FREE( pPath->p_Name );
		}		
		EPHEMERAL_FREE( pAdaptor, pEphemeral->e_Name );
	}
	/*
	 *	Multi Wait
	 */
	list_for_each_entry_safe( pJoint, pW, 
							&pAdaptor->a_Events, j_Head.j_Wait) {
		JOINT_FREE( pJoint );
	}
	HashListDestroy( &pAdaptor->a_Ephemeral );

	PFSFree( pAdaptor );
	PaxosAcceptSetTag( pAccept, PFS_FAMILY, NULL );

	RwUnlock( &CNTRL.c_RwLock );

	RETURN( Ret );
}

int
PFSEphemeral( PaxosAccept_t *pAccept, char *pPathName )
{
	PFSPath_t*			pPath;
	SessionAdaptor_t*	pAdaptor;

	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, PFS_FAMILY );

	if( !EPHEMERAL_GET( pAdaptor, pPathName ) ) {
		EPHEMERAL_ALLOC( pAdaptor, pPathName );
		pPath	= PATH_GET( pPathName );
		if( !pPath )	pPath = PATH_ALLOC( pPathName );
		pPath->p_RefCnt++;
		pPath->p_Flags	|= FILE_EPHEMERAL;
	}
	return( 0 );
}

int
FileCacheWRITE( struct PaxosAccept *pAccept, 
	char *pName, off_t Offset, size_t Len, void *pData )
{ 
	int	Ret;

	Ret = FileCacheWrite( pBC, pName, Offset, Len, pData );
	if( Ret < 0 ) {
		Ret = FileCacheCreate( pBC, pName );
		if( Ret >= 0 ) {

			pfsUpdateDir( pAccept, pName, DIR_ENTRY_CREATE );

			Ret = FileCacheWrite( pBC, pName, Offset, Len, pData );
		}
	}
	if( Ret > 0 ) {
		pfsUpdateDir( pAccept, pName, DIR_ENTRY_UPDATE );
	}
	return( Ret );
}

int
PFSDataApply( struct PaxosAccept* pAccept, PaxosSessionHead_t* pRpl )
{
	PFSWriteReq_t*		pReq;
	int	Ret;

	DBG_ENT();

	switch( (int)pRpl->h_Code ) {
	case PFS_WRITE:
		pReq = (PFSWriteReq_t*)(pRpl+1);

		char *pName = RootOmitted(pReq->w_Name);
		if( pReq->w_Flags & FILE_EPHEMERAL ) {
			PFSEphemeral( pAccept, pName );
		}
		Ret = FileCacheWRITE( pAccept, pName, 
					pReq->w_Offset, pReq->w_Len, (char*)(pReq+1));
		if( Ret < 0 )	goto err;
		break;
	}
	RETURN( 0 );
err:
	RETURN( -1 );
}

int PFSPostS( struct PaxosAccept* pAccept, PFSHead_t* pH );

int
PFSOutOfBandReq( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSOutOfBandRpl_t *pRpl;
	PaxosSessionHead_t*	pData;
	int	Ret = -1;

	DBG_ENT();

	PaxosAcceptOutOfBandLock( pAccept );

	pData	= _PaxosAcceptOutOfBandGet( pAccept, &pH->h_Head.h_Id );

	ASSERT( pData );

#ifdef KKK
#else
	if ( pData->h_Len <= sizeof(*pData) ) {
		PFSCmd_t cmd = PFS_OUTOFBAND;
		switch ( (int)pData->h_Code ) {
		default:
			break;
		case PFS_WRITE:
			cmd = PFS_OUTOFBAND;
			break;
		case PFS_POST_MSG:
			cmd = PFS_POST_MSG;
			break;
		}
		pRpl = (PFSOutOfBandRpl_t*)MSGALLOC( cmd, 0, sizeof(*pRpl),
									PFSMalloc );	
		pRpl->o_Head.h_Error	= EINVAL;

		PaxosAcceptOutOfBandUnlock( pAccept );

		Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->o_Head.h_Head.h_Len);
		RETURN( Ret );
	}
#endif
	PFSHead_t	*pO;
	PFSHead_t	*pBody;

	pBody	= (PFSHead_t*)(pData+1);	// skip Push
	
	switch( (int)pBody->h_Head.h_Cmd ) {

	case PFS_POST_MSG:

		pO	= PaxosAcceptGetfMalloc(pAccept)( pBody->h_Head.h_Len );
		memcpy( pO, pBody, pBody->h_Head.h_Len );

		PFSPostS( pAccept, pO );

		PaxosAcceptOutOfBandUnlock( pAccept );
		break;

	default:
		Ret = PFSDataApply( pAccept, pData );

		PaxosAcceptOutOfBandUnlock( pAccept );

		pRpl = (PFSOutOfBandRpl_t*)MSGALLOC( PFS_OUTOFBAND, 0, sizeof(*pRpl),
									PFSMalloc );	
		if( Ret ) 	pRpl->o_Head.h_Error	= errno;
		else		pRpl->o_Head.h_Error	= 0;

		Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->o_Head.h_Head.h_Len);
		break;
	}
	RETURN( Ret );
}

int
PFSWriteS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSWriteReq_t *pReq = (PFSWriteReq_t*)pH;
	PFSWriteRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	char *pName = RootOmitted(pReq->w_Name);
	if( pReq->w_Flags & FILE_EPHEMERAL ) {
		PFSEphemeral( pAccept, pName );
	}
	pRpl	= (PFSWriteRpl_t*)MSGALLOC( PFS_WRITE, 0, sizeof(*pRpl), 
												PFSMalloc );	
	Ret = FileCacheWRITE( pAccept, pName, 
					pReq->w_Offset, pReq->w_Len, pReq->w_Data );

	if( Ret < 0 ) 	pRpl->w_Head.h_Error	= errno;
	else			pRpl->w_Head.h_Error	= 0;
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->w_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSReadS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSReadReq_t *pReq = (PFSReadReq_t*)pH;
	PFSReadRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSReadRpl_t*)MSGALLOC( PFS_READ, 0, 
						sizeof(*pRpl) + pReq->r_Len, PFSMalloc );	
	Ret = FileCacheRead( pBC, RootOmitted(pReq->r_Name), 
					pReq->r_Offset, pReq->r_Len, (char*)(pRpl+1) );

	if( Ret > 0 ) {
		pRpl->r_Len						= Ret;
		pRpl->r_Head.h_Head.h_Len		= sizeof(*pRpl) + Ret;
		pRpl->r_Head.h_Error			= 0;
	} else {
		pRpl->r_Len						= 0;
		pRpl->r_Head.h_Head.h_Len		= sizeof(*pRpl);
		pRpl->r_Head.h_Error			= errno;
	}
	Ret = ACCEPT_REPLY_MSG( pAccept, FALSE,
			pRpl, sizeof(*pRpl) + pReq->r_Len, pRpl->r_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSCreateS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSCreateReq_t *pReq = (PFSCreateReq_t*)pH;
	PFSCreateRpl_t *pRpl;
	int	Ret = 0;

	DBG_ENT();
	char *pName = RootOmitted(pReq->c_Name);
	if( pReq->c_Flags & FILE_EPHEMERAL ) {
		PFSEphemeral( pAccept, pName );
	}

	pRpl	= (PFSCreateRpl_t*)MSGALLOC( PFS_CREATE, 0, sizeof(*pRpl),
									PFSMalloc );	
	Ret	= FileCacheCreate( pBC, pName );
	if( Ret == 0 ) {
		pfsUpdateDir( pAccept, pName, DIR_ENTRY_CREATE );
	}

	if( Ret )	pRpl->c_Head.h_Error = errno;
	else		pRpl->c_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->c_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSDeleteS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSDeleteReq_t *pReq = (PFSDeleteReq_t*)pH;
	PFSDeleteRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSDeleteRpl_t*)MSGALLOC( PFS_DELETE, 0, sizeof(*pRpl),
									PFSMalloc );	
	pRpl->d_Head.h_Error	= -1;

	char *pName = RootOmitted(pReq->d_Name);
	Ret = FileCacheDelete( pBC, pName );
	if( Ret == 0 ) {
		pfsUpdateDir( pAccept, pName, DIR_ENTRY_DELETE );
	}

	if( Ret )	pRpl->d_Head.h_Error = errno;
	else		pRpl->d_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->d_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSTruncateS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSTruncateReq_t *pReq = (PFSTruncateReq_t*)pH;
	PFSTruncateRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSTruncateRpl_t*)MSGALLOC( PFS_TRUNCATE, 0, sizeof(*pRpl),
									PFSMalloc );	

	char *pName = RootOmitted(pReq->t_Name);
	Ret = FileCacheTruncate( pBC, pName, pReq->t_Len );
	if( Ret == 0 ) {
		pfsUpdateDir( pAccept, pName, DIR_ENTRY_UPDATE );
	}

	if( Ret )	pRpl->t_Head.h_Error = errno;
	else		pRpl->t_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->t_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
pfsCopy( struct PaxosAccept *pAccept, char *pFrom, char *pTo )
{
	int	Ret;
	off_t	Off = 0;
	char	Buff[4096];
	int		n;

	Ret = FileCacheDelete( pBC, pTo );
	if( Ret == 0 ) {
		pfsUpdateDir( pAccept, pTo, DIR_ENTRY_DELETE );
	}
	Ret = FileCacheCreate( pBC, pTo );
	if( Ret == 0 ) {
		pfsUpdateDir( pAccept, pTo, DIR_ENTRY_CREATE );
	}
	if( Ret < 0 )	goto ret;
	while( (Ret = n = FileCacheRead( pBC, pFrom,
							Off, sizeof(Buff), Buff ) ) > 0 ) {
		Ret = FileCacheWrite( pBC, pTo, Off, n, Buff );
		if( Ret < 0 )	goto ret;
		Off	+= n;
	}
ret:
	return( Ret );
}

int
PFSCopyS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSCopyReq_t *pReq = (PFSCopyReq_t*)pH;
	PFSCopyRpl_t *pRpl;
	int	Ret;

	pRpl	= (PFSCopyRpl_t*)MSGALLOC( PFS_COPY, 0, sizeof(*pRpl),
									PFSMalloc );	

	char *pFrom = RootOmitted(pReq->c_From);
	char *pTo = RootOmitted(pReq->c_To);

	Ret = pfsCopy( pAccept, pFrom, pTo );

	if( Ret < 0 )	pRpl->c_Head.h_Error = errno;
	else			pRpl->c_Head.h_Error = 0;
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->c_Head.h_Head.h_Len );
	return( Ret );
}

int
PFSConcatS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSConcatReq_t *pReq = (PFSConcatReq_t*)pH;
	PFSConcatRpl_t *pRpl;
	int	Ret;
	off_t	Off = 0;
	char	Buff[4096];
	int		n;

	pRpl	= (PFSConcatRpl_t*)MSGALLOC( PFS_CONCAT, 0, sizeof(*pRpl),
									PFSMalloc );	

	char *pFrom = RootOmitted(pReq->c_From);
	char *pTo = RootOmitted(pReq->c_To);
	while( (Ret = n = FileCacheRead( pBC, pFrom,
							Off, sizeof(Buff), Buff ) ) > 0 ) {
		Ret = FileCacheAppend( pBC, pTo, n, Buff );
		if( Ret < 0 )	goto ret;
		Off	+= n;
	}
	if( Ret >= 0 ) {
		pfsUpdateDir( pAccept, pTo, DIR_ENTRY_UPDATE );
	}
ret:
	if( Ret < 0 )	pRpl->c_Head.h_Error = errno;
	else			pRpl->c_Head.h_Error = 0;
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
					pRpl, sizeof(*pRpl), pRpl->c_Head.h_Head.h_Len );
	return( Ret );
}

int
PFSStatS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSStatReq_t *pReq = (PFSStatReq_t*)pH;
	PFSStatRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSStatRpl_t*)MSGALLOC( PFS_STAT, 0, sizeof(*pRpl), PFSMalloc );	

	char *pName = RootOmitted(pReq->s_Name);

	Ret = FileCacheStat( pBC, pName, &pRpl->s_Stat );
LOG( pALOG, LogINF, "pName[%s] Ret=%d st_ctime=%lu", 
pName, Ret, pRpl->s_Stat.st_ctime );
	if( Ret < 0 ) {
		// directory ?
		MY_REAL_PATH( pName );
		Ret = stat( PathName, &pRpl->s_Stat );
LOG( pALOG, LogINF, "PathName[%s] Ret=%d st_ctime=%lu", 
PathName, Ret, pRpl->s_Stat.st_ctime );
	}

	if( Ret )	pRpl->s_Head.h_Error = errno;
	else		pRpl->s_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, FALSE,
					pRpl, sizeof(*pRpl), pRpl->s_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSMKdirS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSMkdirReq_t *pReq = (PFSMkdirReq_t*)pH;
	PFSMkdirRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSMkdirRpl_t*)MSGALLOC( PFS_MKDIR, 0, sizeof(*pRpl),
										PFSMalloc );	

	Ret = pfsMkdir( pAccept, pReq->m_Name );
	if( Ret ) 	pRpl->m_Head.h_Error = errno;
	else		pRpl->m_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
				pRpl, sizeof(*pRpl), pRpl->m_Head.h_Head.h_Len );
	RETURN( Ret );
}

int
PFSRmdirS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSRmdirReq_t *pReq = (PFSRmdirReq_t*)pH;
	PFSRmdirRpl_t *pRpl;
	int	Ret;

	DBG_ENT();

	pRpl	= (PFSRmdirRpl_t*)MSGALLOC( PFS_RMDIR, 0, sizeof(*pRpl),
										PFSMalloc );	

	Ret = pfsRmdir( pAccept, pReq->r_Name );

	if( Ret ) 	pRpl->r_Head.h_Error = errno;
	else		pRpl->r_Head.h_Error = 0;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
				pRpl, sizeof(*pRpl), pRpl->r_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
pfsLockGarbage( void* pArg )
{
	PFSLock_t*	pLock = (PFSLock_t*)pArg;

	if( list_empty( &pLock->l_AdaptorWait )
		&& list_empty( &pLock->l_AdaptorHold )
		&& list_empty( &pLock->l_AdaptorEvent ) ) {

		ASSERT( !pLock->l_Cnt );
		LOCK_FREE( pLock );
		return( 0 );
	}
	return( -1 );
}

int
PFSLockNotify( struct PaxosAccept *pAccept, PFSLock_t *pLock )
{
	PFSJoint_t*	pJoint;
	struct PaxosAccept*	pAcceptEvent;
	PaxosClientId_t *pClientAccept, *pClientEvent;
	PFSEventLock_t	*pEvent;
	int	Ret = 0;

	/* Lock回収通知 */
	SEQ_INC( pLock->l_EventSeq );
	list_for_each_entry( pJoint, &pLock->l_AdaptorEvent, j_Head.j_Event ) {

		pAcceptEvent	= pJoint->j_pAdaptor->a_pAccept;

		pClientAccept	= PaxosAcceptorGetClientId( pAccept );
		pClientEvent	= PaxosAcceptorGetClientId( pAcceptEvent );

		if( !HoldHashCmp( pClientAccept, pClientEvent ) ) {
			continue;
		}
LOG( pALOG, LogDBG, "l_Cnt=%d j_Type=%d", pLock->l_Cnt, pJoint->j_Type ); 
		ASSERT( pLock->l_Cnt != 0 );
		if( pJoint->j_Type == JOINT_EVENT_LOCK_RECALL ) {
			if(!HashGet(&pLock->l_HoldHash, pClientEvent))	continue;
		}
		pEvent	= (PFSEventLock_t*)MSGALLOC( PFS_EVENT_LOCK, 0, 
									sizeof(*pEvent), PFSMalloc );	
		strncpy( pEvent->el_Name, pLock->l_Name, sizeof(pEvent->el_Name));
		pEvent->el_Ver	= pLock->l_Ver;
		pEvent->el_Time		= time(0);
		pEvent->el_EventSeq	= pLock->l_EventSeq;

		Ret = PaxosAcceptEventRaise( pAcceptEvent, 
						(PaxosSessionHead_t*)pEvent );

LOG( pALOG, LogDBG, "NOTIFY:Pid=%d", 
PaxosAcceptorGetClientId( pAcceptEvent )->i_Pid );
	}
	return( Ret );
}

int
PFSLockS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSLockReq_t 	*pReq = (PFSLockReq_t*)pH;
	PFSLockRpl_t 	*pRpl;
	int	Ret = 0;
	PFSLock_t*	pLock;
	SessionAdaptor_t*	pAdaptor;
	PFSJoint_t*	pJoint;
	PFSJointType_t	Type;
	bool_t	bEmptyWait;

	DBG_ENT();

	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, PFS_FAMILY );
ASSERT( pAdaptor );

	pLock	= (PFSLock_t*)DHashListGet( &CNTRL.c_Lock, (void*)pReq->l_Name );
	if( !pLock ) {
		pLock = LOCK_ALLOC( pReq->l_Name );
		ASSERT( pLock );
	}
	if( !list_empty( &pLock->l_AdaptorHold) ) {

		bEmptyWait	= list_empty( &pLock->l_AdaptorWait );

		LOG( pALOG, LogINF, 
		"bEmptyWait=%d l_bRead=%d l_bTry=%d l_Cnt=%d l_Ver=%d", 
		bEmptyWait, pReq->l_bRead, pReq->l_bTry, pLock->l_Cnt, pLock->l_Ver );
		/*
		 * R状態でR要求かつ要求待ちがないのでRを発行
		 */
		if( bEmptyWait && pReq->l_bRead && pLock->l_Cnt > 0 ) {
			goto reply;
		}
		pJoint = HashGet(&pLock->l_HoldHash, PaxosAcceptorGetClientId(pAccept));
LOG( pALOG, LogDBG, "pJoint=%p", pJoint ); 
		if( pJoint ) {
			/* Delegを持っているので解放し、再取得 ??? */
			pfsUnlock( pAccept, &pAccept->a_Id, pJoint, FALSE );
			if( pReq->l_bRead && pLock->l_Cnt >= 0 )	goto reply;
			if( !pReq->l_bRead && pLock->l_Cnt == 0 )	goto reply;
		}
		/* 待ち 重複Lockがある Master交替時
			応答済みはDupricateではじかれる
		*/
		if( list_empty( &pAdaptor->a_LockWait ) ) {
			pAdaptor->a_LockRead	= pReq->l_bRead;
			/* Jointを使っていない */
			list_add_tail( &pAdaptor->a_LockWait, &pLock->l_AdaptorWait );
			pAdaptor->a_pLockWait	= pLock;

			LOG( pALOG, LogINF, 
			"WAIT:Pid=%d l_bRead=%d l_Cnt=%d l_AdaptorWait=%d", 
			PaxosAcceptorGetClientId( pAccept )->i_Pid,
			pReq->l_bRead, pLock->l_Cnt, bEmptyWait );


			/* Lock回収通知 */
			if( bEmptyWait )	Ret = PFSLockNotify( pAccept, pLock );
		}

		RETURN( 0 );
	}

reply:
	if( pReq->l_bRead ) {
		pLock->l_Cnt++;

		LOG( pALOG, LogINF,
		"Read:l_Cnt=%d l_Ver=%d Pid=%d", pLock->l_Cnt, pLock->l_Ver,
		PaxosAcceptorGetClientId( pAccept )->i_Pid );

		if( pLock->l_Cnt == 1 )	SEQ_INC( pLock->l_Ver );
		Type = JOINT_LOCK_READ_HOLD;
	} else {
		pLock->l_Cnt--;
		SEQ_INC( pLock->l_Ver );
		Type = JOINT_LOCK_WRITE_HOLD;
	}
	/*
	 * l_AdaptorHoldにPFSJointのj_Head.j_Event
	 * a_LockHoldにPFSJointのj_Head.j_Wait
	 */
	pJoint = JOINT_ALLOC( pAdaptor, &pAdaptor->a_LockHold, Type, 
						&pLock->l_AdaptorHold, pfsLockGarbage, pLock );

	HashPut( &pLock->l_HoldHash, PaxosAcceptorGetClientId(pAccept), pJoint );

	pRpl	= (PFSLockRpl_t*)MSGALLOC( PFS_LOCK, 0, sizeof(*pRpl),
										PFSMalloc );	
	pRpl->l_Ver	= pLock->l_Ver;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
				pRpl, sizeof(*pRpl), pRpl->l_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSTryLockS( struct PaxosAccept* pAccept, PFSHead_t* pH, bool_t bAccepted )
{
	PFSLockReq_t 	*pReq = (PFSLockReq_t*)pH;
	PFSLockRpl_t 	*pRpl;
	PFSLock_t*	pLock;
	bool_t		bLock = FALSE;

//LOG( pALOG, "ENT:[%s] l_bRead=%d l_bTry=%d bAccepted=%d", 
//	pReq->l_Name, pReq->l_bRead, pReq->l_bTry, bAccepted  ); 

	if( pReq->l_bTry ) {
		pLock	= (PFSLock_t*)
			DHashListGet( &CNTRL.c_Lock, (void*)pReq->l_Name );

		if( !pLock )	bLock = TRUE;
		else {
			if( pReq->l_bRead ) {
				if( pLock->l_Cnt >= 0 )	bLock = TRUE;
			} else {
				if( pLock->l_Cnt == 0 ) bLock = TRUE;
			}
		}

		if( !bLock ) {

			pRpl	= (PFSLockRpl_t*)
				MSGALLOC( PFS_LOCK, 0, sizeof(*pRpl), PFSMalloc );	

			pRpl->l_Ver	= pLock->l_Ver;
			pRpl->l_Head.h_Error	= EBUSY;

			LOG( pALOG, LogINF,
			"BUSY:[%s] l_Cnt=%d l_Ver=%d bRead=%d bAccepted=%d", 
			pLock->l_Name, pLock->l_Cnt, pLock->l_Ver, 
			pReq->l_bRead, bAccepted );

			if( !bAccepted )	PaxosAcceptedStart( pAccept, &pH->h_Head );

			ACCEPT_REPLY_MSG( pAccept, bAccepted,
				pRpl, sizeof(*pRpl), pRpl->l_Head.h_Head.h_Len );

			return( 0 );
		}
	}
	if( bAccepted ) {
//LOG( pALOG, "CALL PFSLockS:[%s]", pReq->l_Name ); 
		PFSLockS( pAccept, pH );
		return( 0 );
	} else {
//LOG( pALOG, "CALL PaxosRequest:[%s]", pReq->l_Name ); 
		return( 1 );
	}
}

int
pfsUnlock( struct PaxosAccept *pAccept, 
			PaxosClientId_t* pId, PFSJoint_t* pJoint, bool_t bAbort )
{
	int	Ret = 0;
	PFSLock_t	*pLock	= (PFSLock_t*)pJoint->j_pArg;
	PFSJointType_t		Type;

	DBG_ENT();

LOG( pALOG, LogDBG, "l_Name[%s] l_Cnt=%d bAbort=%d", 
						pLock->l_Name, pLock->l_Cnt, bAbort );
	ASSERT( pLock->l_Cnt );

	if( bAbort && pLock->l_Cnt < 0 ) {
		Ret = FileCacheDelete( pBC, pLock->l_Name );
		LOG( pALOG, LogINF, "[%s] is Deleted(Ret=%d)", pLock->l_Name, Ret );
	}
DBG_TRC("[%s] Cnt=%d\n", pLock->l_Name, pLock->l_Cnt );
	if( pLock->l_Cnt > 0 )	pLock->l_Cnt--;
	else					pLock->l_Cnt++;

	HashRemove( &pLock->l_HoldHash, pId );

	if( !JOINT_FREE( pJoint ) )	return( 0 );

	if( pLock->l_Cnt == 0 ) {
		/* 待ちを起床する */
		struct SessionAdaptor*	pAdaptorWait, *pW;
		PFSLockRpl_t*		pLockRpl;

		list_for_each_entry_safe( pAdaptorWait, pW,
									&pLock->l_AdaptorWait, a_LockWait ) {

			if( !pAdaptorWait->a_LockRead && pLock->l_Cnt )	continue;

			if( pAdaptorWait->a_LockRead ) {

				LOG( pALOG, LogINF, "ReadWait:l_Cnt=%d", pLock->l_Cnt );

				pLock->l_Cnt++;
				if( pLock->l_Cnt == 1 ) SEQ_INC( pLock->l_Ver );
				Type = JOINT_LOCK_READ_HOLD;
			} else{
				pLock->l_Cnt--;
				SEQ_INC( pLock->l_Ver );
				Type = JOINT_LOCK_WRITE_HOLD;
			}

			list_del_init( &pAdaptorWait->a_LockWait );
			pAdaptorWait->a_pLockWait	= NULL;
			pJoint = JOINT_ALLOC( pAdaptorWait, &pAdaptorWait->a_LockHold, 
					Type, &pLock->l_AdaptorHold, pfsLockGarbage, pLock );

			HashPut( &pLock->l_HoldHash, 
				PaxosAcceptorGetClientId(pAdaptorWait->a_pAccept), pJoint );

			pLockRpl	= (PFSLockRpl_t*)
							MSGALLOC( PFS_LOCK, 0, sizeof(*pLockRpl),
										PFSMalloc );	
			pLockRpl->l_Ver	= pLock->l_Ver;

			LOG( pALOG, LogINF, "WAKE:Pid=%d", 
			PaxosAcceptorGetClientId( pAdaptorWait->a_pAccept )->i_Pid );

			Ret = ACCEPT_REPLY_MSG( pAdaptorWait->a_pAccept, TRUE,
				pLockRpl, sizeof(*pLockRpl), pLockRpl->l_Head.h_Head.h_Len );
			if( Ret >= 0 )	Ret = 0;

			if( pLock->l_Cnt < 0 )	break;
		}
		pAdaptorWait = list_first_entry( &pLock->l_AdaptorWait, 
									struct SessionAdaptor, a_LockWait );
		if( pAdaptorWait ) {	
			Ret = PFSLockNotify( pAdaptorWait->a_pAccept, pLock );
		}
	}
	RETURN( Ret );
}

int
PFSUnlockS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSUnlockReq_t *pReq = (PFSUnlockReq_t*)pH;
	PFSUnlockRpl_t *pRpl;
	int	Ret = 0;
	SessionAdaptor_t*	pAdaptor;
	PFSLock_t*	pLock;
	PFSJoint_t*	pJoint;
	bool_t	Found = FALSE;

	DBG_ENT();

	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, 
												PFS_FAMILY );
	pLock	= (PFSLock_t*)DHashListGet( &CNTRL.c_Lock, (void*)pReq->l_Name );

	list_for_each_entry( pJoint, &pAdaptor->a_LockHold, j_Head.j_Wait ) {

		if( pJoint->j_pArg == pLock ) {
			Found = TRUE;
			break;
		}
	}

if( !Found ) {
PFSJoint_t*	pJoint1;
PaxosAccept_t*	pAccept1;

if( pLock ) {
LOG( pALOG, LogDBG,
"#####(!Found) Packet Pid=%d-%d Seq=%u SeqWait=%u [%s] l_Cnt=%d pAdaptor=%p", 
pH->h_Head.h_Id.i_Pid, pH->h_Head.h_Id.i_No, 
pH->h_Head.h_Id.i_Seq, pH->h_Head.h_Id.i_SeqWait, pLock->l_Name, pLock->l_Cnt, pAdaptor );

	list_for_each_entry( pJoint1, &pLock->l_AdaptorHold, j_Head.j_Event ) {
		pAccept1	= pJoint1->j_pAdaptor->a_pAccept;
		LOG( pALOG, LogINF, "Hold Pid=%d-%d", 
			pAccept1->a_Id.i_Pid, pAccept1->a_Id.i_No );
	}
} else {

LOG( pALOG, LogDBG,
"#####(!Found) Packet Pid=%d-%d Seq=%u SeqWait=%u pAdaptor=%p", 
pH->h_Head.h_Id.i_Pid, pH->h_Head.h_Id.i_No, 
pH->h_Head.h_Id.i_Seq, pH->h_Head.h_Id.i_SeqWait, pAdaptor );
}

LOG( pALOG, LogDBG, "Accept Pid=%d-%d Opened=%d Start=%u End=%u Reply=%u",
pAccept->a_Id.i_Pid, pAccept->a_Id.i_No,
pAccept->a_Opened, pAccept->a_Accepted.a_Start, pAccept->a_Accepted.a_End,
pAccept->a_Accepted.a_Reply );

LOG( pALOG, LogDBG, 
"NEXT=%p",DHashNext( &CNTRL.c_Lock.hl_Hash, (void*)pReq->l_Name));
}
//ASSERT( Found );

	if( !pLock || !Found ) {
		Ret = -1;
	} else {
		pfsUnlock( pAccept, &pAccept->a_Id, pJoint, FALSE );
	}
	pRpl	= (PFSUnlockRpl_t*)MSGALLOC( PFS_UNLOCK, 0, sizeof(*pRpl),
										PFSMalloc );	
	pRpl->l_Head.h_Error	= Ret;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
			pRpl, sizeof(*pRpl), pRpl->l_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSUnlockSuperS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSUnlockSuperReq_t *pReq = (PFSUnlockSuperReq_t*)pH;
	PFSUnlockSuperRpl_t *pRpl;
	int	Ret = -1;
	PFSLock_t*	pLock;
	PFSJoint_t*	pJoint;
	PaxosClientId_t	*pId;

	DBG_ENT();

	pId	= &pReq->l_Id;
	pLock	= (PFSLock_t*)DHashListGet( &CNTRL.c_Lock, (void*)pReq->l_Name );
	if( pLock ) {

		pJoint = HashGet(&pLock->l_HoldHash, pId );
		LOG( pALOG, LogDBG, "pJoint=%p", pJoint ); 
		if( pJoint ) {
			/* Delegを持っているので解放 */
			pfsUnlock( pAccept, pId,  pJoint, FALSE );
			Ret = 0;
		}
	} else {
		LOG( pALOG, LogDBG,
		"#####(!Found) Pid=%d-%d Seq=%u SeqWait=%u [%s] l_Cnt=%d pAdaptor=%p", 
		pId->i_Pid, pId->i_No );
	}
	pRpl	= (PFSUnlockSuperRpl_t*)MSGALLOC( PFS_UNLOCK_SUPER, 
							0, sizeof(*pRpl), PFSMalloc );	
	pRpl->l_Head.h_Error	= Ret;

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
			pRpl, sizeof(*pRpl), pRpl->l_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSReadDirS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSReadDirReq_t *pReq = (PFSReadDirReq_t*)pH;
	PFSReadDirRpl_t *pRpl;
	int	Ret = 0;
	int	i, n, N, Len;
	struct dirent*	pDirent;
	PFSDirent_t*	pFileDirent;
	Msg_t		*pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	pMsgRpl	= MsgCreate( 2, PaxosAcceptGetfMalloc(pAccept),
							PaxosAcceptGetfFree(pAccept) );
	pRpl	= (PFSReadDirRpl_t*)MsgMalloc( pMsgRpl, sizeof(*pRpl) );
	MsgVecSet( &Vec, VEC_MINE, pRpl, sizeof(*pRpl), sizeof(*pRpl) );
	MsgAdd( pMsgRpl, &Vec );
					
	N	= pReq->r_Number;
	Len	= sizeof(PFSDirent_t)*N;

	pFileDirent	= (PFSDirent_t*)MsgMalloc( pMsgRpl, Len );	

	MsgVecSet( &Vec, VEC_MINE, pFileDirent, Len, Len );
	MsgAdd( pMsgRpl, &Vec );

    MSGINIT( pRpl, PFS_READDIR, 0, sizeof(*pRpl) );

	pDirent	= MsgMalloc( pMsgRpl, sizeof(struct dirent)*N );

	n = FileCacheReadDir( pBC, pReq->r_Name, pDirent, pReq->r_Index, N );
LOG( pALOG, LogDBG, "r_Name[%s] Index=%d N=%d n=%d",
pReq->r_Name, pReq->r_Index, N, n ); 
	if( n < 0 ) {

		MsgFree( pMsgRpl, pDirent );

		pRpl->r_Head.h_Error	= errno;
		goto ret;
	} 
	for( i = 0; i < n; i++ ) {
LOG( pALOG, LogDBG, "### [%s] ###", pDirent[i].d_name );
		pFileDirent[i].de_ent	= pDirent[i];
	}
	MsgFree( pMsgRpl, pDirent );

	pRpl->r_Number	= n;
	pRpl->r_Head.h_Head.h_Len += sizeof(PFSDirent_t)*n;

ret:
	Ret = PaxosAcceptReplyReadByMsg( pAccept, pMsgRpl );

	RETURN( Ret );
}

int
pfsPathGarbage( void* pArg )
{
	PFSPath_t*	pPath = (PFSPath_t*)pArg;

	if( list_empty( &pPath->p_AdaptorEvent ) ) {
		PATH_FREE( pPath->p_Name );
		return( 0 );
	}
	return( -1 );
}

int
PFSEventDirS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSEventDirReq_t *pReq = (PFSEventDirReq_t*)pH;
	PFSEventDirRpl_t *pRpl;
	PFSPath_t*	pPath;
	int	Ret;
	SessionAdaptor_t*	pAdaptor;
	PFSJoint_t	*pJoint, *pW;

	DBG_ENT();

	errno	= 0;

	struct PaxosAccept	*pE;	
	PaxosSessionHead_t	Head;

LOG( pALOG, LogDBG, "ed_Set=%d ed_Name[%s]", pReq->ed_Set, pReq->ed_Name );

	pRpl	= (PFSEventDirRpl_t*)MSGALLOC( PFS_EVENT_DIR, 0, 
										sizeof(*pRpl), PFSMalloc );	

	Head.h_Id	= pReq->ed_Id;
	pE	= PaxosAcceptGet( CNTRL.c_pAcceptor, &Head.h_Id, FALSE );
	if( !pE ) {
		pRpl->ed_Head.h_Error	= ENOENT;
		Ret	= -1;
		goto err1;
	}
	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pE, PFS_FAMILY );

	if( pReq->ed_Set ) {
		pPath = PATH_GET( pReq->ed_Name );
		if( !pPath ) {
			pPath	= PATH_ALLOC( pReq->ed_Name );
LOG( pALOG, LogDBG, "PATH_ALLOC Error" );
			if( !pPath )	RETURN( -1 );
		}
		/* set Wait */
		pJoint 		= JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, 
								JOINT_EVENT_PATH, &pPath->p_AdaptorEvent,
								pfsPathGarbage, pPath );
		ASSERT( pJoint );
		Ret = 0;
	} else {
		/* cancel Wait */

		Ret = -1;
		pPath = PATH_GET( pReq->ed_Name );
		ASSERT( pPath );

		list_for_each_entry_safe( pJoint, pW, 
							&pAdaptor->a_Events, j_Head.j_Wait ) {

			if( pJoint->j_pArg != pPath )	continue;

			JOINT_FREE( pJoint );

			Ret = 0;
			break;
		}
	}
	pRpl->ed_Head.h_Error	= errno;
	PaxosAcceptPut( pE );
err1:
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
				pRpl, sizeof(*pRpl), pRpl->ed_Head.h_Head.h_Len );

	RETURN( Ret );
}

int
PFSEventLockS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSEventLockReq_t *pReq = (PFSEventLockReq_t*)pH;
	PFSEventLockRpl_t *pRpl;
	int	Ret;
	SessionAdaptor_t*	pAdaptor;
	PFSJoint_t	*pJoint, *pW;
	PFSLock_t*	pLock;

	DBG_ENT();

	struct PaxosAccept	*pE;	
	PaxosSessionHead_t	Head;

	pRpl	= (PFSEventLockRpl_t*)MSGALLOC( PFS_EVENT_LOCK, 
									0, sizeof(*pRpl), PFSMalloc );	

	Head.h_Id	= pReq->el_Id;
	pE	= PaxosAcceptGet( CNTRL.c_pAcceptor, &Head.h_Id, FALSE );
	if( !pE ) {
		pRpl->el_Head.h_Error	= ENOENT;
		Ret	= -1;
		goto err1;
	}
	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pE, PFS_FAMILY );

	pLock = (PFSLock_t*)DHashListGet( &CNTRL.c_Lock, (void*)pReq->el_Name );
	if( pReq->el_Set ) {
		if( !pLock ) {
			pLock = LOCK_ALLOC( pReq->el_Name );
		}
		/* set Wait */
		PFSJointType_t	Type;

		if( pReq->el_bRecall )	Type	= JOINT_EVENT_LOCK_RECALL;
		else					Type	= JOINT_EVENT_LOCK;

		pJoint 		= JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, 
								Type, &pLock->l_AdaptorEvent,
								pfsLockGarbage, pLock );
		ASSERT( pJoint );
		Ret = 0;
	} else {
		/* cancel Wait */

		Ret = -1;
		ASSERT( pLock );

		list_for_each_entry_safe( pJoint, pW, 
						&pAdaptor->a_Events, j_Head.j_Wait ) {

			if( pJoint->j_pArg != pLock )	continue;

			JOINT_FREE( pJoint );
			Ret = 0;
			break;
		}
	}
	pRpl->el_Head.h_Error	= Ret;
	PaxosAcceptPut( pE );
err1:
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
						pRpl, sizeof(*pRpl), sizeof(*pRpl) );

	RETURN( Ret );
}

int
queueGarbage( void* pVoid )
{
	PFSQueue_t	*pQueue = (PFSQueue_t*)pVoid;

	if( list_empty( &pQueue->q_AdaptorMsg )
		&& list_empty( &pQueue->q_AdaptorWait )
		&& list_empty( &pQueue->q_AdaptorEvent ) ) {

		QUEUE_FREE( pQueue );
		return( 0 );
	} else {
		return( -1 );
	}
}

int
PFSEventQueueS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSEventQueueReq_t *pReq = (PFSEventQueueReq_t*)pH;
	PFSEventQueueRpl_t *pRpl;
	int	Ret;
	SessionAdaptor_t*	pAdaptor;
	PFSJoint_t	*pJoint, *pW;
	PFSQueue_t*	pQueue;
	Msg_t		*pMsgRpl;
	MsgVec_t	Vec;

	DBG_ENT();

	struct PaxosAccept	*pE;	
	PaxosSessionHead_t	Head;

	Head.h_Id	= pReq->eq_Id;
	pE	= PaxosAcceptGet( CNTRL.c_pAcceptor, &Head.h_Id, FALSE );
	if( !pE ) {
		Ret	= ENOENT;
		goto err1;
	}
	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pE, PFS_FAMILY );

	pQueue = (PFSQueue_t*)DHashListGet( &CNTRL.c_Queue, (void*)pReq->eq_Name );
	if( pReq->eq_Set ) {
		if( !pQueue ) {
			pQueue = QUEUE_ALLOC( pReq->eq_Name );
		}
		/* set Wait */
		pJoint 		= JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, 
								JOINT_EVENT_QUEUE, &pQueue->q_AdaptorEvent,
								queueGarbage, pQueue );
		ASSERT( pJoint );
		Ret = 0;
	} else {
		/* cancel Wait */

		Ret = -1;
		ASSERT( pQueue );

		list_for_each_entry_safe( pJoint, pW, 
						&pAdaptor->a_Events, j_Head.j_Wait ) {

			if( pJoint->j_pArg != pQueue )	continue;

			JOINT_FREE( pJoint );
			Ret = 0;
			break;
		}
	}
	PaxosAcceptPut( pAccept );
err1:
	pMsgRpl = MsgCreate( 1, PaxosAcceptGetfMalloc(pAccept),
								PaxosAcceptGetfFree(pAccept) );

	pRpl	= (PFSEventQueueRpl_t*)MsgMalloc( pMsgRpl, sizeof(*pRpl) );
	MSGINIT( pRpl, PFS_EVENT_QUEUE, 0, sizeof(*pRpl) );
	pRpl->eq_Head.h_Error	= Ret;

	MsgVecSet( &Vec, VEC_MINE, pRpl, sizeof(*pRpl), sizeof(*pRpl) );
	MsgAdd( pMsgRpl, &Vec );
	
	Ret = PaxosAcceptReplyByMsg( pAccept, pMsgRpl );

	RETURN( Ret );
}

int
pfsPeekReply( PaxosAccept_t *pAccept, PFSQueue_t *pQueue )
{
	Msg_t		*pMsgPostReq, *pMsgPeekRpl;
	MsgVec_t	Vec;
	PFSPostMsgReq_t	*pPostReq;
	PFSPeekMsgRpl_t	*pPeekRpl;

	pMsgPostReq	= list_first_entry( &pQueue->q_AdaptorMsg, Msg_t, m_Lnk );

	ASSERT( pMsgPostReq );

	pMsgPeekRpl	= MsgClone( pMsgPostReq );

	pPostReq	= (PFSPostMsgReq_t*)MsgFirst( pMsgPeekRpl );
	PaxosAcceptor_t	*pAcceptor;
	pAcceptor	= PaxosAcceptGetAcceptor( pAccept );
	if( pAcceptor->c_bCksum ) {
		ASSERT( !in_cksum64( pPostReq, pPostReq->p_Head.h_Head.h_Len, 0 ) );
	}
	MsgTrim( pMsgPeekRpl, sizeof(*pPostReq) );

	pPeekRpl = (PFSPeekMsgRpl_t*)MsgMalloc( pMsgPeekRpl, sizeof(*pPeekRpl) );
	MSGINIT( pPeekRpl, PFS_PEEK_MSG, 0, sizeof(*pPeekRpl) + pPostReq->p_Len );
	pPeekRpl->p_Len	= pPostReq->p_Len;
	MsgVecSet( &Vec, VEC_MINE, pPeekRpl, sizeof(*pPeekRpl), sizeof(*pPeekRpl) );
	MsgInsert( pMsgPeekRpl, &Vec );

	pQueue->q_Peek	= TRUE;

	PaxosAcceptReplyByMsg( pAccept, pMsgPeekRpl );
	return( 0 );
}

int
PFSPostS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSPostMsgReq_t 	*pPostReq = (PFSPostMsgReq_t*)pH;
	PFSQueue_t		*pQueue;
	PFSPostMsgRpl_t	*pPostRpl;
	SessionAdaptor_t*	pAdaptor;
	Msg_t			*pMsgPostReq, *pMsgPostRpl;
	MsgVec_t	Vec;
	int	Ret = 0;

	pQueue = (PFSQueue_t*)DHashListGet(&CNTRL.c_Queue,(void*)pPostReq->p_Name);
	if( !pQueue ) {
		pQueue	= QUEUE_ALLOC( pPostReq->p_Name );
	}
	ASSERT( pQueue );

	pMsgPostRpl = MsgCreate( 1, PaxosAcceptGetfMalloc(pAccept),
								PaxosAcceptGetfFree(pAccept) );
	pPostRpl	= (PFSPostMsgRpl_t*)MsgMalloc( pMsgPostRpl, sizeof(*pPostRpl) );
	MSGINIT( pPostRpl, PFS_POST_MSG, 0, sizeof(*pPostRpl) );

	MsgVecSet( &Vec, VEC_MINE, pPostRpl, sizeof(*pPostRpl), sizeof(*pPostRpl) );
	MsgAdd( pMsgPostRpl, &Vec );

	if( pQueue->q_CntMsg > QUEUE_POST_MAX ) {

		pPostRpl->p_Head.h_Error	= EPIPE;

		PaxosAcceptReplyByMsg( pAccept, pMsgPostRpl );
		
		PaxosAcceptGetfFree(pAccept)( pH );

		return( Ret );
	}
	/* Hold */
	pMsgPostReq = MsgCreate( 2, PaxosAcceptGetfMalloc(pAccept),
								PaxosAcceptGetfFree(pAccept) );

	MsgVecSet( &Vec, VEC_MINE, pH, pH->h_Head.h_Len, pH->h_Head.h_Len );
	MsgAdd( pMsgPostReq, &Vec );

	list_add_tail( &pMsgPostReq->m_Lnk, &pQueue->q_AdaptorMsg );
	pQueue->q_CntMsg++;

	pAdaptor	= list_first_entry( &pQueue->q_AdaptorWait,
									SessionAdaptor_t, a_QueueWait );

	if( !pQueue->q_Peek && pAdaptor ) {	// Post to Client
		pfsPeekReply( pAdaptor->a_pAccept, pQueue );
	}

	PaxosAcceptReplyByMsg( pAccept, pMsgPostRpl );

	/*
	 *	Event
	 */
	PFSJoint_t	*pJoint;
	struct PaxosAccept	*pAcceptEvent;
	PaxosClientId_t	*pClientAccept;
	PaxosClientId_t	*pClientEvent;
	PFSEventQueue_t	*pEvent;
	SEQ_INC( pQueue->q_EventSeq );
	list_for_each_entry( pJoint, &pQueue->q_AdaptorEvent, 
									j_Head.j_Event ) {

		pAcceptEvent	= pJoint->j_pAdaptor->a_pAccept;

		pClientAccept	= PaxosAcceptorGetClientId( pAccept );
		pClientEvent	= PaxosAcceptorGetClientId( pAcceptEvent );

		if( !HoldHashCmp( pClientAccept, pClientEvent ) ) {
			continue;
		}
		pEvent	= (PFSEventQueue_t*)MSGALLOC( PFS_EVENT_QUEUE, 0, 
											sizeof(*pEvent), PFSMalloc );	
		strncpy( pEvent->eq_Name, pQueue->q_Name, sizeof(pEvent->eq_Name));
		pEvent->eq_PostWait	= 0;
		pEvent->eq_Id		= *pClientAccept;
		pEvent->eq_Time		= time(0);
		pEvent->eq_EventSeq	= pQueue->q_EventSeq;

		Ret = PaxosAcceptEventRaise( pAcceptEvent, 
						(PaxosSessionHead_t*)pEvent );
	}
	return( Ret );
}

int
PFSPeekS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSPeekMsgReq_t *pReq = (PFSPeekMsgReq_t*)pH;
	PFSQueue_t	*pQueue;
	SessionAdaptor_t*	pAdaptor;

	pQueue = (PFSQueue_t*)DHashListGet(&CNTRL.c_Queue,(void*)pReq->p_Name);
	if( !pQueue ) {
		pQueue	= QUEUE_ALLOC( pReq->p_Name );
	}
	ASSERT( pQueue );

	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, 
															PFS_FAMILY );
	list_add_tail( &pAdaptor->a_QueueWait, &pQueue->q_AdaptorWait );
	pAdaptor->a_pQueueWait	= pQueue;

	if( !pQueue->q_Peek && pQueue->q_CntMsg > 0 ) {	// Reply
		pfsPeekReply( pAccept, pQueue );
	}
	/*
	 *	Event
	 */
	PFSJoint_t	*pJoint;
	struct PaxosAccept	*pAcceptEvent;
	PaxosClientId_t	*pClientAccept;
	PaxosClientId_t	*pClientEvent;
	PFSEventQueue_t	*pEvent;

	SEQ_INC( pQueue->q_EventSeq );
	list_for_each_entry( pJoint, &pQueue->q_AdaptorEvent, 
									j_Head.j_Event ) {

		pAcceptEvent	= pJoint->j_pAdaptor->a_pAccept;

		pClientAccept	= PaxosAcceptorGetClientId( pAccept );
		pClientEvent	= PaxosAcceptorGetClientId( pAcceptEvent );

		if( !HoldHashCmp( pClientAccept, pClientEvent ) ) {
			continue;
		}
		pEvent	= (PFSEventQueue_t*)MSGALLOC( PFS_EVENT_QUEUE, 0, 
											sizeof(*pEvent), PFSMalloc );	
		strncpy( pEvent->eq_Name, pQueue->q_Name, sizeof(pEvent->eq_Name));
		pEvent->eq_PostWait	= 1;
		pEvent->eq_Id		= *pClientAccept;
		pEvent->eq_EventSeq	= pQueue->q_EventSeq;

		PaxosAcceptEventRaise( pAcceptEvent, (PaxosSessionHead_t*)pEvent );
	}
	queueGarbage( pQueue );

	return( 0 );
}

int
pfsPeekAck( SessionAdaptor_t *pAdaptor, bool_t bRemove )
{
	Msg_t	*pMsg;
	PFSQueue_t	*pQueue;

	/* unlink my Adaptor */
	list_del_init( &pAdaptor->a_QueueWait );
	pQueue	= pAdaptor->a_pQueueWait;
	pAdaptor->a_pQueueWait	= NULL;

	/* remove message */
	if( bRemove ) {
		pMsg	= list_first_entry( &pQueue->q_AdaptorMsg, Msg_t, m_Lnk );
		list_del_init( &pMsg->m_Lnk );
		pQueue->q_CntMsg--;
		MsgDestroy( pMsg );
	}

	ASSERT( pQueue->q_Peek );

	pQueue->q_Peek	= FALSE;

	/* kick next one */
	if( !list_empty( &pQueue->q_AdaptorMsg )
		&& !list_empty( &pQueue->q_AdaptorWait ) ) {

		pAdaptor	= list_first_entry( &pQueue->q_AdaptorWait,
									SessionAdaptor_t, a_QueueWait );
		pfsPeekReply( pAdaptor->a_pAccept, pQueue );
	} else {
		queueGarbage( pQueue );
	}
	return( 0 );
}

int
PFSPeekAckS( struct PaxosAccept* pAccept, PFSHead_t* pH )
{
	PFSPeekAckReq_t	*pReq = (PFSPeekAckReq_t*)pH;
	PFSPeekAckRpl_t	*pRpl;
	PFSQueue_t	*pQueue;
	Msg_t		*pMsgRpl;
	MsgVec_t	Vec;
	SessionAdaptor_t	*pAdaptor;

	pMsgRpl = MsgCreate( 1, PaxosAcceptGetfMalloc(pAccept),
								PaxosAcceptGetfFree(pAccept) );
	pRpl	= (PFSPeekAckRpl_t*)MsgMalloc( pMsgRpl, sizeof(*pRpl) );
	MSGINIT( pRpl, PFS_PEEK_ACK, 0, sizeof(*pRpl) );

	MsgVecSet( &Vec, VEC_MINE, pRpl, sizeof(*pRpl), sizeof(*pRpl) );
	MsgAdd( pMsgRpl, &Vec );

	pQueue = (PFSQueue_t*)DHashListGet(&CNTRL.c_Queue, (void*)pReq->a_Name);
	if( !pQueue ) {
		pRpl->a_Head.h_Error	= ENOENT;
	} else {
		pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, 
														PFS_FAMILY );
		pfsPeekAck( pAdaptor, TRUE );
	}
	PaxosAcceptReplyByMsg( pAccept, pMsgRpl );
	return( 0 );
}

int
PFSRejected( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM )
{
	PFSHead_t*	pH = (PFSHead_t*)pM;
	PFSHead_t	Rpl;
	struct PaxosAccept*	pAccept;
	int	Ret;

	DBG_ENT();

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept )	RETURN( -1 );

	MSGINIT( &Rpl, pH->h_Head.h_Cmd, PAXOS_REJECTED,sizeof(Rpl) );	
	Rpl.h_Head.h_Master	= PaxosGetMaster(PaxosAcceptorGetPaxos(pAcceptor));

	LOG( pALOG, LogERR, "SEND REJECT pAccept=%p", pAccept );

	Ret = PaxosAcceptSend( pAccept, (PaxosSessionHead_t*)&Rpl );
	PaxosAcceptPut( pAccept );

	RETURN( Ret );
}

Msg_t*
PFSBackupPrepare(int num )
{
	Msg_t	*pMsgList;

	pMsgList = MsgCreate( ST_MSG_SIZE, Malloc, Free );

	STFileList( PFS_PFS, pMsgList );
	if( num > 0 ) {
		Hash_t		*pTree;
		IOFile_t	*pF;
		char		Path[PATH_NAME_MAX];
		StatusEnt_t	*pEnt;

		pTree	= STTreeCreate( pMsgList );

		pF = IOFileCreate( PFS_SNAP_UPD_LIST, O_RDONLY, 0, Malloc, Free );
		while( fgets( Path, sizeof(Path), pF->f_pFILE ) ) {
			if( Path[strlen(Path)-1] == '\n' )	Path[strlen(Path)-1] = 0;
			pEnt = STTreeFindByPath( Path + strlen(PFS_PFS)+1, pTree );
			//ASSERT( pEnt );
			if ( pEnt )
				pEnt->e_Type	|= ST_NON;
		}
		IOFileDestroy( pF, TRUE );

		STTreeDestroy( pTree );
	} else {	
		int	fd;

		fd = open( PFS_SNAP_UPD_LIST, O_RDWR|O_TRUNC|O_CREAT, 0666 );
		close( fd );
	}

	return( pMsgList );
}

int
PFSSessionShutdown( PaxosAcceptor_t *pAcceptor, 
				PaxosSessionShutdownReq_t *pReq, uint32_t nextDecide )
{
	int	Fd;
	char	buff[1024];

	FileCachePurgeAll( pBC );

	if( !strcmp("ABORT", pReq->s_Comment ) ) {
		ABORT();
	}
	Fd = creat( FILE_SHUTDOWN, 0666 );
	if( Fd < 0 )	goto err;

	LOG( PaxosAcceptorGetLog(pAcceptor), LogINF, 
		"nextDecide=%u comment[%s]", nextDecide, pReq->s_Comment ); 

	sprintf( buff, "%u [%s]\n", nextDecide, pReq->s_Comment );
	write( Fd, buff, strlen(buff) );

	close( Fd );
	return( 0 );
err:
	return( -1 );
}

static int	AgainBackup;

int
PFSBackup( PaxosSessionHead_t *pM )
{
	FileCacheFlushAll( pBC );

	Msg_t	*pMsgNew, *pMsgOld, *pMsgDel, *pMsgUpd;
	StatusEnt_t	*pS, *pSEnd;
	MsgVec_t	Vec;
	IOFile_t	*pF;
	PathEnt_t	*pP;
	int			i;
	char	Path[FILENAME_MAX];
	int	cnt;
	time_t	Alive;
	timespec_t	AliveSpec;
	size_t		Size;
	int	total;

	AliveSpec = PaxosGetAliveTime(PaxosAcceptorGetPaxos(CNTRL.c_pAcceptor), 
					pM->h_From );
	Alive	= AliveSpec.tv_sec;
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
		"From=%d Alive=%llu", pM->h_From, Alive );

	pMsgOld = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgNew = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgDel = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pMsgUpd = MsgCreate( ST_MSG_SIZE, Malloc, Free );

	if( pM->h_Len > sizeof(PaxosSessionHead_t) ) {
		pSEnd	= (StatusEnt_t*)((char*)pM + pM->h_Len);
		pS		= (StatusEnt_t*)(pM+1);
		while( pS != pSEnd ) {
			MsgVecSet( &Vec, 0, pS, sizeof(*pS), sizeof(*pS) );
			MsgAdd( pMsgOld, &Vec );
			pS++;
		}
	}
	STFileList( PFS_PFS, pMsgNew );
	STDiffList( pMsgOld, pMsgNew, Alive, pMsgDel, pMsgUpd );

	pF = IOFileCreate( PFS_SNAP_DEL_LIST, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										Malloc, Free );
	for( i = 0; i < pMsgDel->m_N; i++ ) {
		IOWrite( (IOReadWrite_t*)pF, &pMsgDel->m_aVec[i].v_Len,
										sizeof(pMsgDel->m_aVec[i].v_Len) );
		IOWrite( (IOReadWrite_t*)pF, pMsgDel->m_aVec[i].v_pStart,
										pMsgDel->m_aVec[i].v_Len );
	}
	IOFileDestroy( pF, TRUE );

	pF = IOFileCreate( PFS_SNAP_UPD_LIST, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										Malloc, Free );
	for( cnt = 0, total = 0, Size = 0, pP = (PathEnt_t*)MsgFirst(pMsgUpd); 
				pP; pP = ( PathEnt_t*)MsgNext(pMsgUpd) ) {

		if( Size < TAR_SIZE_MAX ) {
			sprintf( Path, "%s/%s\n", PFS_PFS, (char*)(pP+1) );
			IOWrite( (IOReadWrite_t*)pF, Path, strlen(Path) );
			Size	+= pP->p_Size;
			cnt++;
		}
		total++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
	"UPD[%d/%d]", cnt, total );

	AgainBackup	= total - cnt;

	IOFileDestroy( pF, TRUE );

	MsgDestroy( pMsgOld );
	MsgDestroy( pMsgNew );
	MsgDestroy( pMsgDel );
	MsgDestroy( pMsgUpd );

	char	command[128];
	int		status;

	unlink( PFS_SNAP_TAR );
	sprintf( command, "tar -cf %s -T %s", PFS_SNAP_TAR, PFS_SNAP_UPD_LIST );

	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
	"TAR BEFORE[%s]", command );

	status = system( command );

	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF, "TAR AFTER" );
ASSERT( !status );
	return( 0 );
}

int
PFSRestore()
{
	IOFile_t	*pF;
	PathEnt_t	*pP;
	Msg_t	*pMsgDel;
	MsgVec_t	Vec;
	int		i;
	int		Len;
	char	Path[FILENAME_MAX];
	int		cnt;

	pMsgDel = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pF = IOFileCreate( PFS_SNAP_DEL_LIST, O_RDONLY, 0, Malloc, Free );
	if( !pF ) {
		MsgDestroy( pMsgDel );
		return( -1 );
	}
	while( IORead( (IOReadWrite_t*)pF, &Len, sizeof(Len) ) > 0 ) {
		pP 	= (PathEnt_t*)MsgMalloc( pMsgDel, Len );
		IORead( (IOReadWrite_t*)pF, pP, Len );
		MsgVecSet(&Vec, VEC_MINE, pP, Len, Len);
		MsgAdd( pMsgDel, &Vec );
	}
	for( cnt = 0, i = pMsgDel->m_N - 1; i >= 0; i-- ) {
		pP	= (PathEnt_t*)pMsgDel->m_aVec[i].v_pStart;
		sprintf( Path, "%s/%s", PFS_PFS, (char*)(pP+1) );

		if( pP->p_Type & ST_DIR )	rmdir( Path );
		else						unlink( Path );
		cnt++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF, "DEL[%d]", cnt );

	IOFileDestroy( pF, TRUE );
	MsgDestroy( pMsgDel );

	char	command[128];
	int		status;

	sprintf( command, "tar -xf %s", PFS_SNAP_TAR );
	status = system( command );
ASSERT( !status );
	return( 0 );
}

int
PFSTransferSend( IOReadWrite_t *pW )
{
	struct stat	Stat;
	int	Ret;
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];

	Ret = stat( PFS_SNAP_DEL_LIST, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( PFS_SNAP_DEL_LIST, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );

	Ret = stat( PFS_SNAP_UPD_LIST, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( PFS_SNAP_UPD_LIST, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );

	Ret = stat( PFS_SNAP_TAR, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( PFS_SNAP_TAR, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );
	/*
	 *	Again ?
	 */
	IOWrite( pW, &AgainBackup, sizeof(AgainBackup ) );

	return( 0 );
}

int
PFSTransferRecv( IOReadWrite_t *pR )
{
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];
	int		Ret;

	IORead( pR, &Size, sizeof(Size) );
	fd = open( PFS_SNAP_DEL_LIST, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( PFS_SNAP_UPD_LIST, O_WRONLY|O_APPEND, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( PFS_SNAP_TAR, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	/*
	 *	Incremental backup
	 */
	IORead( pR, &Ret, sizeof(Ret) );
	return( Ret );
}

#define	SNAP_LOCK						1
#define	SNAP_PATH						2
#define	SNAP_QUEUE						3	
#define	SNAP_SESSION					4
#define	SNAP_ADAPTOR					5
#define	SNAP_ADAPTOR_LOCK_WAIT			6
#define	SNAP_ADAPTOR_LOCK_HOLD			7
#define	SNAP_ADAPTOR_EPHEMERAL			8
#define	SNAP_ADAPTOR_EVENT_PATH			9
#define	SNAP_ADAPTOR_EVENT_LOCK			10
#define	SNAP_ADAPTOR_EVENT_LOCK_RECALL	11
#define	SNAP_ADAPTOR_QUEUE_WAIT			12
#define	SNAP_ADAPTOR_EVENT_QUEUE		13
#define	SNAP_ADAPTOR_END				-1

int
PFSGlobalMarshal( IOReadWrite_t *pW )
{
	DBG_ENT();

	/*
	 *	Status
	 */
	PFSLock_t*	pLock;
	PFSPath_t*	pPath;
	PFSQueue_t*	pQueue;
	Msg_t*	pMsg;
	int	i;
	int32_t	tag;

	list_for_each_entry( pLock, &CNTRL.c_Lock.hl_Lnk, l_Lnk ) {
		tag	= SNAP_LOCK;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pLock, sizeof(*pLock) );
	}
	list_for_each_entry( pPath, &CNTRL.c_Path.hl_Lnk, p_Lnk ) {
		tag	= SNAP_PATH;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pPath, sizeof(*pPath) );
	}
	list_for_each_entry( pQueue, &CNTRL.c_Queue.hl_Lnk, q_Lnk ) {
		tag	= SNAP_QUEUE;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pQueue, sizeof(*pQueue) );
		if( pQueue->q_CntMsg > 0 ) {
			list_for_each_entry( pMsg, &pQueue->q_AdaptorMsg, m_Lnk ) {
				IOMarshal( pW, pMsg, sizeof(*pMsg) );
				for( i = 0; i < pMsg->m_N; i++ ) {
					IOMarshal( pW, &pMsg->m_aVec[i], 
										sizeof(pMsg->m_aVec[i]) );
					IOMarshal( pW, pMsg->m_aVec[i].v_pVoid,
										pMsg->m_aVec[i].v_Size );
				}
			}
		}
	}
	tag	= SNAP_SESSION;
	IOMarshal( pW, &tag, sizeof(tag) );

	RETURN( 0 );
}

int
PFSGlobalUnmarshal( IOReadWrite_t *pR )
{
	DBG_ENT();

	/*
	 * Status
	 */
	int32_t	tag;
	PFSLock_t	Lock;
	PFSPath_t	Path;
	PFSQueue_t	Queue;
	PFSQueue_t*	pQueue;
	Msg_t	Msg, *pMsg;
	MsgVec_t	Vec;
	int	i, j;
	char	*pBuf;
	int	Ret = 0;
	
	while( IOUnmarshal( pR, &tag, sizeof(tag) ) > 0 ) {
		switch( tag ) {
		default:	DBG_PRT("tag=%d\n", tag ); Ret = -1; goto end;
		case SNAP_LOCK:
			IOUnmarshal( pR, &Lock, sizeof(Lock) );
			LOCK_ALLOC( Lock.l_Name );
			DBG_TRC("SNAP_LOCK[%s]\n", Lock.l_Name );
			break;
		case SNAP_PATH:
			IOUnmarshal( pR, &Path, sizeof(Path) );
			PATH_ALLOC( Path.p_Name );
			DBG_TRC("SNAP_PATH[%s]\n", Path.p_Name );
			break;
		case SNAP_QUEUE:
			IOUnmarshal( pR, &Queue, sizeof(Queue) );
			pQueue = QUEUE_ALLOC( Queue.q_Name );
			pQueue->q_Peek = Queue.q_Peek;
			if( Queue.q_CntMsg > 0 ) {
				for( j = 0; j < Queue.q_CntMsg; j++ ) {
					IOUnmarshal( pR, &Msg, sizeof(Msg) );
					pMsg	= MsgCreate( Msg.m_Size,
								PaxosAcceptorGetfMalloc(CNTRL.c_pAcceptor),
								PaxosAcceptorGetfFree(CNTRL.c_pAcceptor) );
					
					for( i = 0; i < Msg.m_N; i++ ) {
						IOUnmarshal( pR, &Vec, sizeof(Vec) );
						pBuf = MsgMalloc( pMsg, Vec.v_Size );
						IOUnmarshal( pR, pBuf, Vec.v_Size );
						Vec.v_pStart =
							pBuf + (Vec.v_pStart - (char*)Vec.v_pVoid);
						Vec.v_pVoid	= pBuf;
						Vec.v_Flags	|= VEC_MINE;
						MsgAdd( pMsg, &Vec );
					}
					list_add_tail( &pMsg->m_Lnk, &pQueue->q_AdaptorMsg ); 
					pQueue->q_CntMsg++;
				}
			}
			DBG_TRC("SNAP_QUEUE[%s] Queue.q_Cnt=%d pQueue->q_Cnt=%d\n", 
					Queue.q_Name, Queue.q_Cnt, pQueue->q_Cnt );
			break;
		case SNAP_SESSION:
			goto end;
		}
	}
	Ret = -1;
end:
	RETURN( Ret );
}

int
PFSAdaptorMarshal( IOReadWrite_t *pW, struct PaxosAccept* pAccept )
{
	int32_t	tag;
	SessionAdaptor_t*	pAdaptor;
	PFSLock_t*	pLock;
	PFSJoint_t*	pJoint;
	Ephemeral_t*	pEphemeral;
	PFSPath_t*		pPath;
	PFSQueue_t*		pQueue;


	pAdaptor	= (SessionAdaptor_t*)PaxosAcceptGetTag( pAccept, PFS_FAMILY );
	if( pAdaptor ) {
		tag	= SNAP_ADAPTOR;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pAdaptor, sizeof(*pAdaptor) );
	} else {
//		DBG_PRT("!!!!!!!!!! NO ADAPTOR pAccept=%p\n", pAccept );
//		PaxosAcceptDump( pAccept );
		goto end;
	}

	pLock	= pAdaptor->a_pLockWait;

	if( pLock ) {
		tag	= SNAP_ADAPTOR_LOCK_WAIT;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pLock->l_Name, sizeof(pLock->l_Name) );
		LOG( pALOG, LogINF, "SNAP_ADAPTOR_LOCK_WAIT[%s] Pid=%u", 
			pLock->l_Name, PaxosAcceptorGetClientId(pAccept)->i_Pid );
	}

	list_for_each_entry( pJoint, &pAdaptor->a_LockHold, j_Head.j_Wait ) {
		pLock	= (PFSLock_t*)pJoint->j_pArg;
		tag	= SNAP_ADAPTOR_LOCK_HOLD;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, &pJoint->j_Type, sizeof(pJoint->j_Type) );
		IOMarshal( pW, pLock->l_Name, sizeof(pLock->l_Name) );
	}
	list_for_each_entry( pEphemeral, &pAdaptor->a_Ephemeral.hl_Lnk, e_Lnk ) {
		tag	= SNAP_ADAPTOR_EPHEMERAL;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pEphemeral->e_Name, sizeof(pEphemeral->e_Name) );
		DBG_TRC("SNAP_ADAPTOR_EPHEMERAL[%s]\n", pEphemeral->e_Name );
	}
	list_for_each_entry( pJoint, &pAdaptor->a_Events, j_Head.j_Wait ) {
		switch( (int)pJoint->j_Type ) {
		case JOINT_EVENT_PATH:
			pPath	= (PFSPath_t*)pJoint->j_pArg;
			tag = SNAP_ADAPTOR_EVENT_PATH;
			IOMarshal( pW, &tag, sizeof(tag) );
			IOMarshal( pW, pPath->p_Name, sizeof(pPath->p_Name) );	
		DBG_TRC("SNAP_ADAPTOR_EVENT_PATH[%s]\n", pPath->p_Name );
			break;
		case JOINT_EVENT_LOCK:
			pLock	= (PFSLock_t*)pJoint->j_pArg;
			tag = SNAP_ADAPTOR_EVENT_LOCK;
			IOMarshal( pW, &tag, sizeof(tag) );
			IOMarshal( pW, pLock->l_Name, sizeof(pLock->l_Name) );	
		DBG_TRC("SNAP_ADAPTOR_EVENT_LOCK[%s]\n", pLock->l_Name );
			break;
		case JOINT_EVENT_LOCK_RECALL:
			pLock	= (PFSLock_t*)pJoint->j_pArg;
			tag = SNAP_ADAPTOR_EVENT_LOCK_RECALL;
			IOMarshal( pW, &tag, sizeof(tag) );
			IOMarshal( pW, pLock->l_Name, sizeof(pLock->l_Name) );	
		DBG_TRC("SNAP_ADAPTOR_EVENT_LOCK[%s]\n", pLock->l_Name );
			break;
		case JOINT_EVENT_QUEUE:
			pQueue	= (PFSQueue_t*)pJoint->j_pArg;
			tag = SNAP_ADAPTOR_EVENT_QUEUE;
			IOMarshal( pW, &tag, sizeof(tag) );
			IOMarshal( pW, pQueue->q_Name, sizeof(pQueue->q_Name) );	
		DBG_TRC("SNAP_ADAPTOR_EVENT_QUEUE[%s]\n", pQueue->q_Name );
			break;
		}
	}
	pQueue	= pAdaptor->a_pQueueWait;
	if( pQueue ) {
		tag	= SNAP_ADAPTOR_QUEUE_WAIT;
		IOMarshal( pW, &tag, sizeof(tag) );
		IOMarshal( pW, pQueue->q_Name, sizeof(pQueue->q_Name) );
		LOG( pALOG, LogINF, "SNAP_ADAPTOR_QUEUE_WAIT[%s] Pid=%u", 
			pQueue->q_Name, PaxosAcceptorGetClientId(pAccept)->i_Pid );
	}

end:
	tag	= SNAP_ADAPTOR_END;
	IOMarshal( pW, &tag, sizeof(tag) );
	return( 0 );
}

int
PFSAdaptorUnmarshal( IOReadWrite_t *pR, struct PaxosAccept* pAccept )
{
	int32_t	tag;
	SessionAdaptor_t*	pAdaptor;
	PFSLock_t*	pLock;
	PFSJoint_t*	pJoint;
	PFSJointType_t	Type;
	char	LockName[PATH_NAME_MAX];
	char	EphemeralName[PATH_NAME_MAX];
	PFSPath_t	*pPath;
	char	PathName[PATH_NAME_MAX];
	char	QueueName[PATH_NAME_MAX];
	PFSQueue_t	*pQueue;

	while( IOUnmarshal( pR, &tag, sizeof(tag) ) > 0 ) {
		DBG_TRC("tag=%d\n", tag );
		switch( tag ) {
		default:	DBG_TRC("tag=%d\n", tag ); break;
		case SNAP_ADAPTOR_END:	goto end;
		case SNAP_ADAPTOR:
			pAdaptor = (SessionAdaptor_t*)PFSMalloc( sizeof(*pAdaptor) );

			IOUnmarshal( pR, (char*)pAdaptor, sizeof(*pAdaptor) );

			list_init( &pAdaptor->a_LockWait );
			pAdaptor->a_pLockWait	= NULL;
			list_init( &pAdaptor->a_LockHold );

			list_init( &pAdaptor->a_QueueWait );
			pAdaptor->a_pQueueWait	= NULL;

			HashListInit( &pAdaptor->a_Ephemeral, PRIMARY_101, 
					HASH_CODE_STR, HASH_CMP_STR, NULL, 
					PaxosAcceptGetfMalloc( pAccept ), 
					PaxosAcceptGetfFree( pAccept ), NULL );
			list_init( &pAdaptor->a_Events );
			pAdaptor->a_pAccept	= pAccept;
			PaxosAcceptSetTag( pAccept, PFS_FAMILY, pAdaptor );
			break;
		case SNAP_ADAPTOR_LOCK_WAIT:
			IOUnmarshal( pR, LockName, sizeof(LockName) );
DBG_TRC("LOCK_WAIT LockName[%s] LockRead=%d Pid=%u\n",
LockName, pAdaptor->a_LockRead, PaxosAcceptorGetClientId(pAccept)->i_Pid );
			pLock = (PFSLock_t*)DHashListGet(&CNTRL.c_Lock,(void*)LockName);

			ASSERT( pLock );

			list_add( &pAdaptor->a_LockWait, &pLock->l_AdaptorWait );	
			pAdaptor->a_pLockWait	= pLock;
			break;
		case SNAP_ADAPTOR_LOCK_HOLD:
			IOUnmarshal( pR, &Type, sizeof(Type) );
			IOUnmarshal( pR, LockName, sizeof(LockName) );
DBG_TRC("LOCK_HOLD LockName[%s] Type=%d Pid=%u\n", 
LockName, Type, PaxosAcceptorGetClientId(pAccept)->i_Pid );
			pLock = (PFSLock_t*)DHashListGet(&CNTRL.c_Lock,(void*)LockName);

			ASSERT( pLock );

			if( Type == JOINT_LOCK_READ_HOLD )	pLock->l_Cnt++;
			else								pLock->l_Cnt--;

			pJoint = JOINT_ALLOC( pAdaptor, &pAdaptor->a_LockHold, Type,
					&pLock->l_AdaptorHold, pfsLockGarbage, pLock );
			HashPut( &pLock->l_HoldHash, 
					PaxosAcceptorGetClientId(pAccept), pJoint );
			break;
		case SNAP_ADAPTOR_EPHEMERAL:
			IOUnmarshal( pR, EphemeralName, sizeof(EphemeralName) );
DBG_TRC("EphemeralName[%s]\n", EphemeralName );
			EPHEMERAL_ALLOC( pAdaptor, EphemeralName );
			pPath = PATH_GET( EphemeralName );

			ASSERT( pPath );

			pPath->p_RefCnt++;
			break;
		case SNAP_ADAPTOR_EVENT_PATH:
			IOUnmarshal( pR, PathName, sizeof(PathName) );
			pPath = PATH_GET( PathName );

			ASSERT( pPath );

			JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, JOINT_EVENT_PATH,
					&pPath->p_AdaptorEvent, pfsPathGarbage, pPath );
			break;
		case SNAP_ADAPTOR_EVENT_LOCK:
			IOUnmarshal( pR, LockName, sizeof(LockName) );
			pLock = (PFSLock_t*)DHashListGet(&CNTRL.c_Lock, (void*)LockName );

			ASSERT( pLock );

			JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, JOINT_EVENT_LOCK,
					&pLock->l_AdaptorEvent, pfsLockGarbage, pLock );
			break;
		case SNAP_ADAPTOR_EVENT_LOCK_RECALL:
			IOUnmarshal( pR, LockName, sizeof(LockName) );
			pLock = (PFSLock_t*)DHashListGet(&CNTRL.c_Lock, (void*)LockName );

			ASSERT( pLock );

			JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, JOINT_EVENT_LOCK_RECALL,
					&pLock->l_AdaptorEvent, pfsLockGarbage, pLock );
			break;

		case SNAP_ADAPTOR_EVENT_QUEUE:
			IOUnmarshal( pR, QueueName, sizeof(QueueName) );
			pQueue = (PFSQueue_t*)DHashListGet(&CNTRL.c_Queue,
								(void*)QueueName );
			ASSERT( pQueue );

			JOINT_ALLOC( pAdaptor, &pAdaptor->a_Events, JOINT_EVENT_QUEUE,
					&pQueue->q_AdaptorEvent, queueGarbage, pQueue );
			break;

		case SNAP_ADAPTOR_QUEUE_WAIT:
			IOUnmarshal( pR, QueueName, sizeof(QueueName) );
			pQueue = (PFSQueue_t*)DHashListGet(
							&CNTRL.c_Queue,(void*)QueueName );

			ASSERT( pQueue );

			list_add_tail( &pAdaptor->a_QueueWait, &pQueue->q_AdaptorWait );	
			pAdaptor->a_pQueueWait	= pQueue;
DBG_TRC("QUEUE_WAIT Name[%s] Pid=%u\n",
QueueName, PaxosAcceptorGetClientId(pAccept)->i_Pid );
			break;
		}
	}
end:
DBG_TRC("END\n",0);
	return( 0 );
}

int
PFSValidate( struct PaxosAcceptor* pAcceptor, uint32_t seq,
								PaxosSessionHead_t* pM, int From )
{
	int	Ret = 0;
	int	Cmd;

	Cmd	= pM->h_Cmd;

	if( ( Cmd & PAXOS_SESSION_FAMILY_MASK) != PFS_FAMILY )	return( Ret );

	switch( Cmd ) {
	case PFS_OUTOFBAND:
		Ret = PaxosAcceptorOutOfBandValidate( pAcceptor, seq, pM, From ); 
		break;
	}

	return( Ret );
}

int
PFSAccepted( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	PFSHead_t*		pFileHead = (PFSHead_t*)pM;
	int	Ret = 0;
	bool_t	bFree = TRUE;

	RwLockW( &CNTRL.c_RwLock );

DBG_TRC("!!! ACCEPTED pAccept=0x%x [pid=%d No=%d Seq=%d]\n", 
			pAccept, pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq );
	ASSERT( pAccept );

	switch( (int)pM->h_Cmd ) {
	case PFS_WRITE: 	PFSWriteS( pAccept, pFileHead ); break;
	case PFS_CREATE: 	PFSCreateS( pAccept, pFileHead ); break;
	case PFS_DELETE: 	PFSDeleteS( pAccept, pFileHead ); break;
	case PFS_TRUNCATE: 	PFSTruncateS( pAccept, pFileHead ); break;
	case PFS_COPY: 		PFSCopyS( pAccept, pFileHead ); break;
	case PFS_CONCAT: 	PFSConcatS( pAccept, pFileHead ); break;
	case PFS_LOCK: 	PFSTryLockS( pAccept, pFileHead, TRUE ); break;
	case PFS_UNLOCK: 	PFSUnlockS( pAccept, pFileHead ); break;
	case PFS_UNLOCK_SUPER: 	PFSUnlockSuperS( pAccept, pFileHead ); break;
	case PFS_OUTOFBAND:	
		PFSOutOfBandReq( pAccept,pFileHead); 
		break;
	case PFS_EVENT_DIR: 	
		PFSEventDirS( pAccept,  pFileHead ); 
		break;
	case PFS_EVENT_LOCK: 	
		PFSEventLockS( pAccept, pFileHead ); 
		break;
	case PFS_MKDIR: 	
		PFSMKdirS( pAccept, pFileHead ); 
		break;
	case PFS_RMDIR: 	
		PFSRmdirS( pAccept, pFileHead ); 
		break;
	case PFS_POST_MSG: 	
		PFSPostS( pAccept, pFileHead ); 
		bFree	= FALSE;
		break;
	case PFS_PEEK_MSG: 	
		PFSPeekS( pAccept, pFileHead ); 
		break;
	case PFS_PEEK_ACK: 	
		PFSPeekAckS( pAccept, pFileHead ); 
		break;
	case PFS_EVENT_QUEUE: 	
		PFSEventQueueS( pAccept, pFileHead ); 
		break;
	default:
		ABORT();
		break;
	}
	RwUnlock( &CNTRL.c_RwLock );

	if( bFree )	PFSFree( pFileHead );

	return( Ret );
}

int
PFSAbort( struct PaxosAcceptor* pAcceptor, uint32_t page )
{
	Printf("=== Catch-up Error(Page=%d)\n", page );
	return( 0 );
}

int
PFSRequest( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM, int fd )
{
	PFSHead_t*	pF = (PFSHead_t*)pM;
	struct PaxosAccept*	pAccept;
	int	Ret = 0;
	bool_t	bValidate = FALSE;
	bool_t	bAccepted;
	PFSLockReq_t 	*pLockReq;
	bool_t	bReq = FALSE;

	
DBG_TRC("FCmd=%d INET=%s [Pid=%d No=%d Seq=%d]CODE=%d\n", 
pF->h_Cmd, inet_ntoa(pM->h_Id.i_Addr.sin_addr), 
pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq,
pM->h_Code );
	ASSERT( (pM->h_Cmd & PAXOS_SESSION_FAMILY_MASK) == PFS_FAMILY );

	switch( (int)pM->h_Cmd ) {
	case PFS_READDIR: 	
	case PFS_READ:
	case PFS_STAT:

		pAccept = PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );

		ASSERT( pAccept );

		if( PaxosAcceptedStart( pAccept, pM ) ) {
LOG( pALOG, LogINF, 
" AcceptedStart ERROR FCmd=%d INET=%s [Pid=%d No=%d Seq=%d] CODE=%d", 
pM->h_Cmd, inet_ntoa(pM->h_Id.i_Addr.sin_addr), 
pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq,
pM->h_Code );
			goto skip;
		}

		RwLockR( &CNTRL.c_RwLock );

		switch( (int)pM->h_Cmd ) {
			case PFS_READDIR:	PFSReadDirS( pAccept, pF ); break;
			case PFS_READ:		PFSReadS( pAccept, pF ); break;
			case PFS_STAT:		PFSStatS( pAccept, pF ); break;
		}
		RwUnlock( &CNTRL.c_RwLock );
skip:
		PaxosAcceptPut( pAccept );

		break;

	default:	// Paxos投入
		if( (int)pM->h_Cmd == PFS_OUTOFBAND )	 {
			bValidate = TRUE;

pAccept = PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
ASSERT( pAccept );
bAccepted = PaxosAcceptIsAccepted( pAccept, pM );
if( bAccepted ) {
DBG_PRT("pM Pid=%d-%d-%d Try=%d Cmd=0x%x\n",
pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Reuse, pM->h_Id.i_Try, pM->h_Cmd );
}
PaxosAcceptPut( pAccept );
if( bAccepted )	return( 0 );

		}
		if ( TRUE ) {
			Ret = PaxosRequest( PaxosAcceptorGetPaxos(pAcceptor), 
								pM->h_Len, pM, bValidate );
		} else {
			pLockReq = (PFSLockReq_t*)pF;
			if( pLockReq->l_bTry ) {

				pAccept = PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );

				/* BUSYであれば応答される */
				Ret = PFSTryLockS( pAccept, pF, FALSE );

				if( Ret >= 1 ) bReq	= TRUE;

				PaxosAcceptPut( pAccept );
			} else {
				bReq	= TRUE;
			}
			if( bReq ) {
				Ret = PaxosRequest( PaxosAcceptorGetPaxos(pAcceptor), 
						pM->h_Len, pM, bValidate );
LOG( PaxosAcceptorGetLog(pAcceptor), LogINF,
"l_bRead=%d l_bTry=%d Ret=%d bReq=%d", 
pLockReq->l_bRead, pLockReq->l_bTry, Ret, bReq );
			}
		}
		break;
	}
	return( Ret );
}

int
PFSSessionLock()
{
	RwLockW( &CNTRL.c_RwLock );
	return( 0 );
}

int
PFSSessionUnlock()
{
	RwUnlock( &CNTRL.c_RwLock );
	return( 0 );
}

int
PFSSessionAny( struct PaxosAcceptor* pAcceptor, 
					PaxosSessionHead_t* pM, int fdSend )
{
	PFSHead_t*	pF = (PFSHead_t*)(pM+1);
	PFSHead_t	Rpl;
	int	Ret;

	PFSReadReq_t 	*pReq;
	PFSReadRpl_t	*pRpl;

	PFSCntrlRpl_t	*pRplCntrl;

	PFSDumpReq_t	*pReqDump;
	PFSDumpRpl_t	*pRplDump;

	PFSRASReq_t		*pReqRAS;
	PFSRASRpl_t		RplRAS;

	struct Session	*pSession;

	int	fd;	

	switch( (int)pF->h_Head.h_Cmd ) {
	case PFS_READ:
		pReq = (PFSReadReq_t*)pF;

		fd	= open( pReq->r_Name, O_RDWR, 0666 );
		if( fd >= 0 ) {

			pRpl	= (PFSReadRpl_t*)MSGALLOC( PFS_READ, 0, 
							sizeof(*pRpl) + pReq->r_Len, PFSMalloc );	

			Ret = lseek( fd, pReq->r_Offset, 0 );
			Ret = read( fd, (pRpl+1), pReq->r_Len );

			close( fd );

			pRpl->r_Head.h_Head.h_Cksum64	= 0;
			if( pAcceptor->c_bCksum ) {
				pRpl->r_Head.h_Head.h_Cksum64	=
					 in_cksum64( pRpl, pRpl->r_Head.h_Head.h_Len, 0 );
			}
			pRpl->r_Len	= Ret;
			SendStream( fdSend, (char*)pRpl, pRpl->r_Head.h_Head.h_Len );
			PFSFree( pRpl );

		} else {
			MSGINIT( &Rpl, PFS_READ, 0,sizeof(Rpl) );	
			Rpl.h_Error	= errno;
			Rpl.h_Head.h_Cksum64	= 0;
			if( pAcceptor->c_bCksum ) {
				Rpl.h_Head.h_Cksum64	= 
					in_cksum64( &Rpl, Rpl.h_Head.h_Len, 0 );
			}
			SendStream( fdSend, (char*)&Rpl, sizeof(Rpl) );
		}
		break;

	case PFS_CNTRL:
		pRplCntrl	= (PFSCntrlRpl_t*)MSGALLOC( PFS_CNTRL, 0, 
							sizeof(*pRplCntrl), PFSMalloc );	
		pRplCntrl->c_Cntrl	= CNTRL;
		pRplCntrl->c_pCntrl	= &CNTRL;
		pRplCntrl->c_Head.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			pRplCntrl->c_Head.h_Head.h_Cksum64	=
				 in_cksum64( pRplCntrl, pRplCntrl->c_Head.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)pRplCntrl, pRplCntrl->c_Head.h_Head.h_Len );
		PFSFree( pRplCntrl );
		break;

	case PFS_DUMP:
		pReqDump = (PFSDumpReq_t*)pF;
		pRplDump	= (PFSDumpRpl_t*)MSGALLOC( PFS_DUMP, 0, 
							sizeof(*pRplDump) + pReqDump->d_Size, PFSMalloc );	
		memcpy( pRplDump->d_Data, pReqDump->d_pStart, pReqDump->d_Size ); 
		pRplDump->d_Head.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			pRplDump->d_Head.h_Head.h_Cksum64	=
				 in_cksum64( pRplDump, pRplDump->d_Head.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)pRplDump, pRplDump->d_Head.h_Head.h_Len );
		PFSFree( pRplDump );
		break;

	default:
		MSGINIT( &Rpl, pF->h_Head.h_Cmd, 0, sizeof(Rpl) );	
		Rpl.h_Error	= -1;
		Rpl.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			Rpl.h_Head.h_Cksum64	= in_cksum64( &Rpl, Rpl.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)&Rpl, sizeof(Rpl) );
		break;

	case PFS_RAS:
		pReqRAS = (PFSRASReq_t*)pF;

		LOG( PaxosAcceptorGetLog(pAcceptor), LogINF, 
			"r_cell[%s] r_Path[%s]", pReqRAS->r_Cell, pReqRAS->r_Path );

		pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, NULL, NULL, pReqRAS->r_Cell );

		PaxosSessionSetLog( pSession, PaxosAcceptorGetLog(pAcceptor));

		char *pPath = RootOmitted(pReqRAS->r_Path);
		Ret = PFSRasThreadCreate( PaxosAcceptorGetCell(pAcceptor)->c_Name,
									pSession, pPath,
									PaxosAcceptorMyId(pAcceptor),
									_PFSRasSend );
		CNTRL.c_bRas	= TRUE;

    	MSGINIT( &RplRAS, PFS_RAS, 0, sizeof(RplRAS) );
		RplRAS.r_Head.h_Error	= Ret;
		RplRAS.r_Head.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			RplRAS.r_Head.h_Head.h_Cksum64	= 
						in_cksum64( &RplRAS, RplRAS.r_Head.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)&RplRAS, sizeof(RplRAS) );
		break;
	}
	return( 0 );
}

