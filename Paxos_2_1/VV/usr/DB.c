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
#include	"neo_db.h"
#include	<inttypes.h>

#define	COLUMN_SEQ		"Seq"
#define	COLUMN_VER		"Ver"
#define	COLUMN_NAME		"Name"
#define	COLUMN_VN		"VN"
#define	COLUMN_VV		"VV"
#define	COLUMN_VS		"VS"
#define	COLUMN_SI		"SI"
#define	COLUMN_LV		"LV"
#define	COLUMN_LS		"LS"
#define	COLUMN_CELL		"Cell"
#define	COLUMN_POOL		"Pool"
#define	COLUMN_TIME		"Time"
#define	COLUMN_STATUS	"Status"

#define	COLUMN_WRITE_TABLE	"WR_TBL"
#define	COLUMN_COW_TABLE	"COW_TBL"	// hash written by Unit-number
#define	COLUMN_READ_VV		"RD_VV"
#define	COLUMN_REF			"Ref"

#define	LOCK_LIST	"LOCK_LIST"

#define	SEQ_LIST	"SEQ_LIST"
#define	SEQ_SEQ		COLUMN_SEQ
#define	SEQ_NAME	COLUMN_NAME

#define	COPY_LOG		"COPY_LOG"
#define	COPY_SEQ		COLUMN_SEQ
#define	COPY_FR			"Fr"
#define	COPY_FR_SNAP	"FrSnap"
#define	COPY_TO			"To"
#define	COPY_TO_SNAP	"ToSnap"
#define	COPY_FR_LS		"FrLSs"
#define	COPY_TO_LS		"ToLSs"
#define	COPY_TO_COPIED	"ToCopied"
#define	COPY_START		"Start"
#define	COPY_END		"End"
#define	COPY_STAGE		"Stage"
#define	COPY_STATUS		COLUMN_STATUS

#define	VN_VV_LIST	"VN_VV_LIST"

#define	VN_LIST			"VN_LIST"
#define	VN_VV			"VV_NAME"
#define	VN_TARGET		"Target"
#define	VN_POOL			COLUMN_POOL	
#define	VN_SIZE			"Size"
#define	VN_STRIPE		"Stripe"
#define	VN_REF			COLUMN_REF
#define	VN_VER			COLUMN_VER
#define	VN_TIME			COLUMN_TIME
#define	VN_STATUS		COLUMN_STATUS
#define	VN_TYPE			"Type"
#define		TYPE_VOLUME		"Volume"
#define		TYPE_SNAPSHOT	"Snapshot"

#define	VV_LIST			"VV_LIST"
#define	VV_NAME			"VV_NAME"
#define	VV_VER			COLUMN_VER
#define	VV_WR_TBL		COLUMN_WRITE_TABLE
#define	VV_COW_TBL		COLUMN_COW_TABLE
#define	VV_RD_VV		COLUMN_READ_VV
#define	VV_REF			COLUMN_REF

#define	WR_VS		COLUMN_VS
#define	WR_SI		COLUMN_SI
#define	WR_LV		COLUMN_LV
#define	WR_LS		COLUMN_LS

#define	COW_UNIT	"COW_UNIT"

#define	LV_LIST		"LV_LIST"
#define	LV_LV		COLUMN_LV
#define	LV_FLAG		"Flag"
#define	LV_TOTAL	"Total"
#define	LV_USABLE	"Usable"
#define	LV_CELL		COLUMN_CELL
#define	LV_POOL		COLUMN_POOL

//#define	LV_SEQ		COLUMN_SEQ
#define	LV_LS		COLUMN_LS
//#define	LV_FLAG		"Flag"
#define	LV_WR		COLUMN_WRITE_TABLE
#define	LV_VS		COLUMN_VS
#define	LV_SI		COLUMN_SI

#define	MAX_ITEM	20

typedef	struct DB {
	Mtx_t			db_Mtx;
	void			*db_pMgr;
	struct Session	*db_pSession;
	DlgCacheCtrl_t	db_DlgCtrl;
	Cnd_t			db_Cnd;
	pthread_t		db_HoldTh;
	Hash_t			db_HashCell;
	r_hold_t		db_Hold;
	int				db_HoldCnt;
} DB_t;
static	DB_t	DB;

static	int _DBAllocLS( void *pDB, 
		char *pVV, uint64_t VS, int SI, char *pLV, uint32_t *pLS );
static	int _DBFreeLS( void *pDB, char *pLV, uint32_t LS );
int DBGetCell( void *pDB, char *pLV, char *pCell );
int _DBGetCell( void *pDB, char *pLV, char *pCell );
int _DBGetLV( void *pDB, char *pGroup, int N, LS_t *pLS );
int _DBUpdateUsable( void *pDB, char *pLV, int Cnt );


typedef struct DB_VT {
	list_t		vt_Lnk;
	uint64_t	vt_VS;
	int			vt_SI;
	char		vt_LV[PATH_NAME_MAX];
	int			vt_LS;
} DB_VT_t;

