/******************************************************************************
*
  PaxosSessionClient.c 	--- client part of Paxos-Session Library
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
 *	試験			塩尻常春,久保正業
 *	特許、著作権	トライテック
 *					特願2010-538664(PCT/JP2010/066906)
 *
 */
/*
 *	Master探し
 *	 少なくとも一つの接続がある限りmaster探し
 *	 新仮masterは全サーバを循環で訪問
 *	 master切断時,master選定待機待ちのタイムアウトは
 *	始めからmaster探し
 *	 master選定待ちで少数派のみとの接続はタイムアウトによる
 *	この時前記により順番に全サーバを訪問する
 *	接続があれば直ちに通知される
 *	 不定応答時にmaster探しをするとパケット、接続が増えるのでやらない
 */

//#define	_DEBUG_
//#define	_LOCK_

#define		_PAXOS_SESSION_CLIENT_
#include	"PaxosSession.h"

static struct {
	bool_t			g_bInit;
	Mtx_t			g_Mtx;
	list_t			g_Session;
	pthread_t		g_Health;
	FdEventCtrl_t	g_FdEvCtrl;
	pthread_t		g_ThreadFdEvent;
} Global;


int _ConnectToCell( Session_t* pSession );
int	_PaxosSessionAbort( Session_t *pSession );

int
PaxosSessionLogCreate( Session_t* pSession, int Size, int Flags, 
						FILE* pFile, int Level )
{
	ASSERT( pSession->s_pLog == NULL );
	pSession->s_pLog	= LogCreate( Size, 
									pSession->s_fMalloc, pSession->s_fFree, 
									Flags, pFile, Level );
	return( 0 );
}

void
PaxosSessionLogDestroy( Session_t* pSession )
{
	if( pSession->s_pLog ) {
		LogDump( pSession->s_pLog );
		LogDestroy( pSession->s_pLog );
	}
	pSession->s_pLog	= NULL;
}

struct Log*
PaxosSessionGetLog( Session_t* pSession )
{
	return( pSession->s_pLog );
}

int
PaxosSessionSetLog( Session_t* pSession, struct Log* pLog )
{
	pSession->s_pLog	= pLog;
	return( 0 );
}

pHandle _ConnectToServer( Session_t* pSession, int j );

int
PthreadCreate( pthread_t* pThread, void*(*fThread)(void*), void*pArg )
{
	int	Ret;

	Ret = pthread_create( pThread, NULL, fThread, pArg );

ASSERT( !Ret );
	return( Ret );
}

int
PthreadDetach()
{
	int	Ret;

	Ret = pthread_detach( pthread_self() );
ASSERT( !Ret );
	return( Ret );
}

void
PthreadExit( void* pRet )
{
ASSERT( pRet == NULL );
	pthread_exit( pRet );
}

PaxosSessionHead_t* RecvMsg( Session_t* pSession, pHandle pH );

void
_NEW_MASTER( Session_t* pSession )
{
	int	j;

	for( j = 0; j < pSession->s_Max; j++ ) {
		if( ++pSession->s_NewMaster >= pSession->s_Max ) {
			pSession->s_NewMaster = 0;
		}
#ifdef	ZZZ
		if( pSession->s_pServer[pSession->s_NewMaster].s_CheckLast 
							< time(0) ) {
LOG( pSession->s_pLog, LogDBG, 
"s_Master(%d)<-s_NewMaster(%d) s_CheckLast(%lu) time(0)=%lu ###", 
pSession->s_Master, pSession->s_NewMaster, 
pSession->s_pServer[pSession->s_NewMaster].s_CheckLast, time(0) );

			pSession->s_Master	= pSession->s_NewMaster;
			return;
		}
#else
		if( pSession->s_pServer[pSession->s_NewMaster].s_CheckLast && 
		pSession->s_pServer[pSession->s_NewMaster].s_CheckLast <= time(0) ) {

LOG( pSession->s_pLog, LogDBG, 
"s_Master(%d)<-s_NewMaster(%d) s_CheckLast(%lu) time(0)=%lu ###", 
pSession->s_Master, pSession->s_NewMaster, 
pSession->s_pServer[pSession->s_NewMaster].s_CheckLast, time(0) );

			pSession->s_Master	= pSession->s_NewMaster;
			return;
		}
#endif
	}
	pSession->s_Master	= -1;
}

bool_t
_IsConnectedToCell( Session_t* pSession )
{
	int	j;

	for( j = 0; j < pSession->s_Max; j++ ) {
		if( pSession->s_pServer[j].s_Binded ) {
			return( TRUE );
		}
	}
	return( FALSE );
}

int
RecvMsgFromServerHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	Session_t*	pSession;
	int			j, jj;
	PaxosSessionHead_t*	pM;
	int	Ret;
	pHandle	fd;
	int	l;
	Msg_t		*pMsg;
	MsgVec_t	Vec;

	DBG_ENT();

	if( Mode != EV_LOOP )	return( 0 );

	errno	= 0;

	pSession = (Session_t*)pEv->fd_pArg;

	fd	= INT32_PNT(pEv->fd_Fd);
	j	= pEv->fd_Type;

//	LOG( pSession->s_pLog, "fd=%d j=%d", fd, j );

	pM = RecvMsg( pSession, fd );

	/* 切断には新マスター接続??? */
	if( !pM ) {
//DBG_PRT("pM=NULL j=%d\n", j );

		MtxLock( &pSession->s_Mtx );

		if( !pSession->s_pServer[j].s_Binded )	goto ret;

		Ret = FdEventDel( pEv );
		(pSession->s_fFree)( pEv );
		ASSERT( Ret == 0 );

		Ret = (pSession->s_fShutdown)( fd, 2 );
		Ret = (pSession->s_fClose)( fd );

		pSession->s_pServer[j].s_Binded	= FALSE;
		pSession->s_pServer[j].s_pEvent	= NULL;

		LOG( pSession->s_pLog, LogINF, "#### CLOSE pHandle j=%d ###", j );

		if(  pSession->s_Master == j ) {

			J_SET( j );	// Jeopard
			_NEW_MASTER(pSession);

			for( jj = 0; jj < pSession->s_Max; jj++ ) {

				if( jj == j )	continue;

				_ConnectToServer( pSession, jj );
				LOG( pSession->s_pLog, LogINF, "### CONNECT ALL %d %d ###",
						jj, pSession->s_pServer[jj].s_Binded );

//				if( ++cnt > pSession->s_Max/2 )	break;
			}

			if( pSession->s_Status & SB_WAIT ) {

				LOG( pSession->s_pLog, LogINF, 
						"#### SB_WAIT Broadcast j=%d ###", j );

				CndBroadcast( &pSession->s_CndMaster );

			} else if( pSession->s_Status & SB_ISSUED ) {

				pM	= (PaxosSessionHead_t*)(pSession->s_fMalloc)
								( sizeof(PaxosSessionHead_t) );
				SESSION_MSGINIT( pM, PAXOS_SESSION_EVENT, 
					PAXOS_Im_NOT_MASTER, 0, sizeof(PaxosSessionHead_t) );
				pM->h_Id		= pSession->s_Id;
				pM->h_From		= j;
				pM->h_Master	= -1;

				LOG( pSession->s_pLog, LogINF,
				"POST GENERATE EVENT ImNotMaster CLOSED pM=%p j=%d",
				pM, j );

				pMsg = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree ); 
				MsgVecSet( &Vec, VEC_MINE, pM, pM->h_Len, pM->h_Len );
				MsgAdd( pMsg, &Vec );
				QueuePostEntry( &pSession->s_Post, pMsg, m_Lnk );
			}
		}
		LOG( pSession->s_pLog, LogINF, "#### EXIT-2 j=%d ###", j );
