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

/*
 *	Use only FileCacheXxx API 
 */
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>

#include	"FileCache.h"

BlockCacheCtrl_t BC;

#define	SEG_SIZE	16
#define	SEG_NUM		2
#define	BLOCK_SIZE	SEG_SIZE*SEG_NUM
#define	BLOCK_MAX	2

#define	FD_MAX		5
#define	FILE_SIZE_MAX	(BLOCK_SIZE*10)

char	wbuf[BLOCK_SIZE*BLOCK_MAX];
char	rbuf[BLOCK_SIZE*BLOCK_MAX];

char	wbuf2[BLOCK_SIZE*BLOCK_MAX*2];
char	rbuf2[BLOCK_SIZE*BLOCK_MAX*2];

void
SetBuf( char *pBuf, int Start, int Len )
{
	int	i;
	char	c;

	for( i = 0; i < Len; i++ ) {
		c = i % 10; c |= 0x30;
		pBuf[i] = c;
	}
}

struct Log *pLog;

#define	SEG_ZERO	"SEG_ZERO"
#define	SEG_0		"SEG_0"

void
FdMetaDump( GFdCtrl_t *pFC )
{
	GFdMeta_t	*pF;
	int				i = 0;

	printf("### Dump FdMeta (HashCount=%d)###\n",
					HashCount(&pFC->fc_Meta.ge_Hash) );
	GElmCtrlLock( &pFC->fc_Meta );
	list_for_each_entry( pF, &pFC->fc_Meta.ge_Lnk, fm_GE.e_Lnk ) {

		printf("%d [%s]Fd=%d Size=%jd\n", 
				i++, pF->fm_Name, pF->fm_Fd, pF->fm_Stat.st_size );
	}
	GElmCtrlUnlock( &pFC->fc_Meta );
	printf("=== Dump FdMeta ===\n");
}

void
FdCacheDump( GFdCtrl_t *pFC )
{
	GFd_t	*pF;
	int				i = 0;

	printf("### Dump FdCache (HashCount=%d)###\n",
					HashCount(&pFC->fc_Ctrl.ge_Hash) );
	GElmCtrlLock( &pFC->fc_Ctrl );
	list_for_each_entry( pF, &pFC->fc_Ctrl.ge_Lnk, f_GE.e_Lnk ) {
		printf("%d [%s](%s:%s)Fd=%d FdCksum=%d\n", 
			i++, pF->f_Name, pF->f_DirName, pF->f_BaseName,
			pF->f_Fd, pF->f_FdCksum );
	}
	GElmCtrlUnlock( &pFC->fc_Ctrl );
	printf("=== Dump FdCache ===\n");
}

void
BlockCacheDump( BlockCacheCtrl_t *pBC )
{
	BlockCache_t	*pB;
	int	i = 0;
	int	j;
	int	k;
	unsigned char	c;

	printf("### Dump BlockCache (HashCount=%d)\n"
		"SEG_SIZE=%d SEG_NUM=%d bCksum=%d Lag=%d usec"
		" MAX_SIZE=%"PRIi64" ###\n",
			HashCount(&pBC->bc_Ctrl.ge_Hash),
			pBC->bc_SegmentSize, pBC->bc_SegmentNum, 
			pBC->bc_bCksum, pBC->bc_UsecWB, MAX_SIZE(pBC) );

	GElmCtrlLock( &pBC->bc_Ctrl );
	list_for_each_entry( pB, &pBC->bc_Ctrl.ge_Lnk, b_GE.e_Lnk ) {
		printf("%d %p [%s] %"PRIu64" Len=%d Dirty=%d Flags=0x%x \n", 
			i++, pB, pB->b_Key.k_Name, pB->b_Key.k_Offset, pB->b_Len,
			!list_empty(&pB->b_DirtyLnk), pB->b_Flags );
		for( j = 0; j < pBC->bc_SegmentNum; j++ ) {
			printf("\tCksum64[%d]=%"PRIu64" ", j, pB->b_pCksum64[j] );
			printf("[");
			for( k = 0; k < SEGMENT_CACHE_SIZE(pBC); k++ ) {
				c =(pB->b_pBuf[j*SEGMENT_CACHE_SIZE(pBC)+k]&0xff);
				if( isprint(c))	printf("%c", c );
				else			printf(".");
			}
			printf("]\n");
		}
	}
	GElmCtrlUnlock( &pBC->bc_Ctrl );
	printf("=== Dump BlockCache ===\n");
}

