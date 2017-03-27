/******************************************************************************
*
*  CoS.c 	--- converter program of server side
*
*  Copyright (C) 2011-2012,2016 triTech Inc. All Rights Reserved.
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

//#define	_LOCK_

#define	_PAXOS_SESSION_SERVER_
#include	"CNV.h"
#include	<netinet/tcp.h>
#include	<iconv.h>

#include	"../src/CIFS.h"

CnvCtrlCoS_t	*pCntrlCoS;

int	MyId;

PaxosCell_t			*pPaxosCell;
PaxosSessionCell_t	*pSessionCell;

int			log_size = -1;
struct Log*	pLog;
iconv_t	iConv;

SERVICE

struct	sockaddr_in	Server;


#define	CNVMalloc( s )	Malloc(s)
#define	CNVFree( p )	Free(p)

int
HashEventCode( void* pArg )
{
	int	index;

	PaxosClientId_t*	pId = (PaxosClientId_t*)pArg;

	index	= (int)( pId->i_Addr.sin_addr.s_addr + pId->i_Pid + pId->i_No );

	return( index );
}

int
HashEventCmp( void* pA, void* pB )
{
	PaxosClientId_t	*pAId = (PaxosClientId_t*)pA;
	PaxosClientId_t	*pBId = (PaxosClientId_t*)pB;

	return( !(pAId->i_Addr.sin_addr.s_addr == pBId->i_Addr.sin_addr.s_addr
			&& pAId->i_Pid == pBId->i_Pid && pAId->i_No == pBId->i_No ) ); 
}

int
CNVRequest( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM, int fd )
{
	bool_t	Validate = FALSE;
	struct PaxosAccept	*pAccept;
	PaxosSessionHead_t	Rpl, *pRpl;
	PaxosSessionHead_t	*pPush, *pReq;
	int	Ret;
	struct Paxos	*pPaxos;
	Msg_t		*pRes;
	MsgVec_t	Vec;

LOG( pLog, LogDBG, "Pid[%d-%d] h_Cmd[0x%x]", 
pM->h_Id.i_Pid, pM->h_Id.i_No, pM->h_Cmd );

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept ) {
//		LOG( pALOG, LogDBG, "### ALREADY CLOSED Pid[%d-%d]",
//							pM->h_Id.i_Pid, pM->h_Id.i_No );
		return( 0 );
	}

	switch( (int)pM->h_Cmd ) {

		case CNV_OUTOFBAND:
			PaxosAcceptOutOfBandLock( pAccept );

			pPush	= _PaxosAcceptOutOfBandGet( pAccept, &pM->h_Id );

			/* Pushの分をTrim */
			pReq	= pPush + 1;
			if( pReq->h_Cmd == (int)CNV_UPDATE ) {
				Validate = TRUE;
			}

			PaxosAcceptOutOfBandUnlock( pAccept );

			if( Validate )	goto update;
			pM	= pReq;

		case CNV_REFER:
//LOG( pLog, LogDBG, "CNV_REFER" );
			if( PaxosAcceptedStart( pAccept, pM ) ) {
				goto skip;
			}
			RwLockR( &pCntrlCoS->s_RwLock );

			pRes	= (pIF->IF_fReferToServer)( pAccept, pM );

			if( pRes->m_Err == 0 ) {
				pRpl	= MsgMalloc( pRes, sizeof(*pRpl) );
				*pRpl	= *pM;
				MsgVecSet( &Vec, VEC_MINE, pRpl, sizeof(*pRpl), sizeof(*pRpl) );
				MsgInsert( pRes, &Vec );	
				pRpl->h_Len	= MsgTotalLen( pRes );

				Ret = PaxosAcceptReplyReadByMsg( pAccept, pRes );

			} else {
LOG( pLog, LogERR, "CAN'T REPLY\n");
ABORT();
			}

			RwUnlock( &pCntrlCoS->s_RwLock );
			break;

		case CNV_UPDATE:
update:
LOG( pLog, LogDBG, "CNV_UPDATE:Validate=%d", Validate );
			pPaxos = PaxosAcceptorGetPaxos(pAcceptor);
			Ret	= PaxosRequest( pPaxos, pM->h_Len, pM, Validate );
			if( Ret ) {
				SESSION_MSGINIT( &Rpl, pM->h_Cmd, Ret, 0, sizeof(Rpl) );
				Rpl.h_Master	= PaxosGetMaster( pPaxos );
				Rpl.h_Epoch		= PaxosEpoch( pPaxos );
				Rpl.h_TimeoutUsec	= PaxosMasterElectionTimeout( pPaxos );

				Ret	= PaxosAcceptSend( pAccept, &Rpl );
			}
			break;
		case CNV_EVENT_WAIT:
LOG( pLog, LogDBG, "CNV_EVENT_WAIT:NO RETURN" );
			break;
		case CNV_EVENT_BIND:
LOG( pLog, LogDBG, "CNV_EVENT_BIND" );
			pPaxos = PaxosAcceptorGetPaxos(pAcceptor);
			Ret	= PaxosRequest( pPaxos, pM->h_Len, pM, Validate );
			break;
		default:
			ABORT();
	}
skip:
	PaxosAcceptPut( pAccept );
	return( 0 );
}

int
CNVValidate(struct PaxosAcceptor *pAcceptor, uint32_t seq,
						PaxosSessionHead_t *pM, int From)
{
	int	Ret;

	RwLockW( &pCntrlCoS->s_RwLock );

	Ret	= PaxosAcceptorOutOfBandValidate( pAcceptor, seq, pM, From );

	RwUnlock( &pCntrlCoS->s_RwLock );

	return( Ret );
}

Msg_t*
_CNVInsertCnvCmdMsg( Msg_t *pRes, int CnvCmd, int Error )
{
	PaxosSessionHead_t	*pH;
	MsgVec_t	Vec;
	int	Len = 0;

	if( !pRes )	pRes	= MsgCreate( 1, Malloc, Free );
	else		Len	= MsgTotalLen( pRes );

	pH	= (PaxosSessionHead_t*)Malloc( sizeof(*pH) );
LOG(pLog, LogDBG, "pH=%p", pH );

	SESSION_MSGINIT( pH, CnvCmd, 0, Error, sizeof(*pH) + Len );

	MsgVecSet(&Vec, VEC_MINE, pH, sizeof(*pH), sizeof(*pH));

	MsgInsert( pRes, &Vec );	

	return( pRes );
}

int
CNVAcceptReplyByMsg( struct PaxosAccept *pAccept, 
						Msg_t *pRes, int CnvCmd, int Error )
{
	int	Ret;

LOG(pLog, LogDBG, "CnvCmd=0x%x Error=0x%x", CnvCmd, Error );
	pRes = _CNVInsertCnvCmdMsg( pRes, CnvCmd, Error );

	Ret = PaxosAcceptReplyByMsg( pAccept, pRes );

	return( Ret );
}

int
CNVAcceptReplyReadByMsg( struct PaxosAccept *pAccept, Msg_t *pRes, int CnvCmd )
{
	int	Ret;

	pRes = _CNVInsertCnvCmdMsg( pRes, CnvCmd, 0 );

	Ret = PaxosAcceptReplyReadByMsg( pAccept, pRes );

	return( Ret );
}

int
CNVAccepted( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	PaxosSessionHead_t	*pReq, *pPush;
#ifdef 	ZZZ
	MsgVec_t	Vec;
	Msg_t	*pRes;
#endif
	struct PaxosAcceptor *pAcceptor;

	pAcceptor = PaxosAcceptGetAcceptor( pAccept );

	RwLockW( &pCntrlCoS->s_RwLock );

	switch( (int)pM->h_Cmd ) {

		case CNV_OUTOFBAND:
			PaxosAcceptOutOfBandLock( pAccept );

			pPush	= _PaxosAcceptOutOfBandGet( pAccept, &pM->h_Id );

			/* Pushの分をTrim */
			pReq	= pPush + 1;
			(pIF->IF_fRequestToServer)( pAccept, pReq );
			PaxosAcceptOutOfBandUnlock( pAccept );
			break;

		case CNV_UPDATE:
LOG(pLog, LogDBG, "CNV_UPDATE" ); 
			(pIF->IF_fRequestToServer)( pAccept, pM );
			break;

		case CNV_EVENT_BIND:
LOG(pLog, LogDBG, "CNV_EVENT_BIND" ); 
		{ 	CnvEventSessionBindReq_t	*pReqBind = 
										(CnvEventSessionBindReq_t*)pM;
			CnvEventSessionBindRpl_t	*pRpl;
			struct PaxosAccept	*pEvent;	

			pEvent	= (struct PaxosAccept*)HashGet( &pCntrlCoS->s_HashEvent,
												&pReqBind->eb_EventId );
			ASSERT( pEvent );

			(*pIF->IF_fEventBind)( pAccept, pEvent );

			pRpl = (CnvEventSessionBindRpl_t*)SESSION_MSGALLOC( CNV_EVENT_BIND,
										0, 0, sizeof(*pRpl) );
			/*Ret	=*/ ACCEPT_REPLY_MSG( pAccept, TRUE, 
						pRpl, sizeof(*pRpl), sizeof(*pRpl) );
			break;
		}
		case CNV_EVENT_WAIT:
			PaxosAcceptEventGet( pAccept, NULL );
			break;
	}
	RwUnlock( &pCntrlCoS->s_RwLock );
	return( 0 );
/***
err:
	RwUnlock( &CNTRL.c_RwLock );
	return( -1 );
***/
}