int
DBGetSeq( void *pD, char *pName, uint64_t *pUint64 )
{
	DB_t *pDB = (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char sql[256];
	int	Ret;
	r_tbl_t	*pTbl;
	int	n, len;

	/* no need lock because update seq is atomic */
	sprintf(sql, 
		"update "SEQ_LIST" set "SEQ_SEQ"="SEQ_SEQ"+1 where "SEQ_NAME"='%s';" 
		"commit;",
		pName );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	n = pTbl->t_rec_used;
	if( n <= 0 )	goto err1;

	len = sizeof(uint64_t);
	rdb_get( pTbl, (r_head_t*)pTbl->t_rec, SEQ_SEQ, (char*)pUint64, &len );

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( -1 );
}

int
DBCopyLogSeqGet( void *pD, uint64_t *pUint64 )
{
	return( DBGetSeq( pD, COPY_LOG, pUint64 ) );
}

int
DBCopyLogAdd( void *pD, CopyLog_t *pCL )
{
	DB_t *pDB = (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char sql[256];
	int	Ret;
	r_tbl_t	*pTbl;

	/* insert  Seq is unique */
	sprintf( sql, 
	"insert into "COPY_LOG" "
	"values(%"PRIu64",'%s','%s','%s','%s',"
	"%"PRIu64",%"PRIu64",%"PRIu64",%lu,%lu,%d,%d);"
	"commit;",
	pCL->cl_Seq, pCL->cl_Fr, pCL->cl_FrSnap, pCL->cl_To, pCL->cl_ToSnap, 
	pCL->cl_FrLSs, pCL->cl_ToLSs, pCL->cl_ToCopied,
 	pCL->cl_Start, pCL->cl_End, pCL->cl_Stage, pCL->cl_Status );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err:
	return( -1 );
}

int
DBCopyLogUpdate( void *pD, CopyLog_t *pCL )
{
	DB_t *pDB = (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char sql[256];
	int	Ret;
	r_tbl_t	*pTbl;

	/* update only Copied, End, Stage */
	sprintf( sql, 
		"update "COPY_LOG" "
		"set "COPY_TO_LS"=%"PRIu64","COPY_TO_COPIED"=%"PRIu64","
		""COPY_END"=%lu,"COPY_STAGE"=%d,"COPY_STATUS"=%d "
		"where "COPY_SEQ"=%"PRIu64";"
		"commit;",
		pCL->cl_ToLSs, pCL->cl_ToCopied, 
		pCL->cl_End, pCL->cl_Stage, pCL->cl_Status, pCL->cl_Seq );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err:
	return( -1 );
}

int
DBCopyLogGetBySeq( void *pD, CopyLog_t *pCL )
{
	DB_t *pDB = (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char sql[256];
	int	Ret;
	r_tbl_t	*pTbl;
	r_head_t	*pRec;
	int		len;

	/* select *  */
	sprintf( sql, 
		"select * from "COPY_LOG" " "where "COPY_SEQ"=%"PRIu64";",
		pCL->cl_Seq );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;
	if( pTbl->t_rec_used <= 0 )	goto err1;

	pRec	= (r_head_t*)pTbl->t_rec;

	len	= sizeof(pCL->cl_Fr);
	rdb_get( pTbl, pRec, COPY_FR, pCL->cl_Fr, &len );
	len	= sizeof(pCL->cl_FrSnap);
	rdb_get( pTbl, pRec, COPY_FR_SNAP, pCL->cl_FrSnap, &len );
	len	= sizeof(pCL->cl_To);
	rdb_get( pTbl, pRec, COPY_TO, pCL->cl_To, &len );
	len	= sizeof(pCL->cl_ToSnap);
	rdb_get( pTbl, pRec, COPY_TO_SNAP, pCL->cl_ToSnap, &len );
	len	= sizeof(pCL->cl_FrLSs);
	rdb_get( pTbl, pRec, COPY_FR_LS, (char*)&pCL->cl_FrLSs, &len );
	len	= sizeof(pCL->cl_ToLSs);
	rdb_get( pTbl, pRec, COPY_TO_LS, (char*)&pCL->cl_ToLSs, &len );
	len	= sizeof(pCL->cl_ToCopied);
	rdb_get( pTbl, pRec, COPY_TO_COPIED, (char*)&pCL->cl_ToCopied, &len );
	len	= sizeof(pCL->cl_Start);
	rdb_get( pTbl, pRec, COPY_START, (char*)&pCL->cl_Start, &len );
	len	= sizeof(pCL->cl_End);
	rdb_get( pTbl, pRec, COPY_END, (char*)&pCL->cl_End, &len );
	len	= sizeof(pCL->cl_Stage);
	rdb_get( pTbl, pRec, COPY_STAGE, (char*)&pCL->cl_Stage, &len );
	len	= sizeof(pCL->cl_Status);
	rdb_get( pTbl, pRec, COPY_STATUS, (char*)&pCL->cl_Status, &len );

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
err:
	return( -1 );
}

int
DBListCopy( void *pD, int *pN, CopyLog_t **paCL )
{
	DB_t *pDB = (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char sql[256];
	int	Ret;
	r_tbl_t	*pTbl;
	r_head_t	*pRec;
	int		len;
	CopyLog_t	*aCL, *pCL;
	int		i;

	/* select *  */
	sprintf( sql, "select * from "COPY_LOG";" );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;
	if( pTbl->t_rec_used <= 0 )	goto err1;

	aCL	= (CopyLog_t*)Malloc( pTbl->t_rec_used*sizeof(CopyLog_t) );

	*paCL 	= aCL;
	*pN		= pTbl->t_rec_used;

	for( i = 0, pRec = (r_head_t*)pTbl->t_rec; i < pTbl->t_rec_used;
				i++, pRec = (r_head_t*)((char*)pRec+pTbl->t_rec_size) ) {

		pCL	= &aCL[i];

		len	= sizeof(pCL->cl_Seq);
		rdb_get( pTbl, pRec, COPY_SEQ, (char*)&pCL->cl_Seq, &len );
		len	= sizeof(pCL->cl_Fr);
		rdb_get( pTbl, pRec, COPY_FR, pCL->cl_Fr, &len );
		len	= sizeof(pCL->cl_To);
		rdb_get( pTbl, pRec, COPY_TO, pCL->cl_To, &len );
		len	= sizeof(pCL->cl_FrLSs);
		rdb_get( pTbl, pRec, COPY_FR_LS, (char*)&pCL->cl_FrLSs, &len );
		len	= sizeof(pCL->cl_ToLSs);
		rdb_get( pTbl, pRec, COPY_TO_LS, (char*)&pCL->cl_ToLSs, &len );
		len	= sizeof(pCL->cl_ToCopied);
		rdb_get( pTbl, pRec, COPY_TO_COPIED, (char*)&pCL->cl_ToCopied, &len );
		len	= sizeof(pCL->cl_Start);
		rdb_get( pTbl, pRec, COPY_START, (char*)&pCL->cl_Start, &len );
		len	= sizeof(pCL->cl_End);
		rdb_get( pTbl, pRec, COPY_END, (char*)&pCL->cl_End, &len );
		len	= sizeof(pCL->cl_Stage);
		rdb_get( pTbl, pRec, COPY_STAGE, (char*)&pCL->cl_Stage, &len );
		len	= sizeof(pCL->cl_Status);
		rdb_get( pTbl, pRec, COPY_STATUS, (char*)&pCL->cl_Status, &len );
	}
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
err:
	return( -1 );
}

int
DBCopyAbort( void *pD, uint64_t Seq, CopyLog_t *pCL )
{
	int	Ret;

	pCL->cl_Seq	= Seq;

	Ret = DBCopyLogGetBySeq( pD, pCL );
	if( Ret < 0 )	goto err;
	if( pCL->cl_Stage == COPY_STAGE_COMPLETE ) {
		goto err;
	}
	pCL->cl_Status	|= COPY_STATUS_ABORT;
	Ret = DBCopyLogUpdate( pD, pCL );
	if( Ret < 0 )	goto err;
	return( 0 );
err:
	errno = ENOENT;
	return( -1 );
}

/* Lock DBMS */
/***
 ネットワークロックで排他しても、ロックを持っているAに障害が発生したとき、
ロック待ちのBが動き、AがDBのrollbackを行う前にBがDBにアクセスする場合があるので
DBそのものロックが必要である。
***/
int
_DBLock( void *pD )
{
	DB_t *pDB = (DB_t*)pD;	
	int	Ret;
	pthread_t	MyTh;

	MyTh = pthread_self();	

	LOG(neo_log, LogINF, "th=%p", MyTh );

	MtxLock( &pDB->db_Mtx );

	while( pDB->db_HoldTh && pDB->db_HoldTh != MyTh ) {
		CndWait( &pDB->db_Cnd, &pDB->db_Mtx );
	}
	ASSERT( !pDB->db_HoldTh || pDB->db_HoldTh == MyTh );
	if( !pDB->db_HoldTh )	pDB->db_HoldTh	= MyTh;
	
	if( pDB->db_HoldCnt == 0 ) {

		LOG(neo_log, LogDBG, "rdb_hold[th=%p]", MyTh );

		Ret = rdb_hold( 1, &pDB->db_Hold, R_WAIT );
		ASSERT( Ret == 0 && pDB->db_Hold.h_td );
	}
	pDB->db_HoldCnt++;
	
	MtxUnlock( &pDB->db_Mtx );
	return( 0 );
}

int
_DBUnlock( void *pD )
{
	DB_t *pDB = (DB_t*)pD;	
	int	Ret;
	pthread_t	MyTh;

	MyTh = pthread_self();	

	LOG(neo_log, LogINF, "th=%p", MyTh );

	MtxLock( &pDB->db_Mtx );

	pDB->db_HoldCnt--;

	ASSERT( pDB->db_HoldTh == MyTh );	

	if( pDB->db_HoldCnt == 0 ) {

		LOG(neo_log, LogDBG, "rdb_release[th=%lu] [%s]", 
			MyTh, pDB->db_Hold.h_td->t_name );

		Ret = rdb_release( pDB->db_Hold.h_td );

		ASSERT( Ret == 0 );

		pDB->db_HoldTh = 0;
		CndBroadcast( &pDB->db_Cnd );
	}
	MtxUnlock( &pDB->db_Mtx );
	return( 0 );
}

DlgCache_t*
DBLockR( void *pD, char *pName )
{
	DB_t *pDB = (DB_t*)pD;	
	DlgCache_t	*pDlg;

	LOG(neo_log, LogINF, "pName[%s]", pName  );

	pDlg = DlgCacheLockR( &pDB->db_DlgCtrl, pName, TRUE, pDB, NULL );

	return( pDlg );
}

DlgCache_t*
DBLockW( void *pD, char *pName )
{
	DB_t *pDB = (DB_t*)pD;	
	DlgCache_t	*pDlg;

	LOG(neo_log, LogINF, "pName[%s]", pName  );

	pDlg = DlgCacheLockW( &pDB->db_DlgCtrl, pName, TRUE, pDB, NULL );

	return( pDlg );
}

int
DBUnlock( DlgCache_t *pDlg )
{
	LOG(neo_log, LogINF, "d_Name[%s]", pDlg->d_Dlg.d_Name  );

	DlgCacheUnlock( pDlg, FALSE );

	return( 0 );
}

int
DBUnlockReturn( DlgCache_t *pDlg )
{
	int	Ret;

	LOG(neo_log, LogINF, "d_Name[%s]", pDlg->d_Dlg.d_Name  );

	Ret = DlgCacheReturn( pDlg, NULL );
	if( Ret < 0 )	goto err;

	DlgCacheUnlock( pDlg, TRUE );

	return( 0 );
err:
	return( -1 );
}

VN_DB_List_t*
MallocVN_LIST( void *pD )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTbl;
	int			n, i, len;
	char		*pRec;
	VN_DB_List_t	*pVN_List;
	VN_DB_t			*pVN;
	char		sql[128];
	int	Ret;

	memset( sql, 0, sizeof(sql) );
	sprintf( sql, "select * from "VN_LIST";" );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;

	n = pTbl->t_rec_used;
	if( n < 0 )	goto err;

	len	= sizeof(VN_DB_List_t) + sizeof(VN_DB_t)*n;
	pVN_List	= (VN_DB_List_t*)Malloc( len );
	memset( pVN_List, 0, len );

	pVN_List->l_Cnt	= n;
	for( i = 0, pRec = (char*)pTbl->t_rec; i < n; 
					i++, pRec += pTbl->t_rec_size ) {

		pVN	= &pVN_List->l_aVN[i];

		len = sizeof(pVN->db_Name);
		rdb_get( pTbl, (r_head_t*)pRec, VN_VV, pVN->db_Name, &len );
		len = sizeof(pVN->db_Target);
		rdb_get( pTbl, (r_head_t*)pRec, VN_TARGET, 
							(char*)&pVN->db_Target, &len );
		len = sizeof(pVN->db_Size);
		rdb_get( pTbl, (r_head_t*)pRec, VN_SIZE, (char*)&pVN->db_Size, &len );
		len = sizeof(pVN->db_Stripes);
		rdb_get( pTbl, (r_head_t*)pRec, VN_STRIPE, 
							(char*)&pVN->db_Stripes, &len );
		len = sizeof(pVN->db_Ref);
		rdb_get( pTbl, (r_head_t*)pRec, VN_REF, (char*)&pVN->db_Ref, &len );
		len = sizeof(pVN->db_Ver);
		rdb_get( pTbl, (r_head_t*)pRec, VN_VER, (char*)&pVN->db_Ver, &len );
		len = sizeof(pVN->db_Pool);
		rdb_get( pTbl, (r_head_t*)pRec, VN_POOL, (char*)&pVN->db_Pool, &len );
		len = sizeof(pVN->db_Time);
		rdb_get( pTbl, (r_head_t*)pRec, VN_TIME, (char*)&pVN->db_Time, &len );
	}
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	LOG( neo_log, LogINF, "n=%d", n );

	return( pVN_List );
err:
	return( NULL );
}

VV_DB_List_t*
MallocVV_LIST( void *pD )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTbl;
	int			n, i, len;
	char		*pRec;
	VV_DB_t		*pVV;
	VV_DB_List_t	*pVV_List;
	char		sql[128];
	int	Ret;

	_DBLock( pDB );

	memset( sql, 0, sizeof(sql) );
	sprintf( sql, "select * from "VV_LIST";" );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;

	n = pTbl->t_rec_used;
	if( n < 0 )	goto err;

	len	= sizeof(VV_DB_List_t) + sizeof(VV_DB_t)*n;
	pVV_List	= (VV_DB_List_t*)Malloc( len );
	memset( pVV_List, 0, len );

	pVV_List->l_Cnt	= n;
	for( i = 0, pRec = (char*)pTbl->t_rec; i < n; 
					i++, pRec += pTbl->t_rec_size ) {

		pVV	= &pVV_List->l_aVV[i];

		len = sizeof(pVV->db_Name);
		rdb_get( pTbl, (r_head_t*)pRec, VV_NAME, pVV->db_Name, &len );
		len = sizeof(pVV->db_NameWrTBL);
		rdb_get( pTbl, (r_head_t*)pRec, VV_WR_TBL, pVV->db_NameWrTBL, &len );
		len = sizeof(pVV->db_NameCOW);
		rdb_get( pTbl, (r_head_t*)pRec, VV_COW_TBL, pVV->db_NameCOW, &len );
		len = sizeof(pVV->db_NameRdVV);
		rdb_get( pTbl, (r_head_t*)pRec, VV_RD_VV, pVV->db_NameRdVV, &len );

		pVV->db_LSs		= -1;
		pVV->db_COWs	= -1;
	}
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	LOG( neo_log, LogINF, "n=%d", n );

	_DBUnlock( pDB );

	return( pVV_List );
err:
	_DBUnlock( pDB );
	return( NULL );
}

LV_DB_List_t*
MallocLV_LIST( void *pD )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTbl;
	int			n, i, len;
	char		*pRec;
	LV_DB_t		*pLV;
	LV_DB_List_t	*pLV_List;
	char		sql[128];
	int	Ret;

	_DBLock( pDB );

	memset( sql, 0, sizeof(sql) );
	sprintf( sql, "select * from "LV_LIST";" );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;

	n = pTbl->t_rec_used;
	if( n < 0 )	goto err;

	len	= sizeof(LV_DB_List_t) + sizeof(LV_DB_t)*n;
	pLV_List	= (LV_DB_List_t*)Malloc( len );
	memset( pLV_List, 0, len );

	pLV_List->l_Cnt	= n;
	for( i = 0, pRec = (char*)pTbl->t_rec; i < n; 
					i++, pRec += pTbl->t_rec_size ) {

		pLV	= &pLV_List->l_aLV[i];

		len = sizeof(pLV->db_Name);
		rdb_get(pTbl, (r_head_t*)pRec, LV_LV, pLV->db_Name, &len );
		len = sizeof(pLV->db_Flag);
		rdb_get(pTbl, (r_head_t*)pRec, LV_FLAG, (char*)&pLV->db_Flag, &len);
		len = sizeof(pLV->db_Total);
		rdb_get(pTbl, (r_head_t*)pRec, LV_TOTAL, (char*)&pLV->db_Total, &len);
		len = sizeof(pLV->db_Usable);
		rdb_get(pTbl, (r_head_t*)pRec, LV_USABLE,(char*)&pLV->db_Usable, &len);
		len = sizeof(pLV->db_Cell);
		rdb_get(pTbl, (r_head_t*)pRec, LV_CELL, pLV->db_Cell, &len);
		len = sizeof(pLV->db_Pool);
		rdb_get(pTbl, (r_head_t*)pRec, LV_POOL, pLV->db_Pool, &len);

		HashPut( &pDB->db_HashCell, pLV->db_Name, pLV );
	}
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	LOG( neo_log, LogINF, "n=%d", n );

	_DBUnlock( pDB );

	return( pLV_List );
err:
	_DBUnlock( pDB );
	return( NULL );
}

int
_DB_Init( DlgCache_t *pD, void *pArg )
{
	DB_t *pDB = (DB_t*)pD->d_Dlg.d_pTag;
	int	Ret = 0;

LOG( neo_log, LogDBG, "[%s]", pD->d_Dlg.d_Name );

	if( !strcmp(LV_LIST, pD->d_Dlg.d_Name) ) {

		pD->d_pVoid	= MallocLV_LIST( pDB );
		ASSERT( pD->d_pVoid );

	}else if( !strcmp(VN_VV_LIST, pD->d_Dlg.d_Name) ) {
		VN_VV_List_t	*pVN_VV;
		
		pVN_VV	= Malloc( sizeof(*pVN_VV) );

		pVN_VV->l_pVN_List	= MallocVN_LIST( pDB );
		pVN_VV->l_pVV_List	= MallocVV_LIST( pDB );

		pD->d_pVoid	= pVN_VV;
	}
	return( Ret );
}

int
_DB_Term( DlgCache_t *pD, void *pArg )
{
	int	Ret = 0;
	void	*pTag;

LOG( neo_log, LogDBG, "[%s]", pD->d_Dlg.d_Name );

	if( !strcmp(LV_LIST, pD->d_Dlg.d_Name) ) {

		DB_t *pDB;
		pTag	= pD->d_Dlg.d_pTag;
		ASSERT( pTag );
		pDB 	= (DB_t*)pTag;	

		HashClear( &pDB->db_HashCell );

		if( pD->d_pVoid ) {
			Free( pD->d_pVoid );
			pD->d_pVoid	= NULL;
		}
	}else if( !strcmp(VN_VV_LIST, pD->d_Dlg.d_Name) ) {
		VN_VV_List_t	*pVN_VV;

		pVN_VV	= pD->d_pVoid;
		if( pVN_VV ) {
			if( pVN_VV->l_pVN_List )	Free( pVN_VV->l_pVN_List );
			if( pVN_VV->l_pVV_List )	Free( pVN_VV->l_pVV_List );
			Free( pVN_VV );
			pD->d_pVoid	= NULL;
		}
	}
	return( Ret );
}

void*
DBOpen( char *pDBCell, struct Session *pSession, DlgCacheRecall_t *pRC  )
{
	int		ac = 1;
	char	*av[1];

LOG( neo_log, LogDBG, "ENT" );
	MtxInit( &DB.db_Mtx );
	CndInit( &DB.db_Cnd );

	av[0]	= "VV_CLIENT";
	_neo_prologue( &ac, av );
	DB.db_pMgr = rdb_link("CLIENT", pDBCell );
	if( !DB.db_pMgr )	goto err;
	DB.db_pSession	= pSession;

	DlgCacheInitBy( pSession, pRC, &DB.db_DlgCtrl, 100, 
					NULL, NULL, NULL, NULL, _DB_Init, _DB_Term, NULL,
					PaxosSessionGetLog(pSession) );

	HashInit( &DB.db_HashCell, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR,
			NULL, Malloc, Free, NULL );

	strcpy( DB.db_Hold.h_name, SEQ_LIST );
	DB.db_Hold.h_td		= rdb_open( DB.db_pMgr, LOCK_LIST );
	if( !DB.db_Hold.h_td )	goto err;
	DB.db_Hold.h_mode	= R_EXCLUSIVE;

LOG( neo_log, LogDBG, "EXT" );
	return( &DB );
err:
LOG( neo_log, LogERR, "ERR" );
	return( NULL );
}

int
DBClose( void *pD )
{
	DB_t *pDB = (DB_t*)pD;	
	int Ret;

LOG( neo_log, LogDBG, "ENT" );
	DlgCacheDestroy( &DB.db_DlgCtrl );
	
	Ret = rdb_close( pDB->db_Hold.h_td );

	Ret = rdb_unlink( pDB->db_pMgr );
	_neo_epilogue( 0 );

	MtxDestroy( &pDB->db_Mtx );
	return( Ret );
}

int
DBCommit( void *pD )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int Ret;
	r_tbl_t	*pTbl;

	Ret = SqlExecute( pDBMgr, "commit;" );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
DBRollback( void *pD )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int Ret;
	r_tbl_t	*pTbl;

	Ret = SqlExecute( pDBMgr, "rollback;" );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}


int
DBSetLSsAndCOWs( void *pD, VV_DB_t *pVV_DB )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTblW;

	if( pVV_DB->db_LSs < 0 ) {
		pVV_DB->db_LSs	= 0;
		if( pVV_DB->db_NameWrTBL[0] != 0 
				&& pVV_DB->db_NameWrTBL[0] != ' ' ) {
			pTblW	= rdb_open( pDBMgr, pVV_DB->db_NameWrTBL );
			pVV_DB->db_LSs = pTblW->t_rec_used;	
			rdb_close( pTblW );
		}
		pVV_DB->db_COWs	= 0;
		if( pVV_DB->db_NameCOW[0] != 0 
				&& pVV_DB->db_NameCOW[0] != ' ' ) {
			pTblW	= rdb_open( pDBMgr, pVV_DB->db_NameCOW );
			pVV_DB->db_COWs = pTblW->t_rec_used;	
			rdb_close( pTblW );
		}
	}
	return( 0 );
}

