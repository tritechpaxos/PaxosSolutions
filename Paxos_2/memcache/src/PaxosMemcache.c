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
#include	<ctype.h>

struct	Log	*pLog;

MemCacheCtrl_t	MemCacheCtrl;

int SetItemReplyFakeText( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI );
int SetItemReplyFakeBinary( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI );
int SetItemRequestText( FdEvCon_t *pEvCon, Msg_t * pMsg, MemItem_t *pMI );
int SetItemReplyText( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI );
int SetItemReplyBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI );
int SetItemSet( FdEvCon_t *pEvCon, Msg_t *pMsg, 
				Msg_t *pMsgServer, MemItem_t *pMI );
int SetItemSetText( FdEvCon_t *pEvCon, Msg_t *pMsg, 
				Msg_t *pMsgServer, MemItem_t *pMI );
int SetItemSetBinary( FdEvCon_t *pEvCon, Msg_t *pMsg, 
				Msg_t *pMsgServer, MemItem_t *pMI );

int GetItemRegister( FdEvCon_t*, Msg_t*, Msg_t *pMsg, MemItem_t *pMI );
int GetItemReply( FdEvCon_t *pEvCon, Msg_t*, MemItem_t *pMI );

int SendClientError( FdEvCon_t *pEvCon, char *pError );
int SendEnd( FdEvCon_t *pEvCon );
int RequestGetsText( FdEvCon_t *pEvCon, char *pKey );

int SendBinaryHeadWithErr( FdEvCon_t *pEvCon, BinHead_t *pHead, char *pErr );
int SendBinaryByFd( int Fd, Msg_t *pMsg );

int	SendMsgByFd( int Fd, Msg_t *pMsg );

int DeleteItemRequest( FdEvCon_t *pEvCon, Msg_t *pMsg );
int DeleteItemRequestText( FdEvCon_t *pEvCon, Msg_t *pMsg );
int DeleteItemReply( FdEvCon_t *pEvCon, 
		Msg_t *pMsg, Msg_t *pMsgServer, MemItem_t* pMI );
int DeleteItemReplyText( FdEvCon_t *pEvCon, 
		Msg_t *pMsg, Msg_t *pMsgServer, MemItem_t* pMI );
int DeleteItemReplyBinary( FdEvCon_t *pEvCon, 
		Msg_t *pMsg, Msg_t *pMsgServer, MemItem_t* pMI );

int ReplyRetCodeText( FdEvCon_t *pEvCon, RetCode_t RetCode );
int ReplyRetCodeBinary( FdEvCon_t *pEvCon, RetCode_t RetCode, 
						Msg_t *pMsgClient, MemItem_t *pMI );

Msg_t	*RecvServerTextByFd( int Fd );

uint64_t GetMetaCAS( Msg_t *pMsg );
uint64_t GetMetaEpoch( Msg_t *pMsg );
uint64_t SetMetaEpoch( Msg_t *pMsg, uint64_t Epoch );

void	DestroyMsg( Msg_t *pMsg );
Msg_t*	CloneMsg( Msg_t *pMsg );

#define	MEM_LockW()		RwLockW( &MemCacheCtrl.mc_RwLock )
#define	MEM_LockR()		RwLockR( &MemCacheCtrl.mc_RwLock )
#define	MEM_Unlock()	RwUnlock( &MemCacheCtrl.mc_RwLock )

#define	MEM_ITEM_FLUSH	(void*)0x1001
#define	MEM_ITEM_STOP	(void*)0x1003

DlgCache_t*
MemSpaceLockW( FdEvCon_t *pEvCon)
{
	DlgCache_t *pD;

	pD = DlgCacheLockWBy( &MemCacheCtrl.mc_MemSpace, 
					MemCacheCtrl.mc_NameSpace, TRUE, NULL, NULL, pEvCon ); 
	return( pD );
}

DlgCache_t*
MemSpaceLockR( FdEvCon_t *pEvCon )
{
	DlgCache_t *pD;

	pD = DlgCacheLockRBy( &MemCacheCtrl.mc_MemSpace, 
					MemCacheCtrl.mc_NameSpace, TRUE, NULL, NULL, pEvCon ); 

	return( pD );
}

int
MemSpaceUnlock( DlgCache_t *pD, FdEvCon_t *pEvCon )
{
	int	Ret;

	Ret = DlgCacheUnlockBy( pD, FALSE, pEvCon ); 
	return( Ret );
}

MemItem_t*
MemItemCacheLockW( FdEvCon_t *pEvCon, char *pKey, bool_t bCreate, 
					void *pTag, void *pArg )
{
	char		Key[FILE_NAME_MAX];
	DlgCache_t	*pD;
	DlgCache_t	*pDS;
	MemItem_t	*pMI;

	pDS = MemSpaceLockR( pEvCon );

	unsigned char StrMD5[MD5_DIGEST_LENGTH*2+1];

	memset( StrMD5, 0, sizeof(StrMD5) );
	MD5HashStr( (unsigned char*)pKey, (long)strlen(pKey), StrMD5 );

	snprintf(Key,sizeof(Key), PAXOS_MEMCACHE_KEY_FMT, 
							MemCacheCtrl.mc_NameSpace, StrMD5 );

	pD = DlgCacheLockWBy( &MemCacheCtrl.mc_MemItem, Key, bCreate, 
							NULL, NULL, pEvCon ); 

	pD->d_pVoid	= pDS;

	pMI = container_of( pD, MemItem_t, i_Dlg );
	strcpy( pMI->i_Key, pKey );

LOG( pLog, LogDBG, "[%s] pEvCon=%p pMI=%p", Key, pEvCon, pMI );
	return( pMI );
}

MemItem_t*
MemItemCacheLockR( FdEvCon_t *pEvCon, char *pKey, bool_t bCreate, 
					void *pTag, void *pArg )
{
	char		Key[FILE_NAME_MAX];
	DlgCache_t	*pD;
	DlgCache_t	*pDS;
	MemItem_t	*pMI;

	pDS = MemSpaceLockR( pEvCon );

	unsigned char StrMD5[MD5_DIGEST_LENGTH*2+1];

	memset( StrMD5, 0, sizeof(StrMD5) );
	MD5HashStr( (unsigned char*)pKey, (long)strlen(pKey), StrMD5 );

	snprintf(Key,sizeof(Key), PAXOS_MEMCACHE_KEY_FMT, 
							MemCacheCtrl.mc_NameSpace, StrMD5 );

	pD = DlgCacheLockRBy( &MemCacheCtrl.mc_MemItem, Key, bCreate, 
							NULL, NULL, pEvCon ); 

	pD->d_pVoid	= pDS;

	pMI = container_of( pD, MemItem_t, i_Dlg );
	strcpy( pMI->i_Key, pKey );

LOG( pLog, LogDBG, "[%s] pEvCon=%p pMI=%p", Key, pEvCon, pMI );
	return( pMI );
}

int
MemItemCacheUnlock( FdEvCon_t *pEvCon, MemItem_t *pMI, bool_t bFree )
{
	int	Ret;
	DlgCache_t	*pDS;
	DlgCache_t	*pD;

	pD = &pMI->i_Dlg;

	pDS	= pD->d_pVoid;

	Ret = DlgCacheUnlockBy( pD, bFree, pEvCon ); 

	MemSpaceUnlock( pDS, pEvCon );

//LOG( pLog, LogDBG, "pEvCon=%p pMI=%p", pEvCon, pMI );
	return( Ret );
}

void FdEvConFree( FdEvCon_t *pFdEvCon );
char* StrAddr( struct sockaddr *pAddr );

Msg_t* RecvBinaryByFd( int Fd );

GElm_t*
_AllocMemCachedCon( GElmCtrl_t *pGC )
{
	MemCachedCon_t	*pCon;

	pCon	= (MemCachedCon_t*)Malloc( sizeof(*pCon) );
	memset( pCon, 0, sizeof(*pCon) );
	GElmInit( &pCon->m_Elm );

	pCon->m_Fd 		= -1;
	pCon->m_FdBin	= -1;

	MtxInit( &pCon->m_Mtx );
	CndInit( &pCon->m_Cnd );

	pCon->m_Flags	= 0;

	return( &pCon->m_Elm );
}

int
_FreeMemCachedCon( GElmCtrl_t *pGC, GElm_t *pE )
{
	MemCachedCon_t	*pCon = container_of( pE, MemCachedCon_t, m_Elm );;

	Free( pCon );
	return( 0 );
}

int
_SetMemCachedCon( GElmCtrl_t *pGC, GElm_t *pE, void* pKey, void** ppKey,
					void *pArg, bool_t bLoad )
{
	struct sockaddr *pAddr = (struct sockaddr*)pKey;
	int				Fd;
	struct linger	Linger = { 1/*On*/, 10/*sec*/};
	int flags = 1;
	MemCachedCon_t	*pCon = container_of( pE, MemCachedCon_t, m_Elm );

	memcpy( &pCon->m_Addr, pKey, sizeof(pCon->m_Addr) );

	ASSERT( pCon->m_Fd <= 0 );

	if( pCon->m_Fd < 0 ) {
		Fd = socket( AF_INET, SOCK_STREAM, 0 );
//LOG( pLog, LogDBG, "[%s]", StrAddr( pAddr ) );
		if( connect( Fd, pAddr, sizeof(*pAddr ) ) < 0 ) {
			close( Fd );

			goto err;
		}
		pCon->m_Fd	= Fd;
		setsockopt( Fd, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger) );
		setsockopt( Fd, IPPROTO_TCP, TCP_NODELAY, 
							(void *)&flags, sizeof(flags));
	}
	if( pCon->m_FdBin < 0 ) {
		Fd = socket( AF_INET, SOCK_STREAM, 0 );
//LOG( pLog, LogDBG, "[%s]", StrAddr( pAddr ) );
		if( connect( Fd, pAddr, sizeof(*pAddr ) ) < 0 ) {
			close( Fd );

			goto err;
		}
		pCon->m_FdBin	= Fd;
		setsockopt( Fd, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger) );
		setsockopt( Fd, IPPROTO_TCP, TCP_NODELAY, 
							(void *)&flags, sizeof(flags));
	}
	ASSERT( pCon && pCon->m_Fd >= 0 && pCon->m_FdBin >= 0 );

	*ppKey	= &pCon->m_Addr;
	return( 0 );
err:
	if( pCon->m_Fd >= 0 )		close( pCon->m_Fd );
	if( pCon->m_FdBin >= 0 )	close( pCon->m_FdBin );
	return( -1 );
}

int
_UnsetMemCachedCon( GElmCtrl_t *pGC, GElm_t *pE, void** ppKey, bool_t bSave)
{
	MemCachedCon_t	*pCon = container_of( pE, MemCachedCon_t, m_Elm );

	if( pCon->m_Fd > 0 )	close( pCon->m_Fd );
	pCon->m_Fd	= -1;

	if( pCon->m_FdBin > 0 )	close( pCon->m_FdBin );
	pCon->m_FdBin	= -1;

	*ppKey	= &pCon->m_Addr;
//LOG( pLog, LogDBG, "[%s]", StrAddr( (struct sockaddr*)&pCon->m_Addr ) );
	return( 0 );
}

int
_AddrCode( void *pArg )
{
	int	code;
	struct sockaddr_in *pAddr = pArg;

	code	= pAddr->sin_addr.s_addr + pAddr->sin_port;

	return( code );
}

int
_AddrCmp( void *pA, void *pB )
{
	return( memcmp( pA, pB, sizeof(struct sockaddr_in)) );
}

MemCachedCon_t*
GetMemCached( struct sockaddr *pAddr )
{
	MemCachedCon_t	*pCon;
	GElm_t			*pE;

	pE = GElmGet( &MemCacheCtrl.mc_MemCacheds, pAddr, NULL, TRUE, FALSE );
	if( !pE )	goto err;

	pCon	= container_of( pE, MemCachedCon_t, m_Elm );

	return( pCon );
err:
	return( NULL );
}

int
PutMemCached( MemCachedCon_t *pCon, bool_t bFree )
{
	GElmPut( &MemCacheCtrl.mc_MemCacheds, &pCon->m_Elm, bFree, FALSE );

	return( 0 );
}

static struct timeval	Timeout = { 3, 0};
int
_ReqRplMemCachedText( MemCachedCon_t *pCon, Msg_t *pReq, MemItem_t *pMI,
						Msg_t **ppRpl )
{
/* Always sync */
	CmdToken_t	*pTok;
	char	Buf[TOKEN_BUF_SIZE];
	int		Len;
	int	Ret;
	Msg_t	*pRpl;

	pTok	= (CmdToken_t*)pReq->m_pTag2;
	if( MsgGetCmd( pReq ) == META_CMD_CAS ) {
		sprintf( Buf, "%s %s %s %s %s %"PRIu64"\r\n", 
				pTok->c_pCmd, pTok->c_pKey, pTok->c_pFlags, pTok->c_pExptime, 
				pTok->c_pBytes, pMI->i_CAS );
LOG( pLog, LogDBG, "[%s]", Buf );
		Len = strlen( Buf );

		Ret = SendStream( pCon->m_Fd, Buf, Len );
		if( Ret < 0 )	goto err;

		pReq->m_aVec[0].v_Flags |= VEC_DISABLED;	// set Cmd disabled
	}
	else if( MsgIsNoreply( pReq ) ) {

		Len	= pReq->m_aVec[0].v_Len - 10;
		memcpy( Buf, pReq->m_aVec[0].v_pStart, Len );
		sprintf( Buf + Len, "\r\n");
		Len	+= 2;

LOG( pLog, LogDBG, "Buf[%s]", Buf );
		Ret = SendStream( pCon->m_Fd, Buf, Len );
		if( Ret < 0 )	goto err;

		pReq->m_aVec[0].v_Flags |= VEC_DISABLED;	// set Cmd disabled
	}
LOG( pLog, LogDBG, "Cmd[%s]", pTok->c_pCmd );
	Ret = SendMsgByFd( pCon->m_Fd, pReq );
	if( Ret < 0 )	goto err;

	Ret = FdEventWait( pCon->m_Fd, &Timeout );
	if( Ret < 0 )	goto err;

	pRpl 	= RecvServerTextByFd( pCon->m_Fd );
LOG( pLog, LogDBG, "[%s]", pRpl->m_aVec[0].v_pStart );
	if( !pRpl )	goto err;

	*ppRpl	= pRpl;
	return( 0 );
err:
	*ppRpl	= NULL;
	return( -1 );
}

int
_ReqRplMemCachedBinary( MemCachedCon_t *pCon, Msg_t *pReq, 
							MemItem_t *pMI, Msg_t **ppRpl )
{
/* Always sync */
	Msg_t	*pRpl;
	BinHeadTag_t	*pHead, *pHeadServer;
	uint8_t			Opcode;
	int	Ret;

	pHead	= (BinHeadTag_t*)pReq->m_pTag2;

	Opcode	= pHead->t_Head.h_Opcode;
	switch( Opcode ) {
	case BIN_CMD_SETQ:		pHead->t_Head.h_Opcode = BIN_CMD_SET;	break;
	case BIN_CMD_ADDQ:		pHead->t_Head.h_Opcode = BIN_CMD_ADD;	break;
	case BIN_CMD_REPLACEQ:	pHead->t_Head.h_Opcode = BIN_CMD_REPLACE;	break;
	case BIN_CMD_DELETEQ:	pHead->t_Head.h_Opcode = BIN_CMD_DELETE;	break;
	case BIN_CMD_INCRQ:		pHead->t_Head.h_Opcode = BIN_CMD_INCR;		break;
	case BIN_CMD_DECRQ:		pHead->t_Head.h_Opcode = BIN_CMD_DECR;		break;
	case BIN_CMD_QUITQ:		pHead->t_Head.h_Opcode = BIN_CMD_QUIT;		break;
	case BIN_CMD_FLUSHQ:	pHead->t_Head.h_Opcode = BIN_CMD_FLUSH;		break;
	case BIN_CMD_APPENDQ:	pHead->t_Head.h_Opcode = BIN_CMD_APPEND;	break;
	case BIN_CMD_PREPENDQ:	pHead->t_Head.h_Opcode = BIN_CMD_PREPEND;	break;
	case BIN_CMD_GATQ:		pHead->t_Head.h_Opcode = BIN_CMD_GAT;	break;
	default:	break;
	}
	if( pMI && GetMetaCAS(pReq) ) {

//LOG( pLog, LogDBG, 
//"Op(Old=0x%x New=0x%x) CAS=%"PRIu64"->i_CAS=%"PRIu64" bITEM_CAS=%d", 
//Opcode, pHead->t_Head.h_Opcode, be64toh( pHead->t_Head.h_CAS ), 
//pMI->i_CAS, pMI->i_Status & MEM_ITEM_CAS );

		pHead->t_Head.h_CAS	= htobe64( pMI->i_CAS );	
	}

	Ret = SendBinaryByFd( pCon->m_FdBin, pReq );
	if( Ret < 0 ) {
//LOG(pLog,LogDBG,"SendBinaryByFd:Ret=%d", Ret );
		goto err;
	}

	Ret = FdEventWait( pCon->m_FdBin, &Timeout );
	if( Ret < 0 ) {
//LOG(pLog,LogDBG,"FdEventWait:Ret=%d", Ret );
		goto err;
	}

	pRpl 	= RecvBinaryByFd( pCon->m_FdBin );
	if( !pRpl ) {
//LOG(pLog,LogDBG,"RecvBinaryByFd:Ret=%d", Ret );
		goto err;
	}

	if( pMI && pMI->i_pAttrs ) {
		pHeadServer	= (BinHeadTag_t*)pRpl->m_pTag2;
		pMI->i_CAS	= be64toh( pHeadServer->t_Head.h_CAS );
	}
	pHead->t_Head.h_Opcode = Opcode;

	*ppRpl	= pRpl;

//LOG( pLog, LogDBG, "pRpl=%p", pRpl ); 
ASSERT( list_empty(&pRpl->m_Lnk) );
	return( 0 );
err:
	pHead->t_Head.h_Opcode = Opcode;
	*ppRpl	= NULL;
//LOG( pLog, LogDBG, "ERROR Op=%d errno=%d", Opcode, errno ); 
	return( -1 );
}

int
_ReqRplMemCached( MemCachedCon_t *pCon, Msg_t *pReq, MemItem_t *pMI, 
						Msg_t **ppRpl )
{
	Msg_t *pRpl;
	int	Ret;

	*ppRpl	= NULL;

	if (pCon->m_Flags & MEMCACHEDCON_ABORT) {
		goto err;
	}

	if( MsgIsText( pReq ) ) {
		Ret	= _ReqRplMemCachedText( pCon, pReq, pMI, &pRpl );
	} else if( MsgIsBinary( pReq ) ) {
		Ret	= _ReqRplMemCachedBinary( pCon, pReq, pMI, &pRpl );
	} else ABORT();

	if( Ret < 0 ) {	// server side is closed
		goto err;
	}

	*ppRpl	= pRpl;

	return( 0 );
err:
	pCon->m_Flags |= MEMCACHEDCON_ABORT;
	return( -1 );
}

int
ReqRplMemCached( MemCachedCon_t *pCon, Msg_t *pReq, MemItem_t *pMI, 
						Msg_t **ppRpl )
{
	int	Ret;

	MtxLock( &pCon->m_Mtx );

	Ret = _ReqRplMemCached( pCon, pReq, pMI, ppRpl );

	MtxUnlock( &pCon->m_Mtx );

	return( Ret );
}

int
ReqRplMemcachedByKey( MemCachedCon_t *pCon, char *pKey, Msg_t **ppRpl )
{
	BinHead_t	Head;
	int			KeyLen;
	int			Ret;
	Msg_t		*pRpl;

	KeyLen = strlen(pKey);

	Head.h_Magic		= BIN_MAGIC_REQ;
	Head.h_Opcode		= BIN_CMD_GET;
	Head.h_KeyLen		= htons( KeyLen );
	Head.h_ExtLen		= 0;
	Head.h_DataType		= BIN_RAW_BYTES; 
	Head.h_VbucketId	= 0; 
	Head.h_BodyLen		= htonl( KeyLen ) ; 
	Head.h_Opaque		= 0; 
	Head.h_CAS			= 0;
	
	MtxLock( &pCon->m_Mtx );

	if (pCon->m_Flags & MEMCACHEDCON_ABORT) {
		goto err;
	}

	// head
	Ret = SendStream( pCon->m_FdBin, (char*)&Head, sizeof(Head) );
	if( Ret < 0 )	goto err;

	// key
	Ret = SendStream( pCon->m_FdBin, pKey, KeyLen );
	if( Ret < 0 )	goto err;

	Ret = FdEventWait( pCon->m_FdBin, &Timeout );
	if( Ret < 0 )	goto err;

	pRpl 	= RecvBinaryByFd( pCon->m_FdBin );
	if( !pRpl )	goto err;

	*ppRpl	= pRpl;

	MtxUnlock( &pCon->m_Mtx );

	return( 0 );
err:
	*ppRpl	= NULL;
	pCon->m_Flags |= MEMCACHEDCON_ABORT;
	MtxUnlock( &pCon->m_Mtx );
	return( -1 );
}

int
ReqRplMemcachedBySettings( MemCachedCon_t *pCon )
{
	BinHead_t	Head;
	int			KeyLen;
	int			Ret;
	Msg_t		*pRpl;
	char	*pKey = "settings";
	BinHeadTag_t	*pHeadTag;

	MtxLock( &pCon->m_Mtx );

	if( MemCacheCtrl.mc_item_size_max )	goto OK;

	KeyLen	= strlen(pKey);

	Head.h_Magic		= BIN_MAGIC_REQ;
	Head.h_Opcode		= BIN_CMD_STAT;
	Head.h_KeyLen		= htons( KeyLen );
	Head.h_ExtLen		= 0;
	Head.h_DataType		= BIN_RAW_BYTES; 
	Head.h_VbucketId	= 0; 
	Head.h_BodyLen		= htonl( KeyLen ) ; 
	Head.h_Opaque		= 0; 
	Head.h_CAS			= 0;
	

	if (pCon->m_Flags & MEMCACHEDCON_ABORT) {
		goto err;
	}

	// head
	Ret = SendStream( pCon->m_FdBin, (char*)&Head, sizeof(Head) );
	if( Ret < 0 )	goto err;

	// key
	Ret = SendStream( pCon->m_FdBin, pKey, KeyLen );
	if( Ret < 0 )	goto err;

	Ret = FdEventWait( pCon->m_FdBin, &Timeout );
	if( Ret < 0 )	goto err;

	while( 1 ) {
		pRpl 	= RecvBinaryByFd( pCon->m_FdBin );
		if( !pRpl )	goto err;

		pHeadTag	= (BinHeadTag_t*)pRpl->m_pTag2;

		if( !pHeadTag->t_pKey  ) {
			DestroyMsg( pRpl );
			break;
		} else if( !strcmp("item_size_max", pHeadTag->t_pKey ) ) {
			MemCacheCtrl.mc_item_size_max = (size_t)atol( pHeadTag->t_pValue );
		}
LOG(pLog, LogDBG, "key[%s] value[%s]", pHeadTag->t_pKey, pHeadTag->t_pValue );
		DestroyMsg( pRpl );
	}
	MtxUnlock( &pCon->m_Mtx );

OK:
	return( 0 );
err:
	pCon->m_Flags |= MEMCACHEDCON_ABORT;
	MtxUnlock( &pCon->m_Mtx );
	return( -1 );
}

bool_t
IsInMemCacheds( struct sockaddr *pAddr, MemItemAttrs_t *pAttrs )
{
	bool_t	Flag = FALSE;
	int		i;

	for( i = 0; i < pAttrs->a_N; i++ ) {
		if( !memcmp(pAddr,&pAttrs->a_aAttr[i].a_MemCached,sizeof(*pAddr)) ) {
			Flag = TRUE;
			break;
		}
	}
	return( Flag );	
}

