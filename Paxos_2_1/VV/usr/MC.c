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

#include	"VV.h"

static	MCCtrl_t	*pMCCtrl;

GElm_t*
_MCAlloc( GElmCtrl_t *pGE )
{
	MC_t	*pMC;

	pMC	= (MC_t*)Malloc( sizeof(*pMC) );
	if( !pMC )	goto err;

	memset( pMC, 0, sizeof(*pMC) );

	return( &pMC->mc_GE );
err:
	return( NULL );
}

int
_MCFree( GElmCtrl_t *pGE, GElm_t *pElm )
{
	MC_t	*pMC = container_of( pElm, MC_t, mc_GE );

	Free( pMC );

	return( 0 );
}
int
_MCOpen(GElmCtrl_t *pGE, GElm_t *pElm, void *pKey, void **ppKey, 
						void* pArg, bool_t bCreate)
{
	MC_t	*pMC = container_of( pElm, MC_t, mc_GE );
	struct Session	*pSession;

	pSession = VVSessionOpen( pKey, 0, TRUE );
	if( !pSession )	goto err;

	strncpy( pMC->mc_Cell.c_Name, pKey, sizeof(pMC->mc_Cell.c_Name) );

	pMC->mc_pSession	= pSession;
	*ppKey  = pMC->mc_Cell.c_Name;
	return( 0 );
err:
	return( -1 );
}

int
_MCClose( GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave )
{
	MC_t	*pMC = container_of( pElm, MC_t, mc_GE );

	ASSERT( pMC->mc_pSession );

	VVSessionClose( pMC->mc_pSession );

	pMC->mc_pSession	= NULL;

	*ppKey = pMC->mc_Cell.c_Name;

	return( 0 );
}

MC_t*
MCGet( char *pName )
{
	GElm_t	*pElm;
	MC_t	*pMC = NULL;

	pElm	= GElmGet( &pMCCtrl->mcc_Ctrl, pName, NULL, TRUE, TRUE );
	if( !pElm ) {
		errno = ENOENT;
		goto err;
	}
	pMC = container_of( pElm, MC_t, mc_GE );
	
	return( pMC );
err:
	return( NULL );
}

int
MCPut( MC_t *pMC, bool_t bFree )
{
	int	Ret;

	Ret = GElmPut( &pMCCtrl->mcc_Ctrl, &pMC->mc_GE, bFree, bFree );

	return( Ret );
}

int
_MCLoop( GElmCtrl_t *pGE, GElm_t *pElm, void *pVoid )
{
	return( 0 );
}

int
MCInit( int MCMax, MCCtrl_t *pMC )
{
	pMCCtrl	= pMC;

	GElmCtrlInit( &pMCCtrl->mcc_Ctrl, NULL, _MCAlloc, _MCFree, 
		_MCOpen, _MCClose, _MCLoop,
		MCMax, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, 
		Malloc ,Free, NULL );	

	return( 0 );
}

int
MCReadIO( LS_t *pLS, list_t *pEnt )
{
	MC_t	*pMC;
	int	Ret;

	pMC	= MCGet( pLS->ls_Cell );
	if( !pMC ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = LVCompoundRead( pMC->mc_pSession, pLS, pEnt );
	if( Ret < 0 )	goto err2;

	MCPut( pMC, FALSE );
	return( 0 );
err2:
	MCPut( pMC, TRUE );
err1:
	return( -1 );
}

int
MCWriteIO( LS_t *pLS, list_t *pEnt )
{
	MC_t	*pMC;
	int	Ret;

	pMC	= MCGet( pLS->ls_Cell );
	if( !pMC ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = LVCompoundWrite( pMC->mc_pSession, pLS, pEnt );
	if( Ret < 0 )	goto err2;

	MCPut( pMC, FALSE );
	return( 0 );
err2:
	MCPut( pMC, TRUE );
err1:
	return( -1 );
}

int
MCCopyIO( LS_IO_t *pLS_IO )
{
	MC_t	*pMCFr, *pMCTo;
	LV_IO_t	*pIO;
	int	Ret;
	list_t	LnkIO;
	BTCursor_t	Cursor;

	pMCFr	= MCGet( pLS_IO->l_LS.ls_Cell );
	if( !pMCFr ) {
		errno = ENOENT;
		goto err1;
	}
	pMCTo	= MCGet( pLS_IO->l_LSTo.ls_Cell );
	if( !pMCTo ) {
		errno = ENOENT;
		goto err2;
	}
	list_init( &LnkIO );

	BTCursorOpen( &Cursor, &pLS_IO->l_ReqLV_IO );
	BTCHead( &Cursor );
	while( (pIO = BTCGetKey( LV_IO_t*,&Cursor) ) ) {
		list_init( &pIO->io_Lnk );
		list_add_tail( &pIO->io_Lnk, &LnkIO );
		if( BTCNext( &Cursor ) )	break;
	}
	BTCursorClose( &Cursor );

	Ret = LVCompoundCopy( pMCFr->mc_pSession, &pLS_IO->l_LS, &LnkIO,
							pMCTo->mc_pSession, &pLS_IO->l_LSTo );
	if( Ret < 0 )	goto err3;

	MCPut( pMCFr, FALSE );
	MCPut( pMCTo, FALSE );
	return( 0 );
err3:
	MCPut( pMCTo, TRUE );
err2:
	MCPut( pMCFr, TRUE );
err1:
	return( -1 );
}

int
MCFlushAll( char *pCell )
{
	Msg_t	*pMsgReq, *pMsgRpl;
	MsgVec_t	Vec;
	LV_cacheout_req_t	Req;
	LV_cacheout_rpl_t	*pRpl;
	int	Ret;
	MC_t	*pMC;

	pMC	= MCGet( pCell );
	if( !pMC )	goto err;

	pMsgReq	= MsgCreate( 3, Malloc, Free );

	SESSION_MSGINIT( &Req.c_Head, LV_CACHEOUT, 0, 0, sizeof(Req) );
	
	MsgVecSet( &Vec, VEC_HEAD|VEC_REPLY, &Req, sizeof(Req), sizeof(Req) );
	MsgAdd( pMsgReq, &Vec );

	pMsgRpl = PaxosSessionReqAndRplByMsg( pMC->mc_pSession, 
									pMsgReq, FALSE, FALSE );
	pRpl	= (LV_cacheout_rpl_t*)MsgFirst( pMsgRpl );
	if( !pRpl ) {
		Ret = -1;
	} else {
		Ret	= pRpl->c_Head.h_Error;
	}
	MsgDestroy( pMsgRpl );

	MCPut( pMC, FALSE );
	return( Ret );
err:
	return( -1 );
}

int
MCDelete( LS_t *pLS )
{
	int	Ret;
	MC_t	*pMC;

	pMC	= MCGet( pLS->ls_Cell );
	if( !pMC )	goto err;

	Ret = LVDelete( pMC->mc_pSession, pLS );
	if( Ret < 0 )	goto err;

	MCPut( pMC, FALSE );
	return( 0 );
err:
	MCPut( pMC, TRUE );
	return( -1 );
}
