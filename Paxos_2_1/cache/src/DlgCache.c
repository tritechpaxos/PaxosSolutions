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

#include	"DlgCache.h"

int
DCallback( struct Session *pSessionEvent, PFSHead_t *pF )
{
	DlgCache_t	*pD;
	PFSEventLock_t *pEv = (PFSEventLock_t*)pF;
	DlgCacheCtrl_t	*pDC;
	DlgCacheRecall_t	*pRC;

	LOG(PaxosSessionGetLog(pSessionEvent), LogINF,
		"ENT:el_Name[%s]", pEv->el_Name );

	ASSERT( PFS_EVENT_LOCK == (int)pF->h_Head.h_Cmd );

	pRC	= (DlgCacheRecall_t*)PaxosSessionGetTag( pSessionEvent );
	pRC->rc_EventCount++;

	list_for_each_entry( pDC, &pRC->rc_LnkCtrl, dc_Lnk ) {

		pD = DlgCacheGet( pDC, pEv->el_Name, FALSE, 0 );
		if( pD )	break;
	} 
	if( pD ) {
		PFSDlgRecall( pEv, &pD->d_Dlg, pD );

		DlgCachePut( pD, FALSE );
	}
	LOG(PaxosSessionGetLog(pSessionEvent), LogINF,
		"EXT:el_Name[%s] pD=%p", pEv->el_Name, pD );
	return( 0 );
}

void*
DCThreadEvent( void *pArg )
{
	struct Session *pE = (struct Session*)pArg;
	int32_t	count, omitted;
	int	Ret;

	while( 1 ) {
		Ret = PFSEventGetByCallback( pE, &count, &omitted,
										DCallback );
		if( Ret )	break;

		LOG( PaxosSessionGetLog(pE),LogINF,
			"EventGet:Total Ret[%d]  Master is %d.", 
			Ret, PaxosSessionGetMaster( pE ) );
	}
	pthread_exit( 0 );
	return( NULL );
}

int
DlgCacheRecallStart( DlgCacheRecall_t *pRC, struct Session *pEvent )
{
	list_init( &pRC->rc_LnkCtrl );
	pRC->rc_pEvent	= pEvent;

	// Event thread
	PaxosSessionSetTag( pEvent, pRC );

	pthread_create( &pRC->rc_Thread, NULL, DCThreadEvent, (void*)pEvent );

	return( 0 );
}

int
DlgCacheRecallStop( DlgCacheRecall_t *pRC )
{
	pthread_cancel( pRC->rc_Thread );
	pthread_join( pRC->rc_Thread, NULL );

	return( 0 );
}


GElm_t*
_DAlloc( GElmCtrl_t *pGEC )
{
	DlgCacheCtrl_t	*pDC = container_of( pGEC, DlgCacheCtrl_t, dc_Ctrl );
	DlgCache_t	*pD;

	if( pDC->dc_fAlloc ) pD = (*pDC->dc_fAlloc)( pDC );
	else {
		pD	= (DlgCache_t*)Malloc( sizeof(*pD) );
		memset( pD, 0, sizeof(*pD) );
	}
	pD->d_pCtrl	= pDC;

LOG( pDC->dc_pLog, LogDBG, "pDC=%p", pDC );
	return( &pD->d_GE );
}

int
_DFree( GElmCtrl_t *pGEC, GElm_t *pElm )
{
	DlgCacheCtrl_t	*pDC = container_of( pGEC, DlgCacheCtrl_t, dc_Ctrl );
	DlgCache_t	*pD = container_of( pElm, DlgCache_t, d_GE );

	ASSERT( pD->d_Dlg.d_Own == 0 );
	if( pDC->dc_fFree ) (*pDC->dc_fFree)( pDC, pD );
	else				Free( pD );

LOG( pDC->dc_pLog, LogDBG, "pDC=%p", pDC );
	return( 0 );
}

int _DInit( PFSDlg_t *pElm, void *pArg );
int _DTerm( PFSDlg_t *pElm, void *pArg );