int
CNVRejected( struct PaxosAcceptor *pAcceptor, PaxosSessionHead_t *pM )
{
	struct PaxosAccept	*pAccept;
	PaxosSessionHead_t	Rpl;
	int	Ret;

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept )	return( -1 );

	SESSION_MSGINIT( &Rpl, pM->h_Cmd, PAXOS_REJECTED, 0, sizeof(Rpl) );
	Rpl.h_Master	= PaxosGetMaster(PaxosAcceptorGetPaxos(pAcceptor));
	Ret = PaxosAcceptSend( pAccept, &Rpl );

	PaxosAcceptPut( pAccept );
	return( Ret );
}

int
CNVAbort( struct PaxosAcceptor *pAcceptor, uint32_t Seq )
{
	Printf("=== CNVAbort Seq=%u\n", Seq );
	return( 0 );
}

int
CNVGlobalMarshal( IOReadWrite_t *pW )
{
	if( pIF->IF_fGlobalMarshal ) return( (pIF->IF_fGlobalMarshal)( pW ) );
	else	return( 0 );
}

int
CNVGlobalUnmarshal( IOReadWrite_t *pR )
{
	if( pIF->IF_fGlobalUnmarshal ) return( (pIF->IF_fGlobalUnmarshal)( pR ) );
	else	return( 0 );
}

int
CNVSessionLock()
{
	int Ret;

	LOG( pLog, LogDBG, "" );
	RwLockW( &pCntrlCoS->s_RwLock );
	if( pIF->IF_fStop ) {
		Ret = (pIF->IF_fStop)();
		if( Ret ) {
			RwUnlock( &pCntrlCoS->s_RwLock );
		}
	LOG( pLog, LogDBG, "Ret=%d", Ret );
		return( Ret );
	} else {
	LOG( pLog, LogDBG, "OK" );
		return( 0 );
	}
}

int
CNVSessionUnlock()
{
	LOG( pLog, LogDBG, "" );
	if( pIF->IF_fRestart )	(pIF->IF_fRestart)();
	RwUnlock( &pCntrlCoS->s_RwLock );
	return( 0 );
}

