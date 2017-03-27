/******************************************************************************
*
*  CIFSProbeCoS.c 	--- probe program of CIFS
*
*  Copyright (C) 2016 triTech Inc. All Rights Reserved.
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


#define	_CIFS_SERVER_
#define	_PAXOS_SESSION_SERVER_
#include	"CNV.h"
#include	"CIFS.h"
#include	<stdio.h>
#include	<ctype.h>

CtrlCoS_t	CNTRL, *pCNTRL;
PaxosAcceptor_t	Acceptor;

int
CIFSReadCNTRL( struct Session* pSession, void* pH, 
					CtrlCoS_t* pCntrl, CtrlCoS_t** ppCntrl )
{
	CnvMemRootReq_t	RootReq;
	CnvMemRootRpl_t	*pRootRpl;
	void	*pRoot;
	int	Ret;

	memset( &RootReq, 0, sizeof(RootReq) );
	SESSION_MSGINIT( &RootReq.r_Head, CNV_MEM_ROOT, 0, 0, sizeof(RootReq) );
	strcpy( RootReq.r_Name, "CIFS" );
	
	pRootRpl = (CnvMemRootRpl_t*)
			PaxosSessionAny( pSession, pH, (PaxosSessionHead_t*)&RootReq );
	if( pRootRpl->r_Head.h_Error ) {
		errno = pRootRpl->r_Head.h_Error;
		Ret = -1;
		goto err1;
	}
	pRoot	= pRootRpl->r_pRoot;
	*ppCntrl	= pRoot;

	Ret = PaxosSessionDumpMem( pSession, pH, pRoot, pCntrl, sizeof(*pCntrl)); 

	Free( pRootRpl );
	return( Ret );
err1:
	if( pRootRpl )	Free( pRootRpl );
	return( Ret );
}

int
PrintClientId( PaxosClientId_t* pId )
{
	printf("ClientId [%s-%d] Pid=%u-%d Seq=%d Reuse=%d Try=%d\n", 
		inet_ntoa( pId->i_Addr.sin_addr ), ntohs( pId->i_Addr.sin_port ),
		pId->i_Pid, pId->i_No, pId->i_Seq, pId->i_Reuse, pId->i_Try );
	return( 0 );
}

int
PrintAcceptorFdEvent( PaxosAcceptorFdEvent_t *pEv )
{
#ifdef	ZZZ
	printf("Cnt=%d Fd=%d events=0x%x\n", 
			pEv->e_Cnt, pEv->e_IOSock.s_Fd, pEv->e_FdEvent.fd_Events );
#else
	printf("Fd=%d events=0x%x\n", 
			pEv->e_IOSock.s_Fd, pEv->e_FdEvent.fd_Events );
#endif
	return( 0 );
}

int
DumpConCoS( struct Session* pSession, void* pH,
				ConCoS_t *pConCoS, ConCoS_t *pOrg )
{
	PIDMID_t	*pPIDMID, PIDMID;
	PairFID_t	*pPairFID, PairFID;
	PairTID_t	*pPairTID, PairTID;

	printf("=== ConCos[%p] ===\n", pOrg );
	printf("Meta-Real:Session[%u-%u] UID[%u-%u] TID[%u-%u]\n",
			pConCoS->s_SessionKeyMeta,	pConCoS->s_SessionKey,
			pConCoS->s_UIDMeta,			pConCoS->s_UID,
			pConCoS->s_TIDMeta,			pConCoS->s_TID );

	// PIDMID
	LIST_FOR_EACH_ENTRY( pPIDMID, &pConCoS->s_PIDMID.h_Lnk, pm_Lnk, 
												pConCoS, pOrg ) {
		printf("--- ConCos[%p] ---\n", pPIDMID );

		PaxosSessionDumpMem( pSession, pH, pPIDMID, &PIDMID, sizeof(PIDMID) );

		printf("PID[0x%x] MID[0x%x] State[0x%x] Notify[%d]\n", 
			PIDMID.pm_PID, PIDMID.pm_MID, PIDMID.pm_State, PIDMID.pm_bNotify );
		printf("TotalParameterCountReq[%u]\n",PIDMID.pm_TotalParameterCountReq);
		printf("TotalDataCountReq[%u]\n",PIDMID.pm_TotalDataCountReq);
		printf("MaxParameterCountReq[%u]\n",PIDMID.pm_MaxParameterCountReq);
		printf("MaxDataCountReq[%u]\n",PIDMID.pm_MaxDataCountReq);
		printf("TotalParameterCountRes[%u]\n",PIDMID.pm_TotalParameterCountRes);
		printf("TotalDataCountRes[%u]\n",PIDMID.pm_TotalDataCountRes);

		printf("--- ConCoS END ---\n" );

		pPIDMID = &PIDMID;
	}
	// PairFID
	printf("--- FID ---\n" );
	LIST_FOR_EACH_ENTRY( pPairFID, &pConCoS->s_Pair.s_LnkFID, p_Lnk, 
												pConCoS, pOrg ) {

		PaxosSessionDumpMem(pSession, pH, 
						pPairFID, &PairFID, sizeof(PairFID_t));

		printf("--- PairFID[%p] Cnt=%d Meta=0x%x FID=0x%x ---\n", 
				pPairFID, PairFID.p_Cnt, PairFID.p_FIDMeta, PairFID.p_FID );

		pPairFID	= &PairFID;
	}
	printf("--- FID END ---\n" );
	// PairTID
	printf("--- TID ---\n" );
	LIST_FOR_EACH_ENTRY( pPairTID, &pConCoS->s_Pair.s_LnkTID, p_Lnk, 
												pConCoS, pOrg ) {

		PaxosSessionDumpMem(pSession, pH, 
								pPairTID, &PairTID, sizeof(PairTID_t));

		printf("--- PairTID[%p] Cnt=%d Meta=0x%x TID=0x%x ---\n", 
				pPairTID, PairTID.p_Cnt, PairTID.p_TIDMeta, PairTID.p_TID );

		pPairTID	= &PairTID;
	}
	printf("--- TID END ---\n" );
	printf("=== ConCos END ===\n" );
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
//	SessionAdaptor_t	*pAdaptor, Adaptor;
	ConCoS_t	*pConCoS, ConCoS;
	
	int	N;
	int	Family;

	PrintClientId( &pAccept->a_Id );
printf("Cnt=%d pFd=%p Opened=%d Start=%u End=%u Reply=%u Cmd=0x%x Events=%d(%d) Create=%u Access=%u\n",
		pAccept->a_GE.e_Cnt, pAccept->a_pFd, pAccept->a_Opened, 
		pAccept->a_Accepted.a_Start, pAccept->a_Accepted.a_End, 
		pAccept->a_Accepted.a_Reply, pAccept->a_Accepted.a_Cmd,
		pAccept->a_Events.e_Cnt, pAccept->a_Events.e_Send, 
		(uint32_t)pAccept->a_Create, (uint32_t)pAccept->a_Access );

		if( pAccept->a_pFd ) {
			PaxosSessionDumpMem( pSession, pH, 
					(void*)pAccept->a_pFd, (void*)&Ev, sizeof(Ev) );
			PrintAcceptorFdEvent( &Ev );
		}

if( bAdaptor ) {
	N	= pAccept->a_Tag.h_N;
	paL	= (list_t*)Malloc( sizeof(list_t)*N );
	PaxosSessionDumpMem( pSession, pH, 
			pAccept->a_Tag.h_aHeads, paL, sizeof(list_t)*N );

	for( ind = 0; ind < N; ind++ ) {

		LIST_FOR_EACH_ENTRY( pItem, &paL[ind], i_Link, 
									paL, pAccept->a_Tag.h_aHeads ) {

			PaxosSessionDumpMem( pSession, pH, pItem, &Item, sizeof(Item) );

			Family		= (uintptr_t)Item.i_pKey;
			if( Family == CNV_FAMILY ) {
				pConCoS	= (ConCoS_t*)Item.i_pValue;

				PaxosSessionDumpMem( pSession, pH, 
					pConCoS, &ConCoS, sizeof(ConCoS) );

				DumpConCoS( pSession, pH, &ConCoS, pConCoS );
			} else {
				printf("### Family = 0x%x ###\n", Family );
			}
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
	int	Ret;

	Ret = PaxosSessionDumpMem( pSession, pH, 
		CNTRL.s_Ctrl.s_pAcceptor, &Acceptor, sizeof(Acceptor) );

	printf("### Accept ###\n");
	LIST_FOR_EACH_ENTRY( pAccept, &Acceptor.c_Accept.ge_Lnk, a_GE.e_Lnk,
								&Acceptor, CNTRL.s_Ctrl.s_pAcceptor ) {

		PaxosSessionDumpMem( pSession, pH, pAccept, &Accept, sizeof(Accept) );
		DumpAccept( pSession, pH, &Accept, pAccept, TRUE );
		pAccept = &Accept;
	}
	printf("### Accept(END) ###\n");
	return( Ret );
}

int
CIFSReadPIDMID( struct Session* pSession, void* pH )
{
	return( 0 );
}

#ifdef	YYY
int
DumpFd( struct Session *pSession, void *pH, GFd_t *pFd, GFd_t *pOrg )
{
#ifdef	ZZZ
	printf("%p e_Cnt=%d Name[%s] Dir[%s] Base[%s] Fd[%d] st_size[%"PRIu64"]\n",
		pOrg, pFd->f_GE.e_Cnt, pFd->f_Name, pFd->f_DirName, pFd->f_BaseName, 
		pFd->f_Fd, pFd->f_Stat.st_size );
#else
	printf("%p e_Cnt=%d Name[%s] Dir[%s] Base[%s] Fd[%d]\n",
		pOrg, pFd->f_GE.e_Cnt, pFd->f_Name, pFd->f_DirName, pFd->f_BaseName, 
		pFd->f_Fd );
#endif
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
#endif	// YYY

int
usage()
{
printf("CIFSProbeCoS cell id {accept}\n");
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
	Ret = CIFSReadCNTRL( pSession, pH, &CNTRL, &pCNTRL );
	Ret = PaxosSessionDumpMem( pSession, pH, CNTRL.s_Ctrl.s_pAcceptor, 
								&Acceptor, sizeof(Acceptor) );

	if( !strcmp( "accept", av[3] ) ) {
		Ret = ReadAccept( pSession, pH );
	} else {
		usage();
	}

	PaxosSessionUnlock( pSession, pH );
	PaxosSessionFree( pSession );
	return( Ret );
}
