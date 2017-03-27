/******************************************************************************
*
*  PaxosSession.h 	--- header of Paxos-Session Library
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
 *
 *	概要
 *	 Paxosに通信とsession/event管理を付与した
 *	また、参照の分散機能も装備している
 *	Timer機能も有している
 *	 
 *	通信とmaster接続
 *	 通信接続後、クライアントIdによる通信端点作成をサーバに指示する
 *	どのサーバとも接続ができない場合は中止する
 *	一つでも接続があればmasterが見つかるまで永遠に再試行する
 *	masterの切り替え時も同様である
 *	 APが他の処理中にmaster切り替えが発生する
 *	サーバはクライアントにイベントを発行する
 *	クライアントは直ちに新masterに接続し、端点が刈り取られないようにする
 *	従ってクライアントはmasterと常に接続している
 *
 *	master不定状態(Jeopard)
 *	 master交替時には、不明あるいは新が旧を、旧が新を通知する状態がある
 *	クライントはこのような状態を検知したらセルからの通知を待つ
 *	一定時間経っても通知がない場合は、master探しを行う
 *
 *	セッション管理
 *	 master接続後session開始を通知する
 *	本通知はPaxosでagentに伝えられagentはsession状態を持つ
 *	agentはクライアントIdをキーとしてセッションを作成する
 *	どのreplicaのsessionも応答を含めた同一の状態を持つ
 *	TCPによるVCでserver,clientで接続監視を行う
 *	masterでは切断時にはsession断と判断する
 *	clientではmaster断と判断し一定時間後接続する
 *	新masterとなったサーバは一定時間後
 *	session状態の刈り取りを行う
 *
 *	クライアントId
 *	 (INアドレス、プロセスId、ユーザ付与番号、通番、直近更新通番)としている
 *	通信端点、セッションは(IN、プロセスId, ユーザ付与番号)で識別する
 *	トランザクションは通番、直近更新通番を含む
 *
 *	順序管理と重複
 *	 セッション毎に順序管理を行う
 *	サーバに対する重複要求はagentで行う
 *	PaxosによりどのサーバでもPaxos順序が保証される
 *	従って、master切り替えでも追い越しが発生しない
 *	要求通番がsessionの応答通番であれば応答を返す
 *	要求通番がsessionの受理通番であれば処理中なので何もしない
 *	受理時、sessionの処理済み通番が直近更新通番より小さいときは待つ
 *
 * VCの待ち管理は、動的接続切断があるのでスレッドとしている
 */

#ifndef	_PAXOS_SESSION_
#define	_PAXOS_SESSION_

#include	"Paxos.h"

#include	<netdb.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

/*
 *	Default values
 */
#define	DEFAULT_LOG_LEVEL	LogINF

#define	DEFAULT_CORE		3
#define	DEFAULT_EXTENSION	1
#define	DEFAULT_CKSUM_ON	1
#define	DEFAULT_FD_MAX		20
#define	DEFAULT_BLOCK_MAX	2000
#define	DEFAULT_SEG_SIZE	(4*1024)
#define	DEFAULT_SEG_NUM		(16)
#define	DEFAULT_WB_USEC		(1000*100)	// 100 ms

#define	DEFAULT_PAXOS_USEC	(3*1000*1000LL)	// 3 sec
#define	DEFAULT_OUT_MEM_MAX	(1024*1024*100)	// 100 MB

#define	DEFAULT_WORKERS	5

#define	PAXOS_SESSION_ADMIN_PORT_FMT	"/tmp/PAXOS_SESSION_ADMIN_PORT_%s_%d"
#define	EVENT_MAX		200

#define	TIMEOUT_SNAP	30*60*1000	// ms
#define	RETRY_MAX		20

#define	SESSION_FAMILY	0x0

/*
 *	Error
 */
#define	ERR_SESSION_RETRY_MAX	(SESSION_FAMILY|1)
#define	ERR_SESSION_NOT_FOUND	(SESSION_FAMILY|2)
#define	ERR_SESSION_MINORITY	(SESSION_FAMILY|3)

/*
 *	API
 */
typedef PaxosCell_t	PaxosSessionCell_t;

#define	PAXOS_SESSION_FAMILY_MASK	0xffff0000

