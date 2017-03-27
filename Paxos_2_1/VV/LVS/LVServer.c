/******************************************************************************
*
*  LVServer.c 	--- Backing store
*
*  Copyright (C) 2014-2016 triTech Inc. All Rights Reserved.
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
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

//#define	_LOCK_

#define	_PAXOS_SESSION_SERVER_
#define	_PFS_SERVER_
#include	"PFS.h"
#include	"LV.h"
#include	<sys/wait.h>

/*
 *	Paxos File System
 */

int	MyId;

char	*pLV_Tar_Bin = "/bin/tar"; // Default: LINUX Machine.

LVCntrl_t	CNTRL;

#define	LVMalloc( s )	(PaxosAcceptorGetfMalloc(CNTRL.c_pAcceptor))( s )
#define	LVFree( p )		(PaxosAcceptorGetfFree(CNTRL.c_pAcceptor))( p )

int
LVInit( int Core, int Ext, int id, bool_t bCksum,
		int	FdMax, char *pRoot,
		int BlockMax, int SegSize, int SegNum, int UsecWB,
		struct PaxosAcceptor* pAcceptor )
{
	char	Root[PATH_NAME_MAX];
	int	Maximum;
	int	Ret;

	DBG_ENT();

	MyId	= id;
	CNTRL.c_Max	= Core;
	CNTRL.c_Maximum	= Maximum = Core + Ext;
	CNTRL.c_bCksum	= bCksum;
	CNTRL.c_pAcceptor	= pAcceptor;

	RwLockInit( &CNTRL.c_RwLock );

	sprintf( Root, "%s/"LV_ROOT"", pRoot );
	Ret = FileCacheInit( &CNTRL.c_BlockCache, FdMax, Root, BlockMax,
							SegSize, SegNum, UsecWB, bCksum,
							PaxosAcceptorGetLog(pAcceptor) );
	
	if( Ret < 0 )	goto err;
	unlink( FILE_SHUTDOWN );

	RETURN( 0 );
err:
	return( -1 );
}

int
LVAdaptorOpen( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	LVSessionAdaptor_t*	pAdaptor;
	int	Ret = 0;

	DBG_ENT();

	if( (pAdaptor = (LVSessionAdaptor_t *)
			PaxosAcceptGetTag( pAccept, LV_FAMILY )) ) {
	} else {

		pAdaptor = (LVSessionAdaptor_t*)LVMalloc( sizeof(*pAdaptor) );

		pAdaptor->a_pAccept	= pAccept;

		PaxosAcceptSetTag( pAccept, LV_FAMILY, pAdaptor );

	}
	LOG(pLog, LogINF, "!!! SESSION_OPEN pAccept=%p [pid=%d No=%d Seq=%d]", 
		pAccept, pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq );
	RETURN( Ret );
}

int
LVAdaptorClose( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	LVSessionAdaptor_t*	pAdaptor;
	int	Ret = 0;

	DBG_ENT();

LOG(pLog, LogINF, "!!! SESSION_CLOSE pAccept=%p [pid=%d No=%d Seq=%d]", 
			pAccept, pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq );
	if( !pAccept ) {
		/* BINDだけでRecover時には存在しない */
		RETURN( -1 );
	}
	pAdaptor	= (LVSessionAdaptor_t*)PaxosAcceptGetTag( pAccept, 
												LV_FAMILY );
	if( !pAdaptor ) {
	/* BINDだけ */
		LOG( pLog, LogERR, "#### NOT SESSION_OPEN pAccept=%p ###", pAccept);
		RETURN( -1 );
	}

	RwLockW( &CNTRL.c_RwLock );

	LVFree( pAdaptor );
	PaxosAcceptSetTag( pAccept, LV_FAMILY, NULL );

	RwUnlock( &CNTRL.c_RwLock );

	RETURN( Ret );
}

int LVWrite( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM );
int LVRead( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM );

int
LVCompound( struct PaxosAccept *pAccept, PaxosSessionHead_t *pReq )
{
	int	Len, len;
	PaxosSessionHead_t	*pH;
	
	PaxosAcceptHold( pAccept );

//	LOG( pLog, "h_Len=%d", pReq->h_Len );

	Len	= pReq->h_Len;	
	Len	-= sizeof(PaxosSessionHead_t);
	pH	= (PaxosSessionHead_t*)((char*)pReq + sizeof(PaxosSessionHead_t));
	while( Len > 0 ) {
		switch( (int)pH->h_Cmd ) {
		case LV_WRITE:
			LVWrite( pAccept, pH );
			break;
		case LV_READ:
			LVRead( pAccept, pH );
			break;
		default:	ABORT();
		}
		len	= (pH->h_Len + sizeof(long) - 1 )/sizeof(long)*sizeof(long);
		Len	-= len;
		pH	= (PaxosSessionHead_t*)((char*)pH + len);
	}
	PaxosAcceptRelease( pAccept );
	return( 0 );
}

