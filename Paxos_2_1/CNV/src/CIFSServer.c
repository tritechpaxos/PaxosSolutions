/******************************************************************************
*
*  CIFSServer.c 	--- CIFS protocol
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

extern	iconv_t	iConv;
extern	CnvIF_t	*pIF;

//SERVICE

CtrlCoS_t	CtrlCoS;
//FdEventCtrl_t	*pFdEvCtrlServer	= &CtrlCoS.s_Ctrl.s_FdEvCtrlServer;

// Recieve from CIFS server
int
HandlerRecvFromCIFS( FdEvent_t *pEv, FdEvMode_t Mode )
{
	ConCoS_t	*pConCoS = container_of( pEv, ConCoS_t, s_FdEvent );
	uint32_t	Total;
	uint8_t		WordCount;
	uint16_t	ByteCount;
	SMB_Header_t		*pHead;
	SMB_Parameters_t	*pParam = NULL;	
//	SMB_Data_t			*pData = NULL;	
	int	Fd;
	Msg_t	*pRpl;
	uint32_t	PID;
	PIDMID_t	*pPIDMID;
	bool_t		bMeta = FALSE;
	PairFID_t	*pPair = NULL;
	PairTID_t	*pPairTID = NULL;
	PaxosSessionHead_t	*pH;

	Fd	= pEv->fd_Fd;

	pRpl = (*pIF->IF_fRecvMsgByFd)( Fd, Malloc, Free );	// Bare: SMB

LOG(pLog, LogDBG, "pConCoS=%p Fd[%d] Rpl[%p] errno[%d]", 
pConCoS, Fd, pRpl, errno );
#ifdef	ZZZ
	ASSERT( pRpl );
#else
	if( !pRpl )	return( 0 ); // ???
#endif

	Total		= (uintptr_t)pRpl->m_pTag0;
	WordCount	= (uintptr_t)pRpl->m_pTag1;
	ByteCount	= (uintptr_t)pRpl->m_pTag2;

	pHead	= (SMB_Header_t*)pRpl->m_aVec[SMB_MSG_HEAD].v_pStart;
	if( WordCount )	pParam	= (SMB_Parameters_t*)
								pRpl->m_aVec[SMB_MSG_PARAM].v_pStart;
//	if( ByteCount )	pData	= (SMB_Data_t*)pRpl->m_aVec[3].v_pStart;

LOG( pLog,LogDBG,
"<---RPL:Cmd=0x%x Error(0x%x:0x%x) Flags=0x%x Flags2=0x%x", 
pHead->h_Command, pHead->h_Status.e_ErrorClass, pHead->h_Status.e_ErrorCode,
pHead->h_Flags, pHead->h_Flags2 );

LOG(pLog, LogDBG, "Total[%d] WordCount[%d] ByteCount[%d]", 
Total, WordCount, ByteCount );

LOG( pLog, LogDBG, 
"PIDHigh[0x%x] PIDLow[0x%x] MID[0x%x] UID[0x%x] TID[0x%x]",
pHead->h_PIDHigh, pHead->h_PIDLow, pHead->h_MID, pHead->h_TID );

	if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_OFF ) {

		// Reply to Client
		if( !bMeta && pRpl )	CIFSMetaHeadInsert( pRpl, NULL );

LOG(pLog, LogDBG, "s_CnvCmd=0x%x", pConCoS->s_CnvCmd );
		CNVAcceptReplyByMsg( pConCoS->s_pAccept, 
								pRpl, pConCoS->s_CnvCmd, 0 );
	} else {

		MtxLock( &pConCoS->s_Mtx );

		PID	= (pHead->h_PIDHigh<<16)&0xffff0000;
		PID	|= pHead->h_PIDLow;

		pPIDMID	= PIDMIDFind( &pConCoS->s_PIDMID, PID, pHead->h_MID, FALSE );
ASSERT( pPIDMID );

		if( pPIDMID && pPIDMID->pm_bNotify ) {
LOG( pLog, LogDBG, "EventRaise" );

			pPIDMID->pm_bNotify = FALSE;

			pH	= (PaxosSessionHead_t*)Malloc( sizeof(*pH) + Total );
			memset( pH, 0, sizeof(*pH) );
			memcpy( pH+1, pHead, Total );
			pH->h_Len	= sizeof(*pH) + Total;

			MsgDestroy( pRpl );

			PaxosAcceptEventRaise(pConCoS->s_pEvent, pH);

			goto exit;
		}

LOG( pLog,LogDBG,"<---REPLY:Cmd=0x%x Error(0x%x:0x%x) Flags=0x%x Flags2=0x%x", 
pHead->h_Command, pHead->h_Status.e_ErrorClass, pHead->h_Status.e_ErrorCode,
pHead->h_Flags, pHead->h_Flags2 );
LOG( pLog, LogDBG, 
"PIDHigh[0x%x] PIDLow[0x%x] MID[0x%x] UID[0x%x] TID[0x%x]",
pHead->h_PIDHigh, pHead->h_PIDLow, pHead->h_MID, pHead->h_UID, pHead->h_TID );

	if( pHead->h_Status.e_ErrorClass ) {
		PIDMIDDelete( &pConCoS->s_PIDMID, pPIDMID );
		goto ret;
	}

	if( WordCount )	pParam	= (SMB_Parameters_t*)pRpl->m_aVec[2].v_pStart;
//	if( ByteCount )	pData	= (SMB_Data_t*)pRpl->m_aVec[3].v_pStart;

	/*
	 *	Check Interim response
	 */
	if( pPIDMID->pm_State == TransactionPrimaryRequest
			&& WordCount == 0 && ByteCount == 0 ) {

		pPIDMID->pm_State = ReceivedInterimResponse;

		goto reply;
	}
	// Meta
	switch( pHead->h_Command ) {

		case SMB_COM_TREE_DISCONNECT:
		{
			MetaHead_t	*pMeta;

			pPairTID	= PairCoSGetByTIDMeta( 
							&pConCoS->s_Pair, pConCoS->s_TIDMeta, FALSE );
			ASSERT( pPairTID );
LOG(pLog, LogDBG, "SMB_COM_TREE_DISCONNECT:pPairTID=%p TID:0x%x(Meta)<-0x%x", 
pPairTID, pPairTID->p_TIDMeta, pPairTID->p_TID );

			pMeta	= (MetaHead_t*)CIFSMetaReturn( pConCoS->s_TIDMeta );
			bMeta	= TRUE;

			CIFSMetaHeadInsert( pRpl, pMeta );

			pHead->h_TID = pPairTID->p_TIDMeta;	// DISCONNECT TID accepted

			PairCoSPutTID( &pConCoS->s_Pair, pPairTID, TRUE );
			pPairTID = NULL;
			pConCoS->s_TID = 0;
			
			break;
		}
		case SMB_COM_NEGOTIATE:
		{
			SMB_Parameters_NT_LM_Res_t	*pParamNT_LM;
			pParamNT_LM = (SMB_Parameters_NT_LM_Res_t*)pParam->p_Words;
			pConCoS->s_SessionKey	= pParamNT_LM->p_SessionKey;
//	if( !CtrlCoS.s_Ctrl.s_bSnoop )	goto reply/*skip2*/;
			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON
					&& pConCoS->s_SessionKeyMeta ) {
			
				pParamNT_LM->p_SessionKey = pConCoS->s_SessionKeyMeta;
			}
LOG(pLog, LogDBG, "Snoop=%d SessionKeyMeta[0x%x] SessionKey[0x%x]",
CtrlCoS.s_Ctrl.s_Snoop, pConCoS->s_SessionKeyMeta, pConCoS->s_SessionKey );
			break;
		}
		case SMB_COM_SESSION_SETUP_ANDX:
			pConCoS->s_UID	= pHead->h_UID;
