/******************************************************************************
*
*  xjPing.c 	---xjPing protocol
*
*  Copyright (C) 2011 triTech Inc. All Rights Reserved.
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

#include	"../../xjPing/h/neo_db.h"

#include	"Converter.h"

struct	sockaddr_in	Server;

#define	OLD_LIST	"OLD_LIST"
#define	DEL_LIST	"DEL_LIST"
#define	UPD_LIST	"UPD_LIST"
#define	TAR			"TAR"

p_head_t*
xjPacket( int fd, void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	p_head_t	head, *pHead;
	int		l;

	if( PeekStream( fd, (char*)&head, sizeof(head) ) < 0 ) {
		return( NULL );
	} 
	l = head.h_len + sizeof(head);
	pHead	= Malloc( l );

	RecvStream( fd, (char*)pHead, l );

	return( pHead );
}

Msg_t*
xjPingRequest( int fd, bool_t *pUpdate, 
			void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	Msg_t*	pMsg;
	MsgVec_t	Vec;
	p_head_t	*pHead;
	p_head_t head;
	int		l;

again:
	*pUpdate	= TRUE;

	pHead	= xjPacket( fd, fMalloc, fFree );
	if( !pHead )	return( NULL );
	switch( pHead->h_cmd ) {
		case R_CMD_DUMP_PROC:
		case R_CMD_DUMP_INF:
		case R_CMD_DUMP_RECORD:
		case R_CMD_DUMP_TBL:
		case R_CMD_DUMP_USR:
		case R_CMD_DUMP_MEM:
		case R_CMD_DUMP_CLIENT:
			memcpy( &head, pHead, sizeof(head) );
			head.h_err = E_RDB_NOSUPPORT;
			(fFree)( pHead );
			SendStream( fd, (char*)&head, sizeof(head) );
			goto again;

		case R_CMD_FIND:
		case R_CMD_LIST:
			*pUpdate = FALSE;
			break;
	}

	pMsg	= MsgCreate( 1, fMalloc, fFree );

	l = pHead->h_len + sizeof(*pHead);
	MsgVecSet( &Vec, VEC_MINE, pHead, l, l );
	MsgAdd( pMsg, &Vec );

	return( pMsg );
}

int
xjMulti( p_head_t *pHead )
{
	int	Ret = 0;
	int	i;
	char	*pC;
	r_req_sql_t	*pSql;

	if( pHead->h_cmd == R_CMD_SQL ) {
		pSql = (r_req_sql_t*)pHead;
LOG( pLog, "[%s]", (char*)(pSql+1) );
		for( pC = (char*)(pSql+1), i = 0; 
				i < pHead->h_len - (sizeof(r_req_sql_t)-sizeof(p_head_t)); 
						pC++, i++ ) {
			if( *pC == ';' )	Ret++;
		}
	} else {
		Ret = 1;
	}
	return( Ret );
} 

typedef struct xjPingAdaptor {
	int				a_Fd;
	int				a_CreateHandle;
	p_req_con_t		a_ReqCon;
	r_req_link_t	a_ReqLink;
} xjPingAdaptor_t;

Msg_t*
xjPingReqAndRplServer( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	int	fd;
	int	Ret;
	Msg_t	*pRes;
	MsgVec_t	Vec;
	Msg_t		*pMsg;
	p_head_t	*pHead;
	int	m, l;
	xjPingAdaptor_t	*pAdaptor;

	MsgVecSet( &Vec, 0, pM+1, pM->h_Len-sizeof(*pM), pM->h_Len-sizeof(*pM) );
	pMsg	= MsgCreate( 1,
					PaxosAcceptGetfMalloc(pAccept), 
					PaxosAcceptGetfFree(pAccept) );
	MsgAdd( pMsg, &Vec );
	MsgDestroy( pMsg );

	pAdaptor	= (xjPingAdaptor_t*)PaxosAcceptGetTag( pAccept, CNV_FAMILY );
	fd	= pAdaptor->a_Fd;
	ASSERT( fd );
	if( !pAdaptor->a_CreateHandle ) {
		if( connect( fd, (struct sockaddr*)&Server, 
							sizeof(struct sockaddr_in)) < 0 ) {
			return( NULL );
		}
		pAdaptor->a_CreateHandle = 1;
	}
	pHead	= (p_head_t*)(pM+1);
	m = xjMulti( pHead );
LOG( pLog, "ENT:cmd=0x%x m=%d", pHead->h_cmd, m );
again:
	switch( pHead->h_cmd ) {
		case P_LINK_CON:
			memcpy( &pAdaptor->a_ReqCon, pHead, sizeof(pAdaptor->a_ReqCon ) );
			break;
		case R_CMD_LINK:
			memcpy( &pAdaptor->a_ReqLink, pHead, sizeof(pAdaptor->a_ReqLink ) );
			break;
	}
	Ret = SendStream( fd, (char*)(pM+1), pM->h_Len - sizeof(*pM) );
	if( Ret < 0 ) {
		return( NULL );
	}
		
	pRes	= MsgCreate( m+1,
					PaxosAcceptGetfMalloc(pAccept), 
					PaxosAcceptGetfFree(pAccept) );

	do {
more:
		pHead	= xjPacket( fd,
					PaxosAcceptGetfMalloc(pAccept), 
					PaxosAcceptGetfFree(pAccept) );
		if( pHead == NULL ) {
			close( fd );
			MsgDestroy( pRes );
			PaxosAcceptSetTag( pAccept, CNV_FAMILY, NULL );
			goto again;
		}
		l	= pHead->h_len + sizeof(*pHead);
		MsgVecSet( &Vec, VEC_MINE, pHead, l, l );
		MsgAdd( pRes, &Vec );

		if( pHead->h_cmd == R_CMD_FIND ) {
			r_res_find_t	*pResFind = (r_res_find_t*)pHead;
			if( pResFind->f_more )	goto more;
		}

	} while( --m > 0 );

LOG( pLog, "EXT:cmd=0x%x", pHead->h_cmd );
	return( pRes );	
}

int
xjPingStop()
{
	int	Ret;
	char command[256];

	sprintf( command, "../xjPingStop.sh %d", MyId );
	Ret = system( command );

	LOG( pLog, "Ret=%d", Ret );

	return( Ret );
}

int
xjPingRestart()
{
	int	Ret;
	char command[256];

	sprintf( command, "../xjPingRestart.sh %d", MyId );
	Ret = system( command );
	return( Ret );
}

Msg_t*
xjPingBackupPrepare()
{
	char command[256];
	int	Ret;
	Msg_t	*pMsg;
	MsgVec_t	Vec;
	StatusEnt_t	*pS, S;
	IOFile_t	*pF;

	sprintf( command, "../xjPingPrepare.sh %d", MyId );
	Ret = system( command );
	ASSERT( !Ret );
	
	pMsg = MsgCreate( ST_MSG_SIZE, Malloc, Free );
	pF	= IOFileCreate( OLD_LIST, O_RDONLY, 0, Malloc, Free );
	while( IORead( (IOReadWrite_t*)pF, &S, sizeof(S) ) > 0 ) {
		pS	= (StatusEnt_t*)MsgMalloc( pMsg, sizeof(S) );
		*pS	= S;
		MsgVecSet( &Vec, VEC_MINE, pS, sizeof(*pS), sizeof(*pS) );
		MsgAdd( pMsg, &Vec );
	}
	IOFileDestroy( pF, TRUE );
	return( pMsg );
}

int
xjPingBackup( PaxosSessionHead_t* pM )
{
	StatusEnt_t	*pS, *pSEnd;
	IOFile_t	*pF;
	char command[256];
	int	Ret;

	pF	= IOFileCreate( OLD_LIST, O_WRONLY|O_TRUNC|O_CREAT, 0, Malloc, Free );
	if( pM->h_Len > sizeof(PaxosSessionHead_t) ) {
		pSEnd	= (StatusEnt_t*)((char*)pM + pM->h_Len);
		pS		= (StatusEnt_t*)(pM+1);
		while( pS != pSEnd ) {
			IOWrite( (IOReadWrite_t*)pF, pS, sizeof(*pS) );
			pS++;
		}
	}
	IOFileDestroy( pF, TRUE );

	sprintf( command, "../xjPingBackup.sh %d", MyId );
	Ret = system( command );
LOG( pLog, "Ret=%d", Ret );
	ASSERT( !Ret );
	return( 0 );
}

int
xjPingRestore()
{
	char command[256];
	int	Ret;

	sprintf( command, "../xjPingRestore.sh %d", MyId );
	Ret = system( command );
	ASSERT( !Ret );
	return( 0 );
}

int
xjPingTransferSend( IOReadWrite_t *pW )
{
	struct stat	Stat;
	int	Ret;
	int	Size;
	int	fd, n;
	char	Buff[1024*4];

	/*
	 *	Delete file
	 */
	Ret = stat( DEL_LIST, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( DEL_LIST, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );

	/*
	 *	Tar file
	 */
	Ret = stat( TAR, &Stat );
	ASSERT( Ret == 0 );

	Size	= Stat.st_size;

	IOWrite( pW, &Size, sizeof(Size) );

	fd = open( TAR, O_RDONLY, 0666 );
	if( fd < 0 )	return( -1 );

	while( (n = read( fd, Buff, sizeof(Buff) ) ) > 0 ) {
		IOWrite( pW, Buff, n );
	}
	close( fd );
	return( 0 );
}

