/******************************************************************************
*
*  CNV.h 	--- converter header
*
*  Copyright (C) 2011,2016 triTech Inc. All Rights Reserved.
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
#ifndef	_CONVERTER_

#define	_CONVERTER_

#include	"PaxosSession.h"
#include	"Status.h"

#include	<wchar.h>
#include	<locale.h>

#define	MAX_WORKERS		5

#define	CNV_FAMILY			(2<<16)
typedef enum	CNVCmd {
	CNV_OUTOFBAND	= (CNV_FAMILY|1),
	CNV_UPDATE		= (CNV_FAMILY|2),
	CNV_REFER		= (CNV_FAMILY|3),
	CNV_READ		= (CNV_FAMILY|4),

	CNV_PSUEDO_REQ	= (CNV_FAMILY|5),	// successive request
	CNV_PSUEDO_RPL	= (CNV_FAMILY|6),	// successive reply

	CNV_EVENT_BIND	= (CNV_FAMILY|7),	// set Event Session to Normal
	CNV_EVENT_WAIT	= (CNV_FAMILY|8),

	CNV_MEM_ROOT	= (CNV_FAMILY|9),
} CNVCmd_t;

#define	CNV_EVENT_NO		1

#define	CNV_SNAP_PAXOS	"CNV_SNAP_PAXOS"
#define	CNV_SNAP		"CNV_SNAP"

struct FdEvCoC;
struct CnvCtrlCoC;

//Client
int CNVEventSessionBind( struct Session *pSession, 
							struct Session *pEvent );

//	Server
extern	int	CNVAcceptReplyByMsg( struct PaxosAccept *pAccept, 
										Msg_t *pRes, int CnvCmd, int Error );
extern	int	CNVAcceptReplyReadByMsg( struct PaxosAccept *pAccept, 
										Msg_t *pRes, int CnvCmd );

typedef struct CnvIF {
	// Server IF
	int		(*IF_fOpen)(struct PaxosAccept*, PaxosSessionHead_t* );
	int		(*IF_fClose)(struct PaxosAccept*, PaxosSessionHead_t* );
	int		(*IF_fRequestToServer)(struct PaxosAccept*, PaxosSessionHead_t*);
	Msg_t*	(*IF_fReferToServer)(struct PaxosAccept*, PaxosSessionHead_t*);

	Msg_t*	(*IF_fBackupPrepare)(int);
	int		(*IF_fBackup)(PaxosSessionHead_t*);
	int		(*IF_fRestore)(void);
	int		(*IF_fTransferSend)( IOReadWrite_t* );
	int		(*IF_fTransferRecv)( IOReadWrite_t* );

	int		(*IF_fGlobalMarshal)( IOReadWrite_t* );
	int		(*IF_fGlobalUnmarshal)( IOReadWrite_t* );

	int		(*IF_fAdaptorMarshal)(IOReadWrite_t*,struct PaxosAccept*);
	int		(*IF_fAdaptorUnmarshal)(IOReadWrite_t*,struct PaxosAccept*);

	int		(*IF_fStop)(void);
	int		(*IF_fRestart)(void);

	Msg_t*	(*IF_fRecvMsgByFd)(int fd, 
				void*(*fMalloc)(size_t), void(*fFree)(void*) );
	int		(*IF_fSendMsgByFd)(int fd, Msg_t *pMsg);

	struct CnvCtrlCoS	*(*IF_fInitCoS)(void*);	

	int				(*IF_fEventBind)(struct PaxosAccept *pNomal, 
										struct PaxosAccept *pEvent );

	// Client IF
	int		(*IF_fRequest)( struct FdEvCoC *pFdEvCoC, 
							Msg_t *pReq, bool_t* pUpdate );
	int		(*IF_fReply)( struct FdEvCoC *pFdEvCoC, Msg_t*pRpl, int Status );

	struct FdEvCoC	*(*IF_fConAlloc)(void);
	void			(*IF_fConFree)(struct FdEvCoC*);

	int		(*IF_fSessionOpen)( struct FdEvCoC *pFdCon, char *pCellName );
	int		(*IF_fSessionClose)( struct FdEvCoC *pFdCon );

	struct CnvCtrlCoC	*(*IF_fInitCoC)(void*);	
} CnvIF_t;

typedef struct CnvMemRootReq {
	PaxosSessionHead_t	r_Head;
	char				r_Name[NAME_MAX];
} CnvMemRootReq_t;
	
typedef struct CnvMemRootRpl {
	PaxosSessionHead_t	r_Head;
	void				*r_pRoot;
} CnvMemRootRpl_t;
	
typedef struct CnvMemDumpReq {
	PaxosSessionHead_t	d_Head;
	void				*d_pStart;
} CnvMemDumpReq_t;
	
typedef struct CnvMemDumpRpl {
	PaxosSessionHead_t	d_Head;
	void				*d_pStart;
	unsigned char		d_Data[0];
} CnvMemDumpRpl_t;
	
typedef	struct CnvReadReq {
	PaxosSessionHead_t	r_Head;
	char				r_Name[PATH_NAME_MAX];
	off_t				r_Off;
	size_t				r_Len;
} CnvReadReq_t;

typedef	struct CnvReadRpl {
	PaxosSessionHead_t	r_Head;
	size_t				r_Len;
	char				r_Data[1];
} CnvReadRpl_t;

typedef struct CnvEventSessionBindReq {
	PaxosSessionHead_t	eb_Head;
	PaxosClientId_t		eb_EventId;
} CnvEventSessionBindReq_t;

typedef struct CnvEventSessionBindRpl {
	PaxosSessionHead_t		eb_Head;
} CnvEventSessionBindRpl_t;

typedef struct CnvEventWaitReq {
	PaxosSessionHead_t	ew_Head;
} CnvEventWaitReq_t;

typedef struct CnvEventWaitRpl {
	PaxosSessionHead_t	ew_Head;
	char				ew_CellName[PAXOS_CELL_NAME_MAX];	
	int					ew_No;
} CnvEventWaitRpl_t;


/*
 *	Controler
 */
