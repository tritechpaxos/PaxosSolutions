/******************************************************************************
*
*  TestPaxos.c 	--- test program of Paxos Library
*
*  Copyright (C) 2010-2014,2016 triTech Inc. All Rights Reserved.
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

/**
 * 試験
 *	環境
 *	 PAXOS_SERVER_MAX=5
 *	 PAXOS_MAX=5
 *	 down.sh	全てのサーバをshutdown
 *	 rmperm.sh	永続化データを削除(最初に実行)
 *	 TestPaxos	テストプログラム
 *	 admin		管理コマンド
 *
 *	1.片方向の通信断の検知
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)master選択後
 *			admin 0 stop 1で0から1の送信断
 *		[結果]
 *		  masterなしとなる
 *	2.通信回復
 *		1)admin 0 restart 1で0から1の送信回復
 *		[結果]
 *		  masterを降りた時誕生を更新するのでmasterは1
 *		  通信回復時にmaster選択を乱さない
 *	3.round獲得
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		[結果]
 *		 一定時間後、0がroundを獲得する
 *		4)admin 1 newround
 *		[結果]
 *		 1がroudを獲得するが、一定時間後、0がroundを獲得する
 *	4.round獲得不能
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 1 recvstop 9 1(COLLECTを受け付けない)
 *		5)admin 0 newround
 *		[結果]
 *			admin 0 paxosでroundが獲得できないのを確認
 *		6)admin 1 newround
 *		[結果]
 *			admin 1 paxosでround獲得
 *		7)admin 1 recvstop 9 0(COLLECT解除)
 *		[結果]
 *		 実装上は、masterのみがroundを獲得するので、
 *		 一定時間後、0がroundを獲得する
 *	4.初期データ投入サーバでnewround
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		 0のround獲得を待つ
 *		4)admin 0 command AAA 0
 *		5)admin 0 command CCC 2
 *		6)admin 0 newround
 *		[結果]
 *			admin 0/1/2 paxosで決定を確認
 *			CCCは実行保留
 *		7)admin 0 command BBB 2
 *		8)admin 0 newround
 *		[結果]
 *		 BBB,CCCの順に実行
 *	5.先にroundに投入されたデータが有効
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 0 recvstop 6 1(ALIVEを受け付けない)
 *		5)admin 1 recvstop 6 1(ALIVEを受け付けない)
 *		6)admin 2 recvstop 6 1(ALIVEを受け付けない)
 *		7)admin 0 recvstop 9 1(COLLECTを受け付けない)
 *		8)admin 1 command AAA 0
 *		9)admin 1 newround(roundに載せる)
 *		10)admin 0 command BBB 0
 *		11)admin 0 newround
 *		[結果]
 *			AAAは実行
 *			BBBは拒否
 *	6.決定の引き継ぎ
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		 master選択後
 *		4)admin 1 recvstop 6 1(ALIVEを受け付けない)
 *		5)admin 1 recvstop 14 1(SUCCESSを受け付けない)
 *		6)admin 2 recvstop 6 1(ALIVEを受け付けない)
 *		7)admin 2 recvstop 14 1(SUCCESSを受け付けない)
 *		8)admin 0 command AAA 0
 *		9)admin 0 newround
 *		[結果]
 *			0でAAAは実行
 *		10)admin 0 down
 *		11)TestPaxos 3
 *		12)admin 3 command BBBB 0
 *		13)admin 3 newround
 *		14)admin 3 newround
 *		[結果]
 *			3でAAAは実行
 *			BBBは拒否
 *		15)admin 1 newround
 *		[結果]
 *			1でAAAは実行
 *		16)admin 2 newround
 *		[結果]
 *			2でAAAは実行
 *	7.accept後の決定は引き継がれる
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		 master選択後
 *		4)admin 1 recvstop 14 1(SUCCESSを受け付けない)
 *		5)admin 1 recvstop 8 1(CATCHUP応答を受け付けない)
 *		6)admin 2 recvstop 14 1(SUCCESSを受け付けない)
 *		7)admin 2 recvstop 8 1(CATCHUP応答を受け付けない)
 *		8)0からAAAAを投入
 *		[結果]
 *			0でAAAは実行
 *		9)admin 0 down
 *		10)admin 2 command BBBB 0
 *		11)admin 2 recvstop 14 0(SUCCESSを解除)
 *		12)TestPaxos 3
 *		[結果]
 *		 再起動後の2でBBBBが拒否される
 *		 1,2,3でAAAAが実行される
 *	8.collectを受け付けない
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)admin 1 recvstop 9 1(COLLECTを受け付けない)
 *		4)TestPaxos 2
 *		5)masterが選択される前に
 *			admin 2 recvstop 9 1(COLLECTを受け付けない)
 *		6)0からコマンド投入	0がmaster
 *		[結果]
 *		 timeoutでPAXOS_REJECTEDが返される
 *	9.lastを受け付けない
 *		1)TestPaxos 0
 *		2)admin 0 recvstop 10 1(LASTを受け付けない)
 *		3)TestPaxos 1
 *		4)TestPaxos 2
 *		5)masterが選択される前に
 *		6)0からコマンド投入	0がmaster
 *		[結果]
 *		 timeoutでPAXOS_REJECTEDが返される
 *	10.beginを受け付けない
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 1 recvstop 12 1(BEGINを受け付けない)
 *		5)0からコマンド投入	0がmaster
 *		[結果]
 *		 masterである限り永遠に再試行 
 *	11.acceptを受け付けない
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 0 recvstop 13 1(ACCEPTを受け付けない)
 *		5)0からコマンド投入	0がmaster
 *		[結果]
 *		 masterである限り永遠に再試行 
 *		 admin 0 paxos 1でroundが更新されている
 *	12.successを受け付けない
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 1 recvstop 14 1(SUCCESSを受け付けない)
 *		5)admin 1 recvstop 8 1(CATCHUP応答を受け付けない)
 *		6)0からコマンド投入	0がmaster
 *		[結果]
 *		 0,2でコマンドが実行される
 *		7)admin 1 recvstop 8 0(CATCHUP応答)
 *		[結果]
 *		 1でコマンドが実行される
 *	13.抑止
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)admin 0 command AAAA 0
 *		5)admin 0 suppress 100
 *		6)admin 0 begin 0
 *		[結果]
 *		 masterでacceptされないと実行されない
 *		 suppressが解除されroundチェック後、実行される
 *	14.抑止
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)TestPaxos 3
 *		5)admin 0 command AAAA 0
 *		6)admin 1 suppress 100
 *		7)admin 0 begin 0
 *		[結果]
 *		 全てで実行される
 *		 1はacceptを返さないがsuccessを受け入れる
 *		 抑止中でもページ更新行われるのでページ削除も抑止する
 *	15.catchup
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0からコマンド投入	0がmaster
 *		5)admin 0 down
 *		6)TestPaxos 0
 *		[結果]
 *		 0でコマンドが再実行される
 *	16.catchup不能
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0からコマンド投入	0がmaster
 *		5)0から1ページ分投入し、暫く待つ
 *		6)0から1ページ分投入
 *		7)admin 0 paxos でキャシュページ確認
 *		8)TestPaxos 3
 *		[結果]
 *		 3は終了する
 *	16.catchup
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0から1ページ分投入
 *		5)admin 0 removepage
 *		6)admin 1 catchupstop 1
 *		7)admin 2 catchupstop 1
 *		8)TestPaxos 3
 *		[結果]
 *		 暫く待っても3はcatchupしない		 
 *		9)admin 2 catchupstop 0
 *		[結果]
 *		 はcatchup
 *	16.catchup
 *	20.実行がreplicaで失敗する
 *		1)TestPaxos 0
 *		2)TestPaxos 1 1(Seq1でアボート)
 *		3)0からコマンド投入	0がmaster
 *		4)TestPaxos 1
 *		[結果]
 *		 0でコマンドが実行される
 *		 1でコマンドが再実行される
 *	21.実行がmasterで失敗する
 *		1)TestPaxos 0 1(Seq1でアボート)
 *		2)TestPaxos 1
 *		3)0からコマンド投入	0がmaster
 *		4)TestPaxos 0
 *		[結果]
 *		 1でコマンドが実行される
 *		 0でコマンドが再実行される
 *	22.REDIRECT
 *		1)TestPaxos 0
 *		2)admin 0 recvstop 6 1(ALIVE)
 *		3)admin 0 command AAAA 0
 *		4)admin 0 newround
 *		5)TestPaxos 1
 *		6)admin 1 newround
 *		7)admin 1 newround
 *		[結果]
 *		 redirectが発生する
 *	23.autonomic
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0からコマンド投入	0がmaster
 *		5)TestPaxos 3 autonomic 0
 *		6)admin 0 paxos
 *		7)admin 3 paxos
 *		[結果]
 *		 復旧を確認
 *	24.autonomic
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0からコマンド投入	0がmaster
 *		5)admin 0 command Vaaa 2	Vは帯域外
 *		6)admin 0 newround
 *		7)TestPaxos 3 autonomic 0
 *		8)admin 0 paxos
 *		9)admin 3 paxos
 *		[結果]
 *		 Gap,帯域外データも含めて復旧を確認
 *	24.export/import
 *		1)TestPaxos 0
 *		2)TestPaxos 1
 *		3)TestPaxos 2
 *		4)0からコマンド投入	0がmaster
 *		5)admin 0 command Vaaaa 2
 *		6)admin 0 newround
 *		7)TestPaxos 0 export EXPORT_0
 *		8)TestPaxos 3 import EXPORT_0
 *		9)admin 0 paxos
 *		10)admin 3 paxos
 *		[結果]
 *		 Gap,帯域外データも含めて復旧を確認
 */

