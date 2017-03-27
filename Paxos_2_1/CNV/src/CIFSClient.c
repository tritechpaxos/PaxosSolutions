/******************************************************************************
*
*  CIFSClient.c 	--- CIFS protocol
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
 *	çÏê¨			ìnï”ìTçF
 *	ééå±
 *	ì¡ãñÅAíòçÏå†	ÉgÉâÉCÉeÉbÉN
 */

//#define	_DEBUG_

#include	"CNV.h"
#include	"CIFS.h"
#include	<iconv.h>

CtrlCoC_t	CtrlCoC;

extern	iconv_t	iConv;

bool_t	bUnicode;

struct	sockaddr_in	Server;

PairFID_t *
_PairFIDGetCoC( ConCoC_t *pCon, char *pName, int NameLen )
{
	PairFID_t	*pPair;
	char	NameMD5[MD5_DIGEST_LENGTH*2 + 1];

	if( NameLen == 0 )	NameLen = strlen( pName );
	
	NameMD5[MD5_DIGEST_LENGTH*2] = 0;
	MD5HashStr( (unsigned char*)pName, NameLen, (unsigned char*)NameMD5 );

//	pPair	= (PairFID_t*)HashGet( &pCon->c_HashFIDName, NameMD5 );
//	if( pPair ) {
//		pPair->p_Cnt++;
//	} else {
		pPair = (PairFID_t*)Malloc( sizeof(*pPair) );
		memset( pPair, 0, sizeof(*pPair) );

		list_init( &pPair->p_Lnk );
		strcpy( pPair->p_NameMD5, NameMD5 );
		pPair->p_FIDMeta = IdMapGet( CtrlCoC.c_pIdMap ); 
		pPair->p_Cnt	= 1;

		list_add_tail( &pPair->p_Lnk, &pCon->c_LnkFID );
		HashPut( &pCon->c_HashFIDName, pPair->p_NameMD5, pPair );
		HashPut( &pCon->c_HashFIDMeta, 
						(void*)(uintptr_t)pPair->p_FIDMeta, pPair );
//	}
LOG(pLog, LogDBG, "NameLen=%d [%s] pPair=%p p_Cnt=%d", 
NameLen, pPair->p_NameMD5, pPair, pPair->p_Cnt );
	return( pPair );
}

int
_PairFIDPutCoC( ConCoC_t *pCon, uint16_t FIDMeta )
{
	PairFID_t	*pPair;

	pPair = (PairFID_t*)HashGet( &pCon->c_HashFIDMeta, 
								(void*)(uintptr_t)FIDMeta );

	if( !pPair ) {
		errno	= ENOENT;
		goto err;
	}
	if( --pPair->p_Cnt == 0 ) {

		IdMapPut( CtrlCoC.c_pIdMap, pPair->p_FIDMeta ); 

		HashRemove( &pCon->c_HashFIDName, pPair->p_NameMD5 );
		HashRemove( &pCon->c_HashFIDMeta, (void*)(uintptr_t)pPair->p_FIDMeta );
		list_del( &pPair->p_Lnk );		

		Free( pPair );
	}
	return( 0 );
err:
	return( -1 );
}


