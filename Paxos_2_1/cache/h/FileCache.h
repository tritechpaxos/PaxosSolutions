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

#ifndef	_FILE_CACHE_
#define	_FILE_CACHE_

#include	"NWGadget.h"
#include	<dirent.h>

//#define	FD_MAX	20

#ifndef	PATH_NAME_MAX
#define	PATH_NAME_MAX	1024
#endif
#ifndef	FILE_NAME_MAX
#define	FILE_NAME_MAX	256
#endif
#define	BLOCK_CACHE_NAME_MAX	PATH_NAME_MAX
//#define	BLOCK_CACHE_MAX			100

//#define	SEGMENT_CACHE_SIZE	(1024*4)			// 4KiB
//#define	SEGMENT_CACHE_NUM	16					// 16
//#define	BLOCK_CACHE_SIZE	(SEGMENT_CACHE_SIZE*SEGMENT_CACHE_NUM)	// 64KiB
#define	FILE_BLOCKS		64	// 4MiB	
#define	FILE_ENTRIES	1024

#define	BLOCK_CKSUM_FILE_SUFFIX	".CKSUM64"

#define	FILE_CACHE_INDEX( pFC, offset, dir0, dir1, base ) \
	({off_t off1; \
		off1 = offset/(pFC)->fc_FileSize; \
		base = off1%(pFC)->fc_Entries; off1 /= (pFC)->fc_Entries; \
		dir1 = off1%(pFC)->fc_Entries; dir0 = off1/(pFC)->fc_Entries;})

#define	FILE_CACHE_PATH( pFC, name, offset, Path ) \
	({off_t dir0, dir1, base; \
		FILE_CACHE_INDEX( pFC, offset, dir0, dir1, base ); \
		snprintf( Path, sizeof(Path), "%s/%jd/%jd/%jd", \
			name, dir0, dir1, base );})

#define	FILE_CACHE_PATH_REAL( pFC, name, offset, Path, PathReal ) \
	({off_t dir0, dir1, base; \
		FILE_CACHE_INDEX( pFC, offset, dir0, dir1, base ); \
		snprintf(Path, sizeof(Path), "%s/%jd/%jd/%jd", \
			name, dir0, dir1, base ); \
		snprintf(PathReal,sizeof(PathReal),"%s/%s",(pFC)->fc_Root,Path);})

typedef struct GFdCtrl {
	GElmCtrl_t	fc_Ctrl;
	char		fc_Root[PATH_MAX];
	struct Log*	fc_pLog;
	GElmCtrl_t	fc_Meta;
	uint64_t	fc_FileSize;
	int			fc_Entries;
} GFdCtrl_t;

typedef	struct GFdMeta {
	GElm_t		fm_GE;
	int			fm_Fd;
	char		fm_Name[PATH_MAX];
	char		fm_Path[PATH_MAX];
	struct stat	fm_Stat;
} GFdMeta_t;

typedef struct GFd {
	GElm_t		f_GE;
	char		f_Name[PATH_MAX];
	char		f_Path[PATH_MAX];
	off_t		f_Offset;
	char		f_DirName[PATH_MAX];
	char		f_BaseName[FILENAME_MAX];
	int			f_Fd;
	int			f_FdCksum;
	int			f_errno;
	GFdCtrl_t	*f_pFC;	
} GFd_t;

typedef	struct BlockCacheCtrl {
	GElmCtrl_t	bc_Ctrl;
	int			bc_SegmentSize;
	int			bc_SegmentNum;
	list_t		bc_DirtyLnk;
	bool_t		bc_bCksum;
	int			bc_UsecWB;	// WriteBack, Check cksum delay time
	pthread_t	bc_Thread;
	GFdCtrl_t	bc_FdCtrl;
	struct Log*	bc_pLog;
} BlockCacheCtrl_t;

#define	SEGMENT_CACHE_SIZE( pBC )		((pBC)->bc_SegmentSize)
#define	SEGMENT_CACHE_NUM( pBC )		((pBC)->bc_SegmentNum)
#define	BLOCK_CACHE_SIZE( pBC ) \
		(SEGMENT_CACHE_SIZE( pBC )*SEGMENT_CACHE_NUM( pBC ))