/*	Session */
struct Session*
MemSessionOpen( char *pCellName, int SessionNo, bool_t IsSync )
{
	struct Session *pSession = NULL; 
	int	Ret;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, Malloc, Free, pCellName );
	if( !pSession )	goto err;

	if( pLog )	PaxosSessionSetLog( pSession, pLog );

	Ret	= PaxosSessionOpen( pSession, SessionNo, IsSync );
	if( Ret != 0 ) {
		errno = EACCES;
		goto err1;
	}
	return( pSession );
err1:
	PaxosSessionFree( pSession );
err:
	return( NULL );
}

void
MemSessionClose( void * pSession )
{
	PaxosSessionClose( (struct Session*)pSession );
	PaxosSessionFree( pSession );
}

void
MemSessionAbort( void * pSession )
{
	PaxosSessionAbort( (struct Session*)pSession );
	PaxosSessionFree( pSession );
}

int
ReadCrLf( int fd, char *pBuf, int Size  )
{
	int		n, l;
	char	*cp;
	bool_t	bFound = FALSE;

again:
	l = recv( fd, pBuf, Size, MSG_PEEK );
	if( l <= 0 ) goto err;	

	cp	= pBuf;
	n	= 0;
	while( l > 0 ) {
		n++; l--;
		if( *cp++ == '\n' ) {
			bFound = TRUE;
			break; 
		}
	}
	if( !bFound )	goto again;

	RecvStream( fd, pBuf, n );

	return( n );
err:
	return( -1 );
}

int
SendMsgByFd( int Fd, Msg_t *pMsg )
{
	int	i;
	MsgVec_t	*pVec;
	int	Ret;

LOG(pLog, LogDBG, "Fd=%d", Fd );
/* through */
	for( i = 0; i < pMsg->m_N; i++ ) {
		pVec	= &pMsg->m_aVec[i];
		if( pVec->v_Flags & VEC_DISABLED )	continue;

//LOG(pLog, LogDBG, "Fd=%d [%s] v_Len=%d", Fd, pVec->v_pStart, pVec->v_Len );
		Ret = SendStream( Fd, pVec->v_pStart, pVec->v_Len );
		if( Ret < 0 )	goto err;
	}
	return( 0 );
err:
	return( -1 );
}

void
DestroyMsg( Msg_t *pMsg )
{
	if( pMsg->m_pTag2 )	{
		Free( pMsg->m_pTag2 );
		pMsg->m_pTag2 = NULL;
	}
	MsgDestroy( pMsg );
}

CmdToken_t*
CloneToken( CmdToken_t *pTok )
{
	CmdToken_t *pCloneTok;
	int64_t		diff;

	pCloneTok = (CmdToken_t*)Malloc( sizeof(*pCloneTok) );
	if( !pCloneTok ) goto ret;

	memcpy( pCloneTok, pTok, sizeof(*pCloneTok) );
	diff	= (uintptr_t)pCloneTok - (uintptr_t)pTok;
	pCloneTok->c_pCmd		= (char*)pTok->c_pCmd + diff;
	pCloneTok->c_pKey		= (char*)pTok->c_pKey + diff;
	pCloneTok->c_pFlags		= (char*)pTok->c_pFlags + diff;
	pCloneTok->c_pExptime	= (char*)pTok->c_pExptime + diff;
	pCloneTok->c_pBytes		= (char*)pTok->c_pBytes + diff;
	pCloneTok->c_pNoreply	= (char*)pTok->c_pNoreply + diff;
	pCloneTok->c_pCAS		= (char*)pTok->c_pCAS + diff;
	pCloneTok->c_pValue		= (char*)pTok->c_pValue + diff;
	pCloneTok->c_pSave		= (char*)pTok->c_pSave + diff;
ret:
	return( pCloneTok );
}

BinHeadTag_t*
CloneHead( BinHeadTag_t *pHead )
{
	BinHeadTag_t	*pCloneHead;
	int	ExtLen = 0, KeyLen = 0, Len;
	int64_t		diff;

	ExtLen	= pHead->t_Head.h_ExtLen;
	KeyLen	= ntohs( pHead->t_Head.h_KeyLen );

	Len	= sizeof(BinHeadTag_t) + ExtLen + KeyLen;
	pCloneHead	= (BinHeadTag_t*)Malloc( Len + 1 ); 
	if( !pCloneHead )	goto ret;

	memcpy( pCloneHead, pHead, Len+1 );
	diff	= (uintptr_t)pCloneHead - (uintptr_t)pHead;
	pCloneHead->t_pExt		= (char*)pHead->t_pExt + diff;
	pCloneHead->t_pKey		= (char*)pHead->t_pKey + diff;
ret:
	return( pCloneHead );
}

Msg_t*
CloneMsg( Msg_t *pMsg )
{
	Msg_t	*pCloneMsg;

	pCloneMsg	= MsgClone( pMsg );
	if( pMsg->m_pTag2 ) {
		if( MsgIsText( pMsg ) ) {
			pCloneMsg->m_pTag2	= CloneToken( pMsg->m_pTag2 );
		} else if( MsgIsBinary( pMsg ) ) {
			pCloneMsg->m_pTag2	= CloneHead( pMsg->m_pTag2 );
		} else {
			ABORT();
		}
	}
	return( pCloneMsg );
}


int
ClearFlag( FdEvCon_t *pEvCon, int Flag )
{
	MtxLock( &pEvCon->c_Mtx );

	pEvCon->c_Flags	&= ~Flag;

	MtxUnlock( &pEvCon->c_Mtx );
	return( 0 );
}

int
WaitFlag( FdEvCon_t *pEvCon, int Flag )
{
	MtxLock( &pEvCon->c_Mtx );

	while( !(pEvCon->c_Flags & Flag ) ) {
		CndWait( &pEvCon->c_Cnd, &pEvCon->c_Mtx );
		if( pEvCon->c_Flags & FDEVCON_ABORT )	goto err;
	}
	pEvCon->c_Flags	&= ~Flag;

	MtxUnlock( &pEvCon->c_Mtx );
	return( 0 );
err:
	MtxUnlock( &pEvCon->c_Mtx );
	return( -1 );
}

int
WakeupFlag( FdEvCon_t *pEvCon, int Flag, void *pVoid )
{
	MtxLock( &pEvCon->c_Mtx );

	pEvCon->c_pVoid	= pVoid;
	pEvCon->c_Flags	|= Flag;
	CndBroadcast( &pEvCon->c_Cnd );

	MtxUnlock( &pEvCon->c_Mtx );
	return( 0 );
}

#define	WakeupAbort( pEvCon )		WakeupFlag( pEvCon, FDEVCON_ABORT, NULL )

int
DeleteAttrFile( char *pDlgKey )
{
	int		Ret;

	Ret = PFSDelete( MemCacheCtrl.mc_pSession, pDlgKey );
	return( Ret );
}

MemItemAttrs_t*
ReadAttr( char *pDlgKey )
{
	struct File *pFile;
	int	Ret;
	MemItemAttrs_t	*pAttrs, *pAttrsNew;
	int	Len;


	pFile = PFSOpen( MemCacheCtrl.mc_pSession, pDlgKey, 0 );
	if( !pFile )	goto err;

	pAttrs	= (MemItemAttrs_t*)Malloc( sizeof(*pAttrs) );

	Ret = PFSRead( pFile, (char*)pAttrs, sizeof(*pAttrs) );
LOG(pLog, LogDBG, "###[%s] Ret=%d ###", pDlgKey, Ret );
	if( Ret < 0 )	goto err1;

	if( pAttrs->a_N > 1 ) {

		Len	= (pAttrs->a_N - 1 )*sizeof(MemItemAttr_t);
		pAttrsNew	= (MemItemAttrs_t*)Malloc( sizeof(*pAttrsNew) + Len );

		*pAttrsNew	= *pAttrs;

		Ret = PFSRead( pFile, (char*)(pAttrsNew+1), Len );
		if( Ret < 0 )	goto err2;

		Free( pAttrs );
		pAttrs = pAttrsNew;
	}

	PFSClose( pFile );
	return( pAttrs );
err2:
	Free( pAttrsNew );
err1:
	Free( pAttrs );
	PFSClose( pFile );
err:
	return( NULL );
}

int
WriteAttr( MemItemAttrs_t *pAttrs, char *pDlgKey )
{
	struct File *pFile;
	int	Ret;
	int	Len;


	pFile = PFSOpen( MemCacheCtrl.mc_pSession, pDlgKey, 0 );
	if( !pFile )	goto err;

//{
//	char Str[1024];
//	StrAddr( Str, &pAttrs->a_aAttr[0].a_MemCached);
//	LOG( pLog, LogDBG, "[%s]", Str );
//}
ASSERT( pAttrs->a_N >= 0 );
	Len	= sizeof(*pAttrs) + (pAttrs->a_N-1)*sizeof(MemItemAttr_t);

	Ret = PFSWrite( pFile, (char*)pAttrs, Len );
	if( Ret < 0 )	goto err1;

	PFSClose( pFile );
	return( 0 );
err1:
	PFSClose( pFile );
err:
	return( -1 );
}

int
FindAttr( MemItemAttrs_t *pAttrs, struct sockaddr *pAddrMemcached )
{
	int	i;

	for( i = 0; i < pAttrs->a_N; i++ ) {
		if( !memcmp( pAddrMemcached, &pAttrs->a_aAttr[i].a_MemCached, 
				sizeof(struct sockaddr)) )	return( i );
	}
	return( -1 );
}

int
CancelAttr( MemItemAttrs_t *pAttrs, struct sockaddr *pAddrMemCached )
{
	int	i;

	i = FindAttr( pAttrs, pAddrMemCached );
	if( i < 0 ) return( -1 );

	if( pAttrs->a_aAttr[i].a_pData) {
		DestroyMsg( pAttrs->a_aAttr[i].a_pData );
	}
	if( pAttrs->a_aAttr[i].a_pDiff) {
		DestroyMsg( pAttrs->a_aAttr[i].a_pDiff );
	}
	for( ++i; i < pAttrs->a_N; i++ ) {
		pAttrs->a_aAttr[i-1]	= pAttrs->a_aAttr[i];
	}
	pAttrs->a_N--;
	return( 0 );
}

bool_t
IsSameMsg( Msg_t *pMsgA, Msg_t *pMsgB )
{
	CmdToken_t	*pTokA, *pTokB;
	BinHeadTag_t	*pHeadA, *pHeadB;
	char	*pDataA, *pDataB;
	int		lA, lB;
	bool_t	bSame;

	if( MsgIsText( pMsgA ) ) {
		if( !MsgIsText( pMsgB ) )	return( FALSE );

		pTokA	= (CmdToken_t*)pMsgA->m_pTag2;
		pTokB	= (CmdToken_t*)pMsgB->m_pTag2;

		if( strcmp( pTokA->c_pCmd, pTokB->c_pCmd ) )	return( FALSE );
		
	} else if( MsgIsBinary( pMsgA ) ) {
		if( !MsgIsBinary( pMsgB ) )	return( FALSE );

		pHeadA	= (BinHeadTag_t*)pMsgA->m_pTag2;
		pHeadB	= (BinHeadTag_t*)pMsgB->m_pTag2;

		if( pHeadA->t_Head.h_Opcode 
			!= pHeadB->t_Head.h_Opcode )	return( FALSE );
	} else {
		return( FALSE );
	}
	pDataA	= pMsgA->m_aVec[1].v_pStart;
	lA		= pMsgA->m_aVec[1].v_Len;

	pDataB	= pMsgB->m_aVec[1].v_pStart;
	lB		= pMsgB->m_aVec[1].v_Len;

	bSame	= ( pMsgA->m_N == pMsgB->m_N
				&& lA == lB 
				&& !memcmp( pDataA, pDataB, lA ) );

	return( bSame );
}

int
MsgSetCmdStore( Msg_t *pMsg )
{
	CmdToken_t		*pTok;
	BinHeadTag_t	*pHead;
	uint8_t			Opcode;
	MetaCmd_t	MetaCmd = META_CMD_OTHERS;

	if( MsgIsText( pMsg ) ) {
		pTok	= (CmdToken_t*)pMsg->m_pTag2;
		if( !strcmp( pTok->c_pCmd, "set" ) )		MetaCmd = META_CMD_SET;
		else if( !strcmp( pTok->c_pCmd, "add"))		MetaCmd = META_CMD_ADD;
		else if( !strcmp( pTok->c_pCmd, "replace"))	MetaCmd = META_CMD_REPLACE;
		else if( !strcmp( pTok->c_pCmd, "cas")) {
			MetaCmd = META_CMD_CAS;	
			MsgSetCAS( pMsg );
		}
		else if( !strcmp( pTok->c_pCmd, "append"))	MetaCmd = META_CMD_APPEND;	
		else if( !strcmp( pTok->c_pCmd, "prepend"))	MetaCmd = META_CMD_PREPEND;	
	} else if( MsgIsBinary( pMsg ) ) {
		pHead	= (BinHeadTag_t*)pMsg->m_pTag2;
		Opcode	= pHead->t_Head.h_Opcode;
		switch( Opcode ) {
			case BIN_CMD_SETQ:
			case BIN_CMD_SET:
				MetaCmd	= META_CMD_SET;
				break;
			case BIN_CMD_ADDQ:
			case BIN_CMD_ADD:
				MetaCmd	= META_CMD_ADD;
				break;
			case BIN_CMD_REPLACEQ: 
			case BIN_CMD_REPLACE:
				MetaCmd	= META_CMD_REPLACE;
				break;
			case BIN_CMD_APPENDQ: 
			case BIN_CMD_APPEND:
				MetaCmd	= META_CMD_APPEND;
				break;
			case BIN_CMD_PREPENDQ: 
			case BIN_CMD_PREPEND:
				MetaCmd	= META_CMD_PREPEND;
				break;
			default:;
		}
		if( pHead->t_Head.h_CAS )	MsgSetCAS( pMsg );
	} else {
		ABORT();
	}
	MsgSetCmd( pMsg, MetaCmd );
	return( 0 );
}

bool_t
IsDiffMsg( Msg_t *pMsg )
{
	MetaCmd_t	MetaCmd;

	MetaCmd	= MsgGetCmd( pMsg );

	switch( MetaCmd ) {
		case META_CMD_SET:
		case META_CMD_ADD:
		case META_CMD_REPLACE:
		case META_CMD_CAS:
			return( FALSE );
		default:;
	}
	return( TRUE );
}

uint64_t
GetMetaEpoch( Msg_t *pMsg )
{
	CmdToken_t		*pTok;
	BinHeadTag_t	*pHead;
	uint64_t		Epoch;

	if( MsgIsText( pMsg ) ) {
		pTok	= (CmdToken_t*)pMsg->m_pTag2;
		Epoch		= pTok->c_Epoch;
	} else if( MsgIsBinary( pMsg ) ) {
		pHead	= (BinHeadTag_t*)pMsg->m_pTag2;
		Epoch		= pHead->t_Epoch;
	}
	else	ABORT();

	return( Epoch );
}

uint64_t
SetMetaEpoch( Msg_t *pMsg, uint64_t Epoch )
{
	CmdToken_t		*pTok;
	BinHeadTag_t	*pHead;

	if( MsgIsText( pMsg ) ) {
		pTok	= (CmdToken_t*)pMsg->m_pTag2;
		pTok->c_Epoch = Epoch;
	} else if( MsgIsBinary( pMsg ) ) {
		pHead	= (BinHeadTag_t*)pMsg->m_pTag2;
		pHead->t_Epoch = Epoch;
	}
	else	ABORT();

	return( Epoch );
}

uint64_t
GetMetaCAS( Msg_t *pMsg )
{
	CmdToken_t		*pTok;
	BinHeadTag_t	*pHead;
	uint64_t		CAS;

	if( MsgIsText( pMsg ) ) {
		pTok	= (CmdToken_t*)pMsg->m_pTag2;
		CAS		= pTok->c_MetaCAS;
	} else if( MsgIsBinary( pMsg ) ) {
		pHead	= (BinHeadTag_t*)pMsg->m_pTag2;
		CAS		= pHead->t_MetaCAS;
	}
	else	ABORT();

	return( CAS );
}

uint64_t	SeqMetaCAS;

int
SeqMetaCASInit( DlgCache_t *pD, void *pArg )
{
	struct File *pFile;
	int	Ret;

	pFile = PFSOpen( MemCacheCtrl.mc_pSession, pD->d_Dlg.d_Name, 0 );
	if( !pFile )	goto err;

	Ret = PFSRead( pFile, (char*)&SeqMetaCAS, sizeof(SeqMetaCAS) );
	if( Ret < 0 )	goto err1;

	PFSClose( pFile );

	return( 0 );
err1:
	PFSClose( pFile );
err:
	return( 0 );
}

int
SeqMetaCASTerm( DlgCache_t *pD, void *pArg )
{
	struct File *pFile;
	int	Ret;

	pFile = PFSOpen( MemCacheCtrl.mc_pSession, pD->d_Dlg.d_Name, 0 );
	if( !pFile )	goto err;

	Ret = PFSWrite( pFile, (char*)&SeqMetaCAS, sizeof(SeqMetaCAS) );
	if( Ret < 0 )	goto err1;

	PFSClose( pFile );

	return( 0 );
err1:
	PFSClose( pFile );
err:
	return( 0 );
}

uint64_t
GetSeqMetaCAS(void)
{
	uint64_t	MetaCAS;
	char		SeqKey[PATH_NAME_MAX];
	DlgCache_t	*pD;

	snprintf( SeqKey, sizeof(SeqKey),
				"%s/SeqMetaCAS", MemCacheCtrl.mc_NameSpace );

	pD = DlgCacheLockW( &MemCacheCtrl.mc_SeqMetaCAS, SeqKey, TRUE, NULL, NULL );

	CAS_INC( SeqMetaCAS );
	MetaCAS	= SeqMetaCAS;

	DlgCacheUnlock( pD, FALSE );

	return( MetaCAS );
}

RetCode_t
MetaCacheCheck( FdEvCon_t *pEvConClient, MemItem_t *pMI, 
		struct sockaddr *pAddrMemCached, Msg_t *pMsg )
{
	MemItemAttrs_t	*pAttrs, *pAttrsNew;
	MemItemAttr_t	*pAttr, Attr;
	int	i, l;
	uint64_t	CAS, MsgCAS;
	Msg_t	*pDiff = NULL, *pBody;
	Msg_t	*pMsgServer;
	int		Ret;
	MetaCmd_t	MetaCmd;
	bool_t	bExpired = FALSE;

	MsgSetCmdStore( pMsg );		// convert to MetaCmd
	MetaCmd = MsgGetCmd( pMsg );
	MsgCAS	= GetMetaCAS( pMsg );

	pAttrs	= pMI->i_pAttrs;

	if( !pAttrs ) {
	/* 全体で初めての登録 */
		if( MetaCmd == META_CMD_REPLACE 
				|| MetaCmd == META_CMD_CAS 
				|| MsgCAS ) {

			//LOG( pLog, LogDBG, "Not exist." );
			MsgSetRC( pMsg, RET_NOENT );
			MemCacheCtrl.mc_Statistics.s_error++;
			goto ret;
		}

		if( IsDiffMsg( pMsg ) ) {

			//LOG( pLog, LogDBG, "New but diff cmd" );

			MsgSetRC( pMsg, RET_ERROR );
			MemCacheCtrl.mc_Statistics.s_error++;
			goto ret;
		}
		pAttrs	= (MemItemAttrs_t*)Malloc( sizeof(MemItemAttrs_t) );
		pMI->i_pAttrs	= pAttrs;
LOG(pLog, LogDBG, "pMI=%p i_pAttrs=%p", pMI, pAttrs );
	
		memset( pAttrs, 0, sizeof(*pAttrs) );
		pAttrs->a_MetaCAS	= GetSeqMetaCAS();
		pAttrs->a_Epoch		= 1;	//
		pAttrs->a_aAttr[0].a_MemCached = *pAddrMemCached;
		pAttrs->a_N		= 1;

		//LOG( pLog, LogDBG, "New" );

		MsgSetRC( pMsg, RET_NEW );
		MemCacheCtrl.mc_Statistics.s_new++;
		goto ret;
	}
	// expiration check
	// expire　でもデータは残っている
	timespec_t	Now;

	TIMESPEC( Now );
LOG(pLog,LogDBG,"ExpTime=%lu Now=%lu",
pMI->i_ExpTime.tv_sec, Now.tv_sec );
	if( pMI->i_ExpTime.tv_sec && TIMESPECCOMPARE( pMI->i_ExpTime, Now ) < 0 ) {

		LOG(pLog, LogERR, "EXPIRED tv_sec=%d-%d", 
			pMI->i_ExpTime.tv_sec, Now.tv_sec );

		bExpired	= TRUE;
	}