LOG(pLog, LogDBG, "Snoop=%d pConCoS=%p UIDMeta[0x%x] UID[0x%x]",
CtrlCoS.s_Ctrl.s_Snoop, pConCoS, pConCoS->s_UIDMeta, pConCoS->s_UID );
			break;

		case SMB_COM_TREE_CONNECT_ANDX:
#ifdef	ZZZ
			pConCoS->s_TID	= pHead->h_TID;
LOG(pLog, LogDBG, "Snoop=%d pConCoS=%p TIDMeta[0x%x] TID[0x%x]",
CtrlCoS.s_Ctrl.s_Snoop, pConCoS, pConCoS->s_TIDMeta, pConCoS->s_TID );
			break;
#else
		{	SMB_Parameters_Connect_Rpl_t	*pParamConnect;

			pParamConnect = (SMB_Parameters_Connect_Rpl_t*)pParam->p_Words;

LOG(pLog,LogDBG,"SMB_COM_TREE_CONNECT_ANDX");
			pPairTID	= PairCoSGetByTIDMeta( &pConCoS->s_Pair, 
									pConCoS->s_TIDMeta, TRUE );
			if( pPairTID->p_TID != pHead->h_TID ) {
LOG(pLog, LogDBG, 
"### SET:pPairTID=%p TIDMeta[0x%x] p_TID[0x%x] h_TID[0x%x] ###",
pPairTID, pPairTID->p_TIDMeta, pPairTID->p_TID, pHead->h_TID );
				PairCoSSetTID( &pConCoS->s_Pair, pPairTID, pHead->h_TID ); 
			}

			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
				pHead->h_TID	= pPairTID->p_TIDMeta;
			}
			if( pPairTID ) {
				if( pConCoS->s_TIDFlags & TREE_CONNECT_ANDX_DISCONNECT_TID ) {
					PairCoSPutTID( &pConCoS->s_Pair, pPairTID, TRUE );
				} else {
					PairCoSPutTID( &pConCoS->s_Pair, pPairTID, FALSE );
				}
				pPairTID = NULL;
			}
			break;
		}
#endif

		case SMB_COM_NT_CREATE_ANDX:
		{	SMB_Parameters_CreateAndX_Res_t	*pParamCreate;

			pParamCreate = (SMB_Parameters_CreateAndX_Res_t*)pParam->p_Words;

LOG(pLog,LogDBG,"SMB_COM_NT_CREATE_ANDX");
			pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
									pConCoS->s_FIDMeta, TRUE );
			if( pPair->p_FID != pParamCreate->c_FID ) {
LOG(pLog, LogDBG, "### SET:pPair=%p FIDMeta[0x%x] p_FID[0x%x] c_FID[0x%x] ###",
pPair, pPair->p_FIDMeta, pPair->p_FID, pParamCreate->c_FID );
				PairCoSSetFID( &pConCoS->s_Pair, pPair, pParamCreate->c_FID ); 
			}

			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
				pParamCreate->c_FID	= pPair->p_FIDMeta;
			}
			if( pPair ) {
				PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				pPair = NULL;
			}
			break;
		}
		case SMB_COM_CLOSE:
		{
			MetaHead_t	*pMeta;

			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
LOG(pLog,LogDBG,"SMB_COM_CLOSE");
				pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
									pConCoS->s_FIDMeta, FALSE );
				ASSERT( pPair );