int
CNVSessionAny( struct PaxosAcceptor *pAcceptor, PaxosSessionHead_t *pM, 
				int fdSend )
{
	PaxosSessionHead_t	*pH;
	int	Ret;

	pH	= (PaxosSessionHead_t*)(pM+1);

	LOG(pLog, LogDBG, "h_Cmd=0x%x", pH->h_Cmd );

	switch( (int)pH->h_Cmd ) {
		case CNV_READ: 
		{
			CnvReadReq_t	*pReq;
			CnvReadRpl_t	*pRpl, Rpl;
			int	fd;
			int	l;

			pReq	= (CnvReadReq_t*)pH;
			fd	= open( pReq->r_Name, O_RDONLY, 0666 );
			if( fd >= 0 ) {
				l	= sizeof( *pRpl ) + pReq->r_Len;
				pRpl = (CnvReadRpl_t*)CNVMalloc( l );

				lseek( fd, pReq->r_Off, 0 );
				Ret	= read( fd, pRpl->r_Data, pReq->r_Len );

				close( fd );

				SESSION_MSGINIT( &pRpl->r_Head, CNV_READ, 0, 0, l );
//				pRpl->r_Head.h_Magic	= PAXOS_MAGIC;
				pRpl->r_Len	= Ret;
				send( fdSend, pRpl, pRpl->r_Head.h_Len, 0 );

				CNVFree( pRpl );
			} else {
				SESSION_MSGINIT( &Rpl.r_Head, CNV_READ, 0, errno, sizeof(Rpl));
//				pRpl->r_Head.h_Magic	= PAXOS_MAGIC;
				send( fdSend, &Rpl, sizeof(Rpl), 0 );
			}
			break;
		}
		case CNV_MEM_ROOT:
		{
			CnvMemRootReq_t	*pReq;
			CnvMemRootRpl_t	Rpl;

			pReq	= (CnvMemRootReq_t*)pH;
			ASSERT( !strcmp(pReq->r_Name, "CIFS") );

			SESSION_MSGINIT( &Rpl.r_Head, CNV_MEM_ROOT, 0, 0, sizeof(Rpl) );
			Rpl.r_pRoot	= pCntrlCoS;

			send( fdSend, &Rpl, sizeof(Rpl), 0 );
			break;
		}
	}
	return( 0 );
}

#define	OUTOFBAND_PATH( FilePath, pId ) \
	({char * p; snprintf( FilePath, sizeof(FilePath), "%s/%s-%d-%d-%d", \
		PAXOS_SESSION_OB_PATH, \
		inet_ntoa((pId)->i_Addr.sin_addr), \
		(pId)->i_Pid, (pId)->i_No, (pId)->i_Seq ); \
		p = FilePath; while( *p ){if( *p == '.' ) *p = '_';p++;}})

int
OutOfBandGarbage()
{
	char	command[128];
	int		Ret;

	sprintf( command, "rm %s/*", PAXOS_SESSION_OB_PATH );
	Ret	= system( command );
	return( Ret );
}

int
OutOfBandSave( struct PaxosAcceptor* pAcceptor, struct PaxosSessionHead* pM )
{
	// Overwrite
	char	FileOut[PATH_NAME_MAX];
	int		fd;

	OUTOFBAND_PATH( FileOut, &pM->h_Id );

	fd = open( FileOut, O_RDWR|O_CREAT, 0666 );
	if( fd < 0 )	goto err1;

	write( fd, (char*)pM, pM->h_Len );
DBG_TRC("fd=%d\n", fd );
	close( fd );
	return( 0 );
err1:
	return( -1 );
}

