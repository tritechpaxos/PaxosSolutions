/******************************************************************************
*
*  CoC.c 	--- converter program of Client side
*
*  Copyright (C) 2011,2016 triTech Inc. All Rights Reserved.
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
 *	試験
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

#include	"CNV.h"
#include	<stdio.h>
#include	<netinet/tcp.h>
#include	<iconv.h>

#include	"../src/CIFS.h"

CnvCtrlCoC_t	*pCNTRL;

int	MyId;	// dummy

SERVICE

PaxosSessionCell_t	SessionCell;
struct Log			*pLog;
iconv_t	iConv;

int
CNVEventSessionBind( struct Session *pSession, struct Session *pEvent )
{
	CnvEventSessionBindReq_t	ReqBind;
	CnvEventSessionBindRpl_t	*pRplBind;
	PaxosClientId_t	*pId;
	Msg_t	*pReq, *pRpl;
	MsgVec_t	Vec;
	int	Ret;

	SESSION_MSGINIT( &ReqBind.eb_Head, CNV_EVENT_BIND, 0, 0, sizeof(ReqBind) );
	pId	= PaxosSessionGetClientId( pEvent );
	ReqBind.eb_EventId	= *pId;

	MsgVecSet(&Vec, VEC_HEAD|VEC_REPLY, &ReqBind, 
							sizeof(ReqBind), sizeof(ReqBind));

	pReq	= MsgCreate( 1, Malloc, Free );
	MsgAdd( pReq, &Vec );

	pRpl = PaxosSessionReqAndRplByMsg(pSession, pReq, TRUE, TRUE);

	ASSERT( pRpl );

	pRplBind = (CnvEventSessionBindRpl_t*)MsgFirst( pRpl );
	Ret	= pRplBind->eb_Head.h_Error;

	MsgDestroy( pRpl);

	return( Ret );
}

Msg_t*
RequestAndReplyByMsg( 
	FdEvCoC_t		*pClient,
	Msg_t*			pReq,
	int				CNV_Cmd,
	int				*pStatus )
{
	PaxosSessionHead_t	Push, Head, Out, *pHead;
	int	Len;
	MsgVec_t	Vec;
	Msg_t		*pRpl = NULL;
	bool_t		bUpdate;

	Len	= 0;
	if( pReq )	Len	= MsgTotalLen( pReq );
	Len	+= sizeof(Head);

	if( CNV_Cmd )	SESSION_MSGINIT( &Head, CNV_Cmd, 0, 0, Len );
	else			SESSION_MSGINIT( &Head, CNV_REFER, 0, 0, Len );
LOG( pLog, LogDBG, "CNV_Cmd=0x%x Len=%d", CNV_Cmd, Len );

	if( CNV_Cmd == CNV_UPDATE )	bUpdate	= TRUE;
	else						bUpdate	= FALSE;

	MsgVecSet(&Vec, VEC_HEAD|VEC_REPLY, &Head, sizeof(Head), sizeof(Head));
	if( pReq ) {
		MsgInsert( pReq, &Vec );
	} else {
		pReq	= MsgCreate( 1, Malloc, Free );
		MsgAdd( pReq, &Vec );
	}
#ifdef	ZZZ
#else
//{
//int i;
//for( i = 0; i < pReq->m_N; i++ ) {
//	LOG(pLog, LogDBG, "%d: vv_Flag=0x%x _Len=%d", 
//i, pReq->m_aVec[i].v_Flags, pReq->m_aVec[i].v_Len );
//}
//}
#endif

	if( Len > PAXOS_COMMAND_SIZE ) {

		SESSION_MSGINIT( &Push, PAXOS_SESSION_OB_PUSH, 0, 0, 
							sizeof(Push) + Len );

		MsgVecSet( &Vec, VEC_HEAD, &Push, sizeof(Push), sizeof(Push) );
		MsgInsert( pReq, &Vec );

		SESSION_MSGINIT( &Out, CNV_OUTOFBAND, 0, 0, sizeof(Out) );
		MsgVecSet(&Vec, VEC_HEAD, &Out, sizeof(Out), sizeof(Out));
		MsgAdd( pReq, &Vec );
/*	
 *	PUSH + ( CNV_UPDATE/REFER + Body ) + CNV_OUTOFBAND 
 */
	}

	pRpl = PaxosSessionReqAndRplByMsg(pClient->c_pSession, pReq, bUpdate, TRUE);

	ASSERT( pRpl );

	pHead = (PaxosSessionHead_t*)MsgFirst( pRpl );