int
LVDataApply( struct PaxosAccept* pAccept, PaxosSessionHead_t* pReq )
{
	DBG_ENT();

	switch( (int)pReq->h_Cmd ) {
	case LV_WRITE:
		LVWrite( pAccept, pReq );
		break;
	case LV_COMPOUND_WRITE:
//LOG( pLog, "LV_COMPOUND_WRITE");
		LVCompound( pAccept, pReq );
		break;
	default:
		ABORT();
	}
	RETURN( 0 );
}

int
LVOutOfBandReq( struct PaxosAccept* pAccept,
			PaxosSessionHead_t* pH )
{
	PaxosSessionHead_t*	pData;
	int	Ret = -1;

	DBG_ENT();

	PaxosAcceptOutOfBandLock( pAccept );

	pData	= _PaxosAcceptOutOfBandGet( pAccept, &pH->h_Id );

	ASSERT( pData );

	PaxosSessionHead_t	*pBody;

	pBody	= (pData+1);	// skip Push
	
	switch( (int)pBody->h_Cmd ) {

	default:
		Ret = LVDataApply( pAccept, pBody );
		break;
	}
	PaxosAcceptOutOfBandUnlock( pAccept );

	RETURN( Ret );
}

int
LVRejected( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM )
{
	PaxosSessionHead_t	Rpl;
	struct PaxosAccept*	pAccept;
	int	Ret;

	DBG_ENT();

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept )	RETURN( -1 );

	SESSION_MSGINIT( &Rpl, pM->h_Cmd, PAXOS_REJECTED, 0, sizeof(Rpl) );	
	Rpl.h_Master	= PaxosGetMaster(PaxosAcceptorGetPaxos(pAcceptor));
//LOG( pLog, "SEND REJECT pAccept=%p", pAccept );
	Ret = PaxosAcceptSend( pAccept, &Rpl );
	PaxosAcceptPut( pAccept );

	RETURN( Ret );
}

Msg_t*
LVBackupPrepare(int num )
{
	Msg_t	*pMsgList;

	DBG_ENT();

	pMsgList = MsgCreate( ST_MSG_SIZE, Malloc, Free );

	STFileList( LV_ROOT, pMsgList );
	if( num > 0 ) {
		Hash_t		*pTree;
		IOFile_t	*pF;
		char		Path[PATH_NAME_MAX];
		StatusEnt_t	*pEnt;

		pTree	= STTreeCreate( pMsgList );

		pF = IOFileCreate( LV_SNAP_UPD_LIST, O_RDONLY, 0, Malloc, Free );
		while( fgets( Path, sizeof(Path), pF->f_pFILE ) ) {
			if( Path[strlen(Path)-1] == '\n' )	Path[strlen(Path)-1] = 0;
			pEnt = STTreeFindByPath( Path + strlen(LV_ROOT)+1, pTree );
			//ASSERT( pEnt );
			if ( pEnt )
				pEnt->e_Type	|= ST_NON;
		}
		IOFileDestroy( pF, FALSE );

		STTreeDestroy( pTree );
	} else {	
		int	fd;

		fd = open( LV_SNAP_UPD_LIST, O_RDWR|O_TRUNC|O_CREAT, 0666 );
		close( fd );
	}

	DBG_EXT( 0 );
	return( pMsgList );
}

int
LVSessionShutdown( PaxosAcceptor_t *pAcceptor, 
				PaxosSessionShutdownReq_t *pReq, uint32_t nextDecide )
{
	int	Fd;
	char	buff[1024];

	FileCachePurgeAll( &CNTRL.c_BlockCache );

	Fd = creat( FILE_SHUTDOWN, 0666 );
	if( Fd < 0 )	goto err;

	sprintf( buff, "%u [%s]\n", nextDecide, pReq->s_Comment );
	write( Fd, buff, strlen(buff) );

	close( Fd );
	return( 0 );
err:
	return( -1 );
}

static int	AgainBackup;

int
LVBackup( PaxosSessionHead_t *pM )
{
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
	size_t		Size, SizeMax;
	int	total;

	DBG_ENT();

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
	STFileList( LV_ROOT, pMsgNew );
	STDiffList( pMsgOld, pMsgNew, Alive, pMsgDel, pMsgUpd );

	pF = IOFileCreate( LV_SNAP_DEL_LIST, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										Malloc, Free );
	for( i = 0; i < pMsgDel->m_N; i++ ) {
		IOWrite( (IOReadWrite_t*)pF, &pMsgDel->m_aVec[i].v_Len,
										sizeof(pMsgDel->m_aVec[i].v_Len) );
		IOWrite( (IOReadWrite_t*)pF, pMsgDel->m_aVec[i].v_pStart,
										pMsgDel->m_aVec[i].v_Len );
	}
	IOFileDestroy( pF, FALSE );

	pF = IOFileCreate( LV_SNAP_UPD_LIST, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										Malloc, Free );
	SizeMax = pM->h_EventCnt;
	for( cnt = 0, total = 0, Size = 0, pP = (PathEnt_t*)MsgFirst(pMsgUpd); 
				pP; pP = ( PathEnt_t*)MsgNext(pMsgUpd) ) {

		if( Size < SizeMax ) {
			sprintf( Path, "%s/%s\n", LV_ROOT, (char*)(pP+1) );
			IOWrite( (IOReadWrite_t*)pF, Path, strlen(Path) );
			Size	+= pP->p_Size;
			cnt++;
		}
		total++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
	"UPD[%d/%d]", cnt, total );

	AgainBackup	= total - cnt;

	IOFileDestroy( pF, FALSE );

	MsgDestroy( pMsgOld );
	MsgDestroy( pMsgNew );
	MsgDestroy( pMsgDel );
	MsgDestroy( pMsgUpd );

	char	command[128];
	int		status;

	unlink( LV_SNAP_TAR );
	sprintf( command, "tar -cf %s -T %s > /dev/null 2>&1", LV_SNAP_TAR, LV_SNAP_UPD_LIST );

LOG(PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogDBG, "TAR BEFORE[%s]", command);

	status = system( command );

LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogDBG, "TAR AFTER" );
	if ( status ) {
		LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogERR, "TAR: status=%d", status );
		pM->h_Error = status;
	}
	
	RETURN( 0 );
}

