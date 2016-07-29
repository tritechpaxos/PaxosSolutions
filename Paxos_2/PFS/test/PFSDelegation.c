/******************************************************************************
*
*  PFSDelegation.c 	--- client test program of PFS Library
*
*  Copyright (C) 2013-2016 triTech Inc. All Rights Reserved.
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
 *	çÏê¨			ìnï”ìTçF
 *	ì¡ãñÅAíòçÏå†	ÉgÉâÉCÉeÉbÉN
 */

#include	"PFS.h"
#include	<stdio.h>
#include	<netinet/tcp.h>


static int	_EventDirGetCount_ = 0;
static int	_EventDirCreDelCount_ = 0;
static int	_EventDirUpdtCount_ = 0;
static int	_EventLockGetCount_ = 0;
static int	_EventQueueGetCount_ = 0;
static int	_EventQueuePostCount_ = 0;
static int	_EventQueueWaitCount_ = 0;

PFSDlg_t	Dlg;

Hash_t	DlgHash;

int
DlgPut( PFSDlg_t * pD )
{
	HashPut( &DlgHash, pD->d_Name, pD );
	return( 0 );
}

PFSDlg_t*
DlgGet( char *pName )
{
	PFSDlg_t	*pDlg;

	pDlg	= HashGet( &DlgHash, pName );
	return( pDlg );
}

int
PFSEventDir( struct Session* pSession, PFSEventDir_t* pEvent )
{
	_EventDirGetCount_ ++;
    if( pEvent->ed_Mode != 2 ) _EventDirCreDelCount_ ++;
    else  _EventDirUpdtCount_ ++;

	Printf("PFSEventDir: Mode=%d [%s] [%s]\n", 
		pEvent->ed_Mode, pEvent->ed_Entry, pEvent->ed_Name );

	return( 0 );
}

int
PFSEventLock( struct Session* pSession, PFSEventLock_t* pEvent )
{
	PFSDlg_t	*pD;

	_EventLockGetCount_ ++;

	// find Dlg.
	pD	= DlgGet( pEvent->el_Name );

	Printf(
	"[%u:%u]PFSEventLock->Recall:LockName[%s] el_Ver=%d d_Own=%d d_Ver=%d\n", 
		(uint32_t)pthread_self(), (uint32_t)time(0), 
		pEvent->el_Name, pEvent->el_Ver, pD->d_Own, pD->d_Ver );

	PFSDlgRecall( pEvent, pD, NULL );

	Printf("[%u:%u]<-Recalled:LockName[%s]\n", 
		(uint32_t)pthread_self(), (uint32_t)time(0), pEvent->el_Name );

	return( 0 );
}

int
PFSEventQueue( struct Session* pSession, PFSEventQueue_t* pEvent )
{
	PaxosClientId_t	*pId;

	_EventQueueGetCount_ ++;
	Printf("PFSEventQueue:QueueName[%s] %s\n", 
		pEvent->eq_Name, (pEvent->eq_PostWait == 0 ? "POST" :"WAIT") );
	if( pEvent->eq_PostWait == 0 )	_EventQueuePostCount_++;
	else							_EventQueueWaitCount_++;

	pId	= &pEvent->eq_Id;
	Printf("ClientId [%s-%d] Pid=%u-%d Seq=%d\n", 
		inet_ntoa( pId->i_Addr.sin_addr ), ntohs( pId->i_Addr.sin_port ),
		pId->i_Pid, pId->i_No, pId->i_Seq );
	return( 0 );
}

int
EventCallback( struct Session *pSession, PFSHead_t *pF )
{
	switch( (int)pF->h_Head.h_Cmd ) {
		case PFS_EVENT_DIR:
			PFSEventDir(pSession,(PFSEventDir_t*)pF);
			break;

		case PFS_EVENT_LOCK:
			PFSEventLock(pSession,(PFSEventLock_t*)pF );
			break;

		case PFS_EVENT_QUEUE:
			PFSEventQueue(pSession,(PFSEventQueue_t*)pF );
			break;

		default: Printf("Cmd=%d\n", pF->h_Head.h_Cmd ); break;
	}
	return( 0 );
}

void*
ThreadEvent( void *pArg )
{
	struct Session *pE = (struct Session*)pArg;
	int32_t	count, omitted;
	int	Ret;

	while( 1 ) {
		Ret = PFSEventGetByCallback( pE, &count, &omitted,
										EventCallback );
		if( Ret )	{
			Printf("EventGet:Ret=%d->EXIT\n", Ret );
			break;
		} else {
			Printf("EventGet:Total Count[%d] Ret[%d]  Master is %d.\n", 
			_EventLockGetCount_, Ret, PaxosSessionGetMaster( pE ) );
		}
	}
	pthread_exit( 0 );
	return( NULL );
}

