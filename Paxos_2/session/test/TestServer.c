/******************************************************************************
*
*  TestServer.c 	--- server test program of Paxos-Session Library
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

//#define	_DEBUG_

//#define	_LOCK_

#define	_PAXOS_SESSION_SERVER_
#include	"PaxosSession.h"

PaxosCell_t			*pPaxosCell;
PaxosSessionCell_t	*pSessionCell;

char	PERM[256],	PERM_SNAP[256], PERM_SNAP_PAXOS[256];


#define	OUTOFBAND_PATH( FilePath, pId ) \
	({char * p; snprintf( FilePath, sizeof(FilePath), "%s/%s-%d-%d-%d", \
		PAXOS_SESSION_OB_PATH, \
		inet_ntoa((pId)->i_Addr.sin_addr), \
		(pId)->i_Pid, (pId)->i_No, (pId)->i_Seq ); \
		p = FilePath; while( *p ){if( *p == '.' ) *p = '_';p++;}})

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
	if( !(pH = (pAcceptor->c_IF.if_fMalloc)( H.h_Len )) )	goto err1;

	*pH	= H;
	if( read( fd, (char*)(pH+1), H.h_Len - sizeof(H) )
								!= H.h_Len - sizeof(H) )	goto err2; 

	close( fd );

	return( pH );
err2:
	(pAcceptor->c_IF.if_fFree)( pH );
err1:
DBG_PRT("fd=%d\n", fd );
	close( fd );
err:
	return( NULL );
}

int
Load( void *pTag )
{
	return( PaxosAcceptorGetLoad( (struct PaxosAcceptor*)pTag ) );
}

int
SessionOpenS( struct PaxosAccept* pAccept, PaxosSessionHead_t* pPSH )
{
	Printf("SessionOpenS:Pid=%d-%d Seq=%d\n", 
		pPSH->h_Id.i_Pid, pPSH->h_Id.i_No, pPSH->h_Id.i_Seq );
	Printf("SessionOpenS:SLEEP 60\n"); sleep( 60 );
	return( 0 );
}

int
SessionCloseS( struct PaxosAccept* pAccept, PaxosSessionHead_t* pPSH )
{
	Printf("SessionCloseS:Pid=%d-%d Seq=%d\n", 
		pPSH->h_Id.i_Pid, pPSH->h_Id.i_No, pPSH->h_Id.i_Seq );
	return( 0 );
}

int
SessionLock(void)
{
	Printf("SessionLock\n");
	return( 0 );
}

int
SessionUnlock(void)
{
	Printf("SessionUnlock\n");
	return( 0 );
}

IOReadWrite_t*
SnapCreate( struct PaxosAcceptor* pAcceptor )
{
	IOFile_t	*pFile;

	pFile = IOFileCreate( PERM_SNAP, O_RDWR|O_CREAT, 0666,
				PaxosAcceptorGetfMalloc(pAcceptor), 
				PaxosAcceptorGetfFree(pAcceptor) );

	Printf("SnapCreate pFile=%p\n", pFile );
	return( (IOReadWrite_t*)pFile );
}

IOReadWrite_t*
SnapOpen( struct PaxosAcceptor* pAcceptor )
{
	IOFile_t	*pFile;

	pFile = IOFileCreate( PERM_SNAP, O_RDWR|O_CREAT, 0666,
				PaxosAcceptorGetfMalloc(pAcceptor), 
				PaxosAcceptorGetfFree(pAcceptor) );

	Printf("SnapCreate pFile=%p\n", pFile );
	return( (IOReadWrite_t*)pFile );
}

int
SnapClose( IOReadWrite_t *pIO )
{
	Printf("SnapClose pIO=%p\n", pIO );
	IOFileDestroy( (IOFile_t*)pIO, TRUE );
	return( 0 );
}

int
SnapWrite( IOReadWrite_t* pW )
{
	Printf("SnapWrite pW=%p\n", pW );
	return( 0 );
}

int
SnapRead( IOReadWrite_t *pR/*, struct Session *pSession, void *pH*/ )
{
	Printf("SnapRead pR=%p\n", pR );
	return( 0 );
}

int
SnapAdaptorWrite( IOReadWrite_t *pW, struct PaxosAccept* pAccept )
{
	Printf("SnapAdaptorWrite pW=%p\n", pW );
	return( 0 );
}