int
DBSetVl( void *pD, Vol_t *pVl ) 
{
	int		i, j, J;
	int		P, N;
	DlgCache_t	*pDlgVN_VV;
	VV_DB_List_t	*pVV_List;
	VV_DB_t			*pVV_DB;
	VN_DB_List_t	*pVN_List;
	VN_VV_List_t	*pVN_VV;

	pDlgVN_VV	= DBLockR( pD, VN_VV_LIST );

	pVN_VV	= (VN_VV_List_t*)pDlgVN_VV->d_pVoid;
	pVN_List	= pVN_VV->l_pVN_List;
	pVV_List	= pVN_VV->l_pVV_List;

	// Find my VN_, VV_
	for( i = 0; i < pVN_List->l_Cnt; i++ ) {
		if( !strncmp( pVN_List->l_aVN[i].db_Name, pVl->vl_DB.db_Name, 
				sizeof(pVN_List->l_aVN[i].db_Name) ) ) {
			pVl->vl_DB	= pVN_List->l_aVN[i];
			break;
		}
	}
	for( i = 0, J = -1; i < pVV_List->l_Cnt; i++ ) {
		if( !strncmp( pVV_List->l_aVV[i].db_Name, pVl->vl_DB.db_Name, 
				sizeof(pVV_List->l_aVV[i].db_Name) ) ) {
			pVV_DB	= &pVV_List->l_aVV[i];
			J = i; 
			break;
		}
	}
	if( J < 0 )	goto err1;

	DBSetLSsAndCOWs( pD, pVV_DB );
	pVl->vl_VV	= *pVV_DB;

	P = 0;
	for( i = 0; i < pVV_List->l_Cnt; i++ ) {
		pVV_DB	= &pVV_List->l_aVV[i];
		if( !strncmp(pVV_DB->db_NameRdVV, pVl->vl_DB.db_Name, 
					sizeof(pVV_DB->db_NameRdVV))) P++;
	}
	// Parent
	pVl->vl_Parents = 0;
	if( P > 0 ) {
		pVl->vl_aParent	= (VV_DB_t*)Malloc( P*sizeof(VV_DB_t) );

		for( i = 0; i < pVV_List->l_Cnt && pVl->vl_Parents < P; i++ ) {

			pVV_DB	= &pVV_List->l_aVV[i];
			if( !strncmp(pVV_DB->db_NameRdVV, pVl->vl_DB.db_Name, 
					sizeof(pVV_DB->db_NameRdVV))) {

				DBSetLSsAndCOWs( pD, pVV_DB );

				pVl->vl_aParent[pVl->vl_Parents++] = *pVV_DB;
			}
		}
ASSERT( P == pVl->vl_Parents );
	}
	// Down stream(Child)
	j = J;
	N = 0;
	pVV_DB	= &pVV_List->l_aVV[0];
nextChild:
	for( i = 0; i < pVV_List->l_Cnt; i++ ) {
		if( !strncmp( pVV_DB[i].db_Name, pVV_DB[j].db_NameRdVV, 
									sizeof(pVV_DB[i].db_Name) ) ) {
			j = i; 
			N++;
			if( pVV_DB[j].db_NameRdVV[0] == 0 
						|| pVV_DB[j].db_NameRdVV[0] == ' ' )	break;
			goto nextChild;
		}
	}
	if( N > 0 ) {
		pVl->vl_aChild	= (VV_DB_t*)Malloc( N*sizeof(VV_DB_t) );
		j = J;
		pVV_DB	= &pVV_List->l_aVV[0];
nextChild1:
		for( i = 0; i < pVV_List->l_Cnt; i++ ) {
			if( !strncmp( pVV_DB[i].db_Name, pVV_DB[j].db_NameRdVV, 
									sizeof(pVV_DB[i].db_NameRdVV) ) ) {

				DBSetLSsAndCOWs( pD, &pVV_DB[i] );

				pVl->vl_aChild[pVl->vl_Childs++]  = pVV_DB[i];

				j = i;
				if( pVV_DB[i].db_NameRdVV[0] == 0 
						|| pVV_DB[i].db_NameRdVV[0] == ' ' )	break;
				goto nextChild1;
			}
		}
ASSERT( N == pVl->vl_Childs );
	}

	DBUnlock( pDlgVN_VV );
	return( 0 );
err1:
	DBUnlock( pDlgVN_VV );
	LOG( neo_log, LogERR, "ERROR:[%s]", pVl->vl_DB.db_Name );
	return( -1 );
}

int
DBSetVV( void *pD, VV_t *pVV )
{
	DlgCache_t	*pDlgVN_VV;
	VV_DB_List_t	*pVV_List;
	VV_DB_t	*pVV_DB;
	int		i;
	int	Ret;

	LOG( neo_log, LogINF, "[%s]", pVV->vv_DB.db_Name ); 

	pDlgVN_VV	= DBLockR( pD, VN_VV_LIST );
	if( !pDlgVN_VV )	goto err;

	pVV_List	= ((VN_VV_List_t*)pDlgVN_VV->d_pVoid)->l_pVV_List;
	for( i = 0; i < pVV_List->l_Cnt; i++ ) {
		if( !strncmp( pVV_List->l_aVV[i].db_Name, pVV->vv_DB.db_Name, 
						sizeof(pVV_List->l_aVV[i].db_Name) ) ) {
			pVV_DB	= &pVV_List->l_aVV[i];
			goto next;
		}
	}
	goto err;
next:
	Ret = DBSetLSsAndCOWs( pD, pVV_DB );
	if( Ret < 0 )	goto err1;

	pVV->vv_DB	= *pVV_DB;

	DBUnlock( pDlgVN_VV );
	return( 0 );
err1:
	DBUnlock( pDlgVN_VV );
err:
	return( -1 );
}

VV_t*
DBGetVVByWR_TBL( void *pD, char *pVVName )
{
	VV_t	*pVV;
	DlgCache_t	*pDlgVN_VV;
	VV_DB_List_t	*pVV_List;
	VV_DB_t	*pVV_DB;
	int		i;
	int	Ret;

	LOG( neo_log, LogINF, "[%s]", pVVName ); 

	pDlgVN_VV	= DBLockR( pD, VN_VV_LIST );
	if( !pDlgVN_VV )	goto err;

	pVV_List	= ((VN_VV_List_t*)pDlgVN_VV->d_pVoid)->l_pVV_List;
	for( i = 0; i < pVV_List->l_Cnt; i++ ) {
		if( !strncmp( pVV_List->l_aVV[i].db_NameWrTBL, pVVName, 
						sizeof(pVV_List->l_aVV[i].db_NameWrTBL) ) ) {
			pVV_DB	= &pVV_List->l_aVV[i];
			goto next;
		}
	}
	goto err;
next:
	Ret = DBSetLSsAndCOWs( pD, pVV_DB );
	if( Ret < 0 )	goto err1;

	pVV	= (VV_t*)Malloc( sizeof(*pVV) );
	memset( pVV, 0, sizeof(*pVV) );

	pVV->vv_DB	= *pVV_DB;

	DBUnlock( pDlgVN_VV );

	return( pVV );
err1:
	DBUnlock( pDlgVN_VV );
err:
	return( NULL );
}

int
DBPutVV( void *pD, VV_t *pVV ) 
{
	Free( pVV );
	return( 0 );
}

int
_DBCreateTBL( void *pD, char *pName, int Ver )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	Name[256];
	r_tbl_t	*pTbl;
	r_item_t	aItem[MAX_ITEM];

	/* drop/create VV table */

	// drop
	sprintf( Name, "VT_%s_%d", pName, Ver );
	rdb_drop( pDBMgr, Name );

	// create
	memset( aItem, 0, sizeof(aItem) );

	strcpy( aItem[0].i_name, WR_VS );
	aItem[0].i_attr	= R_ULONG;
	aItem[0].i_len	= sizeof(unsigned long long);

	strcpy( aItem[1].i_name, WR_SI );
	aItem[1].i_attr	= R_INT;
	aItem[1].i_len	= 4;

	strcpy( aItem[2].i_name, WR_LV );
	aItem[2].i_attr	= R_STR;
	aItem[2].i_len	= 64;

	strcpy( aItem[3].i_name, WR_LS );
	aItem[3].i_attr	= R_UINT;
	aItem[3].i_len	= 4;

	pTbl	= rdb_create( pDBMgr, Name, 100, 4, aItem );
	if( !pTbl )	{
		goto err1;
	}
	rdb_close( pTbl );

	return( 0 );
