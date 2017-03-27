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

#include	"FileCache.h"
#include	<ctype.h>
#include	<libgen.h>

#define	GE_DELETE		1
#define	GE_TRUNCATE		2	
#define	GE_FLUSH		3
#define	GE_PURGE		4
#define	GE_FLUSH_ALL	5
#define	GE_PURGE_ALL	6

static	int	ConcatPath( char *pRealPath, size_t size, 
								char *pTarget, char *pPath );
static	bool_t IsMeta( char *pPath, char *pFile );

typedef struct {
	int			l_Cmd;
	char		l_Name[PATH_MAX];
	off_t		l_Offset;
} FdLoopArg_t;

GElm_t*
_FdMetaAlloc( GElmCtrl_t *pGC )
{
//	GFdCtrl_t	*pFC = container_of( pGC, GFdCtrl_t, fc_Meta );
	GFdMeta_t	*pFdMeta;

	pFdMeta	= Malloc( sizeof(*pFdMeta ) );

	ASSERT( pFdMeta != NULL );

	memset( pFdMeta, 0, sizeof(*pFdMeta) );

	return( (void*)pFdMeta );
}

int
_FdMetaFree( GElmCtrl_t *pGC, GElm_t *pElm )
{
	GFdMeta_t	*pFd = container_of( pElm, GFdMeta_t, fm_GE );

	ASSERT( list_empty( &pElm->e_Lnk ) );
	Free( pFd );

	return( 0 );
}

int
_FdMetaSet(GElmCtrl_t *pGC, GElm_t *pElm, void *pKey, void **ppKey, 
						void *pArg, bool_t bCreate)
{
	GFdMeta_t	*pFdMeta = container_of( pElm, GFdMeta_t, fm_GE );
	GFdCtrl_t	*pFC = container_of( pGC, GFdCtrl_t, fc_Meta );
	char	Path[PATH_MAX];
	char	DirName[PATH_MAX];
	char	Buff[PATH_MAX + 10];
	int	Ret;

	strncpy( pFdMeta->fm_Name, (char*)pKey, sizeof(pFdMeta->fm_Name) );

	snprintf( Path, sizeof(Path), "%s/%s/Meta", pFC->fc_Root, (char*)pKey );
	strncpy( pFdMeta->fm_Path, Path, sizeof(pFdMeta->fm_Path) );
	pFdMeta->fm_Fd	= open( Path, O_RDWR, 0666 );
	if( pFdMeta->fm_Fd >= 0 ) {

		Ret = read( pFdMeta->fm_Fd, 
					&pFdMeta->fm_Stat, sizeof(pFdMeta->fm_Stat) );
		if( Ret < 0 )	goto err;

	} else {
		 if( bCreate ) {
			strncpy( Buff, Path, sizeof(Buff) );
			strncpy( DirName, dirname(Buff), sizeof(DirName) );
			sprintf( Buff, "mkdir -p \"%s\"", DirName );
			Ret = system( Buff );
			if( Ret < 0 ) {
				LOG( pFC->fc_pLog, LogERR, "Buff[%s] Ret=%d", Buff, Ret );
			}
			pFdMeta->fm_Fd	= open( Path, O_CREAT|O_RDWR, 0666 );
			fstat( pFdMeta->fm_Fd, &pFdMeta->fm_Stat );
			pFdMeta->fm_Stat.st_mode	|= S_IFREG;
		} else {
			errno = ENOENT;
			goto err;
		}
	}
	*ppKey	= pFdMeta->fm_Name;

	LOG( pFC->fc_pLog, LogINF, "OK:open[%s]", Path );
	return( 0 );
err:
	LOG( pFC->fc_pLog, LogERR, "ERROR:open[%s] bCreate=%d", Path, bCreate );
	return( -1 );
}

int
_FdMetaFlush( GFdMeta_t *pFdMeta )
{
	int	Ret;

	lseek( pFdMeta->fm_Fd, 0, 0 ); 
	Ret = write( pFdMeta->fm_Fd, 
			&pFdMeta->fm_Stat, sizeof(pFdMeta->fm_Stat));
	return( Ret );
}

int
_FdMetaUnset( GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave )
{
	GFdMeta_t	*pFdMeta = container_of( pElm, GFdMeta_t, fm_GE );
	int	Ret;

	if( bSave ) {
ASSERT( pFdMeta->fm_Fd >= 0 );
		Ret = _FdMetaFlush( pFdMeta );
		if( Ret < 0 )	goto err;
	}
	close( pFdMeta->fm_Fd );
	pFdMeta->fm_Fd	= -1;

err:
	if( ppKey )	*ppKey	= pFdMeta->fm_Name;
	return( 0 );
}

int
_FdMetaLoop( GElmCtrl_t *pGE, GElm_t *pElm, void *pVoid )
{
	int	Ret = 0;
	FdLoopArg_t	*pArg = (FdLoopArg_t*)pVoid;
	GFdMeta_t		*pFdMeta = container_of( pElm, GFdMeta_t, fm_GE );

	switch( pArg->l_Cmd ) {
		case GE_DELETE:
			if( !strcmp( pFdMeta->fm_Name, pArg->l_Name) ) {
				_GElmUnset( pGE, pElm, FALSE );	// remove Hash
				_GElmFree( pGE, pElm );			// Free
			}
			break;
		case GE_PURGE:
			if( !strcmp( pFdMeta->fm_Name, pArg->l_Name) ) {
				_GElmUnset( pGE, pElm, TRUE );	// remove Hash & save
				_GElmFree( pGE, pElm );			// Free
			}
			break;
		case GE_PURGE_ALL:
			_GElmUnset( pGE, pElm, TRUE );	// remove Hash & save
			_GElmFree( pGE, pElm );			// Free
			break;
		case GE_FLUSH:
			if( !strcmp( pFdMeta->fm_Name, pArg->l_Name) ) {
				_FdMetaFlush( pFdMeta );	// only save
			}
			break;
		case GE_FLUSH_ALL:
			_FdMetaFlush( pFdMeta );		// only save
			break;
		default:
			Ret = -1;
	}
	return( Ret );
}

GFdMeta_t*
GFdMetaGet( GFdCtrl_t *pFC, char *pName, bool_t bCreate )
{
	GElm_t		*pElm;
	GFdMeta_t	*pFdMeta;

	/*
	 *	bCreate
	 *		FALSE	永続データがあれば作成、なければNULL
	 *		TRUE	永続データが無くても作成
	 */
	pElm	= GElmGet( &pFC->fc_Meta, pName, NULL, TRUE, bCreate );
	if( pElm )	pFdMeta = container_of( pElm, GFdMeta_t, fm_GE );
	else		pFdMeta = NULL;

	LOG( pFC->fc_pLog, LogDBG, "[%s] pFdMeta=%p", pName, pFdMeta );

	return( pFdMeta );
}

int
GFdMetaPut( GFdCtrl_t *pFC, GFdMeta_t *pFdMeta )
{
	int	Ret;

	LOG( pFC->fc_pLog, LogDBG, "[%s] pFdMeta=%p", pFdMeta->fm_Name, pFdMeta );

	Ret = GElmPut( &pFC->fc_Meta, &pFdMeta->fm_GE, FALSE, FALSE );

	return( Ret );
}

int
GFdMetaStat( GFdMeta_t *pFdMeta, struct stat *pStat )
{
//		fstat( pFdMeta->fm_Fd, pStat );
		*pStat	= pFdMeta->fm_Stat;
		return( 0 );
}

int
GFdMetaDelete( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;
	char	Buff[1024];

	Arg.l_Cmd	= GE_DELETE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Meta, &pFC->fc_Meta.ge_Lnk, 0, &Arg );
	if( Ret < 0 ) {
		goto err;
	}

	/* remove directory */
	snprintf( Buff, sizeof(Buff), "rm -rf \"%s/%s\"", pFC->fc_Root, pName );

	system( Buff );

	return( 0 );
err:
	LOG( pFC->fc_pLog, LogINF, "[%s] Ret=%d", Buff, Ret );

	return( Ret );
}

