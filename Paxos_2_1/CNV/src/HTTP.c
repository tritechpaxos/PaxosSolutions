/******************************************************************************
*
*  HTTP.c 	--- HTTP protocol
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
 *	ì¬			“n•Ó“TF
 *	ŽŽŒ±
 *	“Á‹–A’˜ìŒ 	ƒgƒ‰ƒCƒeƒbƒN
 */

//#define	_DEBUG_

#include	"CNV.h"

struct	sockaddr_in	Server;

#define	CR	'\r'
#define	LF	'\n'

#define	GET		"GET "
#define	POST	"POST "
#define	PUT		"PUT "
#define	DELETE	"DELETE "
#define	TRACE	"TRACE "

#define	LENGTH	"Content-Length: "

#define	MAX_LINE	4096

static	int		DumpInit;
static	Mtx_t	DumpLock;
int
HTTPDump( Msg_t *pMsg, char *pTitle )
{
	char	Buff[MAX_LINE];
	char 	C, *pC, *pB;
	int		i;

	if( !DumpInit ) {
		MtxInit( &DumpLock );
		DumpInit	= 1;
	}
	MtxLock( &DumpLock );

	Printf("=== HTTPDump Start(%s) m_Err=%d ===\n", pTitle, pMsg->m_Err );
	for( i = 0, pC = MsgFirst( pMsg ); pC; i++, pC = MsgNext( pMsg ) ) {
again:
		pB	= Buff;
		while( (C = *pC++) ) {
			if( C == '\r' ) {
				*pB++ = '\\'; *pB++ = 'r';
			} else if( C == '\n' ) {
				*pB++ = '\\'; *pB++	= 'n'; *pB++ = '\n'; *pB = 0; break;
			} else {
				*pB++ = C;
			}
		}
		Printf("%d %s", i, Buff );

		if( Buff[0] == '\\' && Buff[1] == 'r' 
			&& Buff[2] == '\\' && Buff[3] == 'n' )	break;

		if( *pC ) {
			goto again;
		}
	}
	Printf("=== HTTPDump End ===\n");
	MtxUnlock( &DumpLock );

	return( 0 );
}

static int
HTTPGetLine( int fd, MsgVec_t *pVec, Msg_t *pMsg )
{
	char	*pS, *pC, *pLine;
	int		i, n, n1, size;

	size	= MAX_LINE;
	pLine	= (char*)MsgMalloc( pMsg, size );
	pS		= pLine;

	MsgVecSet( pVec, VEC_MINE, pLine, size, 0 );

	while( TRUE ) {
		ASSERT( size > 0 );
		n = recv( fd, pS, size, MSG_PEEK );
		if( n == 0 )	errno = EPIPE;
		if( n <= 0 ) {
			return( -1 );
		}

		for( pC = pS, i = 0; i < n; i++, pC++ ) {
			if( *pC == LF ) {
				n1 = recv( fd, pS, i+1, 0 );
				ASSERT( n1 == i+1 );
				pVec->v_Len	+= i + 1;
				goto OK;
			}
		}
		n1 = recv( fd, pS, n, 0 );
		ASSERT( n1 == n );
		pVec->v_Len	+= n;
		pS			+= n;
		size		-= n;
	}
OK:
	pVec->v_pStart[pVec->v_Len]	= 0;
	return( 0 );
}

void*
HTTPHeadByMsg( Msg_t *pMsg )
{
	int	fd	= PNT_INT32( pMsg->m_pTag0 );
	MsgVec_t	Vec;
	int		Body	= 0;
	char	*pBody;
	char	Num[128], *pNum;

	while( TRUE ) {
		if( HTTPGetLine( fd, &Vec, pMsg ) < 0 ) {
			pMsg->m_Err	= errno;
perror("HTTPHeadByMsg");
			return( NULL );
		}
		MsgAdd( pMsg, &Vec );

		if( Vec.v_Len == 1 && ((char*)Vec.v_pStart)[0] == LF )	break;
		if( Vec.v_Len == 2 && ((char*)Vec.v_pStart)[0] == CR
							&& ((char*)Vec.v_pStart)[1] == LF )	break;

		if(Vec.v_Len >= strlen(LENGTH) 
				&& !strncmp( Vec.v_pStart, LENGTH, strlen(LENGTH) ) ) {
			strncpy( Num, Vec.v_pStart+strlen(LENGTH), sizeof(Num) );
			for( pNum = Num; *pNum; pNum++ ) {
				if( *pNum == CR )	*pNum = 0;
				if( *pNum == LF )	*pNum = 0;
			}
			Body	= atoi( Num );
		}
	}
	if( Body > 0 ) {
		pBody	= (pMsg->m_fMalloc)( Body );
		/*n = */RecvStream( fd, pBody, Body );

		MsgVecSet( &Vec, VEC_MINE, pBody, Body, Body );

		MsgAdd( pMsg, &Vec );
	}
	return( NULL );
}

