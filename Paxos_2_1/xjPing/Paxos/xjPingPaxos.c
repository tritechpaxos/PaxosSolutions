/******************************************************************************
*
*  xjPingPaxos.c 	--- 
*
*  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
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
 *	試験		
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_

//#define	_LOCK_

#define	_PAXOS_SESSION_SERVER_
#define	_PFS_SERVER_
#include	"PFS.h"
#include	<netinet/tcp.h>
#include	"xjPingPaxos.h"
#include	"../../cache/h/FileCache.h"

struct Log	*neo_log;

int	neo_errno;
int	_neo_pid;

char	*_neo_procname;
char	*_neo_procbname;
char	_neo_hostname[NEO_NAME_MAX];

char	*svc_myname;

PingCNTRL_t	CNTRL;

r_svc_inf_t	_svc_inf;

_dlnk_t	_neo_port;


extern	struct Log*	pLog;

int
p_dispatch( p_id_t *pd, p_head_t *pH )
{
	int	len;
	p_head_t	head;
	int	err;
	p_req_con_t	reqCon;
	p_res_con_t	resCon;
	p_req_dis_t	reqDis;
	p_res_dis_t	resDis;
	p_Tag_t		*pTag;


	LOG(pLog, LogDBG, "ENT:cmd=0x%x", pH->h_cmd );

	switch( pH->h_cmd ) {
		case R_CMD_LINK:	err = svc_link( pd, pH );	break;
		case R_CMD_UNLINK:	err = svc_unlink( pd, pH );	break;
		case R_CMD_CREATE:	err = svc_create( pd, pH );	break;
		case R_CMD_DROP:	err = svc_drop( pd, pH );	break;
		case R_CMD_OPEN:	err = svc_open( pd, pH );	break;
		case R_CMD_CLOSE:	err = svc_close( pd, pH );	break;
		case R_CMD_CLOSE_CLIENT:err = svc_close_client( pd, pH );break;
		case R_CMD_FLUSH:	err = svc_flush( pd, pH );	break;
		case R_CMD_CANCEL:	err = svc_cancel( pd, pH );	break;
		case R_CMD_INSERT:	err = svc_insert( pd, pH );	break;
		case R_CMD_DELETE:	err = svc_delete( pd, pH );	break;
		case R_CMD_UPDATE:	err = svc_update( pd, pH );	break;
		case R_CMD_SEARCH:	err = svc_search( pd, pH );	break;
		case R_CMD_FIND:	err = svc_find( pd, pH );	break;
		case R_CMD_REWIND:	err = svc_rewind( pd, pH );	break;
		case R_CMD_SEEK:	err = svc_seek( pd, pH );	break;
		case R_CMD_SORT:	err = svc_sort( pd, pH );	break;
		case R_CMD_HOLD:	err = svc_hold( pd, pH );	break;
		case R_CMD_RELEASE:	err = svc_release( pd, pH );	break;
		case R_CMD_HOLD_CANCEL:	err = svc_hold_cancel( pd, pH );
									break;
		case R_CMD_COMPRESS:	err = svc_compress( pd, pH );break;
		case R_CMD_EVENT:	err = svc_event( pd, pH );	break;
		case R_CMD_EVENT_CANCEL:err = svc_event_cancel( pd, pH );
									break;
		case R_CMD_DIRECT:	err = svc_direct( pd, pH );	break;
		case R_CMD_REC_NUMS:	err = svc_number( pd, pH );	break;
		case R_CMD_STOP:	err = svc_stop( pd, pH );	break;
		case R_CMD_SHUTDOWN:	
					err = svc_stop( pd, pH );	
					if( neo_log )	LogDump( neo_log );
					exit( 0 );

		case R_CMD_RESTART:	err = svc_restart( pd, pH );	break;
		case R_CMD_LIST:	err = svc_list( pd, pH );	break;
		

		case R_CMD_DUMP_PROC:	err = svc_dump_proc( pd, pH );break;
		case R_CMD_DUMP_INF:	err = svc_dump_inf( pd, pH );
								neo_HeapPoolDump();
								break;
		case R_CMD_DUMP_RECORD:	err = svc_dump_record( pd, pH );
									break;
		case R_CMD_DUMP_TBL:	err = svc_dump_tbl( pd, pH );break;
		case R_CMD_DUMP_USR:	err = svc_dump_usr( pd, pH );break;
		case R_CMD_DUMP_MEM:	err = svc_dump_mem( pd, pH );break;
		case R_CMD_DUMP_CLIENT:	err = svc_dump_client(pd, pH);break;

		case R_CMD_SQL:		
			pTag	= (p_Tag_t*)pd->i_pTag;
			if( pTag )	PingAcceptHold( pd );
			err = svc_sql(pd, pH);	
			if( pTag )	PingAcceptRelease( pd );
			break;
		case R_CMD_SQL_PARAMS:		
			pTag	= (p_Tag_t*)pd->i_pTag;
			if( pTag )	PingAcceptHold( pd );
			err = svc_sql_params(pd, pH);	
			LOG(pLog, LogINF, "err=%d", err );
			if( pTag )	PingAcceptRelease( pd );
			break;
		case R_CMD_COMMIT:	err = svc_commit( pd, pH );break;
		case R_CMD_ROLLBACK:err = svc_rollback(pd, pH);break;

//        case R_CMD_FILE:	err = svc_file( pd, pH );	break;
		
		case P_LINK_CON:
			len = sizeof(reqCon);
			p_recv( pd, (p_head_t*)&reqCon, &len );
			bcopy( &reqCon.c_local, &pd->i_remote, sizeof(p_port_t) );

			bcopy( &reqCon, &resCon, sizeof(p_head_t) );

			resCon.c_head.h_mode	= P_PKT_REPLY;
			resCon.c_head.h_len	= sizeof(resCon) - sizeof(p_head_t);

			bcopy( &pd->i_local, &resCon.c_remote, sizeof(p_port_t) );
			p_reply( pd, (p_head_t*)&resCon, sizeof(resCon) );
			break;

		case P_LINK_DIS:
			len = sizeof(reqDis);
			p_recv( pd, (p_head_t*)&reqDis, &len );

			bcopy( &reqDis.d_head, &resDis.d_head, sizeof(p_head_t) );
			resDis.d_head.h_mode = P_PKT_REPLY;
			resDis.d_head.h_len  =  0;

			p_reply( pd, (p_head_t*)&resDis, sizeof(resDis) );
			break;

		default:
			len = sizeof(head);
			p_recv( pd, &head, &len );
			err_reply( pd, &head, E_RDB_INVAL );
			neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ),
						   MAINERR02, neo_errsym() );
			err = neo_errno;
			goto err1;
	}
	LOG(pLog, LogDBG, "EXT:cmd=0x%x", pH->h_cmd );
	return( 0 );
err1:
	LOG(pLog, LogDBG, "ERR:cmd=0x%x err=%d", pH->h_cmd, err );
	return( err );
}

void*
PingThread( void *pArg )
{
	p_id_t		*AcceptPd	= (p_id_t*)pArg;
	p_head_t	head;
	int			len;
	bool_t		bLock = FALSE;

	while( p_peek( AcceptPd, &head, sizeof(head), 0 ) == 0 ) {

		switch( head.h_cmd ) {
			case R_CMD_DUMP_LOCK:
				len = sizeof(head);
				p_recv( AcceptPd, (p_head_t*)&head, &len );
				RwLockW( &CNTRL.c_RwLock );
				bLock	= TRUE;
				p_reply( AcceptPd, &head, sizeof(head) );
				break;

			case R_CMD_DUMP_UNLOCK:
				len = sizeof(head);
				p_recv( AcceptPd, (p_head_t*)&head, &len );
				RwUnlock( &CNTRL.c_RwLock );
				bLock	= FALSE;
				p_reply( AcceptPd, &head, sizeof(head) );
				break;

			default:
				if( !bLock )	RwLockW( &CNTRL.c_RwLock );
				p_dispatch( AcceptPd, &head );
				if( !bLock )	RwUnlock( &CNTRL.c_RwLock );
				break;
		}
	}

	/* garbage */

	if( !bLock )	RwLockW( &CNTRL.c_RwLock );

	svc_garbage( AcceptPd );

	close( AcceptPd->i_rfd );

	p_close( AcceptPd );