int
GFdMetaFlush( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_FLUSH;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Meta, &pFC->fc_Meta.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdMetaFlushAll( GFdCtrl_t *pFC )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_FLUSH_ALL;

	Ret = GElmLoop( &pFC->fc_Meta, &pFC->fc_Meta.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdMetaPurge( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_PURGE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Meta, &pFC->fc_Meta.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdMetaPurgeAll( GFdCtrl_t *pFC )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_PURGE_ALL;

	Ret = GElmLoop( &pFC->fc_Meta, &pFC->fc_Meta.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdMetaRm( char *pPath, off_t dir, bool_t bEqual )
{
	off_t	dirW;
	DIR		*pDir;
	struct dirent	*pEnt;
	char	Buff[1024];

loop:
	pDir = opendir( pPath );
	if( !pDir )	goto err;
	while( (pEnt = readdir( pDir )) ) {
		if( isdigit(pEnt->d_name[0]) ) {
			sscanf( pEnt->d_name, "%jd", &dirW );
			if( dirW > dir || ( bEqual && dirW >= dir ) ) {
				snprintf( Buff, sizeof(Buff), "rm -rf \"%s/%s\"", 
							pPath, pEnt->d_name );
				closedir( pDir );

				system( Buff );

				goto loop;
			}
		}
	}
	closedir( pDir );
	return( 0 );
err:
	return( -1 );
}

int
GFdMetaTruncate( GFdCtrl_t *pFC, char *pName, off_t Offset )
{
	char	Path[PATH_MAX];
	off_t	dir0, dir1, base;

	/* remove directory */
	FILE_CACHE_INDEX( pFC, Offset, dir0, dir1, base );

	snprintf( Path, sizeof(Path), "%s/%s", pFC->fc_Root, pName );
	GFdMetaRm( Path, dir0, FALSE );

	snprintf( Path, sizeof(Path), "%s/%s/%jd", pFC->fc_Root, pName, dir0 );
	GFdMetaRm( Path, dir1, FALSE );

	snprintf( Path, sizeof(Path), "%s/%s/%jd/%jd", 
								pFC->fc_Root, pName, dir0, dir1 );
	if( base > 0 )	base--;
	GFdMetaRm( Path, base, TRUE );

	return( 0 );
}

int
_FdSet(GElmCtrl_t *pGE, GElm_t *pElm, void *pKey, void **ppKey, 
						void *pArg, bool_t bCreate)
{
	GFd_t	*pFd = container_of( pElm, GFd_t, f_GE );
	GFdCtrl_t	*pFC = container_of( pGE, GFdCtrl_t, fc_Ctrl );
	char	Name[PATH_MAX];
	int		fd;
	char	Buff[1024];
	int		Ret;
	BlockCacheKey_t	*pBlockKey = (BlockCacheKey_t*)pArg;

	strncpy( pFd->f_Name, pBlockKey->k_Name, sizeof(pFd->f_Name) );

	pFd->f_Offset	= pBlockKey->k_Offset/pFC->fc_FileSize*pFC->fc_FileSize;

	strncpy( pFd->f_Path, pKey, sizeof(pFd->f_Path) );

	snprintf( Name, sizeof(Name), "%s/%s", pFC->fc_Root, pFd->f_Path );

	strncpy( pFd->f_DirName, Name, sizeof(pFd->f_DirName) );
	strcpy( pFd->f_DirName, dirname(pFd->f_DirName) );

	strncpy( pFd->f_BaseName, Name, sizeof(pFd->f_BaseName) );
	strcpy( pFd->f_BaseName, basename(pFd->f_BaseName) );

	pFd->f_errno	= 0;
	fd	= open( Name, O_RDWR, 0666 );
	if( fd < 0 ) {
		if( bCreate ) {
			sprintf( Buff, "mkdir -p \"%s\"", pFd->f_DirName );

			Ret = system( Buff );
			if( Ret < 0 ) {
				LOG( pFC->fc_pLog, LogERR, "Buff[%s] Ret=%d", Buff, Ret );
			}
			fd	= open( Name, O_CREAT|O_RDWR, 0666 );
			if( fd < 0 ) {
				errno	= ENOENT;
				goto err;
			}
		} else {
			errno	= ENOENT;
			goto err;
		}
	}
	pFd->f_Fd	= fd;
	if( pFd->f_FdCksum < 0 ) {
		strncat( Name, BLOCK_CKSUM_FILE_SUFFIX, sizeof(Name) );
		pFd->f_FdCksum	= open( Name, O_CREAT|O_RDWR, 0666 );
	}
	LOG( pFC->fc_pLog, LogDBG, "open:[%s] f_Fd=%d bLoad=%d", 
			pFd->f_Path, pFd->f_Fd, bCreate );

	*ppKey	= pFd->f_Path;

	return( 0 );
err:
	/* pFd be discarded */
	return( -1 );
}

int
_FdSync( GFd_t *pFd )
{
	fsync( pFd->f_Fd );
	fsync( pFd->f_FdCksum );

	return( 0 );
}

int
_FdUnset( GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave )
{
	GFd_t	*pFd = container_of( pElm, GFd_t, f_GE );

	LOG( pFd->f_pFC->fc_pLog, LogDBG, "[%s] f_Fd=%d bSave=%d", 
		pFd->f_Name, pFd->f_Fd, bSave );

	if( pFd->f_Fd < 0 )	goto ret;

	ASSERT( pFd->f_Fd >= 0 );
	if( pFd->f_Fd > 0 ) {

		close( pFd->f_Fd );
		close( pFd->f_FdCksum );

		pFd->f_Fd		= -1;
		pFd->f_FdCksum	= -1;
	}
ret:
	if( ppKey )	*ppKey	= pFd->f_Path;
	return( 0 );
}

GElm_t*
_FdAlloc( GElmCtrl_t *pGE )
{
	GFdCtrl_t	*pFC = container_of( pGE, GFdCtrl_t, fc_Ctrl );
	GFd_t	*pFd;

	pFd	= Malloc( sizeof(*pFd ) );

	ASSERT( pFd != NULL );

	memset( pFd, 0, sizeof(*pFd) );

	pFd->f_Fd		= -1;
	pFd->f_FdCksum	= -1;

	pFd->f_pFC		= pFC;

	return( (void*)pFd );
}

int
_FdFree( GElmCtrl_t *pGE, GElm_t *pElm )
{
	GFd_t	*pFd = container_of( pElm, GFd_t, f_GE );

	ASSERT( list_empty( &pElm->e_Lnk ) );
	Free( pFd );

	return( 0 );
}

int
_FdLoop( GElmCtrl_t *pGE, GElm_t *pElm, void *pVoid )
{
	int	Ret = 0;
	FdLoopArg_t	*pArg = (FdLoopArg_t*)pVoid;
	GFd_t		*pFd = container_of( pElm, GFd_t, f_GE );

	switch( pArg->l_Cmd ) {
		case GE_DELETE:
			if( !strcmp( pFd->f_Name, pArg->l_Name) ) {
				_GElmUnset( pGE, pElm, FALSE );	// remove Hash
				_GElmFree( pGE, pElm );			// Free
			}
			break;
		case GE_TRUNCATE:
			if( !strcmp( pFd->f_Name, pArg->l_Name)
					&& pArg->l_Offset <= pFd->f_Offset  ) {
				_GElmUnset( pGE, pElm, FALSE );	// remove Hash
				_GElmFree( pGE, pElm );			// Free
			}
			break;
		case GE_PURGE:
			if( !strcmp( pFd->f_Name, pArg->l_Name) ) {
				_GElmUnset( pGE, pElm, TRUE );	// sync & remove Hash
				_GElmFree( pGE, pElm );			// Free
			}
			break;
		case GE_PURGE_ALL:
			_GElmUnset( pGE, pElm, TRUE );	// sync & remove Hash
			_GElmFree( pGE, pElm );			// Free
			break;
		case GE_FLUSH:
			if( !strcmp( pFd->f_Name, pArg->l_Name) ) {
				_FdSync( pFd );			// sync
			}
			break;
		case GE_FLUSH_ALL:
			_FdSync( pFd );			// sync
			break;
		default:
			Ret = -1;
	}
	return( Ret );
}

int
GFdCacheInit( GFdCtrl_t *pFC, int FdMax, char *pRoot, 
				uint64_t FileSize, int Entries, struct Log *pLog )
{
	GElmCtrlInit( &pFC->fc_Ctrl, NULL, _FdAlloc, _FdFree, 
		_FdSet, _FdUnset,_FdLoop,
		FdMax, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, 
		Malloc ,Free, NULL );	
	strcpy( pFC->fc_Root, pRoot/*, sizeof(pFC->fc_Root)*/ );
	pFC->fc_FileSize	= FileSize;
	pFC->fc_Entries		= Entries;

	GElmCtrlInit( &pFC->fc_Meta, NULL, _FdMetaAlloc, _FdMetaFree, 
		_FdMetaSet, _FdMetaUnset, _FdMetaLoop,
		FdMax, PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, 
		Malloc ,Free, NULL );	
	pFC->fc_pLog = pLog;
	return( 0 );
}

GFd_t*
GFdCacheCreate( GFdCtrl_t *pFC, char *pName )
{
	GFd_t	*pFd;
	GElm_t	*pElm;

	pElm	= GElmGet( &pFC->fc_Ctrl, pName, NULL, TRUE, TRUE );
	if( pElm )	pFd = container_of( pElm, GFd_t, f_GE );
	else		pFd = NULL;

	LOG( pFC->fc_pLog, LogINF, "[%s] pFd=%p", pName, pFd );

	return( pFd );
}

GFd_t*
GFdCacheGet( GFdCtrl_t *pFC, BlockCacheKey_t *pKey, bool_t bCreate )
{
	GFd_t	*pFd = NULL;
	GElm_t	*pElm;
	char	Path[PATH_MAX];

	FILE_CACHE_PATH( pFC, pKey->k_Name, pKey->k_Offset, Path );

	/*
	 *	bCreate
	 *		FALSE	永続データがあれば作成、なければNULL
	 *		TRUE	永続データが無くても作成
	 */
	pElm	= GElmGet( &pFC->fc_Ctrl, Path, pKey, TRUE, bCreate );
	if( !pElm ) {
			goto err;
	}
	pFd = container_of( pElm, GFd_t, f_GE );
	return( pFd );

err:
	LOG( pFC->fc_pLog, LogDBG, "[%s] errno=%d pFd=%p", Path, errno, pFd );

	return( NULL );
}

int
GFdCachePut( GFdCtrl_t *pFC, GFd_t *pFd )
{
	int	Ret;

	LOG( pFC->fc_pLog, LogDBG, "[%s] pFd=%p", pFd->f_Path, pFd );

	Ret = GElmPut( &pFC->fc_Ctrl, &pFd->f_GE, FALSE, FALSE );

	return( Ret );
}

int
GFdCacheDelete( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_DELETE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );
	return( Ret );
}

int
GFdCacheFlush( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_FLUSH;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdCacheFlushAll( GFdCtrl_t *pFC )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_FLUSH_ALL;

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdCachePurge( GFdCtrl_t *pFC, char *pName )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_PURGE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdCachePurgeAll( GFdCtrl_t *pFC )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_PURGE_ALL;

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );

	return( Ret );
}

int
GFdCacheTruncate( GFdCtrl_t *pFC, char *pName, off_t Offset )
{
	int Ret;
	FdLoopArg_t	Arg;

	Arg.l_Cmd	= GE_TRUNCATE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );
	Arg.l_Offset	= Offset;

	Ret = GElmLoop( &pFC->fc_Ctrl, &pFC->fc_Ctrl.ge_Lnk, 0, &Arg );
	return( Ret );
}

int
FdRead( GFd_t *pFd, off_t Off, void *pData, size_t Len )
{
	int	n;

	ASSERT( pFd->f_Fd >= 0 );

	lseek( pFd->f_Fd, Off, 0 );
	n = read( pFd->f_Fd, pData, Len );

	LOG( pFd->f_pFC->fc_pLog, LogDBG, "%d=[%s](Off=%jd,Len=%zu)", 
				n, pFd->f_Path, Off, Len  );
	ASSERT( n >= 0 );

	return( n );
}

int
FdWrite( GFd_t *pFd, off_t Off, void *pData, size_t Len )
{
	int	n;

	ASSERT( pFd->f_Fd >= 0 );
	lseek( pFd->f_Fd, Off, 0 );
	n = write( pFd->f_Fd, pData, Len );

	LOG( pFd->f_pFC->fc_pLog, LogDBG, "%d =[%s](Off=%jd,Len=%zu)", 
				n, pFd->f_Path, Off, Len  );
	ASSERT( n == Len );

	return( n );
}

int
FdReadCksum( GFd_t *pFd, off_t Off, void *pData, size_t Len )
{
	int	n;

	if( pFd->f_FdCksum < 0 ) {
		return( 0 );
	}
	lseek( pFd->f_FdCksum, Off, 0 );
	n = read( pFd->f_FdCksum, pData, Len );
	return( n );
}

int
FdWriteCksum( GFd_t *pFd, off_t Off, void *pData, size_t Len )
{
	int	n;

	ASSERT( pFd->f_FdCksum >= 0 );

	lseek( pFd->f_FdCksum, Off, 0 );
	n = write( pFd->f_FdCksum, pData, Len );
	return( n );
}

int
_BlockCode( void* pVoid )
{
	BlockCacheKey_t *pKey	= (BlockCacheKey_t*)pVoid;
	long	l;
	int		code;

	l = HashStrCode( pKey->k_Name );
	code	= l + pKey->k_Offset;
	return( code );
}

int
_BlockCompare( void *pA, void *pB )
{
	BlockCacheKey_t	*pKeyA	= (BlockCacheKey_t*)pA;
	BlockCacheKey_t	*pKeyB	= (BlockCacheKey_t*)pB;
	off_t	Ret;

	Ret = pKeyB->k_Offset - pKeyA->k_Offset;
	if( Ret == 0 ) {
		Ret	= memcmp( pKeyB->k_Name, pKeyA->k_Name, sizeof(pKeyA->k_Name) );
	}
	else {
		Ret = Ret > 0 ? 1 : -1;
	}
	return( Ret );
}

int
_BlockLoad( GElmCtrl_t *pGE, GElm_t *pElm, 
					void *pKey, void **ppKey, void *pArg, bool_t bLoad )
{
	BlockCache_t		*pB = container_of( pElm, BlockCache_t, b_GE );
	BlockCacheCtrl_t	*pBC = container_of( pGE, BlockCacheCtrl_t, bc_Ctrl );
	GFdCtrl_t			*pFdCtrl = &pBC->bc_FdCtrl;
	GFd_t	*pFd;
	int		n, i, l, s;
	uint64_t	Cksum64;

	pB->b_Key	= *(BlockCacheKey_t*)pKey;
	pB->b_Flags	= 0;
	memset( pB->b_pBuf, 0, BLOCK_CACHE_SIZE(pBC) );
	memset( pB->b_pValid, 0, sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC) );
	memset( pB->b_pCksum64, 0, sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBC) );