int
DownLoad( PFSDlg_t *pD, void *pArg )
{
	printf("[%u:%u]<== INIT(Own:%d Ver:%d) Guarded by d_Mtx\n", 
		(uint32_t)pthread_self(), (uint32_t)time(0),pD->d_Own, pD->d_Ver);
	printf("\tOn R/W download and init\n");

	return( 0 );
}

int
UpLoad( PFSDlg_t *pD, void *pArg )
{
	printf("[%u:%u]==>TERM(Own:%d Ver:%d) Gurded by d_Mtx\n", 
		(uint32_t)pthread_self(), (uint32_t)time(0), pD->d_Own, pD->d_Ver);
	printf("\tOn R terminate\n");
	printf("\tOn W upload and terminate\n");

	return( 0 );
}

void*
ThreadAbort( void *pArg )
{
	struct Session *pSession = (struct Session*)pArg;

	sleep( 30 );
	Printf("Abort pSession\n");
	PaxosSessionAbort( pSession );

	pthread_exit( 0 );
	return( NULL );
}

void
usage()
{
	printf("cmd cell delegR name [count]\n");
	printf("	HoldR/Release\n");
	printf("cmd cell delegW name [count]\n");
	printf("	HoldW/Release\n");
	printf("cmd cell delegRW name [count]\n");
	printf("	HoldR/HoldW/Release/Release\n");
	printf("cmd cell delegWR name [count]\n");
	printf("	HoldW/HoldR/Release/Release\n");
	printf("cmd cell delegRRet name [count]\n");
	printf("	HoldR/Release/Return,HoldR/Return/Release\n");
	printf("cmd cell delegWRet name [count]\n");
	printf("	HoldW/Release/Return,HoldW/Return/Release\n");
	printf("cmd cell delegR_W name [count]\n");
	printf("	HoldR/Release,HoldW/Release\n");
	printf("cmd cell delegW_R name [count]\n");
	printf("	HoldW/Release,HoldR/Release\n");
	printf("cmd cell recuW name [count]\n");
	printf("cmd cell recuR name [count]\n");
	printf("cmd cell nest nameA nameB\n");
}

