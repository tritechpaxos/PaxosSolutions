/******************************************************************************
*
*  Paxos.h 	--- header of Paxos Library
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
 * 使用方法	
 *	PaxosGetSize	Paxos構造体のサイズの取得
 *	PaxosInitUsec	Paxos構造体の初期化
 *		Id				サーバId(0 <= Id< PAXOS_SERVER_MAX)
 *		aCell			サーバアドレス
 *		UnitUSec		監視時間単位(u秒)
 *		fRecvFrom		受信関数
 *		fSendTo			送信関数
 *		fElection		リーダイベント通知関数
 *			Code			通知コード
 *			Epoch			エポック
 *			FromServer		イベント惹起サーバ
 *		fNotify			通知関数
 *			Mode			Paxos段階モード
 *			Code			通知コード
 *			Seq				実行インスタンス通番
 *			Server			コマンド受付サーバ/master
 *			Epoch			エポック
 *			Len				コマンド長さ
 *			pCmd			コマンド実体
 *			From			データ取得先サーバ
 *		fWritePaxos		ファイル書き出し
 *		fReadPaxos		ファイル読み込み
 *		fMalloc			メモリ取得関数
 *		fFree			メモリ解放関数
 *		fLoad			負荷取得関数
 *	PaxosStart		Paxosの開始
 *	PaxosRequest	Paxosへのコマンド投入
 *	PaxosGetSeq		現在通番の取得
 *	PaxosMyId					サーバId取得	
 *	PaxosIsIamMaster			マスターか
 *	PaxosGetMaster				マスター取得
 *	PaxosMasterElectionTimeout	最大master選択時間
 *	PaxosDecidedTimeout			最大決定時間
 *
 *	PaxosSetTag				Tag設定
 *	PaxosGetTag				Tag参照
 *	PaxosRecvFromPaxos		実体コマンドの取得
 *
 *	PaxosFreeze				Paxosの凍結
 *	PaxosDefreeze			Paxosの解凍
 *	PaxosMarshal			Paxosのエンコード
 *	PaxosUnmarshal			Paxosのデコード
 *	PaxosSnapshotValidate	Unmarshal後の自分化
 *
 *	PaxosHighWater			投入停止
 *	PaxosLowWater			投入再開
 *
 *	使用手順(概略）
 *		PaxosInitで初期化
 *		PaxosSetTagでAPのタグ付け
 *		APスレッド起動
 *			PaxosRecvFromPaxosで実体コマンドを待つ
 *		PaxosRequestで実体コマンド投入	
 */

#include	"NWGadget.h"

#ifndef		__PAXOS__
#define	__PAXOS__
/*
 *	API
 */
#define	PAXOS_ADMIN_PORT_FMT	"/tmp/PAXOS_ADMIN_PORT_%s_%d"

#define	PAXOS_COMMAND_SIZE		1300//512
#define	PAXOS_MAX				10/*20*/
#define	PAXOS_FIRE_MAX	20
#define	PAXOS_PAGE_MAX_LEN		1000//1---for Test

#define	PAXOS_WATER_DT_MS		10000

#define	PAXOS_CELL_NAME_MAX	256

typedef	struct PaxosAddr {
	bool_t				a_Active;
	char				a_Host[PAXOS_CELL_NAME_MAX];
	struct	sockaddr_in	a_Addr;
} PaxosAddr_t;

typedef struct PaxosCell {
	uint32_t		c_Version;
	char			c_Name[PAXOS_CELL_NAME_MAX];
	int				c_Max;
	int				c_Maximum;
	PaxosAddr_t		c_aPeerAddr[0];
} PaxosCell_t;

#define	PaxosCellSize( n )	(sizeof(PaxosCell_t)+sizeof(PaxosAddr_t)*(n))

typedef struct PaxosActive {
	int			a_Target;
	int32_t		a_Max;
	int32_t		a_Maximum;
	int32_t		a_Leader;
	int32_t		a_Epoch;
	int32_t		a_Load;
	uint32_t	a_NextDecide;
	uint32_t	a_BackPage;
} PaxosActive_t;

struct	Paxos;