ret:
		MtxUnlock( &pSession->s_Mtx );
		return( 0 );
	}

	PaxosSessionGetMemberReq_t	Member;
	PaxosSessionGetMemberRpl_t	*pMember;
	
	if( SEQ_CMP( pM->h_Ver,  pSession->s_pCell->c_Version ) > 0 ) {
		/* send GET_MEMBER request */
		LOG( pSession->s_pLog, LogINF,
				"#### GET_MEMBER Request j=%d h_Ver=%d(%d)###", 
				j, pM->h_Ver, pSession->s_pCell->c_Version );

		SESSION_MSGINIT( &Member, PAXOS_SESSION_GET_MEMBER, 
											0, 0, sizeof(Member) );
		Member.m_Head.h_Id			= pSession->s_Id;
		(pSession->s_fGetMyAddr)( fd, &Member.m_Head.h_Id.i_Addr );

		Ret = (pSession->s_fSendTo)( fd, (char*)&Member, sizeof(Member), 0 );
	}
	switch( pM->h_Cmd ) {
		case PAXOS_SESSION_GET_MEMBER:
			/* set Addrs */
			pMember	= (PaxosSessionGetMemberRpl_t*)pM;

			LOG( pSession->s_pLog, LogINF,
				"#### GET_MEMBER Rpl j=%d Ver=%d ###",
				j, pMember->m_Cell.c_Version );

			l	= sizeof(PaxosSessionCell_t)
					+ sizeof(PaxosAddr_t)*pMember->m_Cell.c_Maximum;
			memcpy( pSession->s_pCell, &pMember->m_Cell, l );
			(pSession->s_fFree)( pM );
			break;
		case PAXOS_SESSION_DATA:
			pMsg = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree ); 
			MsgVecSet( &Vec, VEC_MINE, pM, pM->h_Len, pM->h_Len );
			MsgAdd( pMsg, &Vec );
			QueuePostEntry( &pSession->s_Data, pMsg, m_Lnk );
			break;
		default:
			pMsg = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree ); 
			MsgVecSet( &Vec, VEC_MINE, pM, pM->h_Len, pM->h_Len );
			MsgAdd( pMsg, &Vec );
			QueuePostEntry( &pSession->s_Recv, pMsg, m_Lnk );
			break;
	}
	return( 0 );
}