int
CIFSRequestByMsg( FdEvCoC_t *pFdEvCoC, Msg_t *pMsg, bool_t *pUpdate )
{
	ConCoC_t	*pCon = container_of( pFdEvCoC, ConCoC_t, c_FdEvCoC );
	SMB_Header_t		*pHead;
	SMB_Parameters_t	*pParam = NULL;	
	SMB_Data_t			*pData = NULL;	
	uint32_t	Total;
	uint8_t		WordCount;
	uint16_t	ByteCount;
	char		*pC;
	int			len;
	SMB_Parameters_Setup_Req_t	*pReqSetup;
	char buf[1024];int i;
	uint32_t	PID;
	PIDMID_t	*pPIDMID;
	bool_t		bMeta = FALSE;
	int		Ret =0;

	Total		= (uintptr_t)pMsg->m_pTag0;
	WordCount	= (uintptr_t)pMsg->m_pTag1;
	ByteCount	= (uintptr_t)pMsg->m_pTag2;

	pHead	= (SMB_Header_t*)pMsg->m_aVec[SMB_MSG_HEAD].v_pStart;
	if( WordCount )	pParam	= (SMB_Parameters_t*)
								pMsg->m_aVec[SMB_MSG_PARAM].v_pStart;
	if( ByteCount )	pData	= (SMB_Data_t*)
								pMsg->m_aVec[SMB_MSG_DATA].v_pStart;

LOG( pLog, LogDBG, "=>REQ:Cmd=0x%x "
"Flags2=0x%x Uni=%d Total=%d Word=%d Byte=%d pHead=%p pParm=%p pData=%p", 
pHead->h_Command, pHead->h_Flags2, pHead->h_Flags2 & SMB_FLAGS2_UNICODE,
Total, WordCount, ByteCount, pHead, pParam, pData );
LOG( pLog, LogDBG, 
"PIDHigh[0x%x] PIDLow[0x%x] MID[0x%x] UID[0x%x] TID[0x%x]",
pHead->h_PIDHigh, pHead->h_PIDLow, pHead->h_MID, pHead->h_TID );

	if( CtrlCoC.c_Ctrl.c_Snoop == SNOOP_OFF )	goto skip1;

	PID	= (pHead->h_PIDHigh<<16)&0xffff0000;
	PID	|= pHead->h_PIDLow;

	pPIDMID	= PIDMIDFind( &pCon->c_PIDMID, PID, pHead->h_MID, TRUE );

	switch( pHead->h_Command ) {
		case SMB_COM_NEGOTIATE:
		{
			Meta_NT_LM_t	*pMeta;
			SMB_Dialect_t	*pDialect, *pDialectNext;
	
#ifdef	ZZZ
			pC	= (char*)pData->d_Bytes;
			while( ByteCount > 0 ) {
				ASSERT( (*pC & 0xff) == 0x02 );
				pC++;
				len	= strlen( pC );
				Log(pLog,LogDBG,"[%s]", pC );
				ByteCount -= len + 2;
				pC		+= len + 1;
			}
#else
			pDialect	= (SMB_Dialect_t*)pData->d_Bytes;
			while( ByteCount > 0 ) {
				IsSMBDialect( pDialect, &pDialectNext );
				ByteCount -= (char*)pDialectNext - (char*)pDialect;
				pDialect = pDialectNext;
			}
#endif
			pMeta	= (Meta_NT_LM_t*)Malloc( sizeof(*pMeta) );
			pMeta->m_Head.m_Length	= sizeof(*pMeta);
			pMeta->m_SessionKey	= IdMapGet( CtrlCoC.c_pIdMap ); 
			bMeta	= TRUE;
			CIFSMetaHeadInsert( pMsg, &pMeta->m_Head );
			pCon->c_SessionKeyMeta	= pMeta->m_SessionKey;
			break;
		}

		case SMB_COM_SESSION_SETUP_ANDX:
		{	uint8_t *pOEM, *pUni;char *pAccount, *pDomain, *pOS, *pLM;
			uint16_t LenOEM, LenUni, Off;
			Meta_Setup_t	*pMeta;

			pReqSetup	= (SMB_Parameters_Setup_Req_t*)pParam->p_Words;

			LOG(pLog, LogDBG, "AndXCommand[0x%x]", pReqSetup->p_AndXCommand);
			ASSERT( pReqSetup->p_AndXReserved == 0 );
			LOG(pLog, LogDBG, "AndXOffset[%d]", pReqSetup->p_AndXOffset);
			LOG(pLog, LogDBG, "MaxBufferSize[%d]", pReqSetup->p_MaxBufferSize);
			LOG(pLog, LogDBG, "MaxMpxCount[%d]", pReqSetup->p_MaxMpxCount);
			LOG(pLog, LogDBG, "VcNumber[%d]", pReqSetup->p_VcNumber);
			LOG(pLog, LogDBG, "SessionKey[0x%x]", pReqSetup->p_SessionKey);
			//ASSERT( pReqSetup->p_Reserved == 0 );
			LOG(pLog, LogDBG, "Capabilities[0x%x]", pReqSetup->p_Capabilities);

			LenOEM	= pReqSetup->p_OEMPasswordLen;
			LenUni	= pReqSetup->p_UnicodePasswordLen;

			LOG( pLog, LogDBG, "LenOEM=%d LenUni=%d", LenOEM, LenUni);

			pOEM	= &pData->d_Bytes[0];
			pUni	= &pData->d_Bytes[LenOEM];
		
#ifdef	ZZZ
			if( LenOEM ) {
				memset( buf, 0, sizeof(buf) );
				for( i = 0; i < LenOEM; i++ ) {
					sprintf( &buf[i*2], "%.2x", (char)pOEM[i] );
				}
				LOG( pLog, LogDBG, "OEM[%s]", buf );
/***
				if( bUnicode ) {
					wchar_t	*pBuf;

					pBuf = CreateUnicodeToWchar( (uint16_t*)pOEM, LenOEM );

					LOG( pLog, LogDBG,"UniOEM[%S]", pBuf );

					DestroyUnicodeToWchar( pBuf );
				} else {
					LOG( pLog, LogDBG, "OEM-null[%s]", pOEM );
				}
***/
			}
#endif
			if( LenUni ) {

				char	*pOut;
				size_t	outLen;

				memset( buf, 0, sizeof(buf) );

				pOut	= (char*)buf;
				outLen	= sizeof(buf);

				iconv( iConv, (char**)&pUni, (size_t*)&LenUni, &pOut, &outLen );

				LOG( pLog, LogDBG, "Uni[%S]", buf );
			}

			Off	= LenOEM + LenUni;	
			pAccount	= (char*)&pData->d_Bytes[Off];
			if((uintptr_t)pAccount & 1 )	++pAccount;

			pDomain	= pAccount + strlen(pAccount) + 1;	
			pOS	= pDomain + strlen(pDomain) + 1;	
			pLM	= pOS + strlen(pOS) + 1;	

			LOG(pLog, LogDBG, "Off=%d Account[%s] Domain[%s] OS[%s] LM[%s]",
					Off, pAccount, pDomain, pOS, pLM );

			pMeta	= (Meta_Setup_t*)Malloc( sizeof(*pMeta) );
			pMeta->s_Head.m_Length	= sizeof(*pMeta);
			pMeta->s_UID	= IdMapGet( CtrlCoC.c_pIdMap ); 
			bMeta	= TRUE;
			CIFSMetaHeadInsert( pMsg, &pMeta->s_Head );

			pCon->c_UIDMeta	= pMeta->s_UID;
LOG(pLog, LogDBG,"UIDMeta[0x%x]", pMeta->s_UID );
			break;
		}

		case SMB_COM_TREE_CONNECT_ANDX:
		{	uint8_t *pPass;char *pPath, *pService;
			SMB_Parameters_Connect_Req_t	ReqCon;
			Meta_Connect_t	*pMeta;

			memcpy( &ReqCon, pParam->p_Words, sizeof(ReqCon) );

			LOG(pLog, LogDBG, "AndXCommand[0x%x]", ReqCon.c_AndXCommand);
			ASSERT( ReqCon.c_AndXReserved == 0 );
			LOG(pLog, LogDBG, "AndXOffset[%d]", ReqCon.c_AndXOffset);
			LOG(pLog, LogDBG, "Flags[0x%x]", ReqCon.c_Flags);
			LOG(pLog, LogDBG, "PasswordLen[%d]", ReqCon.c_PasswordLen );

			pPass	= pData->d_Bytes;
LOG(pLog,LogDBG,"Bytes[0]=0x%x",*pPass );

			pPath	= (char*)&pData->d_Bytes[ReqCon.c_PasswordLen];

			if( pHead->h_Flags2 & SMB_FLAGS2_UNICODE ) {

				int	Len; wchar_t	*pBuf;

LOG( pLog, LogDBG,"pPath=%p", pPath );
				if((uintptr_t)pPath & 1 )	++pPath;

				Len = UnicodeLen( (uint16_t*)pPath ) + 2;

				pBuf = CreateUnicodeToWchar( (uint16_t*)pPath, Len );

				pService	= pPath + Len;

				LOG( pLog, LogDBG,"pPath=%p:uni[%S] pService[%s]", 
							pPath, pBuf, pService );
				DestroyUnicodeToWchar( pBuf );

			} else {
				pPath	= (char*)&pData->d_Bytes[ReqCon.c_PasswordLen];
				pService	= pPath + strlen(pPath) + 1;
LOG(pLog,LogDBG, "pPath=%p[%s] pServer=%p strlen=%d",
pPath,pPath,pService,strlen(pPath));
			}

			memset( buf, 0, sizeof(buf) );
			for( i = 0; i < ByteCount; i++ ) {
				sprintf( &buf[i*2], "%.2x", (char)pData->d_Bytes[i] );
			}
			LOG( pLog, LogDBG, "%p:Bytes[%s]", pData->d_Bytes, buf );

			pMeta	= (Meta_Connect_t*)Malloc( sizeof(*pMeta) );
			pMeta->c_Head.m_Length	= sizeof(*pMeta);
			pMeta->c_TID	= IdMapGet( CtrlCoC.c_pIdMap ); 
			bMeta	= TRUE;
			CIFSMetaHeadInsert( pMsg, &pMeta->c_Head );

			pCon->c_TIDMeta	= pMeta->c_TID;
LOG(pLog, LogDBG,"TIDMeta[0x%x]", pMeta->c_TID );
			break;
		}

		case SMB_COM_NT_CREATE_ANDX:
		{	SMB_Parameters_CreateAndX_Req_t	*pReq;
			char	*pFileName;
			Meta_Create_t *pMeta;
//			PairFID_t	*pPair;

			pReq = (SMB_Parameters_CreateAndX_Req_t*)pParam->p_Words;
			LOG(pLog, LogDBG, "AndXCommand[0x%x]", pReq->c_AndXCommand);
			ASSERT( pReq->c_AndXReserved == 0 );
			LOG(pLog, LogDBG, "AndXOffset[%d]", pReq->c_AndXOffset);
			ASSERT( pReq->c_Reserved == 0 );
			LOG(pLog, LogDBG, "NameLength[%d]", pReq->c_NameLength);
			LOG(pLog, LogDBG, "Flags[0x%x]", pReq->c_Flags);
			LOG(pLog, LogDBG, "RootDirectoryFID[0x%x]", 
								pReq->c_RootDirectoryFID);
			LOG(pLog, LogDBG, "DesiredAccess[0x%x]", pReq->c_DesiredAccess);
			LOG(pLog, LogDBG, "AllocationSize[%"PRIu64"]", 
								pReq->c_AllocationSize);
			LOG(pLog, LogDBG, "ExtFileAttributes[0x%x]", 
								pReq->c_ExtFileAttributes);
			LOG(pLog, LogDBG, "ShareAccess[0x%x]", pReq->c_ShareAccess);
			LOG(pLog, LogDBG, "CreateDisposition[0x%x]", 
								pReq->c_CreateDisposition);
			LOG(pLog, LogDBG, "CreateOptions[0x%x]", 
								pReq->c_CreateOptions);
			LOG(pLog, LogDBG, "ImpersonationLevel[0x%x]", 
								pReq->c_ImpersonationLevel);
			LOG(pLog, LogDBG, "SecurityFlags[0x%x]", 
								pReq->c_SecurityFlags);

			pFileName	= (char*)pData->d_Bytes;

			if( pHead->h_Flags2 & SMB_FLAGS2_UNICODE ) {

				int	Len; wchar_t	*pBuf;

LOG( pLog, LogDBG,"pFileName=%p", pFileName );
				if((uintptr_t)pFileName & 1 )	++pFileName;

				Len = UnicodeLen( (uint16_t*)pFileName ) + 2;

				pBuf = CreateUnicodeToWchar( (uint16_t*)pFileName, Len );

				LOG( pLog, LogDBG,"pFilename=%p(Len=%d):uni[%S]", 
							pFileName, Len, pBuf );
				DestroyUnicodeToWchar( pBuf );
			} else {
				LOG( pLog, LogDBG,"pFilename[%s]", pFileName );
			}
			pMeta	= (Meta_Create_t*)Malloc( sizeof(*pMeta) );
			memset( pMeta, 0, sizeof(*pMeta) );
			pMeta->c_Head.m_Length	= sizeof(*pMeta);
#ifndef	ZZZ
			pMeta->c_FID	= IdMapGet( CtrlCoC.c_pIdMap ); 
#else
			MtxLock( &pCon->c_Mtx );

			pPair	= _PairFIDGetCoC( pCon, pFileName, pReq->c_NameLength );

			pMeta->c_FID	= pPair->p_FIDMeta;

			MtxUnlock( &pCon->c_Mtx );
#endif
			bMeta	= TRUE;
			CIFSMetaHeadInsert( pMsg, &pMeta->c_Head );

			pCon->c_FIDMeta	= pMeta->c_FID;
LOG(pLog, LogDBG,"FIDMeta[0x%x]", pMeta->c_FID );
			break;
		}

		case SMB_COM_CLOSE:
		{	SMB_Parameters_Close_Req_t	*pReq;

			pReq	= (SMB_Parameters_Close_Req_t*)pParam->p_Words;
			LOG( pLog, LogDBG,"FID[0x%x]", pReq->c_FID ); 
			LOG( pLog, LogDBG,"LastTimeModified[%u]", 
					pReq->c_LastTimeModified ); 

			pCon->c_FIDMeta	= pReq->c_FID;
			break;
		}

		case SMB_COM_TRANSACTION2:
		case SMB_COM_TRANSACTION:
		{	SMB_Parameters_Transaction2_Req_t	*pReq;
			int	i; uint8_t	Name; void *pTrans2Param, *pTrans2Data;

			pReq	= (SMB_Parameters_Transaction2_Req_t*)pParam->p_Words;
			ASSERT( WordCount == (sizeof(SMB_Parameters_Transaction2_Req_t)/2
							+ pReq->t_SetupCount) );

			LOG( pLog, LogDBG,"TotalParameterCount[%u]", 
						pReq->t_TotalParameterCount );
			LOG( pLog, LogDBG,"TotalDataCount[%u]", 
						pReq->t_TotalDataCount );
			LOG( pLog, LogDBG,"MaxParameterCount[%u]", 
						pReq->t_MaxParameterCount );
			LOG( pLog, LogDBG,"MaxDataCount[%u]", pReq->t_MaxDataCount );
			LOG( pLog, LogDBG,"MaxSetupCount[%u]", pReq->t_MaxSetupCount );
			ASSERT( pReq->t_Reserved1 == 0 );
			LOG( pLog, LogDBG,"Flags[0x%x]", pReq->t_Flags );
			LOG( pLog, LogDBG,"Timeout[%ums]", pReq->t_Timeout );
			ASSERT( pReq->t_Reserved2 == 0 );
			LOG( pLog, LogDBG,"ParameterCount[%u]", pReq->t_ParameterCount );
			LOG( pLog, LogDBG,"ParameterOffset[%u]", pReq->t_ParameterOffset );
			LOG( pLog, LogDBG,"DataCount[%u]", pReq->t_DataCount );
			LOG( pLog, LogDBG,"DataOffset[%u]", pReq->t_DataOffset );
			LOG( pLog, LogDBG,"SetupCount[%u]", pReq->t_SetupCount );
			ASSERT( pReq->t_Reserved3 == 0 );
			Name	= pData->d_Bytes[0];
			ASSERT( Name == 0 );

			pPIDMID->pm_State					= TransactionPrimaryRequest;
			pPIDMID->pm_TotalParameterCountReq	= pReq->t_TotalParameterCount;
			pPIDMID->pm_TotalDataCountReq		= pReq->t_TotalDataCount;

			pPIDMID->pm_TotalParameterCountReq	-= pReq->t_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		-= pReq->t_DataCount;

			if( pPIDMID->pm_TotalParameterCountReq 
						|| pPIDMID->pm_TotalDataCountReq ) {

				ASSERT( pPIDMID->pm_State == TransmittedAllRequests );

				pPIDMID->pm_State	= TransactionPrimaryRequest;

LOG(pLog, LogDBG, "PID[0x%x] MID[0x%x] -> TransactionPrimaryRequest",
				pPIDMID->pm_PID, pPIDMID->pm_MID );
			}
			pTrans2Param	= (char*)pHead + pReq->t_ParameterOffset;
			pTrans2Data		= (char*)pHead + pReq->t_DataOffset;

			for( i = 0; i < pReq->t_SetupCount; i++ ) {
				LOG( pLog, LogDBG,"%d:Setup[0x%x]", i, pReq->t_Setup[i] );
				switch( pReq->t_Setup[i] ) {
					case TRANS2_OPEN2:
					{
						Meta_Create_t	*pMeta;
						PairFID_t		*pPair;
						SMB_Parameters_Trans2_Open_Req_t	*pParam;

						pParam	= (SMB_Parameters_Trans2_Open_Req_t*)
									pTrans2Param;

						pMeta	= (Meta_Create_t*)Malloc( sizeof(*pMeta) );
						pMeta->c_Head.m_Length	= sizeof(*pMeta);

						MtxLock( &pCon->c_Mtx );

						pPair	= _PairFIDGetCoC( pCon, 
									(char*)pParam->o_FileName, 0 );

						pMeta->c_FID	= pPair->p_FIDMeta;

						MtxUnlock( &pCon->c_Mtx );

						bMeta	= TRUE;
						CIFSMetaHeadInsert( pMsg, &pMeta->c_Head );

						pCon->c_FIDMeta	= pMeta->c_FID;
LOG(pLog, LogDBG,"FIDMeta[0x%x]", pMeta->c_FID );
						break;
					}
					case	TRANS2_GET_DFS_REFERRAL:
					{
						int	Len; wchar_t	*pBuf;
						SMB_Parameters_Trans2_Get_DFS_Referral_Req_t	*pParam;

						pParam	=(SMB_Parameters_Trans2_Get_DFS_Referral_Req_t*)
									pTrans2Param;

LOG( pLog, LogDBG, "MaxReferralLevel[%d]", pParam->dr_MaxReferralLevel );

						Len = UnicodeLen( pParam->dr_RequestFileName ) + 2;

						pBuf = CreateUnicodeToWchar( 
									pParam->dr_RequestFileName, Len );

						LOG( pLog, LogDBG,"uni[%S]", pBuf );

						DestroyUnicodeToWchar( pBuf );

						break;
					}
				}
			}
			break;
		}
		case SMB_COM_NT_TRANSACT:
		{	SMB_Parameters_NT_Transact_Req_t	*pReq;
			int	i; uint8_t	Name; void *pTransParam, *pTransData;

			pReq	= (SMB_Parameters_NT_Transact_Req_t*)pParam->p_Words;

			LOG( pLog, LogDBG,"MaxSetupCount[%u]", 
						pReq->n_MaxSetupCount );
			LOG( pLog, LogDBG,"TotalParameterCount[%u]", 
						pReq->n_TotalParameterCount );
			LOG( pLog, LogDBG,"TotalDataCount[%u]", 
						pReq->n_TotalDataCount );
			LOG( pLog, LogDBG,"MaxParameterCount[%u]", 
						pReq->n_MaxParameterCount );
			LOG( pLog, LogDBG,"MaxDataCount[%u]", pReq->n_MaxDataCount );
			LOG( pLog, LogDBG,"ParameterCount[%u]", pReq->n_ParameterCount );
			LOG( pLog, LogDBG,"ParameterOffset[%u]", pReq->n_ParameterOffset );
			LOG( pLog, LogDBG,"DataCount[%u]", pReq->n_DataCount );
			LOG( pLog, LogDBG,"DataOffset[%u]", pReq->n_DataOffset );
			LOG( pLog, LogDBG,"SetupCount[%u]", pReq->n_SetupCount );
			LOG( pLog, LogDBG,"Function[0x%x]", pReq->n_Function );

			pPIDMID->pm_State					= TransactionPrimaryRequest;
			pPIDMID->pm_TotalParameterCountReq	= pReq->n_TotalParameterCount;
			pPIDMID->pm_TotalDataCountReq		= pReq->n_TotalDataCount;

			pPIDMID->pm_TotalParameterCountReq	-= pReq->n_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		-= pReq->n_DataCount;

			if( pPIDMID->pm_TotalParameterCountReq 
						|| pPIDMID->pm_TotalDataCountReq ) {

				ASSERT( pPIDMID->pm_State == TransmittedAllRequests );

				pPIDMID->pm_State	= TransactionPrimaryRequest;

LOG(pLog, LogDBG, "PID[0x%x] MID[0x%x] -> TransactionPrimaryRequest",
				pPIDMID->pm_PID, pPIDMID->pm_MID );
			}

			switch( pReq->n_Function ){
			case NT_TRANSACT_CREATE:
				break;
			case NT_TRANSACT_IOCTL:
				break;
			case NT_TRANSACT_SET_SECURITY_DESC:
				break;
			case NT_TRANSACT_NOTIFY_CHANGE:
			{
				NT_Transact_Notify_Change_Setup_t	*pNotify;

				pNotify	= (NT_Transact_Notify_Change_Setup_t*)pReq->n_Setup;

LOG(pLog, LogDBG, "CompletionFilter=0x%x", pNotify->n_CompletionFilter );
LOG(pLog, LogDBG, "FID=0x%x", pNotify->n_FID );
LOG(pLog, LogDBG, "WatchTree=0x%x", pNotify->n_WatchTree );
				break;
			}
			case NT_TRANSACT_RENAME:
				break;
			case NT_TRANSACT_QUERY_SECURITY_DESC:
				break;
			}
			break;
		}
		case SMB_COM_TRANSACTION2_SECONDARY:
		{	SMB_Parameters_Transaction2_Secondary_Req_t	*pReq;
			int	i; uint8_t	Name; void *pTrans2Param, *pTrans2Data;

			pReq	= (SMB_Parameters_Transaction2_Secondary_Req_t*)
									pParam->p_Words;

			LOG( pLog, LogDBG,"TotalParameterCount[%u]", 
						pReq->s_TotalParameterCount );
			LOG( pLog, LogDBG,"TotalDataCount[%u]", pReq->s_TotalDataCount );
			LOG( pLog, LogDBG,"ParameterCount[%u]", pReq->s_ParameterCount );
			LOG( pLog, LogDBG,"ParameterOffset[%u]", pReq->s_ParameterOffset );
			LOG( pLog, LogDBG,"DataCount[%u]", pReq->s_DataCount );
			LOG( pLog, LogDBG,"DataOffset[%u]", pReq->s_DataOffset );
			LOG( pLog, LogDBG,"DataDisplacement[%u]", pReq->s_DataDisplacement);
			LOG( pLog, LogDBG,"FID[0x%x]", pReq->s_FID );

//			pPIDMID->pm_TotalParameterCountReq	= pReq->s_TotalParameterCount;
//			pPIDMID->pm_TotalDataCountReq		= pReq->s_TotalDataCount;

			pPIDMID->pm_TotalParameterCountReq	-= pReq->s_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		-= pReq->s_DataCount;

			break;
		}

		case SMB_COM_WRITE_ANDX:
		{	SMB_Parameters_WriteAndX_Req_t	*pReq;

			pReq	= (SMB_Parameters_WriteAndX_Req_t*)pParam->p_Words;

			ASSERT( pReq->w_AndXCommand == 0xff );
			ASSERT( pReq->w_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%u]", pReq->w_AndXOffset );
			LOG(pLog,LogDBG,"FID[0x%x]", pReq->w_FID );
			LOG(pLog,LogDBG,"Offset[%u]", pReq->w_Offset );
			LOG(pLog, LogDBG,"Timeout[%ums]", pReq->w_Timeout );
			LOG(pLog,LogDBG,"WriteMode[0x%x]", pReq->w_WriteMode );
			LOG(pLog,LogDBG,"Remaining[%u]", pReq->w_Remaining );
			ASSERT( pReq->w_Reserved == 0 );
			LOG(pLog,LogDBG,"DataLength[%u]", pReq->w_DataLength );
			LOG(pLog,LogDBG,"DataOffset[%u]", pReq->w_DataOffset );
			break;
		}

		case SMB_COM_READ_ANDX:
			{SMB_Parameters_ReadAndX_Req_t	*pReq;

			pReq	= (SMB_Parameters_ReadAndX_Req_t*)pParam->p_Words;

			ASSERT( pReq->r_AndXCommand == 0xff );
			ASSERT( pReq->r_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%u]", pReq->r_AndXOffset );
			LOG(pLog,LogDBG,"FID[0x%x]", pReq->r_FID );
			LOG(pLog,LogDBG,"Offset[%u]", pReq->r_Offset );
			LOG(pLog,LogDBG,"MaxCountOfBytesToReturn[%u]", 
								pReq->r_MaxCountOfBytesToReturn );
			LOG(pLog,LogDBG,"MinCountOfBytesToReturn[%u]", 
								pReq->r_MinCountOfBytesToReturn );
			LOG(pLog, LogDBG,"Timeout[%ums]", pReq->r_Timeout );
			LOG(pLog,LogDBG,"Remaining[%u]", pReq->r_Remaining );
			}
			break;

		default:	
			break;
	}
skip1:
	if( !bMeta )	CIFSMetaHeadInsert( pMsg, NULL );
	return( Ret );
}