typedef enum PaxosNotify {
	PAXOS_OK = 0,
	PAXOS_NOOP,
	PAXOS_NOT_ACTIVE,
	PAXOS_Im_NOT_MASTER,
	PAXOS_Im_MASTER,
	PAXOS_MASTER_ELECTED,
	PAXOS_MASTER_CANCELED,
	PAXOS_REJECTED,
	PAXOS_ACCEPTED,
	PAXOS_NOT_GET_PAGE,

	PAXOS_RECOVER,
	PAXOS_VALIDATE,
} PaxosNotify_t;

typedef struct PaxosHead {
	Dlnk_t			p_Lnk;
	int				p_Len;
} PaxosHead_t;

typedef struct PaxosExecute {
	uint32_t		e_Seq;
	bool_t			e_Validate;
	uint32_t		e_BackSeq;
	PaxosNotify_t	e_Notify;
} PaxosExecute_t;

typedef	enum PaxosMode {
	PAXOS_MODE_NORMAL,
	PAXOS_MODE_REQUEST,
	PAXOS_MODE_BEGIN,
	PAXOS_MODE_REDIRECT,
	PAXOS_MODE_ACCEPT,
	PAXOS_MODE_SUCCESS,
	PAXOS_MODE_RECOVER,
	PAXOS_MODE_CATCHUP
} PaxosMode_t;

extern	int PaxosRecvFrom( int RecvFd, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t *pAddrLen );
extern	int PaxosSendTo( struct Paxos*, void* p, size_t len, int flag,
			struct sockaddr_in *pAddr, socklen_t AddrLen );

extern	int PaxosLogCreate( struct Paxos* pPaxos, int Size, int Flags, 
						FILE* pFile, int Level );
extern	void PaxosLogDestroy( struct Paxos* pPaxos );
extern	struct Log*	PaxosGetLog( struct Paxos* pPaxos );
extern	int	PaxosSetLog( struct Paxos* pPaxos, struct Log* pLog );

extern	int	PaxosGetSize(void);

extern	int	PaxosGetMax( struct Paxos* );
extern	int	PaxosGetMaximum( struct Paxos* );

extern	int PaxosInitUsec( struct Paxos *pPaxos, int Id, 
		int	ServerCore, int	ServerExtension,
		PaxosCell_t* pCell,
		int64_t UnitUsec, bool_t bCksum,
		int(*fElection)(struct Paxos*,
				PaxosNotify_t	Code,
				int				Elected,
				uint32_t 		Epoch,
				int32_t			EventFromServer ),
		int(*fNotify)(struct Paxos*,
				PaxosMode_t		Mode,
				PaxosNotify_t	Code,
				uint32_t 		Seq,
				int				Server,
				uint32_t 		Epoch,
				int32_t			Len,
				void*			pCmd,
				int				From ),
		int	(*fOutOfBandMarshal)(struct Paxos*,void*,IOReadWrite_t*),
		int	(*fOutOfBandUnmarshal)(struct Paxos*,void*,IOReadWrite_t*),

		void*(*fMalloc)(size_t),
		void(*fFree)(void*),
		int(*fLoad)(void*) );

typedef enum PaxosStartMode {
	PAXOS_WAIT = 0,		// synchronize and wait decision	
	PAXOS_NO_SEQ,		// synchronize and join into decision
	PAXOS_SEQ			// synchronize/restart and join into decision 
} PaxosStartMode_t;

extern	int PaxosStart( struct Paxos* pPaxos, 
				PaxosStartMode_t StartMode, uint32_t nextDecide );
extern	int PaxosRequest( struct Paxos* pPaxos, 
					int len, void* pCmd, bool_t validate ); 
extern	int PaxosRequestTry( struct Paxos* pPaxos, 
					int len, void* pCmd, bool_t validate ); 