/*
 * load 
 */
	/* Get Fd */
	pFd = GFdCacheGet( pFdCtrl, &pB->b_Key, FALSE );
	if( !pFd ) {
		pB->b_Len	= 0;
		goto exit;
	}

	/* read Data */
	off_t	off;
	off	= pB->b_Key.k_Offset % pFdCtrl->fc_FileSize;

	l = FdRead( pFd, off, pB->b_pBuf, (s = BLOCK_CACHE_SIZE(pBC)) );
	pB->b_Len	= l;
	if( l < s )	memset( pB->b_pBuf + l, 0, s - l );

	if( pBC->bc_bCksum ) {
		/* read Cksum64 */
		n = off/BLOCK_CACHE_SIZE(pBC)
			*SEGMENT_CACHE_NUM(pBC)*sizeof(uint64_t);
		s = ( l + SEGMENT_CACHE_SIZE(pBC) - 1 )/SEGMENT_CACHE_SIZE(pBC);
		l = FdReadCksum( pFd, n, pB->b_pCksum64, s*sizeof(uint64_t) );
		/*  Cksum64 ? */
		for( i = 0; i < s; i++ ) {
			if( pB->b_pCksum64[i] ) {
				Cksum64	= in_cksum64(
							&pB->b_pBuf[SEGMENT_CACHE_SIZE(pBC)*i],
							SEGMENT_CACHE_SIZE(pBC), 0 );
				// ASSERT !!!
				ASSERT( pB->b_pCksum64[i] == Cksum64 );
				// Set cksum valid time
				TIMESPEC( pB->b_pValid[i] );
				TimespecAddUsec( pB->b_pValid[i], pBC->bc_UsecWB );
			}
		}
	}
	/* Put Fd */
	GFdCachePut( pFdCtrl, pFd );

