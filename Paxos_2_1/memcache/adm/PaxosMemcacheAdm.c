/******************************************************************************
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

#include	"PaxosMemCache.h"

struct	Log	*pLog;

int
MemClose( int Fd )
{
	MemCacheLock_req_t	Req;
	MemCacheLock_rpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, MEM_CLOSE, 0, 0, sizeof(Req) );
	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
MemLock( int Fd )
{
	MemCacheLock_req_t	Req;
	MemCacheLock_rpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, MEM_LOCK, 0, 0, sizeof(Req) );
	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
MemUnlock( int Fd )
{
	MemCacheUnlock_req_t	Req;
	MemCacheUnlock_rpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, MEM_UNLOCK, 0, 0, sizeof(Req) );
	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
AddPair( int Fd, AcceptMemcached_t *pPair )
{
	int	Ret;
	MemAddAcceptMemcached_req_t	Req;
	MemAddAcceptMemcached_rpl_t	Rpl;

	MemLock( Fd );

	SESSION_MSGINIT( &Req, MEM_ADD_ACCEPT_MEMCACHED, 0, 0, sizeof(Req) );
	Req.a_AcceptMemcached	= *pPair;

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno = Rpl.a_Head.h_Error;
	if( errno )	goto err1;

	MemUnlock( Fd );

	return( 0 );
err1:
	perror("FAILED" );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
DelPair( int Fd, AcceptMemcached_t *pPair )
{
	int	Ret;
	MemDelAcceptMemcached_req_t	Req;
	MemDelAcceptMemcached_rpl_t	Rpl;

	MemLock( Fd );

	SESSION_MSGINIT( &Req, MEM_DEL_ACCEPT_MEMCACHED, 0, 0, sizeof(Req) );
	Req.d_AcceptMemcached	= *pPair;

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno = Rpl.a_Head.h_Error;
	if( errno )	goto err1;

	MemUnlock( Fd );
	return( 0 );
err1:
	perror("FAILED" );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
SetSockAddr( char *pPort, struct sockaddr *pAddr )
{
	int		s;
	char	Host[PATH_MAX], *pC;
	short	Port = 11211;
    struct addrinfo hints;
    struct addrinfo *result = NULL;
	struct in_addr	pin_addr = {0,};
	struct sockaddr_in *pAddrIN = (struct sockaddr_in*)pAddr;

	memset( Host, 0, sizeof(Host) );
	strcpy( Host, pPort );
	pC = strtok( Host, ":" );
	pC	+= strlen(pC) + 1;
	if( *pC ) {
		Port = atoi( pC );	
	}
    memset(&hints, 0, sizeof(hints));
    hints.ai_family 	= AF_INET;
    hints.ai_socktype 	= SOCK_STREAM;
    hints.ai_flags 		= 0;
    hints.ai_protocol 	= 0;
    hints.ai_canonname 	= NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

 	s = getaddrinfo( Host, NULL, &hints, &result);
	if( s < 0 || !result )	return( -1 );

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;

	freeaddrinfo( result );

	pAddrIN->sin_family	= AF_INET;
	pAddrIN->sin_addr	= pin_addr;
	pAddrIN->sin_port	= htons(Port);

	return( 0 );
}

MemCacheCtrl_t	Ctrl, *pCtrl;

int
DumpMem( int Fd, char *pDataRemote, char *pData, int Len )
{
	int	Ret;

	MemCacheDump_req_t	Req;
	MemCacheDump_rpl_t	Rpl;

	SESSION_MSGINIT( &Req, MEM_DUMP, 0, 0, sizeof(Req) );
	Req.d_pAddr	= pDataRemote;
	Req.d_Len	= Len;

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, pData, Rpl.d_Head.h_Len - sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
DumpCtrl( int Fd )
{
	int	Ret;

	MemCacheCtrl_req_t	Req;
	MemCacheCtrl_rpl_t	Rpl;

	SESSION_MSGINIT( &Req, MEM_CTRL, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	pCtrl	= Rpl.c_pCtrl;
	Ret = DumpMem( Fd, (char*)pCtrl, (char*)&Ctrl, sizeof(Ctrl) );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

void
PrintCon( MemCachedCon_t *pCon )
{
	printf( "m_Fd=%d m_FdBin=%d Addr[%s:%d]\n", pCon->m_Fd, pCon->m_FdBin, 
		inet_ntoa(pCon->m_Addr.sin_addr), ntohs(pCon->m_Addr.sin_port) );
}

int
DumpConPool( int Fd )
{
	int	Ret;
	MemCachedCon_t	*pCon, Con;
	int	i = 0;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	Ret = DumpCtrl( Fd );
	if( Ret < 0 )	goto err1;

	printf("=== Connection Pool Cnt=%d ===\n", Ctrl.mc_MemCacheds.ge_Cnt);

	LIST_FOR_EACH_ENTRY( pCon, &Ctrl.mc_MemCacheds.ge_Lnk, m_Elm.e_Lnk, 
								&Ctrl, pCtrl  ) {

		Ret = DumpMem( Fd, (char*)pCon, (char*)&Con, sizeof(Con) );

		printf("%d:", i );	PrintCon( &Con );

		i++;

		pCon	= &Con;
	}

	MemUnlock( Fd );
	return( 0 );
err1:
	MemUnlock( Fd );
err:
	return( -1 );
}

int
DumpEvent( int Fd )
{
	int	Ret;
	FdEvent_t	FdEvent, *pFdEvent;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	Ret = DumpCtrl( Fd );
	if( Ret < 0 )	goto err1;

#ifdef	_NOT_LIBEVENT_
	printf("=== Event EpollFd=%d Cnt=%d ===\n", 
			Ctrl.mc_FdEvent.ct_EpollFd, Ctrl.mc_FdEvent.ct_Cnt);
#else
	printf("=== Event Cnt=%d ===\n", Ctrl.mc_FdEvClient.ct_Cnt);
#endif

	LIST_FOR_EACH_ENTRY( pFdEvent, &Ctrl.mc_FdEvClient.ct_HashLnk.hl_Lnk, fd_Lnk, 
								&Ctrl, pCtrl  ) {

		Ret = DumpMem( Fd, (char*)pFdEvent, (char*)&FdEvent, sizeof(FdEvent) );

		printf("Fd=%d Type=%d Events=0x%x\n", 
			FdEvent.fd_Fd, FdEvent.fd_Type, FdEvent.fd_Events );

		pFdEvent	= &FdEvent;
	}

	MemUnlock( Fd );
	return( 0 );
err1:
	MemUnlock( Fd );
err:
	return( -1 );
}

int
DumpMsg( int Fd, Msg_t *pMsg )
{
	Msg_t		Msg;
	int	/*i, ii,*/ j, k;
	int	N, l;
	MsgVec_t	*aVec;
	char		Buf[70], c, buf[5];
	int	Ret;

	Ret = DumpMem( Fd, (char*)pMsg, (char*)&Msg, sizeof(Msg) );

	N	= Msg.m_N;
	aVec	= (MsgVec_t*)Malloc( (l= N*sizeof(MsgVec_t)) );
	Ret = DumpMem( Fd, (char*)Msg.m_aVec, (char*)aVec, l );

	printf("pTag0=%p pTag1=%p pTag2=%p N=%d\n",
			 Msg.m_pTag0, Msg.m_pTag1, Msg.m_pTag2, Msg.m_N );
	for( j = 0; j < N; j++ ) {

		l = aVec[j].v_Len > sizeof(Buf) ? 
								sizeof(Buf) : aVec[j].v_Len;

		Ret = DumpMem( Fd, aVec[j].v_pStart, Buf, l );

		printf(" %d len=%d flg(0x%x)[", j, aVec[j].v_Len, aVec[j].v_Flags );

		for( k = 0; k < l; k++ ) {
			c = Buf[k];
			if( c == '\r' )	sprintf( buf, "\\r" );
			else if( c == '\n' )	sprintf( buf, "\\n" );
			else sprintf(buf, "%c", c&0x7f );

			printf("%s", buf );
		}
		printf("]\n");
	}
	return( Ret );
}