static int
CIFSReplyByMsg( FdEvCoC_t *pFdEvCoC, Msg_t *pMsg, int Status )
{
	ConCoC_t	*pCon = container_of( pFdEvCoC, ConCoC_t, c_FdEvCoC );
	SMB_Header_t		*pHead;
	SMB_Parameters_t	*pParam = NULL;	
	SMB_Data_t			*pData = NULL;	
	uint32_t	Total, Len;
	uint8_t		WordCount;
	uint16_t	ByteCount, w16;
	int			Ret;
	SMB_Parameters_NT_LM_Res_t	*pParamNT_LM;
	uint32_t	PID;
	PIDMID_t	*pPIDMID;
	int			Psuedo = 0;
	MetaHead_t	*pMetaHead;
	uint32_t	*pDataLen;
	MsgVec_t	Vec;

	pMetaHead	= (MetaHead_t*)MsgFirst( pMsg );

	pDataLen	= (uint32_t*)((char*)pMetaHead + pMetaHead->m_Length);
	Total	= ntohl( *pDataLen );
	pMsg->m_pTag0	= (void*)(uintptr_t)Total;

	pHead	= (SMB_Header_t*)((char*)pDataLen + sizeof(*pDataLen));
	MsgVecSet( &Vec, VEC_DISABLED, pHead, Total, Total );
	MsgAdd( pMsg, &Vec );

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
			ByteCount	= w16;
			Len	= sizeof(*pData) + ByteCount;

			MsgVecSet( &Vec, VEC_DISABLED, pData, Len, Len );
			MsgAdd( pMsg, &Vec );
			pMsg->m_pTag2	= (void*)(uintptr_t)ByteCount;
		}
	}

	Total		= (uintptr_t)pMsg->m_pTag0;
	WordCount	= (uintptr_t)pMsg->m_pTag1;
	ByteCount	= (uintptr_t)pMsg->m_pTag2;