err1:
	return( -1 );
}

int
_DBCreateCOW( void *pD, char *pName, int Ver )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	Name[256];
	r_tbl_t	*pTbl;
	r_item_t	aItem[MAX_ITEM];

	sprintf( Name, "VC_%s_%d", pName, Ver );
	rdb_drop( pDBMgr, Name );

	/* create COW table */
	memset( aItem, 0, sizeof(aItem) );

	strcpy( aItem[0].i_name, COW_UNIT );
	aItem[0].i_attr	= R_ULONG;
	aItem[0].i_len	= sizeof(unsigned long long);

	pTbl	= rdb_create( pDBMgr, Name, 100, 1, aItem );
	if( !pTbl )	{
		goto err1;
	}
	rdb_close( pTbl );

	return( 0 );
err1:
	return( -1 );
}


int
_DBDeleteVlByColumn( void *pD, char *pName, char *pColumn )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	char	sql[128];
	r_tbl_t	*pTbl;

	sprintf( sql, "delete from "VN_LIST" where %s='%s';", 
			pColumn, pName );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
_DBDeleteVlByVV( void *pD, char *pName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	char	sql[128];
	r_tbl_t	*pTbl;

	sprintf( sql, 
		"delete from "VN_LIST" where "VN_VV"='%s';", pName );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
_DBUpdateVlByVV( void *pD, char *pFr, char *pTo )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	char	sql[128];
	r_tbl_t	*pTbl;

	sprintf( sql, 
		"update "VN_LIST" set "VN_VV"='%s' where "VN_VV"='%s';", pTo, pFr );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
_DBDeleteVV( void *pD, char *pName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	char	sql[128];
	r_tbl_t	*pTbl;

	sprintf( sql, "delete from "VV_LIST" where "VV_NAME" = '%s';", pName );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

/* change VV Name */
int
_DBChangeVV( void *pD, char *pFrName, char *pToName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	char	sql[128];
	r_tbl_t	*pTbl;

	sprintf( sql, 
		"update "VV_LIST" set "VV_NAME"='%s' where "VV_NAME"='%s';", 
			pFrName, pToName );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
_DBReturnLVByVT( void *pD, char *pLVName, char *pVTName, int Cnt )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[128];
	r_tbl_t	*pTbl;
	int	Ret;

	sprintf( sql, 
		"update '%s' set "LV_FLAG"=0 where "LV_WR"='%s';", pLVName, pVTName );  

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	sprintf( sql, 
		"update '%s' set "LV_USABLE"="LV_USABLE"+%d where "LV_LV"='%s';",
					LV_LIST, Cnt, pLVName );  
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err:
	return( -1 );
}

/*
 *	Delete VV
 *		return LV,
 *		delete FrName VV record
 *		delete FrName Vl record
 *		change ToName to FrName
 */
// record list
typedef	struct LV_head {
	list_t	h_LnkLV;
	char	h_LV[PATH_NAME_MAX];
	int		h_Cnt;
/***
	list_t	h_LnkRec;
***/
	DHash_t	h_HashVS;
} LV_head_t;

// LV hash
typedef struct LV_update {
	Hash_t	u_HashLV;
	list_t	u_LnkLV;
} LV_update_t;

void
LV_head_Destroy( Hash_t *pHash, void *pVoid )
{
	LV_head_t		*pLV= (LV_head_t*)pVoid;
/***
	LV_record_VV_t	*pRec;

	while((pRec = list_first_entry(&pLV->h_LnkRec,LV_record_VV_t,r_LnkRec ))){
		list_del( &pRec->r_LnkRec );
		Free( pRec );
	}
***/
	DHashDestroy( &pLV->h_HashVS );
	Free( pLV );
}

int
LV_update_Init( LV_update_t *pUpdate )
{

	HashInit( &pUpdate->u_HashLV, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR,
			NULL, Malloc, Free, LV_head_Destroy );
	list_init( &pUpdate->u_LnkLV );
	return( 0 );
}

int
LV_update_Destroy( LV_update_t *pUpdate )
{
	HashDestroy( &pUpdate->u_HashLV );
	return( 0 );
}

LV_head_t*
LV_head_Alloc( char *pLVName )
{
	LV_head_t	*pLV;

	pLV = (LV_head_t*)Malloc( sizeof(*pLV) );
	memset( pLV, 0, sizeof(*pLV) );

	list_init( &pLV->h_LnkLV );
/***
	list_init( &pLV->h_LnkRec );
***/
	DHashInit( &pLV->h_HashVS, PRIMARY_1013, 200, PRIMARY_1009,
				HASH_CODE_U64, HASH_CMP_U64, NULL, Malloc, Free, NULL );

	strncpy( pLV->h_LV, pLVName, sizeof(pLV->h_LV) );

	return( pLV );
}

int
DBCopyFr( void *pD, VV_t *pVV, VV_IO_t *pCopy )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTblWr=NULL, *pTblCOW=NULL;
	char	*pRecWr=NULL, *pRecCOW=NULL, *pRec;
	uint64_t	VS, COW;
	int	n, i, len;
	int	Ret;
	VS_t		*pVS;
	VSDlg_t		*pDlg;
	uint64_t	uVS;
	int			SI;
	LS_t		*pLS, LS;
	LS_IO_t		*pLS_IO;
	LV_IO_t		ReqIO;
	r_sort_t	aSort[2];
	VS_Id_t	VS_Id;

	memset( &VS_Id, 0, sizeof(VS_Id) );
	strncpy( VS_Id.vs_VV, pVV->vv_DB.db_Name, sizeof(VS_Id.vs_VV) );

	if( pVV->vv_DB.db_NameCOW[0] != 0 && pVV->vv_DB.db_NameCOW[0] != ' ' ) {
		pTblCOW	= rdb_open( pDBMgr, pVV->vv_DB.db_NameCOW );
		if( !pTblCOW )	goto err1;
		n	= pTblCOW->t_rec_used;
		if( n > 0 ) {

			memset( aSort, 0, sizeof(aSort) );	
			strcpy( aSort[0].s_name, COW_UNIT );
			aSort[0].s_order	= R_ASCENDANT;
			rdb_sort( pTblCOW, 1, aSort );
			rdb_search( pTblCOW, NULL );

			pRecCOW	= Malloc( pTblCOW->t_rec_size*n );	// whole data
			n = rdb_find( pTblCOW, n, (r_head_t*)pRecCOW, 0 );
			ASSERT( n == pTblCOW->t_rec_used ); 

			for( i = 0, pRec = pRecCOW; i < pTblCOW->t_rec_used;
						i++, pRec += pTblCOW->t_rec_size ) {
				len	= sizeof( COW );
				rdb_get( pTblCOW, (r_head_t*)pRec, 
							COW_UNIT, (void*)&COW, &len );

				if( DHashGet( &pCopy->io_DHashCOW, &COW ) )	continue;

				DHashPut( &pCopy->io_DHashCOW, &COW, (void*)TRUE );

				SI	= SU2SI( pVV, COW );
				uVS	= SU2VS( pVV, COW );

				pDlg = VLockR( pVV, uVS );

				pVS	= HashListGet( &pDlg->vd_HashListVS, &uVS );
				ASSERT( pVS );

				pLS = &pVS->vs_aLS[SI];

				VS_Id.vs_VS	= uVS;
				VS_Id.vs_SI	= SI;
				pLS_IO	= LS_IOCreate( pCopy, &VS_Id, pLS );

				memset( &ReqIO, 0, sizeof(ReqIO) );
				list_init( &ReqIO.io_Lnk );

				ReqIO.io_Flag	= 0;
				ReqIO.io_Len	= SNAP_UNIT_SIZE;
				ReqIO.io_Off	= B2LO(pVV,SU2B(COW));	// in LS

				LS_IOInsertReqIO( pLS_IO, &ReqIO );

				VUnlock( pDlg );
			}
			Free( pRecCOW );
		}
		rdb_close( pTblCOW );
	} else {
		pTblWr	= rdb_open( pDBMgr, pVV->vv_DB.db_NameWrTBL );
		if( !pTblWr )	goto err1;
		n	= pTblWr->t_rec_used;
		if( n > 0 ) {
			pRecWr	= Malloc( pTblWr->t_rec_size*n );	// whole data

			memset( aSort, 0, sizeof(aSort) );	
			strcpy( aSort[0].s_name, WR_VS );
			aSort[0].s_order	= R_ASCENDANT;
			strcpy( aSort[1].s_name, WR_SI );
			aSort[1].s_order	= R_ASCENDANT;
			rdb_sort( pTblWr, 2, aSort );
			rdb_search( pTblWr, NULL );

			n = rdb_find( pTblWr, n, (r_head_t*)pRecWr, 0 );

			for( i = 0, pRec = pRecWr; i < pTblWr->t_rec_used;
						i++, pRec += pTblWr->t_rec_size ) {

				len	= sizeof( VS );
				rdb_get( pTblWr, (r_head_t*)pRec, WR_VS, (void*)&VS, &len );

				len	= sizeof( SI );
				rdb_get( pTblWr, (r_head_t*)pRec, WR_SI, (char*)&SI, &len ); 

				len	= sizeof( LS.ls_LV );
				rdb_get( pTblWr, (r_head_t*)pRec, WR_LV, LS.ls_LV, &len ); 
				len	= sizeof( LS.ls_LS );
				rdb_get( pTblWr, (r_head_t*)pRec, WR_LS, (void*)&LS.ls_LS, &len );

				Ret = DBGetCell( pDB, LS.ls_LV, LS.ls_Cell );
				if( Ret < 0 )	goto err2;

				VS_Id.vs_VS	= VS;
				VS_Id.vs_SI	= SI;
				pLS_IO	= LS_IOCreate( pCopy, &VS_Id, &LS );

				memset( &ReqIO, 0, sizeof(ReqIO) );
				list_init( &ReqIO.io_Lnk );

				ReqIO.io_Flag	= 0;
				ReqIO.io_Len	= VV_LS_SIZE;
				ReqIO.io_Off	= 0;	// in LS

				LS_IOInsertReqIO( pLS_IO, &ReqIO );
			}
			Free( pRecWr );
		}
		rdb_close( pTblWr );
	}
	return( 0 );
err2:
	if( pRecWr )	Free( pRecWr );
	if( pTblWr )	rdb_close( pTblWr );
err1:
	return( -1 );
}

void
HashDestroyVS( Hash_t *pHash, void * p )
{
	VS_free( p );
}

int
DBCopyTo( void *pD, Vol_t *pVl, VV_t *pVV, VV_IO_t *pCopy )
{
	DB_t *pDB 	= (DB_t*)pD;	
//	void *pDBMgr= pDB->db_pMgr;
	LS_IO_t	*pLS_IO;
	VS_t	*pVS;
	HashList_t	HashListVS;
	int	Ret;

	HashListInit( &HashListVS, PRIMARY_101, HASH_CODE_U64, HASH_CMP_U64, 
					NULL, Malloc, Free, HashDestroyVS );

	Ret = DBSetHashVS( pD, pVV, &HashListVS, 0, 0 );
	if( Ret < 0 )	goto err;

	list_for_each_entry( pLS_IO, &pCopy->io_LS_IO.hl_Lnk, l_Lnk ) {

		pVS	= (VS_t*)HashListGet( &HashListVS, &pLS_IO->l_VS.vs_VS );

		if( !pVS ) {	// one by one, allocate VS
			pVS	= VS_alloc( pVV, pLS_IO->l_VS.vs_VS );
			if( !pVS ) {
				goto err1;
			}
			Ret = DBAllocVS( pDB, pVS, pVl->vl_DB.db_Pool );
			if( Ret < 0 )	goto err1;

			HashListPut( &HashListVS, &pVS->vs_VS, pVS, vs_Lnk );
		}
		pLS_IO->l_LSTo	= pVS->vs_aLS[pLS_IO->l_VS.vs_SI];
	}
	HashListDestroy( &HashListVS );
	return( 0 );
err1:
err:
	HashListDestroy( &HashListVS );
	return( -1 );
}

int
DBReturnLSinVV( void *pD, VV_t *pDB_VV )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTblWR;
	char	*pRecWR, *pRec;
	int		i, n, len;
	DB_VT_t	DB_VT;
	LV_update_t	LVupdate;
	LV_head_t	*pLVhead;
	int	Ret;

	/* open/find WR and return LV  */
	/* XXX _DBLock ? */
	pTblWR	= rdb_open( pDBMgr, pDB_VV->vv_DB.db_NameWrTBL );
	if( !pTblWR )	goto err;

	n	= pTblWR->t_rec_used;
	if( n > 0 ) {
		pRecWR	= Malloc( pTblWR->t_rec_size*n );	// whole data

		rdb_search( pTblWR, NULL );
		n = rdb_find( pTblWR, n, (r_head_t*)pRecWR, 0 );
		ASSERT( n == pTblWR->t_rec_used ); 

		// prepare change LV of Update
		LV_update_Init( &LVupdate );

		for( i = 0, pRec = pRecWR; i < pTblWR->t_rec_used;
						i++, pRec += pTblWR->t_rec_size ) {

			memset( &DB_VT, 0, sizeof(DB_VT) );
			len	= sizeof( DB_VT.vt_LV );
			rdb_get( pTblWR, (r_head_t*)pRec, WR_LV, DB_VT.vt_LV, &len );

			len	= sizeof( DB_VT.vt_LS );
			rdb_get( pTblWR, (r_head_t*)pRec, WR_LS, 
									(char*)&DB_VT.vt_LS, &len );

			if( !(pLVhead = HashGet(&LVupdate.u_HashLV, DB_VT.vt_LV) ) ) {

				pLVhead	= LV_head_Alloc( DB_VT.vt_LV );

				HashPut( &LVupdate.u_HashLV, pLVhead->h_LV, pLVhead );

				list_add_tail( &pLVhead->h_LnkLV, &LVupdate.u_LnkLV );
			}
			pLVhead->h_Cnt++;
		}
		// Return LV
		list_for_each_entry( pLVhead, &LVupdate.u_LnkLV, h_LnkLV ) {

			Ret = _DBReturnLVByVT( pD, 
								pLVhead->h_LV, 
								pDB_VV->vv_DB.db_NameWrTBL,
								pLVhead->h_Cnt ); 
			if( Ret < 0 )	goto err2;
		}
		LV_update_Destroy( &LVupdate );
	}
	/* close table */
	rdb_close( pTblWR );

	return( 0 );
err2:
	LV_update_Destroy( &LVupdate );
	rdb_close( pTblWR );
err:
	return( -1 );
}