void*
RecvMsgFromCellThread( void* pA )
{
	Session_t	*pSession = (Session_t*)pA;
	int			j, j1;
	PaxosSessionHead_t*	pM;
	bool_t	bChange;
	Msg_t	*pMsg;
#ifdef	ZZZ
	int Ret;
#else
#endif

	LOG( pSession->s_pLog, LogINF, "### START ###" );
loop:
#ifdef	ZZZ
	Ret = QueueWaitEntry( &pSession->s_Recv, pMsg, m_Lnk );

	if( Ret ) {
#else
	QueueWaitEntry( &pSession->s_Recv, pMsg, m_Lnk );

	if( !pMsg ) {
#endif

		LOG( pSession->s_pLog, LogINF, "### EXIT ###" );

//DBG_PRT("EXT\n");
		PthreadExit( NULL );
	}
	pM	= (PaxosSessionHead_t*)MsgFirst( pMsg );
ASSERT( pM );

	MtxLock( &pSession->s_Mtx );

	j = pM->h_From;

		/*
		 * Reader
		 */
//LOG( pSession->s_pLog, "j=%d s_Reader=%d h_Reader=%d h_Cmd=0x%x pH=0x%x", 
//			j, pSession->s_Reader, pM->h_Reader,
//			pM->h_Cmd,
//			pSession->s_pServer[j].s_pHandle );

		if( pSession->s_Reader < 0 ) {

//			LOG( pSession->s_pLog, "j=%d Reader=%d", j, pM->h_Reader );
			pSession->s_Reader	= pM->h_Reader;
		}

		if( pM->h_TimeoutUsec ) {
			pSession->s_MasterElectionUsec = pM->h_TimeoutUsec;
			pSession->s_UnitUsec	= pM->h_UnitUsec;
		}
		bChange	= FALSE;
		/*
		 *	leader交替イベント
		 */
		switch( pM->h_Code ) {

		case PAXOS_MASTER_ELECTED:
		case PAXOS_MASTER_CANCELED:
		case PAXOS_Im_NOT_MASTER:
		case PAXOS_Im_MASTER:

			if( j == pM->h_Master ) {

				LOG( pSession->s_pLog, LogINF,
				"I'm Master=%d s_Master=%d", j, pSession->s_Master );

				CLEAR_JEOPARD;

				bChange = (pSession->s_Master != pM->h_Master);
				pSession->s_Master	= pM->h_Master;

			} else if( pM->h_Master == pSession->s_Master ) {
				LOG( pSession->s_pLog, LogINF, "NO ACTION-1 j=%d s_Master=%d", 
						j, pSession->s_Master );
			} else {

				j1	= J_IND( pM->h_Master );

				if( j1 < pSession->s_Max && pSession->s_pJeopard[j1] > 0 ) {

					LOG( pSession->s_pLog, LogINF,
							"JEOPARD j=%d h_Master=%d s_Master=%d Jeopard=%d", 
							j, pM->h_Master, pSession->s_Master, j1 );

					bChange	= ( pSession->s_Master != -1 );
					pSession->s_Master	= -1;

				} else {
					LOG( pSession->s_pLog, LogINF,
					"CHANGE h_From=%d h_Master=%d <- s_Master=%d",
						pM->h_From, pM->h_Master, pSession->s_Master );

					pSession->s_pJeopard[j1]++;

					pSession->s_Master	= pM->h_Master;
					bChange	= TRUE;
				}
			}
			if( bChange )	{
				if( pSession->s_Master >= 0 ) {
					_ConnectToServer( pSession, pSession->s_Master );
					LOG( pSession->s_pLog, LogINF, "CONNECT %d %d",
						pSession->s_Master,
						pSession->s_pServer[pSession->s_Master].s_Binded );
				}
				if( pSession->s_Status & SB_WAIT ) {
					//LOG( pSession->s_pLog, "SB_WAIT s_Master=%d", pSession->s_Master );
					CndBroadcast( &pSession->s_CndMaster );
					MsgDestroy( pMsg );
				} else if( pSession->s_Status & SB_ISSUED ) {
					//LOG( pSession->s_pLog, "POST LEADER pM=%p j=%d j1=%d",pM,j, j1);
					QueuePostEntry( &pSession->s_Post, pMsg, m_Lnk );
				}
			} else {
				LOG( pSession->s_pLog, LogINF, "MsgDestroy");
				MsgDestroy( pMsg );
			}
			MtxUnlock( &pSession->s_Mtx );
			goto loop;

		default:
			break;
		}
		switch( pM->h_Cmd ) {

		/* 閉塞 */
		case PAXOS_SESSION_CLOSE:
			if( !pM->h_Code ) {
				pSession->s_Status |= SB_CLOSED;
			}
			/* follow through */
		default:
			QueuePostEntry( &pSession->s_Post, pMsg, m_Lnk );
			break;

		}

	MtxUnlock( &pSession->s_Mtx );

	goto loop;
//	return( NULL );
}

/*
 *	Health check を送信する
 *	送信を行わないと通信異常を検知できない
 *	検知後Fdをクローズし、受信スレッドに通知する
 *	後処理は受信スレッドが行う
 *	マスタとの通信異常はJeopardとなるので
 *	マスタは充分な時間を待ってABORT処理を行うこと
 */
static void*
HealthSendThread(void*pArg)
{
#ifdef	ZZZ
	Session_t	*pSession;
#else
	Session_t	*pSession, *pSW;
#endif
	Server_t	*pServer;
	int	j;
	PaxosSessionHead_t	Health;
	int	Ret;
	pHandle	fd;

	while( TRUE ) {

		MtxLock( &Global.g_Mtx );

#ifdef	ZZZ
		list_for_each_entry( pSession, &Global.g_Session, s_Lnk ) {
#else
		list_for_each_entry_safe( pSession, pSW, &Global.g_Session, s_Lnk ) {
#endif

			MtxLock( &pSession->s_Mtx );

			LOG( pSession->s_pLog, LogDBG,
					"START Health pSession=%p", pSession );

			for( j = 0; j < pSession->s_Maximum; j++ ) {

				pServer = &pSession->s_pServer[j];

				if( pServer->s_Binded ) {

					fd	= INT32_PNT(pServer->s_pEvent->fd_Fd);

					SESSION_MSGINIT( &Health, PAXOS_SESSION_HEALTH, 
											0, 0, sizeof(Health) );
					Health.h_Id			= pSession->s_Id;
					(pSession->s_fGetMyAddr)( fd, &Health.h_Id.i_Addr );

					Health.h_Cksum64	= in_cksum64(&Health,sizeof(Health),0);
					Ret = (pSession->s_fSendTo)( fd, 
								(char*)&Health, sizeof(Health), 0 );

					LOG( pSession->s_pLog, LogDBG,
								"SEND Health j=%d Ret=%d", j, Ret );

					if( Ret < 0 ) {
						LOG( pSession->s_pLog, LogERR, "Health ERROR j=%d", j );
					}
				}
			}
#ifdef	ZZZ
#else
			if( pSession->s_Master >= 0 ) {
				pSession->s_NoMaster = 0;
			} else {
				if( pSession->s_NoMaster ) {
					// Abort pSession
					if( pSession->s_NoMaster + 
						pSession->s_MasterElectionUsec/1000000LL 
							< time(0) ) {

						LOG( pSession->s_pLog, LogERR,
							 "=== SPLIT BRAIN pSession=%p===", pSession );

						_PaxosSessionAbort( pSession );

						list_del_init( &pSession->s_Lnk );
	
						pSession->s_Err	= ERR_SESSION_MINORITY;
					}
				} else {	// detect minority
					LOG( pSession->s_pLog, LogDBG,
						"DETECT Minority %lu %"PRIu64"",
						time(0), pSession->s_MasterElectionUsec/1000000LL );

					pSession->s_NoMaster	= time(0);
				}
			}
#endif
			LOG( pSession->s_pLog, LogDBG,
						"END Health pSession=%p", pSession );
			MtxUnlock( &pSession->s_Mtx );
		}
		MtxUnlock( &Global.g_Mtx );

		sleep( DT_SESSION_HEALTH );
	}
	return( NULL );
}
/*
 *	接続を行い受信スレッドを起動する
 */
pHandle
_ConnectToServer( Session_t* pSession, int j )
{
	pHandle	pH;
	PaxosSessionHead_t	Bind;
	FdEvent_t	*pEv;


	int	Ret;

	DBG_ENT();

	if( pSession->s_pServer[j].s_Binded ) {
		RETURN( INT32_PNT(pSession->s_pServer[j].s_pEvent->fd_Fd) );
	}
	pSession->s_pServer[j].s_CheckLast	= time(0) + 10;
	pH = (pSession->s_fConnect)( pSession, j );

	if( pH ) {

		/* 通信端点の指示 */
		SESSION_MSGINIT( &Bind, PAXOS_SESSION_BIND, 0, 0, sizeof(Bind) );
		Bind.h_Id			= pSession->s_Id;
		(pSession->s_fGetMyAddr)( pH, &Bind.h_Id.i_Addr );

		Bind.h_Cksum64	= in_cksum64( &Bind, sizeof(Bind), 0 );
		Ret = (pSession->s_fSendTo)( pH, (char*)&Bind, sizeof(Bind), 0 );

		LOG( pSession->s_pLog, LogINF,
				"SEND BIND/ADD EVENT Ret=%d j=%d Pid=%d No=%d pH=0x%x",
			Ret, j,pSession->s_Id.i_Pid,pSession->s_Id.i_No, pH );

		pSession->s_pServer[j].s_Binded		= TRUE;
		(pSession->s_fGetMyAddr)( pH, &pSession->s_pServer[j].s_MyAddr );
		pSession->s_Id.i_Addr	= pSession->s_pServer[j].s_MyAddr;

		pEv	= (FdEvent_t*)(pSession->s_fMalloc)( sizeof(*pEv) );
#ifdef	ZZZ
		FdEventInit( pEv, j, PNT_INT32(pH), EPOLLIN, 
					pSession, RecvMsgFromServerHandler );
#else
		FdEventInit( pEv, j, PNT_INT32(pH), FD_EVENT_READ, 
					pSession, RecvMsgFromServerHandler );
#endif

		FdEventAdd( &Global.g_FdEvCtrl, (uint64_t)PNT_INT32(pH), pEv );

		pSession->s_pServer[j].s_pEvent	= pEv;
		LOG( pSession->s_pLog, LogINF, "s_pEvent=%p", pEv);

		if( !Global.g_Health ) {
			LOG( pSession->s_pLog, LogDBG, "START HEALTH Thread");
			Ret = PthreadCreate( &Global.g_Health,  HealthSendThread, NULL );
		}

	} else {

		J_SET( -1 );

		/***
		LOG( pSession->s_pLog, "CONNECT ERROR j=%d pH=%d",
			j, PNT_INT32( pH ) );
		***/
	}

	RETURN( pH );
}

/*
 *	仮masterに接続
 *		-1の時はwait
 */
int
_ConnectToCell( Session_t* pSession )
{
	int	Ret;
	int	Count = 0;

	DBG_ENT();

again:
	if( !(pSession->s_Status & SB_ACTIVE) )	RETURN( -1 );

	if( pSession->s_Master >= 0 ) {

		if( _ConnectToServer( pSession, pSession->s_Master ) ) {

			RETURN( pSession->s_Master );

		} else {
			
			_NEW_MASTER( pSession );

			/* s_Masterはあるがconnectループをする */
			if( ++Count > pSession->s_Max )	RETURN( -1 );
			goto again;
		}
	} else {

		LOG( pSession->s_pLog, LogINF,
			"ELECTION WAIT %d %d %d %d %d dt=%dms",
			pSession->s_pServer[0].s_Binded,
			pSession->s_pServer[1].s_Binded,
			pSession->s_pServer[2].s_Binded,
			pSession->s_pServer[3].s_Binded,
			pSession->s_pServer[4].s_Binded,
			(int)(pSession->s_MasterElectionUsec/1000) );

		if( !_IsConnectedToCell( pSession ) )	RETURN( -1 );

		pSession->s_Status	|= SB_WAIT;
		Ret = CndTimedWaitWithMs( 
					&pSession->s_CndMaster, &pSession->s_Mtx,
					(int)(pSession->s_MasterElectionUsec/1000) );
		pSession->s_Status	&= ~SB_WAIT;
		if( Ret ) {
			LOG( pSession->s_pLog, LogINF,
				"ELECTION WAIT AFTER Ret=%d s_Master=%d", 
				Ret, pSession->s_Master );

			CLEAR_JEOPARD;
			_NEW_MASTER( pSession );
		}
		goto again;
	}
}

/*
 *	masterに送る
 *		master不明時にはどこかに送り、master通知をもらう
 */
#define	SET_HEAD( p ) \
		(p)->h_Cksum64		= 0; \
		(p)->h_Id			= pSession->s_Id; \
		(p)->h_Id.i_SeqWait	= pSession->s_Update; \
		(p)->h_Id.i_Addr	= pSession->s_pServer[j].s_MyAddr;

PaxosSessionHead_t*
RecvMsg( Session_t* pSession, pHandle pH )
{
	PaxosSessionHead_t	M, *pM;
	
	DBG_ENT();

	if( PeekStreamByFunc( pH, (char*)&M, sizeof(M), pSession->s_fRecvFrom ) ) {
//DBG_PRT("PEEK\n");
		return( NULL );
	}
	pM	= (PaxosSessionHead_t*)(pSession->s_fMalloc)( M.h_Len );
	if( RecvStreamByFunc( pH, (char*)pM, M.h_Len, pSession->s_fRecvFrom ) ) {
		(pSession->s_fFree)( pM );
//DBG_PRT("STREAM\n");
		return( NULL );
	}
	if( pM->h_Cksum64 ) {
		ASSERT( !in_cksum64( pM, M.h_Len, 0 ) );
	}
	RETURN( pM );
}

#ifdef	ZZZ
Msg_t*
PaxosRecvFromCell( Session_t* pSession, int *pReason )
{
	Msg_t*	pMsg = NULL;
	int	Ret;

//DBG_PRT("wait=%d\n", (int)(pSession->s_MasterElectionUsec/1000) );
	Ret = QueueTimedWaitEntryWithMs( &pSession->s_Post, pMsg, m_Lnk, 
					(int)(pSession->s_MasterElectionUsec/1000) );
//DBG_PRT("Ret=%d\n", Ret );
	if( Ret != 0 ) {
		LOG( pSession->s_pLog, LogDBG, "Ret=%d", Ret );
		*pReason = Ret;
		return( NULL );
	} else {
		*pReason	= 0;
		return( pMsg );
	}
}
#else
Msg_t*
PaxosRecvFromCell( Session_t* pSession, int *pReason )
{
	Msg_t*	pMsg = NULL;

//DBG_PRT("wait=%d\n", (int)(pSession->s_MasterElectionUsec/1000) );
	QueueTimedWaitEntryWithMs( &pSession->s_Post, pMsg, m_Lnk, 
					(int)(pSession->s_MasterElectionUsec/1000) );
//DBG_PRT("Ret=%d\n", Ret );
	if( !pMsg ) {
		LOG( pSession->s_pLog, LogDBG, "errno=%d", errno );
		*pReason = errno;
		return( NULL );
	} else {
		*pReason	= 0;
		return( pMsg );
	}
}
#endif

#ifdef	ZZZ
int
PaxosRecvFromCellDiscard( Session_t* pSession )
{
	PaxosSessionHead_t*	pM;
	Msg_t	*pMsg;
	int	Ret;

	do {
		Ret = QueueTimedWaitEntryWithMs( &pSession->s_Post, pMsg, m_Lnk, 0 );
		if( !Ret )  {
			pM	= (PaxosSessionHead_t*)MsgFirst( pMsg );
			LOG( pSession->s_pLog, LogINF,
				"??? FREE pMsg=0x%x i_Seq=%d h_Cmd=%d h_From=%d h_Code=%d", 
					pM, pM->h_Id.i_Seq, pM->h_Cmd, pM->h_From, pM->h_Code );
			MsgDestroy( pMsg );
		}
	} while( !Ret );
	return( 0 );
}
#else
int
PaxosRecvFromCellDiscard( Session_t* pSession )
{
	PaxosSessionHead_t*	pM;
	Msg_t	*pMsg;

	do {
		QueueTimedWaitEntryWithMs( &pSession->s_Post, pMsg, m_Lnk, 0 );
		if( pMsg )  {
			pM	= (PaxosSessionHead_t*)MsgFirst( pMsg );
			LOG( pSession->s_pLog, LogINF,
				"??? FREE pMsg=0x%x i_Seq=%d h_Cmd=%d h_From=%d h_Code=%d", 
					pM, pM->h_Id.i_Seq, pM->h_Cmd, pM->h_From, pM->h_Code );
			MsgDestroy( pMsg );
		}
	} while( pMsg );
	return( 0 );
}
#endif

int
PaxosSessionOpen( Session_t* pSession, int No, bool_t Sync )
{
	PaxosSessionHead_t	Req;
	PaxosSessionHead_t*	pRpl = NULL;
	int	Ret;
	int	j;

	DBG_ENT();

	if( !( pSession->s_Status & SB_INIT) ) {
		RETURN( -1 );
	}
ASSERT( !(pSession->s_Status & SB_ACTIVE ) );

#ifdef	ZZZ
#else
	pSession->s_Post.q_abrt	= 0;
	pSession->s_Recv.q_abrt	= 0;
	pSession->s_NoMaster	= 0;
	pSession->s_Err			= 0;
#endif
	PaxosRecvFromCellDiscard( pSession );

	MtxLock( &pSession->s_Mtx );

#ifdef	ZZZ
	for( j = 0; j < pSession->s_Max; j++ ) {
		pSession->s_pServer[j].s_Binded		= FALSE;
		pSession->s_pServer[j].s_CheckLast	= 0;
		pSession->s_pServer[j].s_pEvent		= NULL;
	}
#endif
	pSession->s_Id.i_Time				= time(0);
	pSession->s_Id.i_No					= No;
	pSession->s_Id.i_Seq				= 0;
	pSession->s_Id.i_SeqWait			= 0;
	pSession->s_Id.i_Reuse++;

	pSession->s_MasterElectionUsec 		= 30000000LL;

	pSession->s_Status	= SB_ACTIVE|SB_INIT;
	pSession->s_Update	= 0;
	pSession->s_Sync	= Sync;

	CLEAR_JEOPARD;
	pSession->s_Master	= 0;
	pSession->s_Reader	= -1;
#ifdef	ZZZ
#else
	for( j = 0; j < pSession->s_Max; j++ ) {
		pSession->s_pServer[j].s_Binded		= FALSE;
		pSession->s_pServer[j].s_CheckLast	= time(0);
		pSession->s_pServer[j].s_pEvent		= NULL;
	}
#endif

	PthreadCreate( &pSession->s_RecvThread,
						RecvMsgFromCellThread, (void*)pSession );

	MtxUnlock( &pSession->s_Mtx );

	SESSION_MSGINIT( &Req, PAXOS_SESSION_OPEN, 0, 0, sizeof(Req) );

	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	pMsgReq = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree );
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );
	
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PaxosSessionHead_t*)MsgFirst( pMsgRpl );
		Ret = pRpl->h_Error;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}

	if( Ret != 0 )	pSession->s_Status &= ~SB_ACTIVE;
	MtxLock( &Global.g_Mtx );

	list_add_tail( &pSession->s_Lnk, &Global.g_Session );

	MtxUnlock( &Global.g_Mtx );

	LOG( pSession->s_pLog, LogINF, "### OPEN(%p %d) ###", pSession, Ret );
	RETURN( Ret );
}

