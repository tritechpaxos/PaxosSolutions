/******************************************************************************
*
*  PaxosSessionProbe.c 	--- probe program of Session
*
*  Copyright (C) 2012-2016 triTech Inc. All Rights Reserved.
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
#define	_PAXOS_SERVER_
#include	"PaxosSession.h"
#include	<stdio.h>

PaxosAcceptor_t	Acceptor;
PaxosAcceptor_t	*pAcceptor;

int
ReadAcceptor( struct Session* pSession, void* pH, 
			PaxosAcceptor_t* pAcceptor, PaxosAcceptor_t** ppAcceptor )
{
	PaxosSessionAcceptorReq_t	Req;
	PaxosSessionAcceptorRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, PAXOS_SESSION_ACCEPTOR, 0, 0, sizeof(Req) );
	Req.a_Head.h_Id	= *PaxosSessionGetClientId( pSession );
	PaxosSessionGetMyAddr( pSession, pH, &Req.a_Head.h_Id.i_Addr );

	Ret = SendStream( PNT_INT32(pH), (char*)&Req, sizeof(Req) );

	Ret = RecvStream( PNT_INT32(pH), (char*)&Rpl, sizeof(Rpl) );

	*pAcceptor	= Rpl.a_Acceptor;
	*ppAcceptor	= Rpl.a_pAcceptor;

	return( Ret );
}

int
ReadMem( struct Session* pSession, void* pH, 
				void* pStart, int Size, void* pData )
{
	PaxosSessionDumpReq_t	Req;
	int	Ret;

	SESSION_MSGINIT( &Req, PAXOS_SESSION_DUMP, 0, 0, sizeof(Req) );
	Req.d_Head.h_Id	= *PaxosSessionGetClientId( pSession );
	PaxosSessionGetMyAddr( pSession, pH, &Req.d_Head.h_Id.i_Addr );

	Req.d_pStart	= pStart;
	Req.d_Size		= Size;

	Ret = SendStream( PNT_INT32(pH), (char*)&Req, sizeof(Req) );

	Ret = RecvStream( PNT_INT32(pH), (char*)pData, Size );

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
PrintHead( PaxosSessionHead_t* pH )
{
	PrintClientId( &pH->h_Id );
	return( 0 );
}

int
PrintFdEvent( FdEvent_t *pEv )
{
	printf("fd=%d enents=0x%x\n", pEv->fd_Fd, pEv->fd_Events );
	return( 0 );
}

int
ReadFdEvent( struct Session* pSession, void* pH )
{
	FdEvent_t	Ev, *pEv;

	printf("### FdEvent Cnt=%d ###\n", 
						Acceptor.c_FdEvent.f_Ctrl.ct_Cnt );
#ifdef	_NOT_LIBEVENT_
	LIST_FOR_EACH_ENTRY( pEv, &Acceptor.c_FdEvent.f_Ctrl.ct_Lnk, fd_Lnk,
								&Acceptor, pAcceptor ) {
#else
	LIST_FOR_EACH_ENTRY( pEv, &Acceptor.c_FdEvent.f_Ctrl.ct_HashLnk.hl_Lnk, 
								fd_Lnk, &Acceptor, pAcceptor ) {
#endif
		ReadMem( pSession, pH, pEv, sizeof(Ev), &Ev );

		PrintFdEvent( &Ev );

		pEv = &Ev;
	}
	printf("=== FdEvent(END) ===\n");
	return( 0 );
}

int
DumpAccept( struct Session* pSession, void* pH,
		PaxosAccept_t* pAccept, PaxosAccept_t* pOrg )
{
	printf("=== Accept ===\n");
	PrintClientId( &pAccept->a_Id );
printf("Cnt=%d pFd=%p Opened=%d Start=%u End=%u Reply=%u Cmd=0x%x Events=%d(%d) Create=%u Access=%u\n",
		pAccept->a_GE.e_Cnt, pAccept->a_pFd, pAccept->a_Opened, 
		pAccept->a_Accepted.a_Start, pAccept->a_Accepted.a_End, 
		pAccept->a_Accepted.a_Reply, pAccept->a_Accepted.a_Cmd,
		pAccept->a_Events.e_Cnt, pAccept->a_Events.e_Send, 
		(uint32_t)pAccept->a_Create, (uint32_t)pAccept->a_Access );

	return( 0 );
}

int
ReadAccept( struct Session* pSession, void* pH )
{
	PaxosAccept_t	*pAccept, Accept;

	printf("#### ACCEPT Cnt=%d ####\n", Acceptor.c_Accept.ge_Cnt );	

	LIST_FOR_EACH_ENTRY( pAccept, &Acceptor.c_Accept.ge_Lnk, a_GE.e_Lnk,
								&Acceptor, pAcceptor ) {
		ReadMem( pSession, pH, pAccept, sizeof(Accept), &Accept );
		DumpAccept( pSession, pH, &Accept, pAccept );
		pAccept = &Accept;
	}
	return( 0 );
}

int
ReadCell( struct Session *pSession, void* pH )
{
	int	j;
	PaxosSessionCell_t	Cell, *pCell;
	int	l;
	Paxos_t	Paxos;
	Server_t	*pServer;

	/*
	 *	Paxos Cell
	 */
	// Read Paxos_t
	ReadMem( pSession, pH, Acceptor.c_pPaxos, sizeof(Paxos), &Paxos );
	// Read Server_t
	l	= Paxos.p_Maximum*sizeof(Server_t);
	pServer = (Server_t*)Malloc( l );
	ReadMem( pSession, pH, Paxos.p_aServer, l, pServer );

	ReadMem( pSession, pH, Acceptor.c_pSessionCell, sizeof(Cell), &Cell );
	l = PaxosCellSize(Cell.c_Maximum);
	pCell = (PaxosSessionCell_t*)Malloc( l );
	ReadMem( pSession, pH, Acceptor.c_pSessionCell, l, pCell );

	printf("#### Cell Addr[%s] ###\n", pCell->c_Name );
	for( j = 0; j < pCell->c_Maximum; j++ ) {
		printf("%d:Seesion(TCP)[%s](%d) Paxos(UDP)[%s](%d)\n", j, 
		inet_ntoa( pCell->c_aPeerAddr[j].a_Addr.sin_addr),
		ntohs( pCell->c_aPeerAddr[j].a_Addr.sin_port),
		inet_ntoa( pServer[j].s_ChSend.c_PeerAddr.sin_addr),
		ntohs( pServer[j].s_ChSend.c_PeerAddr.sin_port) );
	}
	return( 0 );
}