int
DBUpdateVVName( void *pD, char *pColumn, char *pFr, char *pTo )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	sql[256];
	int		Ret;

	sprintf( sql, "update "VV_LIST" set %s='%s' where %s='%s';",
					pColumn, pTo, pColumn, pFr );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	ASSERT( pTbl->t_rec_used == 1 );
	if( pTbl->t_rec_used != 1 )	goto err2;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err2:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
err1:
	return( -1 );
}

int
DBUpdateRdVV( void *pD, char *pName, char *pRdName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	sql[512];
	int		Ret;

	sprintf( sql, 
		"delete from "VV_LIST" where "VV_NAME"='%s';"
		"update "VV_LIST" set "VV_NAME"='%s' where "VV_NAME"='%s';"
		"delete from "VN_LIST" where "VN_VV"='%s';",
				pName, pName, pRdName, pRdName );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
/***
err2:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
***/
err1:
	return( -1 );
}

int
_DBLink( void *pD, char *pName, char *pRdName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	sql[128];
	int		Ret;

	sprintf( sql, 
		"update "VV_LIST" set "VV_RD_VV"='%s' where "VV_NAME"='%s';",
			pRdName, pName );

	LOG( neo_log, LogDBG, "[%s]", sql ); 

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

int
DBLink( void *pD, char *pName, char *pRdName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	DlgCache_t	*pDlgVN_VV;
	int	Ret;

	LOG( neo_log, LogINF, "[%s]->[%s]", pName, pRdName ); 

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	Ret = _DBLink( pD, pName, pRdName );
	if( Ret < 0 )	goto err;

	DBCommit( pDB );
	_DBUnlock( pDB );

	DBUnlockReturn( pDlgVN_VV );
	return( 0 );
err:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgVN_VV );
	return( -1 );
}

int
DBDeleteCommitComplete( void *pD, VV_t *pVV )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	DlgCache_t	*pDlgVN_VV;

	Ret = DBReturnLSinVV( pD, pVV );
	if( Ret < 0 )	goto err;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	/* delete from VV_LIST */
	 Ret = _DBDeleteVV( pD, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* delete from VN_LIST */
	 Ret = _DBDeleteVlByVV( pD, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* commit */
	rdb_commit( pDBMgr );

	/* drop tables */
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameWrTBL );
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameCOW );

	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBUnlockReturn( pDlgVN_VV );
err:
	rdb_rollback( pDBMgr );
	return( -1 );
}

int
DBDeleteCommitBottom( void *pD, VV_t *pVV )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	DlgCache_t	*pDlgVN_VV;

	Ret = DBReturnLSinVV( pD, pVV );
	if( Ret < 0 )	goto err;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	/* delete from VV_LIST */
	Ret = _DBDeleteVV( pD, pVV->vv_DB.db_Name );

	Ret = DBUpdateVVName( pD, VV_NAME, pVV->vv_DB.db_NameRdVV, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* delete from VN_LIST */
	 Ret = _DBDeleteVlByVV( pD, pVV->vv_DB.db_NameRdVV );
	if( Ret < 0 )	goto err1;

	/* commit */
	rdb_commit( pDBMgr );

	/* drop tables */
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameWrTBL );
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameCOW );

	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBUnlockReturn( pDlgVN_VV );
err:
	rdb_rollback( pDBMgr );
	return( -1 );
}

int
DBDeleteCommitTop( void *pD, VV_t *pVV )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	DlgCache_t	*pDlgVN_VV;

	Ret = DBReturnLSinVV( pD, pVV );
	if( Ret < 0 )	goto err;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	/* delete from VV_LIST */
	Ret = _DBDeleteVV( pD, pVV->vv_DB.db_Name );

	Ret = DBUpdateVVName( pD, VV_NAME, pVV->vv_DB.db_NameRdVV, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* delete from VN_LIST */
	 Ret = _DBDeleteVlByVV( pD, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;
	 Ret = _DBUpdateVlByVV( pD, pVV->vv_DB.db_NameRdVV, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* commit */
	rdb_commit( pDBMgr );

	/* drop tables */
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameWrTBL );
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameCOW );

	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBUnlockReturn( pDlgVN_VV );
err:
	rdb_rollback( pDBMgr );
	return( -1 );
}

int
DBDeleteCommitMerge( void *pD, VV_t *pVV, VV_t *pVVParent )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	DlgCache_t	*pDlgVN_VV;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	Ret = DBReturnLSinVV( pD, pVVParent );
	if( Ret < 0 )	goto err;

	/* delete from VV_LIST */
	Ret = _DBDeleteVV( pD, pVVParent->vv_DB.db_Name );

	Ret = DBUpdateVVName(pD, VV_NAME, pVV->vv_DB.db_Name, pVVParent->vv_DB.db_Name);
	if( Ret < 0 )	goto err1;

	/* delete from VN_LIST */
	 Ret = _DBDeleteVlByVV( pD, pVV->vv_DB.db_Name );
	if( Ret < 0 )	goto err1;

	/* commit */
	rdb_commit( pDBMgr );

	/* drop tables */
	rdb_drop( pDBMgr, pVVParent->vv_DB.db_NameWrTBL );
	rdb_drop( pDBMgr, pVVParent->vv_DB.db_NameCOW );

	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBUnlockReturn( pDlgVN_VV );
err:
	rdb_rollback( pDBMgr );
	return( -1 );
}

int
_DBUpdateUsable( void *pD, char *pLV, int Cnt )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int			Ret;
	char		sql[256];
	r_tbl_t		*pTbl;

	if( Cnt < 0 ) {
		sprintf( sql, 
		"update "LV_LIST" set "LV_USABLE"="LV_USABLE"-%d where "LV_LV"='%s';", 
		-Cnt, pLV );
	} else {
		sprintf( sql, 
		"update "LV_LIST" set "LV_USABLE"="LV_USABLE"+%d where "LV_LV"='%s';", 
		Cnt, pLV );
	}
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	return( 0 );
err1:
	return( -1 );
}

/*
 *	Size == 0 -> all
 */
int
DBSetHashVS( void *pD, VV_t *pVV, 
			HashList_t *pHashList, uint64_t iStart, uint64_t Size )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	*pBuf, *pRec;
	int		n, i, len;
	int64_t	iVS;
	VS_t	*pVS = NULL;
	int		SI;
	int		Ret;
	char	search[256];
	uint64_t	iEnd = iStart + Size;
//	DlgCache_t	*pDlgTable;

//	pDlgTable = DBLockR( pDB, pVV->vv_NameWrTBL );
	DlgCache_t	*pDlg;

	pDlg = DBLockR( pDB, LV_LIST );

	_DBLock( pDB );

	pTbl	= rdb_open( pDBMgr, pVV->vv_DB.db_NameWrTBL );
	if( !pTbl ) {
		errno = ENOENT;
		goto err1;
	}
	pBuf	= Malloc( pTbl->t_rec_size*pTbl->t_rec_used );
	if( !pBuf ) {
		errno	= ENOMEM;
		goto err2;
	}
	if( Size ) {
		sprintf( search, ""WR_VS">=%"PRIu64" && "WR_VS"<%"PRIu64"",
														iStart,iEnd );
		LOG( neo_log, LogINF, 
		"WrTBL[%s] search[%s]", pVV->vv_DB.db_NameWrTBL, search );

		rdb_search( pTbl, search );
	} else {
		LOG( neo_log, LogINF, "WrTBL[%s] search[NULL]", pVV->vv_DB.db_NameWrTBL );

		rdb_search( pTbl, NULL );
	}
	while( (n = rdb_find( pTbl, pTbl->t_rec_used, (r_head_t*)pBuf, 0 )) > 0 ) {

		for( i = 0, pRec = pBuf; i < n; i++, pRec += pTbl->t_rec_size ) {

			len	= sizeof( iVS );
			rdb_get( pTbl, (r_head_t*)pRec, WR_VS, (char*)&iVS, &len );

			if( !pVS || pVS->vs_VS != iVS ) {
				pVS	= HashListGet( pHashList, &iVS );
				if( !pVS ) {
					pVS	= VS_alloc( pVV, iVS );
					if( !pVS )	goto err3;

					HashListPut( pHashList, &iVS, pVS, vs_Lnk );
				}
			}
			len	= sizeof( SI );
			rdb_get( pTbl, (r_head_t*)pRec, WR_SI, (char*)&SI, &len ); 

			len	= sizeof( pVS->vs_aLS[SI].ls_LV );
			rdb_get( pTbl, (r_head_t*)pRec, WR_LV, 
									pVS->vs_aLS[SI].ls_LV, &len ); 
			len	= sizeof( pVS->vs_aLS[SI].ls_LS );
			rdb_get( pTbl, (r_head_t*)pRec, WR_LS, 
							(char*)&pVS->vs_aLS[SI].ls_LS, &len );

			Ret = _DBGetCell( pDB, 
								pVS->vs_aLS[SI].ls_LV, 
								pVS->vs_aLS[SI].ls_Cell );
			if( Ret < 0 )	goto err3;

			pVS->vs_aLS[SI].ls_Flags	|= LS_DEFINED;
		}
	}
	Free( pBuf );
	rdb_close( pTbl );

	_DBUnlock( pDB );
//	DBUnlock( pDlgTable );
	DBUnlock( pDlg );

	return( 0 );