typedef enum PaxosCmd {
	PAXOS_SESSION_BIND,
	PAXOS_SESSION_ELECTION,
	PAXOS_SESSION_OPEN,
	PAXOS_SESSION_CLOSE,

	PAXOS_SESSION_BACKUP,
	PAXOS_SESSION_TRANSFER,
	PAXOS_SESSION_MARSHAL,

	PAXOS_SESSION_ABORT,
	PAXOS_SESSION_EVENT,

	PAXOS_SESSION_OB_PUSH,
	PAXOS_SESSION_OB_PULL,

	PAXOS_SESSION_LOCK,
	PAXOS_SESSION_ANY,
	PAXOS_SESSION_UNLOCK,

	PAXOS_SESSION_CLIENT_ABORT,

	PAXOS_SESSION_SHUTDOWN,

#define	DT_SESSION_HEALTH		10	// sec
#define	DT_SESSION_HEALTH_MS	(DT_SESSION_HEALTH*1000)	//msec

	PAXOS_SESSION_HEALTH,
	PAXOS_SESSION_ACCEPTOR,
	PAXOS_SESSION_DUMP,
	PAXOS_SESSION_AUTONOMIC,
	PAXOS_SESSION_DATA,

	PAXOS_SESSION_CHANGE_MEMBER,
	PAXOS_SESSION_GET_MEMBER

} PaxosCmd_t;

typedef struct PaxosClientId {
	struct sockaddr_in	i_Addr;
	pid_t				i_Pid;
	int					i_No;
	time_t				i_Time;
	uint32_t			i_Reuse;
	uint32_t			i_Seq;
	uint32_t			i_Try;
	uint32_t			i_SeqWait;
} PaxosClientId_t;

typedef struct PaxosSessionHead {
	uint64_t		h_Cksum64;
	PaxosCmd_t		h_Cmd;
	int				h_Len;
	PaxosClientId_t	h_Id;
	int				h_Error;
	PaxosNotify_t	h_Code;
	int				h_From;
	int				h_Master;
	uint32_t		h_Epoch;
	int64_t			h_TimeoutUsec;
	int64_t			h_UnitUsec;
	int				h_EventCnt;
	int				h_EventOmitted;
	int				h_Reader;
	uint32_t		h_Ver;
} PaxosSessionHead_t;

#define	SESSION_MSGINIT( pF, cmd, code, error, len ) \
	({PaxosSessionHead_t*p=(PaxosSessionHead_t*)(pF); \
		memset(p,0,sizeof(PaxosSessionHead_t)); \
		/*list_init(&p->h_Paxos.p_Lnk);*/p->h_Cmd=cmd;p->h_Code=code; \
		p->h_Error=error;p->h_Len=len;p->h_Master=-1;})

#define	COMMENT_SIZE	1024
typedef struct PaxosSessionShutdownReq {
	PaxosSessionHead_t	s_Head;
	char				s_Comment[COMMENT_SIZE];
} PaxosSessionShutdownReq_t;

typedef	struct PaxosSessionChangeMemberReq {
	PaxosSessionHead_t	c_Head;
	int					c_Id;
	struct sockaddr_in	c_Addr;
} PaxosSessionChangeMemberReq_t;

typedef	struct PaxosSessionChangeMemberRpl {
	PaxosSessionHead_t	c_Head;
} PaxosSessionChangeMemberRpl_t;

typedef	struct PaxosSessionGetMemberReq {
	PaxosSessionHead_t	m_Head;
} PaxosSessionGetMemberReq_t;

typedef	struct PaxosSessionGetMemberRpl {
	PaxosSessionHead_t	m_Head;
	PaxosSessionCell_t	m_Cell;
} PaxosSessionGetMemberRpl_t;

typedef	struct PaxosSessionDumpReq {
	PaxosSessionHead_t	d_Head;
	void*			d_pStart;
	int				d_Size;
} PaxosSessionDumpReq_t;

typedef	struct PaxosSessionDumpRpl {
	PaxosSessionHead_t	d_Head;
	char			d_Data[0];
} PaxosSessionDumpRpl_t;

/*
 *	Client API
 */