extern	uint32_t	PaxosGetSeq( struct Paxos* pPaxos );
extern	uint32_t	PaxosGetNextDecide( struct Paxos* pPaxos );
extern	int	PaxosMyId( struct Paxos* pPaxos );
extern	bool_t PaxosIsIamMaster( struct Paxos* pPaxos );
extern	int	PaxosGetMaster( struct Paxos* pPaxos );
extern	int64_t	PaxosMasterElectionTimeout( struct Paxos* pPaxos );
extern	int64_t	PaxosDecidedTimeout( struct Paxos* pPaxos );
extern	timespec_t	PaxosCatchupRecvLast( struct Paxos* pPaxos );
extern	uint32_t	PaxosEpoch( struct Paxos* pPaxos );
extern	int PaxosDown( struct Paxos* pPaxos );
extern	void	PaxosSetTag( struct Paxos* pPaxos, void* pTag );
extern	void*	PaxosGetTag( struct Paxos* pPaxos );
extern	int64_t	PaxosUnitUsec( struct Paxos* pPaxos );
extern	int	PaxosGetReader( struct Paxos* pPaxos );
extern	struct sockaddr_in*	PaxosGetAddr( struct Paxos* pPaxos, int j );

extern	Msg_t*	PaxosRecvFromPaxos(struct Paxos*);

extern	int	PaxosFreeze(struct Paxos*);
extern	int	PaxosDefreeze(struct Paxos*);
extern	int	PaxosMarshal(struct Paxos*, IOReadWrite_t*);
extern	int	PaxosUnmarshal(struct Paxos*, IOReadWrite_t*);
extern	int PaxosSnapshotValidate( struct Paxos *pPaxos );
extern	int PaxosAutonomic( struct Paxos *pPaxos, int server );

extern	timespec_t	PaxosGetAliveTime( struct Paxos *pPaxos, int j );

extern	int	PaxosHighWater(struct Paxos*);
extern	int	PaxosLowWater(struct Paxos*);

#ifdef		_PAXOS_SERVER_

struct MetaCmd;

/* time unit is changed to usec */

#define	DT_MSEC			1000
#define	DT_SEC			(1000*DT_MSEC)

#define	DT_UNIT_MIN		20*1000		// 20msec

#define	DT_UNIT	(pPaxos->p_UnitUsec)
#define	DT_L	(10*DT_UNIT)		// execution (実際はValidate)
#define	DT_D	(DT_UNIT)			// communication latency(輻輳を考慮)
#define	DT_C	(DT_UNIT)			// check
#define	DT_Z	(DT_UNIT)			// heart beat

typedef struct PaxosTimeUnits {
	int32_t	u_DT_UNIT;
	int32_t	u_DT_L;
	int32_t	u_DT_D;
	int32_t	u_DT_C;
	int32_t	u_DT_Z;
} PaxosTimeUnits;

#define	DT_CON		(2*DT_D+2*DT_Z)
#define	DT_LEADER	(3*DT_Z+3*DT_D)

#define	DT_NEWROUND	(2*DT_D+2*pPaxos->p_Max*DT_L+DT_L)
#define	DT_SUCCESS	(3*DT_L+2*pPaxos->p_Max*DT_L+2*DT_D)
#define	DT_DECIDE	(DT_LEADER+2*DT_SUCCESS+DT_L+DT_SUCCESS \
						+pPaxos->p_Max*DT_L+DT_D+DT_L)

#define	DT_CACHE_SEC	30	// restore time	(DT_CON+2*DT_D)

#define	DT_AUTONOMIC_SEC	30	// re-join after restore

/*
 *	Memory
 */
static	inline	void* PaxosMalloc( struct Paxos *pPaxos, size_t s );
static	inline	void PaxosFree( struct Paxos *pPaxos, void *p );

typedef struct Quorum {
	int32_t		q_total;	
	int32_t		q_count;
	uint32_t	q_member;
} Quorum_t;

static inline	void
quorum_init( Quorum_t* pq, int32_t total )
{
	pq->q_total		= total;
	pq->q_count		= 0;
	pq->q_member	= 0;
}

static inline	bool_t
quorum_exist( Quorum_t* pq, int32_t j )
{
	return( pq->q_member & (1<<j) );
}

static inline bool_t
quorum_add( Quorum_t* pq, int32_t j )
{
	if( quorum_exist( pq, j ) )	return( FALSE );

	pq->q_member	|= 1<<j;
	pq->q_count++;

	return( TRUE );
}