int
IsDirty( BlockCacheCtrl_t *pBC )
{
	int	Cnt = 0;
	BlockCache_t	*pB;

	GElmCtrlLock( &pBC->bc_Ctrl );
	list_for_each_entry( pB, &pBC->bc_DirtyLnk, b_DirtyLnk ) {
		Cnt++;
	}
	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Cnt );
}

int
main( int ac, char *av[] )
{
	int	FdMax = FD_MAX, BlockMax = BLOCK_MAX, SegSize=SEG_SIZE, SegNum=SEG_NUM, 
		MsecWB = 1000*10;
	BlockCache_t	*pB;
	int	Ret;
	struct stat	Stat;
	int	i;

	pLog = LogCreate( 0, Malloc, Free, 0, stdout, 5 );
//pLog = NULL;

#define	ROOT	"ROOT"

	system( "mkdir -p "ROOT );
	FileCacheInit( &BC, FdMax, ROOT, BlockMax, 
			SegSize, SegNum, MsecWB*1000, 1, pLog  );

	printf(
"/n### 1. Read not-exist file "SEG_ZERO" and create/write/read/delete ###\n");

	Ret = FileCacheRead( &BC, SEG_ZERO, 0, sizeof(rbuf), rbuf );
	if( Ret >= 0 ) {
		goto err;
	} 
	printf("=== FileCacheRead:["SEG_ZERO"] Ret=%d === OK\n", Ret );

	wbuf[0]	= 'A';
	Ret = FileCacheWrite( &BC, SEG_ZERO, 0, SEG_SIZE, wbuf );
	if( Ret >= 0 ) {
		goto err;
	} 
	Ret = FileCacheCreate( &BC, SEG_ZERO );

	Ret = FileCacheWrite( &BC, SEG_ZERO, 0, SEG_SIZE, wbuf );

	Ret = FileCacheRead( &BC, SEG_ZERO, 0, sizeof(rbuf), rbuf );
	if( memcmp( wbuf, rbuf, sizeof(rbuf) ) ) {
		printf("=== FileCacheWrite/Read:["SEG_ZERO"(A...0...)] === NG\n");
		goto err;
	} 
	printf("=== FileCacheWrite/Read:["SEG_ZERO"(A...0...)] On Cache === OK\n");

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	FileCacheDelete( &BC, SEG_ZERO );

	GElmCtrlLock( &BC.bc_Ctrl );
	list_for_each_entry( pB, &BC.bc_Ctrl.ge_Lnk, b_GE.e_Lnk ) {
		if( !strncmp(SEG_ZERO, pB->b_Key.k_Name, sizeof(pB->b_Key.k_Name))) {
			printf("=== FileCacheDelete:["SEG_ZERO"] === NG\n");
			goto err;
		}

	}
	GElmCtrlUnlock( &BC.bc_Ctrl );
	printf("=== FileCacheDelete:["SEG_ZERO"] === OK\n");

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf(
"/n### 2. "SEG_0" create/write/read/flush/delete ###\n");

	wbuf[0]	= 'B';
	Ret = FileCacheCreate( &BC, SEG_0 );
	Ret = FileCacheWrite( &BC, SEG_0, 0, SEG_SIZE, wbuf );
	Ret = FileCacheRead( &BC, SEG_0, 0, SEG_SIZE, rbuf );
	if( memcmp( wbuf, rbuf, sizeof(rbuf) ) ) {
		printf("=== FileCacheWrite/Read:["SEG_0"(B...0...)] === NG\n");
		goto err;
	} 
	printf("=== FileCacheWrite/Read:["SEG_0"(B...0...)] === OK\n");

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCacheFlush( &BC, SEG_0, 1, SEG_SIZE - 1 );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = stat( ROOT"/"SEG_0, &Stat );
	if( Ret )	{printf( SEG_0" --- Not exists\n");goto err;}
	else		{printf( SEG_0" --- Exists\n");}

	printf(
"/n### 3. Write exist file "SEG_0" and purge ###\n");

	wbuf[0]	= 'C';
	Ret = FileCacheWrite( &BC, SEG_0, 0, SEG_SIZE, wbuf );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCachePurge( &BC, SEG_0 );
	Ret = stat( ROOT"/"SEG_0, &Stat );
	if( Ret )	{printf( SEG_0" --- Not exists\n");goto err;}
	else		{printf( SEG_0" --- Exists\n");}

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf(
"/n### 4. Read-Purged file "SEG_0" ###\n");

	Ret = FileCacheRead( &BC, SEG_0, 0, SEG_SIZE, rbuf );
	if( rbuf[0] != wbuf[0] ) {
		printf(SEG_0" Error\n" ); goto err;
	}
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf(
"/n### 5. Write "SEG_0" and wait WriteBack and delete ###\n");

	wbuf[0]	= 'D';
	Ret = FileCacheWrite( &BC, SEG_0, 0, SEG_SIZE, wbuf );
	wbuf[0]	= 'E';
	Ret = FileCacheWrite( &BC, SEG_0, BLOCK_SIZE, SEG_SIZE, wbuf );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf("sleep(%d)\n", MsecWB/1000*2 );
	sleep( MsecWB/1000*2 );
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );
	if( IsDirty( &BC ) ) {
		printf("No WB\n");
		goto err;
	}
	Ret = FileCacheDelete( &BC, SEG_0 );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf(
"/n### 6. create/append "SEG_0" ###\n");

	Ret = FileCacheCreate( &BC, SEG_0 );

	wbuf[0]	= 'F';
	Ret = FileCacheAppend( &BC, SEG_0, 1, wbuf );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	wbuf[1]	= 'G';
	Ret = FileCacheAppend( &BC, SEG_0, 1, &wbuf[1] );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCacheRead( &BC, SEG_0, 0, 2, rbuf );
	if( wbuf[0] != rbuf[0]
		|| wbuf[1] != rbuf[1] ) {
		printf(SEG_0 " Error\n" ); goto err;
	} 
	FileCacheStat( &BC, SEG_0, &Stat );
	if( Stat.st_size != 2 ) {
		printf(SEG_0 "Append \n" ); goto err;
	}

	printf(
"/n### 7. write/truncate/delete "SEG_0" ###\n");

	Ret = FileCacheWrite( &BC, SEG_0, 0, SEG_SIZE, wbuf );

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCacheTruncate( &BC, SEG_0, SEG_SIZE/2 );
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	FileCacheStat( &BC, SEG_0, &Stat );
	if( Stat.st_size != SEG_SIZE/2 ) {
		printf(SEG_0" Truncate-1 \n" ); goto err;
	}
	printf("sleep(%d)\n", MsecWB/1000*2 );
	sleep( MsecWB/1000*2 );

	FileCacheStat( &BC, SEG_0, &Stat );
	if( Stat.st_size != SEG_SIZE/2 ) {
		printf(SEG_0" Truncate-2 \n" ); goto err;
	}

	FileCacheDelete( &BC, SEG_0 );

	printf(
"/n### 8. write long data from random off and read "SEG_0" ###\n");

	long r;

	srandom( time(0) );
	r = random();
	r %= FILE_SIZE_MAX;

	memset( wbuf, 'A', sizeof(wbuf) );

	Ret = FileCacheCreate( &BC, SEG_0 );
	if( Ret < 0 ) {
		perror("FileCacheCreate");
		goto err;
	}
	Ret = FileCacheWrite( &BC, SEG_0, r, 100/*sizeof(wbuf)*/, wbuf );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	printf("off=%ld,len=%ld,c=%c\n", r, sizeof(wbuf), 'A');
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	printf(
"/n### 9. write long data over cache size "SEG_0" ###\n");
	SetBuf( wbuf2, 0, sizeof(wbuf2) );

	Ret = FileCacheWrite( &BC, SEG_0, 0, sizeof(wbuf2), wbuf2 );
	if( Ret != sizeof(wbuf2) )	goto err;

	Ret = FileCacheRead( &BC, SEG_0, 0, sizeof(rbuf2), wbuf2 );
	if( Ret != sizeof(rbuf2) )	goto err;
	if( !memcmp( wbuf2, rbuf2, Ret ) )	goto err;

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	FileCacheDelete( &BC, SEG_0 );

	printf(
"/n### 10. write two data separately and truncate "SEG_0" ###\n");

	char Path[PATH_MAX], PathReal[PATH_MAX];

	SetBuf( wbuf2, 0, sizeof(wbuf2) );
	wbuf2[0] = 'H';

	Ret = FileCacheCreate( &BC, SEG_0 );
	if( Ret < 0 ) {
		perror("FileCacheCreate");
		goto err;
	}
	Ret = FileCacheWrite( &BC, SEG_0, 0, 100, wbuf2 );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	Ret = FileCacheWrite( &BC, SEG_0, BC.bc_FdCtrl.fc_FileSize, 100, wbuf2 );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	sleep( 20 );

	FILE_CACHE_PATH_REAL( &BC.bc_FdCtrl, SEG_0, 0, Path, PathReal );
	Ret = stat( PathReal, &Stat );
	if( Ret )	goto err;

	FILE_CACHE_PATH_REAL( &BC.bc_FdCtrl, SEG_0, 
				BC.bc_FdCtrl.fc_FileSize,  Path, PathReal );
	Ret = stat( PathReal, &Stat );
	if( Ret )	goto err;

	Ret = FileCacheTruncate( &BC, SEG_0, BC.bc_FdCtrl.fc_FileSize );
	Ret = stat( PathReal, &Stat );
	if( !Ret )	goto err;

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	FileCacheDelete( &BC, SEG_0 );

	printf(
"/n### 11. partial write for Cksum64 ###\n");

	Ret = FileCacheCreate( &BC, SEG_0 );
	if( Ret < 0 ) {
		perror("FileCacheCreate");
		goto err;
	}
	Ret = FileCacheWrite( &BC, SEG_0, 0, 100, wbuf );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	Ret = FileCacheWrite( &BC, SEG_0, 1, 1, wbuf2 );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	printf("sleep 20\n");sleep( 20 );
	Ret = FileCacheRead( &BC, SEG_0, 1, 1, rbuf );
	if( Ret < 0 ) {
		perror("FileCacheRead");
		goto err;
	}

	Ret = FileCacheWrite( &BC, SEG_0, 6, SEG_SIZE+1, wbuf2 );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	printf("sleep 20\n");sleep( 20 );
	Ret = FileCacheRead( &BC, SEG_0, 1, 1, rbuf );
	if( Ret < 0 ) {
		perror("FileCacheRead");
		goto err;
	}

	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	FileCacheDelete( &BC, SEG_0 );

	printf(
"/n### 12. file name included blank ###\n");

