/******************************************************************************
*
*  PFSClientMain.c 	--- client test program of PFS Library
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

#include	"PFS.h"
#include	<stdio.h>
#include	<netinet/tcp.h>
#include	<inttypes.h>


/*
 *	試験
 *
 *	 PAXOS_SERVER_MAX=5
 *	 PAXOS_MAX=5
 *	 admin
 *	 PFSProbe
 *
 *	1.接続不能
 *	 1)client　session
 *	[結果]
 *	 エラー終了
 *	2.永久接続
 *	 1)server 0 PFS-0
 *	 2)client　session
 *	[結果]
 *	 永久に接続を試みる
 *	3.接続不能となる
 *	 1)server 0 PFS-0
 *	 2)client session
 *	 3)暫くしてからserver 0をダウン
 *	[結果]
 *	 エラー終了
 *	4.接続
 *	 1)server 0 PFS-0
 *	 2)client session
 *	 3)暫くしてからserver 1 PFS-1
 *	 4)暫くしてからserver 2 PFS-2
 *	[結果]
 *	 接続が完了し終了する
 *	5.masterの切り替え
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client session
 *	 5)暫くしてからserver 0をダウンし再起動する
 *		server 0 PFS-0 autonomic 1
 *	 6)client session
 *	[結果]
 *	 masterが切り替わっている
 *	6.lockW/unlockのmaster切り替え
 *	 1)server 0	PFS-0
 *	 2)server 1	PFS-1
 *	 3)server 2	PFS-2
 *	 4)client lockW LOCK_A
 *	 5)暫くしてからserver 0をダウンし再起動する
 *		server 0 PFS-0 autonomic 1
 *	[結果]
 *	 0でロックが作成され1で削除される
 *	7.lockWの待ち合わせ
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 3)client lockW LOCK_A
 *	 4)暫くしてからさらにclient lockW LOCK_Aを起動
 *	[結果]
 *	 待ち合わせが発生する
 *	8.lockWの待ち合わせ中のmaster切り替え
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client lockW LOCK_A
 *	 5)暫くしてからさらにclient lockW LOCK_Aを起動
 *	 6)暫くしてからserver 0をダウンし再起動する
 *		server 0 PFS-0 autonomic 1
 *	[結果]
 *	 待ち合わせが発生する
 *	9.lockR+lockR
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client lockR LOCK_A
 *	 5)暫くしてからさらにclient lockR LOCK_Aを起動
 *	[結果]
 *	 両者ともlockRがとれる
 *	10.lockW+lockR
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client lockW LOCK_A
 *	 5)暫くしてからさらにclient lockR LOCK_Aを起動
 *	[結果]
 *	 待ち合わせが発生する
 *	10.write
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client write w f
 *	[結果]
 *	 各サーバにfファイルが作成される
 *	 ただし、flushされていない場合がある
 *	11.read
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client read f r
 *	[結果]
 *	 rファイルが作成される
 *	 diff r wで確認
 *	12.存在しないファイルのread
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client read NO_FILE
 *	[結果]
 *	 エラー終了
 *	13.削除
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client delete f
 *	[結果]
 *	 fファイルが削除される
 *	14.outofband
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client writeoutofband w f
 *	[結果]
 *	  fファイル,帯域外ファイルが作成される
 *	 5)server 3 PFS-3 autonomic 2
 *	[結果]
 *	  3にfファイル,帯域外ファイルが作成される
 *	 6)server 3 PFS-3 export 2 EXPORT_2
 *	 7)server 3 PFS-3 import EXPORT_2
 *	  3にfファイル,帯域外ファイルが作成される
 *	15.outofband catchupで継承される
 *	 1)server 0 PFS-0
 *	 2)server 1 PFS-1
 *	 3)server 2 PFS-2
 *	 4)client writeoutofband w f
 *	 3)server 3 PFS-3
 *	[結果]
 *	  3にfファイル,帯域外ファイルが作成される
 *
 */

static int	_EventDirGetCount_ = 0;
static int	_EventDirCreDelCount_ = 0;
static int	_EventDirUpdtCount_ = 0;
static int	_EventLockGetCount_ = 0;
static int	_EventQueueGetCount_ = 0;
static int	_EventQueuePostCount_ = 0;
static int	_EventQueueWaitCount_ = 0;

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
	_EventLockGetCount_ ++;
	Printf("PFSEventLock:LockName[%s]\n", pEvent->el_Name );
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
PFSEventCallback( struct Session *pSession, PFSHead_t *pF )
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

		default: Printf("Cmd=0x%x\n", pF->h_Head.h_Cmd ); break;
	}
	return( 0 );
}