//neo_HeapPoolDump();
	RwUnlock( &CNTRL.c_RwLock );
	pthread_exit( 0 );
	return( NULL );
}

void*
PingAdminThread( void *pArg )
{
	p_id_t	*ListenPd = (p_id_t*)pArg;
	p_id_t	*AcceptPd;
	int		ListenFd, AcceptFd;
	socklen_t		len;
	struct sockaddr_in	From;
	struct sockaddr_in	Local;
	p_port_t	Port;
	pthread_attr_t attr;
	pthread_t	th;

	pthread_attr_init( &attr ); 
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ); 

	ListenFd	= ListenPd->i_rfd;
	listen( ListenFd, 5 );
	memset( &Port, 0, sizeof(Port) );
	while( TRUE ) {
		len	= sizeof(From);
		AcceptFd = accept( ListenFd, (struct sockaddr*)&From, &len );

		memset( &Local, 0, sizeof(Local) );
		len	= sizeof(Local);
		getsockname( AcceptFd, (struct sockaddr*)&Local, &len );
		Port.p_addr.a_pa	= Local;

		RwLockW( &CNTRL.c_RwLock );

		AcceptPd = p_open( &Port );

		RwUnlock( &CNTRL.c_RwLock );

		AcceptPd->i_sfd = AcceptPd->i_rfd = AcceptFd;
		AcceptPd->i_stat	= P_ID_READY;
		AcceptPd->i_id		= P_PORT_EFFECTIVE;

		pthread_create( &th, &attr, PingThread, (void*)AcceptPd );
	}
	return( NULL );
}

int
PingInitDB( char *pAdmin, char *pDB, bool_t Wait )
{
	_neo_procname	= "xjPingServer";
	_neo_procbname	= "xjPingServer";
	_neo_pid	= getpid();
	gethostname( _neo_hostname, NEO_NAME_MAX );

	MtxInit( &_svc_inf.f_Mtx );
	HashInit( &_svc_inf.f_HashOpen, PRIMARY_1009, HASH_CODE_STR, HASH_CMP_STR,
					NULL, neo_malloc, neo_free, NULL );

	_dl_init( &_svc_inf.f_opn_tbl );
	_svc_inf.f_stat = R_SERVER_ACTIVE;
	strcpy(_svc_inf.f_path, pDB ) ;
	svc_myname = "xjPingServer";
	getcwd( (char*)&_svc_inf.f_root, sizeof(_svc_inf.f_root) );

	_dl_init( &_neo_port );
	_svc_inf.f_port	= &_neo_port;

	_svc_inf.f_pPool	= neo_GetHeapPool();

	lex_init();
	sql_lex_init();

	if( !(pBCC = (BlockCacheCtrl_t*)TBL_DBOPEN( pDB )) ) {
		goto err;
	}
	_svc_inf.f_pFileCache	= pBCC;
/* Admin port */
	p_port_t	Port;
	p_id_t		*pd;
	int			ListenFd;

	memset( &Port, 0, sizeof(Port) );

	if( Wait ) {
		if( p_getaddrbyname( pAdmin, &Port ) ) {
			goto err;
		}
	} else {
		unlink( pAdmin );
		Port.p_addr.a_un.sun_family	= AF_UNIX;
		strcpy( Port.p_addr.a_un.sun_path, pAdmin );
	}
	/*
	 * ポート記述子用エリア生成
	 */
	pd = p_open( &Port );
	if( !pd ) {
		neo_errno = E_LINK_NOMEM;
		goto err1;
	}

	if( Wait ) {
		ListenFd = socket( AF_INET, SOCK_STREAM, 0 );
		if( ListenFd < 0 ) {
			neo_errno = unixerr2neo();
			goto err2;
		}
		int on = 1;
		if ( setsockopt( ListenFd, SOL_SOCKET, SO_REUSEADDR,
						(char*)&on,sizeof(on) ) < 0 ){
			neo_errno = unixerr2neo();
			goto err2;
		}
		if(bind(ListenFd, (struct sockaddr *)&pd->i_local.p_addr.a_pa,
			sizeof(pd->i_local.p_addr.a_pa))< 0) {
			neo_errno = unixerr2neo();
			goto err2;
		}
	} else {
		ListenFd = socket( AF_UNIX, SOCK_STREAM, 0 );
		if( ListenFd < 0 ) {
			neo_errno = unixerr2neo();
			goto err2;
		}
		if(bind(ListenFd, (struct sockaddr *)&pd->i_local.p_addr.a_un,
			sizeof(pd->i_local.p_addr.a_un))< 0) {
			neo_errno = unixerr2neo();
			goto err2;
		}
	}
	pd->i_sfd = pd->i_rfd = ListenFd;
	pd->i_stat	= P_ID_READY;
	pd->i_id	= P_PORT_EFFECTIVE;

	pthread_t	th;
	pthread_create( &th, NULL, PingAdminThread, (void*)pd );
	if( Wait ) {
		void	*pRet;
		pthread_join( th, &pRet );
		close( ListenFd );
		p_close( pd );
	}

	return( 0 );
err2:
	close( ListenFd );
err1:
	p_close( pd );
err:
	return( -1 );
}

int	MyId;

PaxosCell_t			PaxosCell;
PaxosSessionCell_t	SessionCell;

struct Log*	pLog;

#define	PingMalloc( s )	(PaxosAcceptorGetfMalloc(CNTRL.c_pAcceptor)(s))
#define	PingFree( p )	(PaxosAcceptorGetfFree(CNTRL.c_pAcceptor)(p))