int
LVRestore()
{
	IOFile_t	*pF;
	PathEnt_t	*pP;
	Msg_t	*pMsgDel;
	MsgVec_t	Vec;
	int		i;
	int		Len;
	char	Path[FILENAME_MAX];
	int		cnt;

	DBG_ENT();

	pMsgDel = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pF = IOFileCreate( LV_SNAP_DEL_LIST, O_RDONLY, 0, Malloc, Free );
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
		sprintf( Path, "%s/%s", LV_ROOT, (char*)(pP+1) );

		if( pP->p_Type & ST_DIR )	rmdir( Path );
		else						unlink( Path );
		cnt++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF, "DEL[%d]", cnt );

	IOFileDestroy( pF, FALSE );
	MsgDestroy( pMsgDel );

	char	command[128];
	int		status;

	sprintf( command, "tar -xf %s > /dev/null 2>&1", LV_SNAP_TAR );
	status = system( command );
ASSERT( !status );
	RETURN( 0 );
}

int
LVTransferSend( IOReadWrite_t *pW )
{
	struct stat	Stat;
	int	Ret;
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];

	DBG_ENT();

	fd = open( LV_SNAP_DEL_LIST, O_RDONLY, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_DEL_LIST, errno );
		Size	= 0;
		IOWrite( pW, &Size, sizeof(Size) );
		AgainBackup = -1;
	}
	else {
		Ret = fstat( fd, &Stat );
		ASSERT( Ret == 0 );

		Size	= Stat.st_size;

		IOWrite( pW, &Size, sizeof(Size) );

		while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
			IOWrite( pW, Buff, n );
		}
		close( fd );
	}

	fd = open( LV_SNAP_UPD_LIST, O_RDONLY, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_UPD_LIST, errno );
		Size	= 0;
		IOWrite( pW, &Size, sizeof(Size) );
		AgainBackup = -1;
	}
	else {
		Ret = fstat( fd, &Stat );
		ASSERT( Ret == 0 );

		Size	= Stat.st_size;

		IOWrite( pW, &Size, sizeof(Size) );

		while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
			IOWrite( pW, Buff, n );
		}
		close( fd );
	}

	fd = open( LV_SNAP_TAR, O_RDONLY, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_TAR, errno );
		Size	= 0;
		IOWrite( pW, &Size, sizeof(Size) );
		AgainBackup = -1;
	}
	else {
		Ret = fstat( fd, &Stat );
		ASSERT( Ret == 0 );

		Size	= Stat.st_size;

		IOWrite( pW, &Size, sizeof(Size) );

		while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
			IOWrite( pW, Buff, n );
		}
		close( fd );
	}
	/*
	 *	Again ?
	 */
	IOWrite( pW, &AgainBackup, sizeof(AgainBackup ) );
	LOG(pLog, LogINF, "AgainBackup=%d", AgainBackup );

	RETURN( 0 );
}

int
LVTransferRecv( IOReadWrite_t *pR )
{
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];
	int		Ret;

	DBG_ENT();

	IORead( pR, &Size, sizeof(Size) );
	fd = open( LV_SNAP_DEL_LIST, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_DEL_LIST, errno );
		return( -1 );
	}
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( LV_SNAP_UPD_LIST, O_WRONLY|O_APPEND, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_UPD_LIST, errno );
		return( -1 );
	}
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( LV_SNAP_TAR, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 ) {
		LOG(pLog, LogERR, "open(%s), errno=%d", LV_SNAP_TAR, errno );
		return( -1 );
	}
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
	RETURN( Ret );
}

#define	SNAP_END	1

int
LVGlobalMarshal( IOReadWrite_t *pW )
{
	RETURN( 0 );
}

int
LVGlobalUnmarshal( IOReadWrite_t *pR )
{
	int	Ret = 0;
	RETURN( Ret );
}