static inline	bool_t
quorum_del( Quorum_t* pq, int32_t j )
{
	if( !quorum_exist( pq, j ) )	return( FALSE );

	pq->q_member	&= ~(1<<j);
	pq->q_count--;

	return( TRUE );
}

static inline	bool_t
quorum_majority( Quorum_t* pq )
{
	if( pq->q_count >= pq->q_total/2 + 1 ) {
		return( TRUE );
	} else {
		return( FALSE );
	}
}

/*
 *	Message
 */
typedef	enum msg_cmd {
	MSG_DUMP				= 0,
	MSG_CONTROL_START		= 1,
	MSG_CONTROL_STOP		= 2,
	MSG_OUTBOUND_STOP		= 3,
	MSG_OUTBOUND_RESTART	= 4,
	MSG_DOWN				= 5,
	MSG_ALIVE				= 6,
	MSG_CATCHUP_REQ			= 7,
	MSG_CATCHUP_RPL			= 8,
	MSG_COLLECT				= 9,
	MSG_LAST				= 10,
	MSG_OLDROUND			= 11,
	MSG_BEGIN				= 12,
	MSG_ACCEPT				= 13,
	MSG_SUCCESS				= 14,
	MSG_RECV_STOP			= 15,
	MSG_BEGIN_REDIRECT		= 16,
	MSG_BIRTH				= 17,
	MSG_CATCHUP_END			= 18,
	MSG_TEST_COMMAND		= 19,
	MSG_TEST_NEWROUND		= 20,
	MSG_TEST_BEGINCAST		= 21,
	MSG_TEST_SUPPRESS		= 22,
	MSG_TEST_CATCHUP		= 23,
	MSG_TEST_REMOVE_PAGE	= 24,
	MSG_RAS					= 25,
	MSG_HIGH_WATER			= 26,
	MSG_HIGH_WATER_ACK		= 27,
	MSG_LOW_WATER			= 28,
	MSG_LOW_WATER_ACK		= 29,
	MSG_IS_ACTIVE			= 30,

	MSG_LOCK				= 31,
	MSG_UNLOCK				= 32,
	MSG_LOG_DUMP			= 33,

	MSG_MAX
} msg_cmd_t;

typedef	struct	msg {
	list_t		m_dlnk;
	int32_t		m_fr;
	int32_t		m_to;
	int32_t		m_len;

	int32_t		m_cmd;
	struct sockaddr_in	m_From;
	uint64_t	m_cksum64;
} msg_t;

/*
 *	Server Status Object
 */
typedef	struct	Server {

#define	PAXOS_SERVER_ACTIVE		0x1
#define	PAXOS_SERVER_CONNECTED	0x2

	uint32_t	s_status;

	int32_t		s_ID;
	int32_t		s_leader;
	int32_t		s_epoch;
	timespec_t	s_Election;
	ChSend_t	s_ChSend;

// Connection
	timespec_t	s_YourTime;
	timespec_t	s_Token;
	timespec_t	s_Alive;

// Catchup
	uint32_t	s_IssuedSeq;	// Issued
	timespec_t	s_IssuedTime;
	uint32_t	s_BackPage;
	uint32_t	s_NextDecide;

// Autonomic
	time_t	s_AutonomicLast;

// Load
	int32_t		s_Load;

} Server_t;

/*
 * Propser/Leader/Agent Object
 */
typedef	struct	MetaCmd {
	uint32_t	c_len;
	uchar_t		c_opaque[PAXOS_COMMAND_SIZE];
} MetaCmd_t;
	
//
// 	Mode
//
typedef	enum PaxosStepMode {
	MODE_NONE,			// AMode,PMode
	MODE_COLLECT,		// AMode 
	MODE_GATHERLAST,	// AMode
	MODE_WAIT,			// PMode
	MODE_BEGINCAST,		// AMode,PMode
	MODE_GATHERACCEPT,	// PMode
	MODE_DECIDED,		// PMode
} PaxosStepMode_t;

//
// PaxosInstance
//
typedef struct Round {
	uint32_t	r_id;
	uint32_t	r_seq;
} Round_t;