	i = FindAttr( pMI->i_pAttrs, pAddrMemCached );
	if ( i >= 0 ) {
		/* 更新である */
		if( MetaCmd == META_CMD_ADD && !bExpired ) {

			if( MsgIsText( pMsg ) ) {
				//LOG( pLog, LogDBG, "exist->Not stored(text)." );
				MsgSetRC( pMsg, RET_NOT_STORED );
			}else {
				//LOG( pLog, LogDBG, "exist." );
				MsgSetRC( pMsg, RET_EXISTS );
			}
			MemCacheCtrl.mc_Statistics.s_error++;
			goto ret;
		}
		if( (MetaCmd == META_CMD_CAS || MsgCAS) && !bExpired ) {

			MemCacheCtrl.mc_Statistics.s_cas++;
			if( pAttrs->a_MetaCAS != MsgCAS ) {
				LOG( pLog, LogDBG, 
					"CAS(%"PRIu64") not matched->Exist(%"PRIu64").",
					MsgCAS, pAttrs->a_MetaCAS );

				MsgSetRC( pMsg, RET_EXISTS );
				MemCacheCtrl.mc_Statistics.s_error++;
				goto ret;
			}
		}
		/* カレントは0に　*/
		pDiff	= pAttrs->a_aAttr[0].a_pDiff;
		pBody	= pAttrs->a_aAttr[0].a_pData;
		if( i > 0 ) {
		// Change to 0
			//LOG( pLog, LogDBG, "%d exchanged for 0", i );

			Attr	= pAttrs->a_aAttr[0];
			pAttrs->a_aAttr[0]	= pAttrs->a_aAttr[i];
			pAttrs->a_aAttr[i]	= Attr;
		}
		pAttr	= &pAttrs->a_aAttr[0];

		/* 更新なので旧差分をパージ、失敗時にはCancelAttrで登録削除となる */
		if( pAttr->a_pDiff )	{
				if( pDiff == pAttr->a_pDiff ) {
					pDiff = NULL;
				}
				DestroyMsg( pAttr->a_pDiff );
				pAttr->a_pDiff = NULL;
		}
		if( pAttrs->a_MetaCAS == pAttr->a_MetaCAS ) {
		/* 更新である */

			/* 差分更新を記録 */
			if( IsDiffMsg( pMsg ) ) {
				pAttr->a_pDiff	= CloneMsg( pMsg );	// for check diff

				//LOG( pLog, LogDBG, "UPDATE with diff" );

				MsgSetRC( pMsg, RET_UPDATE );
				MemCacheCtrl.mc_Statistics.s_update++;

			} else {

				CAS_INC( pAttrs->a_Epoch );		// Update Epoch

				if( pAttr->a_pData && IsSameMsg( pAttr->a_pData, pMsg ) ) {

					//LOG( pLog, LogDBG, "DUP" );

					MsgSetRC( pMsg, RET_DUP );
					MemCacheCtrl.mc_Statistics.s_duprication++;
				} else {
					//LOG( pLog, LogDBG, "UPDATE" );


					MsgSetRC( pMsg, RET_UPDATE );
					MemCacheCtrl.mc_Statistics.s_update++;
				}
			}
		} else {
		/*  レプリケーションかエラー */

			CAS	= pAttrs->a_Epoch;
			CAS_DEC( CAS );
			if( CAS == pAttr->a_Epoch ) {
			/* 直前 */

				if( pDiff ) { 	// difference, append and prepend
				/* 差分のみ */

					if( IsSameMsg( pDiff, pMsg ) ) {

						pAttr->a_pDiff = CloneMsg( pMsg );

						// Update replication
						//LOG( pLog, LogDBG, "REPLICATION diff" );

						MsgSetRC( pMsg, RET_YES );
						MemCacheCtrl.mc_Statistics.s_replication++;
					} else {
						//LOG( pLog, LogDBG, "ERROR diff" );

						MsgSetRC( pMsg, RET_ERROR );
						MemCacheCtrl.mc_Statistics.s_error++;
					}
				} else {
				/* 本体 */
					if( IsSameMsg( pBody, pMsg ) ) {
						// Update replication without keep
						//LOG( pLog, LogDBG, "REPLICATION" );

						MsgSetRC( pMsg, RET_YES );
						MemCacheCtrl.mc_Statistics.s_replication++;
					} else {
						//LOG( pLog, LogDBG, "ERROR" );

						MsgSetRC( pMsg, RET_ERROR );
						MemCacheCtrl.mc_Statistics.s_error++;
					}
				}
			} else {
			/* 直前でなければエラー */
				//LOG( pLog, LogDBG, "ERROR CAS" );

				MsgSetRC( pMsg, RET_ERROR );
				MemCacheCtrl.mc_Statistics.s_error++;
			}
		}
	} else if( pAttrs->a_N > 0 ) {
	/* memcachedの追加である, replace, 差分の追加は不可 */

		if( MetaCmd == META_CMD_REPLACE || MetaCmd == META_CMD_CAS ) {

			//LOG( pLog, LogDBG, "Not exist." );

			MsgSetRC( pMsg, RET_EXISTS );
			MemCacheCtrl.mc_Statistics.s_error++;
			goto ret;
		}
		if( IsDiffMsg( pMsg ) ) {

			//LOG( pLog, LogDBG, "ERROR diff" );

			MsgSetRC( pMsg, RET_ERROR );
			MemCacheCtrl.mc_Statistics.s_error++;
			goto ret;
		}
		/* 再起動時にloadする */
		if( !pMI->i_pData ) {	// reload on restart
			Ret = ReqRplMemcachedByKey( pEvConClient->c_pCon, pMI->i_Key, 
												&pMsgServer );
			MemCacheCtrl.mc_Statistics.s_load++;
			if( Ret < 0 ) {
				//LOG( pLog, LogDBG, "ERROR GetFromMemCachedByAttr" );

				MsgSetRC( pMsg, RET_ERROR );
				MemCacheCtrl.mc_Statistics.s_error++;
				goto ret;
			}
			Ret = GetItemRegister( pEvConClient, 
									pMsg, pMsgServer, pMI );
			if( Ret < 0 ) {
				//LOG( pLog, LogDBG, "ERROR register" );

				MsgSetRC( pMsg, RET_ERROR );
				MemCacheCtrl.mc_Statistics.s_error++;
				goto ret;
			}
		}
		if( IsSameMsg( pMI->i_pData, pMsg ) ) { // check real data

			LOG( pLog, LogDBG, "Replication:pAttrs=%p", pAttrs );

			l = sizeof(*pAttrs) + pAttrs->a_N*sizeof(MemItemAttr_t);
			pAttrsNew = (MemItemAttrs_t*)Malloc( l );

			*pAttrsNew	= *pAttrs;
			memcpy( &pAttrsNew->a_aAttr[1], &pAttrs->a_aAttr[0], 
									pAttrs->a_N*sizeof(MemItemAttr_t) );
			Free(pAttrs );

			memset( &pAttrsNew->a_aAttr[0], 0, sizeof(pAttrsNew->a_aAttr[0]));
			pAttrsNew->a_aAttr[0].a_MemCached = *pAddrMemCached;

			pAttrsNew->a_N++;

			pMI->i_pAttrs	= pAttrsNew;

			MsgSetRC( pMsg, RET_YES );
		} else {
			//LOG( pLog, LogDBG, "ERROR replication" );

			MsgSetRC( pMsg, RET_ERROR );
			MemCacheCtrl.mc_Statistics.s_replication++;
		}
	} else {
		//LOG( pLog, LogDBG, "NEW" );

		MsgSetRC( pMsg, RET_NEW );
		MemCacheCtrl.mc_Statistics.s_new++;
	}
ret:
	if( RET_ERROR == MsgGetRC( pMsg ) ) {
		if( pAttrs )	CancelAttr( pAttrs, pAddrMemCached );
	}
	if( pAttrs ) {
		SetMetaEpoch( pMsg, pAttrs->a_Epoch );
	}
	if( MsgGetRC( pMsg ) >= 0 && MsgCAS ) {

		//LOG( pLog, LogDBG, "WRITE_THROUGH" );

		MemCacheCtrl.mc_Statistics.s_write_through++;
		return( RET_WRITE_THROUGH );
	} else {
		return( RET_OK );
	}
}

int
SetItemUpdate( FdEvCon_t *pEvCon, MemItem_t *pMI, 
				Msg_t *pMsgClient, Msg_t *pMsgServer )
{
	int			Ret;
	timespec_t	Now;
	int	uSec;

	TIMESPEC( Now );

	Ret = SetItemSet( pEvCon, pMsgClient, pMsgServer, pMI );
	if( Ret == 0 ) {

		/* Extime CksumTime */
		pMI->i_ReloadTime	= Now;
		uSec	= MemCacheCtrl.mc_SecExp*1000000L;
		TimespecAddUsec( pMI->i_ReloadTime, uSec );

		if( MemCacheCtrl.mc_bCksum ) {

			pMI->i_Cksum64	= MsgCksum64( pMsgClient, 0, 0, NULL );

			pMI->i_CksumTime	= Now;
			uSec	= MemCacheCtrl.mc_UsecCksum;
			TimespecAddUsec( pMI->i_CksumTime, uSec );
		}
	}
	return( Ret );
}

int
SetItemReplyFake( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI )
{
	int Ret;

	if( MsgIsText( pMsg ) ) {
		Ret = SetItemReplyFakeText( pEvCon, pMsg, pMI );
	} else if ( MsgIsBinary( pMsg ) ) {
		Ret = SetItemReplyFakeBinary( pEvCon, pMsg, pMI );
	} else {
		ABORT();
	}
	return( Ret );	
}

int
SetItemReply( FdEvCon_t *pEvCon, 
		Msg_t *pMsg, Msg_t *pMsgServer, MemItem_t* pMI )
{
	int Ret;

	if( MsgIsText( pMsg ) ) {
		Ret = SetItemReplyText( pEvCon, pMsg, pMsgServer, pMI );
	} else if( MsgIsBinary( pMsg ) ) {
		Ret = SetItemReplyBinary( pEvCon, pMsg, pMsgServer, pMI );
	} else {
		ABORT();
	}
	return( Ret );	
}

void
_WaitWB( FdEvCon_t *pEvCon, int N )
{
	/* Only one thread can wait.
		One ThreadWorker continuously treats reuests on a pEvCon  */
	while( pEvCon->c_CntWB > N ) {	
//LOG(pLog, LogDBG, "pEvCon=%p CntWB=%d N=%d", pEvCon, pEvCon->c_CntWB, N );
		CndWait( &pEvCon->c_Cnd, &pEvCon->c_Mtx );
	}
}

int
WaitWB( FdEvCon_t *pEvCon, int N )
{
	MtxLock( &pEvCon->c_Mtx );

	_WaitWB( pEvCon, N );

	MtxUnlock( &pEvCon->c_Mtx );
	return( 0 );
}

int
SetItemEpilogue( FdEvCon_t *pEvCon, MemItem_t *pMI, bool_t bFree )
{
	/* Threads wait at CntWB == 0 or at LenWB > CntWB */
	MtxLock( &pEvCon->c_Mtx );

	if( --pEvCon->c_CntWB == 0 ) {
		CndBroadcast( &pEvCon->c_Cnd );
	}
LOG( pLog, LogDBG, "### bFree=%d pEvCon=%p CntWB=%d ###", 
			bFree, pEvCon, pEvCon->c_CntWB );
	ASSERT( pEvCon->c_CntWB >= 0 );

	MtxUnlock( &pEvCon->c_Mtx );

	MemItemCacheUnlock( pEvCon, pMI, bFree ); 
	return( 0 );
}

/* SetItemWB */
typedef	struct {
	list_t		r_Lnk;
	FdEvCon_t	*r_pEvCon;
	MemItem_t	*r_pMI;
	Msg_t		*r_pMsg;
} RequestWB_t;

void*
ThreadWorkerWB( void *pArg )
{
	int Ret;
	RequestWB_t	*pReq;
	Msg_t		*pMsg;
	FdEvCon_t	*pEvCon;
	MemItem_t	*pMI;
	Msg_t		*pMsgServer;
	bool_t		bFree = FALSE;
	RetCode_t	RC;

	pthread_detach( pthread_self() );

	while( 1 ) {

		QueueWaitEntry( &MemCacheCtrl.mc_WorkerWB, pReq, r_Lnk );

		pEvCon	= pReq->r_pEvCon;
		pMI		= pReq->r_pMI;
		pMsg	= pReq->r_pMsg;

LOG( pLog, LogDBG, ":START pEvCon=%p pMI=%p pMsg=%p", pEvCon, pMI, pMsg ); 

		Free( pReq );

		RC	= MsgGetRC( pMsg );

		if( pMI->i_pAttrs->a_Epoch != GetMetaEpoch( pMsg )	// atomic
					&& !( RC == RET_NEW || RC == RET_YES )  ) {
LOG(pLog, LogDBG, "WB SKIP:a_Epoch=%"PRIu64" Epoch(Msg)=%"PRIu64" pEvCon=%p(CntWB=%d) RC=%d",
pMI->i_pAttrs->a_Epoch, GetMetaEpoch(pMsg), pEvCon, pEvCon->c_CntWB, RC );
				MemCacheCtrl.mc_Statistics.s_write_back_omitted++;
			goto skip;
		}
//LOG(pLog, LogDBG, "WB Exec:a_Epoch=%"PRIu64" Epoch(Msg)=%"PRIu64" pEvCon=%p(CntWB=%d) RC=%d",
//pMI->i_pAttrs->a_Epoch, GetMetaEpoch(pMsg), pEvCon, pEvCon->c_CntWB, RC );

		Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgServer );
		if( Ret < 0 ) {
			MtxLock( &pMI->i_Mtx );

			CancelAttr( pMI->i_pAttrs, 
					(struct sockaddr*)&pEvCon->c_pCon->m_Addr );

			MtxUnlock( &pMI->i_Mtx );

			MtxLock( &pEvCon->c_Mtx );

			pEvCon->c_Flags |= FDEVCON_ABORT;
			shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );

			MtxUnlock( &pEvCon->c_Mtx );
		}
		if( pMsgServer ) {
ASSERT( list_empty(&pMsgServer->m_Lnk) );
			DestroyMsg( pMsgServer );
		}
skip:
		DestroyMsg( pMsg );

		SetItemEpilogue( pEvCon, pMI, bFree );
LOG( pLog, LogDBG, ":END pEvCon=%p pMI=%p", pEvCon, pMI ); 
	}
	pthread_exit( NULL );
}

int
SetItemWB( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI )
{
	RequestWB_t	*pReq;

	pReq	= (RequestWB_t*)Malloc( sizeof(*pReq) );

	list_init( &pReq->r_Lnk );
	pReq->r_pEvCon	= pEvCon;
	pReq->r_pMI		= pMI;
	pReq->r_pMsg	= pMsg;

	MemCacheCtrl.mc_Statistics.s_write_back++;
	QueuePostEntry( &MemCacheCtrl.mc_WorkerWB, pReq, r_Lnk );

	return( 0 );
}

int
TooLargeCheck( FdEvCon_t *pEvCon, Msg_t *pMsg )
{
	BinHeadTag_t	*pHeadTag;
	int	Ret;

	if( MsgIsText( pMsg ) )	return( 0 );

	if( !MemCacheCtrl.mc_item_size_max ) {
			Ret = ReqRplMemcachedBySettings( pEvCon->c_pCon );
			if( Ret < 0 )	goto err;
	}
	pHeadTag	= (BinHeadTag_t*)pMsg->m_pTag2;
	if( MemCacheCtrl.mc_item_size_max 
			<= ntohl(pHeadTag->t_Head.h_BodyLen ) ) {

		ReplyRetCodeBinary( pEvCon, RET_TOO_LARGE, pMsg, NULL );

		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
SetItem( FdEvCon_t *pEvCon, char *pKey )
{
	int		Ret;
	MemItem_t	*pMI;
	bool_t	bWB;
	bool_t	bFree = FALSE;
	Msg_t	*pMsgClient, *pMsgServer, *pMsgClone;
	int		LenWB;
	RetCode_t	RC;
	RetCode_t	RetMetaCacheCheck;

	bWB		= MemCacheCtrl.mc_bWB;
	LenWB	= MemCacheCtrl.mc_LenWB;

	pMsgClient	= pEvCon->c_pMsg;

	// W-deleg
	pMI = MemItemCacheLockW( pEvCon, pKey, TRUE, NULL, NULL ); 

	if( DlgInitIsOn( &pMI->i_Dlg ) ) DlgInitOff( &pMI->i_Dlg );

	Ret = TooLargeCheck( pEvCon, pMsgClient );
	if( Ret < 0 ) {		// already send error
		if( pMI->i_pAttrs ) {	// delete attribute file
			Ret = DeleteAttrFile( pMI->i_Dlg.d_Dlg.d_Name );
			Free( pMI->i_pAttrs );
			pMI->i_pAttrs	= NULL;
		}
		bFree	= TRUE;
		goto ret;
	}

	RetMetaCacheCheck	= MetaCacheCheck( pEvCon, pMI, 
						(struct sockaddr*)&pEvCon->c_pCon->m_Addr, pMsgClient );
	if( RetMetaCacheCheck == RET_WRITE_THROUGH ) {
		goto through;
	}
	RC	= MsgGetRC( pMsgClient );
	if( RC < 0 ) {	// No Attr

		if( MsgIsText( pMsgClient ) ) {
			if( !MsgIsNoreply( pMsgClient ) )	{
				if( RC == RET_ERROR )	RC = RET_NOT_STORED;
				ReplyRetCodeText( pEvCon, RC );
			}
		} else if( MsgIsBinary( pMsgClient ) ) {
			if( RC == RET_ERROR )	RC = RET_NOT_STORED;
			ReplyRetCodeBinary( pEvCon, RC, pMsgClient, pMI );

		} else {
			ABORT();
		}
		goto ret;

	}
	if( bWB ) {

		MtxLock( &pMI->i_Mtx );	// recursive W and R

		pMsgClone	= CloneMsg( pMsgClient );

		Ret = SetItemUpdate( pEvCon, pMI, pMsgClient, NULL );

		Ret = SetItemReplyFake( pEvCon, pMsgClient, pMI );

		MtxUnlock( &pMI->i_Mtx );

		/* WB Request */
		if( Ret == 0 ) {

			MtxLock( &pEvCon->c_Mtx );

			_WaitWB( pEvCon, LenWB );
			pEvCon->c_CntWB++;

			MtxUnlock( &pEvCon->c_Mtx );

LOG(pLog, LogDBG, "SetItemWB pEvCon=%p c_CntWB=%d", pEvCon, pEvCon->c_CntWB );
			SetItemWB( pEvCon, pMsgClone, pMI );
			return( 0 );

		} else {
			DestroyMsg( pMsgClone );
			MemItemCacheUnlock( pEvCon, pMI, TRUE ); 
			return( 0 );
		}
	}	

through:
LOG(pLog, LogDBG, "Before" );
	WaitWB( pEvCon, 0 );
LOG(pLog, LogDBG, "After" );

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsgClient, pMI, &pMsgServer );
LOG(pLog, LogDBG, "ReqRplMemCached(%d)", Ret );
	if( Ret < 0 ) {
		bFree = TRUE;
		goto ret;
	}
	MtxLock( &pMI->i_Mtx );

	Ret = SetItemUpdate( pEvCon, pMI, pMsgClient, pMsgServer );
LOG(pLog, LogDBG, "SetItemUpdate(%d)", Ret );
	if( Ret < 0 ) {
	if( Ret == RET_ERROR || Ret == RET_NOENT )	bFree = TRUE;
		goto ret1;
	}
	Ret = SetItemReply( pEvCon, pMsgClient, pMsgServer, pMI );
LOG(pLog, LogDBG, "SetItemReply(%d)", Ret );
	if( Ret < 0 ) {
		bFree = TRUE;
		goto ret1;
	}
ret1:
	MtxUnlock( &pMI->i_Mtx );
ret:
LOG( pLog, LogDBG, "### bFree=%d pEvCon=%p CntWB=%d ###", 
			bFree, pEvCon, pEvCon->c_CntWB );
	MemItemCacheUnlock( pEvCon, pMI, bFree ); 

	return( 0 );
}

int
SetItemRequestText( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI )
{
	CmdToken_t	*pTok;
	char	Buf[TOKEN_BUF_SIZE];
	int		Len;
	int	Ret;

	pTok	= (CmdToken_t*)pMsg->m_pTag2;
	if( pTok->c_pCAS ) {
		sprintf( Buf, "%s %s %s %s %s %"PRIu64"\r\n", 
				pTok->c_pCmd, pTok->c_pKey, pTok->c_pFlags, pTok->c_pExptime, 
				pTok->c_pBytes, pMI->i_CAS );
//LOG( pLog, LogDBG, "[%s]", Buf );
	} else {
		sprintf( Buf, "%s %s %s %s %s\r\n", pTok->c_pCmd, pTok->c_pKey,
			pTok->c_pFlags, pTok->c_pExptime, pTok->c_pBytes );
	}
	Len = strlen( Buf );

	Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, Buf, Len );
	if( Ret < 0 )	goto err;

	pMsg->m_aVec[0].v_Flags |= VEC_DISABLED;	// set Cmd disabled

	Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsg );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
OmitCrLf( Msg_t * pMsg )
{
	MsgVec_t	*pVec;

	ASSERT( pMsg->m_N > 0 );

	pVec	= &pMsg->m_aVec[ pMsg->m_N - 1];	
	ASSERT( !(pVec->v_Flags & VEC_DISABLED) ); ASSERT( pVec->v_Len >= 2 );
	ASSERT( !memcmp( "\r\n", pVec->v_pStart, 2 ) );

	pVec->v_Flags	|= VEC_DISABLED;

	return( 0 );
}

int
SetItemSet( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	int	Ret;

	if( MsgIsText( pMsgClient ) ) {
		Ret = SetItemSetText( pEvCon, pMsgClient, pMsgServer, pMI );
	} else if( MsgIsBinary( pMsgClient ) ) {
		Ret = SetItemSetBinary( pEvCon, pMsgClient, pMsgServer, pMI );
	} else {
		ABORT();
	}
	if( Ret == 0 ) {
		if( MsgGetRC( pMsgClient ) > RET_YES ) {
			pMI->i_pAttrs->a_MetaCAS	= GetSeqMetaCAS();
		}
		pMI->i_pAttrs->a_aAttr[0].a_MetaCAS	= pMI->i_pAttrs->a_MetaCAS;
		pMI->i_pAttrs->a_aAttr[0].a_Epoch	= pMI->i_pAttrs->a_Epoch;
		pMI->i_Status	&= ~MEM_ITEM_CAS;
	}
	return( Ret );
}

int
SetItemSetText( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	CmdToken_t	*pTok, *pTok1, *pCmdReply = NULL;
	int	Bytes;
	int	Ret = 0;

	pTok	= (CmdToken_t*)pMsgClient->m_pTag2;

	if( pMsgServer )	pCmdReply	= (CmdToken_t*)pMsgServer->m_pTag2;

	if( !pCmdReply || !strcmp("STORED", pCmdReply->c_pCmd ) ) {

		if( pTok->c_pExptime ) {
			uint32_t	expire;

			expire	= atol( pTok->c_pExptime );
			if( 0 < expire && expire <= 60*60*24*30 ) {
				expire += time(0);
			}
			pMI->i_ExpTime.tv_sec	= expire;
			pMI->i_ExpTime.tv_nsec	= 0;
LOG(pLog,LogDBG,"ExpTime=%lu time=%lu",
pMI->i_ExpTime.tv_sec, time(0) );
		}
		pMsgClient->m_aVec[0].v_Flags |= VEC_DISABLED;	// set Cmd disabled

		if( !strcmp("set", pTok->c_pCmd )
			|| !strcmp("add", pTok->c_pCmd)
			|| !strcmp("replace", pTok->c_pCmd)
			|| !strcmp("cas", pTok->c_pCmd ) ) {

			// replace
			if( pMI->i_pData )	DestroyMsg( pMI->i_pData );
			pMI->i_pData = pMsgClient;
			pEvCon->c_pMsg	= NULL;

		} else if( !strcmp("append", pTok->c_pCmd ) ) {

			pTok1	= (CmdToken_t*)pMI->i_pData->m_pTag2;

			Bytes	= atoi(pTok1->c_pBytes)
					+ atoi( pTok->c_pBytes );
//LOG(pLog, LogDBG, "%s + %s", pTok->c_pBytes, pTok1->c_pBytes );

			sprintf( pTok->c_Buf, "%s %s %s %s %d %s\r\n", 
					pTok1->c_pCmd, pTok1->c_pKey, pTok1->c_pFlags, 
					pTok1->c_pExptime, Bytes, 
					(pTok1->c_pCAS ? pTok1->c_pCAS : "") ); 

			pTok->c_pCmd 	= strtok_r(pTok->c_Buf, " \r\n",&pTok->c_pSave);
			pTok->c_pKey 	= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pFlags	= strtok_r(NULL, " \r\n", &pTok->c_pSave);
			pTok->c_pExptime= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pBytes	= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pCAS	= strtok_r(NULL, " \r\n",&pTok->c_pSave);

			Free( pMI->i_pData->m_pTag2 );
			pMI->i_pData->m_pTag2 = pTok;

			OmitCrLf( pMI->i_pData );
			MsgAddDel( pMI->i_pData, pMsgClient );

			pEvCon->c_pMsg = NULL;

		} else if( !strcmp("prepend", pTok->c_pCmd ) ) {

			pTok1	= (CmdToken_t*)pMI->i_pData->m_pTag2;

			Bytes	= atoi(pTok1->c_pBytes)
					+ atoi( pTok->c_pBytes );

			sprintf( pTok->c_Buf, "%s %s %s %s %d %s\r\n", 
					pTok1->c_pCmd, pTok1->c_pKey, pTok1->c_pFlags, 
					pTok1->c_pExptime, Bytes, 
					(pTok1->c_pCAS ? pTok1->c_pCAS : "") ); 

			pTok->c_pCmd 	= strtok_r(pTok->c_Buf, " \r\n",&pTok->c_pSave);
			pTok->c_pKey 	= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pFlags	= strtok_r(NULL, " \r\n", &pTok->c_pSave);
			pTok->c_pExptime= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pBytes	= strtok_r(NULL, " \r\n",&pTok->c_pSave);
			pTok->c_pCAS	= strtok_r(NULL, " \r\n",&pTok->c_pSave);

			Free( pMI->i_pData->m_pTag2 );

			OmitCrLf( pMsgClient );
			MsgAddDel( pMsgClient, pMI->i_pData );

			pMI->i_pData	= pMsgClient;
			pEvCon->c_pMsg	= NULL;

		}
		if( pTok->c_pFlags )	pMI->i_Flags	= atoi( pTok->c_pFlags );
		if( pTok->c_pBytes )	pMI->i_Bytes	= atoll( pTok->c_pBytes );

	} else if( !strcmp("NOT_STORED", pCmdReply->c_pCmd ) ) {
		Ret = RET_NOT_STORED;		
	} else if( !strcmp("EXISTS", pCmdReply->c_pCmd ) ) {
		Ret = RET_EXISTS;		
	} else if( !strcmp("NOT_FOUND", pCmdReply->c_pCmd ) ) {
		Ret = RET_NOENT;		
	} else {
		Ret = -1;
	}	
//LOG(pLog, LogDBG, "Ret=%d", Ret );
	return( Ret );
}