int
LVAdaptorMarshal( IOReadWrite_t *pW, struct PaxosAccept* pAccept )
{
	int32_t	tag;

	tag	= SNAP_END;
	IOMarshal( pW, &tag, sizeof(tag) );
	return( 0 );
}

int
LVAdaptorUnmarshal( IOReadWrite_t *pR, struct PaxosAccept* pAccept )
{
	int32_t	tag;

	IOUnmarshal( pR, &tag, sizeof(tag) );
	return( 0 );
}

int
LVCacheOut( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	FileCacheFlushAll( &CNTRL.c_BlockCache );
	return( 0 );
}

int
LVWrite( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	LV_write_req_t	*pReq = (LV_write_req_t*)pM;
	LV_write_rpl_t	*pRpl;
	int 	error;
	int	Ret;

	error = 0;
	/*
	 *	Write
	 */
LOG( pLog, LogINF, "[%s/%"PRIu32"] off=%jd len=%zu", 
pReq->w_LS.ls_LV, pReq->w_LS.ls_LS, (intmax_t)pReq->w_Off, pReq->w_Len );
	Ret = FileCacheWrite( &CNTRL.c_BlockCache, pReq->w_LS.ls_LV, 
				pReq->w_Off, pReq->w_Len, (char*)(pReq+1) );
	if( Ret < 0 ) {
		Ret = FileCacheCreate( &CNTRL.c_BlockCache, pReq->w_LS.ls_LV );
		if( Ret == 0 ) {
			Ret = FileCacheWrite( &CNTRL.c_BlockCache, pReq->w_LS.ls_LV, 
				pReq->w_Off, pReq->w_Len, (char*)(pReq+1) );
		}
	}
//	LOG( pLog, "[%s/%"PRIu64"] off=%"PRIu64" len=%"PRIu64" Ret=%d", 
//	pReq->w_LS.ls_LV, pReq->w_LS.ls_LS, pReq->w_Off, pReq->w_Len, Ret );

	if( Ret < 0 )	error = EIO;
	/*
	 *	Reply
	 */
	pRpl	= (LV_write_rpl_t*)LVMalloc( sizeof(*pRpl) );
	SESSION_MSGINIT( pRpl, LV_WRITE, 0, error, sizeof(*pRpl) );

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
			pRpl, pRpl->w_Head.h_Len, pRpl->w_Head.h_Len );

	return( Ret );
}

int
LVRead( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	LV_read_req_t	*pReq = (LV_read_req_t*)pM;
	LV_read_rpl_t	*pRpl;
	size_t			len, size;
	int	Ret;

	/*
	 * Reply buffer
	 */
	len	= sizeof(*pRpl) + pReq->r_Len;
	size	= LONG_BYTE( len );

	pRpl	= (LV_read_rpl_t*)LVMalloc( size );

	LONG_ZERO( pRpl, size );
	SESSION_MSGINIT( pRpl, LV_READ, 0, 0, size );
	/*
	 *	Read
	 */
LOG( pLog, LogINF, "[%s/%"PRIu32"] off=%jd len=%zu", 
pReq->r_LS.ls_LV, pReq->r_LS.ls_LS, (intmax_t)pReq->r_Off, pReq->r_Len );

	Ret = FileCacheRead( &CNTRL.c_BlockCache, pReq->r_LS.ls_LV, 
				pReq->r_Off, pReq->r_Len, (char*)(pRpl+1) );

//	LOG( pLog, "[%s/%"PRIu64"] off=%"PRIu64" len=%"PRIu64" Ret=%d", 
//	pReq->r_LS.ls_LV, pReq->r_LS.ls_LS, pReq->r_Off, pReq->r_Len, Ret );

	pRpl->r_Len	= pReq->r_Len;
	/*
	 *	Reply
	 */
	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
			pRpl, pRpl->r_Head.h_Len, pRpl->r_Head.h_Len );

	return( Ret );
}

int
LVDeleteS( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	LV_delete_req_t	*pReq = (LV_delete_req_t*)pM;
	LV_delete_rpl_t	*pRpl;
	int	Ret;
	/*
	 *	Delete
	 */
	Ret = FileCacheDelete( &CNTRL.c_BlockCache, pReq->d_LS.ls_LV );

	pRpl	= (LV_delete_rpl_t*)LVMalloc( sizeof(*pRpl) );
	SESSION_MSGINIT( pRpl, LV_DELETE, 0, (Ret ? errno: 0), sizeof(*pRpl) );

	Ret = ACCEPT_REPLY_MSG( pAccept, TRUE,
			pRpl, pRpl->d_Head.h_Len, pRpl->d_Head.h_Len );

	return( Ret );
}

int
LVValidate( struct PaxosAcceptor* pAcceptor, uint32_t seq,
								PaxosSessionHead_t* pM, int From )
{
	int	Ret = 0;
	int	Cmd;

	Cmd	= pM->h_Cmd;

	if( ( Cmd & PAXOS_SESSION_FAMILY_MASK) != LV_FAMILY )	return( Ret );

	switch( Cmd ) {
	case LV_OUTOFBAND:
		Ret = PaxosAcceptorOutOfBandValidate( pAcceptor, seq, pM, From ); 
		break;
	}

	return( Ret );
}