LOG(pLog, LogDBG, "CLOSE:pPair=%p FIDMeta[0x%x] FID[0x%x]",
pPair, pPair->p_FIDMeta, pPair->p_FID );

				pMeta	= (MetaHead_t*)CIFSMetaReturn( pPair->p_FIDMeta);
				bMeta	= TRUE;

				CIFSMetaHeadInsert( pRpl, pMeta );

				PairCoSPutFID( &pConCoS->s_Pair, pPair, TRUE );
			}
			pPair	= NULL;
			break;
		}
		case SMB_COM_TRANSACTION2:	// Final response
		{	SMB_Parameters_Transaction2_Res_t *pParamTran;
			void	*pTrans2Param;
			int	i;

			pParamTran = (SMB_Parameters_Transaction2_Res_t*)pParam->p_Words;

			pTrans2Param	= (char*)pHead + pParamTran->t_ParameterOffset;

			if( pPIDMID->pm_TotalParameterCountRes == 0 ) {
				pPIDMID->pm_TotalParameterCountRes	= 
									pParamTran->t_TotalParameterCount;
				pPIDMID->pm_TotalDataCountRes		= 
									pParamTran->t_TotalDataCount;
			}

			pPIDMID->pm_TotalParameterCountRes	-= pParamTran->t_ParameterCount;
			pPIDMID->pm_TotalDataCountRes		-= pParamTran->t_DataCount;

			if( pPIDMID->pm_TotalParameterCountRes == 0
					&& pPIDMID->pm_TotalDataCountRes == 0 ) {

				pPIDMID->pm_State	= TransmittedAllRequests;
			}
			for( i = 0; i < pParamTran->t_SetupCount; i++ ) {
				switch( pParamTran->t_Setup[i] ) {
					case TRANS2_OPEN2:
					{	SMB_Parameters_Trans2_Open_Res_t *pParamRes =
						(SMB_Parameters_Trans2_Open_Res_t*) pTrans2Param;

LOG(pLog, LogDBG, "TRANS2_OPEN2");
						pPair = PairCoSGetByFIDMeta( &pConCoS->s_Pair,
												pParamRes->o_FID, FALSE );
LOG( pLog, LogDBG, "TRANS2_OPEN:Meta=0x%x FID=0x%x Cnt=%d", 
	pPair->p_FIDMeta, pPair->p_FID, pPair->p_Cnt );

						//pTrans2Param	+= 4;
						//pTrans2Data		+= 4;
		
						break;
					}
					case TRANS2_QUERY_FILE_INFORMATION:
						break;
				}
			}

			break;
		}

		case SMB_COM_NT_TRANSACT:
		{	SMB_Parameters_NT_Transact_Res_t	*pRes;

			pRes	= (SMB_Parameters_NT_Transact_Res_t*)pParam->p_Words;

			LOG( pLog, LogDBG,"TotalParameterCount[%u]", 
						pRes->n_TotalParameterCount );
			LOG( pLog, LogDBG,"TotalDataCount[%u]", 
						pRes->n_TotalDataCount );
			LOG( pLog, LogDBG,"ParameterCount[%u]", pRes->n_ParameterCount );
			LOG( pLog, LogDBG,"ParameterOffset[%u]", pRes->n_ParameterOffset );
			LOG( pLog, LogDBG,"DataCount[%u]", pRes->n_DataCount );
			LOG( pLog, LogDBG,"DataOffset[%u]", pRes->n_DataOffset );
			LOG( pLog, LogDBG,"SetupCount[%u]", pRes->n_SetupCount );

			break;
		}

		default:
			break;
	}
	if( pPIDMID && pPIDMID->pm_State == TransmittedAllRequests ) {
		PIDMIDDelete( &pConCoS->s_PIDMID, pPIDMID );
	}
ret:
LOG(pLog,LogDBG,"UID[0x%x] TID[0x%x]", pHead->h_UID, pHead->h_TID );
		if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
			if( pHead->h_UID && pHead->h_UID == pConCoS->s_UID ) {
				pHead->h_UID	= pConCoS->s_UIDMeta;
LOG(pLog, LogDBG, "pConCoS=%p UID:0x%x(Meta)<-0x%x", 
pConCoS, pConCoS->s_UIDMeta, pConCoS->s_UID );
			}
#ifdef	ZZZ
			if( pHead->h_TID && pHead->h_TID == pConCoS->s_TID ) {
				pHead->h_TID	= pConCoS->s_TIDMeta;
LOG(pLog, LogDBG, "pConCos=%p TID:0x%x(Meta)<-0x%x", 
pConCoS, pConCoS->s_TIDMeta, pConCoS->s_TID );
			}
#else
			if( pHead->h_TID && pHead->h_TID == pConCoS->s_TID ) {
				pPairTID	= PairCoSGetByTID( &pConCoS->s_Pair, pHead->h_TID );
				ASSERT( pPairTID );

				pHead->h_TID	= pPairTID->p_TIDMeta;
LOG(pLog, LogDBG, "pPairTID=%p TID:0x%x(Meta)<-0x%x", 
pPairTID, pPairTID->p_TIDMeta, pPairTID->p_TID );
				PairCoSPutTID( &pConCoS->s_Pair, pPairTID, FALSE );
				pPairTID = NULL;
			}
#endif
		}

reply:
		if( !bMeta && pRpl )	CIFSMetaHeadInsert( pRpl, NULL );

		if( !pRpl )	pRpl	= MsgCreate( 1, Malloc, Free );

LOG( pLog, LogDBG, "pPair=%p", pPair );
		if( pPair )	{
			PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
			pPair	= NULL;
		}
		if( pPairTID )	{
			PairCoSPutTID( &pConCoS->s_Pair, pPairTID, FALSE );
			pPair	= NULL;
		}

LOG( pLog, LogDBG, "pConCoS=%p s_CnvCmd=0x%x", pConCoS, pConCoS->s_CnvCmd );
		CNVAcceptReplyByMsg( pConCoS->s_pAccept, pRpl, pConCoS->s_CnvCmd, 0 );

exit:
		MtxUnlock( &pConCoS->s_Mtx );
	}
	return( 0 );
}