/*
 * 使用方法	
 *	PaxosSessionSize		Session構造体のサイズの取得
 *	PaxosSessionInit		Session構造体の初期化
 *		fConnect				接続関数
 *			Id						サーバId(0 <= Id< PAXOS_SERVER_MAX)
 *		fShutdown				切断関数
 *			pHandle					ハンドル
 *			How						方法
 *		fClose					close関数
 *			pHandle					ハンドル
 *		fGetMyAddr				自アドレス取得関数
 *			pHandle					ハンドル
 *			pAddr					アドレス
 *		fSendTo					送信関数
 *			pHandle					ハンドル
 *			pBuff					バッファ
 *			Len						長さ
 *			Flags					フラグ	
 *		fRecvFrom				受信関数
 *			pHandle					ハンドル
 *			pBuff					バッファ
 *			Len						長さ
 *			Flags					フラグ	
 *		fMalloc					メモリ取得関数
 *			Size					長さ
 *		fFree					メモリ返却関数
 *			pAddr					アドレス
 *		pSessionCell				セルアドレス
 *	PaxosSessionDestroy		Session構造体の破壊
 *	PaxosSessionGetCell		セルアドレスの取得
 *	PaxosSessionOpen		Sessionの開始
 *		No						ユーザ付与番号
 *		Sync					同期モード(参照と更新の同期)
 *	PaxosSessionClose		Sessionの終了
 *	PaxosSessionSendToCell	Cellへのデータ送信
 *		N						ベクタの数
 *		aVec					ベクタ配列
 *	PaxosSessionReqAndRplByMsg	トランザクションの送受信
 *		pReq					要求
 *		ppRpl					応答
 *		Update					更新
 *		Forever					永久に待つ
 *	PaxosSessionGetMaster	マスターの参照
 *	PaxosSessionGetReader	最小負荷のレプリカ
 *	PaxosSessionLock		サーバーのlock	
 *		Id						サーバーId
 *		ppHandle				通信ハンドル
 *	PaxosSessionUnlock		サーバーのunlock	
 *		pHandle					通信ハンドル
 *	PaxosSessionAny			特殊メッセージの送受信
 *		pHandle					通信ハンドル
 *		pM						特殊メッセージ
 *	PaxosSessionLogCreate	loggerの生成
 *		Size					バッファサイズ
 *		Flags					フラグ
 *		pFile					ファイル構造体
 *	PaxosSessionLogDestroy	loggerの破壊
 *	PaxosSessionGetLog		loggerの参照
 *	PaxosSessionSetLog		loggerの設定
 *		pLog					logger
 *	PaxosSessionDumpMem		メモリデータの取得	
 *		pStart
 *		Size
 *
 */

#ifdef	_PAXOS_SESSION_CLIENT_
typedef void*	pHandle;

typedef struct Server {
	bool_t				s_Binded;
	time_t				s_CheckLast;
	struct sockaddr_in	s_PeerAddr;
	struct sockaddr_in	s_MyAddr;
	FdEvent_t			*s_pEvent;
} Server_t;

#define	SB_INIT		0x1
#define	SB_ACTIVE	0x2
#define	SB_ISSUED	0x4
#define	SB_CLOSED	0x8
#define	SB_WAIT		0x10

typedef struct Session {
	list_t			s_Lnk;
	Mtx_t			s_Serialize;
	int				s_Err;
	int				s_Status;
	int				s_Master;
	int				s_NewMaster;
	int				s_Reader;
	PaxosClientId_t	s_Id;
	uint32_t		s_Update;
	int64_t			s_MasterElectionUsec;
	int64_t			s_UnitUsec;
	Mtx_t			s_Mtx;
	Cnd_t			s_CndMaster;
	int				s_Max;
	int				s_Maximum;
	Server_t		*s_pServer;		// [PAXOS_SERVER_MAXIMUM];
	int				*s_pJeopard;	// [PAXOS_SERVER_MAX+1];

#define	J_IND( j )	( (j) < 0 ? pSession->s_Max : (j) )
#define	J_SET( j )	pSession->s_pJeopard[J_IND(j)]++
#define	CLEAR_JEOPARD \
	/*LOG(pSession->s_pLog,"CLEAR_JEOPARD");*/ \
	{int ja;for( ja = 0; ja < pSession->s_Max + 1; ja++ ) { \
		if( ja < pSession->s_Max )	pSession->s_pServer[ja].s_CheckLast = 0; \
		pSession->s_pJeopard[ja] = 0; \
	}} 

	Queue_t			s_Post;
	Queue_t			s_Recv;
	pthread_t		s_RecvThread;
	Queue_t			s_Data;
	pHandle			(*s_fConnect)(struct Session*,int j);
	int				(*s_fGetMyAddr)(pHandle,struct sockaddr_in*);
	int				(*s_fShutdown)(pHandle,int);
	int				(*s_fClose)(pHandle);
	int				(*s_fSendTo)(pHandle,char*,size_t,int);
	int				(*s_fRecvFrom)(pHandle,char*,size_t,int );
	void*			(*s_fMalloc)(size_t);
	void			(*s_fFree)(void*);

	PaxosSessionCell_t			*s_pCell;

	struct Log*		s_pLog;
	bool_t			s_Sync;

	void*			s_pVoid;
	time_t			s_NoMaster;
} Session_t;
#else // _PAXOS_SESSION_CLIENT_
struct Session;
#endif // _PAXOS_SESSION_CLIENT_