int
SetItemReplyText( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
LOG( pLog, LogDBG, "### MsgClient=%p m_pTag1=%p ###", 
pMsgClient, pMsgClient->m_pTag1 );

	if( !MsgIsNoreply( pMsgClient ) ) {
		SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
		DestroyMsg( pMsgServer );
	}
	if( pEvCon->c_pMsg ) {
		ASSERT( pMsgClient == pEvCon->c_pMsg );
		pEvCon->c_pMsg	= NULL;
		DestroyMsg( pMsgClient );
	}
	return( 0 );
}

int
ReplyRetCodeText( FdEvCon_t *pEvCon, RetCode_t RetCode )
{
	int	Ret;
	char *pReply = "";
	int	ReplyLen;
	int	Fd;

	Fd	= pEvCon->c_FdEvent.fd_Fd;

	switch( RetCode ) {
		case RET_STORED: 	pReply = "STORED\r\n"; 		ReplyLen = 8;break;
		case RET_NOT_STORED:pReply = "NOT_STORED\r\n";	ReplyLen = 12;break;
		case RET_NOENT:		pReply = "NOT_FOUND\r\n";	ReplyLen = 11;break;
		case RET_EXISTS:	pReply = "EXISTS\r\n";		ReplyLen =  8;break;
		case RET_OK:		pReply = "OK\r\n";			ReplyLen =  4;break;
		case RET_WRITE_THROUGH:	ReplyLen = 0; break;
		default:	ABORT();
	}
LOG(pLog, LogDBG,"pEvCon=%p Fd=%d Reply[%s]", pEvCon, Fd, pReply ); 
	if( ReplyLen > 0 )	Ret = SendStream( Fd, pReply, ReplyLen );
	return( Ret );
}

int
ReplyRetCodeBinary( FdEvCon_t *pEvCon, RetCode_t RetCode, Msg_t *pMsgClient,
						MemItem_t *pMI )
{
	BinHeadTag_t 	*pHeadTagClient;
	BinHead_t		Head;
	uint16_t		Status;

	char	*pErr = NULL;

	switch( RetCode ) {
		case RET_STORED: 	
			Status = BIN_RPL_OK;	
			break;
		case RET_NOT_STORED:
			Status = BIN_RPL_ITEM_NOT_STORED; 
			pErr = "Not stored.";	
			break;
		case RET_NOENT:	
			Status = BIN_RPL_KEY_NOT_FOUND;	
			pErr = "Not found";	
			break;
		case RET_EXISTS:	
			Status = BIN_RPL_KEY_EXISTS;	
			pErr = "Data exists for key.";	
			break;
		case RET_OK:		
			Status = BIN_RPL_OK;	
			break;
		case RET_TOO_LARGE:
			Status = BIN_RPL_VALUE_TOO_LARGE;	
			pErr = "Too large";
			break;
		default:	ABORT();
	}
	pHeadTagClient	= (BinHeadTag_t*)pMsgClient->m_pTag2;
	memset( &Head, 0, sizeof(Head) );
	Head.h_Magic	= BIN_MAGIC_RPL;
	Head.h_Opcode	= pHeadTagClient->t_Head.h_Opcode;
	Head.h_Opaque	= pHeadTagClient->t_Head.h_Opaque;
	Head.h_Status	= htons( Status );
	if( Status )	Head.h_CAS	= 0;
	else 			Head.h_CAS	= htobe64( pMI->i_pAttrs->a_MetaCAS );
	if( pErr ) {
		int	len;

		len = strlen( pErr );
		Head.h_ExtLen	= len;
		Head.h_BodyLen	= htonl( len );
	}

	switch( pHeadTagClient->t_Head.h_Opcode ) {
	case BIN_CMD_SETQ:
	case BIN_CMD_REPLACEQ:
	case BIN_CMD_ADDQ:
	case BIN_CMD_APPENDQ:
	case BIN_CMD_PREPENDQ:
		if( Status != BIN_RPL_OK ) {
			SendBinaryHeadWithErr( pEvCon, &Head, pErr );
		}
		break;
	case BIN_CMD_GETQ:
	case BIN_CMD_GETKQ:
		break;
	default:
		SendBinaryHeadWithErr( pEvCon, &Head, pErr );
		break;
	}
LOG(pLog, LogDBG,"RC=%d MetaCmd[%d] Status=0x%x Opaque=0x%x", 
RetCode, MsgGetCmd(pMsgClient),Status, ntohl(Head.h_Opaque) );
	return( 0 );
}

RetCode_t
SetItemReplyFakeText( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI )
{
	RetCode_t	RC;

	RC	= MsgGetRC( pMsg );

	if( RC >= RET_DUP )	RC	= RET_STORED;

	// Fake reply
	if( !MsgIsNoreply( pMsg ) )	{
		ReplyRetCodeText( pEvCon, RC );
	}
LOG(pLog, LogDBG,"RC=%d MetaCmd[%d] pNoreply=%d", 
RC, MsgGetCmd(pMsg), MsgIsNoreply(pMsg) );
	return( RET_OK );
}

int
SetItemSetBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	BinHeadTag_t *pHeadTagClient, *pHeadTagServer, *pHeadTagMI;
	int		Flags, ExtLen, KeyLen, BodyLen, Work;
	uint64_t	CAS;
	uint16_t	Status;
	int			Ret = 0;
	uint32_t	expire;
	BinExtSet_t	*pBinExtSet;

	pHeadTagClient	= (BinHeadTag_t*)pMsgClient->m_pTag2;
	if( pMsgServer )	pHeadTagServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;

//LOG( pLog, LogDBG, "TagClient=%p TagServer=%p",
//pMsgClient->m_pTag2, (pMsgServer ? pMsgServer->m_pTag2 : NULL) );

	if( pMsgServer ) {
		Status = ntohs( pHeadTagServer->t_Head.h_Status );
//LOG( pLog, LogDBG, "Status=0x%x", Status );

		switch( Status ) {
		case BIN_RPL_OK:				Ret = 0;				break;
		case BIN_RPL_KEY_EXISTS:		Ret = RET_EXISTS;		break;
		case BIN_RPL_KEY_NOT_FOUND:		Ret = RET_NOENT;	break;
		case BIN_RPL_ITEM_NOT_STORED:	Ret = RET_NOT_STORED;	break;
		default:						Ret = -1;				break;
		}
		if( Ret > 0 )		goto ret;
		else if( Ret < 0 )	goto err;
	}

	switch( pHeadTagClient->t_Head.h_Opcode ) {
	case BIN_CMD_SET:
	case BIN_CMD_REPLACE:
	case BIN_CMD_ADD:
	case BIN_CMD_SETQ:
	case BIN_CMD_REPLACEQ:
	case BIN_CMD_ADDQ:
		if( pMI->i_pData)	DestroyMsg( pMI->i_pData );
		pMI->i_pData	= pMsgClient;

		((FdEvCon_t*)pMsgClient->m_pTag0)->c_pMsg = NULL;
		if( pHeadTagClient->t_pExt ) {
			pBinExtSet	= (BinExtSet_t*)pHeadTagClient->t_pExt;
			expire	= ntohl( pBinExtSet->set_Expiration );
			if( 0 < expire ) {
				if( expire <= 60*60*24*30 ) {
					expire += time(0);
				}
				pMI->i_ExpTime.tv_sec	= expire;
				pMI->i_ExpTime.tv_nsec	= 0;
			}
			pMI->i_Flags	= ntohl( pBinExtSet->set_Flags );
LOG(pLog,LogDBG,"ExpTime=%lu time=%lu Flags=0x%x",
pMI->i_ExpTime.tv_sec, time(0), pMI->i_Flags );
		}
		break;

	case BIN_CMD_APPEND:
	case BIN_CMD_APPENDQ:
		pHeadTagMI	= (BinHeadTag_t*)pMI->i_pData->m_pTag2;

		KeyLen	= ntohs( pHeadTagClient->t_Head.h_KeyLen );
		BodyLen	= ntohl( pHeadTagClient->t_Head.h_BodyLen );

		Work	= ntohl( pHeadTagMI->t_Head.h_BodyLen );
		pHeadTagMI->t_Head.h_BodyLen = htonl( Work + BodyLen - KeyLen );

		MsgAddDel( pMI->i_pData, pMsgClient );
		((FdEvCon_t*)pMsgClient->m_pTag0)->c_pMsg = NULL;
		break;

	case BIN_CMD_PREPEND:
	case BIN_CMD_PREPENDQ:
		pHeadTagMI	= (BinHeadTag_t*)pMI->i_pData->m_pTag2;
		pMI->i_pData->m_pTag2 = 0;

		pHeadTagMI->t_Head.h_Opcode	= pHeadTagClient->t_Head.h_Opcode;

		KeyLen	= ntohs( pHeadTagClient->t_Head.h_KeyLen );
		BodyLen	= ntohl( pHeadTagClient->t_Head.h_BodyLen );

		Work	= ntohl( pHeadTagMI->t_Head.h_BodyLen );
		pHeadTagMI->t_Head.h_BodyLen = htonl( Work + BodyLen - KeyLen );

		MsgAddDel( pMsgClient, pMI->i_pData );

		Free( pMsgClient->m_pTag2 );
		pMsgClient->m_pTag2 = pHeadTagMI;

		pMI->i_pData	= pMsgClient;
		((FdEvCon_t*)pMsgClient->m_pTag0)->c_pMsg = NULL;
		break;
	}
	pHeadTagMI	= (BinHeadTag_t*)pMI->i_pData->m_pTag2;

	Flags	= ntohl(((BinExtSet_t*)pHeadTagMI->t_pExt)->set_Flags ); 
	ExtLen	= pHeadTagMI->t_Head.h_ExtLen;
	KeyLen	= ntohs( pHeadTagMI->t_Head.h_KeyLen );
	BodyLen	= ntohl( pHeadTagMI->t_Head.h_BodyLen );

	pMI->i_Flags	= Flags; 
	pMI->i_Bytes	= BodyLen - ExtLen - KeyLen; 
	if( pMsgServer ) {

		LOG(pLog, LogDBG, "OldCAS=%"PRIu64"->NewCAS=%"PRIu64"", 
				pMI->i_CAS, be64toh( pHeadTagServer->t_Head.h_CAS ));

		CAS		= be64toh( pHeadTagServer->t_Head.h_CAS );
		pMI->i_CAS		= CAS; 
//LOG(pLog,LogDBG,"i_CAS=%"PRIu64"", pMI->i_CAS);
		if( CAS )	pMI->i_Status |= MEM_ITEM_CAS;
		else		pMI->i_Status &= ~MEM_ITEM_CAS;
	}
	//LOG(pLog, LogDBG, "Status=0x%x CAS=%"PRIu64"", pMI->i_Status, pMI->i_CAS );
ret:
	return( Ret );
err:
	return( -1 );
}

int
SendBinaryHeadWithErr( FdEvCon_t *pEvCon, BinHead_t *pHead, char *pErr )
{
	int	Ret;

LOG( pLog, LogDBG, 
"Magic[0x%x] Op[0x%x] Opaque[0x%x] Key[%d] Ext[%d] Body[%d] "
"Status[0x%x] CAS[%"PRIu64"]",
pHead->h_Magic, pHead->h_Opcode, ntohl(pHead->h_Opaque), 
ntohs(pHead->h_KeyLen), pHead->h_ExtLen, ntohl(pHead->h_BodyLen),
ntohs(pHead->h_Status), be64toh(pHead->h_CAS) ); 

	Ret = SendStream(pEvCon->c_FdEvent.fd_Fd, (char*)pHead, sizeof(*pHead));
	if( pErr ) {
		Ret = SendStream(pEvCon->c_FdEvent.fd_Fd, pErr, strlen(pErr) );
	}
	return( Ret );
}

RetCode_t
SetItemReplyFakeBinary( FdEvCon_t *pEvCon, Msg_t *pMsg, MemItem_t *pMI )
{
	RetCode_t	RC;

	RC	= MsgGetRC( pMsg );
//LOG(pLog, LogDBG, "RC=%d", RC );

	if( RC >= RET_DUP )	RC	= RET_STORED;

	pMI->i_Status	&= ~MEM_ITEM_CAS;
	ReplyRetCodeBinary( pEvCon, RC, pMsg, pMI );

	return( RET_OK );
}

int
SetItemReplyBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	BinHeadTag_t *pHeadTagClient;
	BinHeadTag_t *pHeadTagServer;

	pHeadTagClient	= (BinHeadTag_t*)pMsgClient->m_pTag2;
	pHeadTagServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;

	pHeadTagServer->t_Head.h_CAS = htobe64( pMI->i_MetaCAS );

	switch( pHeadTagClient->t_Head.h_Opcode ) {
	case BIN_CMD_SETQ:
	case BIN_CMD_REPLACEQ:
	case BIN_CMD_ADDQ:
	case BIN_CMD_APPENDQ:
	case BIN_CMD_PREPENDQ:
		if( (pHeadTagServer->t_Head.h_Status) != htons(BIN_RPL_OK) ) {
			pHeadTagServer->t_Head.h_Opcode	= pHeadTagClient->t_Head.h_Opcode;

			LOG( pLog, LogERR, "Opcode=0x%x-0x%x Status=x%x", 
				pHeadTagClient->t_Head.h_Opcode, 
				pHeadTagServer->t_Head.h_Opcode, 
				ntohs(pHeadTagServer->t_Head.h_Status) );

			pHeadTagServer->t_Head.h_Opcode = pHeadTagClient->t_Head.h_Opcode;

LOG( pLog, LogDBG, "###1 Fd=%d pMsg=%p", pEvCon->c_FdEvent.fd_Fd, pMsgServer );
			SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
		}
		break;
	default:
LOG( pLog, LogDBG, "###2 Fd=%d pMsg=%p", pEvCon->c_FdEvent.fd_Fd, pMsgServer );
		SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
		break;
	}

	DestroyMsg( pMsgServer );
	if( pEvCon->c_pMsg ) {
		ASSERT( pMsgClient == pEvCon->c_pMsg );
		pEvCon->c_pMsg	= NULL;
		DestroyMsg( pMsgClient );
	}

	return( 0 );
}

int
SetItemText( FdEvCon_t *pEvCon, char *pKey )
{
	int	Ret;

	Ret = SetItem( pEvCon, pKey );

	return( Ret );
}

int
SetItemBinary( FdEvCon_t *pEvCon )
{
	Msg_t	*pMsg;
	BinHeadTag_t *pHead;
	int	Ret;

	pMsg	= pEvCon->c_pMsg;
	pHead	= (BinHeadTag_t*)pMsg->m_pTag2;

	Ret = SetItem( pEvCon, pHead->t_pKey );

	return( Ret );
}

int
DeleteItem( FdEvCon_t *pEvCon, char *pKey )
{
	MemItem_t	*pMI;
	int	Ret;
	bool_t		bFree = TRUE;
	Msg_t	*pMsg, *pMsgServer;

	pMsg	= pEvCon->c_pMsg;

	// W-deleg
	pMI = MemItemCacheLockW( pEvCon, pKey, TRUE, NULL, NULL ); 

	if( pMI->i_pAttrs ) {	// delete attribute file
		Ret = DeleteAttrFile( pMI->i_Dlg.d_Dlg.d_Name );
		Free( pMI->i_pAttrs );
		pMI->i_pAttrs	= NULL;
	}

	WaitWB( pEvCon, 0 );	// serialize

	Ret	= ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgServer );
	if( Ret < 0 ) {
		goto ret;
	}

	Ret = DeleteItemReply( pEvCon, pMsg, pMsgServer, pMI );

//LOG( pLog, LogDBG, "### Msg=%p bFree=%d ###", pMsg, bFree );
	DestroyMsg( pMsg );
	pEvCon->c_pMsg = NULL;

ret:
	MemItemCacheUnlock( pEvCon, pMI, bFree ); 
	return( 0 );
}

int
DeleteItemReply( FdEvCon_t *pEvCon, 
		Msg_t *pMsg, Msg_t *pMsgServer, MemItem_t* pMI )
{
	int Ret;

	if( MsgIsText( pMsg ) ) {
		Ret = DeleteItemReplyText( pEvCon, pMsg, pMsgServer, pMI );
	} else if( MsgIsBinary( pMsg ) ) {
		Ret = DeleteItemReplyBinary( pEvCon, pMsg, pMsgServer, pMI );
	} else {
		ABORT();
	}
	return( Ret );	
}

int
DeleteItemRequestText( FdEvCon_t *pEvCon, Msg_t *pMsg )
{
	CmdToken_t	*pTok;
	char	Buf[TOKEN_BUF_SIZE];
	int	Ret;

	pTok		= (CmdToken_t*)pMsg->m_pTag2;

	sprintf( Buf, "delete %s\r\n", pTok->c_pKey );

	Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, Buf, strlen(Buf) );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
DeleteItemReplyText( FdEvCon_t *pEvCon, Msg_t *pMsg, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	CmdToken_t	*pCmdReply;
	int		Ret;

	pCmdReply	= (CmdToken_t*)pMsgServer->m_pTag2;

	if(!strcmp("DELETED", pCmdReply->c_pCmd ))	Ret = 0;
	else	Ret = -1;

	if( !MsgIsNoreply( pMsg) ) {
		SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer ); 
	}

//LOG( pLog, LogDBG, "### MsgServer=%p ###", pMsgServer );
	DestroyMsg( pMsgServer );

	return( Ret );
}

int
DeleteItemReplyBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient, 
				Msg_t *pMsgServer, MemItem_t *pMI )
{
	BinHeadTag_t *pHeadTagClient, *pHeadTagServer;
	int	Ret;

	pHeadTagClient	= (BinHeadTag_t*)pMsgClient->m_pTag2;
	pHeadTagServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;

	if( pHeadTagServer->t_Head.h_Status != htons(BIN_RPL_OK) ) goto err;

	if( pHeadTagClient->t_Head.h_Opcode == BIN_CMD_DELETE ) {
		pHeadTagServer->t_Head.h_Opcode	= pHeadTagClient->t_Head.h_Opcode;

		Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
	}
LOG( pLog, LogDBG, "### MsgServer=%p ###", pMsgServer );
	DestroyMsg( pMsgServer );

	return( Ret );
err:
	LOG( pLog, LogDBG, "ClientOpcode=0x%x ServerOpcode=0x%x",
		pHeadTagClient->t_Head.h_Opcode, pHeadTagServer->t_Head.h_Opcode );

	pHeadTagServer->t_Head.h_Opcode	= pHeadTagClient->t_Head.h_Opcode;

	SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );

//LOG( pLog, LogDBG, "### MsgServer=%p ###", pMsgServer );
	DestroyMsg( pMsgServer );
	return( -1 );
}

int
DeleteItemText( FdEvCon_t *pEvCon, char *pKey )
{
	int	Ret;

	Ret = DeleteItem( pEvCon, pKey );

	return( Ret );
}

int
DeleteItemBinary( FdEvCon_t *pEvCon )
{
	Msg_t	*pMsg;
	BinHeadTag_t	*pHead;
	int	Ret;

	pMsg	= pEvCon->c_pMsg;
	pHead	= (BinHeadTag_t*)pMsg->m_pTag2;

	Ret = DeleteItem( pEvCon, pHead->t_pKey );

	return( Ret );
}

int
_RmAttrsInCSS( void )
{
#define	ENTRY_MAX	100
	PFSDirent_t	aDirent[ENTRY_MAX];
	int		i, Ind, N;
	int		Ret = 0;

	N	= ENTRY_MAX;
	Ind	= 0;
	do {
		Ret = PFSReadDir( MemCacheCtrl.mc_pSession, MemCacheCtrl.mc_NameSpace, 
					aDirent, Ind, &N );
		if( Ret < 0 )	break;

		for( i = 0; i < N; i++ ) {
			if( !strcmp( ".", aDirent[i].de_Name ) 
				|| !strcmp( "..", aDirent[i].de_Name ) )	continue;

				
//LOG( pLog, LogDBG, "### de_Name[%s] ###", aDirent[i].de_Name );
			PFSDelete( MemCacheCtrl.mc_pSession, aDirent[i].de_Name );
		}
		Ind	= N;
	} while( N == ENTRY_MAX );

	return( Ret );
}


int
FlushAllText( FdEvCon_t *pEvCon, Msg_t *pMsg )
{
	Msg_t	*pMsgFromServer;
	int		Ret;
	DlgCache_t	*pD;
	MtxLock( &MemCacheCtrl.mc_MtxCritical );

	pD = MemSpaceLockW( pEvCon );

	WaitWB( pEvCon, 0 );	// serialize request

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, NULL, &pMsgFromServer );
	if( Ret < 0 ) {
		goto ret;
	}
	DestroyMsg( pMsgFromServer );
ret:
	Ret = DlgCacheFlush( &MemCacheCtrl.mc_MemItem, MEM_ITEM_FLUSH, FALSE ); 
	/* Attribute Files in Disk */
	_RmAttrsInCSS();

	if( !MsgIsNoreply( pMsg ) ) {
		ReplyRetCodeText( pEvCon, RET_OK );
	}
//LOG( pLog, LogDBG, "### Msg=%p ###", pMsg );
	MemSpaceUnlock( pD, pEvCon );

	MtxUnlock( &MemCacheCtrl.mc_MtxCritical );

	return( Ret );
}

typedef	struct {
	FdEvCon_t	*te_pEvCon;
	Msg_t		*te_pMsg;
} TEArg_t;

int
FlushAllTextDelay( TimerEvent_t *pTimerEvent )
{
	TEArg_t	*pArg = (TEArg_t*)pTimerEvent->e_pArg;
	FdEvCon_t	*pEvCon = pArg->te_pEvCon;
	Msg_t		*pMsg = pArg->te_pMsg;
	int			Ret;

	Free( pArg );

	Ret = FlushAllText( pEvCon, pMsg );

	MtxLock( &pEvCon->c_Mtx );

LOG( pLog, LogDBG, "pTimerEvent=%p", pEvCon->c_pTimerEvent );
	pEvCon->c_pTimerEvent	= NULL;

	CndBroadcast( &pEvCon->c_Cnd );

	MtxUnlock( &pEvCon->c_Mtx );

	DestroyMsg( pMsg );

	return( Ret );
}

int
FlushAllBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient )
{
	Msg_t	*pMsgServer;
	DlgCache_t	*pD;
	BinHeadTag_t	*pHeadTagClient, *pHeadTagServer;
	int	Ret;

	pHeadTagClient	= (BinHeadTag_t*)pMsgClient->m_pTag2;

	MtxLock( &MemCacheCtrl.mc_MtxCritical );

	pD = MemSpaceLockW( pEvCon );

	WaitWB( pEvCon, 0 );	// serialize request

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsgClient, NULL, &pMsgServer );
	if( Ret < 0 ) {
		goto err1;
	}
	pHeadTagServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;

	ASSERT(pHeadTagServer->t_Head.h_Opaque	== pHeadTagClient->t_Head.h_Opaque);

	ASSERT( pHeadTagServer->t_Head.h_Status == htons(BIN_RPL_OK) );

	Ret = DlgCacheFlush( &MemCacheCtrl.mc_MemItem, MEM_ITEM_FLUSH, FALSE ); 

	if( !MsgIsNoreply( pMsgClient ) ) {

		SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
	}
//LOG( pLog, LogDBG, "### MsgServer=%p ###", pMsgServer );
	if( pMsgServer )	DestroyMsg( pMsgServer );

//LOG( pLog, LogDBG, "### MsgClient=%p ###", pMsgClient );
	DestroyMsg( pMsgClient );
	pEvCon->c_pMsg	= NULL;

	/* Attribute Files in Disk */
	_RmAttrsInCSS();

	MemSpaceUnlock( pD, pEvCon );

	MtxUnlock( &MemCacheCtrl.mc_MtxCritical );

	return( Ret );