void
usage()
{
	printf("cmd cell session                          ... セッション開始/終了\n");
	printf("cmd cell lockW name                       ... ロック\n");
	printf("cmd cell lockR name                       ... ロック\n");
	printf("cmd cell writeinband fromFile [toFile]   ... write filename\n");
	printf("cmd cell writeoutofband fromFile [toFile]  ... write filename\n");
	printf("cmd cell write fromFile [toFile]          ... write filename\n");
	printf("cmd cell read fromFile [toFile]           ... read filename\n");
	printf("cmd cell delete name                      ... delete filename\n");
	printf("cmd cell ephemeral fromFile [toDirctory]  ... ephemeral file (sleep 50)\n");
	printf("cmd cell mkdir name                       ... mkdir directory\n");
	printf("cmd cell rmdir name                       ... rmdir directory\n");
	printf("cmd cell readdir name                     ... read directory\n");
	printf("cmd cell eventdir name version count      ... event directory\n");
	printf("cmd cell eventlockW name count            ... event lock\n");
	printf("cmd cell eventlockR name count            ... event lock\n");
	printf("cmd cell writeinband2 fromFile [toFile]  ... write filename, no sleep\n");
	printf("cmd cell writeoutofband2 fromFile [toFile] ... write filename, no sleep\n");
	printf("cmd cell write2 fromFile [toFile]         ... write filename, no sleep\n");
	printf("cmd cell ephemeral2 fromFile [toDirctory] ... (sleep 1)ephemeral file\n");
	printf("cmd cell ephemeral3 fromFile [toDirctory] ... (no sleep)ephemeral file\n");
	printf("cmd cell eventdir2 name version count     ... event directory (no dir Info.)\n");
	printf("cmd cell lockwrite fromFile [toFile]      ... lockW + write + Unlock\n");
	printf("cmd cell lockread fromFile [toFile]       ... lockR + read + Unlock\n");
	printf("cmd cell lockwriteloop fromFile toFileTop n(0< <9999)\n       ... (n-loop) x (SessionOpen + lockW + write + Unlock + SessionClose)\n");
	printf("cmd cell lockwriteloop2 fromFile toFileTop n(0< <9999)\n       ... SessionOpen + (n-loop) x (lockW + write + Unlock) + SessionClose\n");
	printf("cmd cell lockreadloop fromFile toFileTop n(0< <9999)\n       ... (n-loop) x (SessionOpen + lockR + read + Unlock + SessionClose)\n");
	printf("cmd cell lockreadloop2 fromFile toFileTop n(0< <9999)\n       ... SessionOpen + (n-loop) x (lockR + read + Unlock) + SessionClose\n");
	printf("cmd cell lockW2 name tm                    ... ロック、tm: sleep time\n");
	printf("cmd cell post name data\n");
	printf("cmd cell wait name pBuff Len\n");
	printf("cmd cell eventQueue name\n");
	printf("cmd cell stat name\n");
	printf("cmd cell truncate name len\n");
	printf("cmd cell abort\n");
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

int
main( int ac, char* av[] )
{
	int	fd;
	File_t	*pF;
	int	Ret;
	int	i;
	char	*pFrom, *pTo;

	struct stat sb; char *pBuff = NULL; int Len;

	int loopnum, findex;
	char ofile[64], rfile[64];

	if( ac < 3 ) {
		usage();
		exit( -1 );
	}
/*
 *	Initialize
 */
	struct Session	*pSession = NULL, *pSessionEvent = NULL;
	char	Buff[DATA_SIZE_WRITE];
	// ファイル毎にLOCK名を生成するため
	char	wlockName[64];

	PFSDirent_t	aEntry[20];
	int32_t			Number = 20;

	char	ReadBuff[1024*100];


	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession ) {
		printf("Can't get address [%s].\n", av[1] );
		exit( -1 );
	}

	if( getenv("LOG_SIZE") ) {
		int	log_size;
		log_size = atoi( getenv("LOG_SIZE") );

#ifdef	ZZZ
		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, stderr );
#else
		PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, 
								stderr, DEFAULT_LOG_LEVEL );
