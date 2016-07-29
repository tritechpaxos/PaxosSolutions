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
#include	<inttypes.h>

extern	void		*pDBMgr;
extern	VVCtrl_t	VVCtrl;

int	VVDeleteVolume( void *pDBMgr, char *pVolName );

int
VVCreateLV( void *pDBMgr, char *pLV, char *pCell, char *pPool, int Segments )
{
	int Ret;

	Ret = DBCreateLV( pDBMgr, pLV, pCell, pPool, Segments );
	return( Ret );
}

int
VVDeleteLV( void *pDBMgr, char *pLV )
{
	int Ret;

	Ret = DBDeleteLV( pDBMgr, pLV );
	return( Ret );
}

int
VVMoveLV( void *pD, char *pFrLV, char *pToLV )
{
	int Ret;

	Ret = DBMoveLV( pD, pFrLV, pToLV );
	return( Ret );
}

int
VVIncLV( void *pDBMgr, char *pLV, int Segments )
{
	int Ret;

	Ret = DBIncLV( pDBMgr, pLV, Segments );
	return( Ret );
}

int
VVCreateVolume( void *pDBMgr, char *pVolName, 
			char *pTarget, long long Size, int Stripes, char *pPool )
{
	int Ret;

	Ret = DBCreateVV( pDBMgr, pVolName, pTarget, 
					(uint64_t)Size, Stripes, pPool );
	return( Ret );
}

int
VVCreateSnap( void *pDBMgr, char *pVolName, char *pSnapName )
{
	Vol_t		*pVl;
	DlgCache_t	*pD;
	int Ret;

	//  volume write lock
	pD = VL_LockW( pVolName );
	if( !pD )	goto err2;

	pVl = container_of( pD, Vol_t, vl_Dlg );

	Ret = DBCreateSnap( pDBMgr, pVl, pSnapName );

	// Release volume lock is TRUE to get updated one
	VL_Unlock( pD, TRUE );

	return( Ret );
err2:
	return( -1 );
}

int
VVRestoreSnap( void *pDBMgr, char *pVolName, char *pSnapName )
{
	Vol_t		*pVl;
	DlgCache_t	*pD;
	int Ret;
	VV_t	*pVV;

	//  volume write lock
	pD = VL_LockW( pVolName );
	if( !pD )	goto err2;

	pVl = container_of( pD, Vol_t, vl_Dlg );

	pVV = VV_getW( pVl->vl_DB.db_Name, TRUE, pVl );
	if( !pVV )	goto err3;

	Ret = DBRestoreSnap( pDBMgr, pVV, pSnapName );
	if( Ret < 0 )	goto err4;

	VV_put( pVV, TRUE );

	// Release volume lock
	VL_Unlock( pD, TRUE );

	return( Ret );
err4:
	VV_put( pVV, TRUE );
err3:
	VL_Unlock( pD, TRUE );
err2:
	return( -1 );
}

int VVCopyVV(void *pDBMgr, char *pVolNameFr, char *pVolNameTo, CopyLog_t *pCL);
int VVDeleteSnap( void *pDBMgr, char *pVolName );