#ifdef	ZZZ
#else
struct Session*
PaxosSessionFind( char *pCellName, int NO )
{
	struct Session *pSession;

	MtxLock( &Global.g_Mtx );

	list_for_each_entry( pSession, &Global.g_Session, s_Lnk ) {
		if( !strcmp( pSession->s_pCell->c_Name, pCellName )
				&& pSession->s_Id.i_No == NO ) {
			goto found;
		}
	}
	pSession = NULL;
found:
	MtxUnlock( &Global.g_Mtx );
	return( pSession );
}

#endif

int
PaxosSessionGetSize()
{
	return( sizeof(Session_t) );
}

PaxosSessionCell_t*
PaxosSessionGetCell( Session_t* pSession )
{
	return( pSession->s_pCell );
}

void*
PaxosSessionGetTag( Session_t* pSession )
{
	return( pSession->s_pVoid );
}

void
PaxosSessionSetTag( Session_t* pSession, void *p )
{
	pSession->s_pVoid	= p;
}

void*
ThreadFdEventLoopSession( void *pArg )
{
	FdEventLoop( &Global.g_FdEvCtrl, -1 );

	PthreadExit( (void*)-1 );
	return( (void*)-1 );
}

struct Session*
PaxosSessionAlloc(
			void*(*fConnect)(Session_t*,int),
			int(*fShutdown)(void*,int),
			int(*fClose)(void*),
			int(*fGetMyAddr)(void*,struct sockaddr_in*),
			int(*fSendTo)(void*,char*,size_t,int),
			int(*fRecvFrom)(void*,char*,size_t,int),
			void*(*fMalloc)(size_t),
			void(*fFree)(void*),
			char*pCellName )
{
	//int	Ret = 0;
	int	l;
	int	Maximum;
	PaxosSessionCell_t	*pCell;
	Session_t			*pSession;

	DBG_ENT();

	if( !Global.g_bInit ) {
			MtxInit( &Global.g_Mtx );
			list_init( &Global.g_Session );
			FdEventCtrlCreate( &Global.g_FdEvCtrl );
			PthreadCreate(&Global.g_ThreadFdEvent,
							ThreadFdEventLoopSession,(void*)NULL );
			Global.g_bInit = TRUE;
	}
	if( fMalloc )	pSession	= (Session_t*)(*fMalloc)( sizeof(*pSession) );
	else			pSession	= (Session_t*)Malloc( sizeof(*pSession) );
	if( !pSession )	goto err1;
	memset( pSession, 0, sizeof(*pSession) );

	pSession->s_fConnect	= fConnect;
	pSession->s_fGetMyAddr	= fGetMyAddr;
	pSession->s_fShutdown	= fShutdown;
	pSession->s_fClose	= fClose;
	pSession->s_fSendTo	= fSendTo;
	pSession->s_fRecvFrom	= fRecvFrom;
	if( fMalloc )	pSession->s_fMalloc	= fMalloc;
	else			pSession->s_fMalloc	= Malloc;
	if( fFree )		pSession->s_fFree	= fFree;
	else			pSession->s_fFree	= Free;

	if( pCellName ) {
		pSession->s_pCell = pCell = 
			PaxosSessionCellGetAddr( pCellName, pSession->s_fMalloc );
		if( !pSession->s_pCell )	goto err1;
		Maximum	= pSession->s_pCell->c_Maximum;
		pSession->s_Max		= Maximum;
		pSession->s_Maximum	= Maximum;

		l	= sizeof(Server_t)*Maximum;
		pSession->s_pServer	= (Server_t*)(pSession->s_fMalloc)( l );
		if( !pSession->s_pServer )	goto err2;
		memset( pSession->s_pServer, 0, l );

		l	= sizeof(int)*(Maximum + 1);
		pSession->s_pJeopard	= (int*)(pSession->s_fMalloc)( l );
		if( !pSession->s_pJeopard )	goto err3;
		memset( pSession->s_pJeopard, 0, l );
	}

	pSession->s_Id.i_Pid				= getpid();
	pSession->s_Master					= 0;

	MtxInit( &pSession->s_Serialize );
	MtxInit( &pSession->s_Mtx );
	CndInit( &pSession->s_CndMaster );
#ifdef ZZZ
	CndInit( &pSession->s_CndThread );
#endif
	QueueInit( &pSession->s_Post );
	QueueInit( &pSession->s_Recv );
	QueueInit( &pSession->s_Data );
	list_init( &pSession->s_Lnk );

	pSession->s_Status	|= SB_INIT;

	DBG_EXT( pSession );
	return( pSession );
err3:
	(pSession->s_fFree)( pSession->s_pServer );
err2:
	(pSession->s_fFree)( pCell );
err1:
	DBG_EXT( 0 );
	return( NULL );
}

