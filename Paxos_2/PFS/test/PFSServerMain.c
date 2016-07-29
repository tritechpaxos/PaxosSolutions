/******************************************************************************
*
*  PFSServerMain.c 	--- server test program of PFS Library
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
#include	"PFS.h"
#include	<netinet/tcp.h>

int	id;
int			log_size = -1;
struct Log*	pLog = NULL;

PaxosCell_t			*pPaxosCell;
PaxosSessionCell_t	*pSessionCell;


int	PFSEventDir(struct Session* pSession, PFSEventDir_t* pEvent){return(0);}
int	PFSEventLock(struct Session* pSession, PFSEventLock_t* pEvent){return(0);}
int	PFSEventQueue(struct Session* pSession, PFSEventQueue_t* pEvent){return(0);}

/*
 *	Communicaton
 */
int	RecvFd, SendFd;
int
RecvFrom( struct Paxos* pPaxos, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t *pAddrLen )
{
	int	Ret;
	int	i = 0;
	char	dummy[1024];
again:
	Ret = recvfrom( RecvFd, p, len, flag,(struct sockaddr*)pAddr,pAddrLen);
	if( Ret < 0 && errno == ECONNRESET ) {
		DBG_PRT("port=%d len=%d flag=0x%x *pAddrLen=%d Ret=%d\n", 
				ntohs(pAddr->sin_port), (int)len, flag, *pAddrLen, Ret );
		recvfrom( RecvFd, dummy, sizeof(dummy), 0,
						(struct sockaddr*)pAddr,pAddrLen);

		if( i++ > 5 )	goto next;
		goto again;
	}
next:
	return( Ret );
}

int
SendTo( struct Paxos* pPaxos, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t AddrLen )
{
	int	Ret;

	Ret = sendto( SendFd, p, len, flag, (struct sockaddr*)pAddr, AddrLen );
	if( Ret < 0 && errno == ECONNRESET ) {
		DBG_PRT("port=%d len=%d flag=0x%x AddrLen=%d Ret=%d\n", 
				ntohs(pAddr->sin_port),(int)len, flag, AddrLen, Ret );
	}
	return( Ret );
}

#define	OUTOFBAND_PATH( FilePath, pId ) \
	({char * p; snprintf( FilePath, sizeof(FilePath), "%s/%s-%d-%d-%d-%d-%ld", \
		PAXOS_SESSION_OB_PATH, \
		inet_ntoa((pId)->i_Addr.sin_addr), \
		(pId)->i_Pid, (pId)->i_No, (pId)->i_Seq, (pId)->i_Try, \
		(pId)->i_Time ); \
		p = FilePath; while( *p ){if( *p == '.' ) *p = '_';p++;}})

int
PFSOutOfBandSave( struct PaxosAcceptor* pAcceptor, struct PaxosSessionHead* pM )
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
PFSOutOfBandRemove( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
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
PFSOutOfBandLoad( struct PaxosAcceptor* pAcceptor, PaxosClientId_t* pId )
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
PFSLoad( void *pTag )
{
	return( PaxosAcceptorGetLoad( (struct PaxosAcceptor*)pTag ) );
}

int
InitDirectory( int Target )
{
	char	command[256];

	LOG(pLog,LogINF,"Target=%d", Target );

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
	if( !pSession ) {
		printf("Can't get cell[%s]\n", pSessionCell->c_Name );
		EXIT( -1 );
	}
	if( pLog ) {
		PaxosSessionSetLog( pSession, pLog );
	}

	pIOFile	= IOFileCreate( pFile, O_WRONLY|O_TRUNC, 0, Malloc, Free );
	pW	= (IOReadWrite_t*)pIOFile;

	Ret = PaxosSessionExport( pAcceptor,pSession, j, pW );

	IOFileDestroy( pIOFile, FALSE );
	Free( pSession );

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
		EXIT( -1 );
	}
	Ret = PaxosSessionImport( pAcceptor, (IOReadWrite_t*)pIOFile );

	IOFileDestroy( pIOFile, FALSE );
	return( Ret );
}