void*
HTTPMethodByMsg( Msg_t *pMsg )
{
	int	fd	= PNT_INT32( pMsg->m_pTag0 );
	bool_t	*pUpdate = (bool_t*)pMsg->m_pTag1;
	MsgVec_t	Vec;
	char		*pMethod;
	int			Len;
	int			i;

	// Method
	if( HTTPGetLine( fd, &Vec, pMsg ) < 0 ) {
		pMsg->m_Err	= errno;
perror("HTTPMethodByMsg");
		goto err;
	}
	MsgAdd( pMsg, &Vec );

	pMethod	= MsgFirst( pMsg );
	Len		= MsgLen( pMsg );

	if((Len >= strlen(POST) && !strncmp( pMethod, POST, strlen(POST)))
	|| (Len >= strlen(PUT) && !strncmp( pMethod, PUT, strlen(PUT))) 
	|| (Len >= strlen(DELETE) && !strncmp( pMethod, DELETE, strlen(DELETE)))) {

		*pUpdate	= TRUE;

	} else if( Len >= strlen(GET) && !strncmp( pMethod, GET, strlen(GET) ) ) {
		for( i = 0; i < Len; i++ ) {
			if( pMethod[i] == '?' ) {
				*pUpdate = TRUE;
				break;
			}
		}
	}
	return( HTTPHeadByMsg );
err:
	return( NULL );
}

Msg_t*
HTTPRequestByMsg( struct Session *pSession, int fd, bool_t *pUpdate, 
			void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	Msg_t*	pMsg;

	*pUpdate	= FALSE;

	pMsg	= MsgCreate( 10, fMalloc, fFree );

	pMsg->m_pTag0	= INT32_PNT( fd );
	pMsg->m_pTag1	= pUpdate;

	MsgEngine( pMsg, HTTPMethodByMsg );

	return( pMsg );
}

void*
HTTPStatusByMsg( Msg_t* pMsg )
{
	int	fd	= PNT_INT32( pMsg->m_pTag0 );
	MsgVec_t	Vec;

	// Status
	if( HTTPGetLine( fd, &Vec, pMsg ) < 0 ) {
		pMsg->m_Err	= errno;
perror("HTTPStatusByMsg");
		return( NULL );
	}
	MsgAdd( pMsg, &Vec );
	return( HTTPHeadByMsg );
}

Msg_t*
HTTPResponseByMsg( int fd, void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	Msg_t*		pMsg;

	pMsg	= MsgCreate( 10, fMalloc, fFree);

	pMsg->m_pTag0	= INT32_PNT( fd );

	MsgEngine( pMsg, HTTPStatusByMsg );

	return( pMsg );
}

Msg_t*
HTTPReqAndRplServer( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	int	fd;
	int	Ret;
	Msg_t	*pRes;

	MsgVec_t	Vec;
	Msg_t		*pMsg;
char	REQ[1024];
sprintf( REQ, "RequestToServer" );

	MsgVecSet( &Vec, 0, pM+1, pM->h_Len-sizeof(*pM), pM->h_Len-sizeof(*pM) );
	pMsg	= MsgCreate( 1, Malloc, Free );
	MsgAdd( pMsg, &Vec );
	HTTPDump( pMsg, REQ );
	MsgDestroy( pMsg );

again:
	fd	= PNT_INT32( PaxosAcceptGetTag( pAccept, CNV_FAMILY ) );
	if( fd <= 0 ) {
		if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
			perror("socket open error");
			return( NULL );
		}
		if( connect( fd, (struct sockaddr*)&Server, 
							sizeof(struct sockaddr_in)) < 0 ) {
			perror("connect error");
			return( NULL );
		}
		PaxosAcceptSetTag( pAccept, CNV_FAMILY, INT32_PNT(fd) );
	}
	Ret = send( fd, (pM+1), pM->h_Len - sizeof(*pM), 0 );
	if( Ret < 0 ) {
DBG_PRT("SEND errno=%d fd=%d\n", errno, fd );
		close( fd );
		PaxosAcceptSetTag( pAccept, CNV_FAMILY, NULL );
		goto again;
	}
		
	pRes	= HTTPResponseByMsg( fd, 
					PaxosAcceptGetfMalloc(pAccept), 
					PaxosAcceptGetfFree(pAccept) );
	if( pRes->m_Err == EPIPE ) {
DBG_PRT("EPIPE errno=%d fd=%d\n", errno, fd );
		close( fd );
		pRes->m_Err	= 0;
		PaxosAcceptSetTag( pAccept, CNV_FAMILY, NULL );
		goto again;
	}
	return( pRes );	
}

int
HTTPOpen( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	int	fd;

	if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror("socket open error");
		return( -1 );
	}
	if( connect( fd, (struct sockaddr*)&Server, 
							sizeof(struct sockaddr_in)) < 0 ) {
		perror("connect error");
		return( -1 );
	}
	PaxosAcceptSetTag( pAccept, CNV_FAMILY, INT32_PNT(fd) );
	return( 0 );
}

int
HTTPClose( struct PaxosAccept* pAccept, PaxosSessionHead_t* pM )
{
	int	fd;

	fd	= PNT_INT32( PaxosAcceptGetTag( pAccept, CNV_FAMILY ) );
	close( fd );
	return( 0 );
}

CnvIF_t	HTTP_IF = {
/***
	HTTPOpen, HTTPClose, HTTPReqAndRplServer, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, HTTPRequestByMsg
***/
	.IF_fOpen= 				HTTPOpen, 
	.IF_fClose= 			HTTPClose, 
	.IF_fRequest=			HTTPRequestByMsg
};