err3:	Free( pBuf );
err2:	rdb_close( pTbl );
err1:	_DBUnlock( pDB );
//		DBUnlock( pDlgTable );
	DBUnlock( pDlg );
		return( -1 );
}

int
DBSetHashCOW( void *pD, VV_t *pVV, 
			DHash_t	*pDHashCOW, uint64_t iStart, uint64_t Size )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	*pBuf, *pRec;
	int		n, i, len;
	int64_t	iUnit;
	int		Ret;
	char	search[256];
//	DlgCache_t	*pDlgTable;
	uint64_t	iEnd = iStart + Size;

//	pDlgTable = DBLockR( pDB, pVV->vv_NameCOW );
	_DBLock( pDB );

	pTbl	= rdb_open( pDBMgr, pVV->vv_DB.db_NameCOW );
	if( !pTbl ) {
		errno = ENOENT;
		goto err1;
	}
	pBuf	= Malloc( pTbl->t_rec_size*pTbl->t_rec_used );
	if( !pBuf ) {
		errno	= ENOMEM;
		goto err2;
	}
	if( Size ) {
		sprintf( search, 
			""COW_UNIT">=%"PRIu64" && "COW_UNIT"<%"PRIu64"", iStart, iEnd );

		LOG( neo_log, LogINF, 
		"NameCOW[%s] search[%s]", pVV->vv_DB.db_NameCOW, search );

		rdb_search( pTbl, search );
	} else {
		LOG( neo_log, LogINF, "NameCOW[%s] search[NULL]", pVV->vv_DB.db_NameCOW );
		rdb_search( pTbl, NULL );
	}
	while( (n = rdb_find( pTbl, pTbl->t_rec_used, (r_head_t*)pBuf, 0 )) > 0 ) {

		for( i = 0, pRec = pBuf; i < n; i++, pRec += pTbl->t_rec_size ) {

			rdb_get( pTbl, (r_head_t*)pRec, COW_UNIT, (char*)&iUnit, &len );

			/* Set Hash */
			Ret = DHashPut( pDHashCOW, &iUnit, (void*)TRUE );
			if( Ret < 0 )	goto err3;
		}
	}
	Free( pBuf );
	rdb_close( pTbl );
	_DBUnlock( pDB );
//	DBUnlock( pDlgTable );
	return( 0 );

err3:	Free( pBuf );
err2:	rdb_close( pTbl );
err1:	_DBUnlock( pDB );
//		DBUnlock( pDlgTable );
		return( -1 );
}

int
DBPutHashCOW( void *pD, char *pNameCOW, int n, uint64_t* pCOW  )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t	*pTbl;
	char	*pRec, *pW;
	int		i;
	int		Ret;
//	DlgCache_t	*pDlg;

//	pDlg = DBLockW( pDB, pNameCOW );
	_DBLock( pDB );

	LOG( neo_log, LogINF, "COW[%s] n=%d for PutHashCOW", pNameCOW, n );

	/* set tble */
	pTbl	= rdb_open( pDBMgr, pNameCOW );
	if( !pTbl ) {
		errno = ENOENT;
		goto err1;
	}
	pRec	= Malloc( pTbl->t_rec_size*n );
	if( !pRec ) {
		errno	= ENOMEM;
		goto err2;
	}
	for( i = 0, pW = pRec; i < n; i++, pW += pTbl->t_rec_size ) {
		rdb_set( pTbl, (r_head_t*)pW, COW_UNIT, (char*)&pCOW[i] );
	}
	Ret = rdb_insert( pTbl, n, (r_head_t*)pRec, 0, NULL, 0 );
	if( Ret < 0 )	goto err3;

	Free( pRec );

	rdb_close( pTbl );	// cmmitted

	_DBUnlock( pDB );
//	DBUnlock( pDlg );

	return( 0 );

err3:	
	Free( pRec );
err2:
	rdb_cancel( pTbl );
	rdb_close( pTbl );
err1:
	_DBUnlock( pDB );
//	DBUnlock( pDlg );
	return( -1 );
}

/*
 *	Set all informations into VS
 */
int
DBAllocVS( void *pD, VS_t *pVS, char *pPool )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	SI;
	int	Ret;
	char	sql[512], *pW;
	r_tbl_t	*pTbl;
	int		Stripes;
	DlgCache_t	*pDlg;

	pDlg = DBLockW( pDB, LV_LIST );

	_DBLock( pDB );

	LOG( neo_log, LogINF, ""LV_LIST", LV, VT, "VN_LIST" for AllocVS" );

	Stripes = pVS->vs_pVV->vv_Stripes;
	
	Ret = _DBGetLV( pDB, pPool, Stripes, pVS->vs_aLS );
	if( Ret ) {
		LOG( neo_log, LogERR, "#### Can't get LS in [%s] ####", pPool );
		goto err1;
	}

	for( SI = 0; SI < Stripes; SI++ ) {

		Ret = _DBAllocLS( pDB, pVS->vs_pVV->vv_DB.db_NameWrTBL, pVS->vs_VS, SI, 
				pVS->vs_aLS[SI].ls_LV, &pVS->vs_aLS[SI].ls_LS );
		if( Ret )	goto err1;
		pVS->vs_aLS[SI].ls_Flags	|= LS_DEFINED;

		Ret = DBGetCell( pDB, pVS->vs_aLS[SI].ls_LV, 
								pVS->vs_aLS[SI].ls_Cell );
		if( Ret )	goto err1;
		pVS->vs_aLS[SI].ls_Flags	|= LS_CELL_DEFINED;
	}
	/* set VT_XXX */
	pW	= sql;
	for( SI = 0; SI < Stripes; SI++ ) {
		sprintf( pW, "insert into '%s' values(%"PRIu64",%d,'%s',%d);",
			pVS->vs_pVV->vv_DB.db_NameWrTBL, pVS->vs_VS, SI, 
			pVS->vs_aLS[SI].ls_LV, pVS->vs_aLS[SI].ls_LS );
			pW	+= strlen( pW );
	}
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	/* update Version of VN_LIST in atomic */
	sprintf( sql, 
		"update "VN_LIST" set "VN_VER" = "VN_VER"+1 where "VN_VV" = '%s';",
			pVS->vs_pVV->vv_DB.db_Name );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	DBCommit( pDB );

	_DBUnlock( pDB );
	DBUnlockReturn( pDlg );

	LOG(neo_log,LogINF, "OK");
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlg );
	LOG(neo_log,LogERR, "NG");
	return( -1 );
}

int
DBFreeVS( void *pD, VS_t *pVS )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	SI;
	char	sql[512];
	char	*pFmt = "delete from '%s' where "WR_VS"=%"PRIu64" and "WR_SI"=%d;"; 
	char	*pW;
	int	Ret;
	r_tbl_t		*pTbl;
	int	Stripes;

	Stripes = pVS->vs_pVV->vv_Stripes;

	pW = sql;
	for( SI = 0; SI < Stripes; SI++ ) {
		_DBFreeLS( pDB, pVS->vs_aLS[SI].ls_LV, pVS->vs_aLS[SI].ls_LS );
		sprintf( pW, pFmt, pVS->vs_pVV->vv_DB.db_Name, pVS->vs_VS, SI );
		pW	+= strlen( pW );
	}
	/* delete VS */
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	/* update Version of VN_LIST */
	sprintf( sql, 
		"update "VN_LIST" set "VN_VER" = "VN_VER"+1 where "VN_VV" = '%s';",
			pVS->vs_pVV->vv_DB.db_Name );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	/* commit */
	DBCommit( pDB );
	return( 0 );
err1:
	DBRollback( pDB );
	return( -1 );
}

int
_DBGetLV( void *pD, char *pPool, int N, LS_t *pLS )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTbl;
	r_sort_t	Sort;
	int	Ret;
	char	*pBuf, *pRec;
	int		n;
	int		i, m, len;
	char	search[64];

	pTbl	= rdb_open( pDBMgr, LV_LIST );
	if( !pTbl ) {
		errno = ENOENT;
		goto err1;
	}
	memset( &Sort, 0, sizeof(Sort) );
	strcpy( Sort.s_name, LV_USABLE );
	Sort.s_order	= R_DESCENDANT;
	Ret = rdb_sort( pTbl, 1, &Sort );

	sprintf( search, ""LV_POOL"=='%s'&&"LV_FLAG"==0", pPool );
	Ret = rdb_search( pTbl, search );
	if( Ret < 0 )	goto err2;

	pBuf	= Malloc( pTbl->t_rec_size*N );
	if( !pBuf ) {
		errno	= ENOMEM;
		goto err3;
	}
	n = rdb_find( pTbl, N, (r_head_t*)pBuf, 0 );
	if( n <= 0 ) {
		errno	= ENOENT;
		goto err3;
	}
	int	Usable = 0;

	for( i = 0, pRec = pBuf; i < n; i++, pRec += pTbl->t_rec_size ) {
		len	= sizeof(m);
		rdb_get( pTbl, (r_head_t*)pRec, LV_USABLE, (char*)&m, &len ); 
		Usable += m;		
	}
	if( Usable < N ) {
		errno	= ENOSPC;
		goto err3;
	}
	for( i = 0, pRec = pBuf; i < N; i++, pRec += pTbl->t_rec_size ) {
		if( i == n )	pRec = pBuf;
		len	= sizeof(pLS[i].ls_LV);
		rdb_get( pTbl, (r_head_t*)pRec, LV_LV, (char*)pLS[i].ls_LV, &len ); 
	}
	Free( pBuf );
	rdb_close( pTbl );	// Sort and Refer
	return( 0 );
err3:	Free( pBuf );
err2:	rdb_close( pTbl );
err1:	return( -1 );
}

int
_DBUpdateLV( void *pD, char *pName, int Adds )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char set[128];
	char where[128];
	int			Ret;
	char		sql[256];
	r_tbl_t		*pTbl;

	sprintf( set, 
		"set "LV_TOTAL"="LV_TOTAL"+%d,"LV_USABLE"="LV_USABLE"+%d", Adds, Adds );
	sprintf( where, "where "LV_LV" = '%s'", pName );
	sprintf( sql, "update "LV_LIST" %s %s;", set, where );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );
	return( 0 );
err1:
	return( -1 );
}

/*
 *	Allocate LS
 */
static	int
_DBAllocLS( void *pD, 
		char *pTBL, uint64_t VS, int SI, char *pLV, uint32_t *pLS )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	Rec[512];
	int	Ret;
	r_tbl_t		*pTbl;
	int			len;
	int			n;

	/* get LS  */
	pTbl	= rdb_open( pDBMgr, pLV );
	if( !pTbl ) {
		errno = ENOENT;
		goto err1;
	}
	Ret = rdb_search( pTbl, ""LV_FLAG"==0" );
	if( Ret < 0 )	goto err1;
	n = rdb_find( pTbl, 1, (r_head_t*)Rec, 0 );
	if( n <= 0 ) {
		rdb_close( pTbl );
		goto err1;
	}
	rdb_get( pTbl, (r_head_t*)Rec, LV_LS, (char*)pLS, &len ); 

	n = 1;
	rdb_set( pTbl, (r_head_t*)Rec, LV_FLAG, (char*)&n );
	rdb_set( pTbl, (r_head_t*)Rec, LV_WR, pTBL );
	rdb_set( pTbl, (r_head_t*)Rec, LV_VS, (char*)&VS );
	rdb_set( pTbl, (r_head_t*)Rec, LV_SI, (char*)&SI );

	Ret = rdb_update( pTbl, 1, (r_head_t*)Rec, R_NOWAIT ); 

	rdb_close_client( pTbl );

	/* update LV_LIST, decrement Usable */
	_DBUpdateUsable( pDB, pLV, -1 );

	return( 0 );
err1:
	return( -1 );
}

/*
 *	Delete or Add and Increment
 */
static	int
_DBFreeLS( void *pD, char *pLV, uint32_t LS )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[256];
	int	Ret;
	r_tbl_t		*pTbl;

	/* update pLV table */
	sprintf( sql, "update %s set "LV_FLAG"=0 where "LV_LS"=%d;", pLV, LS );
	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}

	/* update LV_LIST, increment Usable */
	_DBUpdateUsable( pDB, pLV, 1 );
	return( 0 );