int
VVCopyVolume( void *pDBMgr, char *pVolFr, char *pVolTo )
{
	Vol_t		*pVlFr, *pVlTo;
	DlgCache_t	*pDFr, *pDTo;
	int Ret;
	VV_t	*pVV;
	char	SnapFr[PATH_NAME_MAX];
	char	SnapTo[PATH_NAME_MAX];
	int	i;
	uint64_t	Total;
	CopyLog_t	CopyLog;

LOG( pLog, LogDBG, "VL_LockW:Fr[%s] To[%s]", pVolFr, pVolTo );

	pDFr = VL_LockW( pVolFr );
	if( !pDFr )	goto err1;

	pVlFr = container_of( pDFr, Vol_t, vl_Dlg );

	pDTo = VL_LockW( pVolTo );
	if( !pDTo )	goto err2;

	pVlTo = container_of( pDTo, Vol_t, vl_Dlg );

	/* prepare CopyLog */
	memset( &CopyLog, 0, sizeof(CopyLog) );

LOG( pLog, LogDBG, "CopyLog" );

	/* get CopyLog Seq */
	Ret = DBCopyLogSeqGet( pDBMgr, &CopyLog.cl_Seq );
	if( Ret < 0 )	goto err3;

	/* add CopyLog */
	strncpy( CopyLog.cl_Fr, pVolFr, sizeof(CopyLog.cl_Fr) );
	strncpy( CopyLog.cl_To, pVolTo, sizeof(CopyLog.cl_To) );
	Total = pVlFr->vl_VV.db_LSs;
	for( i = 0; i < pVlFr->vl_Childs; i++ ) {
		Total	+= pVlFr->vl_aChild[i].db_LSs;
	}
	CopyLog.cl_FrLSs	= Total;
	CopyLog.cl_Start	= time(0);
	CopyLog.cl_Stage	= COPY_STAGE_CREATE_SNAP;

	sprintf( SnapFr, "%s_%d_TMP", pVlFr->vl_DB.db_Name, pVlFr->vl_DB.db_Ver );
	sprintf( SnapTo, "%s_%d_TMP", pVlTo->vl_DB.db_Name, pVlTo->vl_DB.db_Ver );

	strncpy( CopyLog.cl_FrSnap, SnapFr, sizeof(CopyLog.cl_FrSnap) );
	strncpy( CopyLog.cl_ToSnap, SnapTo, sizeof(CopyLog.cl_ToSnap) );

	Ret = DBCopyLogAdd( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err3;

LOG( pLog, LogDBG, "CreateSnap:Fr[%s] To[%s]", SnapFr, SnapTo );

	Ret = DBCopyCreateSnapLink( pDBMgr, pVlFr, SnapFr, pVlTo, SnapTo );
	if( Ret < 0 ) goto err4;

	Ret = DBCopyLogGetBySeq( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err5;
	if( CopyLog.cl_Status & COPY_STATUS_ABORT ) {
		LOG( pLog, LogSYS, "ABORT:Copy(%s->%s)", pVolFr, pVolTo );
		goto err5;
	}
	CopyLog.cl_End		= time(0);
	CopyLog.cl_Stage	= COPY_STAGE_LINK;
	Ret = DBCopyLogUpdate( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err5;

LOG( pLog, LogDBG, "VL_Unlock:Fr[%s] To[%s]", pVolFr, pVolTo );

	VL_Unlock( pDTo, TRUE );
	VL_Unlock( pDFr, TRUE );

LOG( pLog, LogDBG, "VVCopyVV:Fr[%s] To[%s]", SnapFr, SnapTo );
	// 3.copy SnapFr to SnapTo
	Ret = VVCopyVV( pDBMgr, SnapFr, SnapTo, &CopyLog );
	if( Ret < 0 )	goto err11;

	CopyLog.cl_End		= time(0);
	CopyLog.cl_Stage	= COPY_STAGE_UNLINK;

	Ret = DBCopyLogUpdate( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err11;
	Ret = DBCopyLogGetBySeq( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err11;
	if( CopyLog.cl_Status & COPY_STATUS_ABORT ) {
		LOG( pLog, LogSYS, "ABORT:Copy(%s->%s)", pVolFr, pVolTo );
		goto err11;
	}

LOG( pLog, LogDBG, "VL_LockW:To[%s]", pVolTo );
	 //  volume write lock
	pDTo = VL_LockW( pVolTo );
	if( !pDTo )	goto err11;

LOG( pLog, LogDBG, "VV_getW:To[%s]", pVolTo );
	// get VV
	pVV = VV_getW( pVolTo, TRUE, NULL );
	if( !pVV )	goto err12;

	// change
	Ret = DBLink( pDBMgr, pVolTo, SnapTo );
	if( Ret < 0 )	goto err13;

	CopyLog.cl_End		= time(0);
	CopyLog.cl_Stage	= COPY_STAGE_DELETE_SNAP;

	Ret = DBCopyLogUpdate( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err13;
	Ret = DBCopyLogGetBySeq( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err13;
	if( CopyLog.cl_Status & COPY_STATUS_ABORT ) {
		LOG( pLog, LogSYS, "ABORT:Copy(%s->%s)", pVolFr, pVolTo );
		goto err13;
	}
LOG( pLog, LogDBG, "VV_put:[%s]", pVV->vv_DB.db_Name );
	// put VV
	VV_put( pVV, TRUE );

LOG( pLog, LogDBG, "VL_Unlock:To[%s]", pVolTo );
	// Release volume lock
	VL_Unlock( pDTo, TRUE );

LOG( pLog, LogDBG, "VVDeleteSnap:Fr[%s] To[%s]", SnapFr, SnapTo );
	// 5.delete SnapFr and SnapTo
	Ret = VVDeleteSnap( pDBMgr, SnapFr );
	if( Ret < 0 )	goto err1;

	Ret = VVDeleteSnap( pDBMgr, SnapTo );
	if( Ret < 0 )	goto err1;

	CopyLog.cl_End		= time(0);
	CopyLog.cl_Stage	= COPY_STAGE_COMPLETE;

	Ret = DBCopyLogUpdate( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err1;

LOG( pLog, LogDBG, "END:Ret=%d", Ret );
	return( Ret );
err13:
	VV_put( pVV, TRUE );
err12:
	VL_Unlock( pDTo, TRUE );
err11:
	Ret = DBLink( pDBMgr, pVlTo->vl_DB.db_Name, SnapTo );
	Ret = VVDeleteSnap( pDBMgr, SnapTo );
	Ret = VVDeleteSnap( pDBMgr, SnapFr );
	return( -1 );

err5:
	Ret = VVDeleteSnap( pDBMgr, SnapTo );
err4:
	Ret = VVDeleteSnap( pDBMgr, SnapFr );
err3:
	VL_Unlock( pDTo, TRUE );
err2:
	VL_Unlock( pDFr, TRUE );
err1:
	return( -1 );
}


int
VVCancelCopy(void *pDBMgr, char *pSeq )
{
	CopyLog_t	CopyLog;
	uint64_t	Seq;
	char		*pVolFr, *pVolTo;
	char		*pSnapFr, *pSnapTo;
	Vol_t		/**pVlFr,*/ *pVlTo;
	DlgCache_t	*pDFr, *pDTo;
	int Ret;

	Seq	= atoll( pSeq );

	memset( &CopyLog, 0, sizeof(CopyLog) );
	Ret = DBCopyAbort( pDBMgr, Seq, &CopyLog );
	if( Ret < 0 ) {
		printf("%"PRIu64" [%s/%s]->[%s/%s] is completed or removed.\n", 
				Seq, 
				CopyLog.cl_Fr, CopyLog.cl_FrSnap, 
				CopyLog.cl_To, CopyLog.cl_ToSnap );
		goto err;
	}
	pVolFr	= CopyLog.cl_Fr;
	pVolTo	= CopyLog.cl_To;
	pSnapFr	= CopyLog.cl_FrSnap;
	pSnapTo	= CopyLog.cl_ToSnap;

	/* garbage */
	pDFr = VL_LockW( pVolFr );
	if( !pDFr )	goto err;

//	pVlFr = container_of( pDFr, Vol_t, vl_Dlg );

	pDTo = VL_LockW( pVolTo );
	if( !pDTo )	goto err1;

	pVlTo = container_of( pDTo, Vol_t, vl_Dlg );

	// refer again
	Ret = DBCopyLogGetBySeq( pDBMgr, &CopyLog );
	if( Ret < 0 )	goto err2;

	if( CopyLog.cl_Stage == COPY_STAGE_CREATE_SNAP ) {	
		goto create_snap;
	}
	if( CopyLog.cl_Stage == COPY_STAGE_LINK ) {	
		goto link;
	}
link:
	Ret = DBLink( pDBMgr, pVlTo->vl_DB.db_Name, pSnapTo );
create_snap:
	Ret = VVDeleteSnap( pDBMgr, pSnapTo );
	Ret = VVDeleteSnap( pDBMgr, pSnapFr );

	VL_Unlock( pDTo, FALSE );
	VL_Unlock( pDFr, FALSE );
	return( 0 );

err2:
	VL_Unlock( pDTo, FALSE );
err1:
	VL_Unlock( pDFr, FALSE );
err:
	return( -1 );
}

typedef struct {
	Vol_t	*c_pVl;
	int		c_N;
	VV_t	*c_pVV;
	void	*c_pDBMgr;
	VV_IO_t	*c_pCopy;
} CopyArg_t;

void*
VVCopyLatter( Msg_t *pMsg )
{
	VV_t	*pVV;	

	pVV	= MSG_FUNC_ARG( pMsg );	

	LOG( pLog, LogINF, "[%s]", pVV->vv_DB.db_Name );

	VV_put( pVV, TRUE );

	return( MsgPopFunc(pMsg) );
}

void*
VVCopyFormer( Msg_t *pMsg )
{
	CopyArg_t	*pArg;
	void		*pDBMgr;
	VV_IO_t	*pCopy;
	VV_t	*pVV, *pVVnext;
	Vol_t	*pVl;
	int	N;

	pArg	= (CopyArg_t*)MsgFirst( pMsg );
	pDBMgr	= pArg->c_pDBMgr;
	pVV		= pArg->c_pVV;
	pCopy	= pArg->c_pCopy;
	pVl		= pArg->c_pVl;
	N		= pArg->c_N++;

	LOG( pLog, LogINF, "[%s] LSs=%"PRIi64" COWS=%"PRIi64"", 
				pVV->vv_DB.db_Name, pVV->vv_DB.db_LSs, pVV->vv_DB.db_COWs );

	DBCopyFr( pDBMgr, pVV, pCopy );

	if( N < (pVl->vl_Childs-1) ) {	// Node Only

		pVVnext	= VV_getR( pVl->vl_aChild[N].db_Name, TRUE, pVl );
		MsgPushFunc( pMsg, VVCopyLatter, (void*)pVVnext );

		pArg->c_pVV = pVVnext;

		return( VVCopyFormer );
	} else {
		return( MsgPopFunc(pMsg) );
	}
}

int
VVCopyStep1( void *pDBMgr, Vol_t *pVlFr, Vol_t *pVlTo, VV_t *pVVTo, 
					CopyLog_t *pCL )
{
	VV_IO_t	Copy;
	VV_t	*pVVFr;
	int		Ret;

	VV_IO_init( &Copy );

	if( pVlFr->vl_Childs > 0 ) {
		pVVFr = VV_getR( 
				pVlFr->vl_aChild[pVlFr->vl_Childs-1].db_Name, TRUE, pVlFr );
	} else {
		pVVFr = VV_getR( pVlFr->vl_DB.db_Name, TRUE, pVlFr );
	}
	if( !pVVFr )	goto err;

	LOG( pLog, LogINF, "[%s] LSs=%"PRIi64" COWS=%"PRIi64"", 
			pVVFr->vv_DB.db_Name, pVVFr->vv_DB.db_LSs, pVVFr->vv_DB.db_COWs );

	// construct read request
	Ret = DBCopyFr( pDBMgr, pVVFr, &Copy );
	if( Ret < 0 )	goto err1;

	// attach write request
	Ret = DBCopyTo( pDBMgr, pVlTo, pVVTo, &Copy );
	if( Ret < 0 )	goto err1;

//VV_IO_print( &Copy );
	// execute IO
	Ret = VVKickReqIOCopy( &Copy, pDBMgr, pCL );
	if( Ret < 0 )	goto err1;

	VV_put( pVVFr, FALSE );

	VV_IO_destroy( &Copy );

	return( 0 );
err1:
	VV_put( pVVFr, FALSE );
err:
	VV_IO_destroy( &Copy );
	return( -1 );
}

int
VVCopyStep2( void *pDBMgr, Vol_t *pVlFr, Vol_t *pVlTo, VV_t *pVVTo, 
				CopyLog_t *pCL )
{
	VV_IO_t	Copy;
	VV_t	*pVVFr;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	CopyArg_t	*pArg;
	int		Ret;

	/*
	 *	Read
	 */
	VV_IO_init( &Copy );

	pVVFr = VV_getR( pVlFr->vl_DB.db_Name, TRUE, pVlFr );
	if( !pVVFr )	goto err1;

	pMsg = MsgCreate( 1, Malloc, Free );

	pArg				= MsgMalloc( pMsg, sizeof(*pArg) );
	pArg->c_pVl			= pVlFr;
	pArg->c_N			= 0;
	pArg->c_pVV			= pVVFr;
	pArg->c_pCopy		= &Copy;
	pArg->c_pDBMgr		= pDBMgr;

	MsgVecSet( &Vec, 0, pArg, sizeof(*pArg), sizeof(*pArg) );
	MsgAdd( pMsg, &Vec );

	// read loop
	Ret = MsgEngine( pMsg, VVCopyFormer );
	if( Ret < 0 )	goto err2;

	MsgFree( pMsg, pArg );
	MsgDestroy( pMsg );

	VV_put( pVVFr, TRUE );
	/*
	 *	Write	allocate VS/LS
	 */
//VV_IO_print( &Copy );
	Ret = DBCopyTo( pDBMgr, pVlTo, pVVTo, &Copy );
	if( Ret < 0 )	goto err1;
//VV_IO_print( &Copy );
	/*
	 * Execute read and write
	 */
	Ret = VVKickReqIOCopy( &Copy, pDBMgr, pCL );
	if( Ret < 0 )	goto err1;

	VV_IO_destroy( &Copy );
	return( 0 );
err2:
	MsgFree( pMsg, pArg );
	MsgDestroy( pMsg );
	VV_put( pVVFr, TRUE );
err1:
	VV_IO_destroy( &Copy );
	return( -1 );
}

int
VVCopyVV( void *pDBMgr, char *pVolNameFr, char *pVolNameTo, CopyLog_t *pCL )
{
	Vol_t	*pVlFr, *pVlTo;
	DlgCache_t	*pDFr, *pDTo;
	VV_t	*pVVTo;
	int	Ret;

	pDTo = VL_LockW( pVolNameTo );
	if( !pDTo )	goto err2;

	pVlTo = container_of( pDTo, Vol_t, vl_Dlg );

	// check leaf of To
	if( pVlTo->vl_Childs > 0 )	goto err2;

	pVVTo = VV_getW( pVlTo->vl_DB.db_Name, TRUE, pVlTo );
	if( !pVVTo )	goto err3;

	pDFr = VL_LockR( pVolNameFr );
	if( !pDFr )	goto err2;

	pVlFr = container_of( pDFr, Vol_t, vl_Dlg );

//printf("BEFORE Step1 %lu\n", time(0) );
	/*
	 *	1-st step: copy original(leaf) whole data
	 */
	pCL->cl_Stage	= COPY_STAGE_COPY1;

	Ret = VVCopyStep1( pDBMgr, pVlFr, pVlTo, pVVTo, pCL );
	if( Ret < 0 )	goto err6;
	/*
	 *	2-nd step: copy COW
	 */
//printf("BEFORE Step2 %lu\n", time(0) );
	if( pVlFr->vl_Childs > 0 ) {
		pCL->cl_Stage	= COPY_STAGE_COPY2;

		Ret = VVCopyStep2( pDBMgr, pVlFr, pVlTo, pVVTo, pCL );
		if( Ret < 0 )	goto err6;
	}
//printf("AFTER Step2 %lu\n", time(0) );

	VL_Unlock( pDFr, TRUE );

	VV_put( pVVTo, TRUE ); 
	VL_Unlock( pDTo, TRUE );

	DBCommit( pDBMgr );
	
	return( 0 );

err6:
	VL_Unlock( pDFr, TRUE );
	VV_put( pVVTo, TRUE ); 
err3:
	VL_Unlock( pDTo, TRUE );
err2:
	DBRollback( pDBMgr );
	return( -1 );
}

/*
 *	1. COW hash of TO	(DBSetHashCOW)
 *	2. ReqIO of FR		(DBCopyFr)
 *	3. Bind To to ReqIO	(DBCopyTo)
 *	4. exec Copy command(kick reqIO)
 *	5. change link
 *	6. return LS
 *	7. add COW
 *	8. commit
 *		* Top --- Restore the latest-one
 *		* Bottom --- copy the up-one  to the bottom-one
 *		
 */
int
VVDeleteSnap( void *pDBMgr, char *pVolName )
{
	Vol_t	*pVl;
	DlgCache_t	*pD;
	VV_IO_t	Copy;
	VV_t	*pVV, *pVVParent = NULL, *pVVChild = NULL;
//	int		Ret;


	VV_IO_init( &Copy );

	pD = VL_LockW( pVolName );
	if( !pD )	goto err;

	pVl = container_of( pD, Vol_t, vl_Dlg );

	pVV	= VV_getW( pVl->vl_DB.db_Name, TRUE, pVl );

	if( pVl->vl_Parents > 0 ) {
		pVVParent = VV_getW( pVl->vl_aParent[0].db_Name, TRUE, pVl );
	}

	if( pVl->vl_Parents == 0 ) {	// remove completely

		DBDeleteCommitComplete( pDBMgr, pVV );

	} else {						// merge

		/*Ret = */DBCopyFr( pDBMgr, pVVParent, &Copy );
		/*Ret = */DBCopyTo( pDBMgr, pVl, pVV, &Copy );

		VVKickReqIO( &Copy, WM_COPY );

		DBDeleteCommitMerge( pDBMgr, pVV, pVVParent );
	}

	VV_put( pVV, TRUE );
	if( pVVParent )	VV_put( pVVParent, TRUE );
	if( pVVChild )	VV_put( pVVChild, TRUE );

	// Release volume lock
	VL_Unlock( pD, TRUE );

	VV_IO_destroy( &Copy );
	return( 0 );
err:
	VV_IO_destroy( &Copy );
	return( -1 );
}

int
VVListVV(void *pD, char *pVol )
{
	if( !pVol ) {

		/* VN_LIST */
		DlgCache_t		*pDlgVN;
		VN_DB_List_t	*pVN_List;
		VN_DB_t			*pVN;
		int				iVN;
		char	Time[30];
		int		i, j;

		pDlgVN	= DBLockR( pD, "VN_LIST" );

		pVN_List	= (VN_DB_List_t*)pDlgVN->d_pVoid;
		iVN	= pVN_List->l_Cnt;
		for( i = 0; i < iVN; i++ ) {
			pVN	= &pVN_List->l_aVN[i];
			strcpy( Time, ctime(&pVN->db_Time) );
			for( j = 0; j < sizeof(Time); j++ ) {
				if( Time[j] == '\n' ) {
					Time[j]	= 0;
					break;
				}
			}
			printf("=== Volume[%s] Target[%s] Pool[%s] Size[%"PRIi64"] "
				"\n\t\tStripe[%d] Ver[%d] Time[%s]===\n", 
				pVN->db_Name, pVN->db_Target, pVN->db_Pool, pVN->db_Size,
				pVN->db_Stripes, pVN->db_Ver, Time );
		}
		DBUnlock( pDlgVN );
	} else {

		/* VV_LIST of children */
		DlgCache_t	*pDlgVL;
		Vol_t		*pVl;
		VN_DB_t		*pVN;
		VV_DB_t		*pVV;
		char	Time[30];
		int		i, j;
		int		iVV;

		pDlgVL	= VL_LockR( pVol );
		if( !pDlgVL )	goto err1;

		pVl = container_of( pDlgVL, Vol_t, vl_Dlg );
		pVN	= &pVl->vl_DB;

		strcpy( Time, ctime(&pVN->db_Time) );
		for( j = 0; j < sizeof(Time); j++ ) {
			if( Time[j] == '\n' ) {
				Time[j]	= 0;
				break;
			}
		}
		printf("=== Volume[%s] Target[%s] Pool[%s] Size[%"PRIi64"] "
				"\n\t\tStripe[%d] Ver[%d] Time[%s]===\n", 
				pVN->db_Name, pVN->db_Target, pVN->db_Pool, pVN->db_Size,
				pVN->db_Stripes, pVN->db_Ver, Time );

		iVV	= pVl->vl_Childs;
		for( i = 0; i < iVV; i++ ) {
			pVV	= &pVl->vl_aChild[i];
			printf(
			"\t%d. VV[%s] Wr[%s] LSs[%"PRIi64"] COW[%s] COWs=[%"PRIi64"]\n",
			j++, pVV->db_Name, pVV->db_NameWrTBL, pVV->db_LSs,
			pVV->db_NameCOW, pVV->db_COWs );
		}
		VL_Unlock( pDlgVL, FALSE );
	}
	return( 0 );
err1:
	return( -1 );
}

int
VVListLV(void *pDBMgr )
{
	DlgCache_t		*pDlgLV;
	LV_DB_List_t	*pLV_List;
	LV_DB_t			*pLV;
	int	i, iLV;

	pDlgLV	= DBLockR( pDBMgr, "LV_LIST" );
	if( !pDlgLV )	goto err;

	pLV_List	= (LV_DB_List_t*)pDlgLV->d_pVoid;
	iLV	= pLV_List->l_Cnt;

	for( i = 0; i < iLV; i++ ) {
		pLV	= &pLV_List->l_aLV[i];

		printf("LV[%s] Total[%d] Usable[%d] Pool[%s] Cell[%s]\n",
				pLV->db_Name, pLV->db_Total, pLV->db_Usable, 
				pLV->db_Pool, pLV->db_Cell);
	}
	DBUnlock( pDlgLV );
	return( 0 );
err:
	return( -1 );
}

int
VVListCopy(void *pDBMgr )
{
	int	N;
	CopyLog_t	*aCL, *pCL;
	int		Ret;
	int		i;
	char	Start[32], End[32];
	int		l;

	Ret = DBListCopy( pDBMgr, &N, &aCL );
	if( Ret < 0 )	goto err;

	printf(
	"Id	Fr	To	FrLSs	ToLSs	ToCopied	Start	END	Stage	Status\n");
	for( i = 0; i < N; i++ ) {
		pCL = &aCL[i];

		strcpy( Start, ctime(&pCL->cl_Start) );
		l = strlen(Start); Start[l-1] = 0;
		strcpy( End, ctime(&pCL->cl_End) );
		l = strlen(End); End[l-1] = 0;

		printf(
			"%"PRIu64" %s %s %"PRIu64" %"PRIu64" %"PRIu64" %s %s %d 0x%x\n",
			pCL->cl_Seq, pCL->cl_Fr, pCL->cl_To, pCL->cl_FrLSs,
			pCL->cl_ToLSs, pCL->cl_ToCopied, Start, End,
			pCL->cl_Stage, pCL->cl_Status );
	}
	Free( aCL );
	return( 0 );
err:
	return( -1 );
}

int
VVRenameVolume(void *pDBMgr, char *pFr, char *pTo )
{
	DlgCache_t	*pD;
	int		Ret;

	// volume write lock
	pD = VL_LockW( pFr );
	if( !pD )	goto err;

	Ret = DBRenameVolume( pDBMgr, pFr, pTo );
	if( Ret < 0 )	goto err1;

	// Release volume lock
	VL_Unlock( pD, TRUE );
	return( 0 );
err1:
	VL_Unlock( pD, TRUE );
err:
	return( -1 );
}

int
VVResizeVolume(void *pDBMgr, char *pVol, char *pSize )
{
	DlgCache_t	*pD;
	int		Ret;

	// volume write lock
	pD = VL_LockW( pVol );
	if( !pD )	goto err;

	Ret = DBResizeVolume( pDBMgr, pVol, pSize );
	if( Ret < 0 )	goto err1;

	// Release volume lock
	VL_Unlock( pD, TRUE );
	return( 0 );
err1:
	VL_Unlock( pD, TRUE );
err:
	return( -1 );
}

void
usage(void)
{
	printf("=== Environmental variables(CSS DB RAS) are needed ===\n");
	printf("VVAdmin CreateVolume VolName Target Size Stripes Pool\n");
	printf("VVAdmin deleteVolume VolName\n");
	printf("VVAdmin CreateSnap VolName SnapVol\n");
	printf("VVAdmin RestoreSnap VolName SnapVol\n");
	printf("VVAdmin DeleteSnap SnapVol\n");
	printf("VVAdmin CopyVolume FromVolume ToVolume\n");

	printf("VVAdmin ListCopy\n");
	printf("VVAdmin CancelCopyVolume Id\n");

	printf("\nVVAdmin CreateLV LV_name Cell Pool Segments\n");
	printf("VVAdmin DeleteLV LV_name\n");
	printf("VVAdmin IncLV LV_name Segments\n");
	printf("VVAdmin MoveLV FromLV ToLV\n");

	printf("VVAdmin ListVV [Volume]\n");
	printf("VVAdmin ListLV\n");

	printf("VVAdmin RenameVolume From To\n");
	printf("VVAdmin ResizeVolume Volume Size\n");
}

int
main( int ac, char *av[] )
{
	int	Ret;

	if( ac < 2 ) {
		goto err;
	}
	VV_init();
	Ret = VL_init( "VVAdmin", 1 );
	if( Ret < 0 )	goto err;

	if( !strcmp( av[1], "deleteVolume" ) ) {

		if( ac < 3 )	goto err1;
		if( VVDeleteSnap( pDBMgr, av[2]  ) ) goto err1;

	} else if( !strcmp( av[1], "CreateVolume" ) ) {

		if( ac < 7 )	goto err1;
		if( VVCreateVolume( pDBMgr, 
				av[2], av[3], atoll(av[4]), atoi(av[5]), av[6] ) )	goto err1;

	} else if( !strcmp( av[1], "CreateSnap" ) ) {

		if( ac < 4 )	goto err1;
		if( VVCreateSnap( pDBMgr, av[2], av[3]  ) ) goto err1;

	} else if( !strcmp( av[1], "RestoreSnap" ) ) {

		if( ac < 4 )	goto err1;
		if( VVRestoreSnap( pDBMgr, av[2], av[3] ) ) goto err1;

	} else if( !strcmp( av[1], "DeleteSnap" ) ) {

		if( ac < 3 )	goto err1;
		if( VVDeleteSnap( pDBMgr, av[2] ) ) goto err1;

	} else if( !strcmp( av[1], "CopyVolume" ) ) {

		if( ac < 4 )	goto err1;
		if( VVCopyVolume( pDBMgr, av[2], av[3] ) ) goto err1;

	} else if( !strcmp( av[1], "ListCopy" ) ) {

		if( VVListCopy(pDBMgr) ) goto err1;

	} else if( !strcmp( av[1], "CancelCopyVolume" ) ) {

		if( VVCancelCopy(pDBMgr, av[2]) ) goto err1;

	} else if( !strcmp( av[1], "CreateLV" ) ) {

		if( ac < 6 )	goto err1;
		if( VVCreateLV(pDBMgr, av[2], av[3], av[4], atoi(av[5])) ) goto err1;

	} else if( !strcmp( av[1], "DeleteLV" ) ) {

		if( ac < 3 )	goto err1;
		if( VVDeleteLV(pDBMgr, av[2]) ) goto err1;

	} else if( !strcmp( av[1], "IncLV" ) ) {

		if( ac < 4 )	goto err1;
		if( VVIncLV(pDBMgr, av[2], atoi(av[3])) ) goto err1;

	} else if( !strcmp( av[1], "ListVV" ) ) {

		if( ac < 3 ) {
			 if( VVListVV(pDBMgr, NULL ) ) goto err1;
		} else {
			 if( VVListVV(pDBMgr, av[2] ) ) goto err1;
		}

	} else if( !strcmp( av[1], "ListLV" ) ) {

		if( VVListLV(pDBMgr) ) goto err1;

	} else if( !strcmp( av[1], "MoveLV" ) ) {

		if( ac < 4 )	goto err1;
		if( VVMoveLV(pDBMgr, av[2], av[3]) ) goto err1;

	} else if( !strcmp( av[1], "RenameVolume" ) ) {

		if( ac < 4 )	goto err1;
		if( VVRenameVolume(pDBMgr, av[2], av[3] ) ) goto err1;

	} else if( !strcmp( av[1], "ResizeVolume" ) ) {

		if( ac < 4 )	goto err1;
		if( VVResizeVolume(pDBMgr, av[2], av[3] ) ) goto err1;

	} else	goto err1;

	VL_exit();
	VV_exit();
	return( 0 );
err1:
	VL_exit();
	VV_exit();
err:
	usage();
	exit( -1 );
}