exit:
	*ppKey	= &pB->b_Key;
	return( 0 );
}

int
_BlockSave( GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave )
{
	BlockCache_t		*pB = container_of( pElm, BlockCache_t, b_GE );
	BlockCacheCtrl_t	*pBC = container_of( pGE, BlockCacheCtrl_t, bc_Ctrl );
	GFdCtrl_t			*pFdCtrl = &pBC->bc_FdCtrl;
	GFd_t	*pFd;
	int	l, n, s;
	int	Ret;
/*
 * Save 
 */
	LOG( pBC->bc_pLog, LogDBG, "%p[%s] Off=%jd Size=%d Dirty=%d bSave=%d", 
			pB, pB->b_Key.k_Name, pB->b_Key.k_Offset, 
			pB->b_Len, !list_empty(&pB->b_DirtyLnk), bSave  );

	if( list_empty( &pB->b_DirtyLnk ) )	goto ret;

	/* clear dirty */
	list_del_init( &pB->b_DirtyLnk );

	if( !(pB->b_Flags & BLOCK_CACHE_DIRTY) )	goto ret;

	pB->b_Flags &= ~BLOCK_CACHE_DIRTY;

	if( bSave ) {
		/* Get Fd */
		pFd = GFdCacheGet( pFdCtrl, &pB->b_Key, TRUE );

		ASSERT( pFd );

		/* Write Data */
		off_t	off;
		off	= pB->b_Key.k_Offset % pFdCtrl->fc_FileSize;

		Ret = FdWrite( pFd, off, pB->b_pBuf, (l = pB->b_Len) );
		if( Ret < 0 )	goto err;

		if( pBC->bc_bCksum ) {
			/* Write Cksum64 */
			n = off/BLOCK_CACHE_SIZE(pBC)
					*SEGMENT_CACHE_NUM(pBC)*sizeof(uint64_t);
			s = ( l + SEGMENT_CACHE_SIZE(pBC) - 1 ) /SEGMENT_CACHE_SIZE(pBC);
			Ret = FdWriteCksum( pFd, n, pB->b_pCksum64, s*sizeof(uint64_t) );
			if( Ret < 0 )	goto err;
		}

		/* Put Fd */
		GFdCachePut( pFdCtrl, pFd );
	}
ret:
	*ppKey	= &pB->b_Key;
	return( 0 );
err:
	return( -1 );
}

GElm_t*
_BlockAlloc( GElmCtrl_t *pGE )
{
	BlockCacheCtrl_t	*pBC = container_of( pGE, BlockCacheCtrl_t, bc_Ctrl );
	BlockCache_t	*pB;

	pB	= Malloc( sizeof(*pB ) );

	ASSERT( pB != NULL );

	memset( pB, 0, sizeof(*pB) );

	pB->b_pBuf		= Malloc( BLOCK_CACHE_SIZE(pBC) );
	pB->b_pValid	= Malloc( sizeof(timespec_t)*SEGMENT_CACHE_NUM(pBC) );
	pB->b_pCksum64	= Malloc( sizeof(uint64_t)*SEGMENT_CACHE_NUM(pBC) );

	list_init( &pB->b_DirtyLnk );

	return( (void*)pB );
}

int
_BlockFree( GElmCtrl_t *pGE, GElm_t *pElm )
{
	BlockCache_t	*pB = container_of( pElm, BlockCache_t, b_GE );

	ASSERT( list_empty( &pB->b_GE.e_Lnk ) );
	ASSERT( list_empty( &pB->b_DirtyLnk ) );

	Free( pB->b_pBuf );
	Free( pB->b_pValid );
	Free( pB->b_pCksum64 );
	Free( pB );
	return( 0 );
}

typedef struct {
	int			l_Cmd;
	char		l_Name[PATH_MAX];
	off_t		l_Off;
} LoopArg_t;

int
_BlockLoop( GElmCtrl_t *pGE, GElm_t *pElm, void *pVoid )
{
	BlockCacheCtrl_t	*pBC = container_of( pGE, BlockCacheCtrl_t, bc_Ctrl );
	BlockCache_t		*pB = container_of( pElm, BlockCache_t, b_GE );
	LoopArg_t	*pArg = (LoopArg_t*)pVoid;
	int	Ret = 0;
	off_t	l, iS, s;
	void	*pKey;

	ASSERT( pElm->e_Cnt == 0 );

	LOG( pBC->bc_pLog, LogDBG, "l_Cmd=%d %p[%s] k_Off=%jd", 
			pArg->l_Cmd, pB, pB->b_Key.k_Name, pB->b_Key.k_Offset );

	switch( pArg->l_Cmd ) {
		default:	abort();
		case GE_FLUSH_ALL:
			if( pB->b_Flags	& BLOCK_CACHE_DIRTY ) {
				_BlockSave( pGE, pElm, &pKey, TRUE );	// Save Only
			}
			break;
		case GE_FLUSH:
			if( pB->b_Flags	& BLOCK_CACHE_DIRTY
				&& !strcmp(pB->b_Key.k_Name, pArg->l_Name) ) {
				_BlockSave( pGE, pElm, &pKey, TRUE );	// Save Only
			}
			break;
		case GE_PURGE:
			if( pElm->e_Cnt > 0 ) {
				errno = EBUSY;
				Ret = -1;
			} else {
				if( !strcmp(pB->b_Key.k_Name, pArg->l_Name) ) {
					if( pB->b_Flags	& BLOCK_CACHE_DIRTY ) {
						_GElmUnset( pGE, pElm, TRUE );	// Save & remove Hash
					} else {
						_GElmUnset( pGE, pElm, FALSE );	// remove Hash
					}
					_GElmFree( pGE, pElm );	// Free
				}
			}
			break;
		case GE_PURGE_ALL:
			if( pElm->e_Cnt > 0 ) {
				errno = EBUSY;
				Ret = -1;
			} else {
				if( pB->b_Flags	& BLOCK_CACHE_DIRTY ) {
					_GElmUnset( pGE, pElm, TRUE );	// Save & remove Hash
				} else {
					_GElmUnset( pGE, pElm, FALSE );	// remove Hash
				}
				_GElmFree( pGE, pElm );	// Free
			}
			break;
		case GE_DELETE:
			if(!strncmp( pB->b_Key.k_Name,pArg->l_Name,
								sizeof(pB->b_Key.k_Name))) {

				if( pElm->e_Cnt > 0 ) {
					errno = EBUSY;
					Ret = -1;
				} else {
					_GElmUnset( pGE, pElm, FALSE );	// No Save & remove Hash
					_GElmFree( pGE, pElm );			// Free
				}
			}
			break;
		case GE_TRUNCATE:
			if(!strncmp( pB->b_Key.k_Name,pArg->l_Name,
								sizeof(pB->b_Key.k_Name))) {
				if( pArg->l_Off <= pB->b_Key.k_Offset  )	{

					if( pElm->e_Cnt > 0 ) {
						errno = EBUSY;
						Ret = -1;
					} else {
						_GElmUnset( pGE, pElm, FALSE );	// No Save & remove Hash
						_GElmFree( pGE, pElm );			// Free
					}
				} else if( (l = pArg->l_Off - pB->b_Key.k_Offset ) 
											< BLOCK_CACHE_SIZE(pBC) ) {
					pB->b_Len	= l;
					/* re-calculate checksum */
					if( pBC->bc_bCksum ) {
						iS = l/SEGMENT_CACHE_SIZE(pBC);
						s	= l - iS*SEGMENT_CACHE_SIZE(pBC);
						ASSERT( s > 0 );
						pB->b_pCksum64[iS]	=
							in_cksum64( 
								&pB->b_pBuf[SEGMENT_CACHE_SIZE(pBC)*iS],
								s, 0 );
					}
					LOG( pBC->bc_pLog, LogINF, "l=%d iS=%d s=%d (%"PRIu64"", 
										iS, l, s, pB->b_pCksum64[iS] );
				}
			}
			break;
	}
	return( Ret );	// continue;
}