int
PingSessionOpen( struct PaxosAccept *pAccept, PaxosSessionHead_t* pH )
{
	p_id_t		*pd;
	p_port_t	Local;
	p_Tag_t		*pT;
	PaxosClientId_t	*pClientId;

	if ( (pd = (p_id_t *)PaxosAcceptGetTag( pAccept, XJPING_FAMILY )) ) {
LOG(pLog, LogDBG,
"REUSE?:lnk(empty?->%d),tag(0x%p)),"
"id(%ld),stat(%ld),err(%ld),busy(%d),fds(%d,%d,%d)\n"
"\tLocal(addr(id(%d),pd(0x%p == ? 0x%p),pa(%u,%s),connection(%d,%d),"
"ChanelInfo(%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld),auth(%ld))\n"
"\tRemote(addr(id(%d),pd(0x%p == ? 0x%p),pa(%u,%s),connection(%d,%d),"
"ChanelInfo(%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld),auth(%ld))\n"
"\tcontrol(%d),send(%ld,%ld,%ld,%ld,%ld,%ld,%ld),"
"receive(%ld,%ld,%ld,%ld,%ld,%ld,%ld)\n"
"\tTag(Msgs(empty?->%d),pAccept(0x%p ==? 0x%p),Cmd(0x%x),pSession(0x%p)"
"HoldCount(%d),bHold(%d)",
list_empty(&pd->i_lnk),pd->i_tag,pd->i_id,pd->i_stat,pd->i_err,pd->i_busy,
pd->i_sfd,pd->i_rfd,pd->i_cfd,
pd->i_local.p_addr.a_id,pd->i_local.p_addr.a_pd,pd,
ntohs(pd->i_local.p_addr.a_pa.sin_port),
inet_ntoa(pd->i_local.p_addr.a_pa.sin_addr),
pd->i_local.p_listen,pd->i_local.p_role,
pd->i_local.p_max_data,pd->i_local.p_max_retry,pd->i_local.p_max_time,
pd->i_local.p_min_time,pd->i_local.p_cur_time,pd->i_local.p_rcv_time,
pd->i_local.p_stime,pd->i_local.p_rtime,pd->i_local.p_bufsize,
pd->i_local.p_auth,
pd->i_remote.p_addr.a_id,pd->i_remote.p_addr.a_pd,pd,
ntohs(pd->i_remote.p_addr.a_pa.sin_port),
inet_ntoa(pd->i_remote.p_addr.a_pa.sin_addr),
pd->i_remote.p_listen,pd->i_remote.p_role,
pd->i_remote.p_max_data,pd->i_remote.p_max_retry,pd->i_remote.p_max_time,
pd->i_remote.p_min_time,pd->i_remote.p_cur_time,pd->i_remote.p_rcv_time,
pd->i_remote.p_stime,pd->i_remote.p_rtime,pd->i_remote.p_bufsize,
pd->i_remote.p_auth,
pd->i_cntl,
pd->i_spacket.s_number,pd->i_spacket.s_bytes,pd->i_spacket.s_max_bytes,
pd->i_spacket.s_min_bytes,pd->i_spacket.s_time,pd->i_spacket.s_max_time,
pd->i_spacket.s_min_time,
pd->i_rpacket.s_number,pd->i_rpacket.s_bytes,pd->i_rpacket.s_max_bytes,
pd->i_rpacket.s_min_bytes,pd->i_rpacket.s_time,pd->i_rpacket.s_max_time,
pd->i_rpacket.s_min_time,
list_empty(&((p_Tag_t *)pd->i_pTag)->t_Msgs),
((p_Tag_t *)pd->i_pTag)->t_pAccept,pAccept,
((p_Tag_t *)pd->i_pTag)->t_Cmd,
((p_Tag_t *)pd->i_pTag)->t_pSession,
((p_Tag_t *)pd->i_pTag)->t_HoldCount,
((p_Tag_t *)pd->i_pTag)->t_bHold );
	} else {
		pClientId	= PaxosAcceptorGetClientId( pAccept );

		memset( &Local, 0, sizeof(Local) );
		pd	= p_open( &Local );
		pd->i_remote.p_addr.a_pa	= pClientId->i_Addr;

		pT	= (p_Tag_t*)PingMalloc( sizeof(*pT) );
		memset( pT, 0, sizeof(*pT) );
		list_init( &pT->t_Msgs );
		pT->t_pAccept	= pAccept;
		pd->i_pTag		= pT;

		PaxosAcceptSetTag( pAccept, XJPING_FAMILY, pd );
	}

	return( 0 );
}

int
PingSessionClose( struct PaxosAccept *pAccept, PaxosSessionHead_t* pH )
{
	p_id_t		*pd;

	pd	= (p_id_t*)PaxosAcceptGetTag( pAccept, XJPING_FAMILY );
	PingFree( pd->i_pTag );
	pd->i_pTag	= NULL;
/* garbage */
	svc_garbage( pd );
	p_close( pd );
	PaxosAcceptSetTag( pAccept, XJPING_FAMILY, NULL );
	return( 0 );
}

p_id_t*
PingGetPd( struct PaxosAccept *pAccept, PaxosSessionHead_t *pH )
{
	p_id_t	*pd;
	p_Tag_t	*pT;
	Msg_t		*pMsg;
	MsgVec_t	Vec;

	pd	= (p_id_t*)PaxosAcceptGetTag( pAccept, XJPING_FAMILY );
	pT	= (p_Tag_t*)pd->i_pTag;
	
	pMsg = MsgCreate( 1, neo_malloc, neo_free );
	MsgVecSet( &Vec, 0, pH, pH->h_Len, pH->h_Len );
	MsgAdd( pMsg, &Vec );

	pT->t_Cmd		= pH->h_Cmd;

	MsgFirst( pMsg );
	MsgTrim( pMsg, sizeof(*pH) );
	list_add_tail( &pMsg->m_Lnk, &pT->t_Msgs );

	return( pd );
}

int
PingInit( struct PaxosAcceptor *pAcceptor,
			int FdMax, int BlockMax, int SegSize, int SegNum, 
			int64_t UsecWB, bool_t bCksum )
{
	int	Ret;

	RwLockInit( &CNTRL.c_RwLock );
	CNTRL.c_pAcceptor	= pAcceptor;

	unlink( FILE_SHUTDOWN );
	if( !pAcceptor )	return( 0 );

	Ret = FileCacheInit( pBCC, FdMax, "DB", BlockMax,
							SegSize, SegNum, UsecWB, bCksum,
							PaxosAcceptorGetLog(pAcceptor) );
	if( Ret < 0 ) return( -1 );
	return( 0 );
}

int
PingDispatch( struct PaxosAccept *pAccept, PaxosSessionHead_t *pH )
{
	p_id_t		*pd;
	p_head_t	head;
	int			Ret;

	pd	= PingGetPd( pAccept, pH );

	Ret = p_peek( pd, &head, sizeof(head), 0 );

	Ret = p_dispatch( pd, &head );

	return( Ret );
}

int
PingRequest( struct PaxosAcceptor* pAcceptor, PaxosSessionHead_t* pM, int fd )
{
	bool_t	Validate = FALSE;
	struct PaxosAccept	*pAccept;
	PaxosSessionHead_t	Rpl;
	PaxosSessionHead_t	*pPush, *pReq;
	int	Ret;
	struct Paxos	*pPaxos;

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept ) {
		LOG( pLog, LogERR, "### ALREADY CLOSED Pid[%d-%d]",
							pM->h_Id.i_Pid, pM->h_Id.i_No );
		return( 0 );
	}

	switch( (int)pM->h_Cmd ) {
		case XJPING_OUTOFBAND:
			PaxosAcceptOutOfBandLock( pAccept );

			pPush	= _PaxosAcceptOutOfBandGet( pAccept, &pM->h_Id );

			/* Pushの分をTrim */
			pReq	= pPush + 1;
			if( pReq->h_Cmd == XJPING_UPDATE ) {
				Validate = TRUE;
			}
//p_head_t *pH;
//pH = (p_head_t*)(pReq+1);
//LOG( pLog, "h_Cmd=0x%x h_cmd=0x%x Validate=%d", 
//pReq->h_Cmd, pH->h_cmd, Validate ); 

			PaxosAcceptOutOfBandUnlock( pAccept );

			if( Validate )	goto update;
			pM	= pReq;

		case XJPING_REFER:
			if( PaxosAcceptedStart( pAccept, pM ) ) {
				goto skip;
			}
			RwLockR( &CNTRL.c_RwLock );

			Ret = PingDispatch( pAccept, pM );

			RwUnlock( &CNTRL.c_RwLock );
			break;

		case XJPING_ADMIN:
			RwLockR( &CNTRL.c_RwLock );

			Ret = PingDispatch( pAccept, pM );

			RwUnlock( &CNTRL.c_RwLock );
			break;

		case XJPING_UPDATE:
update:
			Ret	= PaxosRequest( pPaxos = PaxosAcceptorGetPaxos(pAcceptor),
								pM->h_Len, pM, Validate );
			if( Ret ) {
				SESSION_MSGINIT( &Rpl, pM->h_Cmd, Ret, 0, sizeof(Rpl) );
				Rpl.h_Master	= PaxosGetMaster( pPaxos );
				Rpl.h_Epoch		= PaxosEpoch( pPaxos );
				Rpl.h_TimeoutUsec	= PaxosMasterElectionTimeout( pPaxos );

				Ret	= PaxosAcceptSend( pAccept, &Rpl );
			}
			break;
		default:
			ABORT();
	}
