/******************************************************************************
*
*  CIFS.c 	--- CIFS protocol
*
*  Copyright (C) 2016 triTech Inc. All Rights Reserved.
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

#include	"CNV.h"
#include	"CIFS.h"
#include	<iconv.h>

iconv_t	iConv;

int
UnicodeLen( uint16_t *p16 )
{
	int	Len = 0;

	while( *p16 ) {
		p16++;
		Len++;
	}
	return( Len*2 );
}


wchar_t*
CreateUnicodeToWchar( uint16_t *p16, size_t ByteLen )
{
	char	*pIn = (char*)p16;
	size_t	LenIn	= (size_t)ByteLen;
	char	*pOut, *pBuf;
	size_t	LenOut;

	ASSERT( !(ByteLen & 1) );	

	LenOut	= sizeof(wchar_t)*(ByteLen/2+1);
	pBuf = pOut = Malloc( LenOut );
	memset( pOut, 0, LenOut );

	iconv( iConv, &pIn, &LenIn, &pOut, &LenOut );

	return( (wchar_t*)pBuf );
}

void
DestroyUnicodeToWchar( wchar_t *p )
{
	Free( p );
}

int
IsSMBDialect( SMB_Dialect_t *pDialect, SMB_Dialect_t **ppNext )
{
	int	Len;

	if( pDialect->BufferFormat == 0x02 ) {
		Len = strlen( (char*)pDialect->DialectString.NullTerminated );
LOG(pLog, LogDBG, "[%s] Len=%d", 
(char*)pDialect->DialectString.NullTerminated, Len );
		*ppNext	= (SMB_Dialect_t*)((char*)pDialect + Len + 2);
		return( Len + 2 );
	} else {
		return( 0 );
	} 
}

/*
 *	Only CIFS part ( DataLen + CIFS )
 *		From Client and CIFS Server
 */
Msg_t*
CIFSRecvMsgByFd( int Fd, void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	uint32_t	*pDataLen;
	MsgVec_t	Vec;
	uint32_t	Total, Len;
	int	Ret;
	char	*pBuf;
	Msg_t	*pMsg;
	SMB_Header_t		*pHead;
	SMB_Parameters_t	*pParam;	
	SMB_Data_t			*pData;	
	uint8_t		WordCount;
	uint16_t	ByteCount, w16;

	pMsg	= MsgCreate( 5, fMalloc, fFree );

	pDataLen = (uint32_t*)Malloc( sizeof(*pDataLen) );

	Ret = ReadStream( Fd, (char*)pDataLen, sizeof(*pDataLen) );
	if( Ret < 0 ) {
LOG(pLog,LogDBG,"Ret=%d errno[%d]", Ret, errno );
		goto err;
	}

	MsgVecSet( &Vec,VEC_MINE,pDataLen,sizeof(*pDataLen),sizeof(*pDataLen));
	MsgAdd( pMsg, &Vec );

	Total	= ntohl( *pDataLen );
	if( Total & 0xff000000 ) {
		LOG(pLog,LogDBG,"INVALID Total=0x%x", Total );
		goto err;
	}

	pBuf	= (*fMalloc)( Total );

	ASSERT( pBuf );

	Ret = ReadStream( Fd, pBuf, Total );
	if( Ret < 0 ) {
		goto err1;
	}

	pHead	= (SMB_Header_t*)pBuf;

	MsgVecSet( &Vec, VEC_MINE, pBuf, Total, Total );
	MsgAdd( pMsg, &Vec );
	pMsg->m_pTag0	= (void*)(uintptr_t)Total;

	LOG(pLog,LogDBG,"Fd=%d Total=%d Cmd=0x%x", Fd, Total, pHead->h_Command );

	if( Total > sizeof(SMB_Header_t) ) {
		pParam	= (SMB_Parameters_t*)(pHead+1);
		WordCount	= pParam->p_WordCount;
		Len	= sizeof(*pParam) + WordCount*2;

		MsgVecSet( &Vec, VEC_DISABLED, pParam, Len, Len );
		MsgAdd( pMsg, &Vec );
		pMsg->m_pTag1	= (void*)(uintptr_t)WordCount;

		if( Total > sizeof(SMB_Header_t) + Len ) {
			pData	= (SMB_Data_t*)((char*)pParam+Len);
			memcpy( &w16, &pData->d_ByteCount, sizeof(w16) );
			ByteCount	= w16;//ntohs( w16 );
			Len	= sizeof(*pData) + ByteCount;

			MsgVecSet( &Vec, VEC_DISABLED, pData, Len, Len );
			MsgAdd( pMsg, &Vec );
			pMsg->m_pTag2	= (void*)(uintptr_t)ByteCount;
		}
	}
	return( pMsg );
err1:
err:
	MsgDestroy( pMsg );
LOG( pLog, LogDBG, "errno[%d] Fd[%d]", errno, Fd );
	return( NULL );
}