//#define	_DEBUG_

#include	"Paxos.h"

#include	<ctype.h>
#include	<netdb.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

typedef struct Command {
	PaxosHead_t	c_Head;
	char		c_Command[128];
	int			c_Id;
} Command_t;

#define	PRINTF(fmt,...)	printf("[%u] " fmt, (uint32_t)time(0), ##__VA_ARGS__ )

Queue_t	WaitQueue;
int	MyId;

uint32_t	AbortSeq = -1;
/*
 *	Outbound marshal/unmarshal
 */
int
OutboundMarshal( struct Paxos *pPaxos, void *pVoid, IOReadWrite_t *pW )
{
	Command_t	*pCmd = (Command_t*)pVoid;
	int	l;

	l = strlen(pCmd->c_Command);
	IOMarshal( pW, &l, sizeof(l) );
	IOMarshal( pW, pCmd->c_Command, l );

	printf("l=%d [%s]\n", l, pCmd->c_Command );
	return( 0 );
}

int
OutboundUnmarshal( struct Paxos *pPaxos, void *pVoid, IOReadWrite_t *pR )
{
	int	l;
	char Buff[256];

	memset( Buff, 0, sizeof(Buff) );

	IOUnmarshal( pR, &l, sizeof(l) );
	IOUnmarshal( pR, Buff, l );

	printf("l=%d [%s]\n", l, Buff );
	return( 0 );
}