int
DumpOut( struct Session *pSession, void *pH,
				AcceptorOutOfBand_t	*pOut, AcceptorOutOfBand_t *pOrg )
{
	printf("=== OutOfBand Validate[%d] Seq[%u] pData[%p] ===\n", 
			pOut->o_bValidate, pOut->o_Seq, pOut->o_pData );
	PrintClientId( &pOut->o_Id );
	return( 0 );
}

int
ReadOutOfBand( struct Session* pSession, void* pH )
{
	AcceptorOutOfBand_t	*pOut, Out;

	printf("#### OUT_OF_BAND Cnt=%d ####\n", Acceptor.c_OutOfBand.o_Cnt );	
	LIST_FOR_EACH_ENTRY( pOut, &Acceptor.c_OutOfBand.o_Lnk, o_Lnk,
								&Acceptor, pAcceptor ) {
		ReadMem( pSession, pH, pOut, sizeof(Out), &Out );
		DumpOut( pSession, pH, &Out, pOut );
		pOut = &Out;
	}
	return( 0 );
}

int
usage()
{
	printf("PaxosSessionProbe cell id {FdEvent|accept|out_of_band|cell}\n");
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
/*
 *	Initialize
 */
	void*	pH;
	struct Session*	pSession;

	pSession = PaxosSessionAlloc(
			ConnectAdm, Shutdown, CloseFd, GetMyAddr, Send, Recv,
			NULL, NULL, av[1] );

	j = atoi( av[2] );

	Ret = PaxosSessionLock( pSession, j, &pH );
	if( Ret ) {
		printf("Attach error j=%d\n", j );
		exit( -1 );
	}
	Ret = ReadAcceptor( pSession, pH, &Acceptor, &pAcceptor );

	if( !strcmp( "FdEvent", av[3] ) ) {
		Ret = ReadFdEvent( pSession, pH );
	} else if( !strcmp( "accept", av[3] ) ) {
		Ret = ReadAccept( pSession, pH );
	} else if( !strcmp( "out_of_band", av[3] ) ) {
		Ret = ReadOutOfBand( pSession, pH );
	} else if( !strcmp( "cell", av[3] ) ) {
		ReadCell( pSession, pH );
	} else {
		usage();
	}

	PaxosSessionUnlock( pSession, pH );

	PaxosSessionFree( pSession );
	return( 0 );
}