int
BlockCacheInit( BlockCacheCtrl_t *pBC,
			int BlockMax, int SegSize, int SegNum, int UsecWB, 
			bool_t bCksum, struct Log *pLog )
{
	GElmCtrlInit( &pBC->bc_Ctrl, NULL, 
			_BlockAlloc, _BlockFree, _BlockLoad, _BlockSave, _BlockLoop,
			BlockMax, PRIMARY_1009, _BlockCode, _BlockCompare, NULL, 
			Malloc ,Free, NULL );	

	list_init( &pBC->bc_DirtyLnk );
	pBC->bc_SegmentSize		= SegSize;
	pBC->bc_SegmentNum		= SegNum;
	pBC->bc_UsecWB			= UsecWB;
	pBC->bc_bCksum			= bCksum;
	pBC->bc_pLog			= pLog;

	return( 0 );
}

BlockCache_t*
BlockCacheGet( BlockCacheCtrl_t *pBC, BlockCacheKey_t *pKey )
{
	BlockCache_t	*pB;

	pB	= (BlockCache_t*)GElmGet( &pBC->bc_Ctrl, pKey, NULL, TRUE, TRUE );

	if( pB ) {
		LOG( pBC->bc_pLog, LogDBG, "%p[%s] Off=%jd", 
			pB, pB->b_Key.k_Name, pB->b_Key.k_Offset );
	}
	return( pB );
}

int
BlockCachePut( BlockCacheCtrl_t *pBC, BlockCache_t *pB )
{
	int	Ret;

	LOG( pBC->bc_pLog, LogDBG, "%p[%s] Off=%jd", 
			pB, pB->b_Key.k_Name, pB->b_Key.k_Offset );

	Ret = GElmPut( &pBC->bc_Ctrl, (void*)pB, FALSE, FALSE );

	return( Ret );
}

int
BlockCacheWrite( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData )
{
	off_t	i, r, iB, iS, iE, sOff, eOff, dOff, soffB;
	size_t  Len;
	off_t	iBS, iBE;
	BlockCacheKey_t	Key;
	BlockCache_t	*pB;
	size_t	TotalLen = 0;
	size_t	lenB, slen, elen;
	uint64_t	sCksum64, mCksum64, eCksum64, nCksum64;
	

	ASSERT( Size <= MAX_SIZE(pBC) );

	memset( &Key, 0, sizeof(Key) );
	strncpy( Key.k_Name, pName, sizeof(Key.k_Name) );

	dOff	= 0;
	iBS	= Offset/BLOCK_CACHE_SIZE(pBC);
	iBE	= (Offset + Size - 1 )/BLOCK_CACHE_SIZE(pBC); 

	for( iB = iBS; iB <= iBE; iB++, Offset += Len, Size -= Len ) {

		Key.k_Offset	= BLOCK_CACHE_SIZE(pBC)*iB;

		pB	= BlockCacheGet( pBC, &Key );

		GElmRwLockW( &pB->b_GE );

		sOff	= Offset - Key.k_Offset;
		eOff	= Offset + Size - Key.k_Offset;
		if( eOff > BLOCK_CACHE_SIZE(pBC) )	eOff = BLOCK_CACHE_SIZE(pBC);

		Len	= eOff - sOff;
LOG( pBC->bc_pLog, LogDBG, "%p [%s:%jd-%d] iB=%d", 
				pB, Key.k_Name, Key.k_Offset, Len, iB );

		sCksum64 = mCksum64 = eCksum64 = 0;
		if( pBC->bc_bCksum ) {
		/* Cksum64 update-1 */

			r = sOff & 7;
			soffB	= sOff - r;
			lenB	= Len + r;
			lenB	= (lenB+7)/8*8;

			iS = soffB/SEGMENT_CACHE_SIZE(pBC);
			iE = (eOff - 1)/SEGMENT_CACHE_SIZE(pBC);
			if( iS == iE ) {
				if( lenB < SEGMENT_CACHE_SIZE(pBC)/2 && pB->b_pCksum64[iS] ) {
					mCksum64 = in_cksum64( &pB->b_pBuf[soffB], lenB, 0 );
				}
			} else {
				// begin
				slen	= (iS+1)*SEGMENT_CACHE_SIZE(pBC) - soffB;
				ASSERT( slen <= SEGMENT_CACHE_SIZE(pBC) );
				if( slen < SEGMENT_CACHE_SIZE(pBC)/2 && pB->b_pCksum64[iS] ) {
					sCksum64 = in_cksum64( &pB->b_pBuf[soffB], slen, 0 );
				}
				// end
				elen	= eOff - iE*SEGMENT_CACHE_SIZE(pBC);
				ASSERT( elen <= SEGMENT_CACHE_SIZE(pBC) );
				if( elen < SEGMENT_CACHE_SIZE(pBC)/2 && pB->b_pCksum64[iE] ) {
					eCksum64 = in_cksum64( 
							&pB->b_pBuf[iE*SEGMENT_CACHE_SIZE(pBC)], elen, 0 );
				}
			}
		}
		/* write */
		memcpy( &pB->b_pBuf[sOff], &pData[dOff], Len );
		TotalLen += Len;

		if( pBC->bc_bCksum ) {
		/* Cksum64 update-2 */
			if( iS == iE ) {
				if( mCksum64 ) {
					nCksum64 = in_cksum64( &pB->b_pBuf[soffB], lenB, 0 );
					pB->b_pCksum64[iS]	= 
								in_cksum64_sub_add( pB->b_pCksum64[iS], 
											mCksum64, nCksum64 );
				} else {
					pB->b_pCksum64[iS]	=
						in_cksum64( &pB->b_pBuf[SEGMENT_CACHE_SIZE(pBC)*iS],
									SEGMENT_CACHE_SIZE(pBC), 0 );
				}
			} else {
				for( i = iS; i <= iE; i++ ) {
					if( i == iS && sCksum64 ) {
						nCksum64 = in_cksum64( &pB->b_pBuf[soffB], slen, 0 );
						pB->b_pCksum64[iS]	= 
								in_cksum64_sub_add( pB->b_pCksum64[iS], 
											sCksum64, nCksum64 );
					} else if( i == iE && eCksum64 ) {
						nCksum64 = in_cksum64( 
							&pB->b_pBuf[iE*SEGMENT_CACHE_SIZE(pBC)], elen, 0 );
						pB->b_pCksum64[iE]	= 
								in_cksum64_sub_add( pB->b_pCksum64[iE], 
											eCksum64, nCksum64 );
					} else {	
						pB->b_pCksum64[i]	=
							in_cksum64( &pB->b_pBuf[SEGMENT_CACHE_SIZE(pBC)*i],
									SEGMENT_CACHE_SIZE(pBC), 0 );
					}
					// Set cksum valid time
					TIMESPEC( pB->b_pValid[i] );
					TimespecAddUsec( pB->b_pValid[i], pBC->bc_UsecWB );
				}
			}
		}
		dOff	+= Len;
		if( eOff > pB->b_Len )	pB->b_Len = eOff;
		pB->b_Flags	|= BLOCK_CACHE_DIRTY;

		GElmRwUnlock( &pB->b_GE );

		/* Update Dirty in global lock*/
		GElmCtrlLock( &pBC->bc_Ctrl );

		list_del_init( &pB->b_DirtyLnk );

		TIMESPEC( pB->b_Timeout );
		TimespecAddUsec( pB->b_Timeout, pBC->bc_UsecWB );

		list_add_tail( &pB->b_DirtyLnk, &pBC->bc_DirtyLnk );
LOG( pBC->bc_pLog, LogDBG, "list_add_tail DirtyLnk pB=%p",  pB );

		GElmCtrlUnlock( &pBC->bc_Ctrl );

		BlockCachePut( pBC, pB );
	}
	return( TotalLen );
}