extern	int PaxosSessionGetSize(void);
extern	struct Session* PaxosSessionAlloc(
			void*(*fConnect)(struct Session*,int Id),
			int(*fShutdown)(void*pHandle,int How),
			int(*fClose)(void*pHandle),
			int(*fGetMyAddr)(void*pHandle,struct sockaddr_in*pAddr),
			int(*fSendTo)(void*pHandle,char*pBuff,size_t Len,int Flags),
			int(*fRecvFrom)(void*pHandle,char*pBuff,size_t Len,int Flags),
			void*(*fMalloc)(size_t Size),
			void(*fFree)(void*pAddr),
			char*pSessionCellName );
extern	int	PaxosSessionFree( struct Session* pSession );

extern	PaxosSessionCell_t* PaxosSessionGetCell( struct Session* pSession );
extern	int PaxosSessionOpen( struct Session* pSession, int No, bool_t Sync );
extern	int PaxosSessionClose( struct Session* pSession );
extern	int PaxosSessionAbort( struct Session* pSession );
extern	struct Session *PaxosSessionFind( char *pCell, int No );

extern	int PaxosSessionSendToCellByMsg( struct Session* pSession, Msg_t*); 
extern	Msg_t* PaxosSessionReqAndRplByMsg( struct Session* pSession,
								Msg_t*, bool_t bUpdate, bool_t bForEver );
extern	Msg_t* PaxosSessionRecvDataByMsg( struct Session* pSession );
extern	int PaxosSessionGetErr( struct Session* pSession ); 

extern	int PaxosSessionGetMaster( struct Session* pSession );
extern	int PaxosSessionGetReader( struct Session* pSession );

extern	int	PaxosSessionLock( struct Session*, int Id, void** ppHandle );
extern	int	PaxosSessionUnlock( struct Session*, void* pHandle );
extern	PaxosSessionHead_t*	PaxosSessionAny( struct Session*, void* pHandle, 
				PaxosSessionHead_t* pM );

extern	int PaxosSessionLogCreate(struct Session*, int Size, int Flags, 
									FILE* pFile, int Level );
extern	void PaxosSessionLogDestroy(struct Session*);
extern	struct Log* PaxosSessionGetLog( struct Session* );
extern	int	PaxosSessionSetLog( struct Session*, struct Log* pLog );

extern	void*	(*PaxosSessionGetfMalloc(struct Session*))(size_t);
extern	void	(*PaxosSessionGetfFree(struct Session*))(void*);

extern	PaxosClientId_t* PaxosSessionGetClientId( struct Session* );
extern	int	PaxosSessionGetMyAddr(struct Session*,
						void*pHandle,struct sockaddr_in*pAddr);
extern	void*	PaxosSessionGetTag( struct Session* );
extern	void	PaxosSessionSetTag( struct Session*, void* );
extern	int		PaxosSessionChangeMember( struct Session *pSession, 
							int Id, struct sockaddr_in *pAddr );

extern	int	PaxosSessionGetMax( struct Session *pSession );
extern	int	PaxosSessionGetMaximum( struct Session *pSession );
extern	int		PaxosSessionDumpMem( struct Session *pSession, void *pHandle,
							void *pRemote, void *pLocal, size_t Length );
/*
 *	Server API
 */