err1:
	return( -1 );
}

int
_DBGetCell( void *pD, char *pLV_Name, char *pCell )
{
	DB_t *pDB 	= (DB_t*)pD;	
	LV_DB_t	*pLV;
//	DlgCache_t	*pDlg;

//	pDlg = DBLockR( pDB, LV_LIST );

	pLV = (LV_DB_t*)HashGet( &pDB->db_HashCell, pLV_Name );
	if( !pLV )	goto err;

	strcpy( pCell, pLV->db_Cell );

	LOG( neo_log, LogDBG, "LV[%s] Cell[%s]", pLV_Name, pCell );

//	DBUnlock( pDlg );
	return( 0 );
	
err:
//	DBUnlock( pDlg );
	return( -1 );
}

int
DBGetCell( void *pD, char *pLV_Name, char *pCell )
{
	DB_t *pDB 	= (DB_t*)pD;	
	DlgCache_t	*pDlg;
	int		Ret;

	pDlg = DBLockR( pDB, LV_LIST );

	Ret = _DBGetCell( pD, pLV_Name, pCell );

	DBUnlock( pDlg );

	return( Ret );
}

/*
 *	select LV_TOTAL, LV_USAVLE from LV_LIST where LV_LV = %s
 */
static int
_DBisLVinLV_LIST( void *pD, char *pLVName, int *pTotal, int *pUsable )
{
	DB_t *pDB 	= (DB_t*)pD;	
	DlgCache_t		*pDlg;
	LV_DB_t			*pLV;
	LV_DB_List_t	*pLV_List;
	int	i;

	pDlg = DBLockR( pDB, LV_LIST );
	if( !pDlg )	return -1;

	pLV_List	= (LV_DB_List_t*)pDlg->d_pVoid;
	for( i = 0; i < pLV_List->l_Cnt; i++ ) {
		pLV	= &pLV_List->l_aLV[i];
		if( !strncmp( pLV->db_Name, pLVName, sizeof(pLV->db_Name) ) ) {
			*pTotal		= pLV->db_Total;
			*pUsable	= pLV->db_Usable;

			DBUnlock( pDlg );
			return( TRUE );
		}
	}
	DBUnlock( pDlg );
err:
	return( FALSE );
}

int
DBIncLV( void *pD, char *pLVName, int Inc )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTbl;
	int	Total, Usable;
	unsigned int	iUINT;
	char	*pStart, *pR;
	int		i;
	DlgCache_t	*pDlg;

	if( !_DBisLVinLV_LIST( pDB, pLVName, &Total, &Usable ) ) goto err1;

	pDlg = DBLockW( pDB, LV_LIST );
	if( !pDlg )	goto err1;

	_DBLock( pDB );

	/* pLVName */
	pTbl	= rdb_open( pDBMgr, pLVName );

	pStart = Malloc( pTbl->t_rec_size * Inc );

	for( i = 0, iUINT = Total, pR = pStart; i < Inc; 
					i++, iUINT++, pR += pTbl->t_rec_size ) {
		rdb_set( pTbl, (r_head_t*)pR, LV_LS, (char*)&iUINT );
	}	
	rdb_insert( pTbl, Inc, (r_head_t*)pStart, 0, 0, 0 );
	rdb_close_client( pTbl );
	Free( pStart );

	/* LV_LIST */
	_DBUpdateLV( pDB, pLVName, Inc );

	DBCommit( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlg );
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	return( -1 );
}

int
DBCreateLV( void *pD, char *pLVName, 
			char *pCellName, char *pPoolName, int Segments )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[256];
	int	n;
	int	Ret;
	char	Name[64];
	r_item_t	aItem[MAX_ITEM];
	r_tbl_t		*pTbl;
	int	Total, Usable;
	unsigned long long	iULONG;
	unsigned int	iUINT;
	char	*pStart, *pR;
	DlgCache_t	*pDlg/*, *pDlgCell*/;

//	MtxLock( &DB_Mtx );

	/* Already exist? */
	if( _DBisLVinLV_LIST( pDB, pLVName, &Total, &Usable ) )	goto err0;

	pDlg = DBLockW( pDB, LV_LIST );
//	pDlgCell = DBLockW( pDB, CELL_LIST );

	_DBLock( pDB );

	/* new one */
	/* pLVName */
	memset( aItem, 0, sizeof(aItem) );

	strcpy( aItem[0].i_name, LV_LS );
	aItem[0].i_attr	= R_UINT;
	aItem[0].i_len	= 4;

	strcpy( aItem[1].i_name, LV_FLAG );
	aItem[1].i_attr	= R_UINT;
	aItem[1].i_len	= 4;

	strcpy( aItem[2].i_name, LV_WR );
	aItem[2].i_attr	= R_STR;
	aItem[2].i_len	= 64;

	strcpy( aItem[3].i_name, LV_VS );
	aItem[3].i_attr	= R_ULONG;
	aItem[3].i_len	= sizeof(unsigned long long);

	strcpy( aItem[4].i_name, LV_SI );
	aItem[4].i_attr	= R_INT;
	aItem[4].i_len	= 4;

	sprintf( Name, "%s", pLVName );
	pTbl	= rdb_create( pDBMgr, Name, 100, 5, aItem );
	if( !pTbl )	{
		goto err1;
	}

	pStart = Malloc( pTbl->t_rec_size * Segments );
	memset( pStart, 0, pTbl->t_rec_size*Segments );
	for( iULONG = 0, iUINT = 0, pR = pStart; iULONG < Segments; 
						iULONG++, iUINT++, pR += pTbl->t_rec_size ) {
		rdb_set( pTbl, (r_head_t*)pR, LV_LS, (char*)&iUINT );
	}	
	rdb_insert( pTbl, Segments, (r_head_t*)pStart, 0, 0, 0 );
	rdb_close_client( pTbl );
	Free( pStart );

	/* LV_LIST */
	sprintf( sql, 
		"insert into "LV_LIST" values('%s',0,%d,%d,'%s','%s');", 
		pLVName, Segments, Segments, pPoolName, pCellName  );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	if( (n = pTbl->t_rec_used ) <= 0 )	goto err2;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}

	DBCommit( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlg );
	return( 0 );
err2:
	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlock( pDlg );
err0:
	return( -1 );
}

int
DBDeleteLV( void *pD, char *pLVName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[512];
	int	Ret;
	r_tbl_t		*pTbl;
	int	Total, Usable;
	DlgCache_t	*pDlg/*, *pDlgCell*/;
	
	/* LV_LIST */
	Ret = _DBisLVinLV_LIST( pDB, pLVName, &Total, &Usable );
	if ( Ret < 0 ) {
		goto err0;
	}
	else if ( Ret == FALSE ) {
		errno = ENOENT;
		LOG( neo_log, LogERR, "[%s] does not exist", pLVName );
		return 0;
	}

	if( Usable < Total ) {
		errno	= EBUSY;
		LOG( neo_log, LogERR, "[%s] is in use", pLVName );
		return 0;
	}
	pDlg = DBLockW( pDB, LV_LIST );
//	pDlgCell = DBLockW( pDB, CELL_LIST );

	_DBLock( pDB );

	/* delete from LV_LIST, SEQ_LIST and Drop */
	sprintf( sql, 
		"delete from "LV_LIST" where "LV_LV" = '%s';"
		"drop table '%s';", pLVName, pLVName ); 

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	DBCommit( pDB );
	_DBUnlock( pDB );
	DBUnlock( pDlg );
//	DBUnlock( pDlgCell );
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlock( pDlg );
//	DBUnlock( pDlgCell );
err0:
	return( -1 );
}

int
DBMoveLV( void *pD, char *pFrLV, char *pToLV )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	r_tbl_t		*pTblFr, *pTbl;
	int	Total, Usable, ToTotal, ToUsable;
	char	*pStart, *pR;
	int		i, n, len;
	DlgCache_t	*pDlg;
	char	sql[1024];
	VS_Id_t	VS_Id;
	LS_t	LS, ToLS;
	VV_t	*pVV, *pW;
	VV_IO_t	MoveIO;
	LS_IO_t	*pLS_IO;
	LV_IO_t	ReqIO;
	int	Ret;

	if( !_DBisLVinLV_LIST( pDB, pFrLV, &Total, &Usable ) ) {
		errno	= ENOENT;
		 goto err;
	}
	if( !_DBisLVinLV_LIST( pDB, pToLV, &ToTotal, &ToUsable ) ) {
		errno	= ENOENT;
		 goto err;
	}
	if( Total - Usable > ToUsable ) {
		errno	= ENOSPC;
		goto err;
	}
	pDlg = DBLockW( pDB, LV_LIST );
	_DBLock( pDB );

	/* Suppress LS allocate */
	sprintf( sql, 
		"update "LV_LIST" set "LV_FLAG"=1 where "LV_LV" = '%s';", pFrLV );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}
	_DBUnlock( pDB );
	DBUnlockReturn( pDlg );

	/* pFrLV loop  */
	pTblFr	= rdb_open( pDBMgr, pFrLV );

	pStart = Malloc( pTblFr->t_rec_size * pTblFr->t_rec_used );

	rdb_search( pTblFr, ""LV_FLAG"!=0" );
	n	= rdb_find( pTblFr, pTblFr->t_rec_used, (r_head_t*)pStart, 0 );
	if( n < 0 )	goto err2;

	for( i = 0, pR = pStart; i < n; i++, pR += pTblFr->t_rec_size ) {

		memset( &LS, 0, sizeof(LS) );
		memset( &ToLS, 0, sizeof(ToLS) );

		VV_IO_init( &MoveIO );

		len	= sizeof(LS.ls_LS);
		rdb_get( pTblFr, (r_head_t*)pR, LV_LS, (char*)&LS.ls_LS, &len );
		len	= sizeof(VS_Id.vs_VV);
		rdb_get( pTblFr, (r_head_t*)pR, LV_WR, (char*)VS_Id.vs_VV, &len );
		len	= sizeof(VS_Id.vs_VS);
		rdb_get( pTblFr, (r_head_t*)pR, LV_VS, (char*)&VS_Id.vs_VS, &len );
		len	= sizeof(VS_Id.vs_SI);
		rdb_get( pTblFr, (r_head_t*)pR, LV_SI, (char*)&VS_Id.vs_SI, &len );

		/* Suppress Write */
		pW = DBGetVVByWR_TBL( pD, VS_Id.vs_VV );
		if( !pW )	goto err2;

		pVV	= VV_getW( pW->vv_DB.db_Name, TRUE, NULL );
		Free( pW );
		if( !pVV )	goto err2;

		pDlg = DBLockW( pDB, LV_LIST );
		_DBLock( pDB );

		/* alloc LS of ToLV */
		Ret = _DBAllocLS(pD, VS_Id.vs_VV, VS_Id.vs_VS, VS_Id.vs_SI, pToLV,
							&ToLS.ls_LS );
		if( Ret < 0 )	goto err3;

		/* copy LS */
		strcpy( LS.ls_LV, pFrLV );
		strcpy( ToLS.ls_LV, pToLV );

		Ret = DBGetCell( pDB, LS.ls_LV, LS.ls_Cell );
		if( Ret < 0 )	goto err2;
		Ret = DBGetCell( pDB, ToLS.ls_LV, ToLS.ls_Cell );
		if( Ret < 0 )	goto err2;

		pLS_IO	= LS_IOCreate( &MoveIO, &VS_Id, &LS );

		pLS_IO->l_LSTo	= ToLS;

		memset( &ReqIO, 0, sizeof(ReqIO) );
		list_init( &ReqIO.io_Lnk );

		ReqIO.io_Flag	= 0;
		ReqIO.io_Len	= VV_LS_SIZE;
		ReqIO.io_Off	= 0;	// in LS

		LS_IOInsertReqIO( pLS_IO, &ReqIO );

		Ret = VVKickReqIO( &MoveIO, WM_COPY );

		/* return LS of FrLV */
		sprintf( sql, 
		"update '%s' set "WR_LV"= '%s', "WR_LS" = %u "
						"where "WR_LV"='%s' and "WR_LS"=%u;"
		"update '%s' set "LV_FLAG"=0 where "LV_LS" = %u;"
		"update "LV_LIST" set "LV_USABLE"="LV_USABLE"+1 where "LV_LV"='%s';",

			VS_Id.vs_VV, pToLV, ToLS.ls_LS, pFrLV, LS.ls_LS,
			pFrLV, LS.ls_LS, 
			pFrLV );

		Ret = SqlExecute( pDBMgr, sql );
		if( Ret < 0 )	goto err2;
		Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
		if( Ret < 0 )	goto err2;

		while( pTbl ) {
			SqlResultClose( pTbl );
			if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
		}

		_DBUnlock( pDB );
		DBUnlockReturn( pDlg );

		VV_IO_destroy( &MoveIO );

		/* Release Write */
		VV_put( pVV, TRUE );	// cache out
	}	
	rdb_close_client( pTblFr );

	sprintf( sql, 
		"update "LV_LIST" set "LV_FLAG"=0 where "LV_LV" = '%s';",
		pFrLV );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err0;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err0;

	while( pTbl ) {
		SqlResultClose( pTbl );
		if( SqlResultNextOpen( pDBMgr, &pTbl ) ) break;
	}

	DBCommit( pDB );
	return( 0 );