int
BlockCacheRead( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData )
{
	off_t	iB, sOff, eOff, dOff;
	size_t	Len;
	off_t	iBS, iBE;
	off_t	iS;
	BlockCacheKey_t	Key;
	BlockCache_t	*pB;
	size_t	TotalLen = 0;

LOG( pBC->bc_pLog, LogDBG, "[%s:%jd+%zu] pData=%p", 
			pName, Offset, Size , pData );

	ASSERT( Size <= MAX_SIZE(pBC) );

	memset( &Key, 0, sizeof(Key) );
	strncpy( Key.k_Name, pName, sizeof(Key.k_Name) );

	dOff	= 0;
	iBS	= Offset/BLOCK_CACHE_SIZE(pBC);
	iBE	= (Offset + Size - 1 )/BLOCK_CACHE_SIZE(pBC); 

	for( iB = iBS; iB <= iBE; iB++, Offset += Len, Size -= Len ) {

		Key.k_Offset	= BLOCK_CACHE_SIZE(pBC)*iB;

		pB	= BlockCacheGet( pBC, &Key );

		GElmRwLockR( &pB->b_GE );

		sOff	= Offset - Key.k_Offset;
		eOff	= Offset + Size - Key.k_Offset;
		if( eOff > BLOCK_CACHE_SIZE(pBC) )	eOff = BLOCK_CACHE_SIZE(pBC);

ASSERT( sOff < BLOCK_CACHE_SIZE(pBC) );

		Len	= eOff - sOff;
LOG( pBC->bc_pLog, LogDBG,
"%p [%s:%jd+%d] iB=%d dOff=%d sOff=%d eOff=%d pData=%p b_pBuf=%p", 
pB, Key.k_Name, Key.k_Offset, Len, iB, dOff, sOff, eOff, pData, pB->b_pBuf );

		if( pBC->bc_bCksum ) {
			timespec_t	Now;

			TIMESPEC( Now );
			/* Checksum check */
			for( iS = sOff/SEGMENT_CACHE_SIZE(pBC);
				iS < (eOff+SEGMENT_CACHE_SIZE(pBC)-1)/SEGMENT_CACHE_SIZE(pBC);
					iS++ ) {

				if( TIMESPECCOMPARE( Now, pB->b_pValid[iS] ) < 0 )	continue;

				if( pB->b_pCksum64[iS] ) {

LOG( pBC->bc_pLog, LogDBG, "CHECKSUM([%s/%jd]iS=%d:%"PRIx64")", 
		Key.k_Name, Key.k_Offset, iS, pB->b_pCksum64[iS] );

					ASSERT( pB->b_pCksum64[iS] ==
						in_cksum64( 
								&pB->b_pBuf[SEGMENT_CACHE_SIZE(pBC)*iS],
								SEGMENT_CACHE_SIZE(pBC), 0 ) );

					pB->b_pValid[iS]	= Now;
					TimespecAddUsec( pB->b_pValid[iS], pBC->bc_UsecWB );
				}
			}
		}
		/* Read */
LOG( pBC->bc_pLog, LogDBG,
"%p [%s:%jd+%d] iB=%d dOff=%d sOff=%d pData=%p b_pBuf=%p", 
pB, Key.k_Name, Key.k_Offset, Len, iB, dOff, sOff, pData, pB->b_pBuf );

		if( Len > 0 )	memcpy( &pData[dOff], &pB->b_pBuf[sOff], Len );

LOG( pBC->bc_pLog, LogDBG,
"%p [%s:%jd+%d] iB=%d dOff=%d sOff=%d pData=%p b_pBuf=%p", 
pB, Key.k_Name, Key.k_Offset, Len, iB, dOff, sOff, pData, pB->b_pBuf );

		dOff		+= Len;
		TotalLen	+= Len;

		GElmRwUnlock( &pB->b_GE );

		BlockCachePut( pBC, pB );
	}
	return( TotalLen );
}

int
FileCacheCreate ( BlockCacheCtrl_t *pBC, char *pName )
{
	GFdMeta_t	*pFdMeta;
	int	Ret;

	GElmCtrlLock( &pBC->bc_Ctrl );

	pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, pName, TRUE );
	if( pFdMeta ) {
		GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );
		Ret = 0;
	} else {
		Ret = -1;
	}
	GElmCtrlUnlock( &pBC->bc_Ctrl );

	return( Ret );
}

int
FileCacheWrite( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData )
{
	size_t	Len, l;
	size_t	TotalLen = 0;
	GFdMeta_t	*pFdMeta;

	/* update Size */
	pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, pName, FALSE );
	if( !pFdMeta ) {
		errno = ENOENT;
		goto	err;
	}

	GElmRwLockW( &pFdMeta->fm_GE );

	if( pFdMeta->fm_Stat.st_size < Offset + Size ) {
		pFdMeta->fm_Stat.st_size = Offset + Size;
	}
	pFdMeta->fm_Stat.st_mtime	= time(0);

	GElmRwUnlock( &pFdMeta->fm_GE );

	GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );
	while( Size ) {
		l = ( Size < MAX_SIZE(pBC) ? Size : MAX_SIZE(pBC) );

		Len = BlockCacheWrite( pBC, pName, Offset, l, pData );

		if( Len <= 0 )	goto err;
		Offset		+= Len;
		Size		-= Len;
		pData		+= Len;
		TotalLen	+= Len;
	}
	return( TotalLen );
err:
	return( -1 );
}

int
FileCacheRead( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size, char *pData )
{
	size_t	Len, l;
	size_t	TotalLen = 0;
	GFdMeta_t	*pFdMeta;

	pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, pName, FALSE );
	if( !pFdMeta ) {
		/* zero clear return */
		memset( pData, 0, Size );
		errno = ENOENT;
		goto	err;
	}
	GElmRwLockR( &pFdMeta->fm_GE );

	if( pFdMeta->fm_Stat.st_size < Offset + Size ) {
		if( Offset < pFdMeta->fm_Stat.st_size ) {
			Size 	= pFdMeta->fm_Stat.st_size - Offset;
		} else {
			Size	= 0;
		}
	}
	pFdMeta->fm_Stat.st_atime	= time(0);
	GElmRwUnlock( &pFdMeta->fm_GE );

	GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );
	while( Size ) {
		l = ( Size < MAX_SIZE(pBC) ? Size : MAX_SIZE(pBC) );

		Len = BlockCacheRead( pBC, pName, Offset, l, pData );

		if( Len <= 0 )	goto err;
		Offset		+= Len;
		Size		-= Len;
		pData		+= Len;
		TotalLen	+= Len;
	}
	return( TotalLen );
err:
	return( -1 );
}

int
FileCacheFlush( BlockCacheCtrl_t *pBC, 
				char *pName, off_t Offset, size_t Size )
{
	off_t	iB;
	BlockCacheKey_t	Key;
	BlockCache_t	*pB;
	void	*pKey;

	LOG( pBC->bc_pLog, LogINF, 
			"[%s] Offset=%jd Size=%zu", pName, Offset, Size );

	memset( &Key, 0, sizeof(Key) );
	strncpy( Key.k_Name, pName, sizeof(Key.k_Name) );

	GElmCtrlLock( &pBC->bc_Ctrl );

	for( iB = Offset/BLOCK_CACHE_SIZE(pBC);
			iB <= (Offset + Size - 1 )/BLOCK_CACHE_SIZE(pBC);
				iB++ ) {

		Key.k_Offset	= BLOCK_CACHE_SIZE(pBC)*iB;

		pB	= BlockCacheGet( pBC, &Key );

		if( pB ) {
			/* Purge Dirty in global lock*/
//			GElmCtrlLock( &pBC->bc_Ctrl );

			if( !list_empty( &pB->b_DirtyLnk ) ) {
				_BlockSave( &pBC->bc_Ctrl, &pB->b_GE, &pKey, TRUE );// Save
			}

//			GElmCtrlUnlock( &pBC->bc_Ctrl );

			BlockCachePut( pBC, pB );
		}
	}
	GFdCacheFlush( &pBC->bc_FdCtrl, pName );

	GFdMetaFlush( &pBC->bc_FdCtrl, pName );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( 0 );
}