#endif
	}

	pSessionEvent = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );

	PaxosSessionSetLog( pSessionEvent, PaxosSessionGetLog( pSession ) );

	memset( Buff, 0, sizeof(Buff) );

	size_t nWlimit = DEFAULT_OUT_MEM_MAX-sizeof(PaxosSessionHead_t)-
		sizeof(PFSWriteReq_t);

	if( !strcmp( av[2], "session" ) ) {
		Printf("SessionOpen ENT: Master is %d.\n", 
						PaxosSessionGetMaster( pSession ) );
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "lockW" ) ) {
		if( ac < 4 )	goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;
		Ret = PFSLockW( pSession, av[3] );
		Printf("PFSLockW EXT: Ret[%d] errno[%d]\n", Ret, errno );

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "lockW2" ) ) {
		if( ac < 5 )	goto err1;

		int stm = atoi( av[4] );

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;

		Ret = PFSLockW( pSession, av[3] );
		Printf("PFSLockW EXT: Ret[%d] errno[%d]\n", Ret, errno );

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep %d\n", stm);	sleep( stm );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose EXT:Ret[%d] errno[%d]\n", Ret, errno );

	} else if ( !strcmp( av[2], "lockR" ) ) {
		if( ac < 4 )	goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen EXT: Ret[%d] errno[%d]\n", Ret, errno );
		if( Ret !=0 ) goto end;

		Ret = PFSLockR( pSession, av[3] );
		Printf("PFSLockR EXT: Ret[%d] errno[%d]\n", Ret, errno );

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		Printf("sleep 50\n");	sleep( 50 );
		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSUnlock( pSession, av[3] );
		Printf("PFSUnlock EXT:Ret[%d] errno[%d]\n", Ret, errno );

		Ret = PaxosSessionClose( pSession );
		Printf("SessionClose EXT:Ret[%d] errno[%d]\n", Ret, errno );

	} else if ( !strcmp( av[2], "writeinband" ) 
				|| !strcmp( av[2], "writeinband2" ) ) {
		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d.\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		pF = PFSOpen( pSession, pTo, 0 );
		if( !pF ) {
			Printf("PFSOpen: errno[%d] Master is %d.\n",
					errno, PaxosSessionGetMaster( pSession)  );
			goto end;
		}
		while( (Ret = read( fd, Buff, sizeof(Buff) )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWriteOutOfBand( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("PFSWriteInBand: Ret[%d] errno[%d] master=%d\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );

		if ( !strcmp( av[2], "writeinband" ) )  {
			Printf("sleep 50\n");	sleep( 50 );
		}
		PFSClose( pF );
		close( fd );

		Ret = PaxosSessionClose( pSession );
		if ( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "writeoutofband" ) 
				|| !strcmp( av[2], "writeoutofband2" ) ){
		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen: Ret[%d] errno[%d] Master is %d.\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		pF = PFSOpen( pSession, pTo, 0 );
		if( !pF ) {
			Printf("PFSOpen:errno[%d]\n", errno );
			goto end;
		}
		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWriteOutOfBand( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("PFSWriteOutOfBand:Ret[%d] errno[%d] Master is %d\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
		if ( !strcmp( av[2], "writeoutofband" ) ) {
			Printf("sleep 50\n");	sleep( 50 );
			Printf("Now Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		}
		PFSClose( pF );
		Free( pBuff );
		close( fd );

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "write" ) 
				|| !strcmp( av[2], "write2" ) ){
		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto err1;

		Printf("PID[%d] File size=%d\n", getpid(), Len );

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d.\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		pF = PFSOpen( pSession, pTo, 0 );
		if( !pF ) {
			Printf("PFSOpen:errno[%d]\n", errno );
			goto end;
		}
		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWrite( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("PFSWrite:Ret[%d] errno[%d] W_File[%s] Master is %d\n",
					Ret, errno, pTo, PaxosSessionGetMaster( pSession ) );
		if ( !strcmp( av[2], "write" ) ) {
			Printf("sleep 50\n");	sleep( 50 );
			Printf("Now Master is %d.\n", PaxosSessionGetMaster( pSession ) );
		}
		PFSClose( pF );
		Free( pBuff );
		close( fd );

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "lockwrite" ) ) {
		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto err1;
		Printf("PID[%d] File size=%d\n", getpid(), Len );

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d.\n",
				Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		sprintf( wlockName,"LCK%s",pTo);
		Ret = PFSLockW( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockW(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno );
			goto end;
		}

		pF = PFSOpen( pSession, pTo, 0 );
		if( !pF ) {
			Printf("PfsOpen: errno[%d]\n",errno );
			goto end;
		}

		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWrite( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("PFSWrite:Ret[%d] errno[%d] Master is %d\n", 
			Ret, errno, PaxosSessionGetMaster( pSession ) );
		PFSClose( pF );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
		}
		Free( pBuff );
		close( fd );
		if( Ret != 0 ) goto end;

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "lockwriteloop" ) ) {
		if( ac < 6 )	goto err1;
		if( ac >= 6 ) {
			pFrom	= av[3];
			pTo		= av[4];
			loopnum = atoi( av[5] );
		}
		if( loopnum < 1 || 9999 < loopnum ) {
			Printf("Error loopnum=%d\n", loopnum );
			goto err1;
		}

		findex = 1;
		/************************/
		while( loopnum > 0 ) {
		/************************/
		ofile[0] = '\0';
		sprintf( ofile, "%s-%04d", pTo, findex);

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto err1;

		Printf("PID[%d] %u File size=%d\n", getpid(), (uint32_t)time(0), Len );

		Ret = PaxosSessionOpen( pSession, 1230 + loopnum, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d.\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}
		sprintf( wlockName, "LCK%s",ofile );
		Ret = PFSLockW( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockW(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		pF = PFSOpen( pSession, ofile, 0 );
		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWrite( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("%u PFSWrite:Ret[%d] errno[%d] Master is %d\n", 
			(uint32_t)time(0),Ret, errno, PaxosSessionGetMaster( pSession ) );

		PFSClose( pF );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
		}
		Free( pBuff );
		close( fd );
		if( Ret != 0 ) goto end;

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}
		/************************/
		Printf("lockwriteloop: NUM=%d\n",findex);
		loopnum--;
		findex++;
		}
		/************************/
	} else if ( !strcmp( av[2], "lockwriteloop2" ) ) {

		if( ac < 6 )	goto err1;
		if( ac >= 6 ) {
			pFrom	= av[3];
			pTo		= av[4];
			loopnum = atoi( av[5] );
		}
		if( loopnum < 1 || 9999 < loopnum ) {
			Printf("Error loopnum=%d\n", loopnum );
			goto err1;
		}

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d. ## PID[%d] ##\n",
					Ret, errno, PaxosSessionGetMaster( pSession ), getpid() );
			goto end;
		}

		findex = 1;
		/************************/
		while( loopnum > 0 ) {
		/************************/
		ofile[0] = '\0';
		sprintf( ofile, "%s-%04d", pTo, findex);

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto err1;

		Printf("PID[%d] %u File size=%d\n", getpid(),(uint32_t)time(0), Len );

		sprintf( wlockName, "LCK%s",ofile );
		Ret = PFSLockW( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockW(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		pF = PFSOpen( pSession, ofile, 0 );
		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWrite( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("%u PFSWrite:Ret[%d] errno[%d] Master is %d\n", 
		(uint32_t)time(0), Ret, errno, PaxosSessionGetMaster( pSession ) );

		PFSClose( pF );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
		}
		Free( pBuff );
		close( fd );

		if( Ret !=0 ) goto end;
		/************************/
		Printf("lockwriteloop2: NUM=%d\n",findex);
		loopnum--;
		findex++;
		}
		/************************/
		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "read" ) ){
		int	n = 0;
		int Ret2 = 0;

		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}
		fd = -1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if ( Ret != 0 ) {
			Printf("SessionOpen: Ret[%d] errno[%d] Master is %d. PID[%d]\n",
				Ret, errno, PaxosSessionGetMaster( pSession ), getpid() );
			goto end;
		}

		fd = open( pTo, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		pF = PFSOpen( pSession, pFrom, 0 );
		if ( !pF ) {
			Printf("PFSOpen: errno[%d] Master is %d. PID[%d]\n",
				errno, PaxosSessionGetMaster( pSession ), getpid() );
			goto end;
		}
		while( (Ret = PFSRead( pF, ReadBuff, sizeof(ReadBuff) )) > 0 ) {
			n += Ret;
			Ret2 = write( fd, ReadBuff, Ret );
			if( Ret2 < 1 ) Printf("write():Ret2[%d] errno[%d]\n", Ret2, errno);
		}
		Printf("PFSRead:Ret[%d] errno[%d] Master is %d. File-size=%d\n", 
	 			Ret, errno, PaxosSessionGetMaster(pSession), n);
		PFSClose( pF );
		close( fd );

		Ret = PaxosSessionClose( pSession );
		if ( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "lockread" ) ){
		int	n = 0;
		int Ret2 = 0;

		if( ac < 4 )	goto err1;
		if( ac >= 5 ) {
			pFrom	= av[3];
			pTo		= av[4];
		} else {
			pFrom = pTo = av[3];
		}
		fd = -1;


		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret !=0 ) {
			Printf("SessionOpen: Ret[%d] errno[%d] Master is %d. ## PID[%d] ##\n",
					Ret, errno, PaxosSessionGetMaster( pSession ), getpid() );
			goto end;
		}

		sprintf( wlockName, "LCK%s", pFrom );
		Ret = PFSLockR( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockR(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		fd = open( pTo, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		pF = PFSOpen( pSession, pFrom, 0 );
		while( (Ret = PFSRead( pF, ReadBuff, sizeof(ReadBuff) )) > 0 ) {
			n += Ret;
			Ret2 = write( fd, ReadBuff, Ret );
			if( Ret2 < 1 ) Printf("write():Ret2[%d] errno[%d]\n", Ret2, errno);
		}
		Printf("PID[%d] %u PFSRead:Ret[%d] errno[%d] Master is %d. File-size=%d\n", 
 		getpid(), (uint32_t)time(0),Ret, errno, PaxosSessionGetMaster(pSession), n);

		PFSClose( pF );
		close( fd );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "lockreadloop" ) ){
		int	n = 0;
		int Ret2 = 0;

		if( ac < 6 )	goto err1;
		if( ac >= 6 ) {
			pFrom	= av[3];
			pTo		= av[4];
			loopnum = atoi( av[5] );
		}
		if( loopnum < 1 || 9999 < loopnum ) {
			Printf("Error loopnum=%d\n", loopnum );
			goto err1;
		}

		fd = -1;

		findex = 1;
		/************************/
		while( loopnum > 0 ) {
		/************************/
		n = 0;

		rfile[0] = '\0';
		sprintf( rfile, "%s-%04d", pFrom, findex);
		ofile[0] = '\0';
		sprintf( ofile, "%s-%04d", pTo, findex);
		//Printf("PID[%d] READ-File(%s) OutFile(%s)\n", getpid(),rfile,ofile);

		Ret = PaxosSessionOpen( pSession, 1230 + loopnum, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d.\n",
					Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		sprintf( wlockName, "LCK%s", rfile );
		Ret = PFSLockR( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockR(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		fd = open( ofile, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		pF = PFSOpen( pSession, rfile, 0 );
		if( !pF ) {
			Printf("PFSOpen: errno[%d]\n",errno);
			goto end;
		}
		while( (Ret = PFSRead( pF, ReadBuff, sizeof(ReadBuff) )) > 0 ) {
			n += Ret;
			Ret2 = write( fd, ReadBuff, Ret );
			if( Ret2 < 1 ) Printf("write():Ret2[%d] errno[%d]\n", Ret2, errno);
		}
		Printf("PID[%d] %u PFSRead:Ret[%d] errno[%d] Master is %d. File-size=%d\n", 
 		getpid(), (uint32_t)time(0),Ret, errno, PaxosSessionGetMaster(pSession), n);

		PFSClose( pF );
		close( fd );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}
		/************************/
		Printf("lockreadloop: NUM=%d\n",findex);
		loopnum--;
		findex++;
		}
		/************************/
	} else if ( !strcmp( av[2], "lockreadloop2" ) ){
		int	n = 0;
		int Ret2 = 0;

		if( ac < 6 )	goto err1;
		if( ac >= 6 ) {
			pFrom	= av[3];
			pTo		= av[4];
			loopnum = atoi( av[5] );
		}
		if( loopnum < 1 || 9999 < loopnum ) {
			Printf("Error loopnum=%d\n", loopnum );
			goto err1;
		}

		fd = -1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d] Master is %d. ## PID[%d] ##\n",
					Ret, errno, PaxosSessionGetMaster( pSession ), getpid() );
			goto end;
		}

		findex = 1;
		/************************/
		while( loopnum > 0 ) {
		/************************/
		n = 0;

		rfile[0] = '\0';
		sprintf( rfile, "%s-%04d", pFrom, findex);
		ofile[0] = '\0';
		sprintf( ofile, "%s-%04d", pTo, findex);
		//Printf("PID[%d] READ-File(%s) OutFile(%s)\n",getpid(),rfile,ofile);

		sprintf( wlockName, "LCK%s", rfile );
		Ret = PFSLockR( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSLockR(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}

		fd = open( ofile, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto err1;

		pF = PFSOpen( pSession, rfile, 0 );
		if( !pF ) {
			Printf("PFSOpen: errno[%d]\n",errno);
			goto end;
		}
		while( (Ret = PFSRead( pF, ReadBuff, sizeof(ReadBuff) )) > 0 ) {
			n += Ret;
			Ret2 = write( fd, ReadBuff, Ret );
			if( Ret2 < 1 ) Printf("write():Ret2[%d] errno[%d]\n", Ret2, errno);
		}
		Printf("PID[%d] %u PFSRead:Ret[%d] errno[%d] Master is %d. File-size=%d\n", 
 		getpid(), (uint32_t)time(0),Ret, errno, PaxosSessionGetMaster(pSession), n);

		PFSClose( pF );
		close( fd );

		Ret = PFSUnlock( pSession, wlockName );
		if( Ret != 0 ) {
			Printf("PFSUnlock(%s):Ret[%d] errno[%d]\n",wlockName,Ret,errno);
			goto end;
		}
		/************************/
		Printf("lockreadloop2: NUM=%d\n",loopnum);
		loopnum--;
		findex++;
		}
		/************************/
		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "delete" ) ){
		if( ac < 4 )	goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen: Ret[%d] errno[%d] Master is %d.\n",
			Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		Ret = PFSDelete( pSession, av[3] );
		if( Ret != 0 ) {
			Printf("PFSDelete:Ret[%d] File[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose EXT:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "ephemeral" ) ||
	            !strcmp( av[2], "ephemeral2" ) ||
	            !strcmp( av[2], "ephemeral3" ) ) {
		char	Temp[PATH_NAME_MAX];
		int		sleeptm = 50;

		if( ac < 4 )	goto err1;

		if( ac >= 5 ) {
			pFrom	= av[3];
			sprintf( Temp, "%s/%d", av[4], getpid() );
			pTo		= Temp;
		} else {
			sprintf( Temp, "./%d", getpid() );
			pFrom = av[3];
			pTo	= Temp;
		}

		if( !strcmp( av[2],"ephemeral") ) {
			sleeptm = 50;
		} else if( !strcmp( av[2],"ephemeral2") ) {
			sleeptm = 1;
		}

		fd = open( pFrom, O_RDWR|O_SYNC|O_CREAT, 0666 );
		if( fd < 0 )	goto end;

		fstat( fd, &sb );
		if( !(pBuff = Malloc( Len = sb.st_size ) ) ) goto end;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen: Ret[%d] errno[%d] Master is %d.\n",
			Ret, errno, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		pF = PFSOpen( pSession, pTo, FILE_EPHEMERAL );
		if( !pF ) {
			Printf("PFSOpen:pF=%p Master is %d.\n", 
				pF, PaxosSessionGetMaster( pSession ) );
			goto end;
		}

		while( (Ret = read( fd, pBuff, Len )) > 0 ) {
			char *p = pBuff;
			while (Ret > 0 ) {
				int ret = (Ret>=nWlimit)?
						nWlimit-1:Ret;
				ret = PFSWrite( pF, p, ret );
				if( ret < 0 )	break;
				Ret -= ret;
				p += ret;
			}
			if ( Ret != 0 ) break;
		}
		Printf("PFSWrite:errno[%d] Ephemeral-File[%s]\n", errno, pTo );

		if( strcmp( av[2],"ephemeral3") ) {
			Printf("sleep %d\n", sleeptm);	sleep( sleeptm );
		}

		Printf("Master is %d.\n", PaxosSessionGetMaster( pSession ) );

		Ret = PFSClose( pF );
		if( Ret != 0 ) {
			Printf("PFSClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

		Free( pBuff );
		close( fd );

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "mkdir" ) ){
		if( ac < 4 )	goto err1;
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

		Ret = PFSMkdir( pSession, av[3] );
		if( Ret != 0 ) {
			Printf("Mkdir:Ret[%d] errno[%d] dir[%s]\n", Ret, errno, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "rmdir" ) ){
		if( ac < 4 )	goto err1;
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

		Ret = PFSRmdir( pSession, av[3] );
		if( Ret != 0 ) {
			Printf("Rmdir:Ret[%d] errno[%d] dir[%s]\n", Ret, errno, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "readdir" ) ){
		if( ac < 4 )	goto err1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

                int index = 0;
                while ( sizeof(aEntry)/sizeof(aEntry[0]) == Number ) {

		Ret = PFSReadDir( pSession, av[3], aEntry, index, &Number );
		if( Ret != 0 ) {
			Printf("ReadDir:Ret[%d] errno[%d] Number[%d]\n", 
							Ret, errno, Number );
			goto end;
		}

		for( i = 0; i < Number; i++ ) {
			printf("%d [%s]\n", i+index, aEntry[i].de_Name );
		}
                index +=Number;
                }

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

	} else if ( !strcmp( av[2], "eventdir" ) ||
				!strcmp( av[2], "eventdir2" ) ) {
		if( ac < 6 )	goto err1;

		int			cnt;

		cnt		= atoi(av[5]);

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

		Ret = PaxosSessionOpen( pSessionEvent, PFS_SESSION_EVENT, FALSE );
		if( Ret != 0 ) {
			Printf("EventSessionOpen:Ret[%d] errno[%d]\n", Ret, errno );
			goto end;
		}

		Ret = PFSEventDirSet( pSessionEvent, av[3], pSessionEvent );
		Printf("EventDirSet:Ret[%d] errno[%d] dir_name[%s]\n", 
				Ret, errno, av[3] );
		if( Ret != 0 ) goto end;

		while( cnt-- > 0 ) {
			int32_t	count, omitted;

			Printf("\n*** (%d) Wait Event ***\n",atoi(av[5]) - cnt);
			Ret = PFSEventGetByCallback( pSessionEvent, &count, &omitted,
										PFSEventCallback );
			Printf(
			"EventGet:Total Count[%d] Cre/Del[%d] Updt[%d] Ret[%d] errno[%d] events=%d omit=%d\n",
			 _EventDirGetCount_, _EventDirCreDelCount_, _EventDirUpdtCount_, Ret, errno, 
					count, omitted );
			if( Ret != 0 ) {
				PFSEventDirCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}
		}

		Ret = PFSEventDirCancel( pSessionEvent, av[3], pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventDirCancel:Ret[%d] dir_name[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d]\n", Ret );
			goto end;
		}

		Ret = PaxosSessionClose( pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventSessionClose:Ret[%d]\n", Ret );
			goto end;
		}

	} else if ( !strcmp( av[2], "eventlockW" ) ){
		if( ac < 5 )	goto err1;
		int			cnt;
		cnt		= atoi(av[4]);

		Ret = PaxosSessionOpen( pSessionEvent, PFS_SESSION_EVENT, FALSE );
		if( Ret != 0 ) {
			Printf("EventSessionOpen:Ret[%d]\n", Ret );
			goto end;
		}

		Ret = PFSEventLockSet( pSessionEvent, av[3], pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventLockSet:Ret[%d] Lock(W)[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d] errno[%d]\n", Ret,errno );
			PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
			goto end;
		}

		while( cnt-- ) {
			int32_t	count, omitted;

			Printf("\n*** (%d) Wait Event ***\n",atoi(av[4]) - cnt);
			Ret = PFSLockW( pSession, av[3] );
			Printf("PFSLockW:Ret[%d] Lock[%s]\n", Ret, av[3] );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}

			Ret = PFSEventGetByCallback( pSessionEvent, &count, &omitted,
										PFSEventCallback );
			Printf("EventGet:Total Count[%d] Ret[%d]  Master is %d.\n", 
				_EventLockGetCount_, Ret, PaxosSessionGetMaster( pSession ) );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}

			Ret = PFSUnlock( pSession, av[3] );
			Printf("PFSUnlock:Ret[%d] Lock[%s]\n", Ret, av[3] );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}
		}

		Ret = PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventLockCancel:Ret[%d] Lock[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d]\n", Ret );
			goto end;
		}

		Ret = PaxosSessionClose( pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventSessionClose:Ret[%d]\n", Ret );
			goto end;
		}

	} else if ( !strcmp( av[2], "eventlockR" ) ){
		if( ac < 5 )	goto err1;
		int			cnt;
		cnt		= atoi(av[4]);

		Ret = PaxosSessionOpen( pSessionEvent, PFS_SESSION_EVENT, FALSE );
		if( Ret != 0 ) {
			Printf("EventSessionOpen:Ret[%d]\n", Ret );
			goto end;
		}

		Ret = PFSEventLockSet( pSessionEvent, av[3], pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventLockSet:Ret[%d] Lock(R)[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		if( Ret != 0 ) {
			Printf("SessionOpen:Ret[%d]\n", Ret );
			PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
			goto end;
		}

		while( cnt-- ) {
			int32_t	count, omitted;

			Printf("\n*** (%d) Wait Event ***\n",atoi(av[4]) - cnt);
			Ret = PFSLockR( pSession, av[3] );
			Printf("PFSLockR:Ret[%d] Lock[%s]\n", Ret, av[3] );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}

			Ret = PFSEventGetByCallback( pSessionEvent, &count, &omitted,
										PFSEventCallback );
			Printf("EventGet:Total Count[%d] Ret[%d] Master is %d.\n", 
				_EventLockGetCount_, Ret, PaxosSessionGetMaster( pSession ) );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}

			Ret = PFSUnlock( pSession, av[3] );
			Printf("PFSUnlock:Ret[%d] Lock[%s]\n", Ret, av[3] );
			if( Ret != 0 ) {
				PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
				goto end;
			}
		}
		Ret = PFSEventLockCancel( pSessionEvent, av[3], pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventLockCancel:Ret[%d] Lock[%s]\n", Ret, av[3] );
			goto end;
		}

		Ret = PaxosSessionClose( pSession );
		if( Ret != 0 ) {
			Printf("SessionClose:Ret[%d]\n", Ret );
			goto end;
		}

		Ret = PaxosSessionClose( pSessionEvent );
		if( Ret != 0 ) {
			Printf("EventSessionClose:Ret[%d]\n", Ret );
			goto end;
		}

	} else if ( !strcmp( av[2], "post" ) ){
		if( ac <5 ) goto err1;
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSPost( pSession, av[3], av[4], strlen(av[4]) );
		Printf("Post:Ret[%d]\n", Ret );
		if( Ret < 0 ) goto end;

		PaxosSessionClose( pSession );
		Printf("SessionPost:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "wait" ) ){
		char	Buff[1024];
		if(ac<4) goto err1;
		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSPeek( pSession, av[3], Buff, sizeof(Buff) );
		if( Ret < 0 ) goto end;
		Ret = PFSPeekAck( pSession, av[3] );
		if( Ret < 0 ) goto end;

		PaxosSessionClose( pSession );
		Printf("PFSWait:Ret[%d] [%s]\n", Ret, Buff );
		if( Ret != 0 ) goto end;

	} else if ( !strcmp( av[2], "eventQueue" ) ){

		int32_t	count, omitted;
		int	cnt;
		if( ac < 5) goto err1;
		if( ac >= 5 )	cnt	= atoi(av[4]);
		else			cnt	= 1;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_EVENT, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSEventQueueSet( pSession, av[3], pSession );
		Printf("eventQueueSet:Ret[%d]\n", Ret );
		if( Ret < 0 ) goto end;

		while( cnt-- ) {
			Ret = PFSEventGetByCallback( pSession, &count, &omitted,
										PFSEventCallback );
		}

		Ret = PFSEventQueueCancel( pSession, av[3], pSession );
		Printf("eventQueueCancel:Ret[%d]\n", Ret );
		if( Ret < 0 ) goto end;

		PaxosSessionClose( pSession );

	} else if ( !strcmp( av[2], "stat" ) ){
		struct	stat	Stat;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSStat( pSession, av[3], &Stat );
		if( Ret < 0 ) {
			Printf("PFSStat:Ret=%d errno=%d\n", Ret, errno );
		} else {
			Printf("PFSStat:[%s] type=0x%x size=%"PRIi64" mtime=%d\n",
				av[3], Stat.st_mode, Stat.st_size, (int)Stat.st_mtime );
		}

		PaxosSessionClose( pSession );

	} else if ( !strcmp( av[2], "truncate" ) ){
		struct	stat	Stat;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSStat( pSession, av[3], &Stat );
		if( Ret < 0 ) {
			Printf("PFSStat:Ret=%d errno=%d\n", Ret, errno );
		} else {
			Printf("PFSStat:[%s] type=0x%x size=%"PRIi64" mtime=%d\n",
				av[3], Stat.st_mode, Stat.st_size, (int)Stat.st_mtime );
		}
		Ret = PFSTruncate( pSession, av[3], atoi(av[4]) );

		Ret = PFSStat( pSession, av[3], &Stat );
		if( Ret < 0 ) {
			Printf("PFSStat:Ret=%d errno=%d\n", Ret, errno );
		} else {
			Printf("PFSStat:[%s] type=0x%x size=%"PRIi64" mtime=%d\n",
				av[3], Stat.st_mode, Stat.st_size, (int)Stat.st_mtime );
		}

		PaxosSessionClose( pSession );

	} else if ( !strcmp( av[2], "abort" ) ){
		pthread_t	th;
		int32_t	count, omitted;

		Ret = PaxosSessionOpen( pSession, PFS_SESSION_EVENT, FALSE );
		Printf("SessionOpen:Ret[%d]\n", Ret );
		if( Ret != 0 ) goto end;

		Ret = PFSEventQueueSet( pSession, "QUEUE", pSession );
		Printf("eventQueueSet:Ret[%d]\n", Ret );
		if( Ret < 0 ) goto end;

		pthread_create( &th, NULL, ThreadAbort, (void*)pSession );

		Ret = PFSEventGetByCallback( pSession, &count, &omitted,
										PFSEventCallback );
		Printf("PFSEventGet ret=%d\n", Ret );
		pthread_join( th, NULL );
	} else {
		goto err1;
	}
	if( pSession )	PaxosSessionFree( pSession );
	return( 0 );
err1:
	usage();
end:
	return( -1 );
}