err1:
	_DBUnlock( pDB );
	DBUnlock( pDlg );
err:
	return( -1 );
err0:
	DBRollback( pDB );
	return( -1 );
err3:
	_DBUnlock( pDB );
	DBUnlock( pDlg );
	VV_put( pVV, TRUE );
err2:
	rdb_close( pTblFr );
	Free( pStart );
	DBRollback( pDB );
	return( -1 );
}

bool_t
DBisVVinVN_LIST( void *pD, char *pVVName )
{
	DlgCache_t		*pDlgVN_VV;
	VN_DB_List_t	*pVN_List;
	bool_t	bFind = FALSE;
	int		i;

	pDlgVN_VV	= DBLockR( pD, VN_VV_LIST );
	if( !pDlgVN_VV )	goto err;

	pVN_List	= ((VN_VV_List_t*)pDlgVN_VV->d_pVoid)->l_pVN_List;
	for( i = 0; i < pVN_List->l_Cnt; i++ ) {
		if( !strncmp( pVN_List->l_aVN[i].db_Name, pVVName, 
						sizeof(pVN_List->l_aVN[i].db_Name) ) ) {
			bFind	= TRUE;
			break;
		}
	}
	DBUnlock( pDlgVN_VV );
	return( bFind );
err:
	return( FALSE );
}

int
DBCreateVV( void *pD, char *pVVName, 
		char *pTarget, uint64_t Size, int Stripes, char *pPool )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[256];
	int	n;
	int	Ret;
	r_tbl_t		*pTbl;
	int Ver = 0;
	DlgCache_t	*pDlgVN_VV;

	LOG( neo_log, LogINF, ""VN_LIST", "VV_LIST" for [%s]", pVVName );

	if( DBisVVinVN_LIST( pDB, pVVName ) )	goto err1;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	/* create VT table */
	Ret = _DBCreateTBL( pDB, pVVName, Ver );
	if( Ret < 0 )	goto err2;

	/* insert pVVName into VN_LIST and VV_LIST */
	sprintf( sql, 
		"insert into "VN_LIST" values('%s','%s','%s',%"PRIu64",%d,%d,0,%lu,"
		"'"TYPE_VOLUME"',0);"
		"insert into "VV_LIST" values('%s','VT_%s_%d',' ',' ');",
		pVVName, pTarget, pPool, Size, Stripes, Ver, time(0),
		pVVName, pVVName, Ver );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	if( (n = pTbl->t_rec_used ) <= 0 )	goto err3;

	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );

	/* commit */
	DBCommit( pDB );

	_DBUnlock( pDB );

	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err3:
	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );
err2:
	DBRollback( pDB );
	_DBUnlock( pDB );

	DBUnlockReturn( pDlgVN_VV );
err1:
	return( -1 );
}

int
_DBCreateSnap( void *pD, Vol_t *pVl, char *pSnapName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[512];
	int	n;
	int	Ret;
	r_tbl_t		*pTbl;
	int Ver;

	LOG( neo_log, LogINF, "[%s] to [%s]", pSnapName, pVl->vl_DB.db_Name );

	// increment ver
	Ver = ++pVl->vl_DB.db_Ver;

	/* create VT table */
	Ret = _DBCreateTBL( pDB, pVl->vl_DB.db_Name, Ver );
	if( Ret < 0 )	goto err1;

	Ret = _DBCreateCOW( pDB, pVl->vl_DB.db_Name, Ver );
	if( Ret < 0 )	goto err1;

	/* update Ver in VN_LIST, insert pVVName into VN_LIST and VV_LIST */
	sprintf( sql, 
	"update "VN_LIST" set "VN_VER"=%d where "VN_VV"='%s';"
	"insert into "VN_LIST" values('%s','%s','%s',%"PRIu64",%d,%d,0,%lu,"
	"'"TYPE_SNAPSHOT"',0);"
	"update "VV_LIST" set "VV_NAME"='%s' where "VV_NAME"='%s';"
	"insert into "VV_LIST" values('%s','VT_%s_%d','VC_%s_%d','%s');",

		Ver, pVl->vl_DB.db_Name,
		pSnapName, pVl->vl_DB.db_Target, pVl->vl_DB.db_Pool, 
				pVl->vl_DB.db_Size, pVl->vl_DB.db_Stripes, Ver, time(0),
		pSnapName, pVl->vl_DB.db_Name,
			pVl->vl_DB.db_Name, pVl->vl_DB.db_Name, Ver, 
			pVl->vl_DB.db_Name, Ver, pSnapName );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	if( (n = pTbl->t_rec_used ) <= 0 )	goto err2;

	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );

	return( 0 );
err2:
	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );
err1:
	return( -1 );
}

int
DBCreateSnap( void *pD, Vol_t *pVl, char *pSnapName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	DlgCache_t	*pDlgVN_VV;
	int	Ret;

	LOG( neo_log, LogINF, "[%s] to [%s]", pSnapName, pVl->vl_DB.db_Name );

	if( DBisVVinVN_LIST( pDB, pSnapName ) )	goto err1;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	Ret = _DBCreateSnap( pD, pVl, pSnapName );
	if( Ret < 0 )	goto err1;

	/* commit */
	DBCommit( pDB );

	_DBUnlock( pDB );

	DBUnlockReturn( pDlgVN_VV );
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgVN_VV );
	return( -1 );
}

int
DBCopyCreateSnapLink( void *pD, Vol_t *pVlFr, char *pSnapFr,
								Vol_t *pVlTo, char *pSnapTo )
{
	DB_t *pDB 	= (DB_t*)pD;	
	DlgCache_t	*pDlgVN_VV;
	int	Ret;

	LOG( neo_log, LogINF, "Fr:[%s]->[%s] To:[%s]->[%s]", 
			pSnapFr, pVlFr->vl_DB.db_Name, pSnapTo, pVlTo->vl_DB.db_Name );

	if( DBisVVinVN_LIST( pDB, pSnapFr ) )	goto err1;
	if( DBisVVinVN_LIST( pDB, pSnapTo ) )	goto err1;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	Ret = _DBCreateSnap( pDB, pVlFr, pSnapFr );
	if( Ret < 0 )	goto err1;

	Ret = _DBCreateSnap( pDB, pVlTo, pSnapTo );
	if( Ret < 0 )	goto err1;

	Ret = _DBLink( pDB, pVlTo->vl_DB.db_Name, pSnapFr );
	if( Ret < 0 )	goto err1;

	/* commit */
	DBCommit( pDB );
	_DBUnlock( pDB );

	DBUnlockReturn( pDlgVN_VV );
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgVN_VV );
	return( -1 );
}

int
DBRestoreSnap( void *pD, VV_t *pVV, char *pSnapName )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	int	Ret;
	DlgCache_t	*pDlgVN_VV, *pDlgLV;

	LOG( neo_log, LogINF, ""VN_LIST", "VV_LIST" for RestoeSnap" );

	if( !DBisVVinVN_LIST( pDB, pSnapName ) )	goto err;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );
	pDlgLV	= DBLockW( pD, LV_LIST );

	_DBLock( pDB );
	// return LV
	Ret = DBReturnLSinVV( pD, pVV );
	if( Ret < 0 )	goto err1;

	// link to next-snap
	Ret = DBUpdateRdVV( pD, pVV->vv_DB.db_Name, pSnapName );
	if( Ret < 0 )	goto err1;

	/* commit */
	DBCommit( pDB );
	_DBUnlock( pDB );

	/* drop tables */
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameWrTBL );
	rdb_drop( pDBMgr, pVV->vv_DB.db_NameCOW );


	DBUnlockReturn( pDlgLV );
	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgLV );
	DBUnlockReturn( pDlgVN_VV );
err:
	return( -1 );
}

VS_t*
VS_alloc( VV_t *pVV, uint64_t iVS )
{
	VS_t	*pVS;

	pVS	= (VS_t*)Malloc( sizeof(*pVS) );
	if( !pVS ) {
		errno = ENOMEM;
		goto err;
	}
	memset( pVS, 0, sizeof(*pVS) );
	list_init( &pVS->vs_Lnk );
	pVS->vs_VS		= iVS;
	pVS->vs_pVV		= pVV;
	MtxInit( &pVS->vs_Mtx );

	return( pVS );
err:
	return( NULL );
}

void
VS_free( VS_t *pVS )
{
	Free( pVS );
}

int
DBRenameVolume( void *pD, char *pFr, char *pTo )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[512];
	r_tbl_t		*pTbl;
	int 	Ret;
	DlgCache_t	*pDlgVN_VV;

	LOG( neo_log, LogINF, ""VN_LIST", "VV_LIST"" );

	if( !DBisVVinVN_LIST( pDB, pFr ) )	goto err1;
	if( DBisVVinVN_LIST( pDB, pTo ) )	goto err1;

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	sprintf( sql,	
		"update "VN_LIST" set "VN_VV"='%s' where "VN_VV"='%s';"
		"update "VV_LIST" set "VV_NAME"='%s' where "VV_NAME"='%s';"
		"update "VV_LIST" set "VV_RD_VV"='%s' where "VV_RD_VV"='%s';",
		pTo, pFr, pTo, pFr, pTo, pFr );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );

	/* commit */
	DBCommit( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgVN_VV );

	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlock( pDlgVN_VV );
	return( -1 );
}

int
DBResizeVolume( void *pD, char *pVol, char *pSize )
{
	DB_t *pDB 	= (DB_t*)pD;	
	void *pDBMgr= pDB->db_pMgr;
	char	sql[512];
	r_tbl_t		*pTbl;
	int 	Ret;
	DlgCache_t	*pDlgVN_VV;

	LOG( neo_log, LogINF, ""VN_LIST"" );

	if( !DBisVVinVN_LIST( pDB, pVol ) ) {
		LOG( neo_log, LogERR, "pVol=%s", pVol );
		goto err0;
	}

	pDlgVN_VV	= DBLockW( pD, VN_VV_LIST );

	_DBLock( pDB );

	sprintf( sql,	
		"update "VN_LIST" set "VN_SIZE"=%s where "VN_VV"='%s';",
		pSize, pVol );

	Ret = SqlExecute( pDBMgr, sql );
	if( Ret < 0 )	goto err1;
	Ret = SqlResultFirstOpen( pDBMgr, &pTbl );
	if( Ret < 0 )	goto err1;
	do {
		SqlResultClose( pTbl );
	} while( !SqlResultNextOpen( pDBMgr, &pTbl ) && pTbl );

	/* commit */
	DBCommit( pDB );
	_DBUnlock( pDB );
	DBUnlockReturn( pDlgVN_VV );
	return( 0 );
err1:
	DBRollback( pDB );
	_DBUnlock( pDB );
	DBUnlock( pDlgVN_VV );
err0:
	return( -1 );
}