int
CIFSSendMsgByFd( int Fd, Msg_t *pMsg )
{
	int	Ret;

	Ret = MsgSendByFd( Fd, pMsg );
	return( Ret );
}

int
CIFSMetaHeadInsert( Msg_t *pMsg, MetaHead_t *pMetaHead )
{
	MsgVec_t	Vec;

	if( !pMetaHead ) {
		pMetaHead = (MetaHead_t*)Malloc( sizeof(*pMetaHead) );
		memset( pMetaHead, 0, sizeof(*pMetaHead) );
		pMetaHead->m_Length	= sizeof(*pMetaHead);
	}
	MsgVecSet( &Vec,VEC_MINE, pMetaHead, 
					pMetaHead->m_Length, pMetaHead->m_Length);
	MsgInsert( pMsg, &Vec );

	return( 0 );
}

Meta_Return_t*
CIFSMetaReturn( uint16_t ID )
{
	Meta_Return_t *pReturn;

	pReturn	= Malloc( sizeof(*pReturn) );
	memset( pReturn, 0, sizeof(*pReturn) );

	pReturn->r_Head.m_Length	= sizeof(*pReturn);
	pReturn->r_Head.m_Cmd		= META_ID_RETURN;
	pReturn->r_ReturnID			= ID;

	return( pReturn );
}

// PIDMIDlist
int
PIDMIDInit( PIDMIDHead_t *pHead )
{
	MtxInit( &pHead->h_Mtx );
	list_init( &pHead->h_Lnk );
	return( 0 );
}

int
PIDMIDTerm( PIDMIDHead_t *pHead )
{
	PIDMID_t	*pPIDMID;

	MtxLock( &pHead->h_Mtx );

	while( (pPIDMID = list_first_entry( &pHead->h_Lnk, PIDMID_t, pm_Lnk )) ) {

		Free( pPIDMID );
	}
	MtxUnlock( &pHead->h_Mtx );
	return( 0 );
}

PIDMID_t*
PIDMIDFind( PIDMIDHead_t *pHead, uint32_t PID, uint16_t MID, bool_t bCreate )
{
	PIDMID_t	*pPIDMID;

	MtxLock( &pHead->h_Mtx );

LOG( pLog, LogDBG, "FIND:PID=0x%x MID=0x%x", PID, MID );
list_for_each_entry( pPIDMID, &pHead->h_Lnk, pm_Lnk ) {
LOG( pLog, LogDBG, "pPIDMID=%p PID=0x%x MID=0x%x", 
pPIDMID, pPIDMID->pm_PID, pPIDMID->pm_MID );
}
	list_for_each_entry( pPIDMID, &pHead->h_Lnk, pm_Lnk ) {
		if( pPIDMID->pm_PID == PID && pPIDMID->pm_MID == MID ) {
			goto ret;
		}
	}
	if( bCreate ) {
		pPIDMID	= (PIDMID_t*)Malloc( sizeof(*pPIDMID) );

		memset( pPIDMID, 0, sizeof(*pPIDMID) );
		list_init( &pPIDMID->pm_Lnk );
		pPIDMID->pm_PID	= PID;
		pPIDMID->pm_MID	= MID;

		list_add_tail( &pPIDMID->pm_Lnk, &pHead->h_Lnk );
LOG( pLog, LogDBG, "CREATED:pPIDMID=%p", pPIDMID );
	} else {
		pPIDMID	= NULL;
	}
ret:
	MtxUnlock( &pHead->h_Mtx );
LOG( pLog, LogDBG, "pPIDMID=%p", pPIDMID );
	return( pPIDMID );
}