// Request To CIFS Server
int
CIFSRequestToServer(struct PaxosAccept *pAccept, PaxosSessionHead_t *pCNVHead )
{
	int	Fd;
	int		Ret;
	ConCoS_t	*pConCoS;
	uint32_t	Total, Len;
	uint32_t	*pDataLen;
	uint8_t		WordCount;
	uint16_t	ByteCount, w16;
	MetaHead_t			*pMetaHead;
	SMB_Header_t		*pHead;
	SMB_Parameters_t	*pParam = NULL;	
	SMB_Data_t			*pData = NULL;	
	PairFID_t	*pPair = NULL;
	uint32_t	PID;
	PIDMID_t	*pPIDMID;
	int	CNV_Cmd;
	bool_t		bSendToServer = TRUE;
	Msg_t	*pRpl = NULL;
	int		Status = 0;
	bool_t	bUID= TRUE;
	bool_t	bTID= TRUE;

	pConCoS	= (ConCoS_t*)PaxosAcceptGetTag( pAccept, CNV_FAMILY );

	Fd	= pConCoS->s_FdEvent.fd_Fd;

	CNV_Cmd	= pCNVHead->h_Cmd;
	pConCoS->s_CnvCmd	= CNV_Cmd;
LOG(pLog, LogDBG, "h_Cmd[0x%x] Fd[%d] pConCoS[%p]", 
pCNVHead->h_Cmd, Fd, pConCoS );
	if( CNV_Cmd == CNV_PSUEDO_REQ )	goto ret;

	pMetaHead	= (MetaHead_t*)(pCNVHead+1);
	pDataLen	= (uint32_t*)((char*)pMetaHead 
								+ pMetaHead->m_Length);
	Total		= ntohl( *pDataLen );
	pHead		= (SMB_Header_t*)(pDataLen+1);

	if( Total > sizeof(SMB_Header_t) ) {
		pParam	= (SMB_Parameters_t*)(pHead+1);
		WordCount	= pParam->p_WordCount;
		Len	= sizeof(*pParam) + WordCount*2;

		if( Total > sizeof(SMB_Header_t) + Len ) {
			pData	= (SMB_Data_t*)((char*)pParam+Len);
			memcpy( &w16, &pData->d_ByteCount, sizeof(w16) );
			ByteCount	= w16;//ntohs( w16 );
			Len	= sizeof(*pData) + ByteCount;

		}
	}

LOG( pLog, LogDBG, 
"-->REQ:Cmd=0x%x Flags2=0x%x Uni=%d Total=%d Word=%d Byte=%d pHead=%p pParm=%p pData=%p", 
pHead->h_Command, pHead->h_Flags2, pHead->h_Flags2 & SMB_FLAGS2_UNICODE,
Total, WordCount, ByteCount, pHead, pParam, pData );
LOG( pLog, LogDBG, 
"PIDHigh[0x%x] PIDLow[0x%x] MID[0x%x] UID[0x%x] TID[0x%x]",
pHead->h_PIDHigh, pHead->h_PIDLow, pHead->h_MID, pHead->h_UID, pHead->h_TID );
	
	if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_OFF ) {

		switch( pHead->h_Command ) {
			case SMB_COM_NT_TRANSACT:
			{	SMB_Parameters_NT_Transact_Req_t	*pReq;
				pReq	= (SMB_Parameters_NT_Transact_Req_t*)
							pParam->p_Words;
				switch( pReq->n_Function ){
					case NT_TRANSACT_NOTIFY_CHANGE:

						LOG( pLog, LogDBG, "NOTIFY_CHANGE:CNV_PSUEDO_RPL" );

						CNVAcceptReplyByMsg( pAccept, NULL, CNV_Cmd,
											CNV_PSUEDO_RPL );
						break;
				}
				break;
			}
		}
		goto skip1;
	}

	PID	= (pHead->h_PIDHigh<<16)&0xffff0000;
	PID	|= pHead->h_PIDLow;

	pPIDMID	= PIDMIDFind( &pConCoS->s_PIDMID, PID, pHead->h_MID, TRUE );

	// Meta
	switch( pHead->h_Command ) {

		case SMB_COM_NEGOTIATE:
		{
			Meta_NT_LM_t	*pMeta;

			pMeta	= (Meta_NT_LM_t*)pMetaHead;
LOG(pLog,LogDBG,"pConCoS=%p SessionKeyMeta(0x%x<-0x%x)", 
pConCoS, pMeta->m_SessionKey, pConCoS->s_SessionKeyMeta );
			pConCoS->s_SessionKeyMeta	= pMeta->m_SessionKey;
			break;
		}
		case SMB_COM_SESSION_SETUP_ANDX:
		{
			Meta_Setup_t	*pMeta;

			pMeta	= (Meta_Setup_t*)pMetaHead;
LOG(pLog,LogDBG,"pConCoS=%p UIDMeta(0x%x<-0x%x)", 
pConCoS, pMeta->s_UID, pConCoS->s_UIDMeta );
			pConCoS->s_UIDMeta	= pMeta->s_UID;
			break;
		}
		case SMB_COM_TREE_CONNECT_ANDX:
		{
			Meta_Connect_t	*pMeta;
			SMB_Parameters_Connect_Req_t	*pParamConnect;

			pParamConnect = (SMB_Parameters_Connect_Req_t*)pParam->p_Words;

			pMeta	= (Meta_Connect_t*)pMetaHead;
LOG(pLog,LogDBG,"pConCoS=%p TIDMeta(0x%x<-0x%x) Flags(0x%x)", 
pConCoS, pMeta->c_TID, pConCoS->s_TIDMeta, pParamConnect->c_Flags );
			pConCoS->s_TIDMeta	= pMeta->c_TID;
			pConCoS->s_TIDFlags	= pParamConnect->c_Flags;
			break;
		}
		case SMB_COM_NT_CREATE_ANDX:
		{
			Meta_Create_t	*pMeta;

			pMeta	= (Meta_Create_t*)pMetaHead;

LOG(pLog,LogDBG,"pConCoS=%p FIDMeta(0x%x) c_TID(0x%x)", 
pConCoS, pMeta->c_FID );
			pConCoS->s_FIDMeta	= pMeta->c_FID;
			break;
		}
		case SMB_COM_NT_CANCEL:		// cancel pending requests
		{
			PairTID_t	*pPairTID;

			pPairTID	= PairCoSGetByTIDMeta( &pConCoS->s_Pair, 
								pHead->h_TID, FALSE );
			ASSERT( pPairTID );
			pHead->h_TID	= pPairTID->p_TID;
			PairCoSPutTID( &pConCoS->s_Pair, pPairTID, FALSE );

			bTID = FALSE;

			pRpl	= MsgCreate( 1, Malloc, Free );
			CIFSMetaHeadInsert( pRpl, NULL );
			Status	= CNV_PSUEDO_RPL;

			break;
		}
		case SMB_COM_CLOSE:
		{	SMB_Parameters_Close_Req_t *pParamClose;

			pParamClose = (SMB_Parameters_Close_Req_t*)pParam->p_Words;


			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {

LOG(pLog, LogDBG, "SMB_COM_CLOSE");
				pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
												pParamClose->c_FID, FALSE );
				ASSERT( pPair )
			
LOG(pLog,LogDBG,"CLOSE:FIDMeta(0x%x)->FID(0x%x)", 
pParamClose->c_FID, pPair->p_FID );
				pConCoS->s_FIDMeta	= pParamClose->c_FID;
				pParamClose->c_FID	= pPair->p_FID;
			}
LOG(pLog,LogDBG,"CLOSE:FID(0x%x)", 
pParamClose->c_FID );

			break;
		}
		case SMB_COM_WRITE_ANDX:
		{	SMB_Parameters_WriteAndX_Req_t *pParamWrite;

			pParamWrite = (SMB_Parameters_WriteAndX_Req_t*)pParam->p_Words;

			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
LOG(pLog, LogDBG, "SMB_COM_WRITE_ANDX");
				pPair = PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
									pParamWrite->w_FID, FALSE );
				ASSERT( pPair )