int
DumpMemItem( int Fd )
{
	int	Ret;
	GElm_t	*pElm;
	MemItem_t	MemItem, *pMemItem;
	PFSDlg_t	*pDlg;
	int	i, ii;
	MemItemAttrs_t	Attrs, *pAttrs;
	MemItemAttr_t	*pAttr;
	struct sockaddr_in	*pAddrIN;
	int	Len;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	Ret = DumpCtrl( Fd );
	if( Ret < 0 )	goto err1;

	printf(
	"### MemItem Max[%d] Cnt[%d] Cksum[%d] UsecCksum[%d] SecExp[%d] ###\n", 
		Ctrl.mc_MemItem.dc_Ctrl.ge_MaxCnt, Ctrl.mc_MemItem.dc_Ctrl.ge_Cnt,
		Ctrl.mc_bCksum, Ctrl.mc_UsecCksum, Ctrl.mc_SecExp);

	i = 0;
	LIST_FOR_EACH_ENTRY( pElm, &Ctrl.mc_MemItem.dc_Ctrl.ge_Lnk, e_Lnk, 
								&Ctrl, pCtrl  ) {

		pMemItem = container_of( pElm, MemItem_t, i_Dlg.d_GE );

		memset( &MemItem, 0, sizeof(MemItem) );
		Ret = DumpMem( Fd, (char*)pMemItem, (char*)&MemItem, sizeof(MemItem) );

		pElm	= &MemItem.i_Dlg.d_GE;
		pDlg	= &MemItem.i_Dlg.d_Dlg;

		printf( "=== %d. Dlg{key[%s] Own[%d] Ver[%d]} "
				"\n\t\tKey[%s] Status=0x%x GElm{Cnt[%d]} ===",
					i++, pDlg->d_Name, pDlg->d_Own, pDlg->d_Ver, 
					MemItem.i_Key, MemItem.i_Status, pElm->e_Cnt );
		printf("\n");
		if( MemItem.i_pAttrs ) {
			Ret = DumpMem( Fd, (char*)MemItem.i_pAttrs, 
								(char*)&Attrs, sizeof(Attrs) );

printf( "---Meta: a_MetaCAS=%"PRIu64" a_Epoch=%"PRIu64" a_I=%d a_N=%d---\n", 
			Attrs.a_MetaCAS, Attrs.a_Epoch, Attrs.a_I, Attrs.a_N );

			Len	= sizeof(MemItemAttrs_t) + sizeof(MemItemAttr_t)*(Attrs.a_N-1);

			pAttrs	= (MemItemAttrs_t*)Malloc( Len );
			Ret = DumpMem( Fd, (char*)MemItem.i_pAttrs, (char*)pAttrs, Len );

			for( ii = 0; ii < Attrs.a_N; ii++ ) {
				pAttr	= &pAttrs->a_aAttr[ii];
				pAddrIN	= (struct sockaddr_in*)&pAttr->a_MemCached;
				printf(
			"+++ memcached:%d [%s:%d] Flags=0x%x Bytes=%"PRIu64"" 
				" MetaCAS=%"PRIu64" CAS=%"PRIu64" Epoch=%"PRIu64"\n"
				"\ttime(0)=%lu ExpTime=[%d:%ld] CksumTime[%d:%ld]\n"
				"\tCksum64=%"PRIx64" pData=%p pDiff=%p\n",
					ii, inet_ntoa(pAddrIN->sin_addr), ntohs(pAddrIN->sin_port),
					pAttr->a_Flags, pAttr->a_Bytes, 
					pAttr->a_MetaCAS, pAttr->a_CAS, pAttr->a_Epoch,time(0),
					(int)pAttr->a_ExpTime.tv_sec, pAttr->a_ExpTime.tv_nsec, 
					(int)pAttr->a_CksumTime.tv_sec, pAttr->a_CksumTime.tv_nsec,
					pAttr->a_Cksum64, pAttr->a_pData, pAttr->a_pDiff );

				printf("- Data -\n");
				if( pAttr->a_pData ) {
					Ret = DumpMsg( Fd, pAttr->a_pData );
				}
				printf("- Diff -\n");
				if( pAttr->a_pDiff ) {
					Ret = DumpMsg( Fd, pAttr->a_pDiff );
				}
			}
			printf("+++\n");
			Free( pAttrs );
		}
		pElm	= &MemItem.i_Dlg.d_GE;
		printf("=== \n");
	}
	MemUnlock( Fd );
	return( 0 );
err1:
	MemUnlock( Fd );
err:
	return( -1 );
}