/*
 *	Critical section
 */
int
Election( struct Paxos* pPaxos, PaxosNotify_t Notify, 
			int32_t server, uint32_t epoch, int32_t from )
{
	switch( (int)Notify ) {
	case PAXOS_Im_NOT_MASTER:
		PRINTF("===I'm not Master(%d epoch=%u from=%d)===\n",
								server, epoch, from );
		break;
	case PAXOS_MASTER_ELECTED:
		PRINTF("===Master(%d epoch=%u from=%d) is elected===\n", 
								server, epoch, from );
		break;
	case PAXOS_MASTER_CANCELED:
		PRINTF("===Master(%d epoch=%u from=%d) is canceled===\n", 
								server, epoch, from );
		break;
	case PAXOS_Im_MASTER:
		PRINTF("===I'm Master(%d epoch=%u from=%d)===\n",
								server, epoch, from );
		break;
	}
	return( 0 );
}

int
Notify( struct Paxos* pPaxos, PaxosMode_t Mode, PaxosNotify_t Notify, 
	uint32_t seq, int server, uint32_t epoch, int32_t len, void *pC, int from )
{
	Command_t*	pCommand = (Command_t*)pC;

	DBG_ENT();

	switch( Notify ) {
		case PAXOS_NOOP:
			PRINTF("=== NOOP[%d] ===\n", seq );
			break;
		case PAXOS_REJECTED:
			PRINTF("=== Command[%d %u %d %d %s] === ", 
				Notify, seq, server, pCommand->c_Id, pCommand->c_Command );
			printf("REJECTED\n");
			break;

		case PAXOS_RECOVER:
			PRINTF("=== RECOVER SNAPSHOT(%u) ===\n", seq );
			return( 0 );
		case PAXOS_NOT_GET_PAGE:
			PRINTF("=== NOT_GET_PAGE(%d) ===\n",  seq );
			break;
		case PAXOS_VALIDATE:
			PRINTF("=== VALIDATE(%d %d) ===\n",  Mode, seq );
			break;
		default:
			ABORT();
			break;
	}
	DBG_EXT(0);
	return( 0 );
}