LOG(pLog,LogDBG,"WRITE:FIDMeta(0x%x)->FID(0x%x)", 
pParamWrite->w_FID, pPair->p_FID );

ASSERT( pParamWrite->w_FID == pPair->p_FIDMeta );
				pParamWrite->w_FID	= pPair->p_FID;

				PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				pPair	= NULL;
			}

LOG(pLog,LogDBG,"WRITE:FID(0x%x)", 
pParamWrite->w_FID );
			break;
		}
		case SMB_COM_READ_ANDX:
		{	SMB_Parameters_ReadAndX_Req_t *pParamRead;

			pParamRead = (SMB_Parameters_ReadAndX_Req_t*)pParam->p_Words;

			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
LOG(pLog, LogDBG, "SMB_COM_READ_ANDX");
				pPair = PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
									pParamRead->r_FID, FALSE );
				ASSERT( pPair )

LOG(pLog,LogDBG,"READ:FIDMeta(0x%x)->FID(0x%x)", 
pParamRead->r_FID, pPair->p_FID );

ASSERT( pParamRead->r_FID == pPair->p_FIDMeta );

				pParamRead->r_FID	= pPair->p_FID;

				PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				pPair	= NULL;
			}
LOG(pLog,LogDBG,"READ:FID(0x%x)", 
pParamRead->r_FID );
			break;
		}

		case SMB_COM_TRANSACTION:
		{	SMB_Parameters_Transaction_Req_t *pParamTran;
			void	*pTransParam;
			void	*pTransData;
			int	i;

			pParamTran = (SMB_Parameters_Transaction_Req_t*)pParam->p_Words;

			pPIDMID->pm_State					= TransactionPrimaryRequest;
			pPIDMID->pm_TotalParameterCountReq	= 
											pParamTran->t_TotalParameterCount;
			pPIDMID->pm_TotalDataCountReq		= pParamTran->t_TotalDataCount;

			pPIDMID->pm_TotalParameterCountReq	-= pParamTran->t_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		-= pParamTran->t_DataCount;

			if( pPIDMID->pm_TotalParameterCountReq 
						|| pPIDMID->pm_TotalDataCountReq ) {

				ASSERT( pPIDMID->pm_State == TransmittedAllRequests );

				pPIDMID->pm_State	= TransactionPrimaryRequest;

LOG(pLog, LogDBG, "PID[0x%x] MID[0x%x] -> TransactionPrimaryRequest",
				pPIDMID->pm_PID, pPIDMID->pm_MID );
			}
			pTransParam	= (char*)pHead + pParamTran->t_ParameterOffset;
			pTransData		= (char*)pHead + pParamTran->t_DataOffset;

			for( i = 0; i < pParamTran->t_SetupCount; i++ ) {
LOG( pLog, LogDBG,"%d:Setup[0x%x]", i, pParamTran->t_Setup[i] );
			}
			switch( pParamTran->t_Setup[0] ) {
			case TRANS_SET_NMPIPE_STATE:
			case TRANS_RAW_READ_NMPIPE:
			case TRANS_QUERY_NMPIPE_STATE:
			case TRANS_QUERY_NMPIPE_INFO:
			case TRANS_PEEK_NMPIPE:
			case TRANS_TRANSACT_NMPIPE:
			case TRANS_RAW_WRITE_NMPIPE:
			case TRANS_READ_NMPIPE:
			case TRANS_WRITE_NMPIPE:
				pPair	= PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
											pParamTran->t_Setup[1], FALSE );
				ASSERT( pPair );
				pParamTran->t_Setup[1]	= pPair->p_FID;

				PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				pPair	= NULL;
				break;

			case TRANS_WAIT_NMPIPE:
			case TRANS_CALL_NMPIPE:
//			case TRANS_MAIL_SLOT_WRITE:
				break;
			default:
				ABORT();
			}

			break;
		}

		case SMB_COM_TRANSACTION2:
		{	SMB_Parameters_Transaction2_Req_t *pParamTran;
			void	*pTrans2Param;
			void	*pTrans2Data;
			int	i;

			pParamTran = (SMB_Parameters_Transaction2_Req_t*)pParam->p_Words;

			pPIDMID->pm_State					= TransactionPrimaryRequest;
			pPIDMID->pm_TotalParameterCountReq	= 
											pParamTran->t_TotalParameterCount;
			pPIDMID->pm_TotalDataCountReq		= pParamTran->t_TotalDataCount;

			pPIDMID->pm_TotalParameterCountReq	-= pParamTran->t_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		-= pParamTran->t_DataCount;

			if( pPIDMID->pm_TotalParameterCountReq 
						|| pPIDMID->pm_TotalDataCountReq ) {

				ASSERT( pPIDMID->pm_State == TransmittedAllRequests );

				pPIDMID->pm_State	= TransactionPrimaryRequest;

LOG(pLog, LogDBG, "PID[0x%x] MID[0x%x] -> TransactionPrimaryRequest",
				pPIDMID->pm_PID, pPIDMID->pm_MID );
			}
			pTrans2Param	= (char*)pHead + pParamTran->t_ParameterOffset;
			pTrans2Data		= (char*)pHead + pParamTran->t_DataOffset;

			for( i = 0; i < pParamTran->t_SetupCount; i++ ) {
LOG( pLog, LogDBG,"%d:Setup[0x%x]", i, pParamTran->t_Setup[i] );
				switch( pParamTran->t_Setup[i] ) {
					case TRANS2_OPEN2:
						break;
					case TRANS2_QUERY_FILE_INFORMATION:
					{	SMB_Parameters_Trans2_Query_File_Information_Req_t
						*pParamReq =
						(SMB_Parameters_Trans2_Query_File_Information_Req_t*)
							pTrans2Param;
LOG(pLog, LogDBG, "qf_FID[0x%x] qf_InformationLevel[0x%x]",
pParamReq->qf_FID, pParamReq->qf_InformationLevel );

						if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {

							pPair = PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
												pParamReq->qf_FID, FALSE );
LOG(pLog, LogDBG,"TRANS2_QUERY_FILE_INFORMATION qf_FID=0x%x pPair=%p", 
pParamReq->qf_FID, pPair );
							if(  pPair ) {
								pParamReq->qf_FID	= pPair->p_FID;

								PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
								pPair	= NULL;
							}
						}

						pTrans2Param	+= 4;
						pTrans2Data		+= 4;
		
						break;
					}
				}
			}

			break;
		}

		case SMB_COM_NT_TRANSACT:
		{	SMB_Parameters_NT_Transact_Req_t	*pReq;

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
			{
				NT_Transact_IOCTL_Setup_t *pIOCTL;

				if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {

					pIOCTL	= (NT_Transact_IOCTL_Setup_t*)pReq->n_Setup;

LOG(pLog, LogDBG,"NT_TRANSACT_IOCTL");
					pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
												pIOCTL->i_FID, FALSE );
ASSERT( pPair );
					pIOCTL->i_FID	= pPair->p_FID;

					PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				}
				pPair = NULL; 
				break;
			}
			case NT_TRANSACT_SET_SECURITY_DESC:
				break;
			case NT_TRANSACT_NOTIFY_CHANGE:
			{
				NT_Transact_Notify_Change_Setup_t	*pNotify;

				pNotify	= (NT_Transact_Notify_Change_Setup_t*)pReq->n_Setup;

LOG(pLog, LogDBG, "pNotify=%p CompletionFilter=0x%x", 
	pNotify, pNotify->n_CompletionFilter );
LOG(pLog, LogDBG, "FID=0x%x", pNotify->n_FID );
LOG(pLog, LogDBG, "WatchTree=0x%x", pNotify->n_WatchTree );
				if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {

LOG(pLog, LogDBG,"NT_TRANSACT_NOTIFY_CHANGE");
					pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
												pNotify->n_FID, FALSE );
					pNotify->n_FID	= pPair->p_FID;

					PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				}
				pPair = NULL; 
				pPIDMID->pm_bNotify	= TRUE;
				//CNV_Cmd = CNV_PSUEDO_RPL;

				CNVAcceptReplyByMsg( pAccept, NULL, CNV_Cmd, CNV_PSUEDO_RPL );
				bSendToServer = FALSE;
				break;
			}
			case NT_TRANSACT_RENAME:
				break;
			case NT_TRANSACT_QUERY_SECURITY_DESC:
				break;
			}
			break;
		}
		case SMB_COM_TRANSACTION_SECONDARY:
		{	SMB_Parameters_Transaction_Secondary_Req_t *pParamTran;

			pParamTran = (SMB_Parameters_Transaction_Secondary_Req_t*)
									pParam->p_Words;

			pPIDMID->pm_TotalParameterCountReq	
					-= pParamTran->s_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		
					-= pParamTran->s_DataCount;
			break;
		}
		case SMB_COM_TRANSACTION2_SECONDARY:
		{	SMB_Parameters_Transaction2_Secondary_Req_t *pParamTran2;

			pParamTran2 = (SMB_Parameters_Transaction2_Secondary_Req_t*)
									pParam->p_Words;

			// MetaFID->FID
			if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
LOG(pLog,LogDBG,"SMB_COM_TRANSACTION2_SECONDARY");
				pPair	= (PairFID_t*)PairCoSGetByFIDMeta( &pConCoS->s_Pair, 
												pParamTran2->s_FID, FALSE );
				pParamTran2->s_FID	= pPair->p_FID;

				PairCoSPutFID( &pConCoS->s_Pair, pPair, FALSE );
				pPair = NULL;
			}
			pPIDMID->pm_TotalParameterCountReq	
					-= pParamTran2->s_ParameterCount;
			pPIDMID->pm_TotalDataCountReq		
					-= pParamTran2->s_DataCount;
			break;
		}
		default:
			break;
	}