int
LVAccepted( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	int	Ret = 0;
	bool_t	bFree = TRUE;

	RwLockW( &CNTRL.c_RwLock );

DBG_TRC("!!! ACCEPTED pAccept=%p [pid=%d No=%d Seq=%d]\n", 
			pAccept, pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq );
	ASSERT( pAccept );

	switch( (int)pM->h_Cmd ) {
	case LV_WRITE: 	

		{LV_write_req_t	*pReq = (LV_write_req_t*)pM;
		LOG(pLog, LogINF, "LV[%s] LS=%u Off=%jd Len=%zu", 
			pReq->w_LS.ls_LV, pReq->w_LS.ls_LS, (intmax_t)pReq->w_Off, pReq->w_Len );}

		LVWrite( pAccept, pM ); 
		break;
	case LV_CACHEOUT:	
		LVCacheOut( pAccept, pM );
		break;
	case LV_DELETE:	
		LVDeleteS( pAccept, pM );
		break;
	case LV_OUTOFBAND:	
		LVOutOfBandReq( pAccept, pM ); 
		break;
	case LV_COMPOUND_WRITE:
	 	LVCompound( pAccept, pM ); break;
	default:
		ABORT();
		break;
	}
	RwUnlock( &CNTRL.c_RwLock );

	if( bFree )	LVFree( pM );

	return( Ret );
}

int
LVAbort( struct PaxosAcceptor* pAcceptor, uint32_t page )
{
	Printf("=== Catch-up Error(Page=%d)\n", page );
	return( 0 );
}

int
LVRequest( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM, int fd )
{
	struct PaxosAccept*	pAccept;
	int	Ret = 0;
	bool_t	bValidate = FALSE;
	//bool_t	bAccepted;
	
	ASSERT( (pM->h_Cmd & PAXOS_SESSION_FAMILY_MASK) == LV_FAMILY );

	switch( (int)pM->h_Cmd ) {
	case LV_COMPOUND_READ:
//LOG(pLog,"LV_COMPOUND_READ");
	case LV_READ:

		pAccept = PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );

		ASSERT( pAccept );

		if( PaxosAcceptedStart( pAccept, pM ) ) {

			LOG( pLog, LogERR, 
			"AcceptedStart ERROR FCmd=%d INET=%s "
			"[Pid=%d No=%d Seq=%d] CODE=%d", 
			pM->h_Cmd, inet_ntoa(pM->h_Id.i_Addr.sin_addr), 
			pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Id.i_Seq,
			pM->h_Code );

			goto skip;
		}

		RwLockR( &CNTRL.c_RwLock );

		switch( (int)pM->h_Cmd ) {
			case LV_COMPOUND_READ: LVCompound( pAccept, pM ); break;
			case LV_READ:		LVRead( pAccept, pM ); break;
		}
		RwUnlock( &CNTRL.c_RwLock );
skip:
		PaxosAcceptPut( pAccept );

		break;

	default:	// Paxos投入
		if( (int)pM->h_Cmd == LV_OUTOFBAND )	 {

			bValidate = TRUE;
		}

		Ret = PaxosRequest( PaxosAcceptorGetPaxos(pAcceptor), 
								pM->h_Len, pM, bValidate );
		break;
	}
	return( Ret );
}

int
LVSessionLock()
{
	RwLockW( &CNTRL.c_RwLock );
/*
 *	Flush cache
 */
	FileCachePurgeAll( &CNTRL.c_BlockCache );
	return( 0 );
}

int
LVSessionUnlock()
{
	RwUnlock( &CNTRL.c_RwLock );
	return( 0 );
}