/*
 * 使用方法	
 *	PaxosAcceptorSize	Acceptor構造体のサイズの取得
 *	PaxosAcceptorInit	Acceptor構造体の初期化
 *		Id					サーバId(0 <= Id< PAXOS_SERVER_MAX)
 *		pSessionCell		sessionセルアドレス
 *		pPaxosCell			Paxosセルアドレス
 *		UnitSec				Paxos単位時間
 *		pIF					インターフェース関数
 *			if_fWritePaxos		Paxosスナップショット用write関数
 *			if_fReadPaxos		Paxosスナップショット用read関数
 *			if_fMalloc			メモリ取得関数
 *			if_fFree			メモリ解放関数
 *			if_fSessionOpen		session開設関数
 *			if_fSessionClose	session閉鎖関数
 *			if_fRequest			要求受付関数
 *			if_fValidate		データ確認関数
 *			if_fAccepted		合意実行関数
 *			if_fRejected		要求拒否関数
 *			if_fAbort			アボート関数
 *			if_fShutdown		shutdown関数
 *			if_fSnapAdaptorRead		アダプターread関数
 *			if_fOutOfBandSave		帯域外データsave関数
 *			if_fOutOfBandLoad		帯域外データload関数
 *			if_fOutOfBandRemove		帯域外データ削除関数
 *			if_fLock			sessionロック関数
 *			if_fUnlock			sessionアンロック関数
 *			if_fAny				特殊メッセージ処理関数
 *			if_fLoad			負荷計算関数
 *	PaxosAcceptorStart		Acceptorの開始
 *	PaxosAcceptorSetTag		Adaptorの設定
 *		Family					ファミリ
 *		pTag					Adaptor
 *	PaxosAcceptorGetTag		Adaptorの参照
 *		Family					ファミリ
 *	PaxosAcceptIsAccepted	実行のチェック
 *		pId						クライアントID
 *	PaxosAcceptReplyByMsg	応答の送信(マスターであれば送信）
 *		pRpl					応答
 *	PaxosAcceptSend			応答の直接送信
 *		pRpl					応答
 *	JOINT					イベントとの接続
 *		pW						Adaptorの待ちリンク
 *		pE						イベントのリンク
 *		pJ						ジョイント
 *	PaxosAcceptEventRaise	イベントの励起
 *		pEvent					イベントメッセージ
 *	PaxosAcceptorGetLoad	受付Fd数(負荷計算用）
 *	PaxosAcceptorLogCreate	logger生成
 *	PaxosAcceptorLogDestroy	logger破壊
 *	PaxosAcceptorGetLog		logger参照
 *	PaxosAcceptorSetLog		logger設定
 */

#define	PATH_NAME_MAX	512

#define	PAXOS_SESSION_OB_PATH	"DATA"

struct PaxosAcceptor;
struct PaxosAccept;

typedef struct PaxosAcceptorIF {

	void*	(*if_fMalloc)(size_t);
	void	(*if_fFree)(void*);

	int		(*if_fSessionOpen)(struct PaxosAccept*,PaxosSessionHead_t*);
	int		(*if_fSessionClose)(struct PaxosAccept*,PaxosSessionHead_t*);

	int		(*if_fRequest)(struct PaxosAcceptor*,PaxosSessionHead_t*,int);
	int		(*if_fValidate)(struct PaxosAcceptor*,uint32_t,
											PaxosSessionHead_t*,int);
	int		(*if_fAccepted)(struct PaxosAccept*,PaxosSessionHead_t*);

	int		(*if_fRejected)(struct PaxosAcceptor*,PaxosSessionHead_t*);
	int		(*if_fAbort)(struct PaxosAcceptor*,uint32_t);
	int		(*if_fShutdown)(struct PaxosAcceptor*,
						PaxosSessionShutdownReq_t*, uint32_t nextDecide);

	Msg_t*	(*if_fBackupPrepare)(int);
	int		(*if_fBackup)(PaxosSessionHead_t*);
	int		(*if_fTransferSend)(IOReadWrite_t*);
	int		(*if_fTransferRecv)(IOReadWrite_t*);
	int		(*if_fRestore)(void);

	int		(*if_fGlobalMarshal)(IOReadWrite_t*);
	int		(*if_fGlobalUnmarshal)(IOReadWrite_t*);

	int		(*if_fAdaptorMarshal)(IOReadWrite_t*,struct PaxosAccept*);
	int		(*if_fAdaptorUnmarshal)(IOReadWrite_t*,struct PaxosAccept*);

	int		(*if_fLock)(void);
	int		(*if_fAny)(struct PaxosAcceptor*, PaxosSessionHead_t*,int);
	int		(*if_fUnlock)(void);

	int		(*if_fOutOfBandSave)(struct PaxosAcceptor*,PaxosSessionHead_t*);
	PaxosSessionHead_t*
			(*if_fOutOfBandLoad)(struct PaxosAcceptor*,PaxosClientId_t*);
	int		(*if_fOutOfBandRemove)(struct PaxosAcceptor*,PaxosClientId_t*);

	int		(*if_fLoad)(void*);

} PaxosAcceptorIF_t;