err1:
	((FdEvCon_t*)pMsgClient->m_pTag0)->c_pMsg = NULL;
//LOG( pLog, LogDBG, "### MsgClient=%p ###", pMsgClient );
	DestroyMsg( pMsgClient );
	return( -1 );
}

int
FlushAllBinaryDelay( TimerEvent_t *pTimerEvent )
{
	TEArg_t	*pArg = (TEArg_t*)pTimerEvent->e_pArg;
	FdEvCon_t	*pEvCon = pArg->te_pEvCon;
	Msg_t		*pMsg = pArg->te_pMsg;
	int			Ret;

	Free( pArg );

	Ret = FlushAllBinary( pEvCon, pMsg );

	MtxLock( &pEvCon->c_Mtx );

LOG( pLog, LogDBG, "pTimerEvent=%p", pEvCon->c_pTimerEvent );
	pEvCon->c_pTimerEvent	= NULL;

	CndBroadcast( &pEvCon->c_Cnd );

	MtxUnlock( &pEvCon->c_Mtx );

	DestroyMsg( pMsg );

	return( Ret );
}

int
IncrDecrText( FdEvCon_t *pEvCon )
{
	CmdToken_t	*pCmdToken;
	Msg_t	*pMsg, *pMsgFromServer;
	int		Ret;
	MemItem_t	*pMI;

	pMsg		= pEvCon->c_pMsg;
	pCmdToken	= (CmdToken_t*)pMsg->m_pTag2;

	// W-cache
	pMI = MemItemCacheLockW( pEvCon, pCmdToken->c_pKey, TRUE, NULL, NULL ); 

	if( DlgInitIsOn( &pMI->i_Dlg ) ) DlgInitOff( &pMI->i_Dlg );

	MetaCacheCheck( pEvCon, pMI, 
			(struct sockaddr*)&pEvCon->c_pCon->m_Addr, pMsg );

	if( MsgGetRC( pMsg ) < 0 ) {
		if( !MsgIsNoreply( pMsg ) ) {
			ReplyRetCodeText( pEvCon, RET_NOENT );
		}
		goto err;
	}
	if( pMI->i_pData ) {	// purge Body Data for THROUGH
		DestroyMsg( pMI->i_pData );
		pMI->i_pData = NULL;
	}
//LOG(pLog, LogDBG, "Before-4");
	WaitWB( pEvCon, 0 );	// serialize request
//LOG(pLog, LogDBG, "After-4");

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgFromServer );
	if( Ret < 0 )	goto err;
	pMI->i_pAttrs->a_MetaCAS	= GetSeqMetaCAS();
	pMI->i_pAttrs->a_aAttr[0].a_MetaCAS	= pMI->i_pAttrs->a_MetaCAS;
	pMI->i_pAttrs->a_aAttr[0].a_Epoch	= pMI->i_pAttrs->a_Epoch;
	pMI->i_Status	&= ~MEM_ITEM_CAS;

	if( !MsgIsNoreply( pMsg ) ) {
		Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsgFromServer ); 

		DestroyMsg( pMsgFromServer );
	}

	MemItemCacheUnlock( pEvCon, pMI, FALSE ); 
	return( Ret );
err:
	MemItemCacheUnlock( pEvCon, pMI, FALSE ); 
	return( Ret );
}

int
TouchText( FdEvCon_t *pEvCon )
{
	CmdToken_t	*pCmdToken;
	Msg_t	*pMsg, *pMsgFromServer;
	int		Ret;
	MemItem_t	*pMI;

	pMsg		= pEvCon->c_pMsg;
	pCmdToken	= (CmdToken_t*)pMsg->m_pTag2;

	// W-cache
	pMI = MemItemCacheLockW( pEvCon, pCmdToken->c_pKey, TRUE, NULL, NULL ); 

	if( DlgInitIsOn( &pMI->i_Dlg ) ) DlgInitOff( &pMI->i_Dlg );

//LOG(pLog, LogDBG, "Before-5");
	WaitWB( pEvCon, 0 );
//LOG(pLog, LogDBG, "After-5");

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgFromServer );
	if( Ret < 0 ) {
		goto err;
	}
	if( !MsgIsNoreply( pMsg ) ) {
		Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsgFromServer ); 

		DestroyMsg( pMsgFromServer );
	}
	// Addr
	pMI->i_pAttrs->a_I = 0;
	pMI->i_pAttrs->a_N = 1;
	pMI->i_pAttrs->a_aAttr[0].a_MemCached =  
							*(struct sockaddr*)&pEvCon->c_pCon->m_Addr;

	MemItemCacheUnlock( pEvCon, pMI, TRUE ); 
	return( Ret );
err:
	MemItemCacheUnlock( pEvCon, pMI, FALSE ); 
	return( Ret );
}

int
IncrDecrBinary( FdEvCon_t *pEvCon )
{
	BinHeadTag_t	*pHeadClient, *pHeadServer;
	Msg_t	*pMsg, *pMsgServer;
	int		Ret;
	MemItem_t	*pMI;
	BinHead_t	Head;
	BinExtIncrDecr_t	*pIncrDecr;

	pMsg		= pEvCon->c_pMsg;
	pHeadClient	= (BinHeadTag_t*)pMsg->m_pTag2;

	// W-cache
	pMI = MemItemCacheLockW( pEvCon, pHeadClient->t_pKey, TRUE, NULL, NULL ); 

	if( DlgInitIsOn( &pMI->i_Dlg ) ) DlgInitOff( &pMI->i_Dlg );

	if( !pMI->i_pAttrs ) {

		if( pHeadClient->t_Head.h_Opcode == BIN_CMD_INCR
			|| pHeadClient->t_Head.h_Opcode == BIN_CMD_DECR ) {

			pIncrDecr	= (BinExtIncrDecr_t*)(pHeadClient+1);
			if( pIncrDecr->id_Expiration == -1 ) {

				memset( &Head, 0, sizeof(Head) );
				Head.h_Magic	= BIN_MAGIC_RPL;
				Head.h_Opcode	= pHeadClient->t_Head.h_Opcode;
				Head.h_Status	= htons( BIN_RPL_KEY_NOT_FOUND );
				Head.h_Opaque	= pHeadClient->t_Head.h_Opaque;

				Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, 
								(char*)&Head, sizeof(Head) ); 
				Ret = -1;
				goto err;
			}
		}
		pMI->i_pAttrs	= (MemItemAttrs_t*)Malloc( sizeof(MemItemAttrs_t) );
LOG(pLog, LogDBG, "pMI=%p i_pAttrs=%p", pMI, pMI->i_pAttrs );
		memset( pMI->i_pAttrs, 0, sizeof(*pMI->i_pAttrs) );
	}
	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgServer );
	if( Ret < 0 )	goto err;
	pHeadServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;

	ASSERT(pHeadServer->t_Head.h_Opaque	== pHeadClient->t_Head.h_Opaque);

	pMI->i_pAttrs->a_MetaCAS	= GetSeqMetaCAS();
	pMI->i_pAttrs->a_aAttr[0].a_MetaCAS	= pMI->i_pAttrs->a_MetaCAS;
	pMI->i_pAttrs->a_aAttr[0].a_Epoch	= pMI->i_pAttrs->a_Epoch;
	pMI->i_Status	&= ~MEM_ITEM_CAS;

LOG( pLog, LogDBG, "h_Status=0x%x MetaCAS=%"PRIu64"", 
ntohs(pHeadServer->t_Head.h_Status), pMI->i_pAttrs->a_MetaCAS );
	if( pHeadServer->t_Head.h_Status != htons(BIN_RPL_OK)
		|| pHeadClient->t_Head.h_Opcode == BIN_CMD_INCR
		|| pHeadClient->t_Head.h_Opcode == BIN_CMD_DECR ) {

		pHeadServer->t_Head.h_Opcode	= pHeadClient->t_Head.h_Opcode;
		pHeadServer->t_Head.h_CAS	= htobe64( pMI->i_pAttrs->a_MetaCAS );

		Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer );
	}
	DestroyMsg( pMsgServer );

	// Addr
	pMI->i_pAttrs->a_I = 0;
	pMI->i_pAttrs->a_N = 1;
	pMI->i_pAttrs->a_aAttr[0].a_MemCached =  
								*(struct sockaddr*)&pEvCon->c_pCon->m_Addr;

	MemItemCacheUnlock( pEvCon, pMI, FALSE ); 
	return( Ret );
err:
	MemItemCacheUnlock( pEvCon, pMI, FALSE ); 
	return( Ret );
}

int
TouchBinary( FdEvCon_t *pEvCon )
{
	BinHeadTag_t	*pHeadClient;
	Msg_t	*pMsg, *pMsgFromServer;
	int		Ret;
	MemItem_t	*pMI;
	BinHeadTag_t	*pHeadServer;
	uint32_t	expire;
	BinExtTouch_t	*pBinExtTouch;
	timespec_t		ExpTime;
	timespec_t		Now;

	pMsg		= pEvCon->c_pMsg;
	pHeadClient	= (BinHeadTag_t*)pMsg->m_pTag2;

	// W-cache
	pMI = MemItemCacheLockW( pEvCon, pHeadClient->t_pKey, TRUE, NULL, NULL ); 

	if( DlgInitIsOn( &pMI->i_Dlg ) ) DlgInitOff( &pMI->i_Dlg );

	if( pHeadClient->t_pExt ) {
		pBinExtTouch	= (BinExtTouch_t*)pHeadClient->t_pExt;
		expire	= ntohl( pBinExtTouch->t_Expiration );
		if( 0 < expire ) {
			if( expire <= 60*60*24*30 ) {
				expire += time(0);
			}
			ExpTime.tv_sec	= expire;
			ExpTime.tv_nsec	= 0;
		}
LOG(pLog,LogDBG,"i_ExpTime=%lu time(0)=%lu ExpTime=%lu MetaCAS=%"PRIu64"",
pMI->i_ExpTime.tv_sec, time(0), ExpTime.tv_sec, pMI->i_MetaCAS );
	}

	TIMESPEC( Now );
	if( pMI->i_ExpTime.tv_sec
		&& TIMESPECCOMPARE( pMI->i_ExpTime, Now ) < 0 ) {

		CancelAttr( pMI->i_pAttrs, (struct sockaddr*)&pEvCon->c_pCon->m_Addr );
		ReplyRetCodeBinary( pEvCon, RET_NOENT, pMsg, pMI );
		Ret = -1;
		goto	err;
	}
	pMI->i_ExpTime	= ExpTime;
	pMI->i_pAttrs->a_MetaCAS	= GetSeqMetaCAS();
	pMI->i_pAttrs->a_aAttr[0].a_MetaCAS	= pMI->i_pAttrs->a_MetaCAS;

	Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, pMI, &pMsgFromServer );
	if( Ret < 0 )	goto err;

	pHeadServer	= (BinHeadTag_t*)pMsgFromServer->m_pTag2;
	pHeadServer->t_Head.h_CAS = htobe64( pMI->i_MetaCAS );

	switch( pHeadClient->t_Head.h_Opcode ) {

	case BIN_CMD_GAT:
	case BIN_CMD_GATQ:

	case BIN_CMD_TOUCH:

		Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgFromServer );
		DestroyMsg( pMsgFromServer );
		break;
	default:
		ABORT();
		break;
	}
	// Addr ???
	pMI->i_pAttrs->a_I = 0;
	pMI->i_pAttrs->a_N = 1;
	pMI->i_pAttrs->a_aAttr[0].a_MemCached =  
							*(struct sockaddr*)&pEvCon->c_pCon->m_Addr;

	MemItemCacheUnlock( pEvCon, pMI, FALSE );
	return( Ret );
err:
	MemItemCacheUnlock( pEvCon, pMI, TRUE );
	return( Ret );
}

int
GetItem( FdEvCon_t *pEvCon, char *pKey, bool_t bCAS )
{
	int	Ret;
	MemItem_t	*pMI;
	Msg_t		*pMsgClient;
	Msg_t		*pMsgServer;
	bool_t		bFree = FALSE;
	timespec_t	Now;
	int			uSec;
	uint64_t	Cksum64;
	bool_t		bDoCksum = TRUE;

	pMsgClient	= pEvCon->c_pMsg;

	TIMESPEC( Now );

	// R-cache
	pMI = MemItemCacheLockR( pEvCon, pKey, TRUE, NULL, NULL );

	MemCacheCtrl.mc_Statistics.s_refer++;

//LOG(pLog, LogDBG, "After-6");
	WaitWB( pEvCon, 0 );
//LOG(pLog, LogDBG, "Before-6");

	MtxLock( &pMI->i_Mtx );

	if( DlgInitIsOn( &pMI->i_Dlg ) )	DlgInitOff( &pMI->i_Dlg );

	if( !pMI->i_pAttrs )	{

		LOG(pLog, LogERR, "NO ENT" );

		bFree = TRUE;
		Ret = -1;
		goto reply;
	}
	// Expire ?	Only return NOENT

LOG(pLog,LogDBG,"ExpTime=%lu time=%lu Flags=0x%x",
pMI->i_ExpTime.tv_sec, time(0), pMI->i_Flags );
	if( pMI->i_ExpTime.tv_sec && TIMESPECCOMPARE( pMI->i_ExpTime, Now ) < 0 ) {

		LOG(pLog, LogERR, "EXPIRED tv_sec=%d-%d", 
			pMI->i_ExpTime.tv_sec, Now.tv_sec );

		if( MsgIsBinary( pMsgClient ) ) {
			ReplyRetCodeBinary( pEvCon, RET_NOENT, pMsgClient, pMI );
		}
		bFree = TRUE;
		Ret = -1;
		goto ret;
	}

LOG(pLog,LogDBG,
"bCAS=%d pMI=%p MEM_ITEM_CAS=%d i_CAS=%"PRIu64" i_MetaCAS=%"PRIu64""
"i_pAttrs=%p", 
bCAS, pMI, pMI->i_Status & MEM_ITEM_CAS, pMI->i_CAS, pMI->i_MetaCAS, 
pMI->i_pAttrs );

	if( !pMI->i_pData
		|| (bCAS && !(pMI->i_Status & MEM_ITEM_CAS))
		|| TIMESPECCOMPARE( pMI->i_ReloadTime, Now ) < 0 ) { // Get from memcached

		LOG(pLog, LogINF, 
				"Re-Load:i_pData=%p bCAS=%d MEM_ITEM_CAS=%d TimeCmp=%d", 
				pMI->i_pData, bCAS, 
				(pMI->i_pData ? (pMI->i_Status & MEM_ITEM_CAS) : -1),
				TIMESPECCOMPARE( pMI->i_ReloadTime, Now ) );

		MemCacheCtrl.mc_Statistics.s_load++;
		Ret = ReqRplMemcachedByKey( pEvCon->c_pCon, pKey, &pMsgServer );
		if( Ret ) {
//			MtxLock( &pMI->i_Mtx );

			CancelAttr( pMI->i_pAttrs, 
					(struct sockaddr*)&pEvCon->c_pCon->m_Addr );

//			MtxUnlock( &pMI->i_Mtx );

			MtxLock( &pEvCon->c_Mtx );

			pEvCon->c_Flags |= FDEVCON_ABORT;
			shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );
	
			MtxUnlock( &pEvCon->c_Mtx );

			goto ret;
		}
		Ret = GetItemRegister( pEvCon, pMsgClient, pMsgServer, pMI );
		if( Ret == 0 ) {

			pMI->i_ReloadTime	= Now;
			uSec	= MemCacheCtrl.mc_SecExp*1000000L;
			TimespecAddUsec( pMI->i_ReloadTime, uSec );

			if( MemCacheCtrl.mc_bCksum ) {
				pMI->i_Cksum64	= MsgCksum64( pMsgServer, 0, 0, NULL );

				pMI->i_CksumTime	= Now;
				uSec	= MemCacheCtrl.mc_UsecCksum;
				TimespecAddUsec( pMI->i_CksumTime, uSec );

				bDoCksum	= FALSE;
			}
		} else {

			DestroyMsg( pMsgServer );
			bFree	= TRUE;
		}

	} else {	// cached

		MemCacheCtrl.mc_Statistics.s_hit++;
		if( MemCacheCtrl.mc_bCksum && bDoCksum ) {
			if( TIMESPECCOMPARE( pMI->i_CksumTime, Now ) < 0 ) {
				Cksum64	= MsgCksum64( pMI->i_pData, 0, 0, NULL );
				ASSERT( pMI->i_Cksum64	=  Cksum64 );
			}
		}
	}
reply:
if( pMI->i_pAttrs ) {
	LOG(pLog, LogDBG, "i_Flags=0x%x i_CAS=%"PRIu64" i_MetaCAS=%"PRIu64"", 
	pMI->i_Flags, pMI->i_CAS, pMI->i_MetaCAS );
}

	Ret = GetItemReply( pEvCon, pMsgClient, pMI );

ret:
	MtxUnlock( &pMI->i_Mtx );

	MemItemCacheUnlock( pEvCon, pMI, bFree ); 
	return( Ret );
}

int
GetItemText( FdEvCon_t *pEvCon, char *pKey, bool_t bCAS )
{
	int	Ret;

	Ret = GetItem( pEvCon, pKey,  bCAS );
	return( Ret );
}

int
GetItemBinary( FdEvCon_t *pEvCon, char *pKey )
{
	int	Ret;

	Ret = GetItem( pEvCon, pKey, TRUE );
	if( Ret < 0 ) {	// send key not found in no quietly
		Ret = 0;
	}
	DestroyMsg( pEvCon->c_pMsg );
	pEvCon->c_pMsg = NULL;

	return( Ret );
}