int
LVSessionAny( struct PaxosAcceptor *pAcceptor,
				PaxosSessionHead_t* pM, int fdSend )
{
	PaxosSessionHead_t	*pH = (PaxosSessionHead_t*)(pM+1);
	PaxosSessionHead_t	Rpl;
	LVCntrl_rpl_t	*pRplCntrl;
	LVDump_req_t	*pReqDump;
	LVDump_rpl_t	*pRplDump;
	int	len;
	PFSRASReq_t		*pReqRAS;
	PFSRASRpl_t		RplRAS;
	struct Session	*pSession;
	int	Ret;

	switch( (int)pH->h_Cmd ) {
	case LV_CNTRL:
		pRplCntrl	= (LVCntrl_rpl_t*)LVMalloc( sizeof(*pRplCntrl) );
		SESSION_MSGINIT( pRplCntrl, LV_CNTRL, 0, 0, sizeof(*pRplCntrl) );
		pRplCntrl->c_Cntrl	= CNTRL;
		pRplCntrl->c_pCntrl	= &CNTRL;

		pRplCntrl->c_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			pRplCntrl->c_Head.h_Cksum64	= 
					in_cksum64( pRplCntrl, pRplCntrl->c_Head.h_Len, 0 );
		}

		SendStream( fdSend, (char*)pRplCntrl, pRplCntrl->c_Head.h_Len );
		LVFree( pRplCntrl );
		break;

	case LV_DUMP:
		pReqDump = (LVDump_req_t*)pH;
		len = sizeof(*pRplDump) + pReqDump->d_Len;	
		pRplDump = (LVDump_rpl_t*)LVMalloc( len ); 
		SESSION_MSGINIT( pRplDump, LV_DUMP, 0, 0, len );
		memcpy( (pRplDump+1), pReqDump->d_pAddr, pReqDump->d_Len ); 

		pRplDump->d_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			pRplDump->d_Head.h_Cksum64	= 
					in_cksum64( pRplDump, pRplDump->d_Head.h_Len, 0 );
		}

		SendStream( fdSend, (char*)pRplDump, pRplDump->d_Head.h_Len );
		LVFree( pRplDump );
		break;

	case PFS_RAS:
		pReqRAS = (PFSRASReq_t*)pH;

		pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, NULL, NULL, pReqRAS->r_Cell );

		PaxosSessionSetLog( pSession, PaxosAcceptorGetLog(pAcceptor));

		Ret = PFSRasThreadCreate( PaxosAcceptorGetCell(pAcceptor)->c_Name,
									pSession, pReqRAS->r_Path, 
									PaxosAcceptorMyId(pAcceptor),
									_PFSRasSend );

    	MSGINIT( &RplRAS, PFS_RAS, 0, sizeof(RplRAS) );
		RplRAS.r_Head.h_Error	= Ret;
		RplRAS.r_Head.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			RplRAS.r_Head.h_Head.h_Cksum64	= 
						in_cksum64( &RplRAS, RplRAS.r_Head.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)&RplRAS, sizeof(RplRAS) );
		break;

	default:
		SESSION_MSGINIT( &Rpl, pH->h_Cmd, 0, 0, sizeof(Rpl) );	
		Rpl.h_Error	= -1;

		Rpl.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			Rpl.h_Cksum64	= 
					in_cksum64( &Rpl, Rpl.h_Len, 0 );
		}

		SendStream( fdSend, (char*)&Rpl, Rpl.h_Len );
		break;
	}
	return( 0 );
}

#include	<netinet/tcp.h>

PaxosCell_t			*pPaxosCell;
PaxosSessionCell_t	*pSessionCell;


#define	OUTOFBAND_PATH( FilePath, pId ) \
	({char * p; snprintf( FilePath, sizeof(FilePath), "%s/%s-%d-%d-%d-%d-%ld", \
		PAXOS_SESSION_OB_PATH, \
		inet_ntoa((pId)->i_Addr.sin_addr), \
		(pId)->i_Pid, (pId)->i_No, (pId)->i_Seq, (pId)->i_Try, \
		(pId)->i_Time ); \
		p = FilePath; while( *p ){if( *p == '.' ) *p = '_';p++;}})

int
LVOutOfBandSave( struct PaxosAcceptor* pAcceptor, struct PaxosSessionHead* pM )
{
	char	FileOut[PATH_NAME_MAX];
	IOFile_t*	pF;
	int	n;

	OUTOFBAND_PATH( FileOut, &pM->h_Id );

	pF	= IOFileCreate( FileOut, O_WRONLY|O_TRUNC, 0666,
					PaxosAcceptorGetfMalloc(pAcceptor),
					PaxosAcceptorGetfFree(pAcceptor) );
	if( !pF )	goto err1;

	 n = IOWrite( (IOReadWrite_t*)pF, pM, pM->h_Len );
if( n != pM->h_Len ) {
	DBG_PRT("n=%d pM->h_Len=%d\n", n, pM->h_Len );
}
ASSERT( n == pM->h_Len );

	IOFileDestroy( pF, FALSE );
	return( 0 );
err1:
	return( -1 );
}