void*
ThreadExec( void* pArg )
{
	struct Paxos* pPaxos = (struct Paxos*)pArg;
	Msg_t	*pMsg;
	PaxosExecute_t*	pPH;
	Command_t* pC;

	while( (pMsg = PaxosRecvFromPaxos( pPaxos )) ) {
		pPH	= (PaxosExecute_t*)MsgFirst( pMsg );
		pC	= (Command_t*)MsgNext( pMsg );
		switch( (int)pPH->e_Notify ) {

			case PAXOS_ACCEPTED:
//				pC = (Command_t*)(pPH+1);	
				PRINTF("=== Seq=%u Command[%s] ===\n", 
					pPH->e_Seq, pC->c_Command );

				if( pPH->e_Seq == AbortSeq )	kill(0, SIGINT );

				if( PaxosIsIamMaster( pPaxos ) ) {
					list_init( &pC->c_Head.p_Lnk );
					QueuePostEntry( &WaitQueue, pC, c_Head.p_Lnk );
					printf("REPLY\n");
				} else {
					printf("NOT REPLY\n");
				}
		}
		MsgDestroy( pMsg );
	}
	return( NULL );
}

void*
ThreadWait( void* pArg )
{
	struct Paxos* pPaxos = (struct Paxos*)pArg;
	Command_t* pC;

	while( 1 ) {
		QueueTimedWaitEntryWithMs( &WaitQueue, pC, c_Head.p_Lnk, 
					PaxosDecidedTimeout(pPaxos)*1000 );
		if( !pC )	printf("TIMEOUT =>");
		else {
			Free( pC );
			printf("=>");
		}
	}
	return( NULL );
}

int	id;
struct Paxos* pPaxos;
PaxosCell_t	*pPaxosCell;

int		AutonomicId	= -1;