int
_DSetDlgInitEvSet(GElmCtrl_t *pGEC, GElm_t *pElm, void *pKey, void **ppKey, 
						void *pTag, bool_t bLoad )
{
	DlgCache_t	*pD = container_of( pElm, DlgCache_t, d_GE );
	DlgCacheCtrl_t	*pDC = container_of( pGEC, DlgCacheCtrl_t, dc_Ctrl );
	
	PFSDlgInit( pDC->dc_pSession, pDC->dc_pEvent, &pD->d_Dlg, pKey, 
				_DInit, _DTerm, pTag  );

	if( bLoad && pDC->dc_fSet ) {
		(*pDC->dc_fSet)( pDC, pD );
	}
	PFSDlgEventSet( &pD->d_Dlg );

	*ppKey	= pD->d_Dlg.d_Name;

	LOG( pDC->dc_pLog, LogINF, "[%s] bLoad=%d", pD->d_Dlg.d_Name, bLoad );

	return( 0 );
}

int
_DSetDlgInitEvSetBy(GElmCtrl_t *pGEC, GElm_t *pElm, void *pKey, void **ppKey, 
						void *pTag, bool_t bLoad )
{
	DlgCache_t	*pD = container_of( pElm, DlgCache_t, d_GE );
	DlgCacheCtrl_t	*pDC = container_of( pGEC, DlgCacheCtrl_t, dc_Ctrl );
	
	PFSDlgInitByPNT( pDC->dc_pSession, pDC->dc_pEvent, &pD->d_Dlg, pKey, 
				_DInit, _DTerm, pTag  );

	if( bLoad && pDC->dc_fSet ) {
		(*pDC->dc_fSet)( pDC, pD );
	}
	PFSDlgEventSet( &pD->d_Dlg );

	*ppKey	= pD->d_Dlg.d_Name;

	LOG( pDC->dc_pLog, LogINF, "[%s] bLoad=%d", pD->d_Dlg.d_Name, bLoad );

	return( 0 );
}

int
_DUnsetCancelReturn(GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave)
{
	DlgCache_t	*pD = container_of( pElm, DlgCache_t, d_GE );
	DlgCacheCtrl_t	*pDC = container_of( pGE, DlgCacheCtrl_t, dc_Ctrl );

	LOG( pDC->dc_pLog, LogINF, "[%s] bSave=%d", pD->d_Dlg.d_Name, bSave );

	if( bSave && pDC->dc_fUnset ) {
		(*pDC->dc_fUnset)( pDC, pD );
	}
	PFSDlgEventCancel( &pD->d_Dlg );

	if( pD->d_Dlg.d_Own )	PFSDlgReturn( &pD->d_Dlg, pD );

	*ppKey	= pD->d_Dlg.d_Name;
	return( 0 );
}

int
_DInit( PFSDlg_t *pDlg, void *pArg )
{
	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
	int	Ret = 0;

	DlgInitOn( pD );
	if( pDC->dc_fInit )	Ret = (*pDC->dc_fInit)( pD, pArg );
	TIMESPEC( pD->d_Timespec );

LOG( pDC->dc_pLog, LogDBG, "pD=%p pArg=%p Ret=%d", pD, pArg, Ret );
	return( Ret );
}

int
_DTerm( PFSDlg_t *pDlg, void *pArg )
{
	DlgCache_t	*pD = container_of( pDlg, DlgCache_t, d_Dlg );
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
	int	Ret = 0;

	if( pDC->dc_fTerm )	Ret = (*pDC->dc_fTerm)( pD, pArg );
LOG( pDC->dc_pLog, LogDBG, "pD=%p pArg=%p Ret=%d", pD, pArg, Ret );
	return( Ret );
}

#define	DLG_DESTROY	(void*)-1

int
_DLoop( GElmCtrl_t *pGE, GElm_t *pElm, void *pVoid )
{
	DlgCache_t	*pD = container_of( pElm, DlgCache_t, d_GE );
	DlgCacheCtrl_t	*pDC = container_of( pGE, DlgCacheCtrl_t, dc_Ctrl );
	int	Ret;

	LOG( pDC->dc_pLog, LogINF, 
	"[%s] pD=%p pVoid=%p", pD->d_Dlg.d_Name, pD, pVoid );

	if( DLG_DESTROY == pVoid ) {
		_GElmUnset( pGE, pElm, TRUE );	// remove Hash
		list_del_init( &pElm->e_Lnk );
		_GElmFree( pGE, pElm );			// Free
		Ret = 0;
	} else if( pDC->dc_fLoop ) {
		Ret	= (*pDC->dc_fLoop)( pD, pVoid );
	} else {
		Ret = -1;
	}
	return( Ret );
}