LOG( pLog, LogDBG, 
"<=REPLY:Meta=0x%x Cmd=0x%x Error(0x%x:0x%x) Flags=0x%x Flags2=0x%x", 
pMetaHead->m_Cmd,
pHead->h_Command, pHead->h_Status.e_ErrorClass, pHead->h_Status.e_ErrorCode,
pHead->h_Flags, pHead->h_Flags2 );
LOG( pLog, LogDBG, 
"PIDHigh[0x%x] PIDLow[0x%x] MID[0x%x] UID[0x%x] TID[0x%x]",
pHead->h_PIDHigh, pHead->h_PIDLow, pHead->h_MID, pHead->h_TID );

	if( CtrlCoC.c_Ctrl.c_Snoop == SNOOP_OFF )	goto skip1;

	PID	= (pHead->h_PIDHigh<<16)&0xffff0000;
	PID	|= pHead->h_PIDLow;

	pPIDMID	= PIDMIDFind( &pCon->c_PIDMID, PID, pHead->h_MID, FALSE );
	ASSERT( pPIDMID );

	switch( pMetaHead->m_Cmd ) {
		case META_ID_RETURN:
		{	 Meta_Return_t	*pMetaReturn = (Meta_Return_t*)pMetaHead;
LOG(pLog, LogDBG,"Put:ReturnID[0x%x]", pMetaReturn->r_ReturnID );
			_PairFIDPutCoC( pCon, pMetaReturn->r_ReturnID );
			break;
		}
	}
	if( pHead->h_Status.e_ErrorClass ) {
		switch( pHead->h_Command ) {
			case SMB_COM_SESSION_SETUP_ANDX:
LOG(pLog, LogDBG,"Put:UIDMeta[0x%x]", pCon->c_UIDMeta );
				if( pCon->c_UIDMeta ) {
					IdMapPut( CtrlCoC.c_pIdMap, pCon->c_UIDMeta );
					pCon->c_UIDMeta	= 0;
				}
				break;

			case SMB_COM_TREE_CONNECT_ANDX:
LOG(pLog, LogDBG,"Put:TIDMeta[0x%x]", pCon->c_TIDMeta );
				if( pCon->c_TIDMeta ){
					IdMapPut( CtrlCoC.c_pIdMap, pCon->c_TIDMeta );
					pCon->c_TIDMeta	= 0;
				}
				break;

			case SMB_COM_NT_CREATE_ANDX:
LOG(pLog, LogDBG,"Put:FIDMeta[0x%x]", pCon->c_FIDMeta );
				if( pCon->c_FIDMeta ){

					MtxLock( &pCon->c_Mtx );

					_PairFIDPutCoC( pCon, pCon->c_FIDMeta );

					MtxUnlock( &pCon->c_Mtx );

					pCon->c_FIDMeta	= 0;
				}
				break;
		}
		goto ret;
	}

	/*
	 *	Check Interim response
	 */
	if( pPIDMID->pm_State == TransactionPrimaryRequest
			&& WordCount == 0 && ByteCount == 0 ) {

		pPIDMID->pm_State = ReceivedInterimResponse;

		goto skip1;
	}
	switch( pHead->h_Command ) {
		case SMB_COM_NEGOTIATE:
			{int ChallengeLength;char *pDomain;
			ASSERT( WordCount >= 1 );
			LOG(pLog,LogDBG,"WordCount=%d DialectIndex[%d]", 
					WordCount, pParam->p_Words[0] );

			pParamNT_LM = (SMB_Parameters_NT_LM_Res_t*)pParam->p_Words;
			LOG(pLog,LogDBG,"MaxBufferSize[%d]", pParamNT_LM->p_MaxBufferSize );
			LOG(pLog,LogDBG,"SecurityMode[0x%x]", pParamNT_LM->p_SecurityMode );
			LOG(pLog,LogDBG,"MaxMpxCount[%d]", pParamNT_LM->p_MaxMpxCount );
			LOG(pLog,LogDBG,"MaxNumberVcs[%d]", pParamNT_LM->p_MaxNumberVcs );
			LOG(pLog,LogDBG,"MaxBufferSize[%d]", pParamNT_LM->p_MaxBufferSize );
			LOG(pLog,LogDBG,"MaxRawSize[%d]", pParamNT_LM->p_MaxRawSize );
			LOG(pLog,LogDBG,"SessionKey[0x%x]", pParamNT_LM->p_SessionKey );
			LOG(pLog,LogDBG,"Capabilities[0x%x]", pParamNT_LM->p_Capabilities );
			LOG(pLog,LogDBG,"Unicode=0x%x", 
							pParamNT_LM->p_Capabilities & CAP_UNICODE );
			if( pParamNT_LM->p_Capabilities & CAP_UNICODE )	bUnicode = TRUE;
			ChallengeLength	= pParamNT_LM->p_ChallengeLength;
			ASSERT( WordCount == 0x11 );
			pDomain	= (char*)&pData->d_Bytes[ChallengeLength];
			LOG(pLog,LogDBG,"ChallengeLength=%d Domain[%s]", 
							ChallengeLength, pDomain );
			}
			break;

		case SMB_COM_SESSION_SETUP_ANDX:
			{ SMB_Parameters_Setup_Res_t	*pRes;
			char	*pNativeOS, *pNativeLanMan, *pPrimaryDomain;
			int		Len;

			pRes	= (SMB_Parameters_Setup_Res_t*)pParam->p_Words;
			LOG(pLog,LogDBG,"AndXCommand[0x%x]", pRes->p_AndXCommand );
			ASSERT( pRes->p_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%d]", pRes->p_AndXOffset );
			LOG(pLog,LogDBG,"Action[0x%x]", pRes->p_Action );

			pNativeOS	= (char*)pData->d_Bytes;

			if( pHead->h_Flags2 & SMB_FLAGS2_UNICODE ) {

				wchar_t	*pBuf;

				if( ((uintptr_t)pNativeOS) & 1 )	++pNativeOS;

				Len = UnicodeLen( (uint16_t*)pNativeOS ) + 2;
				pBuf = CreateUnicodeToWchar( (uint16_t*)pNativeOS, Len );
				LOG( pLog, LogDBG,"pNativeOS=%p:uni[%S]", pNativeOS, pBuf );
				DestroyUnicodeToWchar( pBuf );

				pNativeLanMan = pNativeOS + Len;

				Len = UnicodeLen( (uint16_t*)pNativeLanMan ) + 2;
				pBuf = CreateUnicodeToWchar( (uint16_t*)pNativeLanMan, Len );
				LOG( pLog, LogDBG,"pNativeLanMan=%p:uni[%S]", 
									pNativeLanMan, pBuf );
				DestroyUnicodeToWchar( pBuf );

				pPrimaryDomain = pNativeLanMan + Len;

				Len = UnicodeLen( (uint16_t*)pPrimaryDomain ) + 2;
				pBuf = CreateUnicodeToWchar( (uint16_t*)pPrimaryDomain, Len );
				LOG( pLog, LogDBG,"pPrimaryDomain=%p:uni[%S]", 
									pPrimaryDomain, pBuf );
				DestroyUnicodeToWchar( pBuf );

			} else {
				LOG( pLog, LogDBG, "pNativeOS[%s]", pNativeOS );
				pNativeLanMan	= pNativeOS + strlen(pNativeOS) + 1;
				LOG( pLog, LogDBG, "pNativeLanMan[%s]", pNativeLanMan );
				pPrimaryDomain	= pNativeLanMan + strlen(pNativeLanMan) + 1;
				LOG( pLog, LogDBG, "pPrimaryDomain[%s]", pPrimaryDomain );
			}
			}
			break;

		case SMB_COM_TREE_CONNECT_ANDX:
		{ 	SMB_Parameters_Connect_Rpl_t	*pRpl;
			char	*pService; char *pNative;
			int		Len;

			LOG(pLog,LogDBG,"TID[0x%x]", pHead->h_TID );

			pRpl	= (SMB_Parameters_Connect_Rpl_t*)pParam->p_Words;
			LOG(pLog,LogDBG,"AndXCommand[0x%x]", pRpl->c_AndXCommand );
			ASSERT( pRpl->c_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%d]", pRpl->c_AndXOffset );
			LOG(pLog,LogDBG,"OptinalSupport[0x%x]", pRpl->c_OptionalSupport );

			pService	= (char*)pData->d_Bytes;
			LOG(pLog,LogDBG,"pService[%s]", pService );
			Len	= strlen(pService);
			pNative	= pService + Len + 1;

			if( pHead->h_Flags2 & SMB_FLAGS2_UNICODE ) {

				wchar_t	*pBuf;

				if( ((uintptr_t)pNative) & 1 )	++pNative;

				Len = UnicodeLen( (uint16_t*)pNative ) + 2;

				pBuf = CreateUnicodeToWchar( (uint16_t*)pNative, Len );

				LOG( pLog, LogDBG,"pNative=%p:uni[%S]", pNative, pBuf );

				DestroyUnicodeToWchar( pBuf );

			} else {
LOG(pLog,LogDBG, "pNative[%s]", pNative );
			}
//			ASSERT( WordCount == 0x3 );
			break;
		}

		case SMB_COM_NT_CREATE_ANDX:
		{ 	SMB_Parameters_CreateAndX_Res_t	*pRes;
			pRes	= (SMB_Parameters_CreateAndX_Res_t*)pParam->p_Words;
			LOG(pLog,LogDBG,"AndXCommand[0x%x]", pRes->c_AndXCommand );
			ASSERT( pRes->c_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%u]", pRes->c_AndXOffset );
			LOG(pLog,LogDBG,"OpLockLevel[%u]", pRes->c_OpLockLevel );
			LOG(pLog,LogDBG,"FID[0x%x]", pRes->c_FID );
			LOG(pLog,LogDBG,"CreateDisposition[0x%x]", 
								pRes->c_CreateDisposition );
			LOG(pLog,LogDBG,"ExtFileAttributes[0x%x]", 
								pRes->c_ExtFileAttributes );
			LOG(pLog,LogDBG,"AllocationSize[%"PRIu64"]", 
								pRes->c_AllocationSize );
			LOG(pLog,LogDBG,"EndOfFile[%"PRIu64"]", pRes->c_EndOfFile );
			LOG(pLog,LogDBG,"ResourceType[0x%x]", pRes->c_ResourceType );
			LOG(pLog,LogDBG,"NMPipeStatus[0x%x]", pRes->c_NMPipeStatus );
			LOG(pLog,LogDBG,"Directory[%d]", pRes->c_Directory );

			break;
		}
		case SMB_COM_CLOSE:
		{
			if( pCon->c_FIDMeta ) {
LOG(pLog,LogDBG,"HashRemove:c_FIDMeta[%d]", pCon->c_FIDMeta );
				MtxLock( &pCon->c_Mtx );

				_PairFIDPutCoC( pCon, pCon->c_FIDMeta );

				MtxUnlock( &pCon->c_Mtx );
				pCon->c_FIDMeta	= 0;
			}
			break;
		}
		case SMB_COM_TRANSACTION2:
		case SMB_COM_TRANSACTION:
		{
			void *pTrans2Param, *pTrans2Data;

			if( WordCount == 0 )	break;

			{ SMB_Parameters_Transaction2_Res_t *pRes;
			int	i; uint8_t *Trans2_Parameters, *Trans2_Data;

			pRes = (SMB_Parameters_Transaction2_Res_t*)pParam->p_Words;
			ASSERT( pParam->p_WordCount == 
						(sizeof(SMB_Parameters_Transaction2_Res_t)/2
							+ pRes->t_SetupCount) );
			LOG( pLog, LogDBG,"TotalParameterCount[%u]", 
						pRes->t_TotalParameterCount );
			LOG( pLog, LogDBG,"TotalDataCount[%u]", 
						pRes->t_TotalDataCount );
			ASSERT( pRes->t_Reserved1 == 0 );
			LOG( pLog, LogDBG,"ParameterCount[%u]", pRes->t_ParameterCount );
			LOG( pLog, LogDBG,"ParameterOffset[%u]", pRes->t_ParameterOffset );
			LOG( pLog, LogDBG,"ParameterDisplacement[%u]", 
								pRes->t_ParameterDisplacement );
			LOG( pLog, LogDBG,"DataCount[%u]", pRes->t_DataCount );
			LOG( pLog, LogDBG,"DataOffset[%u]", pRes->t_DataOffset );
			LOG( pLog, LogDBG,"DataDisplacement[%u]", 
								pRes->t_DataDisplacement );
			LOG( pLog, LogDBG,"SetupCount[%u]", pRes->t_SetupCount );

			if( pPIDMID->pm_TotalParameterCountRes == 0 ) {
				pPIDMID->pm_TotalParameterCountRes	= 
									pRes->t_TotalParameterCount;
				pPIDMID->pm_TotalDataCountRes		= 
									pRes->t_TotalDataCount;
			}

			pPIDMID->pm_TotalParameterCountRes	-= pRes->t_ParameterCount;
			pPIDMID->pm_TotalDataCountRes		-= pRes->t_DataCount;

			if( pPIDMID->pm_TotalParameterCountRes
					|| pPIDMID->pm_TotalDataCountRes ) {

				Psuedo	= 1;
			} else {
				pPIDMID->pm_State	= TransmittedAllRequests;
			}

			ASSERT( pRes->t_Reserved2 == 0 );

			pTrans2Param	= (char*)pHead + pRes->t_ParameterOffset;
			pTrans2Data		= (char*)pHead + pRes->t_DataOffset;

			for( i = 0; i < pRes->t_SetupCount; i++ ) {
				LOG( pLog, LogDBG,"%d:Setup[%u]", i, pRes->t_Setup[i] );
				switch( pRes->t_Setup[i] ) {
					case TRANS2_OPEN2:
						break;
					case	TRANS2_GET_DFS_REFERRAL:
					{
						SMB_Data_Trans2_Get_DFS_Referral_Rpl_t	*pDFS;
						int	j;
						DFS_Referral_t	*pEntry;
						DFS_Referral_V1_t	*pEntry_V1;
						DFS_Referral_V2_t	*pEntry_V2;
						DFS_Referral_V34_t	*pEntry_V34;
						int		Len;
						wchar_t	*pBuf;

						pDFS = (SMB_Data_Trans2_Get_DFS_Referral_Rpl_t*)
								pTrans2Data;
LOG(pLog,LogDBG,"PathConsumed[%d]", pDFS->dr_PathConsumed );
LOG(pLog,LogDBG,"NumberOfReferrals[%d]", pDFS->dr_NumberOfReferrals );
LOG(pLog,LogDBG,"ReferralHeaderFlags[0x%x]", pDFS->dr_ReferralHeaderFlags );
						pEntry	= (DFS_Referral_t*)(pDFS+1);
						for( j = 0; j < pDFS->dr_NumberOfReferrals; j++ ) {

LOG(pLog, LogDBG, 
	"VersionNumber[%d] Size[%d] ServerType[0x%x] ReferralEntryFlags[0x%x]",
								pEntry->r_VersionNumber, pEntry->r_Size,
								pEntry->r_ServerType,
								pEntry->r_ReferralHeaderFlags );

							switch( pEntry->r_VersionNumber ) {

							case DFS_REFERRAL_V1:
								pEntry_V1	= (DFS_Referral_V1_t*)pEntry;

								Len = UnicodeLen(pEntry_V1->v1_ShareName ) + 2;

								pBuf = CreateUnicodeToWchar( 
									pEntry_V1->v1_ShareName, Len );

								LOG( pLog, LogDBG,"uni[%S]", pBuf );

								DestroyUnicodeToWchar( pBuf );
								break;

							case DFS_REFERRAL_V2:
								pEntry_V2	= (DFS_Referral_V2_t*)pEntry;

								ASSERT( pEntry_V2->v2_Proximity == 0 );
LOG(pLog, LogDBG, "TimeToLive[%d]", pEntry_V2->v2_TimeToLive );
LOG(pLog, LogDBG, "DFSPathOffset[%d]", pEntry_V2->v2_DFSPathOffset );
LOG(pLog, LogDBG, "NetworkAddressOffset[%d]", pEntry_V2->v2_NetworkAddressOffset );
								break;
							case DFS_REFERRAL_V3:
							case DFS_REFERRAL_V4:
								pEntry_V34	= (DFS_Referral_V34_t*)pEntry;

LOG(pLog, LogDBG, "TimeToLive[%d]", pEntry_V34->v34_TimeToLive );
								if( pEntry_V34->v34_Head.r_ReferralHeaderFlags
										& NameListReferral ) {	//1
									Referral_Entry_1_t *pReferral1;

									pReferral1	= (Referral_Entry_1_t*)
														(pEntry_V34+1 );
LOG(pLog, LogDBG, "SpecialNameOffset[%d]", pReferral1->e1_SpecialNameOffset );
LOG(pLog, LogDBG, "NumberOfExtendedNames[%d]", pReferral1->e1_NumberOfExpandedNames );
LOG(pLog, LogDBG, "ExpandedNameOffset[%d]", pReferral1->e1_ExpandedNameOffset );
								} else {						// 0
									Referral_Entry_0_t *pReferral0;

									pReferral0	= (Referral_Entry_0_t*)
														(pEntry_V34+1 );
LOG(pLog, LogDBG, "DFSPathOffset[%d]", pReferral0->e0_DFSPathOffset );
LOG(pLog, LogDBG, "DFSAlternatePathOffset[%d]", pReferral0->e0_DFSAlternatePathOffset );
LOG(pLog, LogDBG, "NetworkAddressOffset[%d]", pReferral0->e0_NetworkAddressOffset );
								}
								break;
							}
						}
						break;
					}
				}
			}
			Trans2_Parameters	= &pData->d_Bytes[2];
			for( i = 0; i < pRes->t_ParameterCount; i++ ) {
				LOG( pLog, LogDBG,"%d:Parameters[%u]", 
						i, Trans2_Parameters[i] );
			}
			Trans2_Data	= &pData->d_Bytes[2+pRes->t_ParameterCount];
//			for( i = 0; i < pRes->t_DataCount; i++ ) {
//				LOG( pLog, LogDBG,"%d:Data[%u]", 
//						i, Trans2_Data[i] );
//			}
			}
			break;
		}

		case SMB_COM_WRITE_ANDX:
			{SMB_Parameters_WriteAndX_Res_t	*pRes;

			pRes = (SMB_Parameters_WriteAndX_Res_t*)pParam->p_Words;
			ASSERT( WordCount == 0x6 );
			LOG(pLog,LogDBG,"AndXCommand[0x%x]", pRes->w_AndXCommand );
			ASSERT( pRes->w_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%u]", pRes->w_AndXOffset );
			LOG(pLog,LogDBG,"Count[%u]", pRes->w_Count );
			LOG(pLog,LogDBG,"Available[%u]", pRes->w_Available );
			ASSERT( pRes->w_Reserved == 0 );
			ASSERT( ByteCount == 0 );
			}
			break;

		case SMB_COM_READ_ANDX:
			{SMB_Parameters_ReadAndX_Res_t	*pRes;

			pRes = (SMB_Parameters_ReadAndX_Res_t*)pParam->p_Words;
			ASSERT( WordCount == 0x0c );
			LOG(pLog,LogDBG,"AndXCommand[0x%x]", pRes->r_AndXCommand );
			ASSERT( pRes->r_AndXReserved == 0 );
			LOG(pLog,LogDBG,"AndXOffset[%u]", pRes->r_AndXOffset );
			LOG(pLog,LogDBG,"Available[%u]", pRes->r_Available );
			ASSERT( pRes->r_DataCompactionMode == 0 );
			ASSERT( pRes->r_Reserved1 == 0 );
			LOG(pLog,LogDBG,"DataLength[%u]", pRes->r_DataLength );
//			ASSERT( pRes->r_Reserved2[0] == 0 );
//			ASSERT( pRes->r_Reserved2[1] == 0 );
//			ASSERT( pRes->r_Reserved2[2] == 0 );
//			ASSERT( pRes->r_Reserved2[3] == 0 );
//			ASSERT( pRes->r_Reserved2[4] == 0 );
			}
			break;

		default:	
			break;
	}