typedef	struct PaxosAcceptorFdEvent {
	FdEvent_t				e_FdEvent;
	IOSocket_t				e_IOSock;
	struct PaxosAccept		*e_pAccept;
} PaxosAcceptorFdEvent_t;

typedef struct Joint {
	list_t	j_Wait;
	list_t	j_Event;
} Joint_t;

#define	JOINT_INIT( pJ ) \
	({list_init(&(pJ)->j_Wait);list_init(&(pJ)->j_Event);})

#define	JOINT( pW, pE, pJ ) \
	({list_add(&(pJ)->j_Wait,pW);list_add(&(pJ)->j_Event,pE);})

#define	UNJOINT( pJ ) \
	({list_del_init(&(pJ)->j_Wait);list_del_init(&(pJ)->j_Event);})

extern	void*	(*PaxosAcceptorGetfMalloc(struct PaxosAcceptor*))(size_t);
extern	void	(*PaxosAcceptorGetfFree( struct PaxosAcceptor* ))(void*);
extern	void*	(*PaxosAcceptGetfMalloc( struct PaxosAccept* ))(size_t);
extern	void	(*PaxosAcceptGetfFree( struct PaxosAccept* ))(void*);

extern	int PaxosAcceptorGetSize(void);
extern	int PaxosAcceptorInitUsec( struct PaxosAcceptor *pAcceptor, int Id, 
		int	ServerCore, int ServerExtension, int64_t MemLimit, bool_t bCksum,
		PaxosSessionCell_t	*pSessionCell,
		PaxosCell_t			*pPaxosCell,
		int64_t 			UnitUsec,
		int					Workers,
		PaxosAcceptorIF_t* );

extern	int PaxosAcceptorStartSync( struct PaxosAcceptor* pAcceptor,
				PaxosStartMode_t StartMode, uint32_t nextDecide,
				int (*fInitSync)(int tartget) );
extern	struct Paxos* PaxosAcceptorGetPaxos( struct PaxosAcceptor* pAcceptor );
extern	PaxosSessionCell_t* PaxosAcceptorGetCell(struct PaxosAcceptor* );
extern	int PaxosAcceptorMyId( struct PaxosAcceptor *pAcceptor );
extern	void* PaxosAcceptGetTag( struct PaxosAccept* pAccept, int Family );
extern	void PaxosAcceptSetTag( struct PaxosAccept* pAccept, 
										int Family, void* pTag );
extern	struct PaxosAccept* PaxosAcceptGetToMine( 
								struct PaxosAcceptor* pAcceptor,
								PaxosClientId_t* pClientId,
								int fd );
extern	struct PaxosAccept* PaxosAcceptGet( struct PaxosAcceptor* pAcceptor,
					PaxosClientId_t* pId, bool_t bCreate );
extern	int PaxosAcceptPut( struct PaxosAccept* pAccept );
extern	PaxosClientId_t* PaxosAcceptorGetClientId( 
					struct PaxosAccept* pAccept );
extern	bool_t PaxosAcceptIsAccepted( struct PaxosAccept* pAccept, 
									PaxosSessionHead_t* pId );
extern	bool_t PaxosAcceptIsDupricate( struct PaxosAccept* pAccept, 
									PaxosSessionHead_t* pPSH );
extern	int PaxosAcceptSend( struct PaxosAccept* pAccept, 
									PaxosSessionHead_t* pRplHead );
extern	int PaxosAcceptSendDataByMsg( struct PaxosAccept* pAccept, Msg_t *pM);

extern	int PaxosAcceptReplyByMsg( struct PaxosAccept* pAccept, Msg_t* pMsg );
extern	int PaxosAcceptReplyReadByMsg( struct PaxosAccept* pAccept, 
									Msg_t* pMsg );

extern	TimerEvent_t* 
		PaxosAcceptorTimerSet( struct PaxosAcceptor* pAcceptor, int64_t Ms, 
					int(*fEvent)(TimerEvent_t*), void* pArg );
extern	PaxosSessionHead_t* 
		PaxosAcceptorRecvMsg( struct PaxosAcceptor *pAcceptor, int fd );