void
_DDestroy( Hash_t *pHash, void *pVoid )
{
	DlgCache_t	*pD = (DlgCache_t*)pVoid;
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;

	LOG( pDC->dc_pLog, LogINF, "[%s]", pD->d_Dlg.d_Name );

	if( pDC->dc_fUnset ) {
		(*pDC->dc_fUnset)( pDC, pD );
	}
	PFSDlgReturn( &pD->d_Dlg, pD );

	PFSDlgEventCancel( &pD->d_Dlg );

	list_del( &pDC->dc_Lnk );

	if( pDC->dc_fFree )	(*pDC->dc_fFree)( pDC, pD );
	else				Free( pD );
}

int
DlgCacheInit( struct Session *pSession, DlgCacheRecall_t *pRC,
				DlgCacheCtrl_t *pDC, int MaxD, 
				DlgCache_t	*(*fAlloc)(DlgCacheCtrl_t*), 
				int		(*fFree)(DlgCacheCtrl_t*,DlgCache_t*), 
				int		(*fSet)(DlgCacheCtrl_t*,DlgCache_t*),
				int		(*fUnset)(DlgCacheCtrl_t*,DlgCache_t*),
				int 	(*fInit)(DlgCache_t*,void *pArg),
				int 	(*fTerm)(DlgCache_t*,void *pArg),
				int		(*fLoop)(DlgCache_t*,void *pArg),
				struct Log *pLog )
{
	GElmCtrlInit( &pDC->dc_Ctrl, NULL, _DAlloc, _DFree, 
		_DSetDlgInitEvSet, _DUnsetCancelReturn, _DLoop,
		MaxD, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, 
		Malloc ,Free, _DDestroy );	

	pDC->dc_fAlloc	= fAlloc;
	pDC->dc_fFree	= fFree;
	pDC->dc_fSet	= fSet;
	pDC->dc_fUnset	= fUnset;
	pDC->dc_fInit	= fInit;
	pDC->dc_fTerm	= fTerm;
	pDC->dc_fLoop	= fLoop;
	pDC->dc_pLog	= pLog;

	pDC->dc_pSession	= pSession;
	pDC->dc_pEvent		= pRC->rc_pEvent;

	list_init( &pDC->dc_Lnk );
	list_add_tail( &pDC->dc_Lnk, &pRC->rc_LnkCtrl );

	// Event thread
//	PaxosSessionSetTag( pEvent, pDC );

//	pthread_create( &pDC->dc_Thread, NULL, DCThreadEvent, (void*)pEvent );

	return( 0 );
}

int
DlgCacheInitBy( struct Session *pSession, DlgCacheRecall_t *pRC,
				DlgCacheCtrl_t *pDC, int MaxD, 
				DlgCache_t	*(*fAlloc)(DlgCacheCtrl_t*), 
				int		(*fFree)(DlgCacheCtrl_t*,DlgCache_t*), 
				int		(*fSet)(DlgCacheCtrl_t*,DlgCache_t*),
				int		(*fUnset)(DlgCacheCtrl_t*,DlgCache_t*),
				int 	(*fInit)(DlgCache_t*,void *pArg),
				int 	(*fTerm)(DlgCache_t*,void *pArg),
				int		(*fLoop)(DlgCache_t*,void *pArg),
				struct Log *pLog )
{
	GElmCtrlInit( &pDC->dc_Ctrl, NULL, _DAlloc, _DFree, 
		_DSetDlgInitEvSetBy, _DUnsetCancelReturn, _DLoop,
		MaxD, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, 
		Malloc ,Free, _DDestroy );	

	pDC->dc_fAlloc	= fAlloc;
	pDC->dc_fFree	= fFree;
	pDC->dc_fSet	= fSet;
	pDC->dc_fUnset	= fUnset;
	pDC->dc_fInit	= fInit;
	pDC->dc_fTerm	= fTerm;
	pDC->dc_fLoop	= fLoop;
	pDC->dc_pLog	= pLog;

	pDC->dc_pSession	= pSession;
	pDC->dc_pEvent		= pRC->rc_pEvent;

	list_init( &pDC->dc_Lnk );
	list_add_tail( &pDC->dc_Lnk, &pRC->rc_LnkCtrl );

	return( 0 );
}

int
DlgCacheDestroy( DlgCacheCtrl_t *pDC )
{
	int	Ret;

	Ret = GElmLoop( &pDC->dc_Ctrl, NULL, 
					offsetof(DlgCache_t,d_GE), DLG_DESTROY );

	return( Ret );
}

int
DlgCacheLoop( DlgCacheCtrl_t *pDC, void *pArg )
{
	int	Ret;

	Ret = GElmLoop( &pDC->dc_Ctrl, NULL, 
					offsetof(DlgCache_t,d_GE), pArg );

	return( Ret );
}