ret:
	if( pPIDMID && pPIDMID->pm_State == TransmittedAllRequests ) {
		PIDMIDDelete( &pCon->c_PIDMID, pPIDMID );
	}
skip1:
	if( Status != CNV_PSUEDO_RPL ) {
		MsgTrim( pMsg, pMetaHead->m_Length );

		Ret = CIFSSendMsgByFd( pFdEvCoC->c_FdEvent.fd_Fd, pMsg );	// To Client
		if( Ret < 0 ) goto err;
	}

	return( Psuedo );
err:
	return( -1 );
}

// Connector
FdEvCoC_t*
CIFSFdEvCoCAlloc()
{
	ConCoC_t	*pCon;

	pCon	= (ConCoC_t*)Malloc( sizeof(*pCon) );

	memset( pCon, 0, sizeof(*pCon) );
	MtxInit( &pCon->c_Mtx );
	PIDMIDInit( &pCon->c_PIDMID );

	list_init( &pCon->c_LnkFID );
	HashInit( &pCon->c_HashFIDMeta, PRIMARY_1009, 
			HASH_CODE_INT, FIDMetaCmp, NULL, Malloc, Free, NULL);
	HashInit( &pCon->c_HashFIDName, PRIMARY_1009, 
			HASH_CODE_STR, FIDNameCmp, NULL, Malloc, Free, NULL);

	return( &pCon->c_FdEvCoC );
}

