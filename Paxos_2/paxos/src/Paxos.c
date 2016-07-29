/******************************************************************************
*
*  Paxos.c 	--- source of Paxos Library
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

/*!
 *	作成			渡辺典孝
 *	試験			塩尻常春、久保正業
 *	特許、著作権	トライテック
 *
 *
 *	Multi-Paxos概要
 *
 *	Collect->Last->Begin->Accept->Success(->Ack)の順である
 *	multi-paxosはページﾞ単位に管理される
 *	LeaderはCollectで現在ページﾞの決定、提案(最初は最弱)を
 *	自身の最強roundでAgentに通知する
 *	Agentは通知roundが古ければ拒否する　
 *	Agentは受理時には決定を受け入れ、保持する決定、提案
 *	を付与してLastを返す
 *	LeaderはLast受理時に決定を受け入れ、提案は最強roundを残す
 *	Last受理が過半であればroundを獲得したことになり、
 *	以降個々のpaxosを処理する
 *	Beginは提案を現在roundで強化しAgentに通知する
 *	Agentは保持する提案があり通知された提案が弱ければ拒否する
 *	受け入れればAcceptを返す
 *	Acceptが過半であれば決定であり以後覆ることはない
 *	事実上は、Agentが決定を知らなくとも、過半が受け入れていれば、
 *	決定は確定している　Acceptは決定の契機のためにある
 *	従って、新派が成立したときには、新派は過半数を獲得するので,
 *	旧派と1個以上のAgentを共有しの旧派の最強値を引き継ぐことになる
 *	Successは決定を通知し実行を促す
 *	Paxosインスタンスの実行はシーケンス順に行う
 *	本プログラムではcatchup機能を用意してあるのでAckを省いた
 *
 * 時刻
 *	 本システムは時刻監視によるオートマトンである
 *	DT_Z	heart beat間隔
 *	DT_C	状態監視間隔
 *	DT_D	通信遅れ(輻輳を含む)
 *	DT_L	実行時間
 *
 *	通信切断	DT_D+DT_Z+DT_D+DT_Z=2*(DT_D+DT_Z)
 *				トークンはDT_D後に届き最大DT_Z内に返信されDT_D後に着信
 *				さらに、最大DT_Z内で接続切断の判断でマスター選択有無
 *	Cache保持	2*(DT_D+DT_Z)+DT_D+DT_D=4*DT_D+2*DT_Z
 *				通信切断/接続の認識後のCatchup要求
 *				ただし、ロックのためCatchup要求を出せない場合がある
 *	leader選択	3*(DT_Z+DT_D)
 *				DT_Z+DT_D以内に候補確定、最大DT_Z内に送信され
 *				DT_D後にleader確定
 *				ただし、接続を考慮すると、2*(DT_D+DT_Z)で候補確定となる。
 *	Round獲得	2*DT_D+2*N*DT_L+DT_L
 *				collect編集にN*DT_L送信遅延にDT_D
 *				last編集にDT_L送信遅延にDT_D
 *				gatherlastに最大N*DT_L
 *	Success		3*DT_L+2*N*DT_L+2*DT_D
 *				begin編集にN*DT_L送信遅延にDT_D
 *				begin実行にDT_L、accept編集にDT_L送信遅延にDT_D
 *				gatheracceptに最大N*DT_L
 *				success実行にDT_L
 *	Decide		leader選択+round獲得+決定+通知
 *					(3*DT_Z+10*DT_L+7*N*DT_L+10*DT_D)
 *				leader選択に3*(DT_Z+DT_D)
 *				roundの獲得に2*Success＋DT_L
 *					1回目(Old)	3*DT_L+2*N*DT_L+2*DT_D
 *					newround	DT_L
 *					2回目		3*DT_L+2*N*DT_L+2*DT_D
 *				決定にSuccess
 *				通知編集にN*DT_L送信遅延にDT_D通知実行にDT_L
 *
 *
 * 通信と双方向回路の確認及びmaster選択とcatchup
 *	 通信は送受信関数を初期化時に指定する　
 *	 heart beatを行うのでUDPで可である
 *	全パケットには相手受信時刻と送信時刻を乗せ、heart beat受信時に
 *	双方向仮想回路の確立を確認する
 *	 また、heart beatにはmaster選択情報、catchup情報を乗せる
 *
 * leader選択
 *	 サーバには、立ち上がり、通信回復時刻を登録し、
 *	最長老のサーバがheart beatを介してleaderとして選ばれる
 *	 heart beatでは、立候補時刻、選択したマスターを通知する
 *	複数マスターが混在することがあるが、過半の支持を受けたマスターに
 *	いずれ収束する。
 *	 heart beat受信がではleader候補を選択し通知する
 *	 heart beat受信で同一leader候補が過半であれば合意したleaderである
 *	 即ち、各サーバはleaderを自律的に判定する
 *	 なお、本leader選択はPaxos調停とは独立している
 *	「特願2010-127357／特許4981952」
 *
 *	なお、Ras機能で各サーバに障害を通知できれば、瞬時にleader交替が行われる
 *
 *	catchup
 *	 過半数で合意を進めるので、少数派が遅れる場合がある
 *	このためにcatchupが必要となる
 *	 catchupは決定済みインスタンス通番を比較する
 *	決定済みインスタンス通番以前のインスタンスは全て決定済みである
 *	heart beatで通知された決定済み通番が自分より新しければcatchup要求を行う
 *	応答受信時、処理中であっても決定済みとする
 *	なお、catchup要求中は処理を抑止し、終了後に解除する
 *	また、catchupは任意のペアが自律的に行う
 *	「特願2010-127356/特許4981951」
 *
 *	ページ管理とメタデータ
 *	 PAXOS_MAX個のインスタンスをページとし（クライアント）多重化
 *	 メタ値とメタコマンドによる効率化及びredirect
 *	 メタコマンドの昇順実行管理
 *	「特願2010-173944/特許4916567/US8,745,126B2」
 *
 *	メタ値
 *	 メタ値は、Collect,Lastで交換される
 *	 これによりPAX_MAX個の交換を実現
 *
 *	メタコマンド
 *	 Begin、Sucess及びcatchup応答で通知される
 *
 *	メタコマンド実行
 *	 実行インスタンス通番の順序管理を行う
 *	期待通番より小であれば実行済みなので捨てられ、大であれば保留される
 *	期待通番であればNotify()を呼び出し、保留に次があれば連続実行する
 *	Notify()ではPaxosを排他しているので速やかに実行を終了する
 *
 *	アウトバンドデータ(Validate機能)
 *	 Paxos調停はメタコマンドを扱うが対応する実コマンドは以下とする
 *	PaxosRequest時およびBegin、Accept、Sucsess、catchup受付時、
 *	NotifyでPAXOS_VALIDATEを発行する エラーであればPaxos調停は進行しない
 *	これにより、メタコマンドに対応する実コマンドをPaxos調停に取り込む
 *	「特願2010-023612/特許5123961/US8,775,500B2」
 *
 *	ログの高速化
 *	 高速化のためにコマンドログは永続化をしない
 *
 *	コアとエクステンション
 *	 コア(PAXOS_SERVER_MAX)はPaxos合意に参加する
 *	エクステンション(PAXOS_SERVER_MAXIMUM-PAXOS_SERVER_MAX)はCatchupによる
 *	エクテンションもレプリケーションであり参照の分散用を目的とする
 *	Catchupはハートビートによるので、ハートビート時間の遅延が発生する
 *	このためキャッシュはハートビート依存の時間保持される必要がある
 *	[考察]
 *	 エクステンションをコアに負担分散することが考えられる
 *	しかし、付託されたコアが不能である時他が担うのは難しい
 *	他が付託されたコアの不能を知る時と実際に不能となる時とはズレがある
 *
 *	IPアドレスの動的変更
 *	 動的変更したいサーバを停止する
 *	他のサーバの該当アドレスを変更する
 *	当該サーバを起動する
 *	
 */
#define __USE_POSIX199309  //for centos6.2
//#define __need_timespec 
//#define	_DEBUG_
//#define	_PAXOS_
#define	_PAXOS_SERVER_
//#define	_LOCK_
#include	"Paxos.h"

PaxosMulti_t* GetPage( Paxos_t *pPaxos, int page );
int PutPage( Paxos_t *pPaxos );
int UpdatePage( Paxos_t* pPaxos );

int CatchupRequest( Paxos_t *pPaxos, Server_t* pserver, uint32_t nextDecide );
int CatchupFire( Paxos_t* pPaxos );
int CatchupInstanceRpl( Paxos_t *pPaxos, Server_t* pserver, 
								PaxosMulti_t* pMulti, uint32_t seq );
bool_t Exec( Paxos_t* pPaxos, int k );
int SendSuccess( Paxos_t* pPaxos, int k );
bool_t BeginCast( Paxos_t* pPaxos, int k );

bool_t SendAlive( Paxos_t* pPaxos, Server_t* pserver );
int ChSendTo( ChSend_t* pC, void* p );

#ifdef	ZZZ
#else
static	int release_suppressed_accept( Paxos_t *pPaxos );
#endif

int
PaxosSend( Paxos_t *pPaxos, Server_t *pserver, msg_t *pm )
{
	if( !(pserver->s_status & PAXOS_SERVER_ACTIVE) )	return( -1 );	
	pm->m_cksum64	= 0;
	if( pPaxos->p_bCksum ) {
		memset( &pm->m_dlnk, 0, sizeof(pm->m_dlnk) );
		pm->m_cksum64	= in_cksum64( pm, pm->m_len, 0 );
	}
	ChSend( &pserver->s_ChSend, pm, m_dlnk );
	return( 0 );
}

int
PaxosLogCreate( Paxos_t* pPaxos, int Size, int Flags, FILE* pFile, int Level )
{
	ASSERT( pPaxos->p_pLog == NULL );

	pPaxos->p_pLog	= LogCreate( Size, pPaxos->p_fMalloc, pPaxos->p_fFree, 
									Flags, pFile, Level );
	if( pPaxos->p_pLog )	return( 0 );
	else					return( -1 );
}

void
PaxosLogDestroy( Paxos_t* pPaxos )
{
	if( pPaxos->p_pLog ) {
		LogDestroy( pPaxos->p_pLog );
		pPaxos->p_pLog	= NULL;
	}
}

struct Log*
PaxosGetLog( Paxos_t* pPaxos )
{
	return( pPaxos->p_pLog );
}

int
PaxosSetLog( Paxos_t* pPaxos, struct Log* pLog )
{
	pPaxos->p_pLog	= pLog;
	return( 0 );
}

int
PaxosGetMax( Paxos_t* pPaxos )
{
	return( pPaxos->p_Max );
}

int
PaxosGetMaximum( Paxos_t* pPaxos )
{
	return( pPaxos->p_Maximum );
}

/*
 *	master選択セクション
 *		Leader Elector
 */
bool_t
ElectLeader( Paxos_t* pPaxos, Server_t* pServer )
{
	Server_t*	pserver;
	int	votes[pPaxos->p_Max];
	int		i, j;
	bool_t	fElected = FALSE;

	DBG_ENT();

//
// I am elected as master ?
//
	memset( votes, 0, sizeof(votes) );
	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
		if( j == MyId ) {
				i = ELeader;
			if( i >= 0 )	votes[i]++;
		} else {
			pserver	= &pPaxos->p_aServer[j];

			if( !(pserver->s_status & PAXOS_SERVER_CONNECTED ) )
				continue;

			i = pserver->s_leader;
			if( i >= 0 )	votes[i]++;
		}
	}
	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {

		if( votes[j] >= pPaxos->p_Max/*PAXOS_SERVER_MAX*//2+1 ) {
			//
			//	elected
			//
	LOG( pPaxos->p_pLog, LogINF, "Elected=%d vote=%d", j, votes[j] );

			fElected	= TRUE;
			Elected		= j;
			break;
		}
	}
	if( fElected ) {
		if(  Elected == pPaxos->p_MyId  ) {
			/* masterであり続ければ世代を更新しない */
			if( IamMaster == FALSE ) {
				SEQ_INC( Epoch );
				IamMaster	= TRUE;
				NewRndStart	= TRUE;
				(pPaxos->p_fElection)( pPaxos, 
					PAXOS_Im_MASTER, MyId, Epoch, pServer->s_ID );
			}
		}  else {
			if( IamMaster == TRUE ) {
				IamMaster	= FALSE;
				NewRndStart	= FALSE;
				(pPaxos->p_fElection)( pPaxos, 
					PAXOS_Im_NOT_MASTER, Elected, Epoch, pServer->s_ID );
			}
		}
		(pPaxos->p_fElection)( pPaxos, 
					PAXOS_MASTER_ELECTED, Elected, Epoch, pServer->s_ID );
	} else  {
		if( Elected >= 0 ) {
			(pPaxos->p_fElection)( pPaxos, 
					PAXOS_MASTER_CANCELED, Elected, Epoch, pServer->s_ID );
		}
		Elected	= -1;

		if( IamMaster == TRUE ) {
		/* masterを循環させたければ */
			TIMESPEC( EBirth );

	LOG( pPaxos->p_pLog, LogINF, "EBirth UPDATED EBirth=%u", EBirth );

			IamMaster	= FALSE;
			NewRndStart	= FALSE;
			(pPaxos->p_fElection)( pPaxos, 
					PAXOS_Im_NOT_MASTER, Elected, Epoch, pServer->s_ID );
		}
	}
	RETURN( fElected );
}

bool_t
MyVote( Paxos_t* pPaxos, Server_t* pServer )
{
	Server_t*	pserver;
	int32_t		leader = -1;
	timespec_t	tElection = {0,};
	int	i, majority;
	bool_t		Changed = FALSE;

	DBG_ENT();

	LOG( pPaxos->p_pLog, LogINF, "From=%d s_leader=%d", 
						pServer->s_ID, pServer->s_leader );
//
// my vote to leader on listening to majority
//
	majority = 0;
	for( i = 0; i < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; i++ ) {
		if( i == MyId ) {
			majority++;
			if( leader < 0 || TIMESPECCOMPARE(EBirth,tElection) < 0 ) {
				leader		= MyId;
				tElection	= EBirth;
			}
		} else {
			pserver	= &pPaxos->p_aServer[i];

			if( (pserver->s_status & PAXOS_SERVER_CONNECTED ) ) {

				majority++;

				if( leader < 0 
				|| TIMESPECCOMPARE(pserver->s_Election,tElection) < 0 ) {
					leader		= pserver->s_ID;
					tElection	= pserver->s_Election;
				}
			}
		}
	}
/*
 *	リーダ候補はmajorityでなくもよい？
 *		ただし、少数派による要求を阻止できる
 */
	if( majority < pPaxos->p_Max/*PAXOS_SERVER_MAX*//2 + 1 )	leader	= -1;

	LOG( pPaxos->p_pLog, LogINF, "majority=%d ELeader=%d leader=%d", 
							majority, ELeader, leader );

	if( ELeader == leader ) {
		Changed = FALSE;
	} else {
		ELeader	= leader;
		Changed = TRUE;
	}
	// FYI
	pPaxos->p_aServer[MyId].s_leader	= ELeader;
	pPaxos->p_aServer[MyId].s_epoch		= Epoch;
	pPaxos->p_aServer[MyId].s_Election	= EBirth;
	RETURN( Changed );
}

void
set_leader( Paxos_t* pPaxos, Server_t* pServer )
{
	bool_t		Changed;
	Server_t*	pserver;
	int	j;

	DBG_ENT();

	if( pServer->s_ID >= pPaxos->p_Max/*PAXOS_SERVER_MAX*/ )	goto end;
 
	Changed = MyVote( pPaxos, pServer );
	if( Changed ) {
		for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
			if( j == MyId )	continue;
			pserver = &pPaxos->p_aServer[j];
			SendAlive( pPaxos, pserver );
		}
	}
	ElectLeader( pPaxos, pServer );

end:
	DBG_EXT(0);
}

bool_t
SendAlive( Paxos_t* pPaxos, Server_t* pserver )
{
	msg_alive_t*	pm;


	DBG_ENT();

	pm	= (msg_alive_t*)msg_alloc( pPaxos,
								pserver->s_ID, MSG_ALIVE, sizeof(*pm) );

	pm->a_head.m_From	= pPaxos->p_aServer[pPaxos->p_MyId].s_ChSend.c_PeerAddr;

	pm->a_fr		= pPaxos->p_MyId;
	pm->a_to		= pserver->s_ID;
#ifdef	ZZZ
#else
/***
	if( pPaxos->p_bSuppressAccept ) {
		TIMESPEC( EBirth );
		LOG( pPaxos->p_pLog, "SuppressAccept:REBORN" );
	}
***/
#endif
	pm->a_Election	= EBirth;
	pm->a_leader	= ELeader;
	pm->a_epoch		= Epoch;
	TIMESPEC( pm->a_MyTime );
	pm->a_YourTime	= pserver->s_YourTime;
	if( pPaxos->p_fLoad ) {
		pm->a_Load	= (pPaxos->p_fLoad)(pPaxos->p_pTag);
	} else {
		pm->a_Load	= -1;
	}

	/* atomic */
	pm->a_Catchup		= !(pPaxos->p_CatchupStop);
	pm->a_BackPage		= BackPage;
	pm->a_NextDecide	= NextDecide;

	DBG_TRC("to=%d fr=%d\n", pm->a_to, pm->a_fr );

	PaxosSend( pPaxos, pserver, (msg_t*)pm );

	DBG_EXT( TRUE );
	return( TRUE );
}