int
RequestFromClientText( FdEvCon_t *pEvCon )
{
	int	Ret;
	CmdToken_t	*pCmdToken;
	Msg_t		*pMsg;
	char		*pKey;
	bool_t		bCAS;

	pMsg = pEvCon->c_pMsg;

	pCmdToken	= (CmdToken_t*)pMsg->m_pTag2;

LOG(pLog, LogDBG, "### CMD[%s] ###", MsgFirst(pMsg) );

	pEvCon->c_Flags |= FDEVCON_SYNC;

	if( !strcmp("set", pCmdToken->c_pCmd )
		|| !strcmp("add", pCmdToken->c_pCmd)
		|| !strcmp("replace", pCmdToken->c_pCmd)
		|| !strcmp("append", pCmdToken->c_pCmd)
		|| !strcmp("prepend", pCmdToken->c_pCmd)
		|| !strcmp("cas", pCmdToken->c_pCmd)) {

		Ret = SetItemText( pEvCon, pCmdToken->c_pKey );
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else if( !strcmp("get", pCmdToken->c_pCmd )
				|| !strcmp("gets", pCmdToken->c_pCmd ) ) {

		if( !strcmp("gets", pCmdToken->c_pCmd ) )	bCAS = TRUE;
		else										bCAS = FALSE;

		while( (pKey = strtok_r( NULL," \r\n",&pCmdToken->c_pSave)) ) {

			Ret = GetItemText( pEvCon, pKey, bCAS );
		}
		Ret = SendEnd( pEvCon );

		pEvCon->c_pMsg = NULL;
		if( pMsg )	DestroyMsg( pMsg );
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else if( !strcmp("delete", pCmdToken->c_pCmd ) ) {

		Ret = DeleteItemText( pEvCon, pCmdToken->c_pKey );
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else if( !strcmp("flush_all", pCmdToken->c_pCmd ) ) {

		uint32_t	expire = 0;

		if( pCmdToken->c_pExptime )	expire	= atoi(pCmdToken->c_pExptime);

		if( expire ) {

			TEArg_t	*pArg = (TEArg_t*)Malloc( sizeof(TEArg_t) );
			int	ms;
	
			if( expire <= 60*60*24*30 ) {
				expire += time(0);
			}
			ms	= ( expire - time(0) )*1000;
LOG(pLog, LogDBG, "ms=%d", ms );

			pArg->te_pEvCon	= pEvCon;
			pArg->te_pMsg	= CloneMsg( pEvCon->c_pMsg );
			MsgSetNoreply( pArg->te_pMsg );

			MtxLock( &pEvCon->c_Mtx );

			if( pEvCon->c_pTimerEvent )	{
				TimerCancel( &MemCacheCtrl.mc_Timer,pEvCon->c_pTimerEvent );
			}
			pEvCon->c_pTimerEvent = TimerSet( &MemCacheCtrl.mc_Timer, ms, 
									FlushAllTextDelay, pArg );
LOG(pLog, LogDBG, "pTimerEvent=%p", pEvCon->c_pTimerEvent  );
			MtxUnlock( &pEvCon->c_Mtx );

			if( !MsgIsNoreply( pMsg ) ) {
				ReplyRetCodeText( pEvCon, RET_OK );
			}
		} else {
			Ret = FlushAllText( pEvCon, pEvCon->c_pMsg );
		}
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else if( !strcmp("incr", pCmdToken->c_pCmd )
				|| !strcmp("decr", pCmdToken->c_pCmd ) ) {

		Ret = IncrDecrText( pEvCon );
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else if( !strcmp("touch", pCmdToken->c_pCmd ) ) {

		Ret = TouchText( pEvCon );
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;
	} else if( !strcmp("quit", pCmdToken->c_pCmd ) ) {
		MtxLock( &pEvCon->c_Mtx );
		pEvCon->c_Flags |= FDEVCON_ABORT;
		shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );
		MtxUnlock( &pEvCon->c_Mtx );
	} else if( !strcmp("version", pCmdToken->c_pCmd ) && 
						MemCacheCtrl.mc_Version[0] ) {
		char buf[FILE_NAME_MAX];

		snprintf(buf, sizeof(buf), "VERSION %s\r\n", MemCacheCtrl.mc_Version);

		Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, buf, strlen(buf) );
		if( pEvCon->c_pMsg ) {
			DestroyMsg( pEvCon->c_pMsg );
			pEvCon->c_pMsg = NULL;
		}
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

	} else {
		// Through
		MtxLock( &pEvCon->c_Mtx );
LOG(pLog, LogDBG, "### through[%s] ###", pCmdToken->c_pCmd );

		pEvCon->c_Flags &= ~FDEVCON_SYNC;

		_WaitWB( pEvCon, 0 );

		Msg_t	*pMsgServer;

		MemCacheCtrl.mc_Statistics.s_write_through++;
		Ret = ReqRplMemCached( pEvCon->c_pCon, pMsg, NULL, &pMsgServer );
		DestroyMsg( pMsg );
		pEvCon->c_pMsg = NULL;

		if( Ret < 0 )	goto err1;

		if( pMsgServer ) {
			CmdToken_t	*pCmdToken;
			pCmdToken	= (CmdToken_t*)pMsgServer->m_pTag2;

			if( !MemCacheCtrl.mc_Version[0] && 
				!memcmp("VERSION ", pCmdToken->c_pCmd, 8 ) ) {
				memcpy( MemCacheCtrl.mc_Version, pCmdToken->c_pCmd+8,
							strlen(pCmdToken->c_pCmd) - 10 );
				LOG( pLog, LogDBG, "mc_Version[%s]", MemCacheCtrl.mc_Version );
			}
			Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer ); 
			DestroyMsg( pMsgServer );
		}
		MtxUnlock( &pEvCon->c_Mtx );
	}
	return( Ret );
err1:
	MtxUnlock( &pEvCon->c_Mtx );
	return( -1 );
}

int
SendBinaryByFd( int Fd, Msg_t *pMsg )
{
	BinHeadTag_t	*pHead;
	int	ExtLen, KeyLen;
	int	Ret;

	pHead	= (BinHeadTag_t*)pMsg->m_pTag2;

	ExtLen	= pHead->t_Head.h_ExtLen;
	KeyLen	= ntohs( pHead->t_Head.h_KeyLen );

//LOG( pLog, LogDBG, 
//"pEvCon=%p(fFd=%d) Magic=0x%x Opcode=0x%x ExtLen=%d KeyLen=%d BodyLen=%d", 
//pEvCon, pEvCon->c_FdEvent.fd_Fd,pHead->t_Head.h_Magic,pHead->t_Head.h_Opcode, 
//ExtLen, KeyLen, ntohl(pHead->t_Head.h_BodyLen) );

	// Head
	Ret = SendStream( Fd, (char*)&pHead->t_Head, sizeof(pHead->t_Head) );
	// Ext
	if( ExtLen > 0 ) {
		Ret = SendStream( Fd, (char*)pHead->t_pExt, ExtLen );
	}
	// Key
	if( KeyLen > 0 ) {
		Ret = SendStream( Fd, (char*)pHead->t_pKey, KeyLen );
	}
	// Value
	Ret = SendMsgByFd( Fd, pMsg );

	return( Ret );
}

int
RequestFromClientBinary( FdEvCon_t *pEvCon )
{
	Msg_t	*pMsg;
	int		Ret = 0;
	BinHeadTag_t	*pHead, *pHeadServer;
	uint8_t	Opcode;
	MemCachedCon_t	*pCon;

	pCon = pEvCon->c_pCon;

	pMsg = pEvCon->c_pMsg;
	pHead	= (BinHeadTag_t*)pMsg->m_pTag2;
	Opcode	= pHead->t_Head.h_Opcode;

	pEvCon->c_Flags |= FDEVCON_SYNC;

LOG( pLog, LogDBG, "==>Op=0x%x pOpaque=0x%x Msg=%p", 
pHead->t_Head.h_Opcode, ntohl(pHead->t_Head.h_Opaque), pMsg );
	switch( Opcode ) {
	case BIN_CMD_GETQ:
	case BIN_CMD_GET:
	case BIN_CMD_GETKQ:
	case BIN_CMD_GETK:
		Ret = GetItemBinary( pEvCon, pHead->t_pKey );
		break;
	case BIN_CMD_SETQ:
	case BIN_CMD_SET:
	case BIN_CMD_ADDQ:
	case BIN_CMD_ADD:
	case BIN_CMD_REPLACEQ:
	case BIN_CMD_REPLACE:
	case BIN_CMD_APPENDQ:
	case BIN_CMD_APPEND:
	case BIN_CMD_PREPENDQ:
	case BIN_CMD_PREPEND:
		Ret = SetItemBinary( pEvCon );
		break;
	case BIN_CMD_DELETEQ:
	case BIN_CMD_DELETE:
		Ret = DeleteItemBinary( pEvCon );
		break;

	case BIN_CMD_FLUSHQ:
		MsgSetNoreply( pMsg );
	case BIN_CMD_FLUSH:
		if( pHead->t_pExt ) {
			uint32_t	expire;
			TEArg_t	*pArg = (TEArg_t*)Malloc( sizeof(TEArg_t) );
			int	ms;
			BinHead_t	Head;

			expire	= ntohl( *((uint32_t*)pHead->t_pExt) );
			if( expire <= 60*60*24*30 ) {
				expire += time(0);
			}
			ms	= ( expire - time(0) )*1000;

			pArg->te_pEvCon	= pEvCon;
			pArg->te_pMsg	= CloneMsg( pMsg );
			MsgSetNoreply( pArg->te_pMsg );

			MtxLock( &pEvCon->c_Mtx );

			if( pEvCon->c_pTimerEvent )	{
				TimerCancel( &MemCacheCtrl.mc_Timer,pEvCon->c_pTimerEvent );
			}
			pEvCon->c_pTimerEvent = TimerSet( &MemCacheCtrl.mc_Timer, ms, 
									FlushAllBinaryDelay, pArg );
LOG(pLog, LogDBG, "pTimerEvent=%p", pEvCon->c_pTimerEvent  );
			MtxUnlock( &pEvCon->c_Mtx );

			memset( &Head, 0, sizeof(Head) );
			Head.h_Magic	= BIN_MAGIC_RPL;
			Head.h_Opcode	= BIN_CMD_FLUSH;

			if( Opcode == BIN_CMD_FLUSH ) {
				SendBinaryHeadWithErr( pEvCon, &Head, NULL );
			}
		} else {
			Ret = FlushAllBinary( pEvCon, pMsg );
		}
		break;
	case BIN_CMD_INCRQ:
	case BIN_CMD_DECRQ:
	case BIN_CMD_INCR:
	case BIN_CMD_DECR:
		Ret = IncrDecrBinary( pEvCon );
		break;
	case BIN_CMD_TOUCH:
	case BIN_CMD_GAT:
	case BIN_CMD_GATQ:
		Ret = TouchBinary( pEvCon );
		break;
	case BIN_CMD_QUIT:
	case BIN_CMD_QUITQ:
		MtxLock( &pEvCon->c_Mtx );
		pEvCon->c_Flags |= FDEVCON_ABORT;
		shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );
		MtxUnlock( &pEvCon->c_Mtx );
		break;
	case BIN_CMD_VERSION:
			if( MemCacheCtrl.mc_Version[0] ) {
				int	LenVer;

				pHead->t_Head.h_Magic	= BIN_MAGIC_RPL;
				LenVer	= strlen( MemCacheCtrl.mc_Version );
				pHead->t_Head.h_BodyLen	= htonl( LenVer );

				Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, 
						(char*)&pHead->t_Head, sizeof(pHead->t_Head) );
				Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, 
						MemCacheCtrl.mc_Version,  LenVer );
				break;
			}
	default:
		LOG( pLog, LogDBG, "THROUGH: Op=0x%x pMsg=%p", 
							pHead->t_Head.h_Opcode, pMsg );
		MtxLock( &pEvCon->c_Mtx );
		pEvCon->c_Flags &= ~FDEVCON_SYNC;

		_WaitWB( pEvCon, 0 );

		Msg_t	*pMsgServer;

		MtxLock( &pCon->m_Mtx );

		MemCacheCtrl.mc_Statistics.s_write_through++;
		Ret = _ReqRplMemCached( pEvCon->c_pCon, pMsg, NULL, &pMsgServer );

		DestroyMsg( pMsg );
		pEvCon->c_pMsg = NULL;

		if( Ret < 0 )	goto err1;

		if( pMsgServer ) {

			switch( Opcode ) {
			case BIN_CMD_STAT:
				do {
					Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, 
															pMsgServer ); 
					pHeadServer	= (BinHeadTag_t*)pMsgServer->m_pTag2;
LOG(pLog, LogDBG,"key[%s] value[%s]",
(pHeadServer->t_pKey ? pHeadServer->t_pKey:""),
(pHeadServer->t_pValue ? pHeadServer->t_pValue:"") );
					if(!pHeadServer->t_pKey && !pHeadServer->t_pValue)	break;

					DestroyMsg( pMsgServer );

					pMsgServer = RecvBinaryByFd( pEvCon->c_pCon->m_FdBin );
					if( !pMsgServer )	goto err1;

				} while( 1 );
				break;
			case BIN_CMD_QUITQ:
				break;
			default:
				Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer ); 
				break;
			case BIN_CMD_VERSION:
				if( !MemCacheCtrl.mc_Version[0] ) {
					pHead	= (BinHeadTag_t*)pMsgServer->m_pTag2;
					memcpy( MemCacheCtrl.mc_Version, pHead->t_pValue,
							ntohl( pHead->t_Head.h_BodyLen) );
					LOG( pLog, LogDBG, "len=%d mc_Version[%s]", 
							ntohl( pHead->t_Head.h_BodyLen), 
							MemCacheCtrl.mc_Version );
				}
				Ret = SendBinaryByFd( pEvCon->c_FdEvent.fd_Fd, pMsgServer ); 
				break;
			}
			DestroyMsg( pMsgServer );
		}
		if( Opcode == BIN_CMD_QUIT 
				|| Opcode == BIN_CMD_QUITQ ) {

			pEvCon->c_Flags |= FDEVCON_ABORT;
			shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );
		}
		MtxUnlock( &pCon->m_Mtx );
		MtxUnlock( &pEvCon->c_Mtx );
		break;
	}
	if( pEvCon->c_pMsg ) {
		DestroyMsg( pEvCon->c_pMsg );
		pEvCon->c_pMsg = NULL;
	}
LOG(pLog, LogDBG, "<==OK");
	return( Ret );
err1:
	MtxUnlock( &pEvCon->c_Mtx );
LOG(pLog, LogDBG, "<==NG");
	return( -1 );
}

int
RequestFromClient( FdEvCon_t *pEvCon )
{
	Msg_t	*pMsg;
	int		Ret;

	pMsg = pEvCon->c_pMsg;

	if( MsgIsText( pMsg ) ) {
		Ret = RequestFromClientText( pEvCon );
	} else if( MsgIsBinary( pMsg ) ) {
		Ret = RequestFromClientBinary( pEvCon );
	}
	else 					Ret = -1;

	return( Ret );
}

void*
ThreadWorker( void *pArg )
{
	int Ret;
	Msg_t		*pMsg;
	FdEvCon_t	*pEvCon;
	int			Type;

	pthread_detach( pthread_self() );

	while( 1 ) {

		QueueWaitEntry( &MemCacheCtrl.mc_Worker, pEvCon, c_LnkQ );
		if( !pEvCon )	goto err;

		MEM_LockR();
LOG( pLog, LogDBG, "#### WORKER START ####" );

		Type = pEvCon->c_FdEvent.fd_Type;

		while( 1 ) {
			MtxLock( &pEvCon->c_Mtx );

			pMsg = list_first_entry(&pEvCon->c_MsgList, Msg_t, m_Lnk);

LOG( pLog, LogDBG, 
"#### pEvCon(%d)=%p(pMsg=%p) c_Flags=0x%x pMsg=%p ####", 
Type, pEvCon, pEvCon->c_pMsg, pEvCon->c_Flags, pMsg );

			if( !pMsg ) {

				pEvCon->c_Flags &= ~FDEVCON_WORKER;
				CndBroadcast( &pEvCon->c_Cnd );

				MtxUnlock( &pEvCon->c_Mtx );

				goto next;
			}
			list_del_init( &pMsg->m_Lnk );

			ASSERT( pEvCon->c_pMsg == NULL );

			pEvCon->c_pMsg	= pMsg;

			MtxUnlock( &pEvCon->c_Mtx );

			if( Type == FD_TYPE_CLIENT )		Ret = RequestFromClient(pEvCon);
			else {
				LOG( pLog, LogERR, "c_type=%d", Type );
			}
			if( Ret < 0 ) {
				LOG( pLog, LogERR, "shutdown:c_type=%d Ret=%d", Type, Ret );

				MtxLock( &pEvCon->c_Mtx );

				pEvCon->c_Flags |= FDEVCON_ABORT;
				shutdown( pEvCon->c_FdEvent.fd_Fd, SHUT_RDWR );

				MtxUnlock( &pEvCon->c_Mtx );
			}
		}
next:
LOG( pLog, LogDBG, "#### WORKER END ####" );
		MEM_Unlock();
	}
err:
	pthread_exit( NULL );
}

/*
 *	Text From client
 */
bool_t
IsDigitStr( char *pStr )
{
	int	i, Len;

	Len	= strlen( pStr );
	for( i = 0; i < Len; ++i, ++pStr ) {
		if( isdigit( *pStr ) )	continue;
		return( FALSE );
	}
	return( TRUE );
}

int
RecvClientText( FdEvCon_t *pEvCon )
{
	char	Buf[TOKEN_BUF_SIZE], *pBuf;
	int		n, len;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	CmdToken_t	*pCmdToken;
	int64_t	Len;
	char	*pTok;
	char	*pNull = NULL;
	bool_t	bNoreply = FALSE;

	pMsg = MsgCreate( 3, Malloc, Free );
//LOG(pLog, LogDBG, "=== pMsg=%p ===", pMsg );
	pMsg->m_pTag0	= pEvCon;

	n = ReadCrLf( pEvCon->c_FdEvent.fd_Fd, Buf, sizeof(Buf) );
	if( n < 0 )	goto err1;

	Buf[n] = 0;

	pBuf = Malloc( n+1 );
	memcpy( pBuf, Buf, n+1 );

//	LOG( pLog, LogINF, "n=%d pBuf=%p [%s]", n, pBuf, Buf );

	MsgVecSet( &Vec, VEC_MINE, pBuf, n+1, n );
	MsgAdd( pMsg, &Vec );

	// Tokenize
	pCmdToken	= (CmdToken_t*)Malloc( sizeof(*pCmdToken) );
	memset( pCmdToken, 0, sizeof(*pCmdToken) - TOKEN_BUF_SIZE );

	MsgSetText( pMsg );
	pMsg->m_pTag2	= pCmdToken;

	memcpy( pCmdToken->c_Buf, Buf, n );
	pCmdToken->c_Buf[n]	= 0;

	pCmdToken->c_pCmd		= strtok_r( pCmdToken->c_Buf, " \r\n", 
										&pCmdToken->c_pSave);
#define	NOREPLY	"noreply\r\n"

	if( !pCmdToken->c_pCmd ) {
			SendClientError( pEvCon, "bad command line format" );
			goto err1;
	}
	int	l = sizeof(NOREPLY);
	if( n > l && Buf[n-l] == ' ' && !memcmp( &Buf[n-l+1], NOREPLY, l )) {
		bNoreply = TRUE;
	}   

	if( !strcmp("get", pCmdToken->c_pCmd )
		|| !strcmp("gets", pCmdToken->c_pCmd ) ) {

		char *pC = pCmdToken->c_pSave;

		while( *pC ) {
			if( *pC == ' ' || *pC == '\r' || *pC == '\n' ) {
				pC++;
				continue;
			} else { 
				break;
			}
			
		}
		if( *pC == 0 ) {
			SendClientError( pEvCon, "" );
			goto err1;
		}

	} else if( !strcmp("set", pCmdToken->c_pCmd )
		|| !strcmp("add", pCmdToken->c_pCmd)
		|| !strcmp("replace", pCmdToken->c_pCmd)
		|| !strcmp("append", pCmdToken->c_pCmd)
		|| !strcmp("prepend", pCmdToken->c_pCmd) ) {

		pCmdToken->c_pKey		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pFlags		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pExptime	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pBytes		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pNoreply	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		if( pCmdToken->c_pNoreply ) {
			pNull	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		}

		if( pCmdToken->c_pKey == NULL
		|| pCmdToken->c_pFlags == NULL || !IsDigitStr(pCmdToken->c_pFlags)
		|| pCmdToken->c_pExptime == NULL || !IsDigitStr(pCmdToken->c_pExptime)
		|| pCmdToken->c_pBytes == NULL || !IsDigitStr(pCmdToken->c_pBytes)
		|| (pCmdToken->c_pNoreply && strcmp("noreply",pCmdToken->c_pNoreply))
			|| pNull ) {

			bNoreply = (pCmdToken->c_pNoreply 
						&& !strcmp("noreply",pCmdToken->c_pNoreply));
			if( bNoreply )	goto err1;

			SendClientError( pEvCon, "bad command line format" );
			goto err1;
		}
		// Body Data
		Len	= atoll( pCmdToken->c_pBytes );

		pBuf = Malloc( (len = Len + 2/* Cr+Lf */) );

		MsgVecSet( &Vec, VEC_MINE, pBuf, len, Len );
		MsgAdd( pMsg, &Vec );
		MsgVecSet( &Vec, 0, pBuf+Len, 2, 2 );
		MsgAdd( pMsg, &Vec );

		if( ReadStream( pEvCon->c_FdEvent.fd_Fd, pBuf, len ) < 0 ) {
			goto err1;
		}

	} else if( !strcmp("cas", pCmdToken->c_pCmd ) ) {

		pCmdToken->c_pKey		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pFlags		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pExptime	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pBytes		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pCAS	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pNoreply	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		if( pCmdToken->c_pNoreply )
			pNull	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);

		if( pCmdToken->c_pKey == NULL
		|| pCmdToken->c_pFlags == NULL || !IsDigitStr(pCmdToken->c_pFlags)
		|| pCmdToken->c_pExptime == NULL || !IsDigitStr(pCmdToken->c_pExptime)
		|| pCmdToken->c_pBytes == NULL || !IsDigitStr(pCmdToken->c_pBytes)
		|| pCmdToken->c_pCAS == NULL || !IsDigitStr(pCmdToken->c_pCAS)
		|| (pCmdToken->c_pNoreply && strcmp("noreply",pCmdToken->c_pNoreply))
		|| pNull ) {

			bNoreply = (pCmdToken->c_pNoreply 
						&& !strcmp("noreply",pCmdToken->c_pNoreply));

			if( bNoreply )	goto err1;

			if( pCmdToken->c_pCAS == NULL ) {
				SendStream( pEvCon->c_FdEvent.fd_Fd, "ERROR\r\n", 7 );
			} else {
				SendClientError( pEvCon, "bad command line format" );
			}
			goto err1;
		}
		pCmdToken->c_MetaCAS = atoll( pCmdToken->c_pCAS );
		// Body Data
		Len	= atoll( pCmdToken->c_pBytes );

		pBuf = Malloc( (len = Len + 2/* Cr+Lf */) );

		MsgVecSet( &Vec, VEC_MINE, pBuf, len, Len );
		MsgAdd( pMsg, &Vec );
		MsgVecSet( &Vec, 0, pBuf+Len, 2, 2 );
		MsgAdd( pMsg, &Vec );

		if( ReadStream( pEvCon->c_FdEvent.fd_Fd, pBuf, len ) < 0 ) {
			goto err1;
		}

	} else if( !strcmp("delete", pCmdToken->c_pCmd ) ) {

		pCmdToken->c_pKey		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pNoreply	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		if( pCmdToken->c_pNoreply )
			pNull	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);

		if( pCmdToken->c_pKey == NULL
			|| (pCmdToken->c_pNoreply 
				&& strcmp("noreply",pCmdToken->c_pNoreply))
			|| pNull ) {

			if( bNoreply )	goto err1;

			SendClientError( pEvCon, 
				"bad command line format.  Usage: delete <key> [noreply]" );
			goto err1;
		}

	} else if( !strcmp("flush_all", pCmdToken->c_pCmd ) ) {

		while( (pTok = strtok_r( NULL," \r\n",&pCmdToken->c_pSave)) ) {
			if( !strcmp("noreply",pTok))	pCmdToken->c_pNoreply = pTok;
			else						 	pCmdToken->c_pExptime = pTok;
		}

	} else if( !strcmp("incr", pCmdToken->c_pCmd )
		|| !strcmp("decr", pCmdToken->c_pCmd) ) {

		pCmdToken->c_pKey		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pValue		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pNoreply	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		if( pCmdToken->c_pNoreply )
			pNull	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);

		if( pCmdToken->c_pKey == NULL 
			|| pCmdToken->c_pValue == NULL
			|| (pCmdToken->c_pNoreply 
				&& strcmp("noreply",pCmdToken->c_pNoreply))
			|| pNull ) {

			bNoreply = (pCmdToken->c_pNoreply 
						&& !strcmp("noreply",pCmdToken->c_pNoreply));

			if( bNoreply )	goto err1;

			if(pCmdToken->c_pNoreply 
				&& strcmp("noreply",pCmdToken->c_pNoreply)) {
				SendStream( pEvCon->c_FdEvent.fd_Fd, "ERROR\r\n", 7 );
				goto err1;
			}
			SendClientError( pEvCon, "bad command line format" );
			goto err1;
		}

	} else if( !strcmp("touch", pCmdToken->c_pCmd ) ) {

		pCmdToken->c_pKey		= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pExptime	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pNoreply	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);
		if( pCmdToken->c_pNoreply )
			pNull	= strtok_r( NULL," \r\n",&pCmdToken->c_pSave);

		if( pCmdToken->c_pKey == NULL
		|| pCmdToken->c_pExptime == NULL || !IsDigitStr(pCmdToken->c_pExptime)
		|| (pCmdToken->c_pNoreply && strcmp("noreply",pCmdToken->c_pNoreply))
		|| pNull ) {

			bNoreply = (pCmdToken->c_pNoreply 
						&& !strcmp("noreply",pCmdToken->c_pNoreply));

			if( bNoreply )	goto err1;

			SendClientError( pEvCon, "bad command line format" );
			goto err1;
		}
	}

	if( pCmdToken->c_pNoreply )	MsgSetNoreply( pMsg );

	// Post Data To Worker
	MtxLock( &pEvCon->c_Mtx );

	list_add_tail( &pMsg->m_Lnk, &pEvCon->c_MsgList );

	if( !(pEvCon->c_Flags & FDEVCON_WORKER) ) {

		pEvCon->c_Flags |= FDEVCON_WORKER;

		QueuePostEntry( &MemCacheCtrl.mc_Worker, pEvCon, c_LnkQ );
	}
	MtxUnlock( &pEvCon->c_Mtx );

	return( 0 );
err1:
	DestroyMsg( pMsg );
	return( -1 );
}