#define	VALUE_DEFINED		0x1
#define	VALUE_NOOP			0x2
#define	VALUE_NORMAL		0x4
#define	VALUE_RECOVER		0x8
#define	VALUE_CATCHUP		0x10
#define	VALUE_VALIDATE		0x20

typedef struct value {
	uint32_t	v_Flags;
	int32_t		v_Server;	// Original Data
	uint32_t	v_Epoch;
	int32_t		v_From;		// Data
} value_t;

#define	isDefined( pV )	( ((pV)->v_Flags & VALUE_DEFINED) )
#define	isEmpty( pC )	((pC)->c_len == 0 )
#define	isDecided( k )	(PMode(k)==MODE_DECIDED )
#define	isMyRnd			(AMode == MODE_BEGINCAST \
							&& CurRnd.r_id == MyId )
#define	isSameValue( pA, pB ) \
	( (pA)->v_Flags & VALUE_DEFINED && (pB)->v_Flags & VALUE_DEFINED ? \
		(pA)->v_Server == (pB)->v_Server : 0 )

#define	PAXOS_MODE( pV ) \
	({PaxosMode_t _m; \
		if( (pV)->v_Flags & VALUE_CATCHUP ) _m = PAXOS_MODE_CATCHUP; \
		else _m = PAXOS_MODE_NORMAL; _m;})

typedef struct PaxosInstance {

	timespec_t	p_Deadline;

	PaxosStepMode_t	p_Mode;			// モード
	value_t			p_InitValue;	// 初期値
	value_t			p_RndValue;		// 提案値/合意値
	Round_t			p_RndBallot;	// 提案round
	Quorum_t		p_RndAccQuo;	// Accept Quorum

// catchup
	bool_t		p_Suppressed;

} PaxosInstance_t;

//
// 	Proposer/Leader/Agent
//

typedef struct PaxosMulti {

	list_t			m_Lnk;

//	Page
	struct {
		uint32_t	p_Page;
		int32_t		p_NextK;
		int32_t		p_SuccessCount;
	} m_PageHead;

	PaxosInstance_t		m_aPaxos[PAXOS_MAX];

	struct {
		MetaCmd_t	c_MetaCmdInit;
		MetaCmd_t	c_MetaCmdRnd;
	} m_aMetaCmd[PAXOS_MAX];

	struct {
		PaxosStepMode_t	l_Mode;
		Round_t			l_CurRnd;
		Quorum_t		l_RndInfQuo;
	} m_Leader;

} PaxosMulti_t;

typedef struct msg_dump {
	msg_t		d_head;
	void		*d_addr;
	int32_t		d_len;
} msg_dump_t;

typedef struct msg_control {
	msg_t	c_head;
} msg_control_t;

typedef struct msg_outofband_stop {
	msg_t		s_head;
	int32_t		s_server;
} msg_outofband_stop_t;

typedef struct msg_outofband_restart {
	msg_t		r_head;
	int32_t		r_server;
} msg_outofband_restart_t;

typedef struct msg_down {
	msg_t	d_head;
} msg_down_t;

typedef struct msg_alive {
	msg_t		a_head;
	int32_t		a_fr;
	int32_t		a_to;
	timespec_t	a_Election;
	int32_t		a_leader;
	int32_t		a_epoch;
	timespec_t	a_YourTime;
	timespec_t	a_MyTime;
	bool_t		a_Catchup;
	uint32_t	a_BackPage;
	uint32_t	a_NextDecide;
	int32_t		a_Load;
} msg_alive_t;

typedef struct msg_active_rpl {
	msg_t			a_head;
	PaxosActive_t	a_Active;
} msg_active_rpl_t;

typedef struct msg_ras {
	msg_t		r_head;
} msg_ras_t;

typedef struct msg_test_command {
	msg_t		c_head;
	int			c_k;
	PaxosHead_t	c_cmd;
} msg_test_command_t;

typedef struct msg_test_newround {
	msg_t	i_head;
} msg_test_newround_t;

typedef struct msg_test_begincast {
	msg_t	b_head;
	int		b_k;
} msg_test_begincast_t;