skip:
	PaxosAcceptPut( pAccept );
	return( 0 );
}

int
PingValidate(struct PaxosAcceptor *pAcceptor, uint32_t seq,
						PaxosSessionHead_t *pM, int From)
{
	int	Ret;

	RwLockW( &CNTRL.c_RwLock );

	Ret	= PaxosAcceptorOutOfBandValidate( pAcceptor, seq, pM, From );

	RwUnlock( &CNTRL.c_RwLock );

	return( Ret );
}

int
PingAccepted( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	PaxosSessionHead_t	*pReq, *pPush;

	RwLockW( &CNTRL.c_RwLock );

	switch( (int)pM->h_Cmd ) {

		case XJPING_OUTOFBAND:
			PaxosAcceptOutOfBandLock( pAccept );

			pPush	= _PaxosAcceptOutOfBandGet( pAccept, &pM->h_Id );

			/* Pushの分をTrim */
			pReq	= pPush + 1;
			PingDispatch( pAccept, pReq );

			PaxosAcceptOutOfBandUnlock( pAccept );
			break;

		case XJPING_UPDATE:
//LOG( pLog, "XJPING_UPDATE" );
			PingDispatch( pAccept, pM );

			neo_free( pM );

			break;
	}
	RwUnlock( &CNTRL.c_RwLock );

	return( 0 );
}

int
PingRejected( struct PaxosAcceptor *pAcceptor, PaxosSessionHead_t *pM )
{
	struct PaxosAccept	*pAccept;
	PaxosSessionHead_t	Rpl;
	int	Ret;

	pAccept	= PaxosAcceptGet( pAcceptor, &pM->h_Id, FALSE );
	if( !pAccept )	return( -1 );

	SESSION_MSGINIT( &Rpl, pM->h_Cmd, PAXOS_REJECTED, 0, sizeof(Rpl) );
	Rpl.h_Master	= PaxosGetMaster(PaxosAcceptorGetPaxos(pAcceptor));
	Ret = PaxosAcceptSend( pAccept, &Rpl );

	PaxosAcceptPut( pAccept );
	return( Ret );
}

int
PingAbort( struct PaxosAcceptor *pAcceptor, uint32_t Seq )
{
	Printf("=== XJPINGAbort Seq=%u\n", Seq );
	return( 0 );
}

int
PingShutdown( struct PaxosAcceptor *pAcceptor, 
				PaxosSessionShutdownReq_t *pReq, uint32_t nextDecide )
{
	int	Fd;
	char	buff[1024];
	int	Ret;

	Ret = svc_shutdown();
	LOG( pLog, LogINF, "ret=%d", Ret );
	if( Ret < 0 )	goto err;

	Fd = creat( FILE_SHUTDOWN, 0666 );
	if( Fd < 0 )	goto err;

	sprintf( buff, "%u [%s]\n", nextDecide, pReq->s_Comment );
	write( Fd, buff, strlen(buff) );

	close( Fd );

	return( 0 );
err:
	return( -1 );
}

#define	TAG_PD					1
#define	TAG_PORT				2
#define	TAG_USER				3
#define	TAG_TWAIT				4
#define	TAG_OPN_TABLE			5
#define	TAG_OPN_TABLE_USER		6
#define	TAG_OPN_TABLE_TWAIT		7
#define	TAG_OPN_TABLE_REC		8
#define	TAG_OPN_TABLE_RWAIT		9
#define	TAG_OPN_TABLE_RWAIT_END	10

#define	TAG_END				99

int
PingGlobalMarshal( IOReadWrite_t *pW )
{
	int			tag;
	p_id_t		*pd;
	r_port_t	*pe;
	r_usr_t		*ud, *uId;
	r_twait_t	*tw;
	r_tbl_t		*td;
	int		i;

	/* pd */
	list_for_each_entry( pd, &_neo_port, i_lnk ) {

		pe	= (r_port_t*)pd->i_tag;
		if( !pe )	continue;

		tag	= TAG_PD;
		IOWrite( pW, &tag, sizeof(tag) );
		IOWrite( pW, pd, sizeof(*pd) );

		tag	= TAG_PORT;
		IOWrite( pW, &tag, sizeof(tag) );
		IOWrite( pW, pe, sizeof(*pe) );

		list_for_each_entry( ud, &pe->p_usr_ent, u_port ) {
			tag	= TAG_USER;

			LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor),LogINF,
			"u_Id=%p", ud->u_Id ); 

			ud->u_Id	= ud;
			IOWrite( pW, &tag, sizeof(tag) );
			IOWrite( pW, ud, sizeof(*ud) );
		}
		if( (tw = pe->p_twait) ) {
			tag	= TAG_TWAIT;
			tw->w_Id	= tw;
			IOWrite( pW, &tag, sizeof(tag) );
			IOWrite( pW, tw, sizeof(*tw) );
		}
	}

	/* open table */
	list_for_each_entry( td, &_svc_inf.f_opn_tbl, t_lnk ) {
		tag	= TAG_OPN_TABLE;
		IOWrite( pW, &tag, sizeof(tag) );

		IOWrite( pW, td, sizeof(*td) );

		IOWrite( pW, &td->t_scm->s_n, sizeof(td->t_scm->s_n) );
		IOWrite( pW, td->t_scm, 
				sizeof(r_item_t)*(td->t_scm->s_n-1) + sizeof(r_scm_t) );
		if( td->t_depend ) {
			IOWrite( pW, &td->t_depend->l_n, sizeof(td->t_depend->l_n) );
			IOWrite( pW, td->t_depend,
				sizeof(r_tbl_mod_t)*(td->t_depend->l_n-1)+sizeof(r_tbl_list_t));
		}

		list_for_each_entry( ud, &td->t_usr, u_tbl ) {
			tag	= TAG_OPN_TABLE_USER;
			ASSERT( ud->u_Id == ud );
			IOWrite( pW, &tag, sizeof(tag) );
			IOWrite( pW, &ud, sizeof(ud) );	//  -> u_Id
		}
		list_for_each_entry( tw, &td->t_wait, w_tbl ) {
			tag	= TAG_OPN_TABLE_TWAIT;
			IOWrite( pW, &tag, sizeof(tag) );
			ASSERT( tw->w_Id == tw );
			IOWrite( pW, tw, sizeof(*tw) );
		}

		tag	= TAG_OPN_TABLE_REC;
		IOWrite( pW, &tag, sizeof(tag) );

		IOWrite( pW, td->t_rec, sizeof(r_rec_t)*td->t_rec_num );
		for( i = 0; i < td->t_rec_num; i++ ) {
			IOWrite( pW, td->t_rec[i].r_head, td->t_rec_size );
			if( (ud = td->t_rec[i].r_wait ) ) {
				list_for_each_entry( uId, &ud->u_rwait, u_rwait ) {
					tag	= TAG_OPN_TABLE_RWAIT;
					IOWrite( pW, &tag, sizeof(tag) );
					IOWrite( pW, &uId, sizeof(uId) );
				}
				tag	= TAG_OPN_TABLE_RWAIT_END;
				IOWrite( pW, &tag, sizeof(tag) );
			}
		}
	}
	tag	= TAG_END;
	IOWrite( pW, &tag, sizeof(tag) );
	return( 0 );
}