Msg_t*
RecvServerTextByFd( int Fd )
{
	char	Buf[TOKEN_BUF_SIZE], *pBuf;
	int		n, len;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	CmdToken_t	*pCmdToken;
	int64_t	Len;

	pMsg = MsgCreate( 4, Malloc, Free );
//LOG(pLog, LogDBG, "=== pMsg=%p ===", pMsg );

	Buf[0]	= 0;

	n = ReadCrLf( Fd, Buf, sizeof(Buf) );
	if( n < 0 )	goto err1;

	Buf[n] = 0;
//LOG( pLog, LogDBG, "n=%d [%s]", n, Buf );

	pBuf = Malloc( n );
	memcpy( pBuf, Buf, n );

	MsgVecSet( &Vec, VEC_MINE, pBuf, n, n );
	MsgAdd( pMsg, &Vec );

	pCmdToken	= (CmdToken_t*)Malloc( sizeof(*pCmdToken) );
	MsgSetText( pMsg );
	pMsg->m_pTag2	= pCmdToken;

	memcpy( pCmdToken->c_Buf, Buf, n );
	pCmdToken->c_Buf[n]	= 0;

	pCmdToken->c_pCmd		= strtok_r( pCmdToken->c_Buf, " \r\n",
											&pCmdToken->c_pSave );
	//LOG( pLog, LogINF, "REPLY[%s]", pCmdToken->c_pCmd );

	if( !strcmp( "VALUE", pCmdToken->c_pCmd ) ) {
		// Tokenize
		pCmdToken->c_pKey	= strtok_r(NULL, " \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pFlags	= strtok_r(NULL, " \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pBytes	= strtok_r(NULL, " \r\n",&pCmdToken->c_pSave);
		pCmdToken->c_pCAS	= strtok_r(NULL, " \r\n",&pCmdToken->c_pSave);

//LOG( pLog, LogDBG, "pKey[%s] pFlags[%s] pBytes[%s] pCAS[%s]", 
//pCmdToken->c_pKey, pCmdToken->c_pFlags, pCmdToken->c_pBytes, pCmdToken->c_pCAS);
		// Body Data
		Len	= atoll( pCmdToken->c_pBytes );
		pBuf = Malloc( (len= Len + 2) /*CrLf*/ );

		MsgVecSet( &Vec, VEC_MINE, pBuf, len, len );
		MsgAdd( pMsg, &Vec );
	
		if( ReadStream( Fd, pBuf, len ) < 0 ) goto err1;

		// End Data
		n = ReadCrLf( Fd, Buf, sizeof(Buf) );
		if( n < 0 )	goto err1;

		Buf[n] = 0;
//		LOG( pLog, LogINF, "VALUE [%s] Len=%d", pBuf, Len );

		ASSERT( n == 5 );

	} else if( !strcmp( "STAT", pCmdToken->c_pCmd ) ) {
		while( 1 ) {
			n = ReadCrLf( Fd, Buf, sizeof(Buf) );
			if( n < 0 )	goto err1;

			pBuf = Malloc( n );
			memcpy( pBuf, Buf, n );

			MsgVecSet( &Vec, VEC_MINE, pBuf, n, n );
			MsgAdd( pMsg, &Vec );

			if( !memcmp( "END", Buf, 3 ) )	break;

		}
	}
	return( pMsg );
err1:
	DestroyMsg( pMsg );
	return( NULL );
}

Msg_t*
RecvBinaryByFd( int Fd )
{
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	BinHead_t	Head;
	BinHeadTag_t	*pHead;
	void	*pExt = NULL, *pValue = NULL;
	int	ExtLen = 0, KeyLen = 0, BodyLen = 0, Len;
	int	Ret;

	pMsg = MsgCreate( 2, Malloc, Free );

	Ret = RecvStream( Fd, (char*)&Head, sizeof(Head) );
	if( Ret < 0 ) {
LOG(pLog, LogDBG, "Head read error(Fd=%d)", Fd );
		goto err;
	}

	ExtLen	= Head.h_ExtLen;
	KeyLen	= ntohs( Head.h_KeyLen );
	BodyLen	= ntohl( Head.h_BodyLen );

	Len	= sizeof(BinHeadTag_t) + ExtLen + KeyLen;
	pHead	= (BinHeadTag_t*)Malloc( Len + 1 ); 
	memset( pHead, 0, sizeof(*pHead) );

	pHead->t_Head		= Head;
	if( Head.h_Magic == BIN_MAGIC_REQ ) {
		pHead->t_MetaCAS	= be64toh( Head.h_CAS );	// From client
	}

	Len	= ExtLen + KeyLen;	

	if( Len > 0 ) {
		Ret = RecvStream( Fd, (char*)(pHead+1), Len );
		if( Ret < 0 ) {
			goto err;
		}
	}

	MsgSetBinary( pMsg );
	pMsg->m_pTag2	= pHead;

	LOG( pLog, LogDBG, 
		"pMsg= %p pHead=%p Magic=0x%x Op=0x%x Status=0x%x "
		"ExtLen=%d KeyLen=%d BodyLen=%d CAS=%"PRIu64"", 
		pMsg, pHead, Head.h_Magic, Head.h_Opcode, ntohs(Head.h_Status), 
		ExtLen, KeyLen, BodyLen, be64toh(Head.h_CAS) );

	if( ExtLen > 0 ) {
		pHead->t_pExt = pExt = (char*)(pHead+1);
	}
	if( KeyLen > 0 ) {
		pHead->t_pKey = (char*)(pHead+1) + ExtLen;
		pHead->t_pKey[KeyLen] = 0;
	}
	Len	= BodyLen - ExtLen - KeyLen;
	if( Len < 0 ) {
		LOG( pLog, LogERR, "Len(%d) , 0 ", Len );
		goto err;
	}
	if( Len > 0 ) {

		pValue	= Malloc( Len ); 
		pHead->t_pValue = pValue;
		MsgVecSet( &Vec, VEC_MINE, pValue, Len, Len );
		MsgAdd( pMsg, &Vec );

		Ret = RecvStream( Fd, pValue, Len );
		if( Ret < 0 ) {
LOG(pLog, LogDBG, "ValueLen=%d(Fd=%d)", Len, Fd );
		  goto err;
		}
	}
	return( pMsg );	
err:
	DestroyMsg( pMsg );
LOG(pLog,LogDBG,"errno=%d", errno );
	return( NULL );
}

Msg_t*
RecvBinary( FdEvCon_t *pEvCon )
{
	Msg_t	*pMsg;

	pMsg = RecvBinaryByFd( pEvCon->c_FdEvent.fd_Fd );

	if( pMsg )	pMsg->m_pTag0	= pEvCon;

	return( pMsg );
}

int
RecvClientBinary( FdEvCon_t *pEvCon )
{
	Msg_t		*pMsg;

	pMsg	= RecvBinary( pEvCon );
	if( !pMsg )	goto err1;

	// Post Data To Worker
	MtxLock( &pEvCon->c_Mtx );

	list_add_tail( &pMsg->m_Lnk, &pEvCon->c_MsgList );

	if( !(pEvCon->c_Flags & FDEVCON_WORKER) ) {

		pEvCon->c_Flags |= FDEVCON_WORKER;

		QueuePostEntry( &MemCacheCtrl.mc_Worker, pEvCon, c_LnkQ );
	}
	MtxUnlock( &pEvCon->c_Mtx );

	return( 0 );
err1:
	DestroyMsg( pMsg );
	return( -1 );
}

/*
 *	Handler from clients
 */
int
RecvHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	FdEvCon_t	*pFdEvCon = container_of( pEv, FdEvCon_t, c_FdEvent ); 
	int		Fd = pEv->fd_Fd;
	char	Magic;

	if( PeekStream( Fd, (char*)&Magic, sizeof(Magic) ) ) {

		LOG( pLog, LogERR, "PEEK ERROR pEv=%p Type=%d Fd=%d", 
				pFdEvCon, pFdEvCon->c_FdEvent.fd_Type, Fd );

		MtxLock( &MemCacheCtrl.mc_Mtx );

		// Delete wait for WORKER busy
		MtxLock( &pFdEvCon->c_Mtx );
		// Cancel timer
		if( pFdEvCon->c_pTimerEvent ) {

			void	*pArg;

			pArg = TimerCancel(	&MemCacheCtrl.mc_Timer, 
								pFdEvCon->c_pTimerEvent);
			if( pArg )	{
				Free( pArg );
				pFdEvCon->c_pTimerEvent	= NULL;
			} else {
				while( pFdEvCon->c_pTimerEvent ) {
					LOG( pLog, LogINF, "TimerEvent" );
					CndWait( &pFdEvCon->c_Cnd, &pFdEvCon->c_Mtx );
				}
			}
		}

		while( pFdEvCon->c_Flags & FDEVCON_WORKER ) {
			LOG( pLog, LogERR, "WAIT WORKER" );
			CndWait( &pFdEvCon->c_Cnd, &pFdEvCon->c_Mtx );
		}

		_WaitWB( pFdEvCon, 0 );

		if( pFdEvCon->c_pCon ) {
			if( pFdEvCon->c_pCon->m_Flags & MEMCACHEDCON_ABORT ) {
				PutMemCached( pFdEvCon->c_pCon, TRUE );
			} else {
				PutMemCached( pFdEvCon->c_pCon, FALSE );
			}
LOG(pLog, LogDBG, "c_pCon=%p->NULL");
			pFdEvCon->c_pCon	= NULL;
		}

		pFdEvCon->c_Flags	|= FDEVCON_ABORT;

		ASSERT(list_empty(&pFdEvCon->c_MsgList ) );

		MtxUnlock( &pFdEvCon->c_Mtx );

		FdEventDel( pEv );

		close( Fd );

LOG(pLog, LogDBG, "FdEvConFree(%p)", pFdEvCon );
		FdEvConFree( pFdEvCon );

		MtxUnlock( &MemCacheCtrl.mc_Mtx );

		return( 0 );
	}

	if( Magic & 0x80 )	RecvClientBinary( pFdEvCon );
	else				RecvClientText( pFdEvCon );
	
	return( 0 );
}

/*
 *	クライアントの要求毎にconnectionを張る
 */
FdEvCon_t*
FdEvConAlloc( int Fd, int Type, int Epoll, void *pTag, 
				int (*fHandler)(FdEvent_t*,FdEvMode_t) )
{
	FdEvCon_t	*pFdEvCon;

	pFdEvCon = (FdEvCon_t*)Malloc( sizeof(*pFdEvCon) );
	if( !pFdEvCon )	goto err;

	memset( pFdEvCon, 0, sizeof(*pFdEvCon) );
	list_init( &pFdEvCon->c_LnkQ );
	MtxInit( &pFdEvCon->c_Mtx );
	CndInit( &pFdEvCon->c_Cnd );
	list_init( &pFdEvCon->c_MsgList );

	list_init( &pFdEvCon->c_MI );

	FdEventInit( &pFdEvCon->c_FdEvent, Type, Fd, Epoll, pTag, fHandler );
	return( pFdEvCon );
err:
	return( NULL );
}

void
FdEvConFree( FdEvCon_t *pFdEvCon )
{
	Free( pFdEvCon );
}

int
AcceptHandler( FdEvent_t *pEv, FdEvMode_t Mode )
{
	FdEvCon_t	*pFdEvAccept = container_of( pEv, FdEvCon_t, c_FdEvent ); 
	int ListenFd = pEv->fd_Fd;
	MemCacheCtrl_t	*pMemCacheCtrl = (MemCacheCtrl_t*)pEv->fd_pArg;
	int	FdClient;
	int flags = 1;
	struct linger	Linger = { 1/*On*/, 10/*sec*/};
	struct sockaddr	AddrClient, AddrServer;
	socklen_t	len;
	uint64_t	U64C;
	FdEvCon_t	*pFdEvClient;
	int	Ret;

	DBG_ENT();

	if( Mode != EV_LOOP )	return( 0 );

	memset( &AddrClient, 0, sizeof(AddrClient) );
	len	= sizeof(AddrClient);

	FdClient	= accept( ListenFd, &AddrClient, &len );
	if( FdClient < 0 )	goto err;

	/* Memcached circuit */
	AddrServer	= pFdEvAccept->c_PeerAddr;

	/* Set Linger */
	setsockopt( FdClient, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger) );
	setsockopt( FdClient, IPPROTO_TCP, TCP_NODELAY, 
									(void *)&flags, sizeof(flags));
	U64C	= FdClient;
	pFdEvClient = FdEvConAlloc( FdClient, FD_TYPE_CLIENT, 
						FD_EVENT_READ, pMemCacheCtrl, RecvHandler );

	Ret = FdEventAdd( &pMemCacheCtrl->mc_FdEvClient, 
								U64C, &pFdEvClient->c_FdEvent );
	if( Ret < 0 )	goto err_del;

	pFdEvClient->c_pCon	= GetMemCached( &AddrServer );
	if( !pFdEvClient->c_pCon )	goto err_del;

	LOG( pLog, LogINF, 
		"BUILD CONNECTION:Client(%p:Fd=%d)-Server(%p:Fd=%d,FdBin=%d)",
		pFdEvClient, FdClient, pFdEvClient->c_pCon, 
		pFdEvClient->c_pCon->m_Fd, pFdEvClient->c_pCon->m_FdBin );

	DBG_EXT( 0 );
	return( 0 );
err_del:
LOG(pLog, LogDBG, "FdEvConFree(%p)", pFdEvClient );
	FdEvConFree( pFdEvClient );
	close( FdClient );
err:
	DBG_EXT( -1 );
	return( -1 );
}

char*
StrAddr( struct sockaddr *pAddr )
{
	struct sockaddr_un	*pUN;
	struct sockaddr_in	*pIN;
	static	char Buf[512];
	char	*pBuf = Buf;

	*pBuf	= '\0';

	if( pAddr->sa_family == AF_UNIX ) {
		pUN	= (struct sockaddr_un*)pAddr;
		sprintf( pBuf, "UNIX:%s", pUN->sun_path );
	} else if ( pAddr->sa_family == AF_INET ){
		pIN	= (struct sockaddr_in*)pAddr;
		sprintf( pBuf, "INET:%s/%d", 
				inet_ntoa(pIN->sin_addr), ntohs(pIN->sin_port) );
	}
	return( Buf );
}

int
PairCode( void *pA )
{
	int	Code, *pI, i;
	AcceptMemcached_t	*pAddrA = (AcceptMemcached_t*)pA;
	

	for( Code = 0, pI = (int*)&pAddrA->am_Accept, i = 0; 
			i < sizeof(struct sockaddr)/sizeof(int); i++, pI++ ) {
		Code += *pI;
	}	
	return( Code );
}

int
PairCmp( void *pA, void *pB )
{
	int	Ret;
	AcceptMemcached_t	*pAdA = (AcceptMemcached_t*)pA;
	AcceptMemcached_t	*pAdB = (AcceptMemcached_t*)pB;

	Ret = memcmp( &pAdA->am_Accept, &pAdB->am_Accept, sizeof(struct sockaddr) );
	return( Ret );
}

int
MemCacheWriteEphemeral()
{
	struct Session	*pSessionRAS;
	MemCacheCtrl_t	*pC = &MemCacheCtrl;
	File_t	*pFile;
	char	buff[1024];
	time_t	T;

#define	STAT( m )	pC->mc_Statistics.m

	pSessionRAS = pC->mc_pSessionRAS;
	if( !pSessionRAS )	goto err;

	pFile	= PFSOpen( pSessionRAS, pC->mc_Ephemeral, 0 );
	if( !pFile )	goto err;

	T = time(0);
	sprintf( buff, 
		"%s"
		"new[%"PRIu64"] delete[%"PRIu64"] update[%"PRIu64"]\n"
		"duprication[%"PRIu64"] replication[%"PRIu64"] refer[%"PRIu64"]\n"
		"hit[%"PRIu64"] error[%"PRIu64"] cas[%"PRIu64"] load[%"PRIu64"]\n"
		"write_through[%"PRIu64"] write_back[%"PRIu64"] "
		"write_back_omitted[%"PRIu64"]\n",

		ctime(&T),
		STAT(s_new), STAT(s_delete), STAT(s_update),
		STAT(s_duprication), STAT(s_replication), STAT(s_refer), 
		STAT(s_hit), STAT(s_error), STAT(s_cas), STAT(s_load),
		STAT(s_write_through), STAT(s_write_back), STAT(s_write_back_omitted));

	PFSWrite( pFile, buff, strlen(buff) );

	PFSClose( pFile );
	return( 0 );
err:
	return( -1 );
}