LOG(pLog,LogDBG,"UID[0x%x] TID[0x%x]", pHead->h_UID, pHead->h_TID );
	if( CtrlCoS.s_Ctrl.s_Snoop == SNOOP_META_ON ) {
		if( bUID && pHead->h_UID && pHead->h_UID == pConCoS->s_UIDMeta ) {
			pHead->h_UID	= pConCoS->s_UID;
LOG(pLog, LogDBG, "UID:0x%x(Meta)->0x%x", pConCoS->s_UIDMeta, pConCoS->s_UID );
		}
#ifdef	ZZZ
		if( pHead->h_TID && pHead->h_TID == pConCoS->s_TIDMeta ) {
			pHead->h_TID	= pConCoS->s_TID;
LOG(pLog, LogDBG, "TID:0x%x(Meta)->0x%x", pConCoS->s_TIDMeta, pConCoS->s_TID );
		}
#else
		if( bTID && pHead->h_TID && pHead->h_TID != 0xffff ) {
			PairTID_t	*pPairTID;

			pPairTID	= PairCoSGetByTIDMeta( &pConCoS->s_Pair, 
								pHead->h_TID, FALSE );

			ASSERT( pPairTID );

			pConCoS->s_TIDMeta	= pHead->h_TID;
			pConCoS->s_TID		= pPairTID->p_TID;

			pHead->h_TID = pPairTID->p_TID;

			PairCoSPutTID( &pConCoS->s_Pair, pPairTID, FALSE );

		}
#endif
	}

skip1:
	// Send to server
	if( bSendToServer ) {
		Ret = SendStream( Fd, (char*)pDataLen, Total + sizeof(uint32_t) );
		if( Ret < 0 ) {
			LOG(pLog, LogDBG, "Ret=%d errno=%d", Ret, errno );
			goto err;
		}
	}
	// Reply  to CoC
	if( pRpl ) {
		CNVAcceptReplyByMsg( pConCoS->s_pAccept, 
								pRpl, pConCoS->s_CnvCmd, Status );
	}
ret:
	return( 0 );
err:
	return( -1 );
}