void*
ThreadAutonomic( void* pArg )
{
	struct Paxos	*pPaxos = (struct Paxos*)pArg;
	struct sockaddr_in	ListenAddr, From;
	int ListenFd;
	int	len;
	int	Fd;
	IOSocket_t	IO;

	if( !pPaxosCell->c_aPeerAddr[id].a_Active ) {
		DBG_PRT("id=%d is not active.\n", id );
		exit( -1 );
	}
	ListenAddr	= pPaxosCell->c_aPeerAddr[id].a_Addr;
//	ListenAddr.sin_port = htons( id + PAXOS_UDP_PORT_BASE + 10 );

	if( ( ListenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		printf("socket open error.\n");
		exit( -1 );
	}
	if( bind( ListenFd, (struct sockaddr*)&ListenAddr, 
								sizeof(ListenAddr) ) < 0 ) {
		printf("bind error.\n");
		exit( -1 );
	}
	listen( ListenFd, 5 );

	len	= sizeof(From);
loop:
	Fd	= accept( ListenFd, (struct sockaddr*)&From, (socklen_t*)&len );
	if( Fd < 0 ) {
		printf("accept error.\n");
		exit( -1 );
	}
	IOSocketBind( &IO, Fd );
	PaxosFreeze( pPaxos );
	PaxosMarshal( pPaxos, (IOReadWrite_t*)&IO );
	PaxosDefreeze( pPaxos );
	close( Fd );
goto loop;
	return( NULL );
}

int
GetAutonomic( struct Paxos *pPaxos, int j )
{
	int	Fd;
	struct sockaddr_in	ListenAddr;
	IOSocket_t	IO;

	if( ( Fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		printf("socket open error.\n");
		exit( -1 );
	}
	if( !pPaxosCell->c_aPeerAddr[j].a_Active ) {
		DBG_PRT("j=%d is not active.\n", j );
		exit( -1 );
	}
	ListenAddr	= pPaxosCell->c_aPeerAddr[j].a_Addr;
//	ListenAddr.sin_port = htons( j + PAXOS_UDP_PORT_BASE + 10 );
	if( connect( Fd, (struct sockaddr*)&ListenAddr,
							sizeof(struct sockaddr_in)) < 0 ) {
		close( Fd );
		return( -1 );
	}
	IOSocketBind( &IO, Fd );
	PaxosUnmarshal( pPaxos, (IOReadWrite_t*)&IO );
	PaxosSnapshotValidate( pPaxos );
	close( Fd );
	return( 0 );
}

int
ExportToFile( int j, char *pFile )
{
	IOFile_t	*pIOFile;
	int	Fd;
	struct sockaddr_in	ListenAddr;
	IOSocket_t	IO;
	int32_t	Len, n;
	char	Buff[1024*4];
	IOReadWrite_t	*pR, *pW;
	int	Ret;

	if( ( Fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		printf("socket open error.\n");
		exit( -1 );
	}
	if( !pPaxosCell->c_aPeerAddr[j].a_Active ) {
		DBG_PRT("j=%d is not active.\n", j );
		exit( -1 );
	}
	ListenAddr	= pPaxosCell->c_aPeerAddr[j].a_Addr;
//	ListenAddr.sin_port = htons( j + PAXOS_UDP_PORT_BASE + 10 );
	if( connect( Fd, (struct sockaddr*)&ListenAddr,
							sizeof(struct sockaddr_in)) < 0 ) {
		close( Fd );
		return( -1 );
	}
	IOSocketBind( &IO, Fd );
	pR	= (IOReadWrite_t*)&IO;

	pIOFile	= IOFileCreate( pFile, O_WRONLY|O_TRUNC, 0, Malloc, Free );
	pW	= (IOReadWrite_t*)pIOFile;

	while( (pR->IO_Read)( pR, &Len, sizeof(Len)) > 0 ) {
DBG_PRT("Len=%d\n", Len );
		Ret = (pW->IO_Write)( pW, &Len, sizeof(Len) );	
		while( Len > 0 ) {
			n	= (pR->IO_Read)( pR, Buff, 
						(Len > sizeof(Buff) ? sizeof(Buff) : Len ) );
			Ret = (pW->IO_Write)( pW, Buff, n );	
			Len -= Ret;
		}
	}

	IOFileDestroy( pIOFile, FALSE );
	close( Fd );
	return( 0 );
}

int
ImportFromFile( struct Paxos *pPaxos, char *pFile )
{
	IOFile_t	*pIOFile;

	pIOFile	= IOFileCreate( pFile, O_RDONLY, 0, Malloc, Free );
	if( !pIOFile ) {
		printf("No file[%s].\n", pFile );
		exit( -1 );
	}
	PaxosUnmarshal( pPaxos, (IOReadWrite_t*)pIOFile );
	PaxosSnapshotValidate( pPaxos );

	IOFileDestroy( pIOFile, FALSE );
	return( 0 );
}

void
usage()
{
	printf("TestPaxos cell id {FALSE|TRUE [seq]}\n");
	printf("TestPaxos cell id abortseq seq\n");
	printf("TestPaxos cell id autonomic id\n");
	printf("TestPaxos cell id export file\n");
	printf("TestPaxos cell id import file\n");
	exit( -1 );
}

#define	SERVER_MAX		3
#define	SERVER_MAXIMUM	4
int
main( int ac, char* av[] )
{
	int	j;
	int	Ret;
	bool_t	bExport = FALSE, bImport = FALSE;
	short	Port;
	PaxosStartMode_t	StartMode = PAXOS_NO_SEQ;
	uint32_t	seq;
	struct in_addr	pin_addr;
//	int	AdminFd;

	if( ac < 3 ) {
		usage();
	}
	id	= atoi(av[2]);
	if( ac >= 4 ) {
		if( !strcmp( av[3], "TRUE" ) ) {
			if( ac >= 5 )	seq	= atoi(av[4]);
			else			seq = 0;
			StartMode	= PAXOS_SEQ;
		} else if( !strcmp( av[3], "FALSE" ) ) {
			StartMode	= PAXOS_WAIT;
		} else if( !strcmp( av[3], "abortseq" ) ) {
			if( ac < 5 )	usage();
			AbortSeq 	= atoi(av[4]);
		} else if( !strcmp( av[3], "autonomic" ) ) {
			if( ac < 5 )	usage();
			AutonomicId 	= atoi(av[4]);
		} else if( !strcmp( av[3], "export" ) ) {
			if( ac < 5 )	usage();
			bExport	= TRUE;
		} else if( !strcmp( av[3], "import" ) ) {
			if( ac < 5 )	usage();
			bImport	= TRUE;
		} else {
			usage();
		}
	}
	pPaxosCell	= PaxosGetCell( av[1] );
	for( j = 0; j < pPaxosCell->c_Maximum; j++ ) {
        pin_addr	= pPaxosCell->c_aPeerAddr[j].a_Addr.sin_addr;
        Port	= pPaxosCell->c_aPeerAddr[j].a_Addr.sin_port;
		Port	= ntohs( Port );

		printf("name[%s] id=%d ip[%s:%d]\n",
				pPaxosCell->c_aPeerAddr[j].a_Host, j, 
				inet_ntoa(pin_addr), Port );
	}
/*
 *	Export
 */
	if( bExport ) {
		ExportToFile( id, av[3] );
		return( 0 );
	}
/*
 *	Initialization & Recover
 */
	QueueInit( &WaitQueue );

	pPaxos = (struct Paxos*)Malloc( PaxosGetSize() );

	PaxosInitUsec( pPaxos, id,
		SERVER_MAX, SERVER_MAXIMUM-SERVER_MAX, pPaxosCell, 
		10*1000*1000, TRUE,
	 	Election, Notify, 
		OutboundMarshal, OutboundUnmarshal,
		NULL, NULL, NULL );

	PaxosLogCreate( pPaxos, 0/*1024*/, 0/*LOG_FLAG_BINARY*/, stderr, LogDBG );

	MyId	= PaxosMyId( pPaxos );

	if( AutonomicId >= 0 )	{
		GetAutonomic( pPaxos, AutonomicId );
	}
	if( bImport ) {
		ImportFromFile( pPaxos, av[3] );
	}
	LogDump( PaxosGetLog( pPaxos ) );
/*
 * 	Start
 */
	char	buff[128];
	Command_t*	pC;
	pthread_t	th;

	Ret = pthread_create( &th, NULL, ThreadExec, (void*)pPaxos );
	Ret = pthread_create( &th, NULL, ThreadWait, (void*)pPaxos );
	Ret = pthread_create( &th, NULL, ThreadAutonomic, (void*)pPaxos );
	if( PaxosStart( pPaxos, StartMode, seq ) < 0 ) {
		perror("PaxosStart error");
		exit( -1 );
	}

	printf("=>");
	while( gets( buff ) ) {
		pC	= (Command_t*)Malloc( sizeof(*pC) );
		dlInit( &pC->c_Head.p_Lnk );
		pC->c_Id	= MyId;
		strcpy( pC->c_Command, buff );

		Ret = PaxosRequest( pPaxos, sizeof(*pC), pC, FALSE );
		printf("===Ret[%d]===\n", Ret );
		if( Ret != 0 )	continue;

	}
	return( 0 );	
}