int
PIDMIDDelete( PIDMIDHead_t *pHead, PIDMID_t *pPIDMID )
{
	MtxLock( &pHead->h_Mtx );

LOG( pLog, LogDBG, "pPIDMID=%p PID=0x%x MID=0x%x", 
pPIDMID, pPIDMID->pm_PID, pPIDMID->pm_MID );
	list_del( &pPIDMID->pm_Lnk );

	Free( pPIDMID );

	MtxUnlock( &pHead->h_Mtx );

	return( 0 );
}

int
FIDCmp( void *pA, void *pB )
{
	PairFID_t	*pPairA = (PairFID_t*)pA;
	PairFID_t	*pPairB = (PairFID_t*)pB;

	return( pPairA->p_FID - pPairB->p_FID );
}

int
FIDMetaCmp( void *pA, void *pB )
{
	PairFID_t	*pPairA = (PairFID_t*)pA;
	PairFID_t	*pPairB = (PairFID_t*)pB;

	return( pPairA->p_FIDMeta - pPairB->p_FIDMeta );
}

int
FIDNameCmp( void *pA, void *pB )
{
	PairFID_t	*pPairA = (PairFID_t*)pA;
	PairFID_t	*pPairB = (PairFID_t*)pB;

	return( strcmp(pPairA->p_NameMD5, pPairB->p_NameMD5) );
}


IdMap_t*
IdMapCreate( int Size, void*(*fMalloc)(size_t), void(*fFree)(void*)  )
{
	int	Len;
	IdMap_t	*pIM;

	Len	= ( Size + 7 )/8;
	Len	+= sizeof(IdMap_t);
//printf("Len=%d idMap_t=%d\n", Len, sizeof(IdMap_t) );

	pIM	= (*fMalloc)( Len );
	memset( pIM, 0, Len );

	pIM->IM_fMalloc	= fMalloc;
	pIM->IM_fFree	= fFree;

	pIM->IM_Size	= Size;

	return( pIM );
}

void
IdMapDestroy( IdMap_t *pIM )
{
	(*pIM->IM_fFree)( pIM );
}

uint16_t
IdMapGet( IdMap_t *pIM )
{
	int	byte, bit;
	uint16_t	Prev, Id;

	Prev = pIM->IM_NextId;

	do {
		if( pIM->IM_NextId == 0 )	pIM->IM_NextId++;

		byte	= pIM->IM_NextId/8;
		bit		= pIM->IM_NextId - byte*8;
		if( (pIM->IM_aMap[byte] & 1<<bit ) ) {
			pIM->IM_NextId++;
			if( pIM->IM_NextId >= pIM->IM_Size )	pIM->IM_NextId = 0;
		} else {
			Id = pIM->IM_NextId++;
			if( pIM->IM_NextId >= pIM->IM_Size )	pIM->IM_NextId = 0;
			pIM->IM_aMap[byte]	|= 1<<bit;
			return( Id );
		}
//printf("Prev[%u] NextId[%u]\n", Prev, pIM->IM_NextId );
	} while( Prev != pIM->IM_NextId );
	return( 0 );
}

int
IdMapPut( IdMap_t *pIM, uint16_t Id )
{
	int	byte, bit;

	byte	= Id/8;
	bit		= Id - byte*8;

	if( !(pIM->IM_aMap[byte] & 1<<bit) )	return( -1 );

	pIM->IM_aMap[byte] &= ~(1<<bit);
	if( Id < pIM->IM_NextId )	pIM->IM_NextId	= Id;

	return( 0 );
}

int
IdMapSet( IdMap_t *pIM, uint16_t Id )
{
	int	byte, bit;

	byte	= Id/8;
	bit		= Id - byte*8;

	if( (pIM->IM_aMap[byte] & 1<<bit) )	return( -1 );

	pIM->IM_aMap[byte] |= (1<<bit);
	if( Id < pIM->IM_NextId )	pIM->IM_NextId	= Id;

	return( 0 );
}