void
usage()
{
printf("server cellname my_id root_directory {TRUE [seq]|FALSE}\n");
printf("	TRUE [seq]			... synchronize and join in paxos\n");
printf("	FALSE				... synchronize and wait decision and join\n");
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
#ifdef	ZZZ
printf("	-L Level 			... log level\n");
#endif
}

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
#ifdef	ZZZ
		case 'L':	LogLevel = atoi(av[j++]);	break;
#endif
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
		printf("Can't change directory[%s].\n", av[3] );
	 	perror("");
		usage();
		EXIT( -1 );
	}
	id	= atoi(av[2]);

/*
 *	Address
 */
	pPaxosCell		= PaxosCellGetAddr( av[1], Malloc );
	pSessionCell	= PaxosSessionCellGetAddr( av[1], Malloc );
	if( !pPaxosCell || !pSessionCell ) {
		printf("Can't get cell addr.\n");
		EXIT( -1 );
	}
/*
 *	Log
 */

	if( getenv("LOG_SIZE") ) {
		log_size = atoi( getenv("LOG_SIZE") );
	}
#ifdef	ZZZ
#else
	if( getenv("LOG_LEVEL") ) {
		LogLevel = atoi( getenv("LOG_LEVEL") );
	}
#endif
/*
 *	Initialization
 */
	struct PaxosAcceptor* pAcceptor;
	PaxosAcceptorIF_t	IF;

	memset( &IF, 0, sizeof(IF) );

	IF.if_fMalloc	= Malloc;
	IF.if_fFree		= Free;

	IF.if_fSessionOpen	= PFSAdaptorOpen;
	IF.if_fSessionClose	= PFSAdaptorClose;
	IF.if_fRequest	= PFSRequest;
	IF.if_fValidate	= PFSValidate;
	IF.if_fAccepted	= PFSAccepted;
	IF.if_fRejected	= PFSRejected;
	IF.if_fAbort	= PFSAbort;
	IF.if_fShutdown	= PFSSessionShutdown;

	IF.if_fBackupPrepare	= PFSBackupPrepare;
	IF.if_fBackup			= PFSBackup;
	IF.if_fRestore			= PFSRestore;
	IF.if_fTransferSend		= PFSTransferSend;
	IF.if_fTransferRecv		= PFSTransferRecv;

	IF.if_fGlobalMarshal	= PFSGlobalMarshal;
	IF.if_fGlobalUnmarshal	= PFSGlobalUnmarshal;
	IF.if_fAdaptorMarshal	= PFSAdaptorMarshal;
	IF.if_fAdaptorUnmarshal	= PFSAdaptorUnmarshal;

	IF.if_fLock		= PFSSessionLock;
	IF.if_fUnlock	= PFSSessionUnlock;
	IF.if_fAny		= PFSSessionAny;

	IF.if_fOutOfBandSave	= PFSOutOfBandSave;
	IF.if_fOutOfBandLoad	= PFSOutOfBandLoad;
	IF.if_fOutOfBandRemove	= PFSOutOfBandRemove;

	IF.if_fLoad	= PFSLoad;

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
			PaxosAcceptorLogCreate( pAcceptor, log_size, LOG_FLAG_BINARY, 
								stderr, LogLevel );
			pLog = PaxosAcceptorGetLog( pAcceptor);

			LOG( pLog, LogINF, "###### LOG_SIZE[%d] ####", log_size );
		}
		PaxosSetLog( PaxosAcceptorGetPaxos( pAcceptor ), pLog );

		LogAtExit( pLog );
	}
/*
 *	PFSInit
 */

	PFSInit( Core, Ext, id, bCksum, FdMax, ".", BlockMax, SegSize, SegNum,
				WBUsec, pAcceptor );
/*
 *	autonomic
 */
	if( bExport  ) {
		ExportToFile( pAcceptor, target_id, pFile );
		EXIT( 0 );
	}
	if( bImport ) {
		ImportFromFile( pAcceptor, pFile );
	}
/*
 * 	Start
 */
	if( PaxosAcceptorStartSync( pAcceptor, sMode, seq, InitDirectory ) < 0 ) {
		printf("START ERROR:%m\n");
		exit( -1 );
	}
	return( 0 );
}