typedef enum msg_paxos_mode {
	MODE_MSG_NONE,
	MODE_MSG_DECIDED,
	MODE_MSG_PROPOSED
} msg_paxos_mode_t;

typedef struct msg_paxos {
	msg_paxos_mode_t	m_mode;
	Round_t				m_round;
	value_t				m_value;
} msg_paxos_t;

typedef struct msg_collect {
	msg_t		c_head;
	Round_t		c_CurRnd;
	uint32_t	c_NextDecide;
	uint32_t	c_BackPage;
	uint32_t	c_Page;
	int32_t		c_NextK;
	msg_paxos_t	c_paxos[PAXOS_MAX];
} msg_collect_t;

typedef struct msg_last {
	msg_t		l_head;
	Round_t		l_round;
	uint32_t	l_NextDecide;
	uint32_t	l_Page;
	msg_paxos_t	l_paxos[PAXOS_MAX];
} msg_last_t;

typedef struct msg_oldround {
	msg_t		o_head;
	Round_t		o_round;
	Round_t		o_commit;
} msg_oldround_t;

typedef struct msg_begin {
	msg_t		b_head;
	int32_t		b_To;		// Acceptを返すサーバ
	uint32_t	b_NextDecide;
	uint32_t	b_BackPage;
	uint32_t	b_Page;
	int32_t		b_NextK;
	int32_t		b_k;
	Round_t		b_CurRnd;
	value_t		b_Value;
	MetaCmd_t	b_MetaCmd;	// 絶対に必要
} msg_begin_t;

typedef struct msg_accept {
	msg_t		a_head;
	uint32_t	a_Page;
	int32_t		a_k;
	Round_t		a_round;
	value_t		a_Value;
	MetaCmd_t	a_MetaCmd;	// leaderにbeginが到達しない場合用
} msg_accept_t;

typedef struct msg_success {
	msg_t		s_head;
	uint32_t	s_BackPage;
	uint32_t	s_NextDecide;
	uint32_t	s_Page;
	int32_t		s_NextK;
	int32_t		s_k;
	value_t		s_Value;
	MetaCmd_t	s_MetaCmd;	// 決定値
} msg_success_t;

typedef	struct	Paxos {
	int			p_MallocCount;
	bool_t		p_bSuppressAccept;	// 再参入用
	bool_t		p_Active;
	Mtx_t		p_Lock;
	Cnd_t		p_Cnd;		// ページ更新待ち用
	int32_t		p_MyId;

	Mtx_t		p_LockElection;
	struct {
		bool_t		e_IamMaster;
		bool_t		e_NewRndStart;
		uint32_t	e_Status;
		timespec_t	e_Birth;
		uint32_t	e_Epoch;
		int32_t		e_Elected;
		int32_t		e_Leader;
		int32_t		e_Reader;
	} p_Election;

	char		p_CellName[PAXOS_CELL_NAME_MAX];
	int			p_Max;
	int			p_Maximum;
	Server_t	*p_aServer; // [p_Maximum]

	uint32_t		p_BackPage;
	list_t			p_MultiLnk;
	PaxosMulti_t	p_Multi;

	pthread_t	p_Receiver;
	Queue_t		p_QueuePaxos;
	pthread_t	p_ReceiverPaxos;
	Queue_t		p_QueueHeartbeat;
	pthread_t	p_ReceiverHeartbeat;
	pthread_t	p_Clock;
	pthread_t	p_Heartbeat;

//	Time Unit(sec)
	int64_t		p_UnitUsec;

// Communication
	FdEventCtrl_t	p_FdEvCt;
	int				p_RecvFd;
	int				p_SendFd;
	int				p_AdminFd;

//	OutOfBand marshal/unmarshal
	int			(*p_fOutOfBandMarshal)(struct Paxos*,void*,IOReadWrite_t *pW);
	int			(*p_fOutOfBandUnmarshal)(struct Paxos*,void*,IOReadWrite_t *pR);

// Election
	int			(*p_fElection)(struct Paxos*, PaxosNotify_t, 
					int32_t Leader, uint32_t Epoch, int32_t EventFromServer );

// API
	Queue_t		p_Outbound;

	int			(*p_fNotify)(struct Paxos*, PaxosMode_t, PaxosNotify_t, 
					uint32_t seq, int server, uint32_t epoch,
					int32_t len, void* pCommand, int from );
	uint32_t	p_NextDecide;
	Hash_t		p_Gaps;

// Memory
	void*		(*p_fMalloc)(size_t);
	void		(*p_fFree)(void*);

// Load
	int			(*p_fLoad)(void*);

// New Round
	time_t		p_NewRoundLast;

// Catchup
	uint32_t	p_CatchupNextDecide;
	Server_t*	p_pCatchupServer;

	uint32_t	p_CatchupPage;
	time_t		p_CatchupPageLast;

	timespec_t	p_CatchupRecvLast;
	int			p_CatchupRecvServer;

	time_t		p_IssuedLast;
	uint32_t	p_IssuedSeq;
	
// For Test
	bool_t		p_RecvStop[MSG_MAX];
	bool_t		p_CatchupStop;

// Tag
	void*		p_pTag;

// Log
	struct Log*	p_pLog;
	bool_t		p_bFreeze;

// Checksum
	bool_t		p_bCksum;
	bool_t		p_bPageSuppress;

#ifdef	WWW
// Water
#define	WATER_LOW			0
#define	WATER_HIGH			1
#define	WATER_LOW_ISSUED	2
/*
	  | High | Low  | LOW_ACK
	-------------------------
	L |H(HM) | -    | -
	LI|H(HM) |LI(LM)|L
	H |H(HM) |LI(LM)|H
*/
	Quorum_t	p_WaterReplica;
	int			p_WaterLeader[PAXOS_SERVER_MAX];
#endif
} Paxos_t;