// Pair( FIDMeta-FID, TIDMeta-TID )
int
PairCoSInit( PairCoS_t *pPairCoS )
{
	MtxInit( &pPairCoS->s_Mtx );
	CndInit( &pPairCoS->s_Cnd );
	list_init( &pPairCoS->s_LnkFID );
	list_init( &pPairCoS->s_LnkTID );
	HashInit( &pPairCoS->s_HashFIDMeta, PRIMARY_1009, 
			HASH_CODE_INT, FIDMetaCmp, NULL, Malloc, Free, NULL);

	HashInit( &pPairCoS->s_HashFID, PRIMARY_1009, 
			HASH_CODE_INT, FIDCmp, NULL, Malloc, Free, NULL);

#ifdef	ZZZ
#else
	HashInit( &pPairCoS->s_HashTIDMeta, PRIMARY_101, 
			HASH_CODE_INT, HASH_CMP_INT, NULL, Malloc, Free, NULL);

	HashInit( &pPairCoS->s_HashTID, PRIMARY_101, 
			HASH_CODE_INT, HASH_CMP_INT, NULL, Malloc, Free, NULL);
#endif

	return( 0 );
}

PairFID_t*
PairCoSGetByFIDMeta( PairCoS_t *pPairCoS, uint16_t FIDMeta, bool_t bCreate )
{
	PairFID_t	*pPair;

	if( !FIDMeta ) {
		LOG( pLog, LogDBG, "FIDMeta == 0");
		return( NULL );
	}
	MtxLock( &pPairCoS->s_Mtx );

	pPair	= (PairFID_t*)HashGet( &pPairCoS->s_HashFIDMeta, 
								(void*)(uintptr_t)FIDMeta );
LOG(pLog, LogDBG, 
"bCreate=%d pPair=%p FIDMeta=0x%x", bCreate, pPair, FIDMeta );
if( pPair )	{
LOG( pLog, LogDBG, "p_FIDMeta=0x%x p_FID=0x%x p_Cnt=%d",
pPair->p_FIDMeta, pPair->p_FID, pPair->p_Cnt );
}
	if( !pPair && bCreate ) {
		pPair	= (PairFID_t*)Malloc( sizeof(*pPair) );
		memset( pPair, 0, sizeof(*pPair) );
		list_init( &pPair->p_Lnk );
		pPair->p_FIDMeta	= FIDMeta;

		list_add_tail( &pPair->p_Lnk, &pPairCoS->s_LnkFID );
		HashPut( &pPairCoS->s_HashFIDMeta, (void*)(uintptr_t)FIDMeta, pPair );
	}
	if( pPair )	pPair->p_Cnt++;

	MtxUnlock( &pPairCoS->s_Mtx );
	return( pPair );
}

int
PairCoSSetFID( PairCoS_t *pPairCoS, PairFID_t *pPair, uint16_t FID )
{
	ASSERT( pPair->p_FID == 0 );

	MtxLock( &pPairCoS->s_Mtx );

LOG(pLog, LogDBG,
"pPair=%p p_Cnt=%d p_FIDMeta=0x%x p_FID=0x%x FID=0x%x", 
pPair, pPair->p_Cnt, pPair->p_FIDMeta, pPair->p_FID, FID );
	pPair->p_FID	= FID;
	HashPut( &pPairCoS->s_HashFID, (void*)(uintptr_t)FID, pPair );

	MtxUnlock( &pPairCoS->s_Mtx );
	return( 0 );
}

int
PairCoSPutFID( PairCoS_t *pPairCoS, PairFID_t *pPair, bool_t bDestroy )
{
	MtxLock( &pPairCoS->s_Mtx );

LOG(pLog, LogDBG,
"bDestroy=%d pPair=%p p_Cnt=%d p_FIDMeta=0x%x p_FID=0x%x", 
bDestroy, pPair, pPair->p_Cnt, pPair->p_FIDMeta, pPair->p_FID );
	--pPair->p_Cnt;
ASSERT( pPair->p_Cnt >= 0 );
	if( bDestroy && pPair->p_Cnt == 0 ) {

		list_del( &pPair->p_Lnk );

		HashRemove( &pPairCoS->s_HashFID, (void*)(uintptr_t)pPair->p_FID );
		HashRemove( &pPairCoS->s_HashFIDMeta, 
							(void*)(uintptr_t)pPair->p_FIDMeta );
		Free( pPair );
	}
	MtxUnlock( &pPairCoS->s_Mtx );
	return( 0 );
}

#ifdef	ZZZ
#else
PairTID_t*
PairCoSGetByTIDMeta( PairCoS_t *pPairCoS, uint16_t TIDMeta, bool_t bCreate )
{
	PairTID_t	*pPair;

	if( !TIDMeta ) {
		LOG( pLog, LogDBG, "TIDMeta == 0");
		return( NULL );
	}
	MtxLock( &pPairCoS->s_Mtx );

	pPair	= (PairTID_t*)HashGet( &pPairCoS->s_HashTIDMeta, 
								(void*)(uintptr_t)TIDMeta );
LOG(pLog, LogDBG, 
"bCreate=%d pPair=%p TIDMeta=0x%x", bCreate, pPair, TIDMeta );
if( pPair )	{
LOG( pLog, LogDBG, "p_TIDMeta=0x%x p_TID=0x%x p_Cnt=%d",
pPair->p_TIDMeta, pPair->p_TID, pPair->p_Cnt );
}
	if( !pPair && bCreate ) {
		pPair	= (PairTID_t*)Malloc( sizeof(*pPair) );
		memset( pPair, 0, sizeof(*pPair) );
		list_init( &pPair->p_Lnk );
		pPair->p_TIDMeta	= TIDMeta;

		list_add_tail( &pPair->p_Lnk, &pPairCoS->s_LnkTID );
		HashPut( &pPairCoS->s_HashTIDMeta, (void*)(uintptr_t)TIDMeta, pPair );
	}
	if( pPair )	pPair->p_Cnt++;

	MtxUnlock( &pPairCoS->s_Mtx );
	return( pPair );
}

int
PairCoSSetTID( PairCoS_t *pPairCoS, PairTID_t *pPair, uint16_t TID )
{
	ASSERT( pPair->p_TID == 0 );

	MtxLock( &pPairCoS->s_Mtx );

LOG(pLog, LogDBG,
"pPair=%p p_Cnt=%d p_TIDMeta=0x%x p_TID=0x%x TID=0x%x", 
pPair, pPair->p_Cnt, pPair->p_TIDMeta, pPair->p_TID, TID );
	pPair->p_TID	= TID;
	HashPut( &pPairCoS->s_HashTID, (void*)(uintptr_t)TID, pPair );

	MtxUnlock( &pPairCoS->s_Mtx );
	return( 0 );
}