int
FileCacheFlushAll( BlockCacheCtrl_t *pBC )
{
	LoopArg_t	Arg;
	int	Ret;

	LOG( pBC->bc_pLog, LogINF, "FLUSH ALL");

	Arg.l_Cmd	= GE_FLUSH_ALL;

	GElmCtrlLock( &pBC->bc_Ctrl );

	Ret = GElmLoop( &pBC->bc_Ctrl, &pBC->bc_DirtyLnk, 
			offsetof(BlockCache_t,b_DirtyLnk), &Arg );

	GFdCacheFlushAll( &pBC->bc_FdCtrl );

	GFdMetaFlushAll( &pBC->bc_FdCtrl );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Ret );
}

int
FileCachePurge( BlockCacheCtrl_t *pBC, char *pName )
{
	LoopArg_t	Arg;
	int	Ret;

	LOG( pBC->bc_pLog, LogINF, "PURGE[%s]", pName );

	Arg.l_Cmd	= GE_PURGE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	GElmCtrlLock( &pBC->bc_Ctrl );

	Ret = GElmLoop( &pBC->bc_Ctrl, &pBC->bc_Ctrl.ge_Lnk, 0, &Arg );

	GFdCachePurge( &pBC->bc_FdCtrl, pName );

	GFdMetaPurge( &pBC->bc_FdCtrl, pName );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Ret );
}

int
FileCachePurgeAll( BlockCacheCtrl_t *pBC )
{
	LoopArg_t	Arg;
	int	Ret;

	LOG( pBC->bc_pLog, LogINF, "PURGE ALL");

	Arg.l_Cmd	= GE_PURGE_ALL;

	GElmCtrlLock( &pBC->bc_Ctrl );

	Ret = GElmLoop( &pBC->bc_Ctrl, &pBC->bc_Ctrl.ge_Lnk, 0, &Arg );

	GFdCachePurgeAll( &pBC->bc_FdCtrl );

	GFdMetaPurgeAll( &pBC->bc_FdCtrl );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Ret );
}

/* delete only file, no directory */
int
FileCacheDelete( BlockCacheCtrl_t *pBC, char *pName )
{
	LoopArg_t	Arg;
	int	Ret;

	LOG( pBC->bc_pLog, LogINF, "DELETE[%s]", pName );

	if( !IsMeta( pBC->bc_FdCtrl.fc_Root, pName ) )	return( -1 );

	Arg.l_Cmd	= GE_DELETE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );

	GElmCtrlLock( &pBC->bc_Ctrl );

	Ret = GElmLoop( &pBC->bc_Ctrl, &pBC->bc_Ctrl.ge_Lnk, 0, &Arg );

	GFdCacheDelete( &pBC->bc_FdCtrl, pName );
	GFdMetaDelete( &pBC->bc_FdCtrl, pName );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Ret );
}

int
FileCacheSeek( BlockCacheCtrl_t *pBC, char *pName, off_t Off )
{
	int	Ret;

	LOG( pBC->bc_pLog, LogINF, "SEEK:[%s] Off=%"PRIu64"", pName, Off );

	/* set cache-block in advance */
	Ret = FileCacheRead( pBC, pName, Off, 0, NULL );

	return( Ret );
}

int
FileCacheAppend( BlockCacheCtrl_t *pBC, char *pName, size_t Size, char *pData )
{
	off_t		Offset = 0;
	size_t	Len, l;
	size_t	TotalLen = 0;
	GFdMeta_t		*pFdMeta;

	LOG( pBC->bc_pLog, LogINF, "APPEND:[%s]", pName );

	pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, pName, FALSE );
	if( !pFdMeta ) {
		errno = ENOENT;
		goto	err;
	}
	GElmRwLockW( &pFdMeta->fm_GE );

	Offset	= pFdMeta->fm_Stat.st_size;

	pFdMeta->fm_Stat.st_size += Size;
	pFdMeta->fm_Stat.st_mtime	= time(0);

	GElmRwUnlock( &pFdMeta->fm_GE );

	GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );

	while( Size ) {
		l = ( Size < MAX_SIZE(pBC) ? Size : MAX_SIZE(pBC) );

		Len = BlockCacheWrite( pBC, pName, Offset, l, pData );

		if( Len <= 0 )	goto err;
		Offset		+= Len;
		Size		-= Len;
		pData		+= Len;
		TotalLen	+= Len;
	}
	return( TotalLen );
err:
	return( -1 );
}

int
FileCacheTruncate( BlockCacheCtrl_t *pBC, char *pName, off_t Off )
{
	GFdCtrl_t	*pFdCtrl = &pBC->bc_FdCtrl;
	GFdMeta_t		*pFdMeta;
	LoopArg_t	Arg;

	memset( &Arg, 0, sizeof(Arg) );
	Arg.l_Cmd	= GE_TRUNCATE;
	strncpy( Arg.l_Name, pName, sizeof(Arg.l_Name) );
	Arg.l_Off	= Off;

	GElmCtrlLock( &pBC->bc_Ctrl );

	LOG( pBC->bc_pLog, LogINF, "TRUNCATE:[%s] Off=%jd", pName, Off );
	/* free block-cache */
	GElmLoop( &pBC->bc_Ctrl, &pBC->bc_Ctrl.ge_Lnk, 0, &Arg );

	/* fd-cache */
	GFdCacheTruncate( pFdCtrl, pName, Off );

	/* remove directory */
	GFdMetaTruncate( pFdCtrl, pName, Off );

	pFdMeta = GFdMetaGet( pFdCtrl, pName, FALSE );
	if( pFdMeta ) {
		GElmRwLockW( &pFdMeta->fm_GE );

		pFdMeta->fm_Stat.st_size = Off;
		pFdMeta->fm_Stat.st_mtime	= time(0);

		GElmRwUnlock( &pFdMeta->fm_GE );
		GFdMetaPut( pFdCtrl, pFdMeta );
	}
	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( 0 );
}

int
FileCacheStat( BlockCacheCtrl_t *pBC, char *pName, struct stat *pStat)
{
	GFdCtrl_t	*pFdCtrl = &pBC->bc_FdCtrl;
	GFdMeta_t		*pFdMeta;

	pFdMeta = GFdMetaGet( pFdCtrl, pName, FALSE );
	if( !pFdMeta )	goto err;

	GFdMetaStat( pFdMeta, pStat );

	GFdMetaPut( pFdCtrl, pFdMeta );
	return( 0 );
err:
	return( -1 );
}

inline	static char*
RootOmitted( char *pName )
{
	char	*pC = pName;

	while( *pC ) {
		switch( *pC ) {
			case '/':
				if( *(pC+1) == '\0' ) {
					*pC	= '.';
					break;
				} else {
					pC++;
					continue;
				}
			case '.':
				if( *(pC+1) == '/' ) {
					pC++; pC++;
					continue;
				} else {
					break;
				}
		}
		break;
	}
	return( pC );
}

bool_t
IsMeta( char *pPath, char *pFile )
{
	char	Path[PATH_MAX];
	struct stat	Stat;
	int		Ret;

	if( pFile ) sprintf( Path, "%s/%s/Meta", pPath, pFile );
	else		sprintf( Path, "%s/Meta", pPath );

	Ret = stat( Path, &Stat );

	if( Ret )	return( FALSE );
	else		return( TRUE );
}