typedef struct Gap {
	uint32_t	g_Seq;
	value_t		g_Decision;
	MetaCmd_t	g_MetaCmd;
} Gap_t;

#define	INC_ROUND( pr ) SEQ_INC( (pr)->r_seq )
#define	CMP_ROUND( px, py ) \
	({int _ret; _ret = SEQ_CMP( (px)->r_seq, (py)->r_seq ); \
		if( _ret == 0 ) { \
			if( (px)->r_seq != 0 ) { \
				if( (px)->r_id < (py)->r_id )	_ret = 1; \
				if( (px)->r_id > (py)->r_id )	_ret = -1; \
			} \
		} _ret;})

#define	TO_SEQ( page, k )	(((uint32_t)(page))*PAXOS_MAX + (uint32_t)(k))
#define	TO_PAGE( seq )		(((uint32_t)(seq)/PAXOS_MAX))
#define	TO_K( seq )			(((uint32_t)(seq)%PAXOS_MAX))

#define	MyId			pPaxos->p_MyId
#define	Active			pPaxos->p_Active
#define	NextDecide		pPaxos->p_NextDecide
#define	BackPage		pPaxos->p_BackPage
#define	NewRoundLast	pPaxos->p_NewRoundLast

#define	Election	pPaxos->p_Election
#define	IamMaster		Election.e_IamMaster
#define	NewRndStart		Election.e_NewRndStart
#define	EStatus			Election.e_Status
#define	EBirth			Election.e_Birth
#define	Epoch			Election.e_Epoch
#define	Elected			Election.e_Elected
#define	ELeader			Election.e_Leader
#define	EReader			Election.e_Reader

#define	Multi		pPaxos->p_Multi

#define	Leader		Multi.m_Leader
#define	CurRnd			Leader.l_CurRnd
#define	AMode			Leader.l_Mode
#define	RndInfQuo		Leader.l_RndInfQuo

#define	Success		Multi.m_Success
#define	PrevSend		Success.s_PrevSend
#define	LastSend		Success.s_LastSend
#define	LastWait		Success.s_LastWait

#define	PageHead	pPaxos->p_Multi.m_PageHead
#define	Page			PageHead.p_Page
#define	NextK			PageHead.p_NextK
#define	SuccessCount	PageHead.p_SuccessCount

#define	APAXOS			pPaxos->p_Multi.m_aPaxos
#define	PAXOS( k )		pPaxos->p_Multi.m_aPaxos[k]