typedef	struct CnvCtrlCoC {
	RwLock_t			c_RwLock;
	pthread_t			c_ThreadAdm;
	struct sockaddr_un	c_AdmAddr;
	FdEventCtrl_t		c_FdEventCtrl;
	char				c_CellName[PAXOS_CELL_NAME_MAX];	
	Queue_t				c_Worker;
	pthread_t			c_WorkerThread[MAX_WORKERS];
	int					c_Snoop;
} CnvCtrlCoC_t;

typedef struct CnvCtrlCoS {
	RwLock_t				s_RwLock;
	FdEventCtrl_t			s_FdEventCtrl;
	Hash_t					s_HashEvent;
	struct PaxosAcceptor	*s_pAcceptor;
	int						s_Snoop;
} CnvCtrlCoS_t;


/*
 *	Connector
 */
typedef struct FdEvCoC {
	list_t			c_LnkQ;
	Mtx_t			c_Mtx;
	Cnd_t			c_Cnd;

	TimerEvent_t	*c_pTimerEvent;

#define	FDEVCON_WORKER		0x1
//#define	FDEVCON_WAIT		0x2
//#define	FDEVCON_REPLY		0x4
//#define	FDEVCON_ACK			0x8
//#define	FDEVCON_WB			0x10

#define	FDEVCON_SYNC_END	0x1000
#define	FDEVCON_SYNC		0x2000
#define	FDEVCON_ABORT		0x8000
	int				c_Flags;
	void			*c_pVoid;

#define	FD_TYPE_ACCEPT	1
#define	FD_TYPE_CLIENT	2
	FdEvent_t		c_FdEvent;	// Recv From Client
	struct sockaddr c_PeerAddr;
	struct Session	*c_pSession;
	int				c_NoSession;
	Queue_t			c_QueueNormal;
	Queue_t			c_QueueEvent;
	list_t			c_MsgList;
} FdEvCoC_t;

extern	struct	sockaddr_in	Server;

extern	int	MyId;
extern	struct Log	*pLog;

//extern	CnvIF_t	HTTP_IF;
//extern	CnvIF_t	xjPing_IF;
extern	CnvIF_t	CIFS_IF;

#define	SERVICE \
struct { \
	char	*s_pName; \
	CnvIF_t	*s_pIF; \
} ServiceIF[] = { \
/*	{	"http",		&HTTP_IF },*/ \
/*	{	"xjPing",	&xjPing_IF }*/ \
	{	"CIFS",	&CIFS_IF } \
}; \
CnvIF_t	*pIF;

#define	SET_IF( pName ) \
({int i; \
	for( i = 0; i < sizeof(ServiceIF)/sizeof(ServiceIF[0]); i++ ) { \
		if( !strcmp( (pName), ServiceIF[i].s_pName ) ) { \
			pIF	= ServiceIF[i].s_pIF; \
			break; \
		} \
	}})

#endif