#define	MAX_SIZE( pBC ) \
	(((int64_t)BLOCK_CACHE_SIZE(pBC))*((pBC)->bc_Ctrl.ge_MaxCnt-1))

typedef struct BlockCacheKey {
	char		k_Name[BLOCK_CACHE_NAME_MAX];
	off_t		k_Offset;		// BLOCK_CACHE_BOUDARY
} BlockCacheKey_t;

typedef	struct BlockCache {
	GElm_t			b_GE;
	list_t			b_DirtyLnk;
	timespec_t		b_Timeout;
	BlockCacheKey_t	b_Key;
#define	BLOCK_CACHE_ACTIVE	0x1
#define	BLOCK_CACHE_DIRTY	0x2
	int				b_Flags;
	char*			b_pBuf;			// [BLOCK_CACHE_SIZE]
	timespec_t*		b_pValid;		// [SEGMENT_CACHE_NUM];
	uint64_t*		b_pCksum64;		// [SEGMENT_CACHE_NUM];
	int				b_Len;			// effective size
} BlockCache_t;

extern	int	GFdCacheInit(GFdCtrl_t*, int FdMax, char *pRoot, 
						uint64_t FileSize, int Entries, struct Log *pLog);
extern	GFd_t* GFdCacheCreate( GFdCtrl_t*, char *pName );
extern	GFd_t* GFdCacheGet( GFdCtrl_t*, BlockCacheKey_t *pKey, bool_t bCreate );
extern	int	GFdCachePut( GFdCtrl_t*, GFd_t *pFd );

extern	int GFdCacheDelete( GFdCtrl_t *pFdCtrl, char *pName );
extern	int FdRead( GFd_t *pFd, off_t Off, void *pData, size_t Len );
extern	int FdWrite( GFd_t *pFd, off_t Off, void *pData, size_t Len );
extern	int FdReadCksum( GFd_t *pFd, off_t Off, void *pData, size_t Len );
extern	int FdWriteCksum( GFd_t *pFd, off_t Off, void *pData, size_t Len );

extern	int GFdCachePurge( GFdCtrl_t *pFC, char *pName );

/*
 *	FileCacheFlush			save block-cache
 *	FileCacheFlushAll		save all block-caches
 *	FileCachePurge			save all block-caches & discard all block-caches 
 *	FileCacheDelete			delete file & discard related block-caches
 */
extern	int	FileCacheInit( BlockCacheCtrl_t *pBC, int FdMax, char *pRoot,
			int BlockMax, int SegSize, int SegNum, int UsecWB, 
			bool_t bCksum, struct Log *pLog );

extern	int	FileCacheCreate( BlockCacheCtrl_t *pBC, char *pName );
extern	int	FileCacheWrite( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData );
extern	int	FileCacheRead( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData );
extern	int	FileCacheFlush( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size );
extern	int	FileCacheFlushAll( BlockCacheCtrl_t *pBC );
extern	int	FileCachePurge( BlockCacheCtrl_t *pBC, char *pName );
extern	int	FileCachePurgeAll( BlockCacheCtrl_t *pBC );
extern	int	FileCacheDelete( BlockCacheCtrl_t *pBC, char *pName );

extern	int	FileCacheSeek(BlockCacheCtrl_t *pBC, char *pName, off_t Off );
extern	int	FileCacheTruncate(BlockCacheCtrl_t *pBC, char *pName, off_t Off);
extern	int	FileCacheAppend( BlockCacheCtrl_t *pBC, 
				char *pName, size_t Size, char *pData );
extern	int FileCacheStat( BlockCacheCtrl_t *pBC, char *pName, 
							struct stat *pStat);
extern	int FileCacheReadDir( BlockCacheCtrl_t *pBC, char *pPath, 
								struct dirent *pDirEnt, int Ind, int Num );
extern	int FileCacheRmDir( BlockCacheCtrl_t *pBC, char *pPath );

#endif	//	_FILE_CACHE_