int
main( int ac, char* av[] )
{
	int	Ret;
	pthread_t	th;
	int	count = 1, Cnt;
	int	NO=0;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	if( ac >= 5 && strcmp(av[2],"nest") ) {
		count = atoi(av[4]);
	}
	struct Session	*pSession = NULL, *pSessionEvent = NULL;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	pSessionEvent = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession || !pSessionEvent ) {
		printf("Can't get address [%s].\n", av[1] );
		exit( -1 );
	}
	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, 
								stderr, LogDBG );
	}

	PaxosSessionSetLog( pSessionEvent, PaxosSessionGetLog( pSession ) );

	Ret = PaxosSessionOpen( pSessionEvent, NO++, FALSE );
	if( Ret != 0 ) {
		Printf("EventSessionOpen:Ret[%d]\n", Ret );
		goto end;
	}
	Ret = PaxosSessionOpen( pSession, NO++, FALSE );
	if( Ret != 0 ) {
		Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret,errno );
		PaxosSessionClose( pSessionEvent );
		goto end;
	}
	pthread_create( &th, NULL, ThreadEvent, (void*)pSessionEvent );

	HashInit( &DlgHash, PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR, NULL,
			Malloc, Free, NULL );

	Ret = PFSDlgInit( pSession, pSessionEvent, &Dlg, av[3], 
						DownLoad, UpLoad, NULL );

	Ret = PFSDlgEventSet( &Dlg );

	DlgPut( &Dlg );

	if ( !strcmp( av[2], "delegR" ) ) {

		while( count ) {
			printf("\n### %d:DlgHoldR ###\n", count-- );

delegRagain:
			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("%d. Hold R:Ret=%d\n", count, Ret);
			if( Ret ) {
				printf("sleep(30) and reOpen\n"); sleep(30);

				PaxosSessionClose( pSession );
				PaxosSessionClose( pSessionEvent );

				Ret = PaxosSessionOpen( pSessionEvent, NO++, FALSE );
				if( Ret != 0 ) {
					Printf("EventSessionOpen:Ret[%d]\n", Ret );
					goto end;
				}
				Ret = PaxosSessionOpen( pSession, NO++, FALSE );
				if( Ret != 0 ) {
					Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret,errno );
					PaxosSessionClose( pSessionEvent );
					goto end;
				}
				goto delegRagain;
			}

			printf("Before sleep 20\n");
			sleep( 20 );
			printf("After sleep\n");
			Ret = PFSDlgRelease( &Dlg );
			sleep( 1 );
		}

	} else if ( !strcmp( av[2], "delegW" ) ) {

		while( count ) {
			printf("\n### %d:DlgLock ###\n", count-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);

			printf("Before SLEEP 35\n");
			sleep( 35 );
			printf("After SLEEP\n");

			Ret = PFSDlgRelease( &Dlg );
			sleep( 1 );
		}

	} else if ( !strcmp( av[2], "delegRW" ) ) {

		while( count ) {
			printf("\n### %d:DlgLock ###\n", count-- );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep( 35 );
		}
	} else if ( !strcmp( av[2], "delegWR" ) ) {

		while( count ) {
			printf("\n### %d:DlgLock ###\n", count-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);

			printf("Before SLEEP 35\n");
			sleep( 35 );
			printf("After SLEEP\n");

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep( 1 );
			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep( 1 );
		}
	} else if ( !strcmp( av[2], "delegRRet" ) ) {

		while( count ) {
			printf("\n### %d:DlgLock ###\n", count-- );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);

			printf("SLEEP 10\n");
			sleep( 10 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgReturn( &Dlg, NULL );
			printf("Return:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);

			printf("SLEEP 10\n");
			sleep( 10 );

			Ret = PFSDlgReturn( &Dlg, NULL );
			printf("Return:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep(1);
		}
	} else if ( !strcmp( av[2], "delegWRet" ) ) {

		while( count ) {
			printf("\n### %d:DlgLock ###\n", count-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);

			printf("SLEEP 10\n");
			sleep( 10 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgReturn( &Dlg, NULL );
			printf("Return:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);

			printf("SLEEP 10\n");
			sleep( 10 );

			Ret = PFSDlgReturn( &Dlg, NULL );
			printf("Return:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep(1);
		}
	} else if ( !strcmp( av[2], "delegR_W" ) ) {
		while( count ) {
			printf("\n### %d:HoldR/Release,HoldW/Release ###\n", count-- );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep( 35 );
		}
	} else if ( !strcmp( av[2], "delegW_R" ) ) {
		while( count ) {
			printf("\n### %d:HoldW/Releae,HoldR/Release ###\n", count-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("Hold W:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("Hold R:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );

			printf("SLEEP 35\n");
			sleep( 35 );

			Ret = PFSDlgRelease( &Dlg );
			printf("Release:Ret=%d\n", Ret);
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
			sleep( 35 );
		}
	} else if ( !strcmp( av[2], "recuW" ) ) {

		Cnt = count;
		while( Cnt ) {
			printf("\n### %d:HoldW ===\n", Cnt-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
		}
		Cnt = count;
		while( Cnt ) {
			printf("\n=== %d:Release ###\n", Cnt-- );

			Ret = PFSDlgRelease( &Dlg );
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
		}
	} else if ( !strcmp( av[2], "recuR" ) ) {

		Cnt = count;
		while( Cnt ) {
			printf("\n### %d:HoldR ===\n", Cnt-- );

			Ret = PFSDlgHoldR( &Dlg, NULL );
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
		}
		Cnt = count;
		while( Cnt ) {
			printf("\n=== %d:Release ###\n", Cnt-- );

			Ret = PFSDlgRelease( &Dlg );
			printf("HashCount=%d\n", HashCount(&Dlg.d_Hold) );
		}
	} else if ( !strcmp( av[2], "nest" ) ) {

		PFSDlg_t	DlgB;
		if( ac < 0 )	goto end;

		Ret = PFSDlgInit( pSession, pSessionEvent, &DlgB, av[4], 
						DownLoad, UpLoad, NULL );
		Ret = PFSDlgEventSet( &DlgB );

		DlgPut( &DlgB );

		Cnt = count + 1;
		while( Cnt ) {
			printf("\n### %d:nest ===\n", Cnt-- );

			Ret = PFSDlgHoldW( &Dlg, NULL );
			printf("SLEEP 10 After HoldW[%s]\n", av[3]);
			sleep( 10 );
			Ret = PFSDlgHoldW( &DlgB, NULL );
			printf("SLEEP 10 After HoldW[%s]\n", av[4]);
			sleep( 10 );
			Ret = PFSDlgRelease( &DlgB );
			printf("SLEEP 10 After Release[%s]\n", av[4]);
			sleep( 10 );
			Ret = PFSDlgRelease( &Dlg );
			printf("SLEEP 10 After Release[%s]\n", av[3]);
			sleep( 10 );
		}
	}
	Ret = PaxosSessionClose( pSession );
	if( Ret != 0 ) {
		Printf("SessionClose:Ret[%d]\n", Ret );
		goto end;
	}
	sleep( 1 );
	PFSDlgDestroy( &Dlg );

	Ret = PaxosSessionAbort( pSessionEvent );
	if( Ret != 0 ) {
		Printf("EventSessionAbort:Ret[%d]\n", Ret );
		goto end;
	}
	pthread_join( th, NULL );

	return( 0 );
end:
	return( -1 );
}

