/******************************************************************************
*
*  TestClient.c 	--- client test program of Paxos-Session Library
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

/**
 * 試験
 *	環境
 *	 PAXOS_SERVER_MAX=5
 *	 PAXOS_MAX=5
 *	 rmperm.sh	永続化データを削除
 *	 TestServer TestClient	テストプログラム
 *
 *	1.serverなし
 *		1)TestClient
 *		[結果]
 *		  停止する
 *	2.１つでも生きていれば再試行
 *		1)TestServer 0
 *		2)TestClient
 *		[結果]
 *		  永遠に再試行する
 *	3.再試行後、master選定直後実行
 *		1)TestServer 0
 *		2)TestClient
 *		3)TestServer 1,2
 *		[結果]
 *		  再試行
 *		  master選定直後実行
 *	4.master0が死ぬ(1は要求受理済み）
 *		1)TestServer 0,1,2,3
 *		2)TestClient
 *		3)すぐに0を殺す
 *		[結果]
 *		  0に要求
 *		  0切断検知後master探しで1に接続
 *		  1はmaster0を返す
 *		  ELECTION WAITで待つ
 *		  1はmaster1を通知
 *		  再要求で応答あり
 *	5.master0が死ぬ(1は未受理)
 *		1)TestServer 0,1,2,3
 *		2)admin 1 recvstop 14 1(SUCCESS)
 *		3)admin 1 catchupstop 1(CATCHUP要求)
 *		4)TestClient
 *		5)すぐに0を殺す
 *		[結果]
 *		  0に要求
 *		  0切断検知後master探しで1に接続
 *		  1はmaster0を返す
 *		  ELECTION WAITで待つ
 *		  1はmaster1を通知
 *		  再要求で応答あり
 *	6.master1が死ぬ(2は要求受理済み）
 *		1)TestServer 1,2,3,0
 *		2)TestClient
 *		3)すぐに1を殺す
 *		[結果]
 *		  1に要求
 *		  1切断検知後master探しで2に接続
 *		  2はmaster1を返す
 *		  ELECTION WAITで待つ
 *		  0あるいは2はmaster2を通知
 *		  再要求で応答あり
 *	7.master1が死ぬ(1は未受理)
 *		1)TestServer 1,2,3,0
 *		2)admin 2 recvstop 14 1(SUCCESS)
 *		3)admin 2 catchupstop 1(CATCHUP要求)
 *		4)TestClient
 *		5)すぐに2を殺す
 *		[結果]
 *		  1に要求
 *		  1切断検知後master探しで2に接続
 *		  2はmaster1を返す
 *		  ELECTION WAITで待つ
 *		  0あるいは2はmaster2を通知
 *		  再要求で応答あり
 */

//#define	_DEBUG_

#define	_SERVER_
//#define	_LOCK_

#include	"PaxosSession.h"

int
main( int ac, char* av[] )
{
	int	Ret;

	if( ac < 3 ) {
		printf("TestClient cell log_size [sleep]\n");
		exit( -1 );
	}
	struct Session*	pSession;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
									Send, Recv, Malloc, Free, av[1] );

	PaxosSessionLogCreate( pSession, atoi(av[2]), 0, stderr, LogDBG );

	Ret = PaxosSessionOpen( pSession, 0, TRUE );
	if( Ret ) {
		printf("Open ERROR\n");
		goto ret;
	}
	if( ac >= 4 ) {
		printf("sleep(%s)\n", av[3] );
		sleep( atoi(av[3]) );
	}
	Ret = PaxosSessionClose( pSession );
	printf("PaxosSessionClose:Ret=%d\n", Ret );
ret:
	LogDump( PaxosSessionGetLog( pSession ) );

	Free( pSession );
	return( 0 );
}