int
xjPingTransferRecv( IOReadWrite_t *pR )
{
	int	Size;
	int	fd, n;
	char	Buff[1024*4];

	/*
	 *	Delete file
	 */
	IORead( pR, &Size, sizeof(Size) );

	fd = open( DEL_LIST, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );
	/*
	 *	Tar file
	 */
	IORead( pR, &Size, sizeof(Size) );

	fd = open( TAR, O_RDWR|O_TRUNC|O_CREAT, 0666 );
	if( fd < 0 )	return( -1 );
	while( Size > 0 ) {
		n	= ( Size < sizeof(Buff) ? Size : sizeof(Buff) ); 
		IORead( pR, Buff, n );
		write( fd, Buff, n );
		Size	-= n;
	}
	close( fd );
	return( 0 );
}

/*
 *	autonomicでも接続に行く
 *	xjPingは接続プロトコルを待つので
 *	従って、connectは最初の要求で行う
 */
int
xjPingOpenServer( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	int	fd;
	xjPingAdaptor_t	*pAdaptor;

	if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err;
	}
	pAdaptor = (xjPingAdaptor_t*)Malloc( sizeof(*pAdaptor) );
	memset( pAdaptor, 0, sizeof(*pAdaptor) );

	pAdaptor->a_Fd	= fd;
	
	PaxosAcceptSetTag( pAccept, CNV_FAMILY, pAdaptor );

