/*******************************************************************************
 * 
 *  FCProbe.c --- Probe FileCache for permanent data strage
 * 
 *  Copyright (C) 2015-2016 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

#define	DB_MAN_SYSTEM

#include	<time.h>
#include	<ctype.h>
#include	"neo_db.h"
#include	"../svc/hash.h"
#include	"../dbif/TBL_RDB.h"


r_inf_t	inf;

int
DumpFd( r_man_t *md, GFd_t *pFd, GFd_t *pOrg )
{
	printf("%p e_Cnt=%d Name[%s] Dir[%s] Base[%s] Fd[%d]\n",
		pOrg, pFd->f_GE.e_Cnt, pFd->f_Name, pFd->f_DirName, pFd->f_BaseName, 
		pFd->f_Fd );
	return( 0 );
}

int
ReadFd( r_man_t *md, BlockCacheCtrl_t *pBCC, BlockCacheCtrl_t *pOrg )
{
	GFd_t	Fd, *pFd;

	printf("### FdCache Cnt=%d FdMax=%d Root[%s] ###\n", 
			pBCC->bc_FdCtrl.fc_Ctrl.ge_Cnt,
			pBCC->bc_FdCtrl.fc_Ctrl.ge_MaxCnt,
			pBCC->bc_FdCtrl.fc_Root );
			
	LIST_FOR_EACH_ENTRY( pFd, 
				&pBCC->bc_FdCtrl.fc_Ctrl.ge_Lnk, f_GE.e_Lnk,
								pBCC, pOrg ) {
		rdb_dump_mem( md, (char*)pFd, sizeof(Fd), (char*)&Fd );
		DumpFd( md, &Fd, pFd );

		pFd = &Fd;
	}
	printf("=== FdCache(END) ===\n");
	return( 0 );
}

int
DumpFdMeta( r_man_t *md, GFdMeta_t *pFd, GFdMeta_t *pOrg )
{
	printf("%p e_Cnt=%d Name[%s] Fd[%d] st_size[%zu]\n",
	pOrg, pFd->fm_GE.e_Cnt, pFd->fm_Name, pFd->fm_Fd, pFd->fm_Stat.st_size );
	return( 0 );
}
int
ReadMeta( r_man_t *md, BlockCacheCtrl_t *pBCC, BlockCacheCtrl_t *pOrg )
{
	GFdMeta_t	Fd, *pFd;

	printf("### FdMetaCache Cnt=%d FdMetaMax=%d Root[%s] ###\n", 
			pBCC->bc_FdCtrl.fc_Meta.ge_Cnt,
			pBCC->bc_FdCtrl.fc_Meta.ge_MaxCnt,
			pBCC->bc_FdCtrl.fc_Root );
			
	LIST_FOR_EACH_ENTRY( pFd, 
				&pBCC->bc_FdCtrl.fc_Meta.ge_Lnk, fm_GE.e_Lnk,
								pBCC, pOrg ) {
		rdb_dump_mem( md, (char*)pFd, sizeof(Fd), (char*)&Fd );
		DumpFdMeta( md, &Fd, pFd );
		pFd = &Fd;
	}
	printf("=== FdMetaCache(END) ===\n");
	return( 0 );
}

int
DumpBlock( r_man_t *md, 
		BlockCache_t *pB, BlockCache_t *pOrg, BlockCacheCtrl_t *pBCC )
{
	char		*pBuf;
	timespec_t	*pValidTime;
	uint64_t	*pCksum64;
	int	j, k;
	unsigned char	c;

	printf("%p e_Cnt=%d Name[%s] Off[%"PRIu64"] Len=%d Flags=0x%x \n", 
			pOrg, pB->b_GE.e_Cnt, pB->b_Key.k_Name, pB->b_Key.k_Offset, 
			pB->b_Len, pB->b_Flags );

	pBuf		= Malloc(BLOCK_CACHE_SIZE(pBCC));
	pValidTime	= (timespec_t*)
					Malloc(sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBCC));
	pCksum64	= (uint64_t*)Malloc(sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBCC));

	rdb_dump_mem( md, pB->b_pBuf, BLOCK_CACHE_SIZE(pBCC), pBuf );
	rdb_dump_mem( md, (char*)pB->b_pValid,
				sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBCC), (char*)pValidTime );
	rdb_dump_mem( md, (char*)pB->b_pCksum64,
					sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBCC), (char*)pCksum64 );

	for( j = 0; j < pBCC->bc_SegmentNum; j++ ) {
		printf("\tCksum64[%d]=%"PRIu64" ", j, pCksum64[j] );
		printf("[");
		for( k = 0; k < 32 && k < SEGMENT_CACHE_SIZE(pBCC); k++ ) {
			c =(pBuf[j*SEGMENT_CACHE_SIZE(pBCC)+k]&0xff);
			if( isprint(c))	printf("%c", c );
			else			printf(".");
		}
		printf("]");
		printf(" %s", ctime( &pValidTime[j].tv_sec ) );
	}
	Free( pBuf );
	Free( pValidTime );
	Free( pCksum64 );
	return( 0 );
}

int
ReadBlock( r_man_t *md, BlockCacheCtrl_t *pBCC, BlockCacheCtrl_t *pOrg )
{
	BlockCache_t	B, *pB;

	printf(
		"### BlockCache Cnt=%d Max=%d "
		"SegSize=%d SegNum=%d bCksum=%d UsecWB=%d ###\n", 
	pBCC->bc_Ctrl.ge_Cnt, pBCC->bc_Ctrl.ge_MaxCnt, 
	pBCC->bc_SegmentSize, pBCC->bc_SegmentNum, pBCC->bc_bCksum,
	pBCC->bc_UsecWB );

	LIST_FOR_EACH_ENTRY( pB, 
				&pBCC->bc_Ctrl.ge_Lnk, b_GE.e_Lnk, pBCC, pOrg ) {

		rdb_dump_mem( md, (char*)pB, sizeof(B), (char*)&B );
		DumpBlock( md, &B, pB, pBCC );

		pB = &B;
	}
	printf("=== BlockCache(END) ===\n");
	return( 0 );
}

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_res_dump_inf_t	res;
	r_inf_t		inf;
	BlockCacheCtrl_t	*pBCC, BCC;
	

DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
		printf("usage:FCProbe server {block|meta|fd}\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err;

	if( rdb_dump_lock( md ) < 0 )
		goto err0;

	if( rdb_dump_inf( md, &res ) < 0 )
		goto err1;
	inf = res.i_inf;

	pBCC	= (BlockCacheCtrl_t*)inf.i_svc_inf.f_pFileCache;

	if( rdb_dump_mem( md, (char*)pBCC, sizeof(BCC), (char*)&BCC ) < 0 ) {
		goto err1;
	}

	if( !strcmp("block", av[2]) ) {
		ReadBlock( md, &BCC, pBCC );
	} else if( !strcmp("meta", av[2] ) ) {
		ReadMeta( md, &BCC, pBCC );
	} else if( !strcmp("fd", av[2]) )  {
		ReadFd( md, &BCC, pBCC );
	} 
	rdb_dump_unlock( md );

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
    printf("err1: neo_errno=0x%x\n",neo_errno);
	rdb_dump_unlock( md );
err0:
	rdb_unlink( md );
err:
	return( -1 );
}