int
LVOutOfBandRemove( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
{
	char	FileOut[PATH_NAME_MAX];
	int		Ret;

	OUTOFBAND_PATH( FileOut, pId );
	Ret = unlink( FileOut );
if( Ret ) {
	DBG_PRT("Ret=%d [%s]\n", Ret, FileOut );
}

	return( Ret );
}

struct PaxosSessionHead*
LVOutOfBandLoad( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
{
	char	FileOut[PATH_NAME_MAX];
	PaxosSessionHead_t	H, *pH;
	IOFile_t*	pF;

	OUTOFBAND_PATH( FileOut, pId );

	pF	= IOFileCreate( FileOut, O_RDONLY, 0666,
					PaxosAcceptorGetfMalloc(pAcceptor),
					PaxosAcceptorGetfFree(pAcceptor) );
	if( !pF )	goto err;

	IORead( (IOReadWrite_t*)pF, &H, sizeof(H) );

	if( !(pH = (PaxosAcceptorGetfMalloc(pAcceptor))( H.h_Len )) )	goto err1;

	*pH	= H;
	IORead( (IOReadWrite_t*)pF, (pH+1), H.h_Len - sizeof(H) );

	IOFileDestroy( pF, FALSE );

	return( pH );
err1:
	IOFileDestroy( pF, FALSE );
err:
	return( NULL );
}

int
LVLoad( void *pTag )
{
	return( PaxosAcceptorGetLoad( (struct PaxosAcceptor*)pTag ) );
}

int
InitDirectory( int Target )
{
	char	command[256];

	LOG(pLog, LogINF, "Target=%d", Target );

	sprintf( command, "rm -rf %s", PAXOS_SESSION_OB_PATH );
	system( command );
	DBG_TRC("command[%s]\n", command );

	sprintf( command, "mkdir %s", PAXOS_SESSION_OB_PATH );
	system( command );
	DBG_TRC("command[%s]\n", command );

	return( 0 );
}


int
ExportToFile( struct PaxosAcceptor* pAcceptor, int j, char *pFile )
{
	struct Session	*pSession;
	IOFile_t		*pIOFile;
	IOReadWrite_t	*pW;
	int	Ret;
	struct Log*	pLog = PaxosAcceptorGetLog( pAcceptor);

	pSession	= PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr, 
					Send, Recv, Malloc, Free, pSessionCell->c_Name );
	if( pLog ) {
		PaxosSessionSetLog( pSession, pLog );
	}

	pIOFile	= IOFileCreate( pFile, O_WRONLY|O_TRUNC, 0, Malloc, Free );
	pW	= (IOReadWrite_t*)pIOFile;

	Ret = PaxosSessionExport( pAcceptor,pSession, j, pW );

	IOFileDestroy( pIOFile, FALSE );
	PaxosSessionFree( pSession );

	return( Ret );
}

int
ImportFromFile( struct PaxosAcceptor* pAcceptor, char *pFile )
{
	IOFile_t	*pIOFile;
	int	Ret;

	pIOFile	= IOFileCreate( pFile, O_RDONLY, 0, Malloc, Free );
	if( !pIOFile ) {
		printf("No file[%s].\n", pFile );
		exit( -1 );
	}
	Ret = PaxosSessionImport( pAcceptor, (IOReadWrite_t*)pIOFile );

	IOFileDestroy( pIOFile, FALSE );
	return( Ret );
}

void
usage()
{
printf("server cellname my_id root_directory {FALSE|TRUE [seq]\n");
printf("\tcellname      --- cell name\n");
printf("\tid            --- server id\n");
printf("\tdirectory     --- paxos/DB directory\n");
printf("\tFALSE				... synchronize and wait decision and join\n");
printf("\tTRUE [seq]			... synchronize and join in paxos\n");
printf("	-E target_id file	... let target_id export status to file\n");
printf("	-I file				... import status from file and rejoin\n");
printf("	-c Core				... core number\n");
printf("	-e Extension		... extension number\n");
printf("	-s {1|0}			... checksum on/off\n");
printf("	-F FdMax			... file descriptor max\n");
printf("	-B BlockMax			... cache block max\n");
printf("	-S SegmentSize		... segment size\n");
printf("	-N SegmentNumber	... number of segments in a block\n");
printf("							BlockSize=SegmentSize*SegmentNumber\n");
printf("	-U interval 		... write back interval(usec)\n");
printf("	-u interval 		... Paxos unit interval(usec)\n");
printf("	-M MemMax	 		... max memory to out-of-band\n");
printf("	-W Workers	 		... number of accept Workers\n");
printf("	-L Level 			... log level\n");
}

struct Log*	pLog = NULL;

int
main( int ac, char* av[] )
{
	int	j;
	int	id;
	int		Core = DEFAULT_CORE, Ext = DEFAULT_EXTENSION/*, Maximum*/;
	bool_t	bCksum = DEFAULT_CKSUM_ON;
	int		FdMax = DEFAULT_FD_MAX;
	int		BlockMax = DEFAULT_BLOCK_MAX;
	int		SegSize = DEFAULT_SEG_SIZE, SegNum = DEFAULT_SEG_NUM;
	int64_t	WBUsec	= DEFAULT_WB_USEC;
	int64_t	PaxosUsec	= DEFAULT_PAXOS_USEC;
	int64_t	MemLimit = DEFAULT_OUT_MEM_MAX;
	bool_t	bExport = FALSE, bImport = FALSE;
	int		target_id = -1;
	char	*pFile = NULL;
	int		Workers = DEFAULT_WORKERS;
	PaxosStartMode_t sMode;	uint32_t seq = 0;
	int		LogLevel	= DEFAULT_LOG_LEVEL;

	/*
	 * Opts
	 */
	j = 1;
	while( j < ac ) {
		char	*pc;
		int		l;
		pc = av[j++];
		if( *pc != '-' )	break;
		l = strlen( pc );
		if( l < 2 )	continue;

		switch( *(pc+1) ) {
		case 'E':	bExport = TRUE;
					target_id = atoi(av[j++]);
					pFile = av[j++];
					break;
		case 'I':	bImport = TRUE; pFile = av[j++];	break;
		case 'c':	Core = atoi(av[j++]);	break;
		case 'e':	Ext = atoi(av[j++]);	break;
		case 's':	bCksum = atoi(av[j++]);	break;
		case 'F':	FdMax = atoi(av[j++]);	break;
		case 'B':	BlockMax = atoi(av[j++]);	break;
		case 'S':	SegSize = atoi(av[j++]);	break;
		case 'N':	SegNum = atoi(av[j++]);	break;
		case 'U':	WBUsec = atol(av[j++]);	break;
		case 'u':	PaxosUsec = atol(av[j++]);	break;
		case 'M':	MemLimit = atol(av[j++]);	break;
		case 'W':	Workers = atoi(av[j++]);	break;
		case 'L':	LogLevel = atoi(av[j++]);	break;
		}
	}
	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}
	if( ac < 5 ) {
		usage();
		exit( -1 );
	}
	if( !strcmp("TRUE", av[4] ) ) {
		if( ac >= 6 ) {
			seq	= atoll(av[5]);
			sMode	= PAXOS_SEQ;
		} else {
			sMode	= PAXOS_NO_SEQ;
		}
	} else {
		sMode	= PAXOS_WAIT;
	}

	if( chdir( av[3] ) ) {
		printf("Can't change directory.\n");
		usage();
		exit( -1 );
	}
	id	= atoi(av[2]);

/*
 *	Address
 */
	pPaxosCell		= PaxosCellGetAddr( av[1], Malloc );
	pSessionCell	= PaxosSessionCellGetAddr( av[1], Malloc );
	if( !pPaxosCell || !pSessionCell ) {
		printf("Can't get cell addr.\n");
		exit( -1 );
	}
/*
 *	Log
 */
	int			log_size = -1;

	char *psz_log_size = getenv("LOG_SIZE");
	if( psz_log_size ) {
		log_size = atoi( psz_log_size );
	}
/*
 *	Initialization
 */
	struct PaxosAcceptor* pAcceptor;
	PaxosAcceptorIF_t	IF;

	memset( &IF, 0, sizeof(IF) );

	IF.if_fMalloc	= Malloc;
	IF.if_fFree		= Free;

	IF.if_fSessionOpen	= LVAdaptorOpen;
	IF.if_fSessionClose	= LVAdaptorClose;
	IF.if_fRequest	= LVRequest;
	IF.if_fValidate	= LVValidate;
	IF.if_fAccepted	= LVAccepted;
	IF.if_fRejected	= LVRejected;
	IF.if_fAbort	= LVAbort;
	IF.if_fShutdown	= LVSessionShutdown;

	IF.if_fBackupPrepare	= LVBackupPrepare;
	IF.if_fBackup			= LVBackup;
	IF.if_fRestore			= LVRestore;
	IF.if_fTransferSend		= LVTransferSend;
	IF.if_fTransferRecv		= LVTransferRecv;

	IF.if_fGlobalMarshal	= LVGlobalMarshal;
	IF.if_fGlobalUnmarshal	= LVGlobalUnmarshal;
	IF.if_fAdaptorMarshal	= LVAdaptorMarshal;
	IF.if_fAdaptorUnmarshal	= LVAdaptorUnmarshal;

	IF.if_fLock		= LVSessionLock;
	IF.if_fUnlock	= LVSessionUnlock;
	IF.if_fAny		= LVSessionAny;

	IF.if_fOutOfBandSave	= LVOutOfBandSave;
	IF.if_fOutOfBandLoad	= LVOutOfBandLoad;
	IF.if_fOutOfBandRemove	= LVOutOfBandRemove;

	IF.if_fLoad	= LVLoad;

	pAcceptor = (struct PaxosAcceptor*)Malloc( PaxosAcceptorGetSize() );

	PaxosAcceptorInitUsec( pAcceptor, id, Core, Ext, MemLimit, bCksum,
				pSessionCell, pPaxosCell, PaxosUsec, Workers, &IF );


/*
 *	Logger
 */
	if( log_size >= 0 ) {

		if( pLog ) {
			PaxosAcceptorSetLog( pAcceptor, pLog );
		} else {
			PaxosAcceptorLogCreate( pAcceptor, log_size, 
									LOG_FLAG_BINARY, stderr, LogLevel );
			pLog = PaxosAcceptorGetLog( pAcceptor);

			LOG( pLog, LogINF, "###### LOG_SIZE[%d] ####", log_size );
		}
		PaxosSetLog( PaxosAcceptorGetPaxos( pAcceptor ), pLog );
		LogAtExit( pLog );
	}
/*
 *	LVInit
 */
	LVInit( Core, Ext, id, bCksum, FdMax, ".", BlockMax, SegSize, SegNum,
				WBUsec, pAcceptor );
/*
 *	autonomic
 */
	if( bExport ) {
		ExportToFile( pAcceptor, target_id, pFile );
		exit( 0 );
	}
	if( bImport ) {
		ImportFromFile( pAcceptor, pFile );
	}

/*
 * 	Start
 */
	if( PaxosAcceptorStartSync( pAcceptor, sMode, seq, InitDirectory ) < 0 ) {
		LOG( pLog, LogERR, "START ERROR\n");
		exit( -1 );
	}
	return( 0 );
}