int
OutOfBandRemove( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
{
	char	FileOut[PATH_NAME_MAX];
	int		Ret;

	OUTOFBAND_PATH( FileOut, pId );
	Ret = unlink( FileOut );

	return( Ret );
}

struct PaxosSessionHead*
OutOfBandLoad( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
{
	char	FileOut[PATH_NAME_MAX];
	int		fd;
	PaxosSessionHead_t	H, *pH;

	OUTOFBAND_PATH( FileOut, pId );

	fd = open( FileOut, O_RDWR, 0666 );
DBG_TRC("[%s] fd=%d\n", FileOut, fd );
	if( fd < 0 )											goto err;

	if( read( fd, (char*)&H, sizeof(H) ) != sizeof(H) )		goto err1;

	if( !(pH = (PaxosAcceptorGetfMalloc(pAcceptor))( H.h_Len )) )	goto err1;

	*pH	= H;
	if( read( fd, (char*)(pH+1), H.h_Len - sizeof(H) )
								!= H.h_Len - sizeof(H) )	goto err2; 

	close( fd );

	return( pH );
err2:
	(PaxosAcceptorGetfFree(pAcceptor))( pH );
err1:
	close( fd );
err:
	return( NULL );
}

int
Load( void *pTag )
{
#ifdef	ZZZ
	return( PaxosAcceptorGetThreadCount( (struct PaxosAcceptor*)pTag ) );
#else
	return( PaxosAcceptorGetLoad( (struct PaxosAcceptor*)pTag ) );
#endif
}

void
usage()
{
printf("CoS service server port cellname my_id root {TRUE [seq]|FALSE}\n");
printf("	-c	Core\n");
printf("	-e	Extension\n");
printf("	-b	Cksum64\n");
printf("	-u	usec\n");
printf("	-M	MemLimit\n");
printf("	-W	Workers\n");
printf("	-S {0|1|2}	Snoop{OFF|META_ON|META_OFF}\n");
}

void*
ThreadEventLoopServer( void *pArg )
{
	FdEventCtrl_t	*pFdEvCtrlServer = (FdEventCtrl_t*)pArg;;

	pthread_detach( pthread_self() );

LOG(pLog, LogDBG, "pFdEvCtrlServer=%p:LOOP)", pFdEvCtrlServer );
	FdEventCtrlCreate( pFdEvCtrlServer );

	FdEventLoop( pFdEvCtrlServer, -1 );
	
LOG(pLog, LogDBG, "pFdEvCtrlServer=%p:EXIT)", pFdEvCtrlServer );
	pthread_exit( (void*)-1 );
}

int
main( int ac, char* av[] )
{
	int	j;
	struct in_addr	pin_addr;
    struct addrinfo hints;
    struct addrinfo *result;
    int 	s;
	int		Workers = DEFAULT_WORKERS;
	int64_t	PaxosUsec	= DEFAULT_PAXOS_USEC;
	int64_t	MemLimit = DEFAULT_OUT_MEM_MAX;
	int		Core = DEFAULT_CORE, Ext = DEFAULT_EXTENSION/*, Maximum*/;
	bool_t	bCksum = DEFAULT_CKSUM_ON;
	PaxosStartMode_t sMode;	uint32_t seq = 0;
	int		LogLevel	= DEFAULT_LOG_LEVEL;
	int		Snoop = SNOOP_META_ON;	

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
//		case 'E':	bExport = TRUE;
//					target_id = atoi(av[j++]);
//					pFile = av[j++];
//					break;
//		case 'I':	bImport = TRUE; pFile = av[j++];	break;
		case 'c':	Core = atoi(av[j++]);	break;
		case 'e':	Ext = atoi(av[j++]);	break;
		case 's':	bCksum = atoi(av[j++]);	break;
//		case 'F':	FdMax = atoi(av[j++]);	break;
//		case 'B':	BlockMax = atoi(av[j++]);	break;
//		case 'S':	SegSize = atoi(av[j++]);	break;
//		case 'N':	SegNum = atoi(av[j++]);	break;
//		case 'U':	WBUsec = atol(av[j++]);	break;
		case 'u':	PaxosUsec = atol(av[j++]);	break;
		case 'M':	MemLimit = atol(av[j++]);	break;
		case 'W':	Workers = atoi(av[j++]);	break;
		case 'S':	Snoop = atoi(av[j++]);	break;
		}
	}

	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}

	if( ac < 8 ) {
		usage();
		exit( -1 );
	}

	if( !strcmp("TRUE", av[7] ) ) {
		if( ac >= 9 ) {
			seq	= atoll(av[8]);
			sMode	= PAXOS_SEQ;
		} else {
			sMode	= PAXOS_NO_SEQ;
		}
	} else {
		sMode	= PAXOS_WAIT;
	}
	if( chdir( av[6] ) ) {
		printf("Can't change directory.\n");
		exit( -1 );
	}
	MyId	= atoi(av[5]);

	SET_IF( av[1] );
	if( !pIF ) {
		printf("[%s] is not supported.\n", av[1] );
		exit( -1 );
	}
/*
 *	Clear out-of-band data
 */
	OutOfBandGarbage();