extern	void* PaxosAcceptorMain( void* pArg );
extern	struct PaxosAccept* PaxosAcceptOpen( struct PaxosAcceptor* pAcceptor, 
						PaxosSessionHead_t* pH );
extern	int PaxosAcceptOpenReply( struct PaxosAccept* pAccept,
				PaxosSessionHead_t* pH, int Error );
extern	int PaxosAcceptClose( struct PaxosAccept* pAccept,
					PaxosSessionHead_t* pH, int Error );
extern	int PaxosAcceptSnapshotReply( struct PaxosAccept* pAccept,
			PaxosSessionHead_t* pH, int Error );

extern	struct sockaddr_in*	PaxosAcceptorGetAddr( 
						struct PaxosAcceptor* pAcceptor, int j );

extern	int	PaxosAcceptOutOfBandLock(struct PaxosAccept*);
extern	int	PaxosAcceptOutOfBandUnlock(struct PaxosAccept*);

extern	PaxosSessionHead_t*	
			_PaxosAcceptOutOfBandGet(struct PaxosAccept*, PaxosClientId_t*);
extern	int PaxosAcceptorOutOfBandValidate( struct PaxosAcceptor*, uint32_t,
									PaxosSessionHead_t*, int );

extern	int PaxosAcceptedStart( struct PaxosAccept*, PaxosSessionHead_t* );

extern	int	PaxosAcceptHold( struct PaxosAccept* );
extern	int	PaxosAcceptRelease( struct PaxosAccept* );

extern	int PaxosAcceptorGetLoad( struct PaxosAcceptor* );

extern	int PaxosAcceptorLogCreate(struct PaxosAcceptor*, int Size, int Flags, 
								FILE*, int Level);
extern	void PaxosAcceptorLogDestroy(struct PaxosAcceptor*);
extern	struct Log* PaxosAcceptorGetLog( struct PaxosAcceptor* );
extern	int PaxosAcceptorSetLog(struct PaxosAcceptor*, struct Log*);

extern	struct Log* PaxosAcceptGetLog( struct PaxosAccept* );

extern	int	PaxosAcceptEventRaise( struct PaxosAccept *pAccept,
									PaxosSessionHead_t* pEvent );
extern	int	PaxosAcceptEventGet( struct PaxosAccept *pAccept,
									PaxosSessionHead_t* pH );
extern	int PaxosAcceptorUnmarshal( IOReadWrite_t *pR, 
									struct PaxosAcceptor* pAcceptor );
extern	int PaxosSessionExport( struct PaxosAcceptor *pAcceptor,
				struct Session *pSession, int j, IOReadWrite_t *pW );
extern	int PaxosSessionImport( struct PaxosAcceptor *pAcceptor, 
				IOReadWrite_t* pIO );

extern	struct PaxosAcceptor*	 PaxosAcceptGetAcceptor( struct PaxosAccept* ); 
extern	int	PaxosAcceptorFreeze( struct PaxosAcceptor *pAcceptor,
										PaxosAcceptorFdEvent_t *pEv );
extern	int PaxosAcceptorDefreeze( struct PaxosAcceptor *pAcceptor );

#ifdef	_PAXOS_SESSION_SERVER_

/*
 *	Session
 */
typedef struct PaxosAcceptor {
	Mtx_t				c_Mtx;
	int					c_MyId;
	PaxosSessionCell_t	*c_pSessionCell;
	struct Paxos*		c_pPaxos;
	bool_t				c_bCksum;
	GElmCtrl_t			c_Accept;

	struct {
		Mtx_t	o_Mtx;
		list_t	o_Lnk;
		int		o_Cnt;
		int64_t	o_MemLimit;
		int64_t	o_MemUsed;
		Hash_t	o_Hash;
	} 					c_OutOfBand;
	int					*c_pFds;
	Timer_t				c_Timer;
	bool_t				c_Enabled;
	PaxosAcceptorIF_t	c_IF;
	int					c_MasterWas;

	struct {
		FdEventCtrl_t	f_Ctrl;
	} 					c_FdEvent;

	PaxosAcceptorFdEvent_t *c_pFdFreeze;
	int				c_Workers;
	Queue_t			*c_aMsg;

	struct Log*		c_pLog;
} PaxosAcceptor_t;

typedef	struct ThreadAcceptorArg {
	PaxosAcceptor_t	*a_pAcceptor;
	int				a_No;
} ThreadAcceptorArg_t;