int
SnapAdaptorRead( IOReadWrite_t *pR, struct PaxosAccept* pAccept )
{
	Printf("SnapAdaptorRead pR=%p\n", pR );
	return( 0 );
}

int
ServerShutdown( struct PaxosAcceptor* pAcceptor,
			PaxosSessionShutdownReq_t *pReq, uint32_t nextDecide )
{
	printf("nextDecide=%u [%s]\n", nextDecide , pReq->s_Comment );
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	id;
//	int	Ret;
	int	Core, Extension;
	int64_t	MemLimit;

	if( ac < 7 ) {
//	printf("server cell core extension mem id log_size [[autonomic] id]\n");
	printf("server cell core extension mem id log_size {FALSE|TRUE [seq]}\n");
		exit( -1 );
	}
	Core		= atoi(av[2]);
	Extension	= atoi(av[3]);
	MemLimit	= atoi(av[4]);
	id			= atoi(av[5]);

	sprintf( PERM, "PERM-%d", id );
	sprintf( PERM_SNAP, "PERM-%d_SNAP", id );
	sprintf( PERM_SNAP_PAXOS, "PERM-%d_SNAP_PAXOS", id );

/*
 *	Address
 */
	pPaxosCell		= PaxosCellGetAddr( av[1], Malloc );
	pSessionCell	= PaxosSessionCellGetAddr( av[1], Malloc );
	if( !pPaxosCell || !pSessionCell ) {
		printf("Can't get cell address.\n");
		exit( -1 );
	}

/*
 *	Initialization
 */
	struct PaxosAcceptor* pAcceptor;
	PaxosAcceptorIF_t	IF;

	memset( &IF, 0, sizeof(IF) );

	IF.if_fMalloc	= Malloc;
	IF.if_fFree		= Free;

	IF.if_fSessionOpen	= SessionOpenS;
	IF.if_fSessionClose	= SessionCloseS;

	IF.if_fGlobalMarshal	= SnapWrite;
	IF.if_fGlobalUnmarshal	= SnapRead;
	IF.if_fAdaptorMarshal	= SnapAdaptorWrite;
	IF.if_fAdaptorUnmarshal	= SnapAdaptorRead;

	IF.if_fLock		= SessionLock;
	IF.if_fUnlock	= SessionUnlock;
//	IF.if_fAny		= SessionAny;

	IF.if_fOutOfBandSave	= OutOfBandSave;
	IF.if_fOutOfBandLoad	= OutOfBandLoad;
	IF.if_fOutOfBandRemove	= OutOfBandRemove;

	IF.if_fLoad	= Load;

	IF.if_fShutdown	= ServerShutdown;

	pAcceptor = (struct PaxosAcceptor*)Malloc( PaxosAcceptorGetSize() );

#define	UNIT_USEC	100LL*1000LL	// 100ms

	DBG_PRT("UNIT: %lld usec\n", UNIT_USEC );

	PaxosAcceptorInitUsec( pAcceptor, id, Core, Extension, MemLimit, TRUE,
				pSessionCell, pPaxosCell, UNIT_USEC, DEFAULT_WORKERS, &IF );
	PaxosAcceptorLogCreate( pAcceptor, atoi(av[6]), 0, stderr, LogDBG );
	PaxosSetLog( PaxosAcceptorGetPaxos(pAcceptor ),
					PaxosAcceptorGetLog( pAcceptor ) );

	PaxosStartMode_t	StartMode;
	uint32_t			seq;

	if( !strcmp( av[7], "TRUE" ) ) {
		if( ac >= 9 ) {
			StartMode 	= PAXOS_SEQ;
			seq	= atoi( av[8] );
		} else {
			StartMode	= PAXOS_NO_SEQ;
		}
	} else {
		StartMode = PAXOS_WAIT;
	}
/*
 * 	Start
 */
#ifdef	ZZZ
	if( PaxosAcceptorStart( pAcceptor, 5/*1*/  ) < 0 ) {
#else
	if( PaxosAcceptorStartSync( pAcceptor, StartMode, seq, NULL  ) < 0 ) {
#endif
		printf("START ERROR\n");
		exit( -1 );
	}

	return( 0 );
}