PairTID_t*
PairCoSGetByTID( PairCoS_t *pPairCoS, uint16_t TID )
{
	PairTID_t	*pPair;

	if( !TID ) {
		LOG( pLog, LogDBG, "TID == 0");
		return( NULL );
	}
	MtxLock( &pPairCoS->s_Mtx );

	pPair	= (PairTID_t*)HashGet( &pPairCoS->s_HashTID, (void*)(uintptr_t)TID );
if( pPair )	{
LOG( pLog, LogDBG, "p_TIDMeta=0x%x p_TID=0x%x p_Cnt=%d",
pPair->p_TIDMeta, pPair->p_TID, pPair->p_Cnt );
} else {
LOG( pLog, LogDBG, "TID=0x%x", TID );
}
	if( pPair )	pPair->p_Cnt++;

	MtxUnlock( &pPairCoS->s_Mtx );

	return( pPair );
}

int
PairCoSPutTID( PairCoS_t *pPairCoS, PairTID_t *pPair, bool_t bDestroy )
{
	MtxLock( &pPairCoS->s_Mtx );

LOG(pLog, LogDBG,
"bDestroy=%d pPair=%p p_Cnt=%d p_TIDMeta=0x%x p_TID=0x%x", 
bDestroy, pPair, pPair->p_Cnt, pPair->p_TIDMeta, pPair->p_TID );
	--pPair->p_Cnt;
ASSERT( pPair->p_Cnt >= 0 );
	if( bDestroy && pPair->p_Cnt == 0 ) {

		list_del( &pPair->p_Lnk );

		HashRemove( &pPairCoS->s_HashTID, (void*)(uintptr_t)pPair->p_TID );
		HashRemove( &pPairCoS->s_HashTIDMeta, 
							(void*)(uintptr_t)pPair->p_TIDMeta );
		Free( pPair );
	}
	MtxUnlock( &pPairCoS->s_Mtx );
	return( 0 );
}
#endif

int
ConnectToServer( ConCoS_t *pConCoS, struct sockaddr_in *pAddr )
{
	int Fd;
	int flags = 1;
	struct linger	Linger = { 1/*On*/, 10/*sec*/};
	uint64_t	U64S;
	int	Ret;
	FdEventCtrl_t	*pFdEvCtrlServer;

	pFdEvCtrlServer	= &CtrlCoS.s_Ctrl.s_FdEventCtrl;

	// connect
	if( (Fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err;
	}
	if( connect( Fd, (struct sockaddr*)pAddr, sizeof(*pAddr) ) < 0 ) {
		goto err;
	}
LOG(pLog, LogDBG, "Fd[%d]", Fd );
	// Set Linger, No delay
	setsockopt( Fd, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger) );
	setsockopt( Fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

	// event
	FdEventInit( &pConCoS->s_FdEvent, 0/*FD_TYPE_SERVER*/, Fd, 
						FD_EVENT_READ, pConCoS, HandlerRecvFromCIFS );
	U64S = Fd;
	Ret = FdEventAdd( pFdEvCtrlServer, U64S, &pConCoS->s_FdEvent );

	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
CIFSAdaptorOpen( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	ConCoS_t	*pConCoS;

	PaxosClientId_t	*pId;

	pId	= PaxosAcceptorGetClientId( pAccept );
LOG(pLog, LogDBG, "No[%d]", pId->i_No);

	pConCoS	= (ConCoS_t*)Malloc( sizeof(*pConCoS) );
	memset( pConCoS, 0, sizeof(*pConCoS) );

	list_init( &pConCoS->s_Lnk );
	MtxInit( &pConCoS->s_Mtx );

	PIDMIDInit( &pConCoS->s_PIDMID );

	PairCoSInit( &pConCoS->s_Pair );

	if( pId->i_No != CNV_EVENT_NO ) {
		ConnectToServer( pConCoS, &Server );
	} else {
		HashPut( &CtrlCoS.s_Ctrl.s_HashEvent, pId, pAccept );
	}

	pConCoS->s_pAccept	= pAccept;
	PaxosAcceptSetTag( pAccept, CNV_FAMILY, pConCoS );

	return( 0 );
}

int
CIFSAdaptorClose( struct PaxosAccept *pAccept, PaxosSessionHead_t *pM )
{
	ConCoS_t	*pConCoS;
	PaxosClientId_t	*pId;

	pId	= PaxosAcceptorGetClientId( pAccept );
LOG(pLog, LogDBG, "No[%d]", pId->i_No);

	pConCoS	= (ConCoS_t*)PaxosAcceptGetTag( pAccept,CNV_FAMILY );
	PaxosAcceptSetTag( pAccept, CNV_FAMILY, NULL );

	if( pId->i_No != CNV_EVENT_NO ) {
		FdEventDel( &pConCoS->s_FdEvent );
	} else {
		HashRemove( &CtrlCoS.s_Ctrl.s_HashEvent, pId );
	}

	Free( pConCoS );

	return( 0 );
}

CnvCtrlCoS_t*
CIFSInitCoS( void *pVoid )
{
#ifdef	ZZZ
	pthread_create( &CtrlCoS.s_Ctrl.s_FdEvCtrlServer.ct_Th, NULL, 
					ThreadEventLoopServer, NULL );
#else
//	CtrlCoS.s_Ctrl.s_bFdEventCtrl	= TRUE;
#endif

	return( &CtrlCoS.s_Ctrl );
}

int
CIFSEventBind( struct PaxosAccept *pNormal, struct PaxosAccept *pEvent )
{
	ConCoS_t	*pNormalCoS;

	pNormalCoS	= (ConCoS_t*)PaxosAcceptGetTag( pNormal, CNV_FAMILY );

	pNormalCoS->s_pEvent	= pEvent;

	return( 0 );
}

/*
 *	Interface
 */
CnvIF_t	CIFS_IF = {
	.IF_fOpen	= CIFSAdaptorOpen,	
	.IF_fClose	= CIFSAdaptorClose,	
	.IF_fRecvMsgByFd	= CIFSRecvMsgByFd,	// recv from Server bare CIFS
	.IF_fRequestToServer	= CIFSRequestToServer,	// Request to Server

	.IF_fInitCoS	= CIFSInitCoS,
	.IF_fEventBind	= CIFSEventBind,
};