/*
 *	Leader
 */

/*
 * Round獲得は、リーダとなった後、NewRndStartで１回だけ行う。
 * NewRndStartはGatherLastでFALSEとされる。
 * また、リーダを降りた場合にもFALSEとされる。
 * isMyRndがTRUEであればインスタンスを投入できる。
 * FALSEの場合は新リーダがラウンドを獲得していることになる。
 *
 *	Init->NewRound->Wait->Begincast
 *	Init->Continue->BeginCast
 */
bool_t
Continue( Paxos_t* pPaxos, int k )
{
DBG_TRC("Page=%d k=%d\n", Page, k );

	if( !isMyRnd || PMode( k ) != MODE_NONE ) {
		return( FALSE );
	}

	ASSERT( !isEmpty( &MetaCmdInit(k) ) );

	RndValue( k ) 	= InitValue( k );
	RndValue( k ).v_Epoch	= Epoch;
	MetaCmdRnd( k )	= MetaCmdInit( k );

	PMode( k )		= MODE_BEGINCAST;

	return( TRUE );
}

bool_t
NewRound( Paxos_t* pPaxos )
{
	Server_t*		pserver;
	int	j, k;
	msg_collect_t*	pm;

	DBG_ENT();


	if( AMode != MODE_NONE ) {

	LOG( pPaxos->p_pLog, LogINF, "FALSE AMode=%d", AMode );

		RETURN( FALSE );
	}

	INC_ROUND( &CurRnd );
	CurRnd.r_id		= MyId;

	for( k = 0; k < PAXOS_MAX; k++ ) {
		/*
		 *	初期提案を最弱で設定
		 */
		if( PMode( k ) == MODE_NONE
				&& isDefined( &InitValue( k ) ) ) {

			PMode( k )	= MODE_WAIT;

			ASSERT( !isEmpty( &MetaCmdInit(k) ) );

			RndValue( k )			= InitValue( k );
			RndValue( k ).v_Epoch	= Epoch;
			MetaCmdRnd( k )			= MetaCmdInit( k );
			RndBallot( k ).r_id		= MyId;
			RndBallot( k ).r_seq	= 0;	// 最弱
		}
	}
	AMode	= MODE_GATHERLAST;	
	NewRoundLast	= time(0) + DT_NEWROUND;
	quorum_init( &RndInfQuo, pPaxos->p_Max/*PAXOS_SERVER_MAX*/ );

	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {

		if( j == MyId ) { // 自分には送信しない Ballotはいじらない
			quorum_add( &RndInfQuo, MyId );
if( pPaxos->p_Max/*PAXOS_SERVER_MAX*/ == 1 ) {
	AMode = MODE_BEGINCAST;
}
			continue;
		}
		pserver	= &pPaxos->p_aServer[j];
		pm	= (msg_collect_t*)msg_alloc( pPaxos,
								pserver->s_ID, MSG_COLLECT, sizeof(*pm) );
		pm->c_BackPage		= BackPage;
		pm->c_NextDecide	= NextDecide;
		pm->c_Page			= Page;
		pm->c_NextK			= NextK;

		for( k = 0; k < PAXOS_MAX; k++ ) {
			pm->c_paxos[k].m_mode	= 0;

			if( PMode( k ) == MODE_DECIDED ) {

				pm->c_paxos[k].m_mode = MODE_MSG_DECIDED;
				pm->c_paxos[k].m_value	= RndValue( k );

			} else if( PMode( k ) != MODE_NONE ) {

				pm->c_paxos[k].m_mode = MODE_MSG_PROPOSED;
				pm->c_paxos[k].m_value	= RndValue( k );
				pm->c_paxos[k].m_round	= RndBallot( k );
			}
		}
		pm->c_CurRnd	= CurRnd;

		PaxosSend( pPaxos, pserver, (msg_t*)pm );
	}
	DBG_EXT( TRUE );
	return( TRUE );
}

/*
 * Leader　Delayは要求ページの決定が返る
 * Agent Delay はround獲得のみ
 */
bool_t
GatherLast( Paxos_t* pPaxos, msg_last_t* pml )
{
	int	j;
	int	k;
	Server_t*	pserver;
	bool_t	Ret = FALSE;

	DBG_ENT();

	LOG( pPaxos->p_pLog,LogINF,  
		"AMode=%d fr[%d] l_round[%d %u] CurRnd[%d %u] RndInfQuo=0x%x",
		AMode, pml->l_head.m_fr, pml->l_round.r_id, pml->l_round.r_seq,
		 CurRnd.r_id, CurRnd.r_seq, RndInfQuo.q_member );

	if( AMode != MODE_GATHERLAST )	return( FALSE );

	if( CMP_ROUND( &pml->l_round, &CurRnd ) )	return( FALSE );

	j	= pml->l_head.m_fr;
	pserver	= &pPaxos->p_aServer[j];

	quorum_add( &RndInfQuo, j );

if( CMP( pml->l_Page, Page ) == 0 ) {
	for( k = 0; k < PAXOS_MAX; k++ ) {

		if( pml->l_paxos[k].m_mode == MODE_MSG_DECIDED
			&& PMode( k ) != MODE_DECIDED ) {

			/* agentは既に送っている筈
				受け取っていない場合があった（ロスト？）
				欠落している場合は決定コマンドを要求 */
	LOG( pPaxos->p_pLog,LogINF,  
	"DECIDED  REQUEST j=%d Seq=%u PMode(k)=%d l_NextDecide=%u", 
	j, Seq( k ), PMode( k ), pml->l_NextDecide );
			CatchupRequest( pPaxos, pserver, pml->l_NextDecide );
		}
		if( pml->l_paxos[k].m_mode == MODE_MSG_PROPOSED
			&& PMode( k ) != MODE_DECIDED ) {

	LOG( pPaxos->p_pLog, LogINF,
	"PROPOSED j=%d k=%d RndBallot[%d %u] m_round[%d %u] m_value[0x%x %d %d]", 
	j, k, RndBallot(k).r_id, RndBallot(k).r_seq,
	pml->l_paxos[k].m_round.r_id, pml->l_paxos[k].m_round.r_seq,
	pml->l_paxos[k].m_value.v_Flags, pml->l_paxos[k].m_value.v_Server,
	pml->l_paxos[k].m_value.v_From );

			/* 最強を保持 */
			if( CMP_ROUND( &RndBallot( k ), &pml->l_paxos[k].m_round ) <= 0 
					&& isDefined(&pml->l_paxos[k].m_value) ) {
				if( PMode( k ) == MODE_NONE 
					|| (RndValue( k ).v_Server 
						!= pml->l_paxos[k].m_value.v_Server) ) {

					RndValue( k ) 	= pml->l_paxos[k].m_value;
				}
				RndBallot( k ) 	= pml->l_paxos[k].m_round;

				PMode( k ) 	= MODE_WAIT;
			}
		}
	}
}
	if( quorum_majority( &RndInfQuo ) ) {

		NewRndStart = FALSE;

		for( k = 0; k < PAXOS_MAX; k++ ) {
			if( PMode( k ) != MODE_NONE ) {
				NextK = k + 1;
				if( PMode( k ) != MODE_DECIDED ) {
					PMode( k )	= MODE_BEGINCAST;
				}
			}
		}
		AMode	= MODE_BEGINCAST;

	LOG( pPaxos->p_pLog, LogINF, "GET NEW ROUND RndInfQuo=0x%x", 
					RndInfQuo.q_member );
	/*
	 *	Collecｔ->Lastで一回だけloopするのでcost的に問題なし
	 */
		for( k = 0; k < PAXOS_MAX; k++ ) {
			BeginCast( pPaxos, k );
		}
		// newroundを待っている人のため
		CndBroadcast( &pPaxos->p_Cnd );
	}

	DBG_EXT( Ret );
	return( Ret );
}