p_id_t*
PingFindPd( p_Tag_t *pT )
{
	p_id_t	*pd;

	list_for_each_entry( pd, &_neo_port, i_lnk ) {
		if( pd->i_pTag == pT ) {
			return( pd );
		}
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogERR, 
	"NOT FOUND pT=%p", pT ); 

	return( NULL );
}

r_usr_t*
PingFindUser( r_usr_t *uId , r_port_t **ppPE )
{
	p_id_t		*pd;
	r_port_t	*pe;
	r_usr_t		*ud;

	list_for_each_entry( pd, &_neo_port, i_lnk ) {

		pe	= (r_port_t*)pd->i_tag;
		if( !pe )	continue;

		list_for_each_entry( ud, &pe->p_usr_ent, u_port ) {
			if( ud->u_Id == uId ) {
				if( ppPE )	*ppPE	= pe;
				return( ud );
			}
		}
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogERR,
	"NOT FOUND uId=%p", uId ); 
	return( NULL );
}

r_twait_t*
PingFindTwait( r_twait_t *tId )
{
	p_id_t		*pd;
	r_port_t	*pe;
	r_twait_t	*tw;

	list_for_each_entry( pd, &_neo_port, i_lnk ) {

		pe	= (r_port_t*)pd->i_tag;
		if( !pe )	continue;

		if( (tw = pe->p_twait) ) {
			if( tw->w_Id == tId ) {
				return( tw );
			}
		}
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogERR,
	"NOT FOUND tId=%p", tId ); 

	return( NULL );
}

void	hash_destroy();

int
PingGlobalUnmarshal( IOReadWrite_t *pR )
{
	int	tag;
	p_id_t		*pd;
	r_port_t	*pe;
	r_usr_t		*ud, *udH, *uId;
	r_twait_t	*tw, *tId;
	r_tbl_t		*td = NULL;
	int	n;
	int	l;
	int	i;

	while( IORead( pR, &tag, sizeof(tag) ) >= 0 ) {

		switch( tag ) {

		case TAG_PD:
			pd	= (p_id_t*)PingMalloc( sizeof(*pd) );
			IORead( pR, pd, sizeof(*pd) );

			pd->i_local.p_addr.a_pd	= pd;
			_dl_init( &pd->i_lnk );
			_dl_insert( &pd->i_lnk, &_neo_port );
			break;

		case TAG_PORT:
			pe	= (r_port_t*)PingMalloc( sizeof(*pe) );
			pd->i_tag	= pe;
			IORead( pR, pe, sizeof(*pe) );
			_dl_init( &pe->p_usr_ent );
			HashInit( &pe->p_HashUser, PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR,
				NULL, neo_malloc, neo_free, NULL );
			break;

		case TAG_USER:
			ud	= (r_usr_t*)PingMalloc( sizeof(*ud) );
			IORead( pR, ud, sizeof(*ud) );
			_dl_init( &ud->u_tbl );	
			_dl_init( &ud->u_port );	
			_dl_init( &ud->u_twait );	
			_dl_init( &ud->u_rwait );	
			_dl_insert( &ud->u_port, &pe->p_usr_ent );
			ud->u_pd	= pd;

			LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
			"u_Id=%p", ud->u_Id ); 
/* HashPut はテーブルで*/
			break;

		case TAG_TWAIT:
			tw	= (r_twait_t*)PingMalloc( sizeof(*tw) );	
			IORead( pR, tw, sizeof(*tw) );
			pe->p_twait	= tw;
			_dl_init( &tw->w_tbl );
			_dl_init( &tw->w_usr );
			tw->w_pd	= pd;
/* リンクはテーブルで */
			break;
		case TAG_OPN_TABLE:
			td	= (r_tbl_t*)PingMalloc( sizeof(*td) );
			IORead( pR, td, sizeof(*td) );
			_dl_init( &td->t_lnk );
			_dl_insert( &td->t_lnk, &_svc_inf.f_opn_tbl );
			HashPut( &_svc_inf.f_HashOpen, td->t_name, td );
			_dl_init( &td->t_usr );
			_dl_init( &td->t_wait );
			HashInit( &td->t_HashEqual, PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR,
					NULL, neo_malloc, neo_free, hash_destroy );	
			HashInit( &td->t_HashRecordLock, PRIMARY_11, HASH_CODE_PNT, 
					HASH_CMP_PNT, NULL, neo_malloc, neo_free, NULL );	

			IORead( pR, &n, sizeof(n) );
			l =	sizeof(r_item_t)*(n-1)+sizeof(r_scm_t);
			td->t_scm	= (r_scm_t*)PingMalloc( l );
			IORead( pR, td->t_scm, l );

			if( td->t_depend ) {
				IORead( pR, &n, sizeof(n) );
				l =	sizeof(r_tbl_mod_t)*(n-1)+sizeof(r_tbl_list_t);
				td->t_depend	= (r_tbl_list_t*)PingMalloc( l );
				IORead( pR, td->t_depend, l );
			}
			break;

		case TAG_OPN_TABLE_USER:
			IORead( pR, &uId, sizeof(uId) );

			LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
			"uId=%p", uId ); 

			ud	= PingFindUser( uId, &pe );
			_dl_insert( &ud->u_tbl, &td->t_usr );	
			ud->u_mytd	= td;
			HashPut( &pe->p_HashUser, td->t_name, ud );
			break;

		case TAG_OPN_TABLE_TWAIT:
			IORead( pR, &tId, sizeof(tId) );
			tw	= PingFindTwait( tId );
			_dl_insert( &tw->w_tbl, &td->t_wait );	
			tw->w_td	= td;
			break;

		case TAG_OPN_TABLE_REC:
			if( rec_cntl_alloc( td ) )	goto err;
			IORead( pR, td->t_rec, sizeof(r_rec_t)*td->t_rec_num );

			if( rec_buf_alloc( td ) )	goto err;
			for( i = 0; i < td->t_rec_num; i++ ) {
				IORead( pR, td->t_rec[i].r_head, td->t_rec_size );
				if( td->t_rec[i].r_access ) {
					td->t_rec[i].r_access	= 
							PingFindUser( td->t_rec[i].r_access, NULL );
				}
				if( td->t_rec[i].r_wait ) {
					udH = PingFindUser( td->t_rec[i].r_wait, NULL );
					td->t_rec[i].r_wait	= udH;
					while( IORead( pR, &tag, sizeof(tag) ) >= 0 ) {
						if( tag	== TAG_OPN_TABLE_RWAIT_END )	break;
						IORead( pR, &uId, sizeof(uId) );
						ud	= PingFindUser( uId, NULL );
						_dl_insert( &ud->u_rwait, &udH->u_rwait );
					}
				}
			}
			break;

		case TAG_END:
			goto end;
		}
	}