#ifdef	ZZZ
	if( pHead->h_Error == CNV_PSUEDO_RPL ) {
LOG(pLog,LogDBG,"CNV_PSUEDO_RPL");
		MsgDestroy( pRpl);
		return( NULL );
	} else {
		MsgTrim( pRpl, sizeof(Head) );	// Trim Head
		return( pRpl );
	}
#else
	*pStatus	= pHead->h_Error;

LOG(pLog,LogDBG,"Status=0x%x", *pStatus );
	MsgTrim( pRpl, sizeof(Head) );	// Trim Head
	return( pRpl );
#endif
}

int
ReplyToClientByMsg( struct Session *pSession, int fd, Msg_t *pRpl )
{
	int	Ret = 0;
	char	*pBuf;

	for( pBuf = MsgFirst( pRpl ); pBuf; pBuf = MsgNext( pRpl ) ) {
DBG_TRC("MsgLen=%d\n", MsgLen(pRpl) );
		Ret	= SendStream( fd, pBuf, MsgLen( pRpl ) );
	}
	return( Ret );
}

void*
ThreadWorker( void* pArg )
{
	FdEvCoC_t	*pCon;
	Msg_t		*pMsg;
	int			CNV_cmd;
	Msg_t		*pRpl;
	int			Ret;
	int			Status;

	pthread_detach( pthread_self() );

	while( 1 ) {

		QueueWaitEntry( &pCNTRL->c_Worker, pCon, c_LnkQ );
		if( !pCon )	goto err;

		RwLockR( &pCNTRL->c_RwLock );

		LOG( pLog, LogDBG, "<<< WORKER START >>>" );

		while( 1 ) {
			MtxLock( &pCon->c_Mtx );

			CNV_cmd	= CNV_UPDATE;

			pMsg = list_first_entry(&pCon->c_MsgList, Msg_t, m_Lnk);

LOG( pLog, LogDBG, 
"#### pCon=%p c_Flags=0x%x pMsg=%p ####", 
pCon, pCon->c_Flags, pMsg );

			if( !pMsg ) {

				pCon->c_Flags &= ~FDEVCON_WORKER;
				CndBroadcast( &pCon->c_Cnd );

				MtxUnlock( &pCon->c_Mtx );

				goto next;
			}
			list_del_init( &pMsg->m_Lnk );

			MtxUnlock( &pCon->c_Mtx );

			/*
			 * Ret	= -1	error
			 *		= 0		sendo to CoS
			 *		= 1		pending/not send
			 */
			Ret = (*pIF->IF_fRequest)( pCon, pMsg, &CNV_cmd );
			if( Ret < 0 ) {
				LOG( pLog, LogERR, "EXIT errno=%d", errno );
				goto err1;
			} else if( Ret == 0 ) {	// Send Request
				/*
				 *	Ret		= 1		send psuedo request to server
				 *			= 0		normal
				 *  				Status == CNV_PSUEDO_RPL 
				 *						psuedo response/no reply to client
				 */
request_again:
				pRpl = RequestAndReplyByMsg( pCon, pMsg, CNV_cmd, &Status );

				if( Status != CNV_PSUEDO_RPL ) {

					Ret = (*pIF->IF_fReply)( pCon, pRpl, Status );
					if( Ret > 0 ) {
						CNV_cmd	= CNV_PSUEDO_REQ;
						goto request_again;
					}
				}
			}
#ifdef	ZZZ
			if( Ret < 0 ) {
				LOG( pLog, LogERR, "shutdown:c_type=%d Ret=%d", Type, Ret );

				MtxLock( &pEvCon->c_Mtx );

				pEvCon->c_Flags |= FDEVCON_ABORT;
				shutdown( pEvCon->c_FdEventCtrl.fd_Fd, SHUT_RDWR );

				MtxUnlock( &pEvCon->c_Mtx );
			}
#endif
err1:
			if( pRpl )	MsgDestroy( pRpl );
			MsgDestroy( pMsg );
			MtxUnlock( &pCon->c_Mtx );
		}
next:
		LOG( pLog, LogDBG, "<<< WORKER END >>>" );

		RwUnlock( &pCNTRL->c_RwLock );
	}
err:
	pthread_exit( 0 );
}

/*
 *	クライアントの要求毎にconnectionを張る
 */