static void*
ThreadAdm( void *pArg )
{
	MemCacheCtrl_t	*pMemCacheCtrl = (MemCacheCtrl_t*)pArg;
	int	Ret;
	sigset_t set;
    int s;
	MemAddAcceptMemcached_req_t *pMemAddReq;
	MemDelAcceptMemcached_req_t *pMemDelReq;
	AcceptMemcached_t	*pMemAcceptMemcached, *pPair;
	struct sockaddr	Addr;
	uint64_t	U64;
	FdEvCon_t	*pFdEvCon;
	bool_t	bLocked = FALSE;
	char	StrAccept[128], StrConnect[128];
	struct sockaddr_un	AdmAddr;
	int	FdAdm, ad;
	int	Fd = -1;
	DlgCache_t	*pD;


	pthread_attr_t      thread_attr;
    size_t              stack_size = 0;
    size_t              guard_size = 0;

    pthread_attr_init( &thread_attr );
    pthread_attr_getstacksize( &thread_attr, &stack_size );
    pthread_attr_getguardsize( &thread_attr, &guard_size );
    LOG( pLog, LogINF, "Default stack size %zd[%zdM]; guard size %zd "
        "minimum is %u[%uK]",
        stack_size, stack_size >> 20, guard_size,
        PTHREAD_STACK_MIN, PTHREAD_STACK_MIN >> 10 );


    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s != 0) {
        LOG( pLog, LogINF, "pthread_sigmask s=%d", s );
		goto err;
	}
	pthread_detach( pthread_self() );

	/* 	Admin Port */
	AdmAddr	= pMemCacheCtrl->mc_AdmAddr;

	unlink( AdmAddr.sun_path ); 

	LOG( pLog, LogSYS, "AdmPath[%s]", AdmAddr.sun_path );

	if( (FdAdm = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err;
	}
	if( bind( FdAdm, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		goto err1;
	}
	listen( FdAdm, 1 );

	while( 1 ) {
		PaxosSessionHead_t	M, *pM, Rpl;

		errno = 0;

		ad	= accept( FdAdm, NULL, NULL );
		if( ad < 0 ) {
			LOG( pLog, LogERR, "accept ERROR" );
			continue;
		}
		LOG( pLog, LogSYS, "accepted(%d)", ad );
		while( 1 ) {
			Ret = PeekStream( ad, (char*)&M, sizeof(M) );
			if( Ret ) {
				LOG( pLog, LogERR, "peek ERROR", Ret, M.h_Len );
				break;
			}

			pM	= Malloc( M.h_Len );
			ASSERT( pM );

			if( (Ret = RecvStream( ad, (char*)pM, M.h_Len ) ) ) {
				LOG( pLog, LogERR, "recv ERROR Ret=%d h_Len=%d", Ret, M.h_Len );
				Free( pM );
				break;
			}
			switch( (int)pM->h_Cmd ) {

			case MEM_STOP:

				pD = MemSpaceLockW( NULL );

				Ret = GElmLoop( &MemCacheCtrl.mc_MemItem.dc_Ctrl, 
									NULL, 0, MEM_ITEM_STOP ); 

				MemCacheWriteEphemeral();

				MemSpaceUnlock( pD, NULL );

				errno = 0;
				Ret =	creat( PAXOS_MEM_SHUTDOWN_FILE, 0666 );
				if( Ret < 0 ) {
					LOG( pLog, LogERR, "creat error(errno=%d)", errno );
				}
				else
					LOG( pLog, LogERR, "creat(Ret=%d)", Ret );
				write( Ret, "TEST", 4 );
				close( Ret );

				LogDump( pLog );
   				SESSION_MSGINIT( &Rpl, MEM_STOP, 0, errno, sizeof(Rpl) );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
//
// must close session ???
//

				exit( 0 );
				break;

			case MEM_ADD_ACCEPT_MEMCACHED:

				pMemAddReq	= (MemAddAcceptMemcached_req_t*)pM;
				pMemAcceptMemcached	= &pMemAddReq->a_AcceptMemcached;

				strcpy(StrAccept, StrAddr(&pMemAcceptMemcached->am_Accept));
				strcpy(StrConnect, StrAddr(&pMemAcceptMemcached->am_Memcached));

				if( HashListGet( &MemCacheCtrl.mc_Pair, 
								pMemAcceptMemcached ) ) {
					LOG( pLog, LogERR, 
						"ADD dupricate ERROR:accept[%s] memcached[%s]", 
						StrAccept, StrConnect );

   					SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, EEXIST, sizeof(Rpl) );
					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					break;
				}
				LOG( pLog, LogSYS, "ADD pair:accept[%s] memcached[%s]", 
									StrAccept, StrConnect );

				Addr	= pMemAcceptMemcached->am_Accept;

				if( Addr.sa_family == AF_UNIX ) {

					unlink( ((struct sockaddr_un*)&Addr)->sun_path );

					if( (Fd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
   						SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, errno, sizeof(Rpl) );
						SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
						break;
					}
				} else if( Addr.sa_family == AF_INET ) {
					if( (Fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
   						SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, errno, sizeof(Rpl) );
						SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
						break;
					}
				} else {
					SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, EINVAL, sizeof(Rpl) );
					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					break;
				}
				if( bind( Fd, (struct sockaddr*)&Addr, sizeof(Addr) ) < 0 ) {
					SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, errno, sizeof(Rpl) );
					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					break;
				}
				listen( Fd, 1 );

				U64	= Fd;
				pFdEvCon = (FdEvCon_t*)Malloc( sizeof(*pFdEvCon) );
				FdEventInit( &pFdEvCon->c_FdEvent, FD_TYPE_ACCEPT, 
						Fd, FD_EVENT_READ, pMemCacheCtrl, AcceptHandler );
				pFdEvCon->c_PeerAddr	= pMemAcceptMemcached->am_Memcached;

				Ret = FdEventAdd( &pMemCacheCtrl->mc_FdEvClient, 
										U64, &pFdEvCon->c_FdEvent );
				if( Ret < 0 ) {
					SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, EEXIST, sizeof(Rpl) );
					SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
					goto err1;
				}

				pPair	= (AcceptMemcached_t*)Malloc( sizeof(*pPair) );
				memcpy( pPair, pMemAcceptMemcached, sizeof(*pPair) );

				list_init( &pPair->am_Lnk );

				pPair->am_pAcceptFd	= pFdEvCon;

				HashListPut( &MemCacheCtrl.mc_Pair, pPair, pPair, am_Lnk );

   				SESSION_MSGINIT( &Rpl, MEM_ADD_ACCEPT_MEMCACHED, 
												0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;

			case MEM_DEL_ACCEPT_MEMCACHED:

				pMemDelReq	= (MemDelAcceptMemcached_req_t*)pM;
				pMemAcceptMemcached	= &pMemDelReq->d_AcceptMemcached;

				strcpy(StrAccept, StrAddr(&pMemAcceptMemcached->am_Accept));
				strcpy(StrConnect, StrAddr(&pMemAcceptMemcached->am_Memcached));

				if( (pPair = HashListDel( &MemCacheCtrl.mc_Pair, 
						pMemAcceptMemcached, AcceptMemcached_t*, am_Lnk ))) {

					pFdEvCon	= pPair->am_pAcceptFd;
					FdEventDel( &pFdEvCon->c_FdEvent );
					close( pFdEvCon->c_FdEvent.fd_Fd );
					Free( pFdEvCon );
					Free( pPair );

					LOG( pLog, LogSYS, "DEL pair:accept[%s] memcached[%s]", 
									StrAccept, StrConnect );

   					SESSION_MSGINIT( &Rpl, MEM_DEL_ACCEPT_MEMCACHED, 
												0, 0, sizeof(Rpl) );
				} else {
					LOG( pLog, LogERR, 
						"DEL ERROR(NOENT):accept[%s] memcached[%s]", 
									StrAccept, StrConnect );

   					SESSION_MSGINIT( &Rpl, MEM_DEL_ACCEPT_MEMCACHED, 
												0, ENOENT, sizeof(Rpl) );
				}
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;

			case MEM_LOCK:

				MEM_LockW();
				bLocked	= TRUE;

   				SESSION_MSGINIT( &Rpl, MEM_LOCK, 0, 0, sizeof(Rpl) );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				LOG( pLog, LogINF, "MEM_LOCK");
				break;

			case MEM_UNLOCK:
				MEM_Unlock();
				bLocked	= FALSE;

   				SESSION_MSGINIT( &Rpl, MEM_UNLOCK, 0, 0, sizeof(Rpl) );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				LOG( pLog, LogINF, "MEM_UNLOCK");
				break;

			case MEM_CTRL:
				{MemCacheCtrl_rpl_t RplCtrl;
   				SESSION_MSGINIT( &RplCtrl, MEM_CTRL, 
										0, 0, sizeof(RplCtrl) );
				RplCtrl.c_pCtrl = &MemCacheCtrl;
				SendStream( ad, (char*)&RplCtrl, sizeof(RplCtrl) );
				}
				break;

			case MEM_DUMP:
				{MemCacheDump_req_t *pReqDump = (MemCacheDump_req_t*)pM;
   				SESSION_MSGINIT( &Rpl, MEM_DUMP, 
								0, 0, sizeof(Rpl) + pReqDump->d_Len );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				SendStream( ad, (char*)pReqDump->d_pAddr, 
								pReqDump->d_Len );
				}
				break;

			case MEM_LOG_DUMP:
				LogDump( pLog );
   				SESSION_MSGINIT( &Rpl, MEM_LOG_DUMP, 0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;

			case MEM_CLOSE:
   				SESSION_MSGINIT( &Rpl, MEM_CLOSE, 0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				Free( pM );
				goto close_end;
				break;

			case MEM_RAS_REGIST:
				{MemRASRegist_req_t	*pReq = (MemRASRegist_req_t*)pM;
				 MemRASRegist_rpl_t Rpl;
				char	Path[PATH_NAME_MAX];
				char	Ephemeral[FILE_NAME_MAX];
				struct Session *pRas;

				pRas	= MemCacheCtrl.mc_pSessionRAS;
				if( pRas ) {
					Ret	= EEXIST;
					goto regist_skip;
				}
				pRas	= MemSessionOpen( pReq->r_CellName, 
											SESSION_RAS_NO, TRUE ); 
				MemCacheCtrl.mc_pSessionRAS	= pRas;

				snprintf( Path, sizeof(Path), PAXOS_MEMCACHE_RAS_FMT,
							PAXOS_MEMCACHE_ROOT, MemCacheCtrl.mc_Space );
				sprintf( Ephemeral, "%d", MemCacheCtrl.mc_Id );

				Ret = PFSRasRegister( pRas, Path, Ephemeral );
regist_skip:
   				SESSION_MSGINIT( &Rpl, MEM_RAS_REGIST, 0, Ret, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

//				MemCacheWriteEphemeral();
				}
				break;
			case MEM_RAS_UNREGIST:
				{MemRASUnregist_req_t *pReq = (MemRASUnregist_req_t*)pM;
				 MemRASUnregist_rpl_t Rpl;
				char	Path[PATH_NAME_MAX];
				char	Ephemeral[FILE_NAME_MAX];
				struct Session *pRas;

				pRas	= PaxosSessionFind( pReq->u_CellName, SESSION_RAS_NO );
				if( !pRas ) {
					Ret = ENOENT;
					goto unregist_skip;
				}
				snprintf( Path, sizeof(Path), PAXOS_MEMCACHE_RAS_FMT,
							PAXOS_MEMCACHE_ROOT, MemCacheCtrl.mc_Space );
				sprintf( Ephemeral, "%d", MemCacheCtrl.mc_Id );

				Ret = PFSRasUnregister( pRas, Path, Ephemeral );

				MemSessionClose( pRas );
unregist_skip:
   				SESSION_MSGINIT( &Rpl, MEM_RAS_UNREGIST, 0, Ret, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				}
				break;

			case MEM_ACTIVE:
   				SESSION_MSGINIT( &Rpl, MEM_ACTIVE, 0, 0, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				break;

			default:
				LOG( pLog, LogERR, "Not defined pM->h_Cmd=0x%x", pM->h_Cmd );
				break;
			}
			Free( pM );
		}
close_end:
		close( ad );
		LOG( pLog, LogSYS, "closed(%d) errno=%d", ad, errno );
		if( bLocked ) {
			MEM_Unlock();
			bLocked	= FALSE;
			LOG( pLog, LogINF, "MEM_UNLOCK");
		}
	}
	if( Fd >= 0 )	close( Fd );
	Free( pArg );
	pthread_exit( NULL );
	return( NULL );
err1:
	if( Fd >= 0 )	close( Fd );
err:
	perror("EXIT");
	exit( -1 );
	return( NULL );
}

DlgCache_t*
MemAlloc( DlgCacheCtrl_t *pDC )
{
	MemItem_t	*pMI;

	pMI	= (MemItem_t*)Malloc( sizeof(*pMI) );
	memset( pMI, 0, sizeof(*pMI) );

	MtxInit( &pMI->i_Mtx );

	return( &pMI->i_Dlg );
}

int
MemFree( DlgCacheCtrl_t *pDC, DlgCache_t *pD )
{
	MemItem_t	*pMI = container_of( pD, MemItem_t, i_Dlg );

	Free( pMI );

	return( 0 );
}

int
MemInit( DlgCache_t *pD, void *pArg )
{
	MemItem_t	*pMI = container_of( pD, MemItem_t, i_Dlg );
	char	*pKey = pD->d_Dlg.d_Name;
	int		cnt	= 0;
	int		i;

	while( cnt < 2 ) {
		if( *pKey++ == '/' )	cnt++;
	}	
	pMI->i_pKey	= pKey;

	ASSERT( pMI->i_pAttrs == NULL );

	pMI->i_pAttrs = ReadAttr( pD->d_Dlg.d_Name );
	if( pMI->i_pAttrs ) {
		for( i = 0; i < pMI->i_pAttrs->a_N; i++ ) {
//			pMI->i_Status &= ~MEM_ITEM_CAS;
			pMI->i_pAttrs->a_aAttr[i].a_pData	= NULL;
		}
	}

	LOG( pLog, LogINF, "pMI=%p pKey[%s] Name[%s] pAttrs=%p Own=%d", 
		pMI, pMI->i_pKey, pD->d_Dlg.d_Name, pMI->i_pAttrs, pD->d_Dlg.d_Own );
	return( 0 );
}

int
MemTerm( DlgCache_t *pD, void *pArg )
{
	MemItem_t	*pMI = container_of( pD, MemItem_t, i_Dlg );
	int		i;

	LOG( pLog, LogINF, "pMI=%p pAttrs=%p Own=%d", 
		pMI, pMI->i_pAttrs, pD->d_Dlg.d_Own );

	if( pMI->i_pAttrs ) {
		if( DlgIsW( pD ) ) {
			if( pMI->i_pAttrs->a_N > 0 ) {
				WriteAttr( pMI->i_pAttrs, pD->d_Dlg.d_Name );
			} else {
				DeleteAttrFile( pMI->i_Dlg.d_Dlg.d_Name );
			}
		}
		for( i = 0; i < pMI->i_pAttrs->a_N; i++ ) {
			if( pMI->i_pAttrs->a_aAttr[i].a_pData ) {
				DestroyMsg( pMI->i_pAttrs->a_aAttr[i].a_pData );
			}
			if( pMI->i_pAttrs->a_aAttr[i].a_pDiff ) {
				DestroyMsg( pMI->i_pAttrs->a_aAttr[i].a_pDiff );
			}
		}
		Free( pMI->i_pAttrs );
		pMI->i_pAttrs = NULL;
	} else {
		DeleteAttrFile( pD->d_Dlg.d_Name );
	}
	return( 0 );
}

int
MemLoop( DlgCache_t *pD, void *pArg )
{
//	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
	int Ret = 0;
//	GElmCtrl_t	*pGE;
//	GElm_t		*pElm;
	MemItem_t	*pMI;

	pMI = container_of( pD, MemItem_t, i_Dlg );

	if( MEM_ITEM_FLUSH == pArg ) {

		// in W-mode of NameSpace
		PFSDlgHoldW( &pD->d_Dlg, pArg );	// recall all delegations

		DeleteAttrFile( pD->d_Dlg.d_Name );

		Free( pMI->i_pAttrs );
		pMI->i_pAttrs = NULL;

		PFSDlgReturn( &pD->d_Dlg, NULL );// normally reurn W-deleg
		PFSDlgRelease( &pD->d_Dlg );		// unlock local lock with W-deleg

	}
	else if( MEM_ITEM_STOP == pArg ) {

		if( pMI->i_pAttrs ) {
			if( DlgIsW( pD ) ) {

				DlgCacheReturn( pD, NULL );// normally reurn W-deleg
			}
		}
	}
	return( Ret );
}

int
GetItemRegisterWithBinary( Msg_t *pMsgServer, MemItem_t *pMI )
{
	BinHeadTag_t	*pHead;
	int			Ret = 0;
	BinExtGet_t	*pExtGet;
	int		ExtLen, KeyLen, BodyLen;

	pHead	= (BinHeadTag_t*)pMsgServer->m_pTag2;

	if( pHead->t_Head.h_Status == htons(BIN_RPL_OK) ) {

		/* R-cache */
		if( pMI->i_pData ) {
			LOG( pLog, LogDBG, "Replace Old i_pData=%p->%p", pMI->i_pData, pMsgServer );
			DestroyMsg( pMI->i_pData );
		}
		ExtLen	= pHead->t_Head.h_ExtLen;
		KeyLen	= ntohs( pHead->t_Head.h_KeyLen );
		BodyLen	= ntohl( pHead->t_Head.h_BodyLen );
		pMI->i_Bytes	= BodyLen - ExtLen - KeyLen; 

		pExtGet	= (BinExtGet_t*)pHead->t_pExt;
		if( pExtGet )	pMI->i_Flags	= ntohl( pExtGet->get_Flags );
		pMI->i_CAS		= be64toh( pHead->t_Head.h_CAS );

		if( pMI->i_CAS )	pMI->i_Status	|= MEM_ITEM_CAS;
		else				pMI->i_Status	&= ~MEM_ITEM_CAS;

		pMI->i_pData	= pMsgServer;
LOG(pLog,LogDBG,"i_pData=%p CAS=%"PRIu64" Flags=0x%x", 
pMI->i_pData, pMI->i_CAS, pMI->i_Flags );

	} else {	// Send Error
		pMI->i_Status	&= ~MEM_ITEM_CAS;
		Ret = -1;
LOG( pLog, LogDBG, "Error" );
	}
	return( Ret );
}

int
GetItemRegisterBinary( FdEvCon_t *pClient, 
			Msg_t *pMsgClient, Msg_t *pMsgServer, MemItem_t *pMI )
{
	BinHeadTag_t	*pHead;
	BinHead_t		*pH;
	int			Ret = 0;

	Ret = GetItemRegisterWithBinary( pMsgServer, pMI );
	if( Ret < 0 ) {
		pHead	= (BinHeadTag_t*)pMsgServer->m_pTag2;
		pH	= &(((BinHeadTag_t*)pMsgClient->m_pTag2)->t_Head);
		if( pH->h_Opcode == BIN_CMD_GET
			|| pH->h_Opcode	== BIN_CMD_GETK ) {

			pHead->t_Head.h_Opcode	= pH->h_Opcode;
			pHead->t_Head.h_Opaque	= pH->h_Opaque;

			Ret = SendBinaryByFd( pClient->c_FdEvent.fd_Fd, pMsgServer );
		}
	}
	return( Ret );
}

int
GetItemRegister( FdEvCon_t *pEvCon, 
			Msg_t *pMsgClient, Msg_t *pMsgServer, MemItem_t *pMI )
{
	int Ret;

	/* pMsgServer must be binary !!! */
	if( MsgIsText( pMsgClient ) ) {
		Ret = GetItemRegisterWithBinary( pMsgServer, pMI );
	} else if( MsgIsBinary( pMsgClient ) ) {
		Ret = GetItemRegisterBinary( pEvCon, pMsgClient, pMsgServer, pMI );
	} else {
		ABORT();
	}
	return( Ret );	
}

int
GetItemReplyText( FdEvCon_t *pEvCon, Msg_t *pMsgClient, MemItem_t *pMI )
{
	char Buf[TOKEN_BUF_SIZE];
	CmdToken_t	*pCmdClient;
	int	Ret;

	pCmdClient	= (CmdToken_t*)pMsgClient->m_pTag2;

	if( !pMI->i_pAttrs || !pMI->i_pData ) {
		LOG( pLog, LogERR, "NO DATA" );
		return( -1 );
	} else if( MsgIsText( pMI->i_pData ) ) {

		if( !strcmp("gets", pCmdClient->c_pCmd ) )
				sprintf( Buf, "VALUE %s %d %"PRIu64" %"PRIu64"\r\n",
						pMI->i_Key,
						pMI->i_Flags,
						pMI->i_Bytes,
						pMI->i_MetaCAS );

		else		sprintf( Buf, "VALUE %s %d %"PRIu64"\r\n",
						pMI->i_Key,
						pMI->i_Flags,
						pMI->i_Bytes );

//Buf[strlen(Buf)]=0;
//LOG( pLog, LogDBG, "[%s]",  Buf );

		Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, Buf, strlen(Buf) );

		Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMI->i_pData ); 

		return( Ret );

	} else if( MsgIsBinary( pMI->i_pData ) ) {

		if( !strcmp("gets", pCmdClient->c_pCmd ) )
				sprintf( Buf, "VALUE %s %d %"PRIu64" %"PRIu64"\r\n",
						pMI->i_Key,
						pMI->i_Flags,
						pMI->i_Bytes,
						pMI->i_MetaCAS );

		else		sprintf( Buf, "VALUE %s %d %"PRIu64"\r\n",
						pMI->i_Key,
						pMI->i_Flags,
						pMI->i_Bytes );

		Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, Buf, strlen(Buf) );

		Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMI->i_pData ); 

		Ret = SendStream( pEvCon->c_FdEvent.fd_Fd, "\r\n", 2 );

		return( Ret );
	} else {
		ABORT();
		return( -1 );
	}
}

int
GetItemReplyBinary( FdEvCon_t *pEvCon, Msg_t *pMsgClient, MemItem_t *pMI )
{
	BinHead_t	Head, *pHeadClient;
	BinExtGet_t	Flags;
	int			KeyLen, ExtLen, TotLen;
	int	Fd;
	int	Ret;
	bool_t	bBinary;

LOG(pLog,LogDBG,"###ENT");

	memset( &Head, 0, sizeof(Head) );
	pHeadClient	= &((BinHeadTag_t*)pMsgClient->m_pTag2)->t_Head;
	Fd	= pEvCon->c_FdEvent.fd_Fd;

	if( !pMI->i_pAttrs || !pMI->i_pData ) {

		LOG( pLog, LogERR, "NO DATA" );

		ReplyRetCodeBinary( pEvCon, RET_NOENT, pMsgClient, pMI );

		goto err;

	} else if( MsgIsBinary( pMI->i_pData ) )	bBinary = TRUE;
	else if( MsgIsText( pMI->i_pData ) )		bBinary = FALSE;
	else {
		ABORT();
	}

	ExtLen	= sizeof(Flags);
	switch( pHeadClient->h_Opcode ) {
	case BIN_CMD_GETK:
	case BIN_CMD_GETKQ:
		KeyLen	= strlen(pMI->i_Key );
		break;
	default:
		KeyLen	= 0;
	}
	TotLen	= ExtLen + KeyLen;
	if( bBinary ) {
		TotLen	+= MsgTotalLen( pMI->i_pData );
	} else {
		TotLen	+= MsgTotalLen( pMI->i_pData ) - 2 /* \r\n */;
	}

	Head.h_Magic	= BIN_MAGIC_RPL;
	Head.h_Opcode	= pHeadClient->h_Opcode;
	Head.h_KeyLen	= htons( KeyLen );
	Head.h_ExtLen	= ExtLen;
	Head.h_Status	= htons( BIN_RPL_OK );
	Head.h_BodyLen	= htonl( TotLen );
	Head.h_Opaque	= pHeadClient->h_Opaque;
	Head.h_CAS		= htobe64( pMI->i_MetaCAS );

LOG(pLog,LogDBG,
"ExtLen=%d KeyLen=%d TotLen=%d ValueLen=%d i_Flags=0x%x Opaque=0x%x",
ExtLen, KeyLen, TotLen, TotLen-ExtLen-KeyLen, 
pMI->i_Flags, ntohl(Head.h_Opaque) );

	Flags.get_Flags	= htonl( pMI->i_Flags );

	// Head
	Ret = SendStream( Fd, (char*)&Head, sizeof(Head) );
	if( Ret < 0 )	goto err;

	// Ext
	Ret = SendStream( Fd, (char*)&Flags, sizeof(Flags) );
	if( Ret < 0 )	goto err;

	// Key
	if( KeyLen > 0 ) {
		Ret = SendStream( Fd, pMI->i_Key, KeyLen );
		if( Ret < 0 )	goto err;
	}
	// Value
	if( bBinary ) {
		Ret = SendMsgByFd( pEvCon->c_FdEvent.fd_Fd, pMI->i_pData ); 
	} else {
		char *pdata = MsgFirst(pMI->i_pData);
		int len = MsgLen(pMI->i_pData)-2;
		Ret = SendStream( Fd, pdata, len );
	}
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
GetItemReply( FdEvCon_t *pEvCon, Msg_t *pMsgClient, MemItem_t *pMI )
{
	int Ret;

	if( MsgIsText( pMsgClient ) ) {
		Ret = GetItemReplyText( pEvCon, pMsgClient, pMI );
	} else if( MsgIsBinary( pMsgClient ) ) {
		Ret = GetItemReplyBinary( pEvCon, pMsgClient, pMI );
	} else {
		ABORT();
	}
	return( Ret );	
}

int
SendEnd( FdEvCon_t *pEvCon )
{
	static	char End[] = "END\r\n";

	SendStream( pEvCon->c_FdEvent.fd_Fd, End, strlen(End) );
	return( 0 );
}

int
SendClientError( FdEvCon_t *pEvCon, char *pError  )
{
	static	char Error[80];

	sprintf( Error, "CLIENT_ERROR %s\r\n", pError );

LOG(pLog, LogDBG, "pEvCon=%p Fd=%d [%s]", 
pEvCon, pEvCon->c_FdEvent.fd_Fd, Error );

	SendStream( pEvCon->c_FdEvent.fd_Fd, Error, strlen(Error) );
	return( 0 );
}

int
RequestGetsText( FdEvCon_t *pEvCon, char *pKey )
{
	char Buf[TOKEN_BUF_SIZE];

	sprintf( Buf, "gets %s\r\n", pKey );

	SendStream( pEvCon->c_FdEvent.fd_Fd, Buf, strlen(Buf) );
	return( 0 );
}

void
usage()
{
	printf("PaxosMemcache Id css space\n");
	printf("\t-I ddd(%d)   --- Item cache Max\n", DEFAULT_ITEM_MAX );
	printf("\t-s {1|0}     --- checksum On/Off\n");
	printf("\t-C ddd(%d)   --- checksum interval(msec)\n", DEFAULT_CKSUM_MSEC );
	printf("\t-E ddd(%d)   --- check expire(sec)\n", DEFAULT_EXPTIME_SEC );
	printf("\t-W d(%d)     --- workers\n", DEFAULT_WORKERS );
	printf("\t-w {1|0}     --- write back On/Off\n");
	printf("\t-B d(%d)     --- WB workers\n", DEFAULT_WORKERS );
	printf("\t-l ddd(%d)   --- write back request length\n", DEFAULT_LEN_WB );
}

//void
//bye_mtrace( void )
//{
//	muntrace();
//}

int
main( int ac, char *av[] )
{
	int	Ret;
	int	Id;
	int	Workers = DEFAULT_WORKERS;
	int	WorkerWBs = DEFAULT_WORKERS;
	int	ItemMax	= DEFAULT_ITEM_MAX;
	bool_t	bCksum	= TRUE;
	int	CksumMsec	= DEFAULT_CKSUM_MSEC;
	int	ExpSec		= DEFAULT_EXPTIME_SEC;
	int	LenWB		= DEFAULT_LEN_WB;
	int	i, j;
	bool_t	bWB	= TRUE;
	struct Session	*pSession, *pEvent;
	char	*pCellName;
	char	*pSpace;

//	if( getenv( "MALLOC_TRACE" ) ) {
//		atexit( bye_mtrace );
//		muntrace();
//	}
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
		case 's':	bCksum = atoi(av[j++]);	break;
		case 'I':	ItemMax = atoi(av[j++]);	break;
		case 'C':	CksumMsec = atol(av[j++]);	break;
		case 'E':	ExpSec = atol(av[j++]);	break;
		case 'W':	Workers = atoi(av[j++]);	break;
		case 'B':	WorkerWBs = atoi(av[j++]);	break;
		case 'w':	bWB = atoi(av[j++]);	break;
		case 'l':	LenWB = atoi(av[j++]);	break;
		}
	}

	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}

	if( ac < 4 ) {
		usage();
		goto err;
	}
	Id	= atoi(av[1]);
	pCellName = av[2];
	pSpace = av[3];

	/* Log */
	if( getenv("LOG_SIZE") ) {

		int	log_level	= LogINF;
		int	log_size	= 0;

		log_size	= atoi( getenv("LOG_SIZE") );
		if( getenv("LOG_LEVEL") ) log_level	= atoi( getenv("LOG_LEVEL") );
		pLog = LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
							stderr, log_level );
	}
	/* Shutdown file */
	unlink( PAXOS_MEM_SHUTDOWN_FILE );

	/* Session, pSession and pEvent */
	pSession	= MemSessionOpen( pCellName, 0, TRUE );
	if( !pSession )	{
		printf("Can't open session[%s]\n", pCellName );
		goto err;
	}
	pEvent		= MemSessionOpen( pCellName, 1, TRUE );
	if( !pEvent )	{
		printf("Can't open event session[%s]\n", pCellName );
		goto err;
	}
	/* Init */
	MemCacheCtrl.mc_Id	= Id;
	strncpy( MemCacheCtrl.mc_Space, pSpace, sizeof(MemCacheCtrl.mc_Space) );
	snprintf( MemCacheCtrl.mc_NameSpace, sizeof(MemCacheCtrl.mc_NameSpace),
				PAXOS_MEMCACHE_SPACE_FMT, PAXOS_MEMCACHE_ROOT, pSpace );
	snprintf( MemCacheCtrl.mc_Ephemeral, sizeof(MemCacheCtrl.mc_Ephemeral),
				PAXOS_MEMCACHE_EPHEMERAL_FMT, PAXOS_MEMCACHE_ROOT, pSpace, Id );
	MemCacheCtrl.mc_pSession	= pSession;
	MemCacheCtrl.mc_pEvent		= pEvent;

	GElmCtrlInit( &MemCacheCtrl.mc_MemCacheds, NULL, 
		_AllocMemCachedCon, _FreeMemCachedCon, 
		_SetMemCachedCon, _UnsetMemCachedCon, NULL, MEMCACHED_MAX, PRIMARY_11, 
		_AddrCode, _AddrCmp, NULL, Malloc, Free, NULL );	

	MtxInit( &MemCacheCtrl.mc_Mtx );
	MtxInit( &MemCacheCtrl.mc_MtxCritical );
	RwLockInit( &MemCacheCtrl.mc_RwLock );
	HashListInit( &MemCacheCtrl.mc_Pair, PRIMARY_11, PairCode, PairCmp,
				NULL, Malloc, Free, NULL );

	Ret = DlgCacheRecallStart( &MemCacheCtrl.mc_Recall, pEvent );	
	if( Ret < 0 )	goto err;

	Ret = DlgCacheInitBy( pSession, 
						&MemCacheCtrl.mc_Recall, &MemCacheCtrl.mc_MemItem,
						ItemMax, MemAlloc, MemFree, NULL, NULL,
						MemInit, MemTerm, MemLoop, pLog );
	if( Ret < 0 )	goto err;

	Ret = DlgCacheInitBy( pSession, 
						&MemCacheCtrl.mc_Recall, &MemCacheCtrl.mc_MemSpace,
						5, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, pLog );
	if( Ret < 0 )	goto err;

	Ret = DlgCacheInit( pSession, 
						&MemCacheCtrl.mc_Recall, &MemCacheCtrl.mc_SeqMetaCAS,
						5, NULL, NULL, NULL, NULL,
						SeqMetaCASInit, SeqMetaCASTerm, NULL, pLog );
	if( Ret < 0 )	goto err;

	QueueInit( &MemCacheCtrl.mc_Worker );
	QueueInit( &MemCacheCtrl.mc_WorkerWB );

	Ret = FdEventCtrlCreate( &MemCacheCtrl.mc_FdEvClient );
	if( Ret < 0 )	goto err;

	MemCacheCtrl.mc_bWB			= bWB;
	MemCacheCtrl.mc_bCksum		= bCksum;
	MemCacheCtrl.mc_UsecCksum	= CksumMsec*1000;
	MemCacheCtrl.mc_SecExp		= ExpSec;
	MemCacheCtrl.mc_LenWB		= LenWB;

	/* 	Admin thread */
	MemCacheCtrl.mc_AdmAddr.sun_family	= AF_UNIX;
	sprintf( MemCacheCtrl.mc_AdmAddr.sun_path, PAXOS_MEMCACHE_ADM_FMT, Id );

	pthread_create( &MemCacheCtrl.mc_ThreadAdm, NULL, 
					ThreadAdm, &MemCacheCtrl );

	/* 	Worker thread */
	for( i = 0; i < Workers; i++ ) {
		pthread_create( &MemCacheCtrl.mc_WorkerThread[i], NULL, 
					ThreadWorker, &MemCacheCtrl );
	}

	/* 	Worker WB thread */
	for( i = 0; i < WorkerWBs; i++ ) {
		pthread_create( &MemCacheCtrl.mc_WorkerThread[i], NULL, 
					ThreadWorkerWB, &MemCacheCtrl );
	}
	/* Timer */
	TimerInit( &MemCacheCtrl.mc_Timer, 0, 0, 0, 0, Malloc, Free, 0, NULL );
	TimerEnable( &MemCacheCtrl.mc_Timer );
	TimerStart( &MemCacheCtrl.mc_Timer );

	/* FdLoop */
	Ret = FdEventLoop( &MemCacheCtrl.mc_FdEvClient, -1 );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