int
PaxosSessionFree( Session_t *pSession )
{
	if( pSession->s_pJeopard )	(pSession->s_fFree)( pSession->s_pJeopard );
	if( pSession->s_pServer )	(pSession->s_fFree)( pSession->s_pServer );
	if( pSession->s_pCell )	(pSession->s_fFree)( pSession->s_pCell );
	return( 0 );
}

int
_PaxosSessionAbort( Session_t *pSession )
{
	int	j;
	FdEvent_t	*pEv;

	LOG( pSession->s_pLog, LogINF, "pSession=%p", pSession );

#ifdef	ZZZ
	pSession->s_Status	&= ~SB_ACTIVE;

	/* 受信キューアボート */
	QueueAbort( &pSession->s_Post, SIGKILL );
#endif

	/* サーバ受信入り閉塞 */
	for( j = 0; j < pSession->s_Maximum; j++ ) {

		if( !pSession->s_pServer[j].s_Binded )	continue;

		pEv	= pSession->s_pServer[j].s_pEvent;

		pSession->s_pServer[j].s_Binded	= FALSE;
		pSession->s_pServer[j].s_pEvent	= NULL;

		FdEventDel( pEv );

		(pSession->s_fShutdown)( INT32_PNT(pEv->fd_Fd), 2 );
		(pSession->s_fClose)( INT32_PNT(pEv->fd_Fd) );

		(pSession->s_fFree)( pEv );
	}

	return( 0 );
}