FdEvCoC_t*
FdEvCoCAlloc( int Fd, int Type, int Epoll, void *pTag, 
				int (*fHandler)(FdEvent_t*,FdEvMode_t) )
{
	FdEvCoC_t	*pFdEvCoC;

	pFdEvCoC = (FdEvCoC_t*)(*pIF->IF_fConAlloc)();
	if( !pFdEvCoC )	goto err;

	memset( pFdEvCoC, 0, sizeof(*pFdEvCoC) );
	list_init( &pFdEvCoC->c_LnkQ );
	MtxInit( &pFdEvCoC->c_Mtx );
	CndInit( &pFdEvCoC->c_Cnd );
	list_init( &pFdEvCoC->c_MsgList );

	QueueInit( &pFdEvCoC->c_QueueNormal );
	QueueInit( &pFdEvCoC->c_QueueEvent );

	FdEventInit( &pFdEvCoC->c_FdEvent, Type, Fd, Epoll, pTag, fHandler );

	return( pFdEvCoC );
err:
	return( NULL );
}

void
FdEvCoCFree( FdEvCoC_t *pFdEvCoC )
{
//	Free( pFdEvCoC );
	(*pIF->IF_fConFree)( pFdEvCoC );
}

int
RecvHandler( FdEvent_t *pEv, FdEvMode_t Mode )	// from Client
{
	FdEvCoC_t	*pCon = container_of( pEv, FdEvCoC_t, c_FdEvent ); 
	int Fd = pEv->fd_Fd;
	CnvCtrlCoC_t	*pCnvCtrl = (CnvCtrlCoC_t*)pEv->fd_pArg;
	Msg_t	*pMsg;

	pMsg = (*pIF->IF_fRecvMsgByFd)( Fd, Malloc, Free );
	if( !pMsg ) {

		LOG( pLog, LogERR, "fRecv ERROR pCon=%p Type=%d Fd=%d", 
				pCon, pCon->c_FdEvent.fd_Type, Fd );

		MtxLock( &pCon->c_Mtx );

		pCon->c_Flags	|= FDEVCON_ABORT;

		while( pCon->c_Flags & FDEVCON_WORKER ) {
			LOG( pLog, LogERR, "WAIT WORKER" );
			CndWait( &pCon->c_Cnd, &pCon->c_Mtx );
		}
		(*pIF->IF_fSessionClose)( pCon );

		MtxUnlock( &pCon->c_Mtx );

		FdEventDel( pEv );

		close( Fd );

		FdEvCoCFree( pCon );

		return( 0 );
	}

	// Post Data To Worker
	MtxLock( &pCon->c_Mtx );

	list_add_tail( &pMsg->m_Lnk, &pCon->c_MsgList );

	if( !(pCon->c_Flags & FDEVCON_WORKER) ) {

		pCon->c_Flags |= FDEVCON_WORKER;

		QueuePostEntry( &pCnvCtrl->c_Worker, pCon, c_LnkQ );
	}
	MtxUnlock( &pCon->c_Mtx );

	return( 0 );
}

int
AcceptHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
//	FdEvCoC_t	*pFdEvAccept = container_of( pEv, FdEvCoC_t, c_FdEvent ); 
	int ListenFd = pEv->fd_Fd;
	CnvCtrlCoC_t	*pCnvCtrl = (CnvCtrlCoC_t*)pEv->fd_pArg;
	int	FdClient;
	int flags = 1;
	struct linger	Linger = { 1/*On*/, 10/*sec*/};
	struct sockaddr	AddrClient;
	socklen_t	len;
	uint64_t	U64C;
	FdEvCoC_t	*pConClient;
	int	Ret;

	DBG_ENT();

	if( Mode != EV_LOOP )	return( 0 );

	memset( &AddrClient, 0, sizeof(AddrClient) );
	len	= sizeof(AddrClient);

	FdClient	= accept( ListenFd, &AddrClient, &len );
	if( FdClient < 0 )	goto err;

	/* Set Linger */
	setsockopt( FdClient, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger) );
	setsockopt( FdClient, IPPROTO_TCP, TCP_NODELAY, 
									(void *)&flags, sizeof(flags));
	// Receiver
	U64C	= FdClient;
	pConClient = FdEvCoCAlloc( FdClient, FD_TYPE_CLIENT, 
						FD_EVENT_READ, pCnvCtrl, RecvHandler );

	Ret = FdEventAdd( &pCnvCtrl->c_FdEventCtrl, U64C, &pConClient->c_FdEvent );
	if( Ret < 0 )	goto err_del;

	/* toServer */
	Ret = (*pIF->IF_fSessionOpen)( pConClient, pCnvCtrl->c_CellName );
	if( Ret < 0 )	goto err_del;


	LOG( pLog, LogINF, 
		"BUILD CONNECTION:Client(%p:Fd=%d)", pConClient, FdClient ); 

	DBG_EXT( 0 );
	return( 0 );