void
CIFSFdEvCoCFree( FdEvCoC_t *pFdEvCoC )
{
	ConCoC_t	*pCon = container_of( pFdEvCoC, ConCoC_t, c_FdEvCoC );

	MtxLock( &pCon->c_Mtx );

	PIDMIDTerm( &pCon->c_PIDMID );

	if( pCon->c_SessionKeyMeta )	IdMapPut( CtrlCoC.c_pIdMap,
												pCon->c_SessionKeyMeta );
	if( pCon->c_UIDMeta )	IdMapPut( CtrlCoC.c_pIdMap, pCon->c_UIDMeta );
	if( pCon->c_TIDMeta )	IdMapPut( CtrlCoC.c_pIdMap, pCon->c_TIDMeta );

	MtxUnlock( &pCon->c_Mtx );

	Free( pCon );	
}

int
CIFSSessionOpen( FdEvCoC_t *pFdEvCoC, char *pCellName )
{
//	ConCoC_t	*pConCoC = container_of( pFdEvCoC, ConCoC_t, c_FdEvCoC );
	struct Session	*pSession;
	int	No;
	int	Ret;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
                                Send, Recv, Malloc, Free, pCellName );
	if( pLog ) {
		PaxosSessionSetLog( pSession, pLog );
	}
	No = IdMapGet( CtrlCoC.c_pIdMap ); 
	pFdEvCoC->c_NoSession	= No;
	pFdEvCoC->c_pSession	= pSession;

	Ret = PaxosSessionOpen( pSession, No, TRUE );
	if( Ret < 0 )	goto err;

	// bind
	Ret = CNVEventSessionBind( pSession, CtrlCoC.c_pEvent );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