end:
	list_for_each_entry( td, &_svc_inf.f_opn_tbl, t_lnk ) {
		if( td->t_tag ) {
			TBL_OPEN_SET( td );
		}
	}
	list_for_each_entry( pd, &_neo_port, i_lnk ) {

		pe	= (r_port_t*)pd->i_tag;
		if( !pe )	continue;

		list_for_each_entry( ud, &pe->p_usr_ent, u_port ) {
			ud->u_Id	= ud;
		}
		if( (tw = pe->p_twait) ) {
			tw->w_Id	= tw;
		}
	}
	return( 0 );
err:
	return( -1 );
}

#define	ADAPTOR_NOP	0
#define	ADAPTOR_SET	1

int
PingAdaptorMarshal( IOReadWrite_t *pW, struct PaxosAccept *pAccept )
{
	p_id_t	*pd;
	p_Tag_t	*pT;
	int		tag;

	pd	= (p_id_t*)PaxosAcceptGetTag( pAccept, XJPING_FAMILY );
	if( pd ) {
		tag	= ADAPTOR_SET;
		IOWrite( pW, &tag, sizeof(tag) );

		pT	= pd->i_pTag;
ASSERT( list_empty( &pT->t_Msgs ) );
		IOWrite( pW, &pT, sizeof(pT) );	// pd 識別用
		IOWrite( pW, pT, sizeof(*pT) );
	} else {
		tag	= ADAPTOR_NOP;
		IOWrite( pW, &tag, sizeof(tag) );
	}
	return( 0 );
}

int
PingAdaptorUnmarshal( IOReadWrite_t *pR, struct PaxosAccept *pAccept )
{
	p_id_t	*pd;
	p_Tag_t	*pT;
	int		tag;

	IORead( pR, &tag, sizeof(tag) );
	if( tag == ADAPTOR_NOP )	return( 0 );

	IORead( pR, &pT, sizeof(pT) );
	pd	= PingFindPd( pT );
	pT	= (p_Tag_t*)PingMalloc( sizeof(*pT) );
	IORead( pR, pT, sizeof(*pT) );
	pT->t_pAccept	= pAccept;
	pT->t_pSession	= NULL;
	list_init( &pT->t_Msgs );
	pd->i_pTag	= pT;

	PaxosAcceptSetTag( pAccept, XJPING_FAMILY, pd );
	return( 0 );
}

int
PingSessionLock()
{
	RwLockW( &CNTRL.c_RwLock );

	return( 0 );
}

int
PingSessionUnlock()
{
	RwUnlock( &CNTRL.c_RwLock );
	return( 0 );
}

int
PingSessionAny( struct PaxosAcceptor* pAcceptor, 
					PaxosSessionHead_t* pM, int fdSend )
{
	PFSHead_t*	pF = (PFSHead_t*)(pM+1);
	int	Ret;

	PFSRASReq_t		*pReqRAS;
	PFSRASRpl_t		RplRAS;

	struct Session	*pSession;

	switch( (int)pF->h_Head.h_Cmd ) {
	case PFS_RAS:
		pReqRAS = (PFSRASReq_t*)pF;

		pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, NULL, NULL, pReqRAS->r_Cell );

		PaxosSessionSetLog( pSession, PaxosAcceptorGetLog(pAcceptor));

		char *pPath = RootOmitted(pReqRAS->r_Path);
		Ret = PFSRasThreadCreate( PaxosAcceptorGetCell(pAcceptor)->c_Name,
								pSession, pPath,
								PaxosAcceptorMyId(pAcceptor),
								_PFSRasSend );
LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogDBG,
"c_Name[%s] pPath[%s], Id[%d]", PaxosAcceptorGetCell(pAcceptor)->c_Name,
								pPath, PaxosAcceptorMyId(pAcceptor));

    	MSGINIT( &RplRAS, PFS_RAS, 0, sizeof(RplRAS) );
		RplRAS.r_Head.h_Error	= Ret;
		RplRAS.r_Head.h_Head.h_Cksum64	= 0;
		if( pAcceptor->c_bCksum ) {
			RplRAS.r_Head.h_Head.h_Cksum64	= 
						in_cksum64( &RplRAS, RplRAS.r_Head.h_Head.h_Len, 0 );
		}
		SendStream( fdSend, (char*)&RplRAS, sizeof(RplRAS) );
		break;
	default:
		break;
	}
	return( 0 );
}

#define	DB_UPD	"DB_UPD"
#define	DB_DEL	"DEL_LIST"
#define	DB_TAR	"DB_TAR"

Msg_t*
PingBackupPrepare( int num )
{
	Msg_t	*pMsgList;

	pMsgList = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );

	STFileList( _svc_inf.f_path, pMsgList );
	if( num > 0 ) {
		Hash_t		*pTree;
		IOFile_t	*pF;
		char		Path[PATH_NAME_MAX];
		StatusEnt_t	*pEnt;

		pTree	= STTreeCreate( pMsgList );

		pF = IOFileCreate( DB_UPD, O_RDONLY, 0, Malloc, Free );
		while( fgets( Path, sizeof(Path), pF->f_pFILE ) ) {
			if( Path[strlen(Path)-1] == '\n' )	Path[strlen(Path)-1] = 0;
			pEnt = STTreeFindByPath( Path + strlen(_svc_inf.f_path)+1, pTree );
			//ASSERT( pEnt );
			if ( pEnt )
				pEnt->e_Type	|= ST_NON;
		}
		IOFileDestroy( pF, FALSE );

		STTreeDestroy( pTree );
	} else {	
		int	fd;

		fd = open( DB_UPD, O_RDWR|O_TRUNC|O_CREAT, 0666 );
		close( fd );
	}

	return( pMsgList );
}

static	int	AgainBackup;