typedef enum PaxosAcceptOpen {
	ACCEPT_BINDED	= 0,
	ACCEPT_OPENING	= 1,
	ACCEPT_OPENED	= 2,
	ACCEPT_CLOSING	= 3,
} PaxosAcceptOpen_t;

typedef struct PaxosAccept {
	GElm_t				a_GE;
	Mtx_t				a_Mtx;
	PaxosAcceptorFdEvent_t *a_pFd;
	PaxosClientId_t		a_Id;
	PaxosAcceptOpen_t	a_Opened;
	struct {
		Cnd_t		a_Cnd;
		int32_t		a_Cnt;
		uint32_t	a_Start;	// 要求を受け付けた
		uint32_t	a_End;		// 要求を処理した
		uint32_t	a_Reply;	// 応答
		bool_t		a_Master;
		int			a_Cmd;
	}					a_Accepted;
	struct {
		int		r_Cnt;
		bool_t	r_Hold;
		list_t	r_Msg;
	}					a_Rpls;
	struct {
		int		e_Cnt;
		int		e_Omitted;
		bool_t	e_Send;
		Msg_t	*e_pRaised;
	} 					a_Events;
	PaxosAcceptor_t*	a_pAcceptor;
	Hash_t				a_Tag;
	time_t				a_Create;
	time_t				a_Access;
	time_t				a_Health;
} PaxosAccept_t;

typedef struct AcceptorOutOfBand {
	list_t				o_Lnk;
	PaxosClientId_t		o_Id;
	PaxosSessionHead_t*		o_pData;
	time_t				o_Last;
	time_t				o_Create;
	bool_t				o_bValidate;
	uint32_t			o_Seq;
	bool_t				o_bSaved;
} AcceptorOutOfBand_t;

#define	SESSION_MSGALLOC( cmd, code, error, len ) \
	({PaxosSessionHead_t* pF; \
		pF = (PaxosSessionHead_t*)(pAcceptor->c_IF.if_fMalloc)( len ); \
		SESSION_MSGINIT( pF, cmd, code, error, len ); pF;})

#define	SESSION_MSGFREE( pF )	(pAcceptor->c_IF.if_fFree)( pF );

#define	ACCEPT_REPLY_MSG( a, m, p, s, l ) \
	({Msg_t		*_pMsgRpl; \
	MsgVec_t	_Vec; \
	int	_ret; \
	_pMsgRpl = MsgCreate( 1,	PaxosAcceptGetfMalloc((a)), \
								PaxosAcceptGetfFree((a))); \
	MsgVecSet( &_Vec, VEC_MINE, (p), (s), (l) ); \
	MsgAdd( _pMsgRpl, &_Vec ); \
	if( m ) _ret = PaxosAcceptReplyByMsg( (a), _pMsgRpl ); \
	else	_ret = PaxosAcceptReplyReadByMsg( (a), _pMsgRpl ); \
	_ret;})

typedef	struct PaxosSessionAcceptorReq {
	PaxosSessionHead_t	a_Head;
} PaxosSessionAcceptorReq_t;

typedef	struct PaxosSessionAcceptorRpl {
	PaxosSessionHead_t	a_Head;
	PaxosAcceptor_t		a_Acceptor;
	PaxosAcceptor_t	*	a_pAcceptor;
} PaxosSessionAcceptorRpl_t;


#endif	// _PAXOS_SESSION_SERVER_

extern	struct in_addr PaxosGetAddrInfo( char *pNode );
extern	PaxosCell_t*	PaxosCellGetAddr( char *pCellName, 
										void*(*fMalloc)(size_t) );
extern	PaxosSessionCell_t*	PaxosSessionCellGetAddr( char *pCellName,
										void*(*fMalloc)(size_t) );

extern	void*	Connect( struct Session* pSession, int j );
extern	int		ConnectAdmPort( char *pName, int Id );
extern	int 	ReqAndRplByAny( int Fd, char* pReq, int ReqLen, 
										char* pRpl, int RplLen );
extern	void*	ConnectAdm( struct Session* pSession, int j );
extern	int		Shutdown( void* pVoid, int how );
extern	int		CloseFd( void* pVoid );
extern	int		GetMyAddr( void* pVoid, struct sockaddr_in* pMyAddr );
extern	int		Send( void* pVoid, char* pBuf, size_t Len, int Flags );
extern	int		Recv( void* pVoid, char* pBuf, size_t Len, int Flags );

#endif	// _PAXOS_SESSION_
