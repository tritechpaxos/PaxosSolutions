/******************************************************************************
*
*  PFSProbe.c 	--- probe program of PFS Library
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
 *	試験			塩尻常春
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_


#define	_PFS_SERVER_
#define	_PAXOS_SESSION_SERVER_
#include	"PFS.h"
#include	<stdio.h>
#include	<ctype.h>

int
PFSReadCntrl( struct Session* pSession, void* pH, 
					PFSControl_t* pCntrl, PFSControl_t** ppCntrl )
{
	PFSCntrlReq_t	Req;
	PFSCntrlRpl_t	*pRpl;
	int	Ret;

	MSGINIT( &Req, PFS_CNTRL, 0, sizeof(Req) );
	pRpl = (PFSCntrlRpl_t*)
			PaxosSessionAny( pSession, pH, (PaxosSessionHead_t*)&Req );
	if( pRpl->c_Head.h_Error ) {
		errno = pRpl->c_Head.h_Error;
		Ret = -1;
	} else {
		*pCntrl		= pRpl->c_Cntrl;
		*ppCntrl	= pRpl->c_pCntrl;
		Ret = 0;
	}
	Free( pRpl );
	return( Ret );
}

int
PFSReadMem( struct Session* pSession, void* pH, 
				void* pStart, int Size, void* pData )
{
	PFSDumpReq_t	Req;
	PFSDumpRpl_t	*pRpl;
	int	Ret;

	MSGINIT( &Req, PFS_DUMP, 0, sizeof(Req) );
	Req.d_pStart	= pStart;
	Req.d_Size		= Size;
	pRpl = (PFSDumpRpl_t*)
			PaxosSessionAny( pSession, pH, (PaxosSessionHead_t*)&Req );
	if( pRpl->d_Head.h_Error ) {
		errno = pRpl->d_Head.h_Error;
		Ret = -1;
	} else {
		if(!pData) abort();
		memcpy( pData, pRpl->d_Data, Size );
		Ret = 0;
	}
	Free( pRpl );
	return( Ret );
}
int	PFSEventDir( struct Session* pSession, PFSEventDir_t* pEvent ){return(0);}
int	PFSEventLock( struct Session* pSession, PFSEventLock_t* pEvent ){return(0);}

PFSControl_t	CNTRL, *pCNTRL;
PaxosAcceptor_t	Acceptor;

int
PrintClientId( PaxosClientId_t* pId )
{
	printf("ClientId [%s-%d] Pid=%u-%d Seq=%d Reuse=%d Try=%d\n", 
		inet_ntoa( pId->i_Addr.sin_addr ), ntohs( pId->i_Addr.sin_port ),
		pId->i_Pid, pId->i_No, pId->i_Seq, pId->i_Reuse, pId->i_Try );
	return( 0 );
}

int
PrintFileHead( PFSHead_t* pF )
{
	printf("[FileHead Len=%d Cmd=%d]\n", 
		pF->h_Head.h_Len, pF->h_Head.h_Cmd );
	PrintClientId( &pF->h_Head.h_Id );
	return( 0 );
}

int
PrintPath( PFSPath_t* pPath )
{
	printf("[Path [%s] ]\n", pPath->p_Name );
	return( 0 );
}

int
PrintLock( PFSLock_t* pLock )
{
	printf("[Lock [%s] Ver=%d Cnt=%d]\n", 
					pLock->l_Name, pLock->l_Ver, pLock->l_Cnt );
	return( 0 );
}

int
DumpAdaptor( struct Session* pSession, void* pH,
		SessionAdaptor_t* pAdaptor, SessionAdaptor_t* pOrg )
{
	PFSJoint_t	*pJoint, Joint;
	PFSPath_t		*pPath, Path;
	PFSLock_t	*pLock, Lock;

	printf("### Adaptor ###\n");
	LIST_FOR_EACH_ENTRY( pJoint, &pAdaptor->a_Events, j_Head.j_Wait,
								pAdaptor, pOrg ) {
		PFSReadMem( pSession, pH, pJoint, sizeof(Joint), &Joint );
		switch( Joint.j_Type ) {
			case JOINT_EVENT_PATH:
				printf("JOINT_EVENT_PATH\n");
				pPath	= (PFSPath_t*)Joint.j_pArg;
				PFSReadMem( pSession, pH, pPath, sizeof(Path), &Path );
				PrintPath( &Path );
				break;
			case JOINT_EVENT_LOCK:
			case JOINT_EVENT_LOCK_RECALL:
				printf("JOINT_EVENT_LOCK or JOINT_EVENT_LOCK_RECALL\n");
				pLock	= (PFSLock_t*)Joint.j_pArg;
				PFSReadMem( pSession, pH, pLock, sizeof(Lock), &Lock );
				PrintLock( &Lock );
				break;
			default:
				break;
		}
		pJoint	= &Joint;
	}
	printf("=== Adaptor(END) ===\n");
	return( 0 );
}

int
PrintAcceptorFdEvent( PaxosAcceptorFdEvent_t *pEv )
{
	printf("Fd=%d events=0x%x\n", 
			pEv->e_IOSock.s_Fd, pEv->e_FdEvent.fd_Events );
	return( 0 );
}

int
DumpAccept( struct Session* pSession, void* pH,
		PaxosAccept_t* pAccept, PaxosAccept_t* pOrg, bool_t bAdaptor )
{
	PaxosAcceptorFdEvent_t	Ev;
	int	ind;
	list_t	*paL;
	HashItem_t	*pItem, Item;
	SessionAdaptor_t	*pAdaptor, Adaptor;
	int	N;

	PrintClientId( &pAccept->a_Id );
printf("Cnt=%d pFd=%p Opened=%d Start=%u End=%u Reply=%u Cmd=0x%x Events=%d(%d) Create=%u Access=%u\n",
		pAccept->a_GE.e_Cnt, pAccept->a_pFd, pAccept->a_Opened, 
		pAccept->a_Accepted.a_Start, pAccept->a_Accepted.a_End, 
		pAccept->a_Accepted.a_Reply, pAccept->a_Accepted.a_Cmd,
		pAccept->a_Events.e_Cnt, pAccept->a_Events.e_Send, 
		(uint32_t)pAccept->a_Create, (uint32_t)pAccept->a_Access );

		if( pAccept->a_pFd ) {
			PFSReadMem( pSession, pH, pAccept->a_pFd, sizeof(Ev), &Ev );
			PrintAcceptorFdEvent( &Ev );
		}

if( bAdaptor ) {
	N	= pAccept->a_Tag.h_N;
	paL	= (list_t*)Malloc( sizeof(list_t)*N );
	PFSReadMem( pSession, pH, pAccept->a_Tag.h_aHeads, sizeof(list_t)*N, paL );

	for( ind = 0; ind < N; ind++ ) {

		LIST_FOR_EACH_ENTRY( pItem, &paL[ind], i_Link, paL, 
									pAccept->a_Tag.h_aHeads ) {

			PFSReadMem( pSession, pH, pItem, sizeof(Item), &Item );
			pAdaptor	= (SessionAdaptor_t*)Item.i_pValue;
			PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
			DumpAdaptor( pSession, pH, &Adaptor, pAdaptor );
			pItem	= &Item;
		}
	}
}
	return( 0 );
}

int
ReadAccept( struct Session* pSession, void* pH )
{
	PaxosAccept_t	*pAccept, Accept;

	printf("### Accept ###\n");
	LIST_FOR_EACH_ENTRY( pAccept, &Acceptor.c_Accept.ge_Lnk, a_GE.e_Lnk,
								&Acceptor, CNTRL.c_pAcceptor ) {
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, TRUE );
		pAccept = &Accept;
	}
	printf("=== Accept(END) ===\n");
	return( 0 );
}

int
DumpPath( struct Session *pSession, void *pH,
		PFSPath_t *pPath, PFSPath_t *pOrg )
{
	PFSJoint_t	Joint, *pJoint;
	SessionAdaptor_t	Adaptor, *pAdaptor;
	PaxosAccept_t	*pAccept, Accept;

	printf("=== Path[%s] Flags=0x%x RefCnt=%d EventSeq=%u ===\n", 
		pPath->p_Name, pPath->p_Flags, pPath->p_RefCnt, pPath->p_EventSeq );
	LIST_FOR_EACH_ENTRY( pJoint, &pPath->p_AdaptorEvent, j_Head.j_Event,
								pPath, pOrg ) {
		PFSReadMem( pSession, pH, pJoint, sizeof(Joint), &Joint );
		pAdaptor	= Joint.j_pAdaptor;
		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pJoint = &Joint;
	}
	return( 0 );
}

int
DumpLock( struct Session* pSession, void* pH,
		PFSLock_t* pLock, PFSLock_t* pOrg )
{
	PFSJoint_t	Joint, *pJoint;
	SessionAdaptor_t	Adaptor, *pAdaptor;
	PaxosAccept_t	*pAccept, Accept;

	printf("=== LOCK [%s] Cnt=%d ===\n", pLock->l_Name, pLock->l_Cnt );

	printf("->Holders\n");
	LIST_FOR_EACH_ENTRY( pJoint, &pLock->l_AdaptorHold, j_Head.j_Event,
								pLock, pOrg ) {
		PFSReadMem( pSession, pH, pJoint, sizeof(Joint), &Joint );
		pAdaptor	= Joint.j_pAdaptor;
		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pJoint = &Joint;
	}
	printf("->Waiters\n");
	LIST_FOR_EACH_ENTRY( pAdaptor, &pLock->l_AdaptorWait, a_LockWait,
								pLock, pOrg ) {
		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pAdaptor = &Adaptor;
	}
	printf("->Event Waiters\n");
	LIST_FOR_EACH_ENTRY( pJoint, &pLock->l_AdaptorEvent, j_Head.j_Event,
								pLock, pOrg ) {
		PFSReadMem( pSession, pH, pJoint, sizeof(Joint), &Joint );
		pAdaptor	= Joint.j_pAdaptor;
		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pJoint = &Joint;
	}
	return( 0 );
}

int
DumpQueue( struct Session* pSession, void* pH,
		PFSQueue_t* pQueue, PFSQueue_t* pOrg )
{
	PFSJoint_t	Joint, *pJoint;
	SessionAdaptor_t	Adaptor, *pAdaptor;
	PaxosAccept_t	*pAccept, Accept;
	Msg_t		*pMsg, Msg;
	MsgVec_t	*aVec;
	int	i, j;

	printf("=== QUEUE [%s] Cnt=%d Peek=%d===\n", 
		pQueue->q_Name, pQueue->q_CntMsg, pQueue->q_Peek );

	if( pQueue->q_CntMsg > 0 ) {	// data
		printf("->Data\n");
		j	= 0;
		LIST_FOR_EACH_ENTRY( pMsg, &pQueue->q_AdaptorMsg, m_Lnk,
								pQueue, pOrg ) {

			PFSReadMem( pSession, pH, pMsg, sizeof(Msg), &Msg );

			aVec	= (MsgVec_t*)Malloc( sizeof(MsgVec_t)*Msg.m_N );

			PFSReadMem( pSession, pH, Msg.m_aVec, 
								sizeof(MsgVec_t)*Msg.m_N, aVec );
			for( i = 0; i < Msg.m_N; i++ ) {
				printf("[%d] %d: v_Len=%d\n", j, i, aVec[i].v_Len );
			}
			Free( aVec );

			j++;
			pMsg = &Msg;
		}
	}
	printf("->Waiters\n");
	LIST_FOR_EACH_ENTRY( pAdaptor, &pQueue->q_AdaptorWait, a_QueueWait,
								pQueue, pOrg ) {

		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pAdaptor = &Adaptor;
	}
	printf("->Event Waiters\n");
	LIST_FOR_EACH_ENTRY( pJoint, &pQueue->q_AdaptorEvent, j_Head.j_Event,
								pQueue, pOrg ) {
		PFSReadMem( pSession, pH, pJoint, sizeof(Joint), &Joint );
		pAdaptor	= Joint.j_pAdaptor;
		PFSReadMem( pSession, pH, pAdaptor, sizeof(Adaptor), &Adaptor );
		pAccept	= Adaptor.a_pAccept;
		PFSReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept, FALSE );

		pJoint = &Joint;
	}
	return( 0 );
}

int
ReadLock( struct Session* pSession, void* pH )
{
	PFSLock_t	*pLock, Lock;

	printf("### Lock ###\n");
	LIST_FOR_EACH_ENTRY( pLock, &CNTRL.c_Lock.hl_Lnk, l_Lnk,
								&CNTRL, pCNTRL ) {
		PFSReadMem( pSession, pH, pLock, sizeof(Lock), &Lock );
		DumpLock( pSession, pH, &Lock, pLock );
		pLock = &Lock;
	}
	printf("=== Lock(END) ===\n");
	return( 0 );
}

int
ReadPath( struct Session* pSession, void* pH )
{
	PFSPath_t	*pPath, Path;

	printf("### Path ###\n");
	LIST_FOR_EACH_ENTRY( pPath, &CNTRL.c_Path.hl_Lnk, p_Lnk,
								&CNTRL, pCNTRL ) {
		PFSReadMem( pSession, pH, pPath, sizeof(Path), &Path );
		DumpPath( pSession, pH, &Path, pPath );
		pPath = &Path;
	}
	printf("=== Path(END) ===\n");
	return( 0 );
}

int
ReadQueue( struct Session* pSession, void* pH )
{
	PFSQueue_t	*pQueue, Queue;

	printf("### Queue ###\n");
	LIST_FOR_EACH_ENTRY( pQueue, &CNTRL.c_Queue.hl_Lnk, q_Lnk,
								&CNTRL, pCNTRL ) {
		PFSReadMem( pSession, pH, pQueue, sizeof(Queue), &Queue );
		DumpQueue( pSession, pH, &Queue, pQueue );
		pQueue = &Queue;
	}
	printf("=== Queue(END) ===\n");
	return( 0 );
}

int
DumpOut( struct Session *pSession, void *pH,
				AcceptorOutOfBand_t	*pOut, AcceptorOutOfBand_t *pOrg )
{
	PaxosSessionHead_t	Head;

	if( pOut->o_pData ) {
		PFSReadMem( pSession, pH, pOut->o_pData, sizeof(Head), &Head );
		Printf(
		"=== %p Validate[%d] Seq[%u] pData[%p] bSave[%d] "
		"Len[%d] ===\n", 
		pOrg, pOut->o_bValidate, pOut->o_Seq, pOut->o_pData, pOut->o_bSaved,
		Head.h_Len );
	} else {
		Printf(
		"=== %p Validate[%d] Seq[%u] pData[%p] bSave[%d] ===\n", 
		pOrg, pOut->o_bValidate, pOut->o_Seq, pOut->o_pData, pOut->o_bSaved );
	}
	Printf("\t");
	PrintClientId( &pOut->o_Id );
	Printf("\tCreate %s", ctime( &pOut->o_Create ) );
	Printf("\tLast   %s", ctime( &pOut->o_Last ) );
	return( 0 );
}

int
ReadOutOfBand( struct Session* pSession, void* pH )
{
	AcceptorOutOfBand_t	*pOut, Out;

	printf("### OUT_OF_BAND Cnt=%d Mem=%"PRIi64"(%"PRIi64") ###\n", 
			Acceptor.c_OutOfBand.o_Cnt, Acceptor.c_OutOfBand.o_MemUsed,
			Acceptor.c_OutOfBand.o_MemLimit );	
	LIST_FOR_EACH_ENTRY( pOut, &Acceptor.c_OutOfBand.o_Lnk, o_Lnk,
								&Acceptor, CNTRL.c_pAcceptor ) {
		PFSReadMem( pSession, pH, pOut, sizeof(Out), &Out );
		DumpOut( pSession, pH, &Out, pOut );
		pOut = &Out;
	}
	printf("=== OUT_OF_BAND(END) ===\n");
	return( 0 );
}

int
DumpFd( struct Session *pSession, void *pH, GFd_t *pFd, GFd_t *pOrg )
{
	printf("%p e_Cnt=%d Name[%s] Dir[%s] Base[%s] Fd[%d]\n",
		pOrg, pFd->f_GE.e_Cnt, pFd->f_Name, pFd->f_DirName, pFd->f_BaseName, 
		pFd->f_Fd );
	return( 0 );
}

int
ReadFd( struct Session* pSession, void* pH )
{
	GFd_t	Fd, *pFd;

	printf("### FdCache Cnt=%d FdMax=%d Root[%s] ###\n", 
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Ctrl.ge_Cnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Ctrl.ge_MaxCnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Root );
			
	LIST_FOR_EACH_ENTRY( pFd, 
				&CNTRL.c_BlockCache.bc_FdCtrl.fc_Ctrl.ge_Lnk, f_GE.e_Lnk,
								&CNTRL, pCNTRL ) {
		PFSReadMem( pSession, pH, pFd, sizeof(Fd), &Fd );
		DumpFd( pSession, pH, &Fd, pFd );
		pFd = &Fd;
	}
	printf("=== FdCache(END) ===\n");
	return( 0 );
}

int
DumpFdMeta( struct Session *pSession, void *pH, 
				GFdMeta_t *pFd, GFdMeta_t *pOrg )
{
	printf("%p e_Cnt=%d Name[%s] Path[%s] Fd[%d] st_size[%zu]\n",
		pOrg, pFd->fm_GE.e_Cnt, pFd->fm_Name, pFd->fm_Path,
		pFd->fm_Fd, pFd->fm_Stat.st_size );
	return( 0 );
}
int
ReadMeta( struct Session* pSession, void* pH )
{
	GFdMeta_t	Fd, *pFd;

	printf("### FdMetaCache Cnt=%d FdMetaMax=%d Root[%s] ###\n", 
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_Cnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_MaxCnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Root );
			
	LIST_FOR_EACH_ENTRY( pFd, 
				&CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_Lnk, fm_GE.e_Lnk,
								&CNTRL, pCNTRL ) {
		PFSReadMem( pSession, pH, pFd, sizeof(Fd), &Fd );
		DumpFdMeta( pSession, pH, &Fd, pFd );
		pFd = &Fd;
	}
	printf("=== FdMetaCache(END) ===\n");
	return( 0 );
}

int
DumpBlock( struct Session *pSession, void *pH, 
			BlockCache_t *pB, BlockCache_t *pOrg, BlockCacheCtrl_t *pBC )
{
	char		*pBuf;
	timespec_t	*pValidTime;
	uint64_t	*pCksum64;
	int	j, k;
	unsigned char	c;

	printf("%p e_Cnt=%d Name[%s] Off[%"PRIu64"] Len=%d Flags=0x%x \n", 
			pOrg, pB->b_GE.e_Cnt, pB->b_Key.k_Name, pB->b_Key.k_Offset, 
			pB->b_Len, pB->b_Flags );

	if( pB->b_Len <= 0 )	return( 0 );

	pBuf		= Malloc(BLOCK_CACHE_SIZE(pBC));
	pValidTime	= (timespec_t*)
					Malloc(sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC));
	pCksum64	= (uint64_t*)Malloc(sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBC));

	PFSReadMem( pSession, pH, pB->b_pBuf, BLOCK_CACHE_SIZE(pBC), pBuf );
	PFSReadMem( pSession, pH, pB->b_pValid,
					sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC), pValidTime );
	PFSReadMem( pSession, pH, pB->b_pCksum64,
					sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBC), pCksum64 );

	for( j = 0; j < pBC->bc_SegmentNum; j++ ) {
		printf("\tCksum64[%d]=%"PRIu64" ", j, pCksum64[j] );
		printf("[");
		for( k = 0; k < 32 && k < SEGMENT_CACHE_SIZE(pBC); k++ ) {
			c =(pBuf[j*SEGMENT_CACHE_SIZE(pBC)+k]&0xff);
			if( isprint(c))	printf("%c", c );
			else			printf(".");
		}
		printf("]");
		printf(" %s", ctime( &pValidTime[j].tv_sec ) );
	}
	Free( pBuf );
	Free( pValidTime );
	Free( pCksum64 );
	return( 0 );
}

int
ReadBlock( struct Session* pSession, void* pH )
{
	BlockCache_t	B, *pB;

	printf(
		"### BlockCache Cnt=%d Max=%d "
		"SegSize=%d SegNum=%d bCksum=%d UsecWB=%d ###\n", 
	CNTRL.c_BlockCache.bc_Ctrl.ge_Cnt, CNTRL.c_BlockCache.bc_Ctrl.ge_MaxCnt, 
	CNTRL.c_BlockCache.bc_SegmentSize,
	CNTRL.c_BlockCache.bc_SegmentNum, CNTRL.c_BlockCache.bc_bCksum,
	CNTRL.c_BlockCache.bc_UsecWB );

	LIST_FOR_EACH_ENTRY( pB, 
				&CNTRL.c_BlockCache.bc_Ctrl.ge_Lnk, b_GE.e_Lnk,
								&CNTRL, pCNTRL ) {

		PFSReadMem( pSession, pH, pB, sizeof(B), &B );
		DumpBlock( pSession, pH, &B, pB, &CNTRL.c_BlockCache );
		pB = &B;
	}
	printf("=== BlockCache(END) ===\n");
	return( 0 );
}

int
usage()
{
printf("PFSProbe cell id "
		"{accept|path|lock|queue|outofband|meta|fd|block|ras}\n");
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	j;
	int	Ret;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	void*	pH;
	struct Session*	pSession;

	pSession = PaxosSessionAlloc( ConnectAdm, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession ) {
		printf("Can't alloc Session.\n" );
		exit( -1 );
	}

	j = atoi( av[2] );

	Ret = PaxosSessionLock( pSession, j, &pH );
	if( Ret ) {
		printf("SessionLock error j=%d\n", j );
		exit( -1 );
	}
	Ret = PFSReadCntrl( pSession, pH, &CNTRL, &pCNTRL );
	Ret = PFSReadMem( pSession, pH, CNTRL.c_pAcceptor, sizeof(Acceptor), 
											&Acceptor );

	if( !strcmp( "accept", av[3] ) ) {
		Ret = ReadAccept( pSession, pH );
	} else if( !strcmp( "path", av[3] ) ) {
		Ret = ReadPath( pSession, pH );
	} else if( !strcmp( "lock", av[3] ) ) {
		Ret = ReadLock( pSession, pH );
	} else if( !strcmp( "queue", av[3] ) ) {
		Ret = ReadQueue( pSession, pH );
	} else if( !strcmp( "outofband", av[3] ) ) {
		Ret = ReadOutOfBand( pSession, pH );
	} else if( !strcmp( "meta", av[3] ) ) {
		Ret = ReadMeta( pSession, pH );
	} else if( !strcmp( "fd", av[3] ) ) {
		Ret = ReadFd( pSession, pH );
	} else if( !strcmp( "block", av[3] ) ) {
		Ret = ReadBlock( pSession, pH );
	} else if( !strcmp( "ras", av[3] ) ) {
		if( CNTRL.c_bRas )	Ret = 0;
		else				Ret = -1;
	} else {
		usage();
	}

	PaxosSessionUnlock( pSession, pH );
	PaxosSessionFree( pSession );
	return( Ret );
}