int
PingBackup( PaxosSessionHead_t *pM )
{
	Msg_t	*pMsgNew, *pMsgOld, *pMsgDel, *pMsgUpd;
	StatusEnt_t	*pS, *pSEnd;
	MsgVec_t	Vec;
	IOFile_t	*pF;
	PathEnt_t	*pP;
	int			i;
	char	Path[FILENAME_MAX];
	int	cnt;
	time_t	Alive;
	timespec_t	AliveSpec;
	int	total;
	size_t	Size;

	AliveSpec = PaxosGetAliveTime(PaxosAcceptorGetPaxos(CNTRL.c_pAcceptor), 
								pM->h_From );
	Alive	= AliveSpec.tv_sec;

	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
		"From=%d Alive=%llu", pM->h_From, Alive );

	pMsgOld = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );
	pMsgNew = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );
	pMsgDel = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );
	pMsgUpd = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );

	if( pM->h_Len > sizeof(PaxosSessionHead_t) ) {
		pSEnd	= (StatusEnt_t*)((char*)pM + pM->h_Len);
		pS		= (StatusEnt_t*)(pM+1);
		while( pS != pSEnd ) {
			MsgVecSet( &Vec, 0, pS, sizeof(*pS), sizeof(*pS) );
			MsgAdd( pMsgOld, &Vec );
			pS++;
		}
	}
	STFileList( _svc_inf.f_path, pMsgNew );
	STDiffList( pMsgOld, pMsgNew, Alive, pMsgDel, pMsgUpd );

	pF = IOFileCreate( DB_DEL, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										neo_malloc, neo_free );
	for( i = 0; i < pMsgDel->m_N; i++ ) {
		IOWrite( (IOReadWrite_t*)pF, &pMsgDel->m_aVec[i].v_Len,
										sizeof(pMsgDel->m_aVec[i].v_Len) );
		IOWrite( (IOReadWrite_t*)pF, pMsgDel->m_aVec[i].v_pStart,
										pMsgDel->m_aVec[i].v_Len );
	}
	IOFileDestroy( pF, FALSE );

	pF = IOFileCreate( DB_UPD, O_WRONLY|O_TRUNC|O_CREAT, 0, 
										neo_malloc, neo_free );
	for( cnt = 0, total = 0, Size = 0, pP = (PathEnt_t*)MsgFirst(pMsgUpd); 
				pP; pP = ( PathEnt_t*)MsgNext(pMsgUpd) ) {

		if( Size < TAR_SIZE_MAX ) {
			sprintf( Path, "%s/%s\n", _svc_inf.f_path, (char*)(pP+1) );
			IOWrite( (IOReadWrite_t*)pF, Path, strlen(Path) );
			Size	+= pP->p_Size;
			cnt++;
		}
		total++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF,
	"UPD[%d/%d]", cnt, total );

	AgainBackup	= total - cnt;

	IOFileDestroy( pF, FALSE );

	MsgDestroy( pMsgOld );
	MsgDestroy( pMsgNew );
	MsgDestroy( pMsgDel );
	MsgDestroy( pMsgUpd );

	char	command[128];
	int		status;

	unlink( DB_TAR );
	sprintf( command, "tar -cf %s -T %s", DB_TAR, DB_UPD );

LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogDBG,
"TAR BEFORE[%s]", command );

	status = system( command );

LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogDBG, "TAR AFTER" );
ASSERT( !status );
	return( 0 );
}

int
PingRestore()
{
	IOFile_t	*pF;
	PathEnt_t	*pP;
	Msg_t	*pMsgDel;
	MsgVec_t	Vec;
	int		i;
	int		Len;
	char	Path[FILENAME_MAX];
	int		cnt;

	pMsgDel = MsgCreate( ST_MSG_SIZE, neo_malloc, neo_free );
	pF = IOFileCreate( DB_DEL, O_RDONLY, 0, neo_malloc, neo_free );
	if( !pF ) {
		MsgDestroy( pMsgDel );
		return( -1 );
	}
	while( IORead( (IOReadWrite_t*)pF, &Len, sizeof(Len) ) > 0 ) {
		pP 	= (PathEnt_t*)MsgMalloc( pMsgDel, Len );
		IORead( (IOReadWrite_t*)pF, pP, Len );
		MsgVecSet(&Vec, VEC_MINE, pP, Len, Len);
		MsgAdd( pMsgDel, &Vec );
	}
	for( cnt = 0, i = pMsgDel->m_N - 1; i >= 0; i-- ) {
		pP	= (PathEnt_t*)pMsgDel->m_aVec[i].v_pStart;
		sprintf( Path, "%s/%s", _svc_inf.f_path, (char*)(pP+1) );

		if( pP->p_Type & ST_DIR )	rmdir( Path );
		else						unlink( Path );
		cnt++;
	}
	LOG( PaxosAcceptorGetLog(CNTRL.c_pAcceptor), LogINF, "DEL[%d]", cnt );

	IOFileDestroy( pF, FALSE );
	MsgDestroy( pMsgDel );

	char	command[128];
	int		status;

	sprintf( command, "tar -xf %s", DB_TAR );
	status = system( command );
ASSERT( !status );
	return( 0 );
}