int
PaxosSessionAbort( Session_t *pSession )
{
	MtxLock( &pSession->s_Mtx );

#ifdef	ZZZ
	if( pSession->s_Status & SB_ACTIVE )	_PaxosSessionAbort( pSession );
#else
	if( pSession->s_Status & SB_ACTIVE ) {
		pSession->s_Status	&= ~SB_ACTIVE;
	}

	MtxUnlock( &pSession->s_Mtx );
	QueueAbort( &pSession->s_Post, SIGKILL );
	QueueAbort( &pSession->s_Recv, SIGKILL );
	pthread_join( pSession->s_RecvThread, NULL );
	pSession->s_RecvThread = 0;
	MtxLock( &pSession->s_Mtx );
	_PaxosSessionAbort( pSession );
#endif

	MtxUnlock( &pSession->s_Mtx );

	MtxLock( &Global.g_Mtx );

	list_del_init( &pSession->s_Lnk );

	MtxUnlock( &Global.g_Mtx );

	return( 0 );
}

int
PaxosSessionClose( Session_t* pSession )
{
	int	Ret = -1;
	PaxosSessionHead_t	Req;
	PaxosSessionHead_t*	pRpl = NULL;

	DBG_ENT();

	LOG( pSession->s_pLog, LogINF, "### CLOSE(%p %d) ###", pSession, pSession->s_Id.i_No);
	SESSION_MSGINIT( &Req, PAXOS_SESSION_CLOSE, 0, 0, sizeof(Req) );

	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;

	pMsgReq = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree );
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );
	
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PaxosSessionHead_t*)MsgFirst( pMsgRpl );
		Ret = pRpl->h_Error;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	if( Ret )	LOG( pSession->s_pLog, LogERR, "Ret=%d", Ret );

	PaxosSessionAbort( pSession );
	
	RETURN( Ret );
}

int
PaxosSessionGetMaster( Session_t* pSession )
{
	return( pSession->s_Master );
}

int
PaxosSessionGetReader( Session_t* pSession )
{
	if( pSession->s_Reader >= 0 )	return( pSession->s_Reader );
	else							return( pSession->s_Master );
}

int
PaxosSessionLock( Session_t* pSession, int j, void** ppH )
{
	pHandle		pH;
	PaxosSessionHead_t	H;
	int			Ret;

	/* このハンドルはセッションと関係ない。下記はアドレスのため */
	pH = (pSession->s_fConnect)( pSession, j );
	if( pH ) {
		SESSION_MSGINIT( &H, PAXOS_SESSION_LOCK, 0, 0, sizeof(H) );
		H.h_Id	= pSession->s_Id;
		(pSession->s_fGetMyAddr)( pH, &H.h_Id.i_Addr );

		H.h_Cksum64	= in_cksum64( &H, sizeof(H), 0 );
		Ret = (pSession->s_fSendTo)( pH, (char*)&H, sizeof(H), 0 );

		if( Ret < 0 ) {

			LOG( pSession->s_pLog, LogINF, "SendTo Error." );

			(pSession->s_fShutdown)( pH, 2 );
			(pSession->s_fClose)( pH );
			return( -1 );
		}
		Ret = (pSession->s_fRecvFrom)( pH, (char*)&H, sizeof(H), 0 );
		if( Ret < 0 || H.h_Error ) {

			LOG( pSession->s_pLog, LogINF, 
				"Ret=%d h_Error=%d", Ret, H.h_Error );

			(pSession->s_fShutdown)( pH, 2 );
			(pSession->s_fClose)( pH );
			return( -1 );
		}
		*ppH	= pH;
		return( 0 );
	}
	return( -1 );
}

int
PaxosSessionUnlock( Session_t* pSession, void* pH )
{
	PaxosSessionHead_t	H;
	int			Ret;

	SESSION_MSGINIT( &H, PAXOS_SESSION_UNLOCK, 0, 0, sizeof(H) );
	H.h_Id	= pSession->s_Id;
	(pSession->s_fGetMyAddr)( pH, &H.h_Id.i_Addr );

	H.h_Cksum64	= in_cksum64( &H, sizeof(H), 0 );
	Ret = (pSession->s_fSendTo)( pH, (char*)&H, sizeof(H), 0 );
	if( Ret < 0 ) {
		LOG( pSession->s_pLog, LogERR, "Ret=%d", Ret );
		goto err;
	}

	(pSession->s_fShutdown)( pH, 2 );
	(pSession->s_fClose)( pH );
	return( 0 );
err:
	return( -1 );
}

PaxosSessionHead_t*
PaxosSessionAny( Session_t* pSession, void* pH, PaxosSessionHead_t* pReq )
{
	PaxosSessionHead_t	H;
	PaxosSessionHead_t*	pRpl;
	int	Ret;

	SESSION_MSGINIT( &H, PAXOS_SESSION_ANY, 0, 0, sizeof(H)+pReq->h_Len );
	H.h_Id	= pSession->s_Id;
	(pSession->s_fGetMyAddr)( pH, &H.h_Id.i_Addr );

	uint64_t	sum64a, sum64b;

	sum64a	= in_sum64( &H, sizeof(H), 0 );
	sum64b	= in_sum64( pReq, pReq->h_Len, sizeof(H) % 8 );
	sum64a	+= sum64b;
	if( sum64b > sum64a )	sum64a	+= 1;
	H.h_Cksum64	= ~sum64a;

	Ret = (pSession->s_fSendTo)( pH, (char*)&H, sizeof(H), 0 );
	if( Ret < 0 )	goto err;
	Ret = (pSession->s_fSendTo)( pH, (char*)pReq, pReq->h_Len, 0 );
	if( Ret < 0 )	goto err;

	pRpl	= RecvMsg( pSession, pH );
	return( pRpl );
err:
	return( NULL );
}

int
PaxosSessionChangeMember( Session_t *pSession, 
							int Id, struct sockaddr_in *pAddr )
{
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	PaxosSessionChangeMemberReq_t	Req;
	PaxosSessionChangeMemberRpl_t	*pRpl;
	int	Ret = 0;

	SESSION_MSGINIT( &Req, PAXOS_SESSION_CHANGE_MEMBER, 0, 0, sizeof(Req) );
	Req.c_Id	= Id;
	Req.c_Addr	=  *pAddr;

	pMsgReq = MsgCreate( 1, pSession->s_fMalloc, pSession->s_fFree );
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );
	
	pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, FALSE );
	MsgDestroy( pMsgReq );
	if( pMsgRpl ) {
		pRpl	= (PaxosSessionChangeMemberRpl_t*)MsgFirst( pMsgRpl );
		Ret = pRpl->c_Head.h_Error;
		MsgDestroy( pMsgRpl );
	} else {
		Ret = -1;
	}
	return( Ret );
}