int
DlgCacheFlush( DlgCacheCtrl_t *pDC, void *pArg, bool_t bSave )
{
	int	Ret;

	Ret = GElmFlush( &pDC->dc_Ctrl, NULL, 
					offsetof(DlgCache_t,d_GE), pArg, bSave );

	return( Ret );
}

DlgCache_t*
DlgCacheGet( DlgCacheCtrl_t *pDC, char *pName, 
						bool_t bCreate, void *pTag )
{
	DlgCache_t	*pD = NULL;
	GElm_t		*pElm;


	pElm	= GElmGet( &pDC->dc_Ctrl, pName, pTag, bCreate, bCreate );
	if( pElm ) {
		pD = container_of( pElm, DlgCache_t, d_GE );
	}
//	LOG( pDC->dc_pLog, "[%s] pD=%p", pName, pD );

	return( pD );
}

int
DlgCachePut( DlgCache_t *pD, bool_t bFree )
{
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
	int	Ret;

//	LOG( pDC->dc_pLog, "[%s] pD=%p", pD->d_Dlg.d_Name, pD );

	Ret = GElmPut( &pDC->dc_Ctrl, &pD->d_GE, bFree, bFree );

	return( Ret );
}

DlgCache_t*
DlgCacheLockR( DlgCacheCtrl_t *pDC, char *pName, 
				bool_t bCreate, void *pTag, void *pArg )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgHoldR( &pD->d_Dlg, pArg );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

DlgCache_t*
DlgCacheLockW( DlgCacheCtrl_t *pDC, char *pName, 
				bool_t bCreate, void *pTag, void *pArg )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgHoldW( &pD->d_Dlg, pArg );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

DlgCache_t*
DlgCacheLockRBy( DlgCacheCtrl_t *pDC, char *pName, 
		bool_t bCreate, void *pTag, void *pArg, void *pHolder )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgHoldRBy( &pD->d_Dlg, pArg, pHolder );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

DlgCache_t*
DlgCacheLockWBy( DlgCacheCtrl_t *pDC, char *pName, 
		bool_t bCreate, void *pTag, void *pArg, void *pHolder )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgHoldWBy( &pD->d_Dlg, pArg, pHolder );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

int
DlgCacheReturnBy( DlgCache_t *pD, void *pArg, void *pHolder )
{
	int Ret;

	Ret = PFSDlgReturnBy( &pD->d_Dlg, pArg, pHolder );

	return( Ret );
}

DlgCache_t*
DlgCacheTryLockR( DlgCacheCtrl_t *pDC, char *pName, 
				bool_t bCreate, void *pTag, void *pArg )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgTryHoldR( &pD->d_Dlg, pArg );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

DlgCache_t*
DlgCacheTryLockW( DlgCacheCtrl_t *pDC, char *pName, 
				bool_t bCreate, void *pTag, void *pArg )
{
	DlgCache_t	*pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, bCreate, pTag );
	if( !pD )	goto err;

	Ret = PFSDlgTryHoldW( &pD->d_Dlg, pArg );
	if( Ret <  0 ) {
		DlgCachePut( pD, TRUE );
		goto err;
	}
	return( pD );
err:
	return( NULL );
}

int
DlgCacheUnlock( DlgCache_t *pD, bool_t bFree )
{
	int Ret;

	Ret = PFSDlgRelease( &pD->d_Dlg );

	DlgCachePut( pD, bFree );

	return( Ret );
}

int
DlgCacheUnlockBy( DlgCache_t *pD, bool_t bFree, void *pHolder )
{
	int Ret;

	Ret = PFSDlgReleaseBy( &pD->d_Dlg, pHolder );

	DlgCachePut( pD, bFree );

	return( Ret );
}

int
DlgCacheUnlockByKey( DlgCacheCtrl_t *pDC, char *pName )
{
	DlgCache_t *pD;
	int Ret;

	pD = DlgCacheGet( pDC, pName, FALSE, NULL );
	if( !pD ) {
		errno = ENOENT;
		Ret = -1;
		goto err;
	}
	Ret = PFSDlgRelease( &pD->d_Dlg );

	DlgCachePut( pD, FALSE );
	DlgCachePut( pD, FALSE );
err:
	return( Ret );
}

int
DlgCacheReturn( DlgCache_t *pD, void *pArg )
{
	int Ret;

	Ret = PFSDlgReturn( &pD->d_Dlg, pArg );

	return( Ret );
}