/*
 *	Address
 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family 	= AF_INET;
    hints.ai_socktype 	= SOCK_STREAM;
    hints.ai_flags 		= 0;
    hints.ai_protocol 	= 0;
    hints.ai_canonname 	= NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

/*
 *	Server address
 */
   	s = getaddrinfo( av[2], NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	memset( &Server, 0, sizeof(Server) );
	Server.sin_family	= AF_INET;
	Server.sin_addr		= pin_addr;
	Server.sin_port		= htons( atoi(av[3]) );

/*
 *	Paxos, Client
 */
	pPaxosCell		= PaxosCellGetAddr( av[4], Malloc );
	pSessionCell	= PaxosSessionCellGetAddr( av[4], Malloc );
	if( !pPaxosCell || !pSessionCell ) {
		printf("Can't get cell addr.\n");
		EXIT( -1 );
	}
/*
 *	Log
 */
	if( getenv("LOG_LEVEL") ) {
		LogLevel = atoi( getenv("LOG_LEVEL") );
	}
	if( getenv("LOG_SIZE") ) {
		log_size = atoi( getenv("LOG_SIZE") );
//		pLog	= LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
//								stderr, LogLevel );
//		LOG( pLog, LogINF, "###### LOG_SIZE[%d] ####", log_size );
	}
/*
 *	Initialization
 */
	struct PaxosAcceptor* pAcceptor;
	PaxosAcceptorIF_t	IF;

	memset( &IF, 0, sizeof(IF) );

	IF.if_fMalloc	= Malloc;
	IF.if_fFree		= Free;

	IF.if_fSessionOpen	= pIF->IF_fOpen;
	IF.if_fSessionClose	= pIF->IF_fClose;

	IF.if_fRequest	= CNVRequest;
	IF.if_fValidate	= CNVValidate;
	IF.if_fAccepted	= CNVAccepted;
	IF.if_fRejected	= CNVRejected;
	IF.if_fAbort	= CNVAbort;

	IF.if_fBackupPrepare	= pIF->IF_fBackupPrepare;
	IF.if_fBackup			= pIF->IF_fBackup;
	IF.if_fRestore			= pIF->IF_fRestore;
	IF.if_fTransferSend		= pIF->IF_fTransferSend;
	IF.if_fTransferRecv		= pIF->IF_fTransferRecv;

	IF.if_fGlobalMarshal	= pIF->IF_fGlobalMarshal;
	IF.if_fGlobalUnmarshal	= pIF->IF_fGlobalUnmarshal;

	IF.if_fAdaptorMarshal	= pIF->IF_fAdaptorMarshal;
	IF.if_fAdaptorUnmarshal	= pIF->IF_fAdaptorUnmarshal;

	IF.if_fLock		= CNVSessionLock;
	IF.if_fUnlock	= CNVSessionUnlock;
	IF.if_fAny		= CNVSessionAny;

	IF.if_fOutOfBandSave	= OutOfBandSave;
	IF.if_fOutOfBandLoad	= OutOfBandLoad;
	IF.if_fOutOfBandRemove	= OutOfBandRemove;

	IF.if_fLoad	= Load;

	pAcceptor = (struct PaxosAcceptor*)Malloc( PaxosAcceptorGetSize() );

	PaxosAcceptorInitUsec( pAcceptor, MyId, Core, Ext, MemLimit, bCksum,
				pSessionCell, pPaxosCell, PaxosUsec, Workers, &IF );
/*
 *	Logger
 */
	if( log_size >= 0 ) {

		if( pLog ) {
			PaxosAcceptorSetLog( pAcceptor, pLog );
		} else {
			PaxosAcceptorLogCreate( pAcceptor, log_size, LOG_FLAG_BINARY, 
								stderr, LogLevel );
			pLog = PaxosAcceptorGetLog( pAcceptor);

			LOG( pLog, LogINF, "###### LOG_SIZE[%d] ####", log_size );
		}
		PaxosSetLog( PaxosAcceptorGetPaxos( pAcceptor ), pLog );

//		LogAtExit( pLog );
	}

/*
 *	Init
 */
	//CNVInit( pAcceptor );

	pCntrlCoS = (*pIF->IF_fInitCoS)( NULL );

	pCntrlCoS->s_Snoop = Snoop;
	RwLockInit( &pCntrlCoS->s_RwLock );
#ifdef	ZZZ
	FdEventCtrlCreate( &pCntrlCoS->s_FdEventCtrl );
#else
	FdEventCtrlCreate( &pCntrlCoS->s_FdEventCtrl );
	pthread_create( &pCntrlCoS->s_FdEventCtrl.ct_Th, NULL, 
			ThreadEventLoopServer, &pCntrlCoS->s_FdEventCtrl );
#endif
	HashInit( &pCntrlCoS->s_HashEvent, PRIMARY_1013, 
					HashEventCode, HashEventCmp, NULL, Malloc, Free, NULL );

	pCntrlCoS->s_pAcceptor = pAcceptor;
/*
 * 	Start
 */
	if( PaxosAcceptorStartSync( pAcceptor, sMode, seq, NULL ) < 0 ) {
		printf("START ERROR\n");
		exit( -1 );
	}
	return( 0 );
}