CIFSSessionClose( FdEvCoC_t *pFdEvCoC )
{
//	ConCoC_t	*pConCoC = container_of( pFdEvCoC, ConCoC_t, c_FdEvCoC );
	struct Session	*pSession;
	int	Ret;

	pSession	= pFdEvCoC->c_pSession;
	Ret = PaxosSessionClose( pSession );

	IdMapPut( CtrlCoC.c_pIdMap, pFdEvCoC->c_NoSession ); 

	PaxosSessionFree( pSession );

	pFdEvCoC->c_NoSession	= 0;
	pFdEvCoC->c_pSession	= NULL;

	return( Ret );
}

void*
ThreadEvent( void *pVoid )
{
	ConCoC_t	*pConCoC;
	CnvEventWaitReq_t	Req;
	CnvEventWaitRpl_t	*pRpl;
	Msg_t		*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	struct Session	*pSession = (struct Session*)pVoid;
//	SMB_Header_t	*pHead;

	while( 1 ) {
		SESSION_MSGINIT( &Req, CNV_EVENT_WAIT, 0, 0, sizeof(Req) );
		pMsgReq	= MsgCreate( 1, PaxosSessionGetfMalloc(pSession),
								PaxosSessionGetfFree(pSession) );

		MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
		MsgAdd( pMsgReq, &Vec );

		Req.ew_Head.h_Cksum64	= 0;
		Req.ew_Head.h_Cksum64	= in_cksum64( &Req, sizeof(Req), 0 );

		/* âiâìÇ…ë“Ç¬ */
		pMsgRpl = PaxosSessionReqAndRplByMsg( pSession, pMsgReq, TRUE, TRUE );
		MsgDestroy( pMsgReq );

		if( pMsgRpl ) {
			pRpl	= (CnvEventWaitRpl_t*)MsgFirst( pMsgRpl );
			errno	= pRpl->ew_Head.h_Error;
			if( errno )	 {
				MsgDestroy( pMsgRpl );
				goto err1;
			}
		} else {
			goto err1;
		}
		pSession = PaxosSessionFind( pRpl->ew_CellName, pRpl->ew_No );
		pConCoC	= (ConCoC_t*)PaxosSessionGetTag( pSession );

	//	pHead	= (SMB_Header_t*)(pRpl+1);
/*
 *	xxx
 */

		MsgTrim( pMsgRpl, sizeof(CnvEventWaitRpl_t) );
		CIFSSendMsgByFd( pConCoC->c_FdEvCoC.c_FdEvent.fd_Fd, 
								pMsgRpl );// To Client

		MsgDestroy( pMsgRpl );
	}
err1:
	ASSERT( 0 );
	pthread_exit( 0 );
}