int
DumpPair( int Fd )
{
	AcceptMemcached_t	*pPair, Pair;
	struct sockaddr_in	*pAddrIN;
	int	i;
	int	Ret;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	Ret = DumpCtrl( Fd );
	if( Ret < 0 )	goto err1;

	printf("=== Pair Cnt=%d ===\n", Ctrl.mc_Pair.hl_Hash.h_Count );

	i = 0;
	LIST_FOR_EACH_ENTRY( pPair, &Ctrl.mc_Pair.hl_Lnk, am_Lnk, 
								&Ctrl, pCtrl  ) {

		Ret = DumpMem( Fd, (char*)pPair, (char*)&Pair, sizeof(Pair) );

		pAddrIN	= (struct sockaddr_in*)&Pair.am_Accept;
		printf("%d. Client[%s:%d] - ", 
				i, inet_ntoa(pAddrIN->sin_addr), ntohs(pAddrIN->sin_port) );
		pAddrIN	= (struct sockaddr_in*)&Pair.am_Memcached;
 		printf("Server[%s:%d]\n",
				inet_ntoa(pAddrIN->sin_addr), ntohs(pAddrIN->sin_port) );

		pPair	= &Pair;
		i++;
	}
	MemUnlock( Fd );
	return( 0 );
err1:
	MemUnlock( Fd );
err:
	return( -1 );
}