err_del:
LOG(pLog, LogDBG, "FdEvCoCFree(%p)", pConClient );
	FdEvCoCFree( pConClient );
	close( FdClient );
err:
	DBG_EXT( -1 );
	return( -1 );
}


void
usage()
{
	printf("CoC [host:]port cell_name {CIFS|HTTP}\n");
	printf("\t-W n			--- Workers\n");
	printf("\t-S {0|1|2}	--- Snoop{OFF|META_ON|META_OFF}\n");
}

int
main( int ac, char* av[] )
{
	char	host_name[128];

    struct addrinfo hints;
    struct addrinfo *result;
    int 	s;
	struct in_addr	pin_addr;
	struct sockaddr_in	Port;
	int	ListenFd;
	int		LogLevel	= DEFAULT_LOG_LEVEL;
	char	*pHost, *pPort;
	FdEvCoC_t	*pCon;
	uint64_t	U64;
	int		Ret;
	int	Workers = DEFAULT_WORKERS;
	int	Snoop = SNOOP_META_ON;
	int	i, j;

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
		case 'W':	Workers = atoi(av[j++]);	break;
		case 'S':	Snoop = atoi(av[j++]);		break;
		}
	}

	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}

	iConv	= iconv_open( "WCHAR_T", "UNICODE" );
	setlocale( LC_ALL, "" );

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	SET_IF( av[3] );
	if( !pIF ) {
		printf("[%s] is not supported.\n", av[3] );
		exit( -1 );
	}
/*
 *	Logger
 */
	if( getenv("LOG_LEVEL") ) {
		LogLevel = atoi( getenv("LOG_LEVEL") );
	}
	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		pLog = LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
						stderr, LogLevel );
		LOG( pLog, LogINF, "### LOG_SIZE=%d ###", log_size );
	}
/*
 *	Init
 */
	pCNTRL = (*pIF->IF_fInitCoC)( av[2] );

	RwLockInit( &pCNTRL->c_RwLock );
	FdEventCtrlCreate( &pCNTRL->c_FdEventCtrl );
	strncpy( pCNTRL->c_CellName, av[2], sizeof(pCNTRL->c_CellName) );
	QueueInit( &pCNTRL->c_Worker );

	pCNTRL->c_Snoop	= Snoop;

	/* 	Worker thread */
	for( i = 0; i < Workers; i++ ) {
		pthread_create( &pCNTRL->c_WorkerThread[i], NULL, 
					ThreadWorker, pCNTRL );
	}

/*
 *	Client Accept Address
 */
	pHost	= pPort = av[1];	
	while( pHost ) {
		if( *pHost == ':' )  {
			*pHost = '\0';
			strcpy( host_name, av[1] );
			pPort = pHost+1;
			break;
		}	
		pHost++;
	}
	if( pPort == av[1] ) {
		gethostname( host_name, sizeof(host_name) );
	}

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_INET;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = 0;
    hints.ai_protocol   = 0;
    hints.ai_canonname  = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

   	s = getaddrinfo( host_name, NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	memset( &Port, 0, sizeof(Port) );
	Port.sin_family	= AF_INET;
	Port.sin_addr	= pin_addr;
	Port.sin_port	= htons( atoi(pPort) );

	if( (ListenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror("socket");
		exit( -1 );
	}
	if( bind( ListenFd, (struct sockaddr*)&Port, sizeof(Port) ) < 0 ) {
		perror("bind");
		exit( -1 );
	}
	listen( ListenFd, 5 );

	U64	= ListenFd;
	pCon = (FdEvCoC_t*)Malloc( sizeof(*pCon) );
	FdEventInit( &pCon->c_FdEvent, FD_TYPE_ACCEPT, 
						ListenFd, FD_EVENT_READ, pCNTRL, AcceptHandler );

	Ret = FdEventAdd( &pCNTRL->c_FdEventCtrl, U64, &pCon->c_FdEvent );

	/* FdLoop */
	Ret = FdEventLoop( &pCNTRL->c_FdEventCtrl, -1 );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