LOG( pLog, "Fd=%d", pAdaptor->a_Fd );
	return( 0 );
err:
	LOG( pLog, "errno=%d", errno );
	return( -1 );
}

int
xjPingCloseServer( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	xjPingAdaptor_t	*pAdaptor;

	pAdaptor	= (xjPingAdaptor_t*)PaxosAcceptGetTag( pAccept, CNV_FAMILY );
LOG( pLog, "Fd=%d", pAdaptor->a_Fd );
	close( pAdaptor->a_Fd );
	PaxosAcceptSetTag( pAccept, CNV_FAMILY, NULL );
	Free( pAdaptor );
	return( 0 );
}

int
xjPingAdaptorMarshal( IOReadWrite_t *pW, struct PaxosAccept *pAccept )
{
	xjPingAdaptor_t	*pAdaptor;

LOG( pLog, "AdaptorMarshal" );
	pAdaptor	= (xjPingAdaptor_t*)PaxosAcceptGetTag( pAccept, CNV_FAMILY );

	IOWrite( pW, pAdaptor, sizeof(*pAdaptor) );
	return( 0 );
}

int
xjPingAdaptorUnmarshal( IOReadWrite_t *pR, struct PaxosAccept *pAccept )
{
	xjPingAdaptor_t	*pAdaptor;
	p_res_con_t		ResCon;
	r_res_link_t	ResLink;
	int	fd;
	int	Ret;

LOG( pLog, "AdaptorUnmarshal" );
	pAdaptor = (xjPingAdaptor_t*)Malloc( sizeof(*pAdaptor) );
	if( !pAdaptor ) {
		goto err;
	}
	IORead( pR, pAdaptor, sizeof(*pAdaptor) );

LOG( pLog, "RECONNECT" );
	if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err1;
	}
	if( connect( fd, (struct sockaddr*)&Server, 
							sizeof(struct sockaddr_in)) < 0 ) {
		goto err2;
	}
	Ret = SendStream( fd, 
					(char*)&pAdaptor->a_ReqCon, sizeof(pAdaptor->a_ReqCon) );
	Ret = RecvStream( fd, (char*)&ResCon, sizeof(ResCon) );
LOG( pLog, "ResCon.c_head.h_err=%d", ResCon.c_head.h_err );
	if( Ret || ResCon.c_head.h_err ) {
		goto err2;
	}
	Ret = SendStream( fd, 
					(char*)&pAdaptor->a_ReqLink, sizeof(pAdaptor->a_ReqLink) );
	Ret = RecvStream( fd, (char*)&ResLink, sizeof(ResLink) );
LOG( pLog, "ResLink.l_head.h_err=%d", ResLink.l_head.h_err );
	if( Ret || ResLink.l_head.h_err ) {
		goto err2;
	}
	pAdaptor->a_Fd	= fd;
	pAdaptor->a_CreateHandle = 1;

	PaxosAcceptSetTag( pAccept, CNV_FAMILY, pAdaptor );
	return( 0 );

err2:
	close( fd );
err1:
	Free( pAdaptor );
err:
	return( -1 );
}

CnvIF_t	xjPing_IF = {
	.IF_fOpen= 				xjPingOpenServer, 
	.IF_fClose= 			xjPingCloseServer, 
	.IF_fReqAndRplServer=	xjPingReqAndRplServer, 
	.IF_fBackupPrepare=		xjPingBackupPrepare, 
	.IF_fBackup=			xjPingBackup, 
	.IF_fRestore=			xjPingRestore,
	.IF_fTransferSend=		xjPingTransferSend, 
	.IF_fTransferRecv=		xjPingTransferRecv,
	.IF_fRequest=			xjPingRequest,
	.IF_fAdaptorMarshal=	xjPingAdaptorMarshal,
	.IF_fAdaptorUnmarshal=	xjPingAdaptorUnmarshal,
	.IF_fStop=				xjPingStop,
	.IF_fRestart=			xjPingRestart,
};