#define	Seq( k )		TO_SEQ( Page, k )

#define	Deadline( k )	PAXOS(k).p_Deadline
#define	Last( k )		PAXOS(k).p_Last
#define	RndSuccess( k )	PAXOS(k).p_RndSuccess

#define	PMode( k )		PAXOS(k).p_Mode
#define	InitValue( k )	PAXOS(k).p_InitValue
#define	RndValue( k )	PAXOS(k).p_RndValue
#define	RndBallot( k )	PAXOS(k).p_RndBallot
#define	RndAccQuo( k )	PAXOS(k).p_RndAccQuo

#define	Suppressed( k )		PAXOS(k).p_Suppressed

#define	METACMD( k )			Multi.m_aMetaCmd[k]
#define	MetaCmdInit( k )		pPaxos->p_Multi.m_aMetaCmd[k].c_MetaCmdInit
#define	MetaCmdRnd( k )			pPaxos->p_Multi.m_aMetaCmd[k].c_MetaCmdRnd

typedef struct msg_catchup_req {
	msg_t		c_head;
	uint32_t	c_SeqFrom;
	uint32_t	c_SeqTo;
	time_t		c_RecvLastDT;
} msg_catchup_req_t;

typedef struct msg_catchup_rpl {
	msg_t			c_head;
	bool_t			c_True;
	uint32_t		c_MyNextDecide;
	uint32_t		c_Seq;
	PaxosInstance_t	c_PaxosI;
	MetaCmd_t		c_MetaCmd;
} msg_catchup_rpl_t;

typedef struct msg_catchup_end {
	msg_t			c_head;
} msg_catchup_end_t;

typedef struct msg_recv_stop {
	msg_t		s_head;
	bool_t		s_OnOff;
	msg_cmd_t	s_Cmd;	
} msg_recv_stop_t;

typedef struct msg_catchup {
	msg_t		c_head;
	bool_t		c_OnOff;
} msg_catchup_t;

static	inline	void*
PaxosMalloc( struct Paxos *pPaxos, size_t s )
{
	void *p;

	if( pPaxos && pPaxos->p_fMalloc )	p = (pPaxos->p_fMalloc)( s );
	else	p = Malloc( s );
	return( p );
}

static	inline	void
PaxosFree( struct Paxos *pPaxos, void *p )
{
	if( pPaxos && pPaxos->p_fFree )	(pPaxos->p_fFree)( p );
	else	Free( p );
}

#define	msg_init( pm, to, cmd, len ) \
	({memset( pm, 0, len );list_init(&(pm)->m_dlnk);(pm)->m_len = len; \
		(pm)->m_to	= to;(pm)->m_cmd = cmd;})

static inline	msg_t*	
msg_alloc(struct Paxos *pPaxos, int32_t to, msg_cmd_t cmd, size_t size)
{
	msg_t*	pm;
	size_t		len;

	len	= size;
	pm = (msg_t*)PaxosMalloc( pPaxos, len );

	memset( pm, 0, len );

	list_init( &pm->m_dlnk );
	pm->m_len	= len;
	pm->m_to	= to;
	pm->m_cmd	= cmd;

	if( pPaxos )	pm->m_fr	= MyId;
	else			pm->m_fr	= -1;
	return( pm );
}

static inline	void	msg_free( struct Paxos *pPaxos, void* pm )
{
	PaxosFree( pPaxos, pm );
}

extern	int	PaxosValidate( struct Paxos* pPaxos, PaxosMode_t mode,
				uint32_t seq, struct value* pValue, struct MetaCmd* pC );

#endif		//_PAXOS_SERVER_

extern	char*	PaxosGetArgFromConfig( char *pCellName, char *pId, int ArgNo );
extern	PaxosCell_t* PaxosGetCell( char *pCell );
extern	void PaxosFreeCell( PaxosCell_t *pCell );
extern	int PaxosIsActive( char *pCell, int My_Id, int j, 
					PaxosActive_t *pActive );
extern	int PaxosIsActiveSummary( char *pCell, int My_Id, 
					PaxosActive_t *pSummary );
#endif		//__PAXOS__