int
Stop( int Fd )
{
	MemStop_req_t	Req;
	MemStop_rpl_t	Rpl;
	int	Ret;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	SESSION_MSGINIT( &Req, MEM_STOP, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
DumpLog( int Fd )
{
	MemLog_req_t	Req;
	MemLog_rpl_t	Rpl;
	int	Ret;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	SESSION_MSGINIT( &Req, MEM_LOG_DUMP, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	MemUnlock( Fd );
	return( 0 );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
RAS( int Fd, char *pCellName )
{
	MemRASRegist_req_t	Req;
	MemRASRegist_rpl_t	Rpl;
	int	Ret;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	SESSION_MSGINIT( &Req, MEM_RAS_REGIST, 0, 0, sizeof(Req) );
	strncpy( Req.r_CellName, pCellName, sizeof(Req.r_CellName) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	MemUnlock( Fd );
	return( 0 );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
UnRAS( int Fd, char *pCellName )
{
	MemRASUnregist_req_t	Req;
	MemRASUnregist_rpl_t	Rpl;
	int	Ret;

	Ret = MemLock( Fd );
	if( Ret < 0 )	goto err;

	SESSION_MSGINIT( &Req, MEM_RAS_UNREGIST, 0, 0, sizeof(Req) );
	strncpy( Req.u_CellName, pCellName, sizeof(Req.u_CellName) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	MemUnlock( Fd );
	return( 0 );
err:
	MemUnlock( Fd );
	return( -1 );
}

int
Active( int Fd, char *pCellName )
{
	MemActive_req_t	Req;
	MemActive_rpl_t	Rpl;
	int	Ret;
	struct timeval Timeout = { 0, 500000};

	SESSION_MSGINIT( &Req, MEM_ACTIVE, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = FdEventWait( Fd, &Timeout );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

void
usage()
{
printf("PaxosMemcacheAdm Id add client[:port] server[:port]\n");
printf("PaxosMemcacheAdm Id del client[:port] server[:port]\n");
printf("PaxosMemcacheAdm Id event\n");
printf("PaxosMemcacheAdm Id con_pool\n");
printf("PaxosMemcacheAdm Id item\n");
printf("PaxosMemcacheAdm Id pair\n");
printf("PaxosMemcacheAdm Id stop\n");
printf("PaxosMemcacheAdm Id log\n");
printf("PaxosMemcacheAdm Id ras cell\n");
printf("PaxosMemcacheAdm Id unras cell\n");
printf("PaxosMemcacheAdm Id active\n");
}

int
main( int ac, char *av[] )
{
	int	Ret;
	int	Id;
	struct sockaddr_un	AddrAdm;
	int	Fd;

	AcceptMemcached_t	Pair;

	if( ac < 3 ) {
		usage();
		goto err;
	}
	/* connect to AdmPort */
	Id	= atoi(av[1]);
	AddrAdm.sun_family	= AF_UNIX;
	sprintf( AddrAdm.sun_path, PAXOS_MEMCACHE_ADM_FMT, Id );

	Fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( Fd < 0 ) {
		goto err;
	}
	if( connect( Fd, (struct sockaddr*)&AddrAdm, sizeof(AddrAdm) ) < 0 ) {
		close( Fd );
		goto err;
	}
	if( !strcmp( "add", av[2] ) ) {
		if( ac < 5 ) {
			usage();
			goto err1;
		}
		/* pair */
		memset( &Pair, 0, sizeof(Pair) );

		Ret = SetSockAddr( av[3], &Pair.am_Accept );
		if( Ret < 0 )	goto err1;
		Ret = SetSockAddr( av[4], &Pair.am_Memcached );
		if( Ret < 0 )	goto err1;

		Ret = AddPair( Fd, &Pair );
		if( Ret < 0 )	goto err1;

	} else if( !strcmp( "del", av[2] ) ) {
		if( ac < 5 ) {
			usage();
			goto err1;
		}
		/* pair */
		memset( &Pair, 0, sizeof(Pair) );

		Ret = SetSockAddr( av[3], &Pair.am_Accept );
		if( Ret < 0 )	goto err1;
		Ret = SetSockAddr( av[4], &Pair.am_Memcached );
		if( Ret < 0 )	goto err1;

		Ret = DelPair( Fd, &Pair );
		if( Ret < 0 )	goto err1;

	} else if( !strcmp( "con_pool", av[2] ) ) {
		Ret = DumpConPool( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "event", av[2] ) ) {
		Ret = DumpEvent( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "item", av[2] ) ) {
		Ret = DumpMemItem( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "pair", av[2] ) ) {
		Ret = DumpPair( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "stop", av[2] ) ) {
		Ret = Stop( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "log", av[2] ) ) {
		Ret = DumpLog( Fd );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "ras", av[2] ) ) {
		Ret = RAS( Fd, av[3] );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "unras", av[2] ) ) {
		Ret = UnRAS( Fd, av[3] );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "active", av[2] ) ) {
		Ret = Active( Fd, av[3] );
		if( Ret < 0 )	goto err1;
	} else {
		usage();
		goto err1;
	}
	MemClose( Fd );

	return( 0 );
err1:
	MemClose( Fd );
err:
	return( -1 );
}