void*
(*PaxosSessionGetfMalloc(Session_t* pSession))(size_t)
{
	return( pSession->s_fMalloc );
}

void
(*PaxosSessionGetfFree(Session_t* pSession))(void*)
{
	return( pSession->s_fFree );
}

/*
 *	masterに送る
 *		master不明時にはどこかに送り、master通知をもらう
 */
/*
 *	Checksum
 */
int
PaxosSessionMsgCksum64( Session_t *pSession, int j, Msg_t *pReq )
{
	PaxosSessionHead_t*	pM = NULL;
	int	i, is;
	MsgVec_t	*pVec;

	is	= -1;
	for( i = 0; i < pReq->m_N; i++ ) {

		pVec	= &pReq->m_aVec[i];
		if( pVec->v_Flags & VEC_HEAD ) {

			if( is >= 0 ) {
				pM->h_Cksum64	= MsgCksum64( pReq, is, i - is, NULL );
			}
			is	= i;
			pM	= (PaxosSessionHead_t*)pVec->v_pStart;
			SET_HEAD( pM );
		}
	}
	if( is >= 0 ) {
		pM->h_Cksum64	= MsgCksum64( pReq, is, i - is, NULL );
	}
	return( 0 );
}

int
PaxosSessionSendToCellByMsg( Session_t* pSession, Msg_t *pReq )
{
	pHandle	pH;
#ifdef	ZZZ
	int	j;
#else
	int	j = -1;
#endif
	int	Ret;
	int	i;
	MsgVec_t	*pVec;

	MtxLock( &pSession->s_Mtx );

again:
//	LOG( pSession->s_pLog, LogDBG, "j=%d errno=%d", j, errno );

	j = _ConnectToCell( pSession );

	if( j >= 0 ) {
		pH	= INT32_PNT(pSession->s_pServer[j].s_pEvent->fd_Fd);
	} else {
		LOG( pSession->s_pLog, LogERR, "j=%d errno=%d", j, errno );
		goto err;
	}

	/* Checksum */
	PaxosSessionMsgCksum64( pSession, j, pReq );

	for( i = 0; i < pReq->m_N; i++ ) {

		pVec	= &pReq->m_aVec[i];
		if( pVec->v_Len > 0 ) {
			/* ハンドルが無くなると困る? */
			Ret = (pSession->s_fSendTo)( pH, 
								pVec->v_pStart, pVec->v_Len, 0 );

//			LOG( pSession->s_pLog, LogDBG,
//				"v_len=%d Ret=%d", pVec->v_Len, Ret );

			if( Ret < 0 ) {

				_NEW_MASTER(pSession);

				goto again;
			}
		}
	}
	MtxUnlock( &pSession->s_Mtx );
	return(  j );
err:
	MtxUnlock( &pSession->s_Mtx );
	return( -1 );
}

int
PaxosSendToReaderByMsg( Session_t* pSession, Msg_t *pReq )
{
	int					i;
	int					j;
	pHandle				pH;
	int					Ret;
	MsgVec_t			*pVec;

	MtxLock( &pSession->s_Mtx );

	if( ( j = pSession->s_Reader ) >= 0 ) {

		pH = _ConnectToServer( pSession, j );

		if( pH ) {
			/* Checksum */
			PaxosSessionMsgCksum64( pSession, j, pReq );
			for( i = 0; i < pReq->m_N; i++ ) {

				pVec	= &pReq->m_aVec[i];
				if( pVec->v_Len > 0 ) {
					Ret = (pSession->s_fSendTo)( pH, 
								pVec->v_pStart, pVec->v_Len, 0 );
					if( Ret < 0 ) {
						j = pSession->s_Reader = -1;
						goto err;
					}
				}
			}
		} else {
			j = pSession->s_Reader = -1;
		}
	}
	else {
/***
DBG_PRT("READER NOT ACTIVE\n");
***/
	}
	MtxUnlock( &pSession->s_Mtx );
	return( j );
err:
	MtxUnlock( &pSession->s_Mtx );
	return( -1 );
}

Msg_t*
PaxosSessionReqAndRplByMsg( Session_t* pSession, Msg_t *pReq, 
							bool_t Update, bool_t bForEver )
{
	int	i;
	int	j;
	PaxosSessionHead_t*	pM = NULL;
	uint32_t		Seq;
	int	ReplyCmd = -1;
	MsgVec_t	*pVec;
	Msg_t		*pMsg;
#ifdef	ZZZ
#else
	int	Ret;
#endif

	DBG_ENT();

	MtxLock( &pSession->s_Serialize );

	MtxLock( &pSession->s_Mtx );

	if( !(pSession->s_Status & SB_ACTIVE ) ) {
		MtxUnlock( &pSession->s_Mtx );
		errno = EPERM;
		goto err1;
	}
	for( i = 0; i < pReq->m_N; i++ ) {
		pVec	= &pReq->m_aVec[i];
		if( pVec->v_Flags & VEC_REPLY )	{
			ReplyCmd = ((PaxosSessionHead_t*)pVec->v_pStart)->h_Cmd;
		}
	}
	Seq = ++pSession->s_Id.i_Seq;
	pSession->s_Id.i_Try	= 0;
	pSession->s_Status	|= SB_ISSUED;

	MtxUnlock( &pSession->s_Mtx );

again:
	pSession->s_Id.i_Try++;
#ifdef	ZZZ
	if( bForEver != TRUE && pSession->s_Id.i_Try > RETRY_MAX ) {
		pSession->s_Err	= ERR_SESSION_RETRY_MAX;
		errno = EIO;
		goto err1;
	}
#endif
	/* ここでのmaster交替は？ */
	if( Update || pSession->s_Sync ) {
		j	= PaxosSessionSendToCellByMsg( pSession, pReq );
	} else {
		j	= PaxosSendToReaderByMsg( pSession, pReq );
		if( j < 0 ) {
			j	= PaxosSessionSendToCellByMsg( pSession, pReq );
			LOG( pSession->s_pLog, LogINF,
				"READER ERROR THEN PaxosSendToCellByMsg j=%d errno=%d", 
				j, errno );
		}
	}
	if( j < 0 )	{
		LOG( pSession->s_pLog, LogINF, "SendToCell err2 j=%d", j );
		errno = EIO;
		goto err2;
	}
	/* 現在masterに送った */

wait:
#ifdef	ZZZ
	pMsg	= PaxosRecvFromCell( pSession );
	if( !pMsg )	{
		if( ReplyCmd != PAXOS_SESSION_EVENT ) {
			LOG( pSession->s_pLog, LogINF,
				"=== RECV j=%d pM=NULL Seq=%d TIMEOUT s_Master=%d "
				" ReplyCmd=0x%x Try=%d ###", 
			j, Seq, pSession->s_Master, ReplyCmd, pSession->s_Id.i_Try );
		}
		goto again;
	}
#else
	pMsg	= PaxosRecvFromCell( pSession, &Ret );
	if( !pMsg )	{
		if( Ret && Ret != ETIMEDOUT )	{	// Abort
			LOG( pSession->s_pLog, LogINF, "Aborted(ret=%d)", Ret );
			goto err2;
		}
		if( ReplyCmd != PAXOS_SESSION_EVENT ) {
			LOG( pSession->s_pLog, LogINF,
				"=== RECV j=%d pM=NULL Seq=%d TIMEOUT s_Master=%d "
				" ReplyCmd=0x%x Try=%d ###", 
			j, Seq, pSession->s_Master, ReplyCmd, pSession->s_Id.i_Try );
		}
		goto again;
	}
#endif
	pM	= (PaxosSessionHead_t*)MsgFirst( pMsg );
	/* master変更があれば送りなおす */
	switch( pM->h_Code ) {
	case PAXOS_REJECTED:
		LOG( pSession->s_pLog, LogERR, "###### REJECTED ######");
	case PAXOS_Im_MASTER:
	case PAXOS_Im_NOT_MASTER:
	case PAXOS_MASTER_ELECTED:
	case PAXOS_MASTER_CANCELED:
		LOG( pSession->s_pLog, LogERR,
			"### RETRY j=%d pM=%p %u-%u h_Cmd=0x%x h_Code=%d"
			" h_Master=%d h_From=%d s_Master=%d", 
			j, pM, pM->h_Id.i_Seq, Seq, pM->h_Cmd, pM->h_Code, pM->h_Master, 
			pM->h_From, pSession->s_Master );
		/* skip */
		if( j == pM->h_Master && pM->h_Code != PAXOS_Im_MASTER ) {
			MsgDestroy( pMsg );
			goto wait;
		}
		/* RETRY */
		MsgDestroy( pMsg );
		goto again;
	case 0:
		break;
	default:
		DBG_PRT("pM->h_Code=%d\n", pM->h_Code );
		ABORT();
	}

#ifdef	ZZZ
	if( pM->h_Cmd == PAXOS_SESSION_CLIENT_ABORT ) {
		MsgDestroy( pMsg );
		errno = EPIPE;
		goto err2;
	}
#endif

	/* 重複応答、順序チェック */
	if( pM->h_Cmd != ReplyCmd
		|| pM->h_Id.i_Seq != Seq ) {

		LOG( pSession->s_pLog, LogERR,
			"##### NOT Expected h_Cmd=0x%x(0x%x) i_Seq=%u(%u) From=%d #####", 
			pM->h_Cmd, ReplyCmd, pM->h_Id.i_Seq, Seq, pM->h_From );

		MsgDestroy( pMsg );
		goto wait;
	}
	MtxLock( &pSession->s_Mtx );

	pSession->s_Status	&= ~SB_ISSUED;
	if( Update )	pSession->s_Update	= Seq;

	MtxUnlock( &pSession->s_Mtx );

	MtxUnlock( &pSession->s_Serialize );

	return( pMsg );
err2:
	MtxLock( &pSession->s_Mtx );
	pSession->s_Status	&= ~SB_ISSUED;
	MtxUnlock( &pSession->s_Mtx );
err1:
	MtxUnlock( &pSession->s_Serialize );
	RETURN( NULL );
}