int
SendBeginCast( Paxos_t* pPaxos, int k, int To, Round_t* pCurRnd )
{
	int	j;
	msg_begin_t*	pm;
	Server_t*	pserver;

ASSERT( !isEmpty( &MetaCmdRnd(k) ) );

	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {

		/* 抑止されていなければ自分には送らない */
		if( MyId == To ) { // REDIRECTではない
			if( j == MyId ) {
#ifdef	ZZZ
				if( IsTimespecZero( pPaxos->p_CatchupRecvLast ) ) {
#else
				if( !pPaxos->p_bPageSuppress
					&& IsTimespecZero( pPaxos->p_CatchupRecvLast ) ) {
#endif
					RndBallot( k )	= *pCurRnd;
					quorum_add( &RndAccQuo( k ), MyId );
					continue;
				}
				else {
	LOG( pPaxos->p_pLog, LogINF, "SEND TO ME j=%d Seq(k)=%u k=%d", 
					j, Seq(k), k );
				}
			}
		}
		pserver	= &pPaxos->p_aServer[j];
		pm = (msg_begin_t*)msg_alloc( pPaxos, pserver->s_ID,
										MSG_BEGIN, sizeof(*pm) );
		pm->b_To			= To;
		pm->b_BackPage		= BackPage;
		pm->b_NextDecide	= NextDecide;
		pm->b_Page			= Page;
		pm->b_NextK			= NextK;
		pm->b_k				= k;
		pm->b_CurRnd		= *pCurRnd;
		pm->b_Value			= RndValue( k );
		pm->b_Value.v_From	= MyId;
		pm->b_MetaCmd		= MetaCmdRnd( k );

		PaxosSend( pPaxos, pserver, (msg_t*)pm );
	}
	return( 0 );
}

bool_t
BeginCast( Paxos_t* pPaxos, int k )
{
	int	j;
	msg_begin_t*	pm;
	Server_t*		pserver;

	if( PMode( k ) != MODE_BEGINCAST )	return( FALSE );

	if( (j = RndValue( k ).v_From) != MyId ) {

	LOG( pPaxos->p_pLog, LogINF,
	"REDIRECT Sever=%d From=%d Page=%d k=%d", 
	RndValue(k).v_Server, j, Page, k );

			pserver	= &pPaxos->p_aServer[j];
			pm = (msg_begin_t*)msg_alloc( pPaxos, pserver->s_ID,
										MSG_BEGIN_REDIRECT, sizeof(*pm) );
			pm->b_To		= MyId;
			pm->b_Page		= Page;
			pm->b_k			= k;
			pm->b_CurRnd	= CurRnd;
			pm->b_Value		= RndValue( k );

			PaxosSend( pPaxos, pserver, (msg_t*)pm );
	} else {

		SendBeginCast( pPaxos, k, MyId, &CurRnd );
	}
	PMode( k )	= MODE_GATHERACCEPT;

//	starter
	TIMESPEC( Deadline(k) );
	TimespecAddUsec( Deadline(k), DT_SUCCESS );

if( pPaxos->p_Max/*PAXOS_SERVER_MAX*/ == 1 ) {
		PMode( k )		= MODE_DECIDED;
		RndValue( k ).v_Flags	|= VALUE_NORMAL;
		Exec( pPaxos, k );
}

	return( TRUE );
}

bool_t
BeginCastRedirect( Paxos_t* pPaxos, msg_begin_t* pb )
{
	int	k;
	int		Ret;

	DBG_ENT();

	k = pb->b_k;

	LOG( pPaxos->p_pLog, LogINF,
	"CASTREDIRECT Page=%d b_Page=%d k=%d fr=%d To=%d", 
	Page, pb->b_Page, k, pb->b_head.m_fr, pb->b_To );

	ASSERT( Page == pb->b_Page );

	/* OutOfBand Data Check */
	Ret = PaxosValidate( pPaxos, PAXOS_MODE_REDIRECT, 
					Seq( k ), &RndValue( k ), &MetaCmdRnd( k ) );
	ASSERT( !Ret );

	SendBeginCast( pPaxos, k, pb->b_To, &pb->b_CurRnd );

	RETURN( TRUE );
}

bool_t
GatherAccept( Paxos_t* pPaxos, msg_accept_t* pm )
{
	int	j, k;

	DBG_ENT();

DBG_TRC("Page=%d a_Page=%d k=%d j=%d\n", 
Page, pm->a_Page, pm->a_k, pm->a_head.m_fr );

	k	= pm->a_k;
	if( CMP_ROUND( &CurRnd, &pm->a_round ) 
		|| CMP( Page, pm->a_Page ) != 0 
		|| PMode( k ) != MODE_GATHERACCEPT ) {

DBG_TRC("PMode=%d Page=%u a_Page=%u k=%d j=%d\n", 
PMode(k), Page, pm->a_Page, pm->a_k, pm->a_head.m_fr );

			DBG_EXT( FALSE );
			return( FALSE );
	}

	/* leaderにbeginが到達しない場合がある Redirect ? */
	if( isEmpty( &MetaCmdRnd(k) ) ) {
//DBG_PRT("MetaCmdRnd empty Page=%d k=%d v_Server=%d v_From=%d\n",
//	Page, k, RndValue(k).v_Server, RndValue(k).v_From );
	LOG( pPaxos->p_pLog, LogINF,
			"MetaCmdRnd empty Page=%d k=%d", Page, k );
		if( PaxosValidate( pPaxos, PAXOS_MODE_ACCEPT, 
				Seq( k ), &pm->a_Value, &pm->a_MetaCmd ) ) {

			DBG_EXT( FALSE );
			return( FALSE );
		}
		MetaCmdRnd( k )	= pm->a_MetaCmd;
		RndValue( k ).v_From	= MyId;
	}

	j	= pm->a_head.m_fr;

	quorum_add( &RndAccQuo( k ), j );

	/* リーダが含まれる */
	if( quorum_exist( &RndAccQuo( k ), MyId )
			&& quorum_majority( &RndAccQuo( k ) ) ) {
DBG_TRC("DECIDED total=%d count=%d member=0x%x\n", 
RndAccQuo(k).q_total, RndAccQuo(k).q_count, RndAccQuo(k).q_member );

		PMode( k )		= MODE_DECIDED;
ASSERT( RndValue(k).v_From == MyId );

		SendSuccess( pPaxos, k );

		RndValue( k ).v_Flags	|= VALUE_NORMAL;
		Exec( pPaxos, k );
	}

	DBG_EXT( TRUE );
	return( TRUE );
}

bool_t
GatherOldRound( Paxos_t* pPaxos, msg_oldround_t* pm )
{
	if( CMP_ROUND( &CurRnd, &pm->o_commit ) >= 0 )	return( FALSE );

	CurRnd	= pm->o_commit;

	LOG( pPaxos->p_pLog, LogINF, "#### OLD ROUND ###");
	AMode	= MODE_NONE;

	return( TRUE );
}

/*
 *	Agent
 */
bool_t
RecvCollect( Paxos_t* pPaxos, msg_collect_t *pmc )
{
	int	j, k;
	Server_t*	pserver;
	Round_t		r;
	msg_last_t*		pml;
	msg_oldround_t*	pmo;
	PaxosMulti_t*	pMulti;

	DBG_ENT();

	j	= pmc->c_head.m_fr;
	pserver	= &pPaxos->p_aServer[j];
	r	= pmc->c_CurRnd;

	if( CMP_ROUND( &CurRnd, &r ) <= 0 ) {

		CurRnd	= r;

		pml	= (msg_last_t*)msg_alloc( pPaxos,
								pserver->s_ID, MSG_LAST, sizeof(*pml) );

		pml->l_round			= r;
		pml->l_NextDecide		= NextDecide;
		pml->l_Page				= Page;

		if( CMP( pmc->c_Page, Page ) < 0 ) { // leader delay

			pMulti	= GetPage( pPaxos, pmc->c_Page );

	LOG( pPaxos->p_pLog, LogINF, "LEADER DELAY Page=%d c_Page=%d pMulti=%p", 
	Page, pmc->c_Page, pMulti );
			/*
			 *	要求pageを返す
			 */
			if( !pMulti ) {
				//DBG_PRT("LEADER DELAY fr=%d c_Page=%u Page=%u BackPage=%u\n",
				//			j, pmc->c_Page, Page, BackPage );				
				return( FALSE );
			}
			ASSERT( pMulti );

			pml->l_Page				= pMulti->m_PageHead.p_Page;

			for( k = 0; k < PAXOS_MAX; k++ ) {

				ASSERT( pMulti->m_aPaxos[k].p_Mode == MODE_DECIDED );

				pml->l_paxos[k].m_mode	= MODE_MSG_DECIDED;
				pml->l_paxos[k].m_value = pMulti->m_aPaxos[k].p_RndValue;

				/* 決定コマンドを送信 */
	LOG( pPaxos->p_pLog, LogDBG,
	"SEND DECIDED OLD Page=%d k=%d", pmc->c_Page, k );
				CatchupInstanceRpl( pPaxos, pserver, 
							pMulti, TO_SEQ(pMulti->m_PageHead.p_Page, k) );
			}
		} else if( CMP( pmc->c_Page, Page ) > 0 ) {	// agent delay

	LOG( pPaxos->p_pLog, LogDBG,
					"AGENT DELAY j=%d Page=%u c_Page=%u, c_BackPage=%u", 
					j, Page, pmc->c_Page, pmc->c_BackPage );
			if( CMP( pmc->c_BackPage, Page ) <= 0 ) {
				pPaxos->p_IssuedLast	= 0; // catchup　の強制
				CatchupRequest( pPaxos, pserver, pmc->c_NextDecide );
			}
		}
		else {	// Page == c_Page

			for( k = 0; k < PAXOS_MAX; k++ ) {

				if( pmc->c_paxos[k].m_mode	== MODE_MSG_DECIDED ) {
					if( !isDecided( k ) ) {
						/* 決定コマンドを要求 */
						CatchupRequest( pPaxos, pserver, pmc->c_NextDecide );
					}
				} else if( PMode( k ) == MODE_DECIDED ) {

					ASSERT( !isEmpty( &MetaCmdRnd(k) ) );

					pml->l_paxos[k].m_mode	= MODE_MSG_DECIDED;
					pml->l_paxos[k].m_value = RndValue( k );

					/* 決定コマンドを送信 */
	LOG( pPaxos->p_pLog, LogDBG, "SEND DECIDED Page=%d k=%d", Page, k );
					CatchupInstanceRpl( pPaxos, pserver, 
							&Multi, TO_SEQ(Multi.m_PageHead.p_Page, k) );
				} else {
					pml->l_paxos[k] 	= pmc->c_paxos[k];

					/* 受け取った提案がある */
					/* MODE_DECIDED ははじかれている */
					/* COLLECTなのでRndはいじらない */
					if( PMode( k ) != MODE_NONE ) {
						if( pmc->c_paxos[k].m_mode == MODE_MSG_NONE
							|| ( pmc->c_paxos[k].m_mode == MODE_MSG_PROPOSED
								&& CMP_ROUND(&pmc->c_paxos[k].m_round,
								&RndBallot(k) ) <= 0 ) ) {	
	LOG( pPaxos->p_pLog, LogDBG,
	"I HAVE Seq=%u k=%d RndBallot[%d %u] RndValue[0x%x %d %d]",
	Seq( k ), k, RndBallot(k).r_id, RndBallot(k).r_seq, 
	RndValue(k).v_Flags, RndValue(k).v_Server, RndValue(k).v_From );

							pml->l_paxos[k].m_mode	= MODE_MSG_PROPOSED;
							pml->l_paxos[k].m_round	= RndBallot( k );
							pml->l_paxos[k].m_value	= RndValue( k );
							if( !isEmpty( &MetaCmdRnd(k) ) ) {
								pml->l_paxos[k].m_value.v_From	= MyId;
							} 
							else {
	LOG( pPaxos->p_pLog, LogDBG,
	"I HAVE NOT MetaCmd Page=%d k=%d RndValue.v_From=%d",
	Page, k, RndValue(k).v_From );
							}
						}
					}
				}
			}
		}

		PaxosSend( pPaxos, pserver, (msg_t*)pml );
	} else {
		pmo	= (msg_oldround_t*)msg_alloc( pPaxos,
							pserver->s_ID, MSG_OLDROUND, sizeof(*pmo) );

		pmo->o_round	= r;
		pmo->o_commit	= CurRnd;

		PaxosSend( pPaxos, pserver, (msg_t*)pmo );
	}
	DBG_EXT( TRUE );
	return( TRUE );
}

bool_t
RecvBegin( Paxos_t* pPaxos, msg_begin_t *pmb )
{
	int			k;
	Server_t*	pserver;
	Server_t*	pserverAccept;
	msg_accept_t*	pma;
	msg_oldround_t*	pmo;

	DBG_ENT();

	if( pPaxos->p_bSuppressAccept )	RETURN( FALSE );
	pserver			= &pPaxos->p_aServer[pmb->b_head.m_fr];
	pserverAccept	= &pPaxos->p_aServer[pmb->b_To];	// ACCEPTを返すサーバ

	k		= pmb->b_k;

	if( CMP_ROUND( &pmb->b_CurRnd, &CurRnd ) >= 0 ) {

		if( CMP( Page, pmb->b_Page ) < 0 ) {

			DBG_TRC("Page=%d b_Page=%d\n", Page, pmb->b_Page );
			if( CMP( pmb->b_BackPage, Page ) <= 0 ) {
				CatchupRequest( pPaxos, pserver, pmb->b_NextDecide );
			}
			RETURN( FALSE );

		} else if( CMP( Page, pmb->b_Page ) > 0 ) {
DBG_TRC("Page=%d b_Page=%d\n", Page, pmb->b_Page );
			RETURN( FALSE );
		}
		if( PMode( k ) == MODE_DECIDED ) {
			RETURN( FALSE );
		}
		ASSERT( !isEmpty( &pmb->b_MetaCmd ) );

		/* OutOfBand Data */
		if( PaxosValidate( pPaxos, PAXOS_MODE_BEGIN, 
				Seq( k ), &pmb->b_Value, &pmb->b_MetaCmd ) ) {

			msg_test_newround_t	*pmn;

			pmn	= (msg_test_newround_t*)msg_alloc( pPaxos,
						pserverAccept->s_ID, MSG_TEST_NEWROUND, sizeof(*pmn) );

			PaxosSend( pPaxos, pserverAccept, (msg_t*)pmn );
			DBG_EXT( FALSE );
			return( FALSE );
		}
		if( PMode( k ) == MODE_NONE )	PMode( k )	= MODE_WAIT;

		MetaCmdRnd( k )	= pmb->b_MetaCmd;

		RndBallot( k )	= pmb->b_CurRnd;
		RndValue( k )	= pmb->b_Value;
		RndValue( k ).v_From	= MyId;
#ifdef	ZZZ
#else
		if( pPaxos->p_bPageSuppress ) {

			LOG( pPaxos->p_pLog, LogINF, 
				"PageSuppress Page=%u k=%d Seq(k)=%u", Page, k, Seq(k) );

			Suppressed( k )	= TRUE;
			RETURN( FALSE );
		}
#endif
		if( !IsTimespecZero( pPaxos->p_CatchupRecvLast ) ) {
/***
LOG( pPaxos->p_pLog, "SUPRESSED Page=%u k=%d Seq(k)=%u Last-t=%u",
Page, k, Seq(k), (uint32_t)(pPaxos->p_CatchupRecvLast-time(0)) );
***/
			timespec_t	Now;
			TIMESPEC( Now );
			if( TIMESPECCOMPARE( Now, pPaxos->p_CatchupRecvLast ) < 0 ) {

				Suppressed( k )	= TRUE;
				RETURN( FALSE );
			}
		}
		pma	= (msg_accept_t*)msg_alloc( pPaxos,
							pserverAccept->s_ID, MSG_ACCEPT, sizeof(*pma) );

		pma->a_Page		= Page;
		pma->a_k		= k;
		pma->a_round	= RndBallot( k );
		pma->a_Value	= RndValue( k );
		pma->a_MetaCmd	= MetaCmdRnd( k );

		PaxosSend( pPaxos, pserverAccept, (msg_t*)pma );

	} else {
		pmo	= (msg_oldround_t*)msg_alloc( pPaxos,
							pserverAccept->s_ID, MSG_OLDROUND, sizeof(*pmo) );

		pmo->o_round	= pmb->b_CurRnd;
		pmo->o_commit	= CurRnd;

		PaxosSend( pPaxos, pserverAccept, (msg_t*)pmo );
	}
	DBG_EXT( TRUE );
	return( TRUE );
}

/*
 *	Success
 */
int
SendSuccess( Paxos_t* pPaxos, int k )
{
	int j;
	Server_t		*pserver;
	msg_success_t	*pSuccess;
	bool_t	send = FALSE;

	DBG_ENT();

	if( PMode( k ) != MODE_DECIDED )	return( 0 );
 
	ASSERT( !isEmpty( &MetaCmdRnd( k ) ) );

	for( j = 0; j < pPaxos->p_Maximum; j++ ) {
		if( j == pPaxos->p_MyId )	continue;

		pserver	= &pPaxos->p_aServer[j];
		pSuccess = (msg_success_t*)msg_alloc(pPaxos,
						pserver->s_ID, MSG_SUCCESS, sizeof(*pSuccess));

		pSuccess->s_NextDecide	= NextDecide;
		pSuccess->s_BackPage	= BackPage;
		pSuccess->s_Page		= Page;
		pSuccess->s_NextK		= NextK;
		pSuccess->s_k			= k;
		pSuccess->s_Value		= RndValue( k );
		pSuccess->s_MetaCmd		= MetaCmdRnd( k );

		PaxosSend( pPaxos, pserver, (msg_t*)pSuccess );

		send	= TRUE;

//		DBG_TRC("j= %d k=%d PrevSend=%d\n", j, k, PrevSend );
	}
	DBG_EXT( send );
	return( send );
}

bool_t
RecvSuccess( Paxos_t* pPaxos, msg_success_t* pSuccess )
{
	int	j, k;
	Server_t	*pserver;

	DBG_ENT();

	k	= pSuccess->s_k;
	j	= pSuccess->s_head.m_fr;
	pserver	= &pPaxos->p_aServer[j];

DBG_TRC("j=%d k=%d Page=%d s_Page=%d Defined=%d\n", 
			j, k, Page, pSuccess->s_Page, isDefined(&RndValue(k)) );

ASSERT( !isEmpty( &pSuccess->s_MetaCmd ) );

	if( Page != pSuccess->s_Page ) {
		LOG( pPaxos->p_pLog, LogINF,
			"j=%d Page=%u s_Page=%u s_NextDecide=%u s_BackPage=%u", 
			j, Page, pSuccess->s_Page, 
			pSuccess->s_NextDecide, pSuccess->s_BackPage );
	}
	if( CMP( Page, pSuccess->s_Page ) < 0 ) {	// Agent Delay
		DBG_TRC("Page=%d s_Page=%d\n", Page, pSuccess->s_Page );
		if( CMP( pSuccess->s_BackPage, Page ) <= 0 ) {
			CatchupRequest( pPaxos, pserver, pSuccess->s_NextDecide );
		}
		DBG_EXT( FALSE );
		return( FALSE );
	} else if( CMP( Page, pSuccess->s_Page ) > 0 ) {
		RETURN( FALSE );
	}

	if( PMode( k ) != MODE_DECIDED ) {

	/* OutOfBand Data MetaCmdRndと異なるかも知れない */
		if( !isSameValue( &pSuccess->s_Value, &RndValue( k ) ) ) {
			/*
			 * NextDecideを更新していないのでデータはある筈
			 */
			if( PaxosValidate( pPaxos, PAXOS_MODE_SUCCESS, 
				Seq( k ), &pSuccess->s_Value, &pSuccess->s_MetaCmd ) ) {

				LOG( pPaxos->p_pLog, LogINF, "Validate ERROR %u", Seq( k ) );

				DBG_EXT( FALSE );
				return( FALSE );
			}
		}

		PMode( k ) = MODE_DECIDED;

		RndValue( k )			= pSuccess->s_Value;
		RndValue( k ).v_From	= MyId;

		MetaCmdRnd( k )	= pSuccess->s_MetaCmd;

		RndValue( k ).v_Flags	|= VALUE_NORMAL;
		Exec( pPaxos, k );
	}
	DBG_EXT( TRUE );
	return( TRUE );
}

/*
 *	未決定状態であれば新roundを開始する
 */
bool_t
CheckRndSuccessAndNewRound( Paxos_t* pPaxos )
{
	int	k;

	DBG_ENT();

	if( IamMaster != TRUE  ) RETURN( FALSE );

	if( isMyRnd ) {
		for( k = 0; k < PAXOS_MAX; k++ ) {

			timespec_t	Now;
			TIMESPEC( Now );
			if( IsTimespecZero( Deadline(k) )
				|| TIMESPECCOMPARE( Now, Deadline( k ) ) <= 0 )	continue;

			if( PMode( k ) != MODE_DECIDED ) {
	LOG( pPaxos->p_pLog, LogDBG, 
		"NOT DECIDED->NEWROUND Acc=0x%x PMode=%d k=%d Seq=%u", 
		RndAccQuo(k).q_member, PMode(k), k, Seq( k ) );
				AMode	= MODE_NONE;
				NewRound( pPaxos );
				break;
			}
		}
	} else {
		if( AMode != MODE_GATHERLAST ||
				( AMode == MODE_GATHERLAST && NewRoundLast < time(0) ) ) {
	LOG( pPaxos->p_pLog, LogDBG,
	"GATHERLAST->NEWROUND AMode=%d RndInfQuo=0x%x", 
	AMode, RndInfQuo.q_member );
				AMode	= MODE_NONE;
				NewRound( pPaxos );
		}
	}

	DBG_EXT( TRUE );
	return( TRUE );
}

int
Execute( Paxos_t* pPaxos, uint32_t seq, value_t* pD, MetaCmd_t* pC )
{
	int	Ret = 0;
	PaxosExecute_t	*pH;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	char		*pData;

	if( pD->v_Flags & VALUE_NOOP ) {
		(pPaxos->p_fNotify)( pPaxos, 
			PAXOS_MODE( pD ), PAXOS_NOOP, 
			seq, pD->v_Server, pD->v_Epoch,
			pC->c_len, pC->c_opaque, MyId );

	} else {

		pMsg	= MsgCreate( 2, pPaxos->p_fMalloc, pPaxos->p_fFree );

		pH = (PaxosExecute_t*)(pPaxos->p_fMalloc)( sizeof(PaxosExecute_t) );
		pH->e_Seq				= seq;
		pH->e_BackSeq			= TO_SEQ( BackPage, 0 );
		pH->e_Notify		= PAXOS_ACCEPTED;
		if( pD->v_Flags & VALUE_VALIDATE )	pH->e_Validate	= TRUE;
		else								pH->e_Validate	= FALSE;

		MsgVecSet( &Vec, VEC_MINE, pH, sizeof(*pH), sizeof(*pH) );
		MsgAdd( pMsg, &Vec );

		pData	= (pPaxos->p_fMalloc)( pC->c_len );
		memcpy( pData, pC->c_opaque, pC->c_len );
		
		MsgVecSet( &Vec, 0, pData, pC->c_len, pC->c_len );
		MsgAdd( pMsg, &Vec );

		QueuePostEntry( &pPaxos->p_Outbound, pMsg, m_Lnk );
	}
	return( Ret );
}

bool_t
ExecUp( Paxos_t* pPaxos, int k )
{
	Gap_t	*pGap;
	HashItem_t	*pItem;

	DBG_ENT();

	if( NextK < k + 1 ) {
		NextK = k + 1;
DBG_TRC("Page=%d k=%d NextK=%d\n", Page, k, NextK );
	}

DBG_TRC("Seq=%d NextDecide=%d\n", Seq(k), NextDecide );
	if( CMP( Seq( k ), NextDecide ) < 0 ) {
		return( FALSE );
	}
	if( isDefined( &InitValue( k ) ) && 
			InitValue( k ).v_Server != RndValue( k ).v_Server ) {

	LOG( pPaxos->p_pLog, LogDBG,
	"REJECTED  k=%d seq(k)=%u", k, Seq( k ) );

		(pPaxos->p_fNotify)( pPaxos, 
				PAXOS_MODE( &InitValue( k ) ), PAXOS_REJECTED, 
				Seq( k ), InitValue( k ).v_Server, InitValue( k ).v_Epoch,
				MetaCmdInit( k ).c_len, MetaCmdInit( k ).c_opaque, MyId );
	}
		
	if( CMP( Seq( k ), NextDecide ) == 0 ) {
		NextDecide++;
DBG_TRC("NextDecide incremented NextDecide=%d\n", NextDecide );
		if( pPaxos->p_bSuppressAccept ) {
			pPaxos->p_bSuppressAccept = FALSE;
			LOG( pPaxos->p_pLog, LogINF, "SuppressAccept->FLASE" );
		}

/* 応答 */
		Execute( pPaxos, Seq( k ), &RndValue( k ), &MetaCmdRnd( k ) );
		while( (pItem = HashGetItem( &pPaxos->p_Gaps, 
							UINT32_PNT(NextDecide)) ) ) {

			pGap	= (Gap_t*)HASH_VALUE( pItem );
/* 応答 */
			Execute( pPaxos, NextDecide, 
					&pGap->g_Decision, &pGap->g_MetaCmd );

			NextDecide++;
			PaxosFree( pPaxos, (void*)pGap );

			HashRemoveItem( &pPaxos->p_Gaps, pItem );
		}
	} else {
		pGap = (Gap_t*)PaxosMalloc( pPaxos, sizeof(Gap_t) );
		pGap->g_Seq			= Seq( k );
		pGap->g_Decision	= RndValue( k );
		pGap->g_MetaCmd		= MetaCmdRnd( k );
		HashPut( &pPaxos->p_Gaps, UINT32_PNT( pGap->g_Seq ), 
												(void*)pGap );
	}
	SuccessCount++;
	DBG_EXT( TRUE );
	return( TRUE );
}

bool_t
Exec( Paxos_t* pPaxos, int k )
{
	DBG_ENT();
	/*
	 *	ｺﾏﾝﾄﾞ実体がない場合はcatchupを待つ
	 */
	ASSERT( !isEmpty( &MetaCmdRnd(k) ) );

	if( isEmpty( &MetaCmdRnd(k) ) ) {
	LOG( pPaxos->p_pLog, LogDBG, "===NO COMMAND k=%d===", k );
		RETURN( FALSE );
	}

	/* 今の発火を終了させ、次の発火に備える */
	if( pPaxos->p_IssuedLast
		&& CMP( TO_SEQ( Page, NextK ), pPaxos->p_CatchupNextDecide ) >= 0 ) {

	LOG( pPaxos->p_pLog,LogDBG,  
	"STOP FIRE NextDecide=%u p_IssuedLast=%u IssuedSeq=%u CatcupNextSeq=%u", 
	NextDecide, (uint32_t)pPaxos->p_IssuedLast, pPaxos->p_IssuedSeq, 
	pPaxos->p_CatchupNextDecide );

		pPaxos->p_IssuedLast = 0;
		pPaxos->p_CatchupNextDecide = pPaxos->p_IssuedSeq + 1;
	}

	if( ExecUp( pPaxos, k ) ) {

	LOG( pPaxos->p_pLog,LogDBG, "SuccessCount=%d", SuccessCount ); 

		if( SuccessCount >= PAXOS_MAX ) {

			UpdatePage( pPaxos );
		}
	}
	RETURN( TRUE );
}

int
UpdatePage( Paxos_t* pPaxos )
{
	int	k;

	DBG_ENT();

DBG_TRC("===UpdatePage Page=%d SuccessCount=%d\n", Page, SuccessCount );

	if( SuccessCount < PAXOS_MAX )	RETURN( -1 );

#ifdef	ZZZ
	/* 念のために */
	for( k = 0; k < PAXOS_MAX; k++ ) {
		if( PMode( k ) != MODE_DECIDED ) {
	LOG( pPaxos->p_pLog, LogDBG, "ERROR:Page=%d k=%d", Page, k );
			RETURN( -1 );
		}
	}
#endif

	PutPage( pPaxos );

	for( k = 0; k < PAXOS_MAX; k++ ) {
		quorum_init( &RndAccQuo(k), pPaxos->p_Max/*PAXOS_SERVER_MAX*/ );
	}

	Page++;
	NextK	= 0;
	SuccessCount = 0;

/* Wake up */
DBG_TRC("WAKEUP Page=%d\n", Page );
	CndBroadcast( &pPaxos->p_Cnd );

	DBG_EXT( 0 );
	return( 0 );
}

/* At first Called in ImMaster */
int
GetSlot( Paxos_t* pPaxos )
{
	int	k;
	int	err;

	DBG_ENT();

again:
DBG_TRC("IamMaster=%d isMyRnd=%d PAGE=%d NextK=%d\n", 
IamMaster, isMyRnd, Page, NextK );
	if( !IamMaster )	return( -1 );	// May be changed

#ifdef	WWW
	if( pPaxos->p_WaterReplica.q_count > 0 ) {
		LOG( pPaxos->p_pLog, LogINF, "WATER WAIT member=0x%x", 
							pPaxos->p_WaterReplica.q_member );
		err = CndTimedWaitWithMs( &pPaxos->p_Cnd, &pPaxos->p_Lock,
										PAXOS_WATER_DT_MS );
		if( err ) {
			quorum_init( &pPaxos->p_WaterReplica, pPaxos->p_Maximum );
		}
		LOG( pPaxos->p_pLog, LogINF, "WATER WAKEUP err=%d", err );
		goto again;
	}
#endif

	if( isMyRnd ) {

	/*
	 * NextDecideで昇順に投入
	 *  クライアントの異なるリーダへの重複投入は
	 * 異なるインスタンス通番となるが
	 * セッション管理で重複実行しないようにする
	 *	
	 */
LOG( pPaxos->p_pLog, LogDBG, "Page=%d TO_PAGE(%d)=%d", 
	Page, NextDecide, TO_PAGE(NextDecide));

		if( Page == TO_PAGE( NextDecide ) ) {

			for( k = TO_K( NextDecide ); k < PAXOS_MAX; k++ ) {

LOG( pPaxos->p_pLog, LogDBG, "PMode(%d)=%d", k, PMode(k) );

				if( PMode( k ) == MODE_NONE ) {

					RETURN( k );
				}
			}
		}
/* Wait Page update */
		err = CndTimedWaitWithMs( &pPaxos->p_Cnd, &pPaxos->p_Lock,
											DT_SUCCESS/1000 );
		if( err )	RETURN( -err );
		goto again;
	} else {

		if( NewRndStart ) {

	LOG( pPaxos->p_pLog, LogDBG,
	"NEW ROUND ? isMyRnd=%d AMode=%d NextK=%d Page=%d", 
			isMyRnd, AMode, NextK, Page);
	for( k = 0; k < PAXOS_MAX; k++ ) {
	LOG( pPaxos->p_pLog, LogDBG,
	"seq=%u k=%d %d %d (0x%x %d-%d-%d) (0x%x %d-%d-%d)", 
	TO_SEQ(Page,k), k, PMode(k), isDecided(k), 
	InitValue(k).v_Flags, InitValue(k).v_Server, 
	InitValue(k).v_Epoch,InitValue(k).v_From,
	RndValue(k).v_Flags, RndValue(k).v_Server, 
	RndValue(k).v_Epoch,RndValue(k).v_From);
	}
			if( AMode != MODE_GATHERLAST ) {
	LOG( pPaxos->p_pLog, LogDBG, "NEW ROUND");
				AMode	= MODE_NONE;
				NewRound( pPaxos );
			}
			err = CndTimedWaitWithMs( &pPaxos->p_Cnd, &pPaxos->p_Lock,
											DT_SUCCESS/1000 );
			if( err )	RETURN( -err );
			goto again;
		}
		else {
	LOG( pPaxos->p_pLog, LogDBG, "SOMEONE GETS ROUND\n");
			RETURN( -1 );
		}
	}
	RETURN( -1 );
}

int
InitCommand( Paxos_t* pPaxos, int k, MetaCmd_t *pCommand, int Flag )
{
	value_t	V;

	memset( &V, 0 , sizeof(V) );
	V.v_Flags	|= (VALUE_DEFINED|Flag);
	V.v_Server	= MyId;
	V.v_From	= MyId;

	InitValue( k ) 		= V;
	MetaCmdInit( k )	= *pCommand;
	return( k );
}

int
CatchupRequest( Paxos_t *pPaxos, Server_t* pserver, uint32_t nextDecide )
{
	if( pPaxos->p_CatchupStop )	return( -1 );

	if( !(pserver->s_status & PAXOS_SERVER_CONNECTED ) ) {
		LOG( pPaxos->p_pLog, LogINF, "NOT CONNECTED j=%d nextDecide=%u ===", 
				pserver->s_ID, nextDecide );
		return( -1 );
	}

	if( pPaxos->p_IssuedLast ) {
	/* 発火中 */
		if( pPaxos->p_IssuedLast < time(0) ) {
		/* やり直し */
			pPaxos->p_IssuedLast		= 0;
			pPaxos->p_pCatchupServer	= pserver;
			pPaxos->p_CatchupNextDecide	= nextDecide;
	LOG( pPaxos->p_pLog, LogDBG,
	"===== AGAIN FIRE j=%d nextDecide=%u ===", pserver->s_ID, nextDecide );
			CatchupFire( pPaxos );
		} else {

	DBG_TRC("IGNORED-1 j=%d nextDecide=%u CatchupSeq=%u Issued=%d\n", 
	pserver->s_ID, nextDecide,
	pPaxos->p_CatchupNextDecide, pPaxos->p_IssuedSeq );
		/* 検知した最大を記録 */
		/* catchup中にｻｰﾊﾞが死んだら？ */
			if( CMP( nextDecide, pPaxos->p_CatchupNextDecide ) > 0 ) {
				pPaxos->p_pCatchupServer	= pserver;
				pPaxos->p_CatchupNextDecide	= nextDecide;

				/* 他のサーバーにも依頼する */
				if( IsTimespecZero(pserver->s_IssuedTime) ) {
	LOG( pPaxos->p_pLog, LogDBG, "===== FIRE DUP j=%d fr=%u to=%u ===", 
	pserver->s_ID, NextDecide, nextDecide );
					CatchupFire( pPaxos );
				}
			}
		}
	} else {
	/* 発火 */
		pPaxos->p_pCatchupServer	= pserver;
		pPaxos->p_CatchupNextDecide	= nextDecide;
	LOG( pPaxos->p_pLog, LogDBG, "===== FIRE j=%d fr=%u to=%u ===", 
	pserver->s_ID, NextDecide, nextDecide );
			CatchupFire( pPaxos );
	}
	return( 0 );
}

int
CatchupFire( Paxos_t* pPaxos )
{
	Server_t*	pserver;
	uint32_t	seq;
	uint32_t	SeqTo;
	msg_catchup_req_t	*pReq;
	int			cnt;


	if( pPaxos->p_IssuedLast )	seq	= pPaxos->p_IssuedSeq + 1;
	else						seq	= NextDecide;

	/* 念のため */
	if( CMP( seq, pPaxos->p_CatchupNextDecide ) >= 0 ) {
	LOG( pPaxos->p_pLog, LogDBG, "ALREADY FIRED seq=%u CatchupSeq=%u",
	seq, pPaxos->p_CatchupNextDecide );
		pPaxos->p_IssuedLast	= 0;
		return( -1 );
	}

	pserver	= pPaxos->p_pCatchupServer;

DBG_TRC("#### CATCHUP START j=%d seq=%u\n", pserver->s_ID, seq );

	if( pPaxos->p_IssuedLast == 0 ) 
		pPaxos->p_IssuedLast	= time(0) + (2*DT_D)/1000000LL;

	if( CMP( seq, pPaxos->p_CatchupNextDecide ) < 0 ) {

		cnt	= pPaxos->p_CatchupNextDecide - seq;
		if( cnt > PAXOS_FIRE_MAX )	cnt = PAXOS_FIRE_MAX;

		SeqTo	= seq + cnt - 1;

		pserver->s_IssuedSeq	= SeqTo;
		TIMESPEC( pserver->s_IssuedTime );

		pPaxos->p_IssuedLast	+= (DT_L*cnt)/1000000LL;
		pPaxos->p_IssuedSeq		= SeqTo;	

		pReq	= (msg_catchup_req_t*)msg_alloc( pPaxos,
				pserver->s_ID, MSG_CATCHUP_REQ, sizeof(*pReq) );

		pReq->c_SeqFrom			= seq;
		pReq->c_SeqTo			= SeqTo;
		pReq->c_RecvLastDT	= DT_D + DT_L + DT_L*cnt;

DBG_TRC("cnt=%d SeqFrom=%u SeqTo=%u\n", cnt, seq, SeqTo );
		PaxosSend( pPaxos, pserver, (msg_t*)pReq );
	}

DBG_TRC("#### CATCHUP END seq=%u SeqTo=%u\n", seq, SeqTo );
	return( 0 );
}

int
CatchupInstanceRpl( Paxos_t *pPaxos, Server_t* pserver, 
								PaxosMulti_t* pMulti, uint32_t seq )
{
	msg_catchup_rpl_t	*pRpl;
	int32_t	k;

	if( pMulti ) {
		k	= TO_K( seq );
		if( pMulti->m_aPaxos[k].p_Mode != MODE_DECIDED ) {
	LOG( pPaxos->p_pLog,LogDBG,  
	"#### NOT DEFINED page=%u k=%d", 
	pMulti->m_PageHead.p_Page, k );
			return( -1 );
		}
		if( isEmpty( &pMulti->m_aMetaCmd[k].c_MetaCmdRnd ) ) {
	LOG( pPaxos->p_pLog,LogDBG,  
	"#### EMPTY page=%u k=%d", pMulti->m_PageHead.p_Page, k );
			return( -1 );
		}

		pRpl	= (msg_catchup_rpl_t*)msg_alloc( pPaxos,
				pserver->s_ID, MSG_CATCHUP_RPL, sizeof(*pRpl) );

		pRpl->c_True	= TRUE;
		pRpl->c_MyNextDecide	= NextDecide;
		pRpl->c_Seq		= seq;
		pRpl->c_PaxosI	= pMulti->m_aPaxos[k];
		pRpl->c_MetaCmd	= pMulti->m_aMetaCmd[k].c_MetaCmdRnd;
	} else {
		pRpl	= (msg_catchup_rpl_t*)msg_alloc( pPaxos,
				pserver->s_ID, MSG_CATCHUP_RPL, sizeof(*pRpl) );

		pRpl->c_True	= FALSE;
		pRpl->c_Seq		= seq;
	}

	PaxosSend( pPaxos, pserver, (msg_t*)pRpl );

	return( 0 );
}

int
PaxosAutonomic( Paxos_t *pPaxos, int j )
{
	pPaxos->p_aServer[j].s_AutonomicLast	= time(0) + DT_AUTONOMIC_SEC;
	LOG( pPaxos->p_pLog, LogINF, "Autonomic[%d] AutonomicLast=%ld", 
		j, pPaxos->p_aServer[j].s_AutonomicLast );
	return( 0 );
}

Paxos_t	PaxosW;

int
PutPage( Paxos_t *pPaxos )
{
	PaxosMulti_t*	pMulti;
	int	j;
	Server_t	*pserver;

	pMulti	= (PaxosMulti_t*)(pPaxos->p_fMalloc)(sizeof(PaxosMulti_t));
	ASSERT( pMulti != NULL );
	*pMulti	= pPaxos->p_Multi;

	list_init( &pMulti->m_Lnk );

/* Reverse order */
	list_add( &pMulti->m_Lnk, &pPaxos->p_MultiLnk );

/* Remove */
/* これはページ数がよい？ */
	bool_t	bRemove = TRUE;

	for( j = 0; j < pPaxos->p_Maximum; j++ ) {
		if( j == MyId )	continue;
		pserver	= &pPaxos->p_aServer[j];
		if( pserver->s_AutonomicLast < time(0) ) {
			pserver->s_AutonomicLast = 0;
		} else {
			bRemove	= FALSE;
			break;
		}
	}
	
	if( bRemove ) {

		/* 残すページの確定 */
		uint32_t	nextDecide;
		uint32_t	backPage;

		nextDecide	= NextDecide;
		for( j = 0; j < pPaxos->p_Maximum; j++ ) {

			if( j == MyId )	continue;

			pserver	= &pPaxos->p_aServer[j];
			if( !(pserver->s_status & PAXOS_SERVER_CONNECTED ) )	continue;

//			if( pserver->s_NextDecide ) {
				if( CMP( nextDecide, pserver->s_NextDecide ) > 0 ) {
					nextDecide	= pserver->s_NextDecide;
				}
//			}
		}
		backPage	= TO_PAGE( nextDecide );
#ifdef	ZZZ
#else
LOG( pPaxos->p_pLog, LogDBG, 
"Page[%u],backPage[%u]", Page, backPage );

		if( CMP( Page, backPage ) >= PAXOS_PAGE_MAX_LEN ) {
			LOG( pPaxos->p_pLog, LogINF, 
				"PageSupress On:Page[%u],backPage[%u] PAGE_MAX_LEN=%d", 
				Page, backPage, PAXOS_PAGE_MAX_LEN );

			pPaxos->p_bPageSuppress = TRUE;
		}
#endif
		while( (pMulti = list_last_entry(&pPaxos->p_MultiLnk, 
									PaxosMulti_t, m_Lnk)) ) {

			 if( /* pMulti->m_PageHead.p_PageLast < time(0) 
					&& */ ( CMP( backPage, pMulti->m_PageHead.p_Page ) > 0 ) ) {
DBG_TRC("REMOVE backPage=%d Page=%d Last=%lu\n", 
backPage, pMulti->m_PageHead.p_Page, pMulti->m_PageHead.p_PageLast );
				list_del( &pMulti->m_Lnk );
				(pPaxos->p_fFree)( pMulti );
			} else {
				break;
			}
		}
		if( (pMulti = list_last_entry(&pPaxos->p_MultiLnk, 
									PaxosMulti_t, m_Lnk)) ) {
			BackPage = pMulti->m_PageHead.p_Page;
		} else {
			BackPage = Page;
		}
	}
DBG_TRC("Page=%d\n", Page );
	memset(pPaxos->p_Multi.m_aPaxos, 0, sizeof(pPaxos->p_Multi.m_aPaxos));
	memset(pPaxos->p_Multi.m_aMetaCmd, 0, sizeof(pPaxos->p_Multi.m_aMetaCmd));

	return( 0 );
}

PaxosMulti_t*
GetPage( Paxos_t *pPaxos, int page )
{
	PaxosMulti_t*	pMulti;

	if( page == Page )	return( &Multi );
	else {
		/* from Disk or cache ??? */

		list_for_each_entry( pMulti, &pPaxos->p_MultiLnk, m_Lnk ) {
			if( pMulti->m_PageHead.p_Page != page )	continue;

DBG_TRC("page=%d\n", page );
			return( pMulti );
		}
		return( NULL );
	}
}

/*
 *	Dispatcher
 */
int	
server_receive_dump( Paxos_t* pPaxos, msg_t* pm )
{
	msg_dump_t	*pDump = (msg_dump_t*)pm;

	DBG_ENT();
	
	PaxosSendTo( pPaxos, (void*)pDump->d_addr, pDump->d_len, 0,
						&pm->m_From, sizeof(pm->m_From) );
	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_control_start( Paxos_t* pPaxos, msg_t* pm )
{

	DBG_ENT();

	pPaxos->p_MallocCount	= MallocCount();
	
	PaxosSendTo( pPaxos, &pPaxos, sizeof(pPaxos), 0, 
						&pm->m_From, sizeof(pm->m_From) );
	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_stop( Paxos_t* pPaxos, msg_t* pm )
{
	msg_outofband_stop_t*	pstop = (msg_outofband_stop_t*)pm;
	DBG_ENT();
	
	ChOutboundStop( &pPaxos->p_aServer[pstop->s_server].s_ChSend );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_restart( Paxos_t* pPaxos, msg_t* pm )
{
	msg_outofband_restart_t*	prestart = (msg_outofband_restart_t*)pm;
	DBG_ENT();
	
	ChOutboundRestart( &pPaxos->p_aServer[prestart->r_server].s_ChSend );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_down( Paxos_t* pPaxos, msg_t* pm )
{
	int	j;

	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
		ChOutboundDown( &pPaxos->p_aServer[j].s_ChSend );
	}
	Active	= FALSE;
	kill( 0, SIGINT );
	return( 0 );
}

int	
server_receive_birth( Paxos_t* pPaxos, msg_t* pm )
{
	TIMESPEC( EBirth );
	return( 0 );
}

/*
 *	1	DISCONNECT->DISCONNECT
 *	2	DISCONNECT->CONNECT
 *	3	CONNECT->CONNECT
 *	4	CONNECT->DISCONNECT
 */	
typedef enum {
	DIS_DIS,
	DIS_CON,
	CON_CON,
	CON_DIS
} Con_t;

bool_t
IS_CONNECTED( Paxos_t *pPaxos, Server_t *pS )
{
	timespec_t	w, c;
	int	Ret;

	w = pS->s_Token;
	TimespecAddUsec( w, DT_CON );
	TIMESPEC( c );
	Ret = TIMESPECCOMPARE( c, w );
	if( Ret < 0 )	return( TRUE );
	else			return( FALSE );
}

Con_t
CheckConnection( Paxos_t* pPaxos, Server_t* pserver )
{
	Con_t	con;

	DBG_ENT();

	if( IS_CONNECTED( pPaxos, pserver ) ) {
		if( !(pserver->s_status & PAXOS_SERVER_CONNECTED ) ) {
			if( pserver->s_AutonomicLast )	pserver->s_AutonomicLast = 0;
			pserver->s_status	|= PAXOS_SERVER_CONNECTED;
			DBG_TRC("ADD %d\n", pserver->s_ID );
			LOG( pPaxos->p_pLog, LogINF,
			"DIS_CON:CONNECTED[%d],BACKPAGE[%u],s_NextDecied[%u]",
			pserver->s_ID,BackPage,pserver->s_NextDecide);
			con	= DIS_CON;
		} else {
			DBG_TRC("CONNECT CONTINUE %d\n", pserver->s_ID );
			con	= CON_CON;
		}
	} else {
		if( (pserver->s_status & PAXOS_SERVER_CONNECTED ) ) {
			pserver->s_status	&= ~PAXOS_SERVER_CONNECTED;
			DBG_TRC("DEL %d\n", pserver->s_ID );
			LOG( pPaxos->p_pLog, LogINF,
				"CON_DIS:DISCONNECTED[%d]", pserver->s_ID );
			con	= CON_DIS;
		} else {
			DBG_TRC("DISCONNECT CONTINUE %d\n", pserver->s_ID );
//			LOG( pPaxos->p_pLog, "DIS_DIS[%d]", pserver->s_ID );
			con	= DIS_DIS;
		}
	}
	DBG_EXT( con );
	return( con );
}

int
set_reader( Paxos_t* pPaxos )
{
	int				j;
	int32_t			Load;
	int32_t			Reader;
	Server_t		*pserver;

	/*
	 * Load check
	 */
	if( pPaxos->p_fLoad ) {
		pPaxos->p_aServer[MyId].s_Load = (pPaxos->p_fLoad)(pPaxos->p_pTag);
	} else {
		pPaxos->p_aServer[MyId].s_Load = -1;
	}
	Reader	= MyId;
	Load	= pPaxos->p_aServer[MyId].s_Load;
	for( j = 0; j < pPaxos->p_Maximum; j++ ) {

		if( j == MyId )	continue;

		pserver = &pPaxos->p_aServer[j];
		if( (pserver->s_status & PAXOS_SERVER_CONNECTED ) ) {
			if( pserver->s_Load < Load ) {
				Load 	= pserver->s_Load;
				Reader	= j;
			}
		}
	}
	EReader	= Reader;

	return( 0 );
}

int	
server_receive_alive( Paxos_t* pPaxos, msg_t* pm )
{
	Server_t*		pserver = &pPaxos->p_aServer[pm->m_fr];
	msg_alive_t*	palive = (msg_alive_t*)pm;
	bool_t			Changed;
	int				j;
	bool_t			Catchup = FALSE;

	int	ret = -1;

	DBG_ENT();

	/* IP addr change */
	if( memcmp( &pm->m_From, &pserver->s_ChSend.c_PeerAddr, 
						sizeof(struct sockaddr_in) ) ) {
		char	from[32], to[32];

		sprintf( from, "%s:%d",
			inet_ntoa(pserver->s_ChSend.c_PeerAddr.sin_addr),
			ntohs(pserver->s_ChSend.c_PeerAddr.sin_port) );
		sprintf( to, "%s:%d",
			inet_ntoa(pm->m_From.sin_addr),
			ntohs(pm->m_From.sin_port) );

		LOG( pPaxos->p_pLog, LogINF, "%d [%s]->[%s]", pm->m_fr, from, to );

		if( pserver->s_status & PAXOS_SERVER_ACTIVE ) {
			pserver->s_ChSend.c_PeerAddr	= pm->m_From;
		} else {
			pserver->s_status |= PAXOS_SERVER_ACTIVE;
			ChSendInit( &pserver->s_ChSend, pPaxos, 
				pm->m_From, ChSendTo, pPaxos->p_fMalloc, pPaxos->p_fFree );
		}
	}

	TIMESPEC( pserver->s_Alive );
	if( pm->m_to != pPaxos->p_MyId ) {
		LOG( pPaxos->p_pLog, LogINF, "ERROR ! to=%d mine=%d", 
		pm->m_to, pPaxos->p_MyId );
		goto err;
	}
	if( !(pserver->s_status & PAXOS_SERVER_ACTIVE ) ) {
		LOG( pPaxos->p_pLog, LogINF, "ERROR ! =%d is not active.", pm->m_fr );
		goto err;
	}
	if( MyId < pPaxos->p_Max/*PAXOS_SERVER_MAX*/ && pm->m_fr < pPaxos->p_Max/*PAXOS_SERVER_MAX*/ ) {
		Changed	= ( pserver->s_leader != palive->a_leader
		|| TIMESPECCOMPARE(pserver->s_Election,  palive->a_Election) != 0 );
	}

	pserver->s_Election		= palive->a_Election;
	pserver->s_leader		= palive->a_leader;
	pserver->s_epoch		= palive->a_epoch;
	pserver->s_YourTime		= palive->a_MyTime;
	pserver->s_Load			= palive->a_Load;
	pserver->s_BackPage		= palive->a_BackPage;
	pserver->s_NextDecide	= palive->a_NextDecide;

	/* Disconnect(my heart beat) One-Way */
	pserver->s_Token	= palive->a_YourTime;

	/*
	 * Epoch update
	 */
	if( CMP( palive->a_epoch, Epoch ) > 0 ) {
			Epoch	= palive->a_epoch;
			Changed = TRUE;
	}
#ifdef	ZZZ
#else
	/*
	 * Page overflow control
	 */
	if( pPaxos->p_bPageSuppress ) {

		uint32_t	nextDecide;
		uint32_t	backPage;

		nextDecide	= NextDecide;
		for( j = 0; j < pPaxos->p_Maximum; j++ ) {

			if( j == MyId )	continue;

			pserver	= &pPaxos->p_aServer[j];
LOG( pPaxos->p_pLog, LogDBG, 
"j=%d NextDecide=%u s_NextDecide=%u", j, NextDecide, pserver->s_NextDecide );

			if( !(pserver->s_status & PAXOS_SERVER_CONNECTED ) )	continue;

//			if( pserver->s_NextDecide ) {
				if( CMP( nextDecide, pserver->s_NextDecide ) > 0 ) {
					nextDecide	= pserver->s_NextDecide;
				}
//			}
		}
		backPage	= TO_PAGE( nextDecide );

LOG( pPaxos->p_pLog, LogDBG, 
"BackPage=%u backPage=%u", BackPage, backPage );
		if( CMP( BackPage, backPage ) < 0 ) {
			pPaxos->p_bPageSuppress = FALSE;
			LOG( pPaxos->p_pLog, LogDBG, 
				"PageSuppress:Off BackPage=%u backPage=%u", BackPage, backPage );

			release_suppressed_accept( pPaxos );
		}
	}
#endif

	switch( CheckConnection( pPaxos, pserver ) ) {
		case DIS_CON:
		case CON_DIS:
			if( MyId < pPaxos->p_Max/*PAXOS_SERVER_MAX*/ && pm->m_fr < pPaxos->p_Max/*PAXOS_SERVER_MAX*/ ) {
				set_leader( pPaxos, pserver );
			}
			break;
		case CON_CON:
			if( MyId < pPaxos->p_Max/*PAXOS_SERVER_MAX*/ && (Changed || ELeader < 0 )  ) {
				set_leader( pPaxos, pserver );
			}
			/*
			 *	Page
			 */
			/* MtxLockでもよいがautonomic等でロック時間が長いことがあり、
			 * heartbeatを隔離する
			 */
			if( palive->a_Catchup 
					&& !MtxTrylock( &pPaxos->p_Lock ) ) {

				if( CMP( NextDecide, palive->a_NextDecide ) < 0 ) {
					if( CMP( palive->a_BackPage, TO_PAGE(NextDecide) ) <= 0 ) {
						LOG( pPaxos->p_pLog, LogINF, 
							"j=%d NextDecide=%u a_NextDecide=%u", 
							pm->m_fr, NextDecide, palive->a_NextDecide );
						CatchupRequest( pPaxos, pserver, palive->a_NextDecide );
					} else {
						for( j = 0; j < pPaxos->p_Max; j++ ) {
							if( j == MyId )	continue;
							pserver = &pPaxos->p_aServer[j];

							LOG( pPaxos->p_pLog, LogINF, 
								"j=%d =%u a_BackPage=%u a_NextDecide=%u", 
								pm->m_fr, 
								palive->a_BackPage, palive->a_NextDecide );

							switch( CheckConnection( pPaxos, pserver ) ) {
							case DIS_CON:
							case CON_CON:
								if( CMP( pserver->s_BackPage, 
										TO_PAGE(NextDecide) ) <= 0 ) {
									Catchup = TRUE;
									goto next;
								}
								break;
							default:
								break;
							}
						}
					next:
						if( !Catchup ) {
							LOG( pPaxos->p_pLog, LogSYS, 
								"=== CAN'T CATCHUP Page=%u ===", Page );
							(pPaxos->p_fNotify)( pPaxos, 
								PAXOS_MODE_CATCHUP, PAXOS_NOT_GET_PAGE, Page, 
								0, Epoch, 0, NULL, MyId );
							ABORT();
						}
					}
				}
				MtxUnlock( &pPaxos->p_Lock );
			}
			break;
		case DIS_DIS:
			break;
	}
	set_reader( pPaxos );
err:
	DBG_EXT( ret );
	return( ret );
}

int	
server_receive_ras( Paxos_t* pPaxos, msg_t* pm )
{
	Server_t*		pserver = &pPaxos->p_aServer[pm->m_fr];

	LOG( pPaxos->p_pLog, LogINF, "ras=%d", pm->m_fr );

	memset( &pserver->s_Token, 0, sizeof(pserver->s_Token) );

	CheckConnection( pPaxos, pserver );
	ASSERT( !(pserver->s_status & PAXOS_SERVER_CONNECTED) );

	set_reader( pPaxos );
	set_leader( pPaxos, pserver );

	return( 0 );
}

int	
server_receive_test_command( Paxos_t* pPaxos, msg_t* pm )
{
	msg_test_command_t*	pCommand = (msg_test_command_t*)pm;
	MetaCmd_t		command;
	int	k;

	DBG_ENT();

	k = pCommand->c_k;
	if( k >= PAXOS_MAX )	goto err1;

	if( pCommand->c_cmd.p_Len > PAXOS_COMMAND_SIZE )	goto err1;
	

	command.c_len	= pCommand->c_cmd.p_Len;
	memcpy( command.c_opaque, &pCommand->c_cmd, command.c_len );

	if( *((char*)(pCommand+1)) == 'V' ) {
		InitCommand( pPaxos, k, &command, VALUE_VALIDATE );
	} else {
		InitCommand( pPaxos, k, &command, 0 );
	}

	DBG_EXT( 0 );
	return( 0 );
err1:
	DBG_EXT( -1 );
	return( -1 );
}

int	
server_receive_test_newround( Paxos_t* pPaxos, msg_t* pm )
{
//	msg_test_newround_t*	pI = (msg_test_newround_t*)pm;

	DBG_ENT();

	AMode	= MODE_NONE;
	NewRound( pPaxos );

	RETURN( 0 );
}

int	
server_receive_test_begin( Paxos_t* pPaxos, msg_t* pm )
{
	msg_test_begincast_t*	pI = (msg_test_begincast_t*)pm;
	int	k;

	DBG_ENT();

	k = pI->b_k;
	if( k >= PAXOS_MAX )	goto err1;

	if( Continue( pPaxos, k ) )	BeginCast( pPaxos, k );

	DBG_EXT( 0 );
	return( 0 );
err1:
	DBG_EXT( -1 );
	return( -1 );
}

int	
server_receive_test_suppress( Paxos_t* pPaxos, msg_t* pm )
{
	msg_catchup_req_t*	pS = (msg_catchup_req_t*)pm;

	DBG_ENT();

	if( pS->c_RecvLastDT ) {
		TIMESPEC( pPaxos->p_CatchupRecvLast );
		TimespecAddUsec( pPaxos->p_CatchupRecvLast, pS->c_RecvLastDT );
	} else {
		SetTimespecZero( pPaxos->p_CatchupRecvLast );
	}

	RETURN( 0 );
}

int	
server_receive_collect( Paxos_t* pPaxos, msg_t* pm )
{
	msg_collect_t*	pCollect = (msg_collect_t*)pm;

	DBG_ENT();

	RecvCollect( pPaxos, pCollect );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_last( Paxos_t* pPaxos, msg_t* pm )
{
	msg_last_t*		pLast = (msg_last_t*)pm;

	DBG_ENT();

	GatherLast( pPaxos, pLast );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_old( Paxos_t* pPaxos, msg_t* pm )
{
	msg_oldround_t*		pOld = (msg_oldround_t*)pm;

	DBG_ENT();

	GatherOldRound( pPaxos, pOld );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_begin( Paxos_t* pPaxos, msg_t* pm )
{
	msg_begin_t*	pBegin = (msg_begin_t*)pm;

	DBG_ENT();

	RecvBegin( pPaxos, pBegin );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_begin_redirect( Paxos_t* pPaxos, msg_t* pm )
{
	msg_begin_t*	pBegin = (msg_begin_t*)pm;

	DBG_ENT();

	BeginCastRedirect( pPaxos, pBegin );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_accept( Paxos_t *pPaxos, msg_t* pm )
{
	msg_accept_t*	pAccept = (msg_accept_t*)pm;

	DBG_ENT();

	GatherAccept( pPaxos, pAccept );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_success( Paxos_t* pPaxos, msg_t* pm )
{
	msg_success_t*	pSuccess = (msg_success_t*)pm;

	DBG_ENT();

	RecvSuccess( pPaxos, pSuccess );

	DBG_EXT( 0 );
	return( 0 );
}

int	
server_receive_catchup_req( Paxos_t* pPaxos, msg_t *pm)
{
	Server_t*		pserver = &pPaxos->p_aServer[pm->m_fr];
	msg_catchup_req_t*	pReq = (msg_catchup_req_t*)pm;
	PaxosMulti_t*	pMulti;
	timespec_t		Last;
	uint32_t	page;

	uint32_t	seq;
	bool_t		found = FALSE;
	int	Ret;

	seq	= pReq->c_SeqFrom;
	do {
		page	= TO_PAGE( seq );

		pMulti	= GetPage( pPaxos, page );
		if( pMulti ) {

			Ret = CatchupInstanceRpl( pPaxos, pserver, pMulti, seq );
			if( Ret >= 0 ) {
				found	= TRUE;
			} else {
				LOG( pPaxos->p_pLog, LogINF,
				"NOT DEFINED seq: j=%d seq=%d page=%d Page=%d BackPage=%d", 
					pm->m_fr, seq, page, Page, BackPage );

				CatchupInstanceRpl( pPaxos, pserver, NULL, seq );
			}
		} else {
	LOG( pPaxos->p_pLog, LogDBG, 
		"PAGE REMOVED j=%d seq=%d page=%d Page=%d BackPage=%d", 
		pm->m_fr, seq, page, Page, BackPage );
			CatchupInstanceRpl( pPaxos, pserver, NULL, seq );
		}
	} while( CMP( ++seq, pReq->c_SeqTo ) <= 0 );

	if( found ) {

		pPaxos->p_CatchupRecvServer	= pm->m_fr;

		TIMESPEC( Last );
		TimespecAddUsec( Last, pReq->c_RecvLastDT );

		if( TIMESPECCOMPARE( Last, pPaxos->p_CatchupRecvLast ) > 0 ) {
			LOG( pPaxos->p_pLog, LogINF,
					"=== Catchup START j=%d seq=%d page=%d Page=%d", 
					pm->m_fr, seq, page, Page );
			pPaxos->p_CatchupRecvLast	= Last;
		}
	}

	return( 0 );
}

int	
server_receive_catchup_rpl( Paxos_t* pPaxos, msg_t *pm)
{
	Server_t*		pserver = &pPaxos->p_aServer[pm->m_fr];
	msg_catchup_rpl_t*	pRpl =(msg_catchup_rpl_t*)pm;
#ifdef	YYY
	int		j = pm->m_fr;
#else
	int		j;
	int		fr = pm->m_fr;
#endif
	uint32_t	page;
	int			k;
	value_t*	pRndValue;
	MetaCmd_t*	pC;
	msg_catchup_end_t*	pme;

	if( !pRpl->c_True ) {
/*
 *	BackPageが更新されているかもしれない
 *	自身が決定済みでない時にABORT
 */
		LOG( pPaxos->p_pLog, LogSYS,
		"=== CAN'T CATCHUP from j=%d seq=%u NextDecide=%u ===", 
				pm->m_fr, pRpl->c_Seq, NextDecide );

/*
 *	応答が遅い場合に他サーバに要求されているかも
 */
		for( j = 0; j < pPaxos->p_Maximum; j++ ) {
			if( j == fr )	continue;
			pserver = &pPaxos->p_aServer[j];
			if( !IsTimespecZero( pserver->s_IssuedTime ) ) {
				LOG( pPaxos->p_pLog, LogINF,
					"===  CATCHUP requsted td j=%d ===", j );
				return( 0 );
			}
		}
		if( CMP( NextDecide, pRpl->c_Seq ) <= 0 ) {

			(pPaxos->p_fNotify)( pPaxos, 
				PAXOS_MODE_CATCHUP, PAXOS_NOT_GET_PAGE, pRpl->c_Seq, 
				0, Epoch, 0, NULL, MyId );
			ABORT();
		}
	}
	page	= TO_PAGE( pRpl->c_Seq );
	k		= TO_K( pRpl->c_Seq );
	pRndValue 	= &pRpl->c_PaxosI.p_RndValue;
	pC 		= &pRpl->c_MetaCmd;

DBG_TRC("j=%d MyNextDecide=%u Seq=%d page=%u k=%d /\
 Page=%u NextK=%d NextDecide=%u CatchupNextDecide=%u\n",
j, pRpl->c_MyNextDecide, pRpl->c_Seq,  page, k, 
Page, NextK, NextDecide, pPaxos->p_CatchupNextDecide );

	if( Page == page  ) {

		pPaxos->p_CatchupPageLast = 0;

		if( PMode( k ) != MODE_DECIDED ) {
			/* OutOfBand Data */
			if( PaxosValidate( pPaxos, PAXOS_MODE_CATCHUP, 
							Seq( k ), pRndValue, pC ) ) {
	LOG( pPaxos->p_pLog, LogDBG, "VALIDATE ERROR Seq(k)=%u", Seq( k ) );
				return( 0 );
			}
			PMode( k )				= MODE_DECIDED;

			RndValue( k )			= *pRndValue;
			RndValue( k ).v_From	= MyId;
			RndValue( k ).v_Flags	|= VALUE_CATCHUP;
			MetaCmdRnd( k )			= pRpl->c_MetaCmd;

			Exec( pPaxos,  k );
		}
		else {
DBG_TRC("DECIDED Seq=%u\n", Seq(k) );
		}
	}
	else {
DBG_TRC("NOT j=%d Page=%u page=%u c_Seq=%u\n", j, Page, page, pRpl->c_Seq );
	}
DBG_TRC("Page=%d NextK=%d NextDecide=%u\n", Page, NextK, NextDecide );
DBG_TRC("seq=%u CatchupNextDecide=%u\n", pRpl->c_Seq, pPaxos->p_CatchupNextDecide );

	if( CMP( pRpl->c_Seq, pPaxos->p_CatchupNextDecide - 1 ) >= 0 ) {

		pPaxos->p_IssuedLast = 0;

		for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {

			pserver	= &pPaxos->p_aServer[j];
			if( IsTimespecZero( pserver->s_IssuedTime ) )	continue;

			SetTimespecZero( pserver->s_IssuedTime );

	LOG( pPaxos->p_pLog, LogDBG,
		 "##### SEND MSG_CATCHUP_END j=%d Seq=%u ######", j, Seq( NextK ) );
			pme = (msg_catchup_end_t*)msg_alloc( pPaxos,
							j, MSG_CATCHUP_END, sizeof(*pme) );
			PaxosSend( pPaxos, pserver, (msg_t*)pme );
		}
	} else if( pPaxos->p_IssuedLast
			&&  CMP( pRpl->c_Seq, pPaxos->p_IssuedSeq ) == 0 ) {
DBG_TRC("CONTINUE-3 NextDecide=%u, CtachupNextSeq=%u IssuedSeq=%u\n",
	NextDecide, pPaxos->p_CatchupNextDecide, pPaxos->p_IssuedSeq );
		CatchupFire( pPaxos );
	}
	else {
DBG_TRC("NO ACTION NextDecide=%u, CtachupNextSeq=%u IssuedSeq=%u\n",
	NextDecide, pPaxos->p_CatchupNextDecide, pPaxos->p_IssuedSeq );
	}
	return( 0 );
}

static	int
release_suppressed_accept( Paxos_t *pPaxos )
{
	int	k;
	Server_t*		pserver;
	msg_accept_t*	pma;

	SetTimespecZero( pPaxos->p_CatchupRecvLast );

	for( k = 0; k < PAXOS_MAX; k++ ) {

		if( PMode( k ) != MODE_DECIDED && Suppressed( k ) ) {

			pserver = &pPaxos->p_aServer[RndBallot( k ).r_id];

			pma	= (msg_accept_t*)msg_alloc( pPaxos,
							pserver->s_ID, MSG_ACCEPT, sizeof(*pma) );

			pma->a_Page		= Page;
			pma->a_k		= k;
			pma->a_round	= RndBallot( k );
			pma->a_Value	= RndValue( k );
			pma->a_MetaCmd	= MetaCmdRnd( k );

			Suppressed( k )	= FALSE;
	LOG( pPaxos->p_pLog, LogDBG,
	"### SEND SUPPRESSED k=%d Seq(k)=%u to=%d-%u fr=%d", 
	k, Seq(k), RndBallot(k).r_id, RndBallot(k).r_seq, pma->a_head.m_fr );

			PaxosSend( pPaxos, pserver, (msg_t*)pma );
		}
	}
	return( 0 );
}

int	
server_receive_catchup_end( Paxos_t* pPaxos, msg_t *pm)
{
	if( pm->m_fr == pPaxos->p_CatchupRecvServer ) {

		LOG( pPaxos->p_pLog, LogINF, "#### RELEASED j=%d ####", pm->m_fr );

		release_suppressed_accept( pPaxos );
	}
	return( 0 );
}

int	
server_receive_recv_stop( Paxos_t* pPaxos, msg_t *pm)
{
	msg_recv_stop_t	*pStop	= (msg_recv_stop_t*)pm;

	pPaxos->p_RecvStop[pStop->s_Cmd]	= pStop->s_OnOff;
	return( 0 );
}

int	
server_receive_test_catchup( Paxos_t* pPaxos, msg_t *pm)
{
	msg_catchup_t	*pCatchup	= (msg_catchup_t*)pm;

	pPaxos->p_CatchupStop	= pCatchup->c_OnOff;
	return( 0 );
}

int	
server_receive_test_remove_page( Paxos_t* pPaxos, msg_t *pm)
{
	PaxosMulti_t	*pMulti;

	while( (pMulti = list_last_entry(&pPaxos->p_MultiLnk, 
									PaxosMulti_t, m_Lnk)) ) {

		list_del( &pMulti->m_Lnk );
		(pPaxos->p_fFree)( pMulti );
	}
	BackPage	= Page;
	return( 0 );
}

#ifdef	WWW
int	
server_receive_high_water( Paxos_t* pPaxos, msg_t *pm)
{
	int	j;
/***
	msg_t	*pma;
	Server_t*		pserver;
***/

	j = pm->m_fr;

	quorum_add( &pPaxos->p_WaterReplica, j );

	LOG( pPaxos->p_pLog, LogINF, "j=%d", j );

/***
	pserver	= &pPaxos->p_aServer[j];
	pma = msg_alloc( pPaxos, j, MSG_HIGH_WATER_ACK, sizeof(*pma) );
	ChSend( &pserver->s_ChSend, pma, m_dlnk );
***/
	return( 0 );
}

int	
server_receive_high_water_ack( Paxos_t* pPaxos, msg_t *pm)
{
/***
	int	j;

	j = pm->m_fr;

	pPaxos->p_WaterLeader[j]	= WATER_OFF;

	LOG( pPaxos->p_pLog, "j=%d state=%d", j, pPaxos->p_WaterLeader[j] );
***/

	return( 0 );
}

int	
server_receive_low_water( Paxos_t* pPaxos, msg_t *pm)
{
	int	j;
	msg_t	*pma;
	Server_t*		pserver;

	j = pm->m_fr;

	quorum_del( &pPaxos->p_WaterReplica, j );

	LOG( pPaxos->p_pLog, LogINF, "j=%d", j );

	CndBroadcast( &pPaxos->p_Cnd );

	pserver	= &pPaxos->p_aServer[j];
	pma = msg_alloc( pPaxos, j, MSG_LOW_WATER_ACK, sizeof(*pma) );

	PaxosSend( pPaxos, pserver, pma );

	return( 0 );
}

int	
server_receive_low_water_ack( Paxos_t* pPaxos, msg_t *pm)
{
	int	j;

	j = pm->m_fr;

	switch( pPaxos->p_WaterLeader[j] ) {
		case WATER_LOW_ISSUED:
			pPaxos->p_WaterLeader[j] = WATER_LOW;
			break;
		default:;
	}
	LOG( pPaxos->p_pLog, LogINF, "j=%d state=%d", j, pPaxos->p_WaterLeader[j] );

	return( 0 );
}
#endif	// WWW

int	
server_receive_is_active( Paxos_t* pPaxos, msg_t *pm )
{
	msg_active_rpl_t	*pRpl;

	pRpl	= (msg_active_rpl_t*)msg_alloc( pPaxos,
							-1, MSG_IS_ACTIVE, sizeof(*pRpl) );
	pRpl->a_Active.a_Target		= MyId;
	pRpl->a_Active.a_Max		= pPaxos->p_Max;
	pRpl->a_Active.a_Maximum	= pPaxos->p_Maximum;
	pRpl->a_Active.a_Leader		= ELeader;
	pRpl->a_Active.a_Epoch		= Epoch;
	pRpl->a_Active.a_NextDecide	= NextDecide;
	pRpl->a_Active.a_BackPage	= BackPage;

	if( pPaxos->p_fLoad ) {
		pRpl->a_Active.a_Load	= (pPaxos->p_fLoad)(pPaxos->p_pTag);
	} else {
		pRpl->a_Active.a_Load	= -1;
	}

	PaxosSendTo( pPaxos, pRpl, sizeof(*pRpl), 0,
						&pm->m_From, sizeof(pm->m_From) );
	return( 0 );
}

typedef	int(*receive_func_t)(struct Paxos*, msg_t* pm);
typedef	struct receive {
	receive_func_t	r_receive;
} receive_t;

receive_t	dispatch[] = {
/* MSG_DUMP */	 			{ server_receive_dump },
/* MSG_CONTROL_START */		{ server_receive_control_start },
/* MSG_CONTROL_STOP */ 		{ NULL/*server_receive_control_stop*/ },
/* MSG_OUTBOUND_STOP */ 	{ server_receive_stop },
/* MSG_OUTBOUND_RESTART */ 	{ server_receive_restart },
/* MSG_DOWN	 */ 			{ server_receive_down },
/* MSG_ALIVE */				{ server_receive_alive },
/* MSG_CATCHUP_REQ */		{ server_receive_catchup_req },
/* MSG_CATCHUP_RPL */		{ server_receive_catchup_rpl },
/* MSG_COLLECT */			{ server_receive_collect },
/* MSG_LAST */				{ server_receive_last },
/* MSG_OLDROUND */			{ server_receive_old },
/* MSG_BEGIN */				{ server_receive_begin },
/* MSG_ACCEPT */			{ server_receive_accept },
/* MSG_SUCCESS */			{ server_receive_success },
/* MSG_RECV_STOP */			{ server_receive_recv_stop },
/* MSG_BEGIN_REDIRECT */	{ server_receive_begin_redirect },
/* MSG_BIRTH */				{ server_receive_birth },
/* MSG_CATCHUP_END */		{ server_receive_catchup_end },
/* MSG_TEST_COMMAND */		{ server_receive_test_command },
/* MSG_TEST_NEWROUND */		{ server_receive_test_newround },
/* MSG_TEST_BEGIN */		{ server_receive_test_begin },
/* MSG_TEST_SUPPRESS */		{ server_receive_test_suppress },
/* MSG_TEST_CATCHUP */		{ server_receive_test_catchup },
/* MSG_TEST_REMOVE_PAGE */	{ server_receive_test_remove_page },
/* MSG_RAS */				{ server_receive_ras },
#ifdef	WWW
/* MSG_HIGH_WATER */		{ server_receive_high_water },
/* MSG_HIGH_WATER_ACK */	{ server_receive_high_water_ack },
/* MSG_LOW_WATER */			{ server_receive_low_water },
/* MSG_LOW_WATER_ACK */		{ server_receive_low_water_ack },
#else
/* MSG_HIGH_WATER */		{ NULL },
/* MSG_HIGH_WATER_ACK */	{ NULL },
/* MSG_LOW_WATER */			{ NULL },
/* MSG_LOW_WATER_ACK */		{ NULL },
#endif
/* MSG_IS_ACTIVE */			{ server_receive_is_active },
};

int
PaxosRecvFrom( int RecvFd, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t *pAddrLen )
{
	int	Ret;
	int	i = 0;
	char	dummy[1024];
again:
	Ret = recvfrom( RecvFd,p,len,flag,(struct sockaddr*)pAddr,pAddrLen);
	if( Ret < 0 && errno == ECONNRESET ) {
		//DBG_PRT("port=%d len=%d flag=0x%x *pAddrLen=%d Ret=%d\n", 
		//	ntohs(pAddr->sin_port), (int)len, flag, (int)*pAddrLen, Ret );
		recvfrom( RecvFd, dummy, sizeof(dummy), 0,
					(struct sockaddr*)pAddr,pAddrLen);

		if( i++ > 5 )	goto next;
		goto again;
	}
next:
	return( Ret );
}

int
PaxosSendTo( Paxos_t *pPaxos, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t AddrLen )
{
	int	Ret;

	DBG_TRC("To[%s:%d]\n", 
		inet_ntoa(pAddr->sin_addr), ntohs(pAddr->sin_port) );

	if( *(int*)&pAddr->sin_addr ) {
		Ret = sendto( pPaxos->p_SendFd, p, len, 
						flag, (struct sockaddr*)pAddr, AddrLen );
	} else {
		Ret = WriteStream( pPaxos->p_AdminFd, p, len );
	}
	if( Ret < 0 && errno == ECONNRESET ) {
		LOG(pPaxos->p_pLog, LogERR,
			"port=%d len=%d flag=0x%x AddrLen=%d Ret=%d\n", 
			ntohs(pAddr->sin_port),(int)len, flag, AddrLen, Ret );
	}
	return( Ret );
}

/*
 * ハンドラースレッドは一人
 */
int
PaxosRecvHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	Paxos_t*	pPaxos = (Paxos_t*)pEv->fd_pArg;
	msg_t*	pm;
	struct sockaddr_in	from;
	int		err;
	socklen_t	len = sizeof(from);
	int		s;
	int		cmd;
	msg_t	W;
//	bool_t	bLock	= FALSE;

	DBG_ENT();

	if( Mode != EV_LOOP )	return( 0 );

	if( Active == FALSE ) {
		DBG_EXT(-1);
		return( -1 );
	}
	memset( &W, 0, sizeof(W) );
	memset( &from, 0, sizeof(from) );

	if( pEv->fd_Type == SOCK_DGRAM ) {
		err = PaxosRecvFrom( pEv->fd_Fd, &W, sizeof(W), MSG_PEEK, 
													&from, &len );
	} else {
		err = ReadStream( pEv->fd_Fd, &W, sizeof(W) );
	}
	if( err <= 0 ) goto err1;
	ASSERT( err == sizeof(W) );

	s = W.m_len;
	pm = (msg_t*)PaxosMalloc( pPaxos, s );
	if( !pm ) {
		err = -1;
		goto err1;
	}
	if( pEv->fd_Type == SOCK_DGRAM ) {
		len	= sizeof(from);
		err = PaxosRecvFrom( pEv->fd_Fd, (char*)pm, s, 0, &from, &len );
	} else {
		*pm	= W;
		if( s - sizeof(W) > 0 ) {
			err = ReadStream( pEv->fd_Fd, pm+1, s-sizeof(W) );
		}
	}
	if( err < 0 )	goto err2;

	if( pm->m_cksum64 ) {
		memset( &pm->m_dlnk, 0, sizeof(pm->m_dlnk) );
		ASSERT( in_cksum64( pm, pm->m_len, 0 ) == 0 );
	}
	list_init( &pm->m_dlnk );

	cmd	= pm->m_cmd;

	if( pPaxos->p_RecvStop[cmd] ) {
		PaxosFree( pPaxos, pm );
		DBG_EXT( 0 );
		return( 0 );
	}

	switch( cmd ) {
	case MSG_LOCK:
		DBG_TRC("MSG_LOCK\n");
		MtxTrylock( &pPaxos->p_LockElection );
		MtxTrylock( &pPaxos->p_Lock );
		PaxosFree( pPaxos, pm );
		break;
	case MSG_UNLOCK:
		DBG_TRC("MSG_UNLOCK\n");
		MtxUnlock( &pPaxos->p_LockElection );
		MtxUnlock( &pPaxos->p_Lock );
		PaxosFree( pPaxos, pm );
		break;
	case MSG_CONTROL_START:
	case MSG_CONTROL_STOP:
	case MSG_DUMP:
		(dispatch[pm->m_cmd].r_receive)( pPaxos, pm );
		PaxosFree( pPaxos, pm );
		break;

	case MSG_LOG_DUMP:
		DBG_TRC("MSG_LOG_DUMP\n");
		LogDump( pPaxos->p_pLog );
		PaxosFree( pPaxos, pm );
		break;

	case MSG_RAS:
		LOG( pPaxos->p_pLog, LogINF, "MSG_RAS" );
	case MSG_ALIVE:
		DBG_TRC("MSG_ALIVE/MSG_RAS\n");
		QueuePostEntry( &pPaxos->p_QueueHeartbeat, pm, m_dlnk );	
		break;
	default:
		if( QueuePostEntry( &pPaxos->p_QueuePaxos, pm, m_dlnk ) < 0 ) {
			LOG( pPaxos->p_pLog, LogERR, "Discard MSG(%d): QueueCnt=%d", pm->m_cmd, QueueCnt( &pPaxos->p_QueuePaxos) );
			PaxosFree( pPaxos, pm );
		}
		break;
	}
	DBG_EXT( 0 );
	return( 0 );
err2:
	PaxosFree( pPaxos, pm );
err1:
	FdEventDel( pEv );
	close( pEv->fd_Fd );
	if( pPaxos->p_AdminFd == pEv->fd_Fd )	pPaxos->p_AdminFd = 0;	
	MtxUnlock( &pPaxos->p_LockElection );
	MtxUnlock( &pPaxos->p_Lock );
	PaxosFree( pPaxos, pEv );
	DBG_EXT( err );
	return( 0 );
}

int
PaxosAcceptHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	Paxos_t*	pPaxos = (Paxos_t*)pEv->fd_pArg;
	int	AcceptFd;
	FdEvent_t	*pEv1;
	int	Ret;

	DBG_ENT();

	if( Mode != EV_LOOP )	return( 0 );

	AcceptFd	= accept( pEv->fd_Fd, NULL, NULL );
	if( AcceptFd < 0 )	goto err;
	if( pPaxos->p_AdminFd > 0 ) {
		LOG( pPaxos->p_pLog, LogINF, "BUSY:Fd=%d", pPaxos->p_AdminFd );
		close( AcceptFd );
		goto err;
	} else {
		pPaxos->p_AdminFd	= AcceptFd;
	}
	pEv1	= (FdEvent_t*)PaxosMalloc( pPaxos, sizeof(*pEv1) );
	FdEventInit( pEv1, SOCK_STREAM, AcceptFd, 
					FD_EVENT_READ, pPaxos, PaxosRecvHandler );
	Ret = FdEventAdd( &pPaxos->p_FdEvCt, AcceptFd, pEv1 );
	if( Ret < 0 )	goto err1;

	DBG_EXT( 0 );
	return( 0 );
err1:
	pPaxos->p_AdminFd	= -1;
	PaxosFree( pPaxos, pEv1 );
err:
	DBG_EXT( -1 );
	return( 0 );
}

/*
 *	Detector time
 */
int
PaxosDetector( void* arg )
{
	Paxos_t*	pPaxos = (Paxos_t*)arg;
	Server_t	*pserver;
	int	j;

	DBG_ENT();

	MtxLock( &pPaxos->p_Lock );

	CheckRndSuccessAndNewRound( pPaxos );

	if( pPaxos->p_IssuedLast && pPaxos->p_IssuedLast < time(0) ) {
		pPaxos->p_IssuedLast	= 0;
		for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
			pserver	= &pPaxos->p_aServer[j];
			SetTimespecZero( pserver->s_IssuedTime );
		}
	}

	timespec_t	Now;
	TIMESPEC( Now );
	if( !IsTimespecZero( pPaxos->p_CatchupRecvLast )
		&& TIMESPECCOMPARE(pPaxos->p_CatchupRecvLast, Now ) < 0 ) {

		LOG( pPaxos->p_pLog, LogINF, "Timeout=>OK");
		release_suppressed_accept( pPaxos );
	}

	MtxUnlock( &pPaxos->p_Lock );

	RETURN( 0 );
}

void*
ThreadReceiverPaxos( void* pa )
{
	Paxos_t	*pPaxos	= (Paxos_t*)pa;
	msg_t	*pM = NULL;

	while( TRUE ) {
		if( Active == FALSE )	pthread_exit( (void*)0 );
		
		QueueWaitEntry( &pPaxos->p_QueuePaxos, pM, m_dlnk );

		MtxLock( &pPaxos->p_Lock );

		(dispatch[pM->m_cmd].r_receive)( pPaxos, pM );

		MtxUnlock( &pPaxos->p_Lock );

		PaxosFree( pPaxos, pM );
	}
}

void*
ThreadReceiverHeartbeat( void* pa )
{
	Paxos_t	*pPaxos	= (Paxos_t*)pa;
	msg_t	*pM = NULL;

	while( TRUE ) {
		if( Active == FALSE )	pthread_exit( (void*)0 );
		
		QueueWaitEntry( &pPaxos->p_QueueHeartbeat, pM, m_dlnk );
DBG_TRC("RECEIVED HEARTBEAT fr[%d]\n", pM->m_fr );

		MtxLock( &pPaxos->p_LockElection );

		(dispatch[pM->m_cmd].r_receive)( pPaxos, pM );

		MtxUnlock( &pPaxos->p_LockElection );

		PaxosFree( pPaxos, pM );
	}
}

int
ChSendTo( ChSend_t* pC, void* p )
{
	Paxos_t*	pPaxos = (Paxos_t*)pC->c_pVoid;
	msg_t*		pM	= (msg_t*)p;
	int	Ret;

	Ret = PaxosSendTo( pPaxos, (void*)pM, pM->m_len, 0,
						&pC->c_PeerAddr, sizeof(pC->c_PeerAddr) );
	return( Ret );
}

#ifdef	_SENDER_
void*
ThreadSender( void* pa )
{
	ChSend_t*	pc = (ChSend_t*)pa;

	while( TRUE ) {
		if( isChOutboundDown( pc ) )	pthread_exit( (void*)0 );
		ChOutbound( pc, m_dlnk );
	}
}
#endif

void*
ThreadClock( void* pa )
{
	Paxos_t	*pPaxos	= (Paxos_t*)pa;

	while( TRUE ) {
		if( Active == FALSE )	pthread_exit( (void*)0 );
		PaxosDetector( pPaxos );
		usleep( DT_C );
	}
}

void*
ThreadHeartbeat( void* pa )
{
	Paxos_t	*pPaxos	= (Paxos_t*)pa;
	Server_t*	pserver;
	int	j;

	while( TRUE ) {
		if( Active == FALSE )	pthread_exit( (void*)0 );

		MtxLock( &pPaxos->p_LockElection );

		TIMESPEC( pPaxos->p_aServer[MyId].s_Alive );

		for( j = 0; j < pPaxos->p_Maximum; j++ ) {
			if( j == MyId )	continue;

			pserver = &pPaxos->p_aServer[j];
			switch( CheckConnection( pPaxos, pserver ) ) {
				case DIS_CON:
					/* masterの選び直し */
					set_leader( pPaxos, pserver );
					break;
				case CON_DIS:
					/* masterの選び直し */
					set_leader( pPaxos, pserver );

					/* Catchup中を解除する */
					//MtxLock( &pPaxos->p_Lock );
					if( !IsTimespecZero(pserver->s_IssuedTime) ) {
	LOG( pPaxos->p_pLog, LogDBG,
	"REMOVE CATCHUP j=%d", pserver->s_ID );
						pPaxos->p_IssuedLast	= 0;
						SetTimespecZero( pserver->s_IssuedTime );
					}
					//MtxUnlock( &pPaxos->p_Lock );
#ifdef	WWW
					pPaxos->p_WaterLeader[pserver->s_ID] = WATER_LOW;
					quorum_del( &pPaxos->p_WaterReplica, pserver->s_ID );
					LOG( pPaxos->p_pLog, "Water=%d", pserver->s_ID ); 
#endif
					break;
				case CON_CON:
				case DIS_DIS:
					break;
			}
		}
		for( j = 0; j < pPaxos->p_Maximum; j++ ) {
			if( j == MyId )	continue;

			pserver = &pPaxos->p_aServer[j];
			if( pserver->s_status & PAXOS_SERVER_ACTIVE )	{
				SendAlive( pPaxos, pserver );
			}
		}
/* for performance test */
if( pPaxos->p_Max/*PAXOS_SERVER_MAX*/ == 1 )	IamMaster = TRUE;

		MtxUnlock( &pPaxos->p_LockElection );

		usleep( DT_Z );
	}
}

/*
 *	API
 */
int
PaxosInitUsec( Paxos_t *pPaxos, int Id, 
		int	ServerCore, int ServerExtension,
		PaxosCell_t* pCell,
		int64_t UnitUsec, bool_t bCksum,
		int(*fElection)(Paxos_t*,PaxosNotify_t,int,uint32_t,int32_t),
		int(*fNotify)(Paxos_t*, PaxosMode_t,PaxosNotify_t,
							uint32_t,int,uint32_t,int32_t,void*,int),

		int(*fOutOfBandMarshal)(Paxos_t*,void*,IOReadWrite_t*),
		int(*fOutOfBandUnmarshal)(Paxos_t*,void*,IOReadWrite_t*),

		void*(*fMalloc)(size_t),
		void(*fFree)(void*),
		int(*fLoad)(void*) )
{
	int	j, k;

	memset( pPaxos, 0, sizeof(*pPaxos) );

	if( !fMalloc )	fMalloc = Malloc;
	if( !fFree )	fFree = Free;

	if( UnitUsec < DT_UNIT_MIN )	UnitUsec	= DT_UNIT_MIN;

	memset( pPaxos, 0, sizeof(*pPaxos) );
	pPaxos->p_MyId	= Id;
	TIMESPEC( EBirth );
	ELeader					= -1;
	pPaxos->p_UnitUsec		= UnitUsec;
	pPaxos->p_bCksum		= bCksum;
	pPaxos->p_fElection		= fElection;
	pPaxos->p_fNotify		= fNotify;

	pPaxos->p_fOutOfBandMarshal		= fOutOfBandMarshal;
	pPaxos->p_fOutOfBandUnmarshal	= fOutOfBandUnmarshal;

	pPaxos->p_fMalloc		= fMalloc;
	pPaxos->p_fFree			= fFree;
	pPaxos->p_fLoad			= fLoad;

	MtxInit( &pPaxos->p_Lock );
	CndInit( &pPaxos->p_Cnd );
	MtxInit( &pPaxos->p_LockElection );
	QueueInit( &pPaxos->p_QueuePaxos );
	QueueInit( &pPaxos->p_QueueHeartbeat );
	QueueInit( &pPaxos->p_Outbound );

	strncpy( pPaxos->p_CellName, pCell->c_Name, sizeof(pPaxos->p_CellName) );
	pPaxos->p_Max		= ServerCore;
	pPaxos->p_Maximum	= ServerCore + ServerExtension;

	pPaxos->p_aServer	= (Server_t*)(*fMalloc)
							( sizeof(Server_t)*pPaxos->p_Maximum );	
	memset( pPaxos->p_aServer, 0, sizeof(Server_t)*pPaxos->p_Maximum );

	for( j = 0; j < pCell->c_Maximum; j++ ) {
		pPaxos->p_aServer[j].s_ID	= j;
		if( pCell->c_aPeerAddr[j].a_Active ) {
			pPaxos->p_aServer[j].s_status |= PAXOS_SERVER_ACTIVE;
			ChSendInit( &pPaxos->p_aServer[j].s_ChSend, pPaxos, 
				pCell->c_aPeerAddr[j].a_Addr, ChSendTo, fMalloc, fFree );
		}
	}
	list_init( &pPaxos->p_MultiLnk );

	quorum_init( &RndInfQuo, pPaxos->p_Max/*PAXOS_SERVER_MAX*/ );
	for( k = 0; k < PAXOS_MAX; k++ ) {
		quorum_init( &RndAccQuo(k), pPaxos->p_Max/*PAXOS_SERVER_MAX*/ );
	}

	HashInit( &pPaxos->p_Gaps, PRIMARY_11, HASH_CODE_INT, HASH_CMP_INT, 
				NULL, fMalloc, fFree, NULL );

	Elected	= -1;

	FdEventCtrlCreate( &pPaxos->p_FdEvCt );

	return( 0 );
}

int
PaxosMyId( Paxos_t* pPaxos )
{
	return( MyId );
}

bool_t
PaxosIsIamMaster( Paxos_t* pPaxos )
{
	bool_t	Ret;

	/* atomic とする */
	Ret	= IamMaster;
	return( Ret );
}

int
PaxosGetMaster( Paxos_t* pPaxos ) 
{
	int	Ret;

	/* atomic とする */
	Ret = Elected;
	return( Ret );
}

int
_PaxosRequest( Paxos_t* pPaxos, int len, void* pCommand, int Flag )
{
	int Ret = PAXOS_OK;
	int	k;
	MetaCmd_t	PC;

DBG_TRC("ENT len=%d(%d) Flag=%d\n", len, PAXOS_COMMAND_SIZE, Flag);

	if( !Active ) {
		Ret = PAXOS_NOT_ACTIVE;
		goto exit;
	}
	if( IamMaster ) {

		k = GetSlot( pPaxos );
DBG_TRC("GetSlot k=%d\n",k);
		if( k < 0 )	 {
			Ret	= PAXOS_REJECTED;
DBG_TRC("REJECTED\n");
			goto exit;
		}
DBG_TRC("k=%d Seq=%d\n", k, Seq(k) );
		PC.c_len	= len;
		if( len > PAXOS_COMMAND_SIZE ) {
			Ret	= PAXOS_REJECTED;
			goto exit;
		}
		memcpy( PC.c_opaque, pCommand, len );
		if( Flag & VALUE_VALIDATE ) {
			value_t V;

			memset( &V, 0, sizeof(V) );
			V.v_Flags	= Flag;
			V.v_Server	= MyId;
			V.v_Epoch	= Epoch;
			V.v_From	= MyId;

			if( PaxosValidate( pPaxos, PAXOS_MODE_REQUEST, 
					Seq( k ), &V, &PC ) ) {
				Ret	= PAXOS_REJECTED;
				goto exit;
			}
		}

		InitCommand( pPaxos, k, &PC, Flag );
DBG_TRC("InitCommand k=%d\n",k);

		if( Continue( pPaxos, k ) )	BeginCast( pPaxos, k );
		else {
DBG_TRC("Continue ERROR\n");
			Ret	= PAXOS_REJECTED;
			goto exit;
		}
	} else {
		Ret	= PAXOS_Im_NOT_MASTER;
	}
exit:

DBG_TRC("EXT\n");
	return( Ret );
}

int
PaxosRequest( Paxos_t* pPaxos, int len, void* pCommand, bool_t Validate )
{
	int	Flags = 0;
	int	Ret;

	MtxLock( &pPaxos->p_Lock );

	if( Validate )	Flags |= VALUE_VALIDATE;

	Ret = _PaxosRequest( pPaxos, len, pCommand, Flags );

	MtxUnlock( &pPaxos->p_Lock );
	return( Ret );
}

int
PaxosRequestTry( Paxos_t* pPaxos, int len, void* pCommand, bool_t Validate )
{
	int	Flags = 0;
	int	Ret;

	if( MtxTrylock( &pPaxos->p_Lock ) )	return( -1 );

	if( Validate )	Flags |= VALUE_VALIDATE;

	Ret = _PaxosRequest( pPaxos, len, pCommand, Flags );

	MtxUnlock( &pPaxos->p_Lock );
	return( Ret );
}

void*
ThreadFdEventLoopPaxos( void* pa )
{
	Paxos_t*	pPaxos = (Paxos_t*)pa;

	DBG_ENT();

	FdEventLoop( &pPaxos->p_FdEvCt, FOREVER );	// forever

	DBG_EXT( 0 );
	return( NULL );
}

int
PaxosFdEventAdd( Paxos_t *pPaxos, uint64_t Key, FdEvent_t *pEv )
{
	int	Ret;

	DBG_ENT();

	pEv->fd_pArg = pPaxos;
	Ret = FdEventAdd( &pPaxos->p_FdEvCt, Key, pEv  );

	DBG_EXT( Ret );
	return( Ret );
}

int
PaxosFdEventDel( Paxos_t *pPaxos, FdEvent_t *pEv )
{
	int	Ret;

	DBG_ENT();

	Ret = FdEventDel( pEv );

	DBG_EXT( Ret );
	return( Ret );
}

int
PaxosInitPort( Paxos_t *pPaxos )
{
	char	PATH[PAXOS_CELL_NAME_MAX];
	struct sockaddr_in	InAddr;
	int	RecvFd, SendFd;
	int	Ret;
	FdEvent_t	*pEv;

	DBG_ENT();
/*
 *	Open Paxos port and bind it
 */
	if( (RecvFd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		LOG( pPaxos->p_pLog, LogINF, "RecvFd socket ERROR" );
		goto err;
	}
	InAddr	= pPaxos->p_aServer[pPaxos->p_MyId].s_ChSend.c_PeerAddr;
	if( bind( RecvFd, (struct sockaddr*)&InAddr, sizeof(InAddr) ) < 0 ) {
		LOG( pPaxos->p_pLog, LogINF, "RecvFd bind ERROR" );
		goto err1;
	}
	if( (SendFd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		LOG( pPaxos->p_pLog, LogINF, "SendFd socket ERROR" );
		goto err2;
	}
	pPaxos->p_RecvFd		= RecvFd;
	pPaxos->p_SendFd		= SendFd;

	pEv	= (FdEvent_t*)PaxosMalloc( pPaxos, sizeof(*pEv) );
	FdEventInit( pEv, SOCK_DGRAM, RecvFd, 
					FD_EVENT_READ, pPaxos, PaxosRecvHandler );
	Ret = FdEventAdd( &pPaxos->p_FdEvCt, RecvFd, pEv );
	if( Ret < 0 )	goto err2;
/*
 *	Admin port
 */
	struct sockaddr_un	AdmAddr;
	int		LFd;

	sprintf( PATH, PAXOS_ADMIN_PORT_FMT, pPaxos->p_CellName, MyId );
	unlink( PATH );

	memset( &AdmAddr, 0, sizeof(AdmAddr) );
	AdmAddr.sun_family	= AF_UNIX;
	strcpy( AdmAddr.sun_path, PATH );

	LFd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( LFd < 0 )	goto err2;

	if( bind( LFd, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		goto err3;
	}
	listen( LFd, 5 );
	pEv	= (FdEvent_t*)PaxosMalloc( pPaxos, sizeof(*pEv) );
	FdEventInit( pEv, SOCK_STREAM, LFd, 
				FD_EVENT_READ, pPaxos, PaxosAcceptHandler );
	Ret = FdEventAdd( &pPaxos->p_FdEvCt, LFd, pEv );
	if( Ret < 0 )	goto err3;

	DBG_EXT( 0 );
	return( 0 );

err3:	close( LFd );
err2:	close( SendFd );
err1:	close( RecvFd );
		DBG_EXT( -1 );
err:	return( -1 );
}

int
PaxosStart( Paxos_t* pPaxos, PaxosStartMode_t StartMode, uint32_t nextDecide )
{
	int	Ret;	

	MtxLock( &pPaxos->p_Lock );

	Active	= TRUE;

	if( StartMode == PAXOS_WAIT ) {
		pPaxos->p_bSuppressAccept	= TRUE;
	} if( StartMode == PAXOS_SEQ ) {
		NextDecide	= nextDecide;
		Page	= TO_PAGE( nextDecide );
		NextK	= TO_K( nextDecide );
		SuccessCount	= NextK;
	}
LOG( pPaxos->p_pLog, LogDBG, 
	"StartMode=%d NextDecide=%d Page=%d NextK=%d SuccessCount=%d",
	StartMode, NextDecide, Page, NextK, SuccessCount );

	Ret = pthread_create( &pPaxos->p_FdEvCt.ct_Th, NULL, 
						ThreadFdEventLoopPaxos, (void*)pPaxos );
	if( Ret < 0 )	goto err2;

	Ret	= PaxosInitPort( pPaxos );
	if( Ret < 0 )	goto err2;

	Ret = pthread_create( &pPaxos->p_ReceiverPaxos, NULL, 
						ThreadReceiverPaxos, (void*)pPaxos );
	if( Ret < 0 )	goto err3;
	Ret = pthread_create( &pPaxos->p_ReceiverHeartbeat, NULL, 
						ThreadReceiverHeartbeat, (void*)pPaxos );
	if( Ret < 0 )	goto err4;
	Ret = pthread_create( &pPaxos->p_Clock, NULL, 
						ThreadClock, (void*)pPaxos );
	if( Ret < 0 )	goto err5;
	Ret = pthread_create( &pPaxos->p_Heartbeat, NULL, 
						ThreadHeartbeat, (void*)pPaxos );
	if( Ret < 0 )	goto err6;

	MtxUnlock( &pPaxos->p_Lock );
	return( Ret );
err6:
err5:
err4:
err3:
err2:
	Active	= FALSE;
	MtxUnlock( &pPaxos->p_Lock );
	return( Ret );
}

int
PaxosDown( Paxos_t* pPaxos )
{
	int	j;

	MtxLock( &pPaxos->p_Lock );
	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
		ChOutboundDown( &pPaxos->p_aServer[j].s_ChSend );
	}
	Active	= FALSE;
	MtxUnlock( &pPaxos->p_Lock );
	return( 0 );
}

int64_t
PaxosDecidedTimeout( Paxos_t* pPaxos )
{
	return( DT_DECIDE );
}

int64_t
PaxosMasterElectionTimeout( Paxos_t* pPaxos )
{
		return( DT_LEADER );
}

uint32_t
PaxosEpoch( Paxos_t* pPaxos )
{
	return( Epoch );
}

int
PaxosGetSize()
{
	return( sizeof(Paxos_t) );
}

uint32_t
PaxosGetSeq( Paxos_t* pPaxos )
{
	return( Seq( NextK ) );
}

uint32_t
PaxosGetNextDecide( Paxos_t* pPaxos )
{
	return( NextDecide );
}

void
PaxosSetTag( Paxos_t* pPaxos, void* pTag )
{
	pPaxos->p_pTag	= pTag;
}

void*
PaxosGetTag( Paxos_t* pPaxos )
{
	return( pPaxos->p_pTag );
}

int64_t
PaxosUnitUsec( Paxos_t* pPaxos )
{
	return( pPaxos->p_UnitUsec );
}

struct sockaddr_in*
PaxosGetAddr( Paxos_t* pPaxos, int j )
{
	return( &pPaxos->p_aServer[j].s_ChSend.c_PeerAddr );
}

Msg_t*
PaxosRecvFromPaxos( Paxos_t* pPaxos )
{
	Msg_t*	pMsg;

	QueueWaitEntry( &pPaxos->p_Outbound, pMsg, m_Lnk );

	return( pMsg );
}

int
PaxosValidate( Paxos_t* pPaxos, PaxosMode_t mode, 
				uint32_t seq, value_t* pValue, MetaCmd_t* pC )
{
	int				Ret;

DBG_TRC("seq=%u mode=%d v_Flags=0x%x v_From=%d pC->c_len=%d\n", 
	seq, mode, pValue->v_Flags, pValue->v_From, pC->c_len );

	if( !( pValue->v_Flags & VALUE_VALIDATE ) )	return( 0 );
	if( pValue->v_From == MyId 
			&& mode != PAXOS_MODE_RECOVER
			&& mode != PAXOS_MODE_REQUEST )	return( 0 );

	Ret = (pPaxos->p_fNotify)( pPaxos, mode, PAXOS_VALIDATE, 
			seq, pValue->v_Server, pValue->v_Epoch, 
			pC->c_len, pC->c_opaque,
			pValue->v_From );

	if( Ret ) {
		LOG( pPaxos->p_pLog, LogINF,
		"### VALIDATE ERROR seq=%u v_Server=%d v_From=%d", 
			seq, pValue->v_Server, pValue->v_From );
	}
	return( Ret );
}

int
PaxosGetReader( Paxos_t* pPaxos )
{
	return( EReader );
}

int
PaxosFreeze( Paxos_t* pPaxos )
{
#ifdef ZZZ
	/*	Suspend Event */
	pPaxos->p_bFreeze	= TRUE;
	QueueSuspend( &pPaxos->p_QueuePaxos );

 again:
	/*	最初に入り閉塞 */
	MtxLock( &pPaxos->p_Lock );

	if ( pPaxos->p_IssuedLast > 0 ) {
		MtxUnlock( &pPaxos->p_Lock );
		LOG( pPaxos->p_pLog, LogINF,
			"IssuedLast(%u) catchup", pPaxos->p_IssuedLast );
		usleep( 20*1000 );
		goto again;
	}

	/* 消費者が待ちになるまで待つ				*/
	/*  消費者はシリアライズなので一人であり、	*/
	/*	待ちは更新処理終了を意味する 			*/
	while( pPaxos->p_Outbound.q_cnt > 0 || pPaxos->p_Outbound.q_waiter != 1 ) {
		usleep( 20*1000 );
	} 
#else
	pPaxos->p_CatchupStop	= TRUE;

again:
	if ( pPaxos->p_IssuedLast > 0 ) {
		LOG( pPaxos->p_pLog, LogINF,
			"IssuedLast(%u) catchup", pPaxos->p_IssuedLast );
		usleep( 20*1000 );
		goto again;
	}

	/*	Suspend Event */
	pPaxos->p_bFreeze	= TRUE;
	QueueMax( &pPaxos->p_QueuePaxos, 100 );
	QueueSuspend( &pPaxos->p_QueuePaxos );

	while( pPaxos->p_Outbound.q_cnt > 0 || pPaxos->p_Outbound.q_waiter != 1 ) {
		usleep( 20*1000 );
	} 

	MtxLock( &pPaxos->p_Lock );
#endif
	return( 0 );
}

int
PaxosDefreeze( Paxos_t* pPaxos )
{
	pPaxos->p_bFreeze	= FALSE;
	MtxUnlock( &pPaxos->p_Lock );
	/*	Resume Event */
	QueueMax( &pPaxos->p_QueuePaxos, 0 );
	QueueResume( &pPaxos->p_QueuePaxos );
#ifdef ZZZ
#else
	pPaxos->p_CatchupStop	= FALSE;
#endif
	return( 0 );
}

int
PaxosMarshal( Paxos_t* pPaxos, IOReadWrite_t *pW )
{
	int	i, k;
	Hash_t	*pHash;
	Gap_t	*pGap;
	HashItem_t	*pItem;

	/*	Paxos */
	IOMarshal( pW, pPaxos, sizeof(*pPaxos) );

	/* OutOfBand */
	for( k = 0; k < PAXOS_MAX; k++ ) {
		if( RndValue(k).v_Flags & VALUE_VALIDATE ) {

			(pPaxos->p_fOutOfBandMarshal)( pPaxos, MetaCmdRnd(k).c_opaque, pW );
		}
	}

	/* Gaps */
	pHash	= &pPaxos->p_Gaps;

	IOMarshal( pW, &pHash->h_Count, sizeof(pHash->h_Count) );

	for( i = 0; i < pHash->h_N; i++ ) {
		if( !list_empty( &pHash->h_aHeads[i] ) ) {
			list_for_each_entry( pItem, &pHash->h_aHeads[i], i_Link ) {
				pGap = (Gap_t*)pItem->i_pValue;

				IOMarshal( pW, pGap, sizeof(*pGap) );
			}
		}
	}
	return( 0 );
}

int
PaxosUnmarshal( Paxos_t* pPaxos, IOReadWrite_t *pR )
{
	Paxos_t	W;
	int	i, k, N;
	Gap_t	*pGap;

	/*	Paxos */
	IOUnmarshal( pR, &W, sizeof(*pPaxos) );

	Multi	= W.p_Multi;
	list_init( &Multi.m_Lnk );
	NextDecide	= W.p_NextDecide;
	BackPage	= Page;

	/* OutOfBand */
	for( k = 0; k < PAXOS_MAX; k++ ) {
		if( RndValue(k).v_Flags & VALUE_VALIDATE ) {
			(pPaxos->p_fOutOfBandUnmarshal)
							( pPaxos, MetaCmdRnd(k).c_opaque, pR );
		}
	}
	/* Gaps */
	IOUnmarshal( pR, &N, sizeof(N) );
	for( i = 0; i < N; i++ ) {
		pGap = (Gap_t*)PaxosMalloc( pPaxos, sizeof(Gap_t) );

		IOUnmarshal( pR, pGap,  sizeof(*pGap) );
		HashPut( &pPaxos->p_Gaps, UINT32_PNT( pGap->g_Seq ), (void*)pGap );
	}
	return( 0 );
}

int
PaxosSnapshotValidate( Paxos_t *pPaxos )
{
	int	k;

	for( k = 0; k < PAXOS_MAX; k++ ) {

	LOG( pPaxos->p_pLog, LogINF, "Page=%d NextDecide=%u k=%d Flags=0x%x", 
	Page, NextDecide, k, RndValue(k).v_Flags );

		Suppressed( k )	=	FALSE;
		if( PMode( k ) != MODE_DECIDED )	continue;

		/* OutOfBand */
		if( PaxosValidate( pPaxos, PAXOS_MODE_RECOVER, 
						Seq( k ), &RndValue( k ), &MetaCmdRnd( k ) ) ) {
			return( -1 );
		}

		RndValue( k ).v_From	= MyId;
		RndValue( k ).v_Flags	|= VALUE_RECOVER;
	}
	return( 0 );
}

timespec_t
PaxosCatchupRecvLast( Paxos_t *pPaxos )
{
	return( pPaxos->p_CatchupRecvLast );
}

timespec_t
PaxosGetAliveTime( Paxos_t *pPaxos, int j )
{
	return( pPaxos->p_aServer[j].s_Alive );
}

#ifdef	WWW
int
PaxosHighWater( Paxos_t *pPaxos )
{
	msg_t	*pm;
	int		j;
	Server_t	*pserver;
	int	Ret;

	MtxLock( &pPaxos->p_Lock );

	j = Elected;
	if( j >= 0 ) {

		pPaxos->p_WaterLeader[j]	= WATER_HIGH;
		pserver	= &pPaxos->p_aServer[j];
		pm = msg_alloc( pPaxos, j, MSG_HIGH_WATER, sizeof(*pm) );

		PaxosSend( pPaxos, pserver, pm );

		LOG( pPaxos->p_pLog, LogINF, "j=%d[%d]", j, pPaxos->p_WaterLeader[j] );
		Ret	= 0;
	} else {
		Ret	= -1;
	}
	MtxUnlock( &pPaxos->p_Lock );

	return( Ret );
}

int
PaxosLowWater( Paxos_t *pPaxos )
{
	msg_t	*pm;
	int		j;
	Server_t	*pserver;

	MtxLock( &pPaxos->p_Lock );

	for( j = 0; j < pPaxos->p_Max/*PAXOS_SERVER_MAX*/; j++ ) {
		if( pPaxos->p_WaterLeader[j] != WATER_LOW ) {
			pPaxos->p_WaterLeader[j] = WATER_LOW_ISSUED;

			pserver	= &pPaxos->p_aServer[j];
			pm = msg_alloc( pPaxos, j, MSG_LOW_WATER, sizeof(*pm) );

			PaxosSend( pPaxos, pserver, pm );

			LOG( pPaxos->p_pLog, LogINF, 
				"j=%d[%d]", j, pPaxos->p_WaterLeader[j] );
		}
	}
	MtxUnlock( &pPaxos->p_Lock );

	return( 0 );
}
#endif

