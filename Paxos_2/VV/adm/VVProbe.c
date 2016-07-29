/******************************************************************************
*
*  VVProbe.c 	--- probe program of LVServer 
*
*  Copyright (C) 2014,2016 triTech Inc. All Rights Reserved.
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
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_


#define	_PAXOS_SESSION_SERVER_
#define	_PAXOS_SESSION_CLIENT_
#include	"VV.h"
#include	<stdio.h>
#include	<ctype.h>

VVCtrl_t	CNTRL, *pCNTRL;

int
VVLock( int Fd )
{
	PaxosSessionHead_t	Req, Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, VV_LOCK, 0, 0, sizeof(Req) );
	Ret = ReqAndRplByAny( Fd, (char*)&Req, sizeof(Req), 
							(char*)&Rpl, sizeof(Rpl) );
	if( Rpl.h_Error ) {
		errno = Rpl.h_Error;
		Ret = -1;
	} else {
		Ret = 0;
	}
	return( Ret );
}

int
VVUnlock( int Fd )
{
	PaxosSessionHead_t	Req, Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, VV_UNLOCK, 0, 0, sizeof(Req) );
	Ret = ReqAndRplByAny( Fd, (char*)&Req, sizeof(Req), 
							(char*)&Rpl, sizeof(Rpl) );
	if( Rpl.h_Error ) {
		errno = Rpl.h_Error;
		Ret = -1;
	} else {
		Ret = 0;
	}
	return( Ret );
}

int
VVReadCntrl( int Fd, VVCtrl_t* pCntrl, VVCtrl_t** ppCntrl )
{
	VVCtrl_req_t	Req;
	VVCtrl_rpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, VV_CNTRL, 0, 0, sizeof(Req) );
	Ret = ReqAndRplByAny( Fd, (char*)&Req, sizeof(Req), 
							(char*)&Rpl, sizeof(Rpl) );
	if( Rpl.c_Head.h_Error ) {
		errno = Rpl.c_Head.h_Error;
		Ret = -1;
	} else {
		*pCntrl		= Rpl.c_Ctrl;
		*ppCntrl	= Rpl.c_pCtrl;
		Ret = 0;
	}
	return( Ret );
}

int
VVReadMem( int Fd, void* pStart, int Size, void* pData )
{
	VVDump_req_t	Req;
	VVDump_rpl_t	*pRpl;
	int	len;
	int	Ret;

	SESSION_MSGINIT( &Req, VV_DUMP, 0, 0, sizeof(Req) );
	Req.d_pAddr	= pStart;
	Req.d_Len	= Size;
	pRpl = (VVDump_rpl_t*)Malloc( (len = sizeof(*pRpl) + Size) );
	Ret = ReqAndRplByAny( Fd, (char*)&Req, sizeof(Req), (char*)pRpl, len );
	if( pRpl->d_Head.h_Error ) {
		errno = pRpl->d_Head.h_Error;
		Ret = -1;
	} else {
		if(!pData) abort();
		memcpy( pData, (pRpl+1), Size );
		Ret = 0;
	}
	Free( pRpl );
	return( Ret );
}

int
VVDumpBlock( int i, int Fd, C_Block_t *pB, C_Block_t *pOrg, C_BCtrl_t *pBC )
{
	char		*pBuf;
	int	k;
	unsigned char	c;

	printf("%d(%p) e_Cnt=%d d_Name[%s] Flags=0x%x Cksum64=0x%"PRIx64"\n\t%s", 
		i, pOrg, pB->b_Dlg.d_GE.e_Cnt, pB->b_Dlg.d_Dlg.d_Name, 
		pB->b_Flags, pB->b_Cksum64, ctime(&pB->b_Valid.tv_sec) );

	pBuf		= Malloc(C_BLOCK_SIZE(pBC));

	VVReadMem( Fd, pB->b_pBuf, C_BLOCK_SIZE(pBC), pBuf );

	printf("[");
	for( k = 0; k < 32 && k < C_BLOCK_SIZE(pBC); k++ ) {
		c =(pBuf[k]&0xff);
		if( isprint(c))	printf("%c", c );
		else			printf(".");
	}
	printf("]\n");

	Free( pBuf );
	return( 0 );
}

int
ReadCache( int Fd )
{
	C_Block_t	B, *pB;
	int	i = 0;

	printf( "### ReadBlockCache Cnt=%d Max=%d "
		"BlockSize=%d bCksum=%d Usec=%d ###\n", 
	CNTRL.c_Cache.bc_Ctrl.dc_Ctrl.ge_Cnt, 
	CNTRL.c_Cache.bc_Ctrl.dc_Ctrl.ge_MaxCnt, 
	CNTRL.c_Cache.bc_BlockSize,
	CNTRL.c_Cache.bc_bCksum,
	CNTRL.c_Cache.bc_Usec );

	if( !CNTRL.c_Cache.bc_Ctrl.dc_Ctrl.ge_MaxCnt )	return( -1 );

	LIST_FOR_EACH_ENTRY( pB, 
			&CNTRL.c_Cache.bc_Ctrl.dc_Ctrl.ge_Lnk, 
				b_Dlg.d_GE.e_Lnk, &CNTRL, pCNTRL ) {

		VVReadMem( Fd, pB, sizeof(B), &B );

		VVDumpBlock( i++, Fd, &B, pB, &CNTRL.c_Cache );

		pB = &B;
	}
	printf("=== BlockCache(END) ===\n");
	return( 0 );
}

int
VVDumpDlg( int i, int Fd, DlgCache_t *pD, DlgCache_t *pOrg )
{
	printf("%d(%p) e_Cnt=%d d_Name[%s] Own=%d Ver=%d %s", 
		i, pOrg, pD->d_GE.e_Cnt, pD->d_Dlg.d_Name, 
		pD->d_Dlg.d_Own, pD->d_Dlg.d_Ver, ctime(&pD->d_Timespec.tv_sec));
	return( 0 );
}

int
VVDumpDlgCtrl( int Fd, DlgCacheCtrl_t *pDC, void *pRemote, void *pLocal )
{
	DlgCache_t	D, *pD;
	int	i = 0;

	LIST_FOR_EACH_ENTRY( pD, 
			&pDC->dc_Ctrl.ge_Lnk, 
				d_GE.e_Lnk, pLocal, pRemote ) {

		VVReadMem( Fd, pD, sizeof(D), &D );

		VVDumpDlg( i++, Fd, &D, pD  );

		pD = &D;
	}
	return( 0 );
}

int
ReadVLDlg( int Fd )
{
	printf( "### VLDlg Cnt=%d Max=%d ###\n",
			CNTRL.c_VlDlg.dc_Ctrl.ge_Cnt, 
			CNTRL.c_VlDlg.dc_Ctrl.ge_MaxCnt );

	if( !CNTRL.c_VlDlg.dc_Ctrl.ge_MaxCnt )	return( -1 );

	VVDumpDlgCtrl( Fd, &CNTRL.c_VlDlg, pCNTRL, &CNTRL  );	

	printf("=== VLDlg(END) ===\n");
	return( 0 );
}

int
ReadVVDlg( int Fd )
{
	printf( "### VVDlg Cnt=%d Max=%d ###\n",
			CNTRL.c_VVDlg.dc_Ctrl.ge_Cnt, 
			CNTRL.c_VVDlg.dc_Ctrl.ge_MaxCnt );

	if( !CNTRL.c_VVDlg.dc_Ctrl.ge_MaxCnt )	return( -1 );

	VVDumpDlgCtrl( Fd, &CNTRL.c_VVDlg, pCNTRL, &CNTRL );	

	printf("=== VVDlg(END) ===\n");
	return( 0 );
}

int
ReadVSDlg( int Fd )
{
	printf( "### VSDlg Cnt=%d Max=%d ###\n",
			CNTRL.c_VSDlg.dc_Ctrl.ge_Cnt, 
			CNTRL.c_VSDlg.dc_Ctrl.ge_MaxCnt );

	if( !CNTRL.c_VSDlg.dc_Ctrl.ge_MaxCnt )	return( -1 );

	VVDumpDlgCtrl( Fd, &CNTRL.c_VSDlg, pCNTRL, &CNTRL );	

	printf("=== VSDlg(END) ===\n");
	return( 0 );
}

int
PrintClientId( PaxosClientId_t* pId )
{
	printf("ClientId [%s-%d] Pid=%u-%d Seq=%d Reuse=%d Try=%d\n", 
		inet_ntoa( pId->i_Addr.sin_addr ), ntohs( pId->i_Addr.sin_port ),
		pId->i_Pid, pId->i_No, pId->i_Seq, pId->i_Reuse, pId->i_Try );
	return( 0 );
}

int
ReadRecall( int Fd )
{
	DlgCacheCtrl_t	DC, *pDC;
	Session_t	S, *pS;
	int	i = 0;

	printf( "### Recall ###\n" );

	if( !CNTRL.c_Recall.rc_pEvent )	return( -1 );

	pS	= CNTRL.c_Recall.rc_pEvent;
	VVReadMem( Fd, pS, sizeof(S), &S );
	PrintClientId( &S.s_Id );

	LIST_FOR_EACH_ENTRY( pDC, 
			&CNTRL.c_Recall.rc_LnkCtrl, 
				dc_Lnk, &CNTRL, pCNTRL ) {

		VVReadMem( Fd, pDC, sizeof(DC), &DC );

		printf("%d ==== START\n", i );

		VVDumpDlgCtrl( Fd, &DC, pDC, &DC );	

		printf("%d ==== END\n", i++ );

		pDC = &DC;
	}
	printf("=== Recall(END) ===\n");
	return( 0 );
}

int
VVDumpMC( int i, int Fd, MC_t *pM, MC_t *pOrg )
{
	printf("%d(%p) Name[%s]\n", i, pOrg, pM->mc_Cell.c_Name );
	return( 0 );
}

int
ReadMC( int Fd )
{
	MC_t	M, *pM;
	int	i = 0;

	printf( "### MC Cnt=%d Max=%d ###\n",
			CNTRL.c_MC.mcc_Ctrl.ge_Cnt, CNTRL.c_MC.mcc_Ctrl.ge_MaxCnt );

	if( !CNTRL.c_MC.mcc_Ctrl.ge_MaxCnt )	return( -1 );

	LIST_FOR_EACH_ENTRY( pM, 
			&CNTRL.c_MC.mcc_Ctrl.ge_Lnk, mc_GE.e_Lnk, &CNTRL, pCNTRL ) {

		VVReadMem( Fd, pM, sizeof(M), &M );

		VVDumpMC( i++, Fd, &M, pM );

		pM = &M;
	}
	printf("=== MC(END) ===\n");
	return( 0 );
}

#ifdef	ZZZ
#else
int
BareCmd( int Fd, int Cmd )
{
	VVActive_req_t	Req;
	VVActive_rpl_t	Rpl;
	int	Ret;
	struct timeval Timeout = { 1, 0};

	SESSION_MSGINIT( &Req, Cmd, 0, 0, sizeof(Req) );

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
	printf( "%m\n" );
	return( -1 );
}

int
RegCmd( int Fd, int Cmd, char *pRasCell )
{
	VVRasReg_req_t	Req;
	VVRasReg_rpl_t	Rpl;
	int	Ret;
	struct timeval Timeout = { 1, 0};

	memset( &Req, 0, sizeof(Req) );
	SESSION_MSGINIT( &Req.r_Head, Cmd, 0, 0, sizeof(Req) );
	strncpy( Req.r_RasCell, pRasCell, sizeof(Req.r_RasCell) );

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
	printf( "%m\n" );
	return( -1 );
}
#endif

int
usage()
{
	printf("VVProbe name id {log|reg ras_cell|unreg ras_cell|active|stop}\n");
	printf("VVProbe name id {cache|VL|VV|VS|RC|MC}\n");
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	Ret;
	int	Fd;
	int	Id;
	char	*pName;
	char	*pCmd;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	pName	= av[1];
	Id		= atoi(av[2]);
	pCmd	= av[3];

	Fd	= ConnectAdmPort( pName, Id );
	if( Fd < 0 ) {
		printf("Can't connect to [%s_%d].\n", pName, Id );
		exit( -1 );
	}
	if( !strcmp("log", pCmd ) ) {
		Ret = BareCmd( Fd, VV_LOG_DUMP );
		return( Ret );
	}
	if( !strcmp("active", pCmd ) ) {
		Ret = BareCmd( Fd, VV_ACTIVE );
		return( Ret );
	}
	if( !strcmp("stop", pCmd ) ) {
		Ret = BareCmd( Fd, VV_STOP );
		return( Ret );
	}
	if( !strcmp("reg", pCmd ) ) {
		if( ac < 5 ) {
			usage();
			exit( -1 );
		}
		Ret = RegCmd( Fd, VV_RAS_REGIST, av[4] );
		return( Ret );
	}
	if( !strcmp("unreg", pCmd ) ) {
		if( ac < 5 ) {
			usage();
			exit( -1 );
		}
		Ret = RegCmd( Fd, VV_RAS_UNREGIST, av[4] );
		return( Ret );
	}

	Ret = VVLock( Fd );
	if( Ret ) {
		printf("Lock ERROR [%s_%d].\n", pName, Id );
		exit( -1 );
	}
	Ret = VVReadCntrl( Fd, &CNTRL, &pCNTRL );

	if( !strcmp( "cache", pCmd ) ) {
		Ret = ReadCache( Fd );
	} else if( !strcmp("VL", pCmd ) ) {
		Ret = ReadVLDlg( Fd );
	} else if( !strcmp("VV", pCmd ) ) {
		Ret = ReadVVDlg( Fd );
	} else if( !strcmp("VS", pCmd ) ) {
		Ret = ReadVSDlg( Fd );
	} else if( !strcmp("RC", pCmd ) ) {
		Ret = ReadRecall( Fd );
	} else if( !strcmp("MC", pCmd ) ) {
		Ret = ReadMC( Fd );
	} else {
		usage();
	}

	VVUnlock( Fd );

	return( Ret );
}