#define	BLANK_FILE	"A B"

	Ret = FileCacheCreate( &BC, BLANK_FILE);
	if( Ret < 0 ) {
		perror("FileCacheCreate");
		goto err;
	}
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCacheDelete( &BC, BLANK_FILE );
	if( Ret < 0 ) {
		perror("FileCacheDelete");
		goto err;
	}

	printf(
"/n### 13. FileCacheReadDir ###\n");

#define	A	"A"
#define	D_B	"D/B"
	struct dirent	Dir[10];

	Ret = FileCacheCreate( &BC, A );
	if( Ret < 0 ) {
		perror("FileCacheCreate:A");
		goto err;
	}
	Ret = FileCacheCreate( &BC, D_B );
	if( Ret < 0 ) {
		perror("FileCacheCreate:D_B");
		goto err;
	}

	Ret = FileCacheWrite( &BC, A, 0, 100, wbuf );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	Ret = FileCacheWrite( &BC, D_B, 1, 1, wbuf2 );
	if( Ret < 0 ) {
		perror("FileCacheWrite");
		goto err;
	}
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );

	Ret = FileCacheReadDir( &BC, ".", Dir, 0, 10 );
	if( Ret < 0 ) {
		perror("FileCacheReadDir:[.]");
		goto err;
	}
	for( i = 0; i < Ret; i++ ) {
		printf( "%d.[%s]\n", i, Dir[i].d_name );
	}
	Ret = FileCacheReadDir( &BC, A, Dir, 0, 10 );
	if( Ret >= 0 ) {
		perror("FileCacheReadDir:["A"]");
		goto err;
	}
	Ret = FileCacheReadDir( &BC, "D", Dir, 0, 10 );
	if( Ret < 0 ) {
		perror("FileCacheReadDir:[D]");
		goto err;
	}
	for( i = 0; i < Ret; i++ ) {
		printf( "%d.[%s]\n", i, Dir[i].d_name );
	}
	Ret = FileCacheDelete( &BC, A );
	if( Ret < 0 ) {
		perror("FileCacheDelete: A");
		goto err;
	}
	Ret = FileCacheRmDir( &BC, "D" );
	if( Ret >= 0 ) {
		perror("FileRmDir-1: D");
		goto err;
	}
	Ret = FileCacheDelete( &BC, D_B );
	if( Ret < 0 ) {
		perror("FileCacheDelete: D_B");
		goto err;
	}
	Ret = FileCacheRmDir( &BC, "D" );
	if( Ret < 0 ) {
		perror("FileRmDir-2: D");
		goto err;
	}

	printf("### SUCCESS ###\n");
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );
	return( 0 );
err:
	printf("### FAIL ###\n");
	BlockCacheDump( &BC );
	FdCacheDump( &BC.bc_FdCtrl );
	FdMetaDump( &BC.bc_FdCtrl );
	return( -1 );
}