int
PingTransferSend( IOReadWrite_t *pW )
{
	struct stat	Stat;
	int	Ret;
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];

	Ret = stat( DB_DEL, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( DB_DEL, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );

	Ret = stat( DB_UPD, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( DB_UPD, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );

	Ret = stat( DB_TAR, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( DB_TAR, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );
	/*
	 *	AGAIN?
	 */
	IOWrite( pW, &AgainBackup, sizeof(AgainBackup) );

	return( 0 );
}

int
PingTransferRecv( IOReadWrite_t *pR )
{
	long long	Size;
	int	fd, n;
	char	Buff[1024*4];
	int	Ret;

	IORead( pR, &Size, sizeof(Size) );
	fd = open( DB_DEL, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( DB_UPD, O_WRONLY|O_APPEND, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Size, sizeof(Size) );
	fd = open( DB_TAR, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );

	IORead( pR, &Ret, sizeof(Ret) );

	return( Ret );
}

#define	OUTOFBAND_PATH( FilePath, pId ) \
	({char * p; snprintf( FilePath, sizeof(FilePath), "%s/%s-%d-%d-%d-%d-%ld", \
		PAXOS_SESSION_OB_PATH, \
		inet_ntoa((pId)->i_Addr.sin_addr), \
	      (pId)->i_Pid, (pId)->i_No, (pId)->i_Seq, (pId)->i_Try, \
	      (pId)->i_Time); \
		p = FilePath; while( *p ){if( *p == '.' ) *p = '_';p++;}})

int
OutOfBandGarbage()
{
	char	command[128];
	int		Ret;

	sprintf( command, "rm -rf %s", PAXOS_SESSION_OB_PATH );
	Ret	= system( command );
	if( Ret )	return( Ret );
	sprintf( command, "mkdir %s", PAXOS_SESSION_OB_PATH );
	Ret	= system( command );
	return( Ret );
}

int
OutOfBandSave( struct PaxosAcceptor* pAcceptor, struct PaxosSessionHead* pM )
{
	// Overwrite
	char	FileOut[PATH_NAME_MAX];
	int		fd;

	OUTOFBAND_PATH( FileOut, &pM->h_Id );

	fd = open( FileOut, O_WRONLY|O_TRUNC|O_CREAT/*|O_NONBLOCK*/, 0666 );
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

	if( !(pH = (PaxosAcceptorGetfMalloc(pAcceptor))( H.h_Len )) )	goto err1;

	*pH	= H;
	if( read( fd, (char*)(pH+1), H.h_Len - sizeof(H) )
								!= H.h_Len - sizeof(H) )	goto err2; 

	close( fd );

	return( pH );
err2:
	(PaxosAcceptorGetfFree(pAcceptor))( pH );
err1:
	close( fd );
err:
	return( NULL );
}

int
Load( void *pTag )
{
	return( PaxosAcceptorGetLoad( (struct PaxosAcceptor*)pTag ) );
}

PaxosCell_t			*pPaxosCell;
PaxosSessionCell_t	*pSessionCell;
PaxosSessionCell_t	*pRasSessionCell;

void
usage()
{
printf( "xjPingPaxos [opt] cellname id directory {FALSE|TRUE [seq]}\n");
printf("\txjPingPaxos   --- load module\n");
printf("\tcellname      --- cell name\n");
printf("\tid            --- server id\n");
printf("\tdirectory     --- paxos/DB directory\n");
printf("	FALSE				... synchronize and wait decision and join\n");
printf("	TRUE [seq]			... synchronize and join in paxos\n");
printf("\t-c Core			... core number\n");
printf("\t-e Extension		... extension number\n");
printf("\t-s {1|0}			... checksum on/off\n");
printf("	-F FdMax			... file descriptor max\n");
printf("	-B BlockMax			... cache block max\n");
printf("	-S SegmentSize		... segment size\n");
printf("	-N SegmentNumber	... number of segments in a block\n");
printf("							BlockSize=SegmentSize*SegmentNumber\n");
printf("	-U interval 		... write back interval(usec)\n");
printf("\t-W Workers 		... number of accept Workers\n");
printf("	-L Level 			... log level\n");
}

int
main( int ac, char* av[] )
{
	int	j;
	int	Ret;
	int		Core = DEFAULT_CORE, Ext = DEFAULT_EXTENSION/*, Maximum*/;
	bool_t	bCksum = DEFAULT_CKSUM_ON;
	int64_t	PaxosUsec	= DEFAULT_PAXOS_USEC;
	int64_t	MemLimit = DEFAULT_OUT_MEM_MAX;
	int		Workers = DEFAULT_WORKERS;
	char	AdminPort[NEO_MAXPATHLEN];
	char	*pPort = NULL;
	int		LogLevel	= DEFAULT_LOG_LEVEL;
	int		FdMax = DEFAULT_FD_MAX;
	int		BlockMax = DEFAULT_BLOCK_MAX;
	int		SegSize = DEFAULT_SEG_SIZE, SegNum = DEFAULT_SEG_NUM;
	int64_t	WBUsec	= DEFAULT_WB_USEC;

	PaxosStartMode_t sMode;	uint32_t seq = 0;

	/*
	 * Opts
	 */
	j = 1;
	while( j < ac ) {
		char	*pc;
		int		l;
		pc = av[j++];
		if( *pc != '-' )	break;
		l = strlen( pc );
		if( l < 2 )	continue;

		switch( *(pc+1) ) {
		case 'c':	Core = atoi(av[j++]);	break;
		case 'e':	Ext = atoi(av[j++]);	break;
		case 's':	bCksum = atoi(av[j++]);	break;
		case 'u':	PaxosUsec = atol(av[j++]);	break;
		case 'M':	MemLimit = atol(av[j++]);	break;
		case 'W':	Workers = atoi(av[j++]);	break;
		case 'F':	FdMax = atoi(av[j++]);	break;
		case 'B':	BlockMax = atoi(av[j++]);	break;
		case 'S':	SegSize = atoi(av[j++]);	break;
		case 'N':	SegNum = atoi(av[j++]);	break;
		case 'U':	WBUsec = atol(av[j++]);	break;
		case 'P':	pPort = av[j++];	break;
		case 'L':	LogLevel = atoi(av[j++]);	break;
		}
	}
	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}

	if( ac < 5 ) {
		usage();
		exit( -1 );
	}
	if( !strcmp("TRUE", av[4] ) ) {
		if( ac >= 6 ) {
			seq	= atoll(av[5]);
			sMode	= PAXOS_SEQ;
		} else {
			sMode	= PAXOS_NO_SEQ;
		}
	} else {
		sMode	= PAXOS_WAIT;
	}
	if( chdir( av[3] ) ) {
		printf("Can't change directory[%s].\n", av[3] );
		exit( -1 );
	}
/*
 *	Log
 */
	int			log_size;

	if( getenv("LOG_SIZE") ) {
		log_size = atoi( getenv("LOG_SIZE") );
		pLog	= LogCreate( log_size, neo_malloc, neo_free, LOG_FLAG_BINARY, 
							stderr, LogLevel );
		if( !pLog )	exit( -1 );

		LOG( pLog, LogINF, "###### LOG_SIZE[%d] ####", log_size );
		neo_log	= pLog;
	}
/*
 * init xjPing
 */
	if( pPort ) {	// Single
		if( PingInitDB( pPort, "DB", TRUE ) ) {
			printf("Can't open [%s].\n", "DB" );
		}
		PingInit( NULL, 0, 0, 0, 0, 0, 0 );
		return( 0 );
	} else {		// Paxos
		Ret = snprintf( AdminPort, sizeof(AdminPort), 
				"/tmp/"XJPING_ADMIN_PORT"_%s_%s", av[1], av[2] );
		if( Ret >= sizeof(AdminPort) )	return( -1 );
		if( PingInitDB( AdminPort, "DB", FALSE ) ) {
			printf("Can't open [%s].\n", "DB" );
			return( -1 );
		}
	}
/*
 *	MyId
 */
	MyId	= atoi(av[2]);
/*
 *	Clear out-of-band data
 */
	if( OutOfBandGarbage() ) {
		printf("Can't garbage out-of-band.\n");
		return( -1 );
	}
/*
 *	Address
 */
	pPaxosCell		= PaxosCellGetAddr( av[1], Malloc );
	pSessionCell	= PaxosSessionCellGetAddr( av[1], Malloc );
	if( !pPaxosCell || !pSessionCell ) {
		printf("Can't get cell addr[%s:%s].\n", av[1], av[2] );
		exit( -1 );
	}
/*
 *	Initialization
 */
	struct PaxosAcceptor* pAcceptor;
	PaxosAcceptorIF_t	IF;

	memset( &IF, 0, sizeof(IF) );

	IF.if_fMalloc	= neo_malloc;
	IF.if_fFree		= neo_free;

	IF.if_fSessionOpen	= PingSessionOpen;
	IF.if_fSessionClose	= PingSessionClose;

	IF.if_fRequest	= PingRequest;
	IF.if_fValidate	= PingValidate;
	IF.if_fAccepted	= PingAccepted;
	IF.if_fRejected	= PingRejected;
	IF.if_fAbort	= PingAbort;
	IF.if_fShutdown	= PingShutdown;

	IF.if_fBackupPrepare	= PingBackupPrepare;
	IF.if_fBackup			= PingBackup;
	IF.if_fRestore			= PingRestore;
	IF.if_fTransferSend		= PingTransferSend;
	IF.if_fTransferRecv		= PingTransferRecv;

	IF.if_fGlobalMarshal	= PingGlobalMarshal;
	IF.if_fGlobalUnmarshal	= PingGlobalUnmarshal;

	IF.if_fAdaptorMarshal	= PingAdaptorMarshal;
	IF.if_fAdaptorUnmarshal	= PingAdaptorUnmarshal;

	IF.if_fLock		= PingSessionLock;
	IF.if_fUnlock	= PingSessionUnlock;
	IF.if_fAny		= PingSessionAny;

	IF.if_fOutOfBandSave	= OutOfBandSave;
	IF.if_fOutOfBandLoad	= OutOfBandLoad;
	IF.if_fOutOfBandRemove	= OutOfBandRemove;

	IF.if_fLoad	= Load;

	pAcceptor = (struct PaxosAcceptor*)neo_malloc( PaxosAcceptorGetSize() );

	PaxosAcceptorInitUsec( pAcceptor, MyId, Core, Ext, MemLimit, bCksum,
			pSessionCell, pPaxosCell, PaxosUsec, Workers, &IF );

	PingInit( pAcceptor, FdMax, BlockMax, SegSize, SegNum, WBUsec, bCksum );

	if( pLog ) {

		PaxosAcceptorSetLog( pAcceptor, pLog );
		PaxosSetLog( PaxosAcceptorGetPaxos( pAcceptor ), pLog );
	}
	if( PaxosAcceptorStartSync( pAcceptor, sMode, seq, NULL ) < 0 ) {
		printf("START ERROR\n");
		exit( -1 );
	}
	return( 0 );

}