static struct dirent*
ReadDir( BlockCacheCtrl_t *pBC, DIR *pDir, char *pPath )
{
	struct dirent *pDirEnt;

	while( (pDirEnt = readdir( pDir )) ) {
LOG( pBC->bc_pLog, LogDBG, "d_name[%s] d_type=%d", 
pDirEnt->d_name, pDirEnt->d_type );

		/* skip XXX .CKSUM */
		if( strstr( pDirEnt->d_name, BLOCK_CKSUM_FILE_SUFFIX ) )	continue;

		/* skip ".XXX" */
		if( '.' == pDirEnt->d_name[0] )	continue;

		/* check Meta */
		if( pDirEnt->d_type == DT_DIR ) {
			if( IsMeta( pPath, pDirEnt->d_name ) ) {
				pDirEnt->d_type = DT_REG;
			}
LOG( pBC->bc_pLog, LogDBG, "pPath[%s] d_name[%s] d_type=%d", 
pPath, pDirEnt->d_name, pDirEnt->d_type );
		} else	{
			continue;
		}
		break;	// OK
	}
	return( pDirEnt );
}

/* omit ".", "./", "/" */
int
ConcatPath( char *pRealPath, size_t size, char *pTarget, char *pPath )
{
	size_t		Len, n;
	char	*pC;

	pC	= pPath;
	Len	= strlen( pPath );

	if( Len >= 1 && '.' == *pC ) {	pC++; Len--;}
	if( Len >= 1 && '/' == *pC ) {	pC++; Len--;}

	if( Len > 0 )	n = snprintf( pRealPath, size, "%s/%s", pTarget, pC );
	else			n = snprintf( pRealPath, size, "%s", pTarget );

	pC	= pRealPath;
	Len	= n;

	if( Len >= 1 && '.' == *pC ) {	pC++; Len--;}
	if( Len >= 1 && '/' == *pC ) {	pC++; Len--;}

	if( Len < n ) {
		if( Len > 0 ) {
			memmove( pRealPath, pC, Len );
			pRealPath[Len]	= 0;
			n	= Len;
		} else {
			strcpy( pRealPath, "." );
			n 	= 1;
		}
	}
	return( n );
}

int 
FileCacheReadDir( BlockCacheCtrl_t *pBC, char *pPath, 
				struct dirent *pDirEnt, int Ind, int Num )
{
	DIR*	pDir;
	char	*pPathName;
	struct dirent*	pDirent;
	int		n, N;
	char	Path[PATH_NAME_MAX];
	GFdMeta_t	*pFdMeta;
	char	Buff[PATH_NAME_MAX];
	char	DirName[PATH_NAME_MAX],	BaseName[FILE_NAME_MAX];

	pPathName	= RootOmitted( pPath );

	/*stat  Disk */
	if( IsMeta( pBC->bc_FdCtrl.fc_Root, pPathName ) )	goto err;

	GElmCtrlLock( &pBC->bc_Ctrl );

	n	= 0;
	N	= 0;
	/* From FdMetaCache */
	list_for_each_entry( pFdMeta, &pBC->bc_FdCtrl.fc_Meta.ge_Lnk, fm_GE.e_Lnk) {

		strncpy( Buff, pFdMeta->fm_Name, sizeof(Buff) );
		strncpy( DirName, dirname(Buff), sizeof(DirName) );
		strncpy( Buff, pFdMeta->fm_Name, sizeof(Buff) );
		strncpy( BaseName, basename(Buff), sizeof(BaseName) );

LOG(pBC->bc_pLog, LogDBG, 
"CACHE:pPathName[%s] Dir[%s] Base[%s]", 
pPathName, DirName, BaseName );
		if( strcmp( pPathName, DirName ) )	continue;

		if( pDirEnt ) {
			if( N >= Ind ) {
				memset( &pDirEnt[n], 0, sizeof(pDirEnt[n]) );
				strncpy( pDirEnt[n].d_name, BaseName, 
						sizeof(pDirEnt[n].d_name) );
				if( ++n >= Num )	goto ret;
			}
		} else {
			++n;
		}
		++N;
	}

	/* From Disk */
	ConcatPath( Path, sizeof(Path), pBC->bc_FdCtrl.fc_Root, pPathName );

	pDir = opendir( Path );
	if( !pDir ) {
		if( n > 0 )	goto ret;
		else		goto err1;
	}
	while( (pDirent = ReadDir( pBC, pDir, Path )) ) {

		ConcatPath( Buff, sizeof(Buff), pPathName, pDirent->d_name );
LOG(pBC->bc_pLog, LogDBG, 
"DISK:pPathName[%s] d_name[%s] Buff[%s]", 
pPathName, pDirent->d_name , Buff);
		// exists in cache?
		pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, Buff, FALSE );
		if( pFdMeta ) {
			GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );
			continue;
		}
		if( pDirEnt ) {
			if( N >= Ind ) {
				memset( &pDirEnt[n], 0, sizeof(pDirEnt[n]) );
				pDirEnt[n]	= *pDirent;
				if( ++n >= Num )	goto ret;
			}
		} else {
			++n;
		}
		++N;
	}
	closedir( pDir );
ret:
	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( n );
err1:
	GElmCtrlUnlock( &pBC->bc_Ctrl );
err:
	return( -1 );
}

int
FileCacheRmDir( BlockCacheCtrl_t *pBC, char *pPath )
{
	char	*pPathName;
	int		Ret;
	char	Path[PATH_NAME_MAX];

	pPathName	= RootOmitted( pPath );

	Ret	= ConcatPath( Path, sizeof(Path), pBC->bc_FdCtrl.fc_Root, pPathName );
	ASSERT( Ret < sizeof(Path) );

	/*stat  Disk */
	if( IsMeta( Path, NULL ) )	goto err;

	GElmCtrlLock( &pBC->bc_Ctrl );

	/* Disk---  only remove pure directory without childs */
	Ret = rmdir( Path );

	GElmCtrlUnlock( &pBC->bc_Ctrl );
	return( Ret );
err:
	return( -1 );
}

void*
ThreadBlockCacheWB( void* pArg )
{
	BlockCacheCtrl_t	*pBC = pArg;
	BlockCache_t		*pB;
	timespec_t	Now;
	bool_t	bContinue;
	void	*pKey;
	GFdMeta_t	*pFdMeta;

	pthread_detach( pthread_self() );

loop:
	/* wait time lag */
	usleep( pBC->bc_UsecWB );
	TIMESPEC( Now );
	while( 1 ) {

		GElmCtrlLock( &pBC->bc_Ctrl );

		pB = list_first_entry( &pBC->bc_DirtyLnk, BlockCache_t, b_DirtyLnk );

		if( pB && (TIMESPECCOMPARE( Now, pB->b_Timeout ) > 0) ) {

			_BlockSave( &pBC->bc_Ctrl, &pB->b_GE, &pKey, TRUE );
			/* save Meta too */
			pFdMeta = GFdMetaGet( &pBC->bc_FdCtrl, pB->b_Key.k_Name, TRUE );
			if( pFdMeta ) {
				_FdMetaFlush( pFdMeta );
				GFdMetaPut( &pBC->bc_FdCtrl, pFdMeta );
			}
			bContinue	= TRUE;
		} else {
			bContinue	= FALSE;
		}
		GElmCtrlUnlock( &pBC->bc_Ctrl );


		//LOG( pBC->bc_pLog, "bContinue=%d", bContinue );
		if( bContinue )	sched_yield();
		else	break;
	}
	goto loop;
}

int
FileCacheInit( BlockCacheCtrl_t *pBC, int FdMax, char *pRoot,
			int BlockMax, int SegSize, int SegNum, int UsecWB, 
			bool_t bCksum, struct Log *pLog )
{
	BlockCacheInit( pBC, BlockMax, SegSize, SegNum, UsecWB, bCksum, pLog );

	GFdCacheInit( &pBC->bc_FdCtrl, FdMax, pRoot, 
						SegSize*SegNum*FILE_BLOCKS, FILE_ENTRIES, pLog );

	pthread_create( &pBC->bc_Thread, NULL, ThreadBlockCacheWB, pBC );

	return( 0 );
}