Msg_t*
PaxosSessionRecvDataByMsg( Session_t *pSession )
{
	Msg_t	*pRpl;

	QueueWaitEntry( &pSession->s_Data, pRpl, m_Lnk );

	MsgFirst( pRpl );
	MsgTrim( pRpl, sizeof(PaxosSessionHead_t) );
	return( pRpl );
}

int
PaxosSessionGetErr( Session_t *pSession )
{
	return( pSession->s_Err );
}

PaxosClientId_t*
PaxosSessionGetClientId( Session_t *pSession )
{
	if( pSession->s_Status & SB_INIT )	return( &pSession->s_Id );
	else	return( NULL );
}

int
PaxosSessionGetMyAddr( Session_t *pSession, 
				void *pH, struct sockaddr_in *pAddr )
{
	return( (pSession->s_fGetMyAddr)( pH, pAddr ) );
}

int
PaxosSessionGetMax( Session_t *pSession )
{
	return( pSession->s_Max );
}

int
PaxosSessionGetMaximum( Session_t *pSession )
{
	return( pSession->s_Maximum );
}

void*
Connect( struct Session* pSession, int j )
{
	int	fd;
	PaxosSessionCell_t*	pCell;

	pCell	= PaxosSessionGetCell( pSession );

	if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )	return( NULL );
	if( connect( fd, (struct sockaddr*)&pCell->c_aPeerAddr[j].a_Addr, 
							sizeof(struct sockaddr_in)) < 0 ) {
		close( fd );
		return( NULL );
	}
	int	flag = 1;
	setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	return( INT32_PNT( fd ) );
}

int
ConnectAdmPort( char *pName, int Id )
{
	struct sockaddr_un	AdmAddr;
	char	PATH[PAXOS_CELL_NAME_MAX];
	int	Fd;

	sprintf(PATH, PAXOS_SESSION_ADMIN_PORT_FMT, pName, Id );
	memset( &AdmAddr, 0, sizeof(AdmAddr) );
	AdmAddr.sun_family	= AF_UNIX;
	strcpy( AdmAddr.sun_path, PATH );

	Fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( Fd < 0 )	goto err;

	if( connect( Fd, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 )
		goto err1;

	return( Fd );
err1:
	close( Fd );
err:
	return( -1 );
}

void*
ConnectAdm( struct Session *pSession, int j )
{
	int	Fd;

	Fd	= ConnectAdmPort( pSession->s_pCell->c_Name, j );
	if( Fd < 0 )	goto err;
	return( INT32_PNT(Fd) );
err:
	return( NULL );
}

#ifdef	ZZZ
#else
int
ReqAndRplByAny( int Fd, char* pReq, int ReqLen, char* pRpl, int RplLen )
{
	PaxosSessionHead_t	Any;
	int	Ret;

	SESSION_MSGINIT( &Any, PAXOS_SESSION_ANY, 0, 0, sizeof(Any) + ReqLen);
	Ret = SendStream( Fd, (char*)&Any, sizeof(Any) );
	if( Ret ) {
		goto err;
	}
	Ret = SendStream( Fd, pReq, ReqLen );
	if( Ret ) {
		goto err;
	}
	Ret = RecvStream( Fd, pRpl, RplLen );
	if( Ret ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
} 
#endif

int
Shutdown( void* pVoid, int how )
{
	return( shutdown( PNT_INT32( pVoid ), how ) );
}

int
CloseFd( void* pVoid )
{
	return( close( PNT_INT32( pVoid  )) );
}

int
GetMyAddr( void* pVoid, struct sockaddr_in* pMyAddr )
{
	socklen_t	len = sizeof(struct sockaddr_in);

	getsockname( PNT_INT32(pVoid), (struct sockaddr*)pMyAddr, &len );
	return( 0 );
}

int
Send( void* pVoid, char* pBuf, size_t Len, int Flags )
{
	int	Ret;

	Ret = send( PNT_INT32(pVoid), pBuf, Len, Flags|MSG_NOSIGNAL );
	return( Ret );
}

int
Recv( void* pVoid, char* pBuf, size_t Len, int Flags )
{
	return( recv( PNT_INT32(pVoid), pBuf, Len, Flags ) );
}