CnvCtrlCoC_t*
CIFSInitCoC( void *pVoid )
{
	struct	Session	*pEvent;
	int		Ret;

	CtrlCoC.c_pIdMap	= IdMapCreate( 0x10000, Malloc, Free );

	// event thread
	pEvent = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
						Send, Recv, Malloc, Free, (char*)pVoid );
	if( pLog ) {
		PaxosSessionSetLog( pEvent, pLog );
	}
	IdMapSet( CtrlCoC.c_pIdMap, CNV_EVENT_NO ); 
	Ret = PaxosSessionOpen( pEvent, CNV_EVENT_NO, TRUE );
	if( Ret < 0 )	goto err;

	CtrlCoC.c_pEvent	= pEvent;

LOG(pLog, LogDBG, "Event Session=%p", pEvent );
	pthread_create( &CtrlCoC.c_ThreadEvent, NULL, ThreadEvent, pEvent );

	return( &CtrlCoC.c_Ctrl );
err:
	return( NULL );
}

/*
 *	Interface
 */
CnvIF_t	CIFS_IF = {
	.IF_fInitCoC		= CIFSInitCoC,
	.IF_fRecvMsgByFd	= CIFSRecvMsgByFd,
	.IF_fSendMsgByFd	= CIFSSendMsgByFd,
	.IF_fRequest		= CIFSRequestByMsg,	/* request from client */
	.IF_fReply			= CIFSReplyByMsg,
	.IF_fConAlloc		= CIFSFdEvCoCAlloc,
	.IF_fConFree		= CIFSFdEvCoCFree,
	.IF_fSessionOpen	= CIFSSessionOpen,
	.IF_fSessionClose	= CIFSSessionClose,
};
