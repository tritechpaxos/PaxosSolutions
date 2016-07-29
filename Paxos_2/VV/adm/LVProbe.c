/******************************************************************************
*
*  LVProbe.c 	--- probe program of LVServer 
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


#define	_PAXOS_SESSION_SERVER_
#include	"LV.h"
#include	<stdio.h>
#include	<ctype.h>

int
LVReadCntrl( struct Session* pSession, void* pH, 
					LVCntrl_t* pCntrl, LVCntrl_t** ppCntrl )
{
	LVCntrl_req_t	Req;
	LVCntrl_rpl_t	*pRpl;
	int	Ret;

	SESSION_MSGINIT( &Req, LV_CNTRL, 0, 0, sizeof(Req) );
	pRpl = (LVCntrl_rpl_t*)
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
LVReadMem( struct Session* pSession, void* pH, 
				void* pStart, int Size, void* pData )
{
	LVDump_req_t	Req;
	LVDump_rpl_t	*pRpl;
	int	Ret;

	SESSION_MSGINIT( &Req, LV_DUMP, 0, 0, sizeof(Req) );
	Req.d_pAddr	= pStart;
	Req.d_Len		= Size;
	pRpl = (LVDump_rpl_t*)
			PaxosSessionAny( pSession, pH, (PaxosSessionHead_t*)&Req );
	if( pRpl->d_Head.h_Error ) {
		errno = pRpl->d_Head.h_Error;
		Ret = -1;
	} else {
		if(!pData) abort();
		memcpy( pData, (pRpl+1), Size );
		Ret = 0;
	}
	Free( pRpl );
	return( Ret );
}

LVCntrl_t	CNTRL, *pCNTRL;
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
PrintAcceptorFdEvent( PaxosAcceptorFdEvent_t *pEv )
{
	printf("Fd=%d events=0x%x\n", 
			pEv->e_IOSock.s_Fd, pEv->e_FdEvent.fd_Events );
	return( 0 );
}

int
DumpAccept( struct Session* pSession, void* pH,
		PaxosAccept_t* pAccept, PaxosAccept_t* pOrg )
{
	PaxosAcceptorFdEvent_t	Ev;

	PrintClientId( &pAccept->a_Id );
printf("Cnt=%d pFd=%p Opened=%d Start=%u End=%u Reply=%u Cmd=0x%x Events=%d(%d) Create=%u Access=%u\n",
		pAccept->a_GE.e_Cnt, pAccept->a_pFd, pAccept->a_Opened, 
		pAccept->a_Accepted.a_Start, pAccept->a_Accepted.a_End, 
		pAccept->a_Accepted.a_Reply, pAccept->a_Accepted.a_Cmd,
		pAccept->a_Events.e_Cnt, pAccept->a_Events.e_Send, 
		(uint32_t)pAccept->a_Create, (uint32_t)pAccept->a_Access );

	if( pAccept->a_pFd ) {
		LVReadMem( pSession, pH, pAccept->a_pFd, sizeof(Ev), &Ev );
		PrintAcceptorFdEvent( &Ev );
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
		LVReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept );
		pAccept = &Accept;
	}
	printf("=== Accept(END) ===\n");
	return( 0 );
}

int
DumpOut( struct Session *pSession, void *pH,
				AcceptorOutOfBand_t	*pOut, AcceptorOutOfBand_t *pOrg )
{
	PaxosSessionHead_t	Head;

	if( pOut->o_pData ) {
		LVReadMem( pSession, pH, pOut->o_pData, sizeof(Head), &Head );
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
		LVReadMem( pSession, pH, pOut, sizeof(Out), &Out );
		DumpOut( pSession, pH, &Out, pOut );
		pOut = &Out;
	}
	printf("=== OUT_OF_BAND(END) ===\n");
	return( 0 );
}

int
DumpFdMeta( struct Session *pSession, void *pH, 
				GFdMeta_t *pFd, GFdMeta_t *pOrg )
{
	printf("%p e_Cnt=%d Name[%s] Fd[%d] st_size[%zu]\n",
	pOrg, pFd->fm_GE.e_Cnt, pFd->fm_Name, pFd->fm_Fd, pFd->fm_Stat.st_size );
	return( 0 );
}
int
ReadFdMeta( struct Session* pSession, void* pH )
{
	GFdMeta_t	Fd, *pFd;

	printf("### FdMetaCache Cnt=%d FdMetaMax=%d Root[%s] ###\n", 
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_Cnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_MaxCnt,
			CNTRL.c_BlockCache.bc_FdCtrl.fc_Root );
			
	LIST_FOR_EACH_ENTRY( pFd, 
				&CNTRL.c_BlockCache.bc_FdCtrl.fc_Meta.ge_Lnk, fm_GE.e_Lnk,
								&CNTRL, pCNTRL ) {
		LVReadMem( pSession, pH, pFd, sizeof(Fd), &Fd );
		DumpFdMeta( pSession, pH, &Fd, pFd );
		pFd = &Fd;
	}
	printf("=== FdMetaCache(END) ===\n");
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
		LVReadMem( pSession, pH, pFd, sizeof(Fd), &Fd );
		DumpFd( pSession, pH, &Fd, pFd );
		pFd = &Fd;
	}
	printf("=== FdCache(END) ===\n");
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

	pBuf		= Malloc(BLOCK_CACHE_SIZE(pBC));
	pValidTime	= (timespec_t*)
					Malloc(sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC));
	pCksum64	= (uint64_t*)Malloc(sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBC));

	LVReadMem( pSession, pH, pB->b_pBuf, BLOCK_CACHE_SIZE(pBC), pBuf );
	LVReadMem( pSession, pH, pB->b_pValid,
					sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC), pValidTime );
	LVReadMem( pSession, pH, pB->b_pCksum64,
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

		LVReadMem( pSession, pH, pB, sizeof(B), &B );
		DumpBlock( pSession, pH, &B, pB, &CNTRL.c_BlockCache );
		pB = &B;
	}
	printf("=== BlockCache(END) ===\n");
	return( 0 );
}

int
usage()
{
	printf("LVProbe cell id {accept|outofband|meta|fd|block}\n");
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
		printf("Attach error j=%d\n", j );
		exit( -1 );
	}
	Ret = LVReadCntrl( pSession, pH, &CNTRL, &pCNTRL );
	Ret = LVReadMem( pSession, pH, CNTRL.c_pAcceptor, sizeof(Acceptor), 
											&Acceptor );

	if( !strcmp( "accept", av[3] ) ) {
		Ret = ReadAccept( pSession, pH );
	} else if( !strcmp( "outofband", av[3] ) ) {
		Ret = ReadOutOfBand( pSession, pH );
	} else if( !strcmp( "meta", av[3] ) ) {
		Ret = ReadFdMeta( pSession, pH );
	} else if( !strcmp( "fd", av[3] ) ) {
		Ret = ReadFd( pSession, pH );
	} else if( !strcmp( "block", av[3] ) ) {
		Ret = ReadBlock( pSession, pH );
	} else {
		usage();
	}

	PaxosSessionUnlock( pSession, pH );
	PaxosSessionFree( pSession );

	return( 0 );
}
