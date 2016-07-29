/******************************************************************************
*
*  NWGadget.c 	--- source of Gadghet Library
*
*  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
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

/**
 *	作成	渡辺典孝
 *	試験	渡辺典孝,久保正業
 *
 */

#include <ctype.h>
#include	"NWGadget.h"

/*
 *	Log
 */
typedef	struct Log {
	Mtx_t		l_Mtx;
	int			l_Flags;
	int			l_Level;
	void*		(*l_fMalloc)(size_t);
	void		(*l_fFree)(void*);
	FILE*		l_pFile;
	int			l_Size;
	uint32_t	l_Seq;
	int*		l_pBufStart;	// index in buffer 
	int*		l_pBufNext;
	char*		l_pBufEnd;
	char*		l_pBuf;
} Log_t;

Log_t*
LogCreate( int Size, void*(*fMalloc)(size_t), void(fFree)(void*), 
			int Flags, FILE* pFile, int LogLevel )
{
	Log_t*	pLog;

	pLog	= (Log_t*)fMalloc( sizeof(Log_t) + Size );
	if( !pLog )	return( NULL );

	memset( pLog, 0, sizeof(*pLog) );

	MtxInit( &pLog->l_Mtx );

	pLog->l_fMalloc	= fMalloc;
	pLog->l_fFree	= fFree;
	pLog->l_pFile	= pFile;
	pLog->l_Size	= Size;
	pLog->l_Flags	= Flags;
	pLog->l_Level	= LogLevel;

	if( Size > 0 ) {
		pLog->l_pBuf	= (char*)(pLog + 1);
		pLog->l_pBufNext = pLog->l_pBufStart = (int*)pLog->l_pBuf;
		*pLog->l_pBufStart	= -1;
		pLog->l_pBufEnd	= pLog->l_pBuf + Size;
	}

	return( pLog );
}

int
LogDestroy( Log_t *pLog )
{
	if( pLog )	(pLog->l_fFree)( pLog );
	return( 0 );
}

void
LogPrintf( FILE* pFile, char *pBuf )
{
	char	*pFmt, *pF, c;
	int		i = 0, L;
	uintptr_t	a[30];

	pF = pFmt	= pBuf;
	L = (int)strlen( pFmt );
	pBuf	+= (L+1+sizeof(int)-1)/sizeof(int)*sizeof(int);

	while( *pFmt ) {
		if( i >= 30 )	abort();
		if( *pFmt++ != '%' )	continue;
		if( (c = *pFmt++) == 's' ) {
			a[i++]	= (uintptr_t)pBuf;
			L	= (int)strlen( pBuf );
			pBuf	+= (L+1+sizeof(int)-1)/sizeof(int)*sizeof(int);
#ifdef	ZZZ
#else
		} else if( c == 'S' ) {
			a[i++]	= (uintptr_t)pBuf;
			L	= (int)wcslen( (wchar_t*)pBuf );
			pBuf	+= ((L+1)*sizeof(wchar_t)+sizeof(int)-1)
						/sizeof(int)*sizeof(int);
#endif
		} else if( c == 'p' ) {
			a[i++]	= *((uintptr_t*)pBuf);
			pBuf	+= sizeof(void*);
		} else {
			a[i++]	= (uintptr_t)(*((uintptr_t*)pBuf));
			pBuf	+= sizeof(uintptr_t);
		}
	}
	if( pFile == NULL ) {
		Printf( pF, 
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9],
		a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19],
		a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29] );	
		Printf("\n");
	} else {
		fprintf( pFile, pF,
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9],
		a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19],
		a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29] );	
		fprintf( pFile, "\n" );
		fflush( pFile );
	}
}

int
_LogDump( Log_t* pLog )
{
	int		Start;

	if( *pLog->l_pBufStart > 0 ) {
		if(  pLog->l_Flags & LOG_FLAG_BINARY ) {
assert( pLog->l_Size < 1000000000LL );
			fwrite( &pLog->l_Size, sizeof(pLog->l_Size), 1, pLog->l_pFile );
			Start	= (int)((char*)pLog->l_pBufStart 
							- (char*)pLog->l_pBuf);
			fwrite( &Start, sizeof(Start), 1, pLog->l_pFile );
			fwrite( pLog->l_pBuf, pLog->l_Size, 1, pLog->l_pFile );
			fflush( pLog->l_pFile );
		} else {
			LogDumpBuf( (int)((char*)pLog->l_pBufStart 
						- (char*)pLog->l_pBuf), 
					pLog->l_pBuf, 
					pLog->l_pFile );
		}
		pLog->l_pBufStart	= pLog->l_pBufNext	= (int*)pLog->l_pBuf;
		*pLog->l_pBufStart	= -1;
	}

	return( 0 );
}

int
Log( Log_t* pLog, int Level, char* pFmt, ... )
{
	char	*pBuf, *pNext;
	va_list	ap;
	char	*pS, c;
	void*	p;
#ifdef	ZZZ
#else
	wchar_t	*pW;
#endif
	int		L;
	char	Buf[1024];
	if( Level > pLog->l_Level )	return( 0 );

#define	LOG_SEQ	"%u."

	pBuf	= Buf;

	strcpy( pBuf, LOG_SEQ );
	L = (int)strlen( LOG_SEQ );
	strcpy( pBuf + L, pFmt );
	L += (int)strlen( pFmt );
	pBuf	+= (L+1+sizeof(int)-1)/sizeof(int)*sizeof(int);

	*((int*)pBuf)	= pLog->l_Seq;
	pBuf	+= sizeof(uintptr_t);	

	va_start( ap, pFmt );
	while( *pFmt ) {
		if( *pFmt++ != '%' )	continue;
		if( (c = *pFmt++) == 's' ) {
			pS	= va_arg( ap, char *);
			L = (int)strlen( pS );
			strcpy( pBuf, pS );
			pBuf	+= (L+1+sizeof(int)-1)/sizeof(int)*sizeof(int);
#ifdef	ZZZ
#else
		} else if( c == 'S' ) {
			pW	= va_arg( ap, wchar_t *);
			L	= (int)wcslen( pW );
			memcpy( pBuf, pW, (L+1)*sizeof(wchar_t) );
			pBuf	+= ((L+1)*sizeof(wchar_t)+sizeof(int)-1)
						/sizeof(int)*sizeof(int);
#endif
		} else if( c == 'p' ) {
			p = va_arg( ap, void*);
			*((void**)pBuf)	= p;
			pBuf	+= sizeof(void*);	
		} else {
			*((uintptr_t*)pBuf)	= va_arg(ap, uintptr_t);
			pBuf	+= sizeof(uintptr_t);	
		}
	}
	va_end( ap );
	*pBuf	= 0;
	++pLog->l_Seq;

	if( pLog->l_Size <= 0 ) {
		MtxLock( &pLog->l_Mtx );
		LogPrintf( pLog->l_pFile, Buf );
		MtxUnlock( &pLog->l_Mtx );
		return( 0 );
	}

#define	IS_LOG_BUF_OVERFLOW( L ) \
	( pLog->l_pBufStart <= pLog->l_pBufNext && \
		pLog->l_pBufEnd <= ((char*)(pLog->l_pBufNext+1) + L + sizeof(int) ) )

#define	IS_LOG_BUF_OVERRUN( L ) \
	(pLog->l_pBufStart > pLog->l_pBufNext && \
		pLog->l_pBufStart <= (int*)((char*)(pLog->l_pBufNext+1) + L + sizeof(int) ) )

#define	IS_LOG_BUF_WRAP( L ) \
	(IS_LOG_BUF_OVERFLOW( L ) || IS_LOG_BUF_OVERRUN( L ) )

	MtxLock( &pLog->l_Mtx );

	L = (int)(pBuf - Buf);
	if( IS_LOG_BUF_WRAP( L ) ) {
		if( !(pLog->l_Flags & LOG_FLAG_WRAP) )	_LogDump( pLog );
		if( IS_LOG_BUF_OVERFLOW( L ) ) {
			*pLog->l_pBufNext	= 0;
			pLog->l_pBufNext	= (int*)pLog->l_pBuf;
		}
		while( IS_LOG_BUF_OVERRUN( L ) ) {
			pLog->l_pBufStart	= (int*)(pLog->l_pBuf + *pLog->l_pBufStart);
		}
	}
	memcpy( pLog->l_pBufNext+1, Buf, L );
	pNext	= (char*)(pLog->l_pBufNext+1) + L;
	*pLog->l_pBufNext	= (int)(pNext - pLog->l_pBuf);
	pLog->l_pBufNext	= (int*)pNext; 
	*pLog->l_pBufNext	= -1;

	MtxUnlock( &pLog->l_Mtx );

	return( 0 );
}

int
LogDumpBuf( int Start, char *pBuf, FILE* pFile )
{
	int*	pBufStart = (int*)( pBuf + Start );

	while( *pBufStart >= 0 ) {
		LogPrintf( pFile, (char*)(pBufStart+1) );
		pBufStart = (int*)( pBuf + *pBufStart );
	}
	return( 0 );
}

int
LogDump( Log_t* pLog )
{
	int	Ret;

	if( pLog == NULL || pLog->l_Size <= 0 ) {
		return( -1 );
	}
	MtxLock( &pLog->l_Mtx );

	Ret = _LogDump( pLog );

	MtxUnlock( &pLog->l_Mtx );
	return( Ret );
}

static struct Log *_pLogExit;
static	void
_LogExit( void )
{
	LogDump( _pLogExit );
	LogDestroy( _pLogExit );
}

static void
_LogSigHandler( int sig )
{
	exit( -sig );
}

void
LogAtExit( struct Log *pLog )
{
	_pLogExit	= pLog;
	signal( SIGSEGV, _LogSigHandler );
	signal( SIGABRT, _LogSigHandler );
	atexit( _LogExit );
}

/*
 *	Memory
 */
static Mtx_t	_mallocMtx;
static int		_mallocInitFlag;
static int		_mallocCnt;

#ifdef _DEBUG_MEMORY_

#define	_DEBUG_MEMORY_FREE_MAX_	10000

static	Hash_t	_MallocHash;
static	list_t	_FreeList;
static	int		_FreeCnt;

typedef	struct {
	list_t		i_Lnk;
	void*		i_pVoid;
	size_t		i_Len;
	time_t		i_Malloc;
	pthread_t	i_ThreadMalloc;
	time_t		i_Free;
	pthread_t	i_ThreadFree;
} _MallocItem_t;

void
_MallocItemDump( _MallocItem_t *pI )
{
	Printf("[%p %d Malloc(%lu %u) Free(%lu %u)]\n",
		pI->i_pVoid, pI->i_Len, 
		pI->i_Malloc, (unsigned int)pI->i_ThreadMalloc,
		pI->i_Free, (unsigned int)pI->i_ThreadFree );
}

void
MallocItemDump( void* pKey, void* pItem )
{
	_MallocItem_t*	pI = (_MallocItem_t*)pItem;

	_MallocItemDump( pI );
}

void
MallocDump()
{
	Printf("### MallocDump START ###\n");
	MtxLock(&_mallocMtx);	

	HashDump( &_MallocHash );

	MtxUnlock(&_mallocMtx);	
	Printf("### END ###\n");
}

int
MallocCount()
{
	int	Cnt;

	MtxLock(&_mallocMtx);	

	Cnt = HashCount( &_MallocHash );

	MtxUnlock(&_mallocMtx);	
	return( Cnt );
}

#else
void
MallocDump()	{}

int
MallocCount()
{
	return( _mallocCnt );
}

#endif	// _DEBUG_MEMORY_

static void
_mallocInit()
{
	if( !_mallocInitFlag ) {
		_mallocInitFlag = 1;
		_mallocCnt 		= 0;

		MtxInit( &_mallocMtx );

#ifdef _DEBUG_MEMORY_
		HashInit( &_MallocHash, PRIMARY_101, HASH_CODE_PNT, HASH_CMP_PNT,
					MallocItemDump, malloc, free, NULL );
		list_init( &_FreeList );
#endif
	}
}

void*	
Malloc( size_t s ) 
{
	void*	p;

//ASSERT( s < (size_t)0x7fffffff );

	if( !_mallocInitFlag )	_mallocInit();

	MtxLock(&_mallocMtx);	

	p = malloc( s );
//ASSERT( p );
	if( p )	_mallocCnt++;

#ifdef _DEBUG_MEMORY_
	_MallocItem_t*	pI = (_MallocItem_t*)malloc( sizeof(_MallocItem_t) );
	list_init( &pI->i_Lnk );
	pI->i_pVoid		= p;
	pI->i_Len		= s;
	pI->i_Malloc	= time(0);
	pI->i_ThreadMalloc	= pthread_self();
	HashPut( &_MallocHash, p, pI );
#endif

	MtxUnlock(&_mallocMtx);	
	return( p );
}

void	
Free( void* p ) 
{
	MtxLock(&_mallocMtx);	

#ifdef _DEBUG_MEMORY_
	_MallocItem_t*	pI;

	pI = (_MallocItem_t*)HashRemove( &_MallocHash, p );
	if( !pI ) {
		list_for_each_entry( pI, &_FreeList, i_Lnk ) {
			_MallocItemDump( pI );
		}
		Printf("p=%p\n", p );
		ABORT();
	}

	pI->i_Free	= time(0);
	pI->i_ThreadFree	= pthread_self();

	list_add_tail( &pI->i_Lnk, &_FreeList );
	_FreeCnt++;
	if( _FreeCnt > _DEBUG_MEMORY_FREE_MAX_ ) {
		pI = list_first_entry( &_FreeList, _MallocItem_t, i_Lnk );	
		list_del( &pI->i_Lnk );
		free( pI );
		_FreeCnt--;
	}
#endif

	free( p );
	_mallocCnt--;

	MtxUnlock(&_mallocMtx);	
}

void*
Calloc( int s )
{
	void	*p;
	p = Malloc( s );
	memset( p, 0, s );
	return( p );
}

/*
 * NHeap
 */

/*
 *	メモリ管理では、独立性を保つため、リンク管理も別にする
 */
typedef struct _dlink {
	struct _dlink	*l_next;
	struct _dlink   *l_prev;
} _dlink_t;

static	inline void
INSERT( _dlink_t *This, _dlink_t *before )
{
	This->l_prev	= before->l_prev;
	This->l_next	= before;
	if( before->l_prev )
		before->l_prev->l_next = This;
	before->l_prev	= This;
}

static 	inline void
NDELETE( _dlink_t *This )
{
	if( This->l_prev )
		This->l_prev->l_next = This->l_next;
	if( This->l_next )
		This->l_next->l_prev = This->l_prev;
}
#define	pHEAP( i )	(((pool_t*)(pPool->p_aHeaps))+i)

//#define	SINT	2	// sizeof(int) == 4
#define	BINT	(sizeof(long)*8)	// bit width of int

struct	zheap;
typedef	struct zseg {
	union	{
		struct {
			struct	zseg	*f_back;	/* next */
			struct	zseg	*f_frwd;	/* previous */
		} s_free;
		struct {
			int	d_id;	/* data type */
			int	d_cnt;	/* link count */
		} s_data;
	} s_union;
	struct	zseg	*s_frwd;	/* previous */
	long			s_size;		/* size/next */
	struct zheap*	pHeap;
} ZSEG;

#define	f_seg	s_union.s_free
#define	f_next	s_union.s_free.f_back
#define	f_prev	s_union.s_free.f_frwd
#define	d_seg	s_union.s_data
#define	d_type	s_union.s_data.d_id
#define	d_link	s_union.s_data.d_cnt

typedef struct zheap {
	ZSEG		*h_last;
	long		h_free;		/* free bytes */
	long		h_LowHigh;
	_dlink_t	h_freeLow;
	_dlink_t	h_freeHigh;
	NHeapPool_t*	h_pHeapPool;

	int	h_flg;		/* flag */
#define			STATIS	0x100		/* statistics */
	int	h_statis[BINT];
	BTree_t*	h_pFree;
	ZSEG		h_WorkZSEG;
} ZHEAP;

static	int		_hinit();
static void _hfree( ZHEAP *_zheap, ZSEG *p );
static char	* _halloc( struct zheap *_zheap, size_t size, int type );
static	inline void INSERT( _dlink_t *This, _dlink_t *before );
static 	inline void NDELETE( _dlink_t *This );

static	int _hinit( ZHEAP *_zheap, size_t bsize, size_t LowHigh, int Flags );

typedef struct pool {
	size_t	h_size;
	size_t	h_LowHigh;
	ZHEAP	*h_heap;
} pool_t;

static long
NHeapBTCompare( void *pL, void *pR )
{
	ZSEG	*pSegR = (ZSEG*)pR;
	ZSEG	*pSegL = (ZSEG*)pL;

	long	cmp;

	cmp = pSegL->s_size - pSegR->s_size;
	if( cmp != 0 )	return( cmp );
	else		return( (long)((char*)pL - (char*)pR) );
}

static void
NHeapBTDump( void *p )
{
	ZSEG *segp = (ZSEG*)p;

	Printf( "<(%p):s_size=%ld>", segp, segp->s_size );
}

static BTree_t*
NHeapBTInit( int n )
{
	BTree_t	*pBTree;

	pBTree	= (BTree_t*)Malloc( sizeof(*pBTree) );
	BTreeInit( pBTree, n, NHeapBTCompare, NHeapBTDump, Malloc, Free, NULL );
	return( pBTree );
}

static inline int
NHeapBTInsert( BTree_t *pBTree, ZSEG *pSeg )
{
	int	Ret;

	Ret = BTInsert( pBTree, pSeg );
	return( Ret );
}

static inline int
NHeapBTDelete( BTree_t *pBTree, ZSEG *pSeg )
{
	int	Ret;

	Ret = BTDelete( pBTree, (void*)pSeg );
	return( Ret );
}

static inline ZSEG*
NHeapBTGetAndDelete( ZHEAP* _zheap, long size )
{
	BTree_t		*pBTree;
	BTCursor_t	Cursor;
	ZSEG		*pSeg = NULL;

	pBTree	= _zheap->h_pFree;
	_zheap->h_WorkZSEG.s_size = size;

	BTCursorOpen( &Cursor, pBTree );
	if( BTCFindLower( &Cursor, &_zheap->h_WorkZSEG ) )	goto ret;
	pSeg	= BTCGetKey( ZSEG*, &Cursor );
	BTCDelete( &Cursor );
ret:
	BTCursorClose( &Cursor );
	return( pSeg );
}

static	void*
NHeapPoolAlloc( NHeapPool_t* pPool, size_t s )
{
	void*	p;

	if( pPool->p_fMalloc )	p = (pPool->p_fMalloc)( s );
	else					p = Malloc( s );
	return( p );
}

static void
NHeapPoolFree( NHeapPool_t* pPool, void* p )
{
	if( pPool->p_fFree )	(pPool->p_fFree)( p );
	else					Free( p );
}

#define	POOL_ALLOC( pP, s ) \
	{(pP)->p_Use += s; \
	if( (pP)->p_Max < (pP)->p_Use ) (pP)->p_Max = (pP)->p_Use;}

#define	POOL_FREE( pP, s )	pP->p_Use -= s

int
NHeapPoolInit( NHeapPool_t* pPool,
				int Unit, int Max, 
				void*(*fMalloc)(size_t), 
				void(*fFree)(void*),
				int Flags )
{
	int	i;
	size_t	size, LowHigh;

#define	LOW_HIGH	512

	memset( pPool, 0, sizeof(*pPool) );

	pPool->p_HeapUnit	= Unit;
	pPool->p_HeapMax	= Max;
	pPool->p_fMalloc	= fMalloc;
	pPool->p_fFree		= fFree;
	pPool->p_Flags		= Flags;

	pPool->p_aHeaps	= NHeapPoolAlloc( pPool, sizeof(pool_t)*Max );
	if( !pPool->p_aHeaps )		goto err1;

	memset( pPool->p_aHeaps, 0, sizeof(pool_t)*Max );

	Unit = Unit/sizeof(long)*sizeof(long);
	size	= Unit;
	LowHigh	= LOW_HIGH;
	for( i = 0; i < Max; i++, size *= 2, LowHigh += LOW_HIGH ) {
		pPool->p_Size	+= size;
		pHEAP( i )->h_size		= size;		// Bytes
		pHEAP( i )->h_LowHigh	= LowHigh;	// Bytes

		if( !(pPool->p_Flags & HEAP_DYNAMIC ) ) {
			pHEAP(i)->h_heap = (ZHEAP*)NHeapPoolAlloc(pPool, size);		
			if( !pHEAP(i)->h_heap )	goto err2;
			if( !_hinit( pHEAP(i)->h_heap, size, LowHigh, pPool->p_Flags ) )
				goto err2;
			pHEAP(i)->h_heap->h_pHeapPool	= pPool;
			POOL_ALLOC( pPool, 2 );
		}
	}
	return( 0 );
err2:
	for( i = 0; i < Max; i++ ) {
		if( pHEAP(i)->h_heap )	
			NHeapPoolFree( pPool, pHEAP(i)->h_heap );
	}
err1:	
	return( -1 );
}

int
NHeapPoolDestroy( NHeapPool_t* pPool )
{
	int	i;

	for( i = 0; i < pPool->p_HeapMax; i++ ) {
		if( pHEAP(i)->h_heap )	
			NHeapPoolFree( pPool, pHEAP(i)->h_heap );
	}
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		NHeapMalloc()
 * 機能        
 *		メモリの確保
 * 関数値
 *      正常: !NULL
 *      異常: NULL
 * 注意
 ******************************************************************************/
void *
NHeapMalloc( NHeapPool_t* pPool, size_t size )
{
	int			i;
	register struct pool 	*hp;
	void			*mp = NULL;
	ZHEAP			*pZheap;

	for( i = 0; i < pPool->p_HeapMax; i++ ) {

		hp = pHEAP(i);

		if( !(pZheap = hp->h_heap) ) {
			pZheap = hp->h_heap	= (ZHEAP *)NHeapPoolAlloc( pPool, hp->h_size );
			if( !pZheap  )	goto err;

			pZheap->h_flg	= pPool->p_Flags;
			if( !_hinit( pZheap, hp->h_size, hp->h_LowHigh, pPool->p_Flags ) )
					goto err;
			pZheap->h_pHeapPool	= pPool;
			POOL_ALLOC( pPool, 2 );
		}

		mp = _halloc( pZheap, size, i );
		if( !mp )	continue;

		((ZSEG*)(mp)-1)->pHeap	= pZheap;
		break;
	}
err:
	return( mp );
}


/******************************************************************************
 * 関数名
 *		NHeapZalloc()
 * 機能        
 *		初期化したメモリの確保
 * 関数値
 *      正常: !NULL
 *      異常: NULL
 * 注意
 ******************************************************************************/
void *
NHeapZalloc( NHeapPool_t* pPool, size_t size )
{
	void	*mp;

	if( (mp = NHeapMalloc( pPool, size )) )
		memset( mp, 0, size );
		
	return( mp );
}

/**
 * @brief 確保したメモリサイズの変更を行う。
 *
 * @param[in] cp
 * @param[in] size
 * @return エラーが起きたら０を返す。
 */
void *
NHeapRealloc( void* cp, size_t size )
{
	ZSEG	*segp = (ZSEG*)cp;
	void	*newp;
	long	cpsize;

	segp--;

	if ((newp = NHeapMalloc( segp->pHeap->h_pHeapPool, size)) == 0) {
		goto err;
	}

	cpsize = -segp->s_size;
	memcpy( newp, cp, (cpsize < size ? cpsize : size));
	NHeapFree( cp );

	return( newp );
err:;
	return( 0 );
}

/******************************************************************************
 * 関数名
 *		NHeapFree()
 * 機能        
 *		確保したメモリの開放
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
int
NHeapFree( void* cp )
{
	ZSEG		*pZseg;
	ZHEAP		*pHeap;

	pZseg	= (ZSEG*)cp;
	pZseg--;
	pHeap	= pZseg->pHeap;

	_hfree( pHeap, (ZSEG*)cp );

	return( 0 );
}



/******************************************************************************
 * 関数名
 *		_hinit()
 * 機能        
 *		ヒープ領域を初期化する
 * 関数値
 *      正常: !0 (取得サイズ)
 *      異常: 0
 * 注意
 ******************************************************************************/
static	int
_hinit( ZHEAP *_zheap, size_t bsize, size_t LowHigh, int Flags )
{
	ZSEG	*segp, *lsegp;

	/* size check */
	/*	neeed start/end terminator */
	if( (uintptr_t)_zheap & (sizeof(long)-1) ) {
		return( 0 );
	}
	if( bsize <= sizeof(ZHEAP) + 2*sizeof( ZSEG ) ) {
		return( 0 );
	}
	bsize	-= sizeof( ZHEAP ) + 2*sizeof( ZSEG );
	bsize = bsize/sizeof(ZSEG);

	memset( _zheap, 0, sizeof(ZHEAP) );
	_zheap->h_flg	= Flags;

	/* initialize segment */
	segp = (ZSEG *)&_zheap[1];
	segp->s_frwd	= 0;
	segp->s_size	= (long)bsize;

	lsegp	= segp + 1 + bsize;
	lsegp->s_frwd	= segp;
	lsegp->s_size	= -1;
	lsegp->d_type	= -1;
	lsegp->d_link	= 0;

	/* initialize freelist */
	_zheap->h_LowHigh	= (long)(LowHigh/sizeof(ZSEG));
	_zheap->h_freeLow.l_prev = _zheap->h_freeLow.l_next
					= &_zheap->h_freeLow;
	_zheap->h_freeHigh.l_prev = _zheap->h_freeHigh.l_next
					= &_zheap->h_freeHigh;
	if( _zheap->h_flg & HEAP_BEST_FIT ) {
		_zheap->h_pFree	= NHeapBTInit( 50 );
		NHeapBTInsert( _zheap->h_pFree, segp );
	} else {
		if( segp->s_size < _zheap->h_LowHigh )	
				INSERT( (_dlink_t*)segp, &_zheap->h_freeLow );
		else	INSERT( (_dlink_t*)segp, &_zheap->h_freeHigh );
	}

	/* initialize others */
	_zheap->h_last	= lsegp;
	_zheap->h_free	= (long)bsize;	// ZSEGs

	return( (int)bsize );
}


/******************************************************************************
 * 関数名
 *		_halloc()
 * 機能        
 *		領域を取得する
 * 関数値
 *      正常: !0 (アドレス)
 *      異常: 0
 * 注意
 ******************************************************************************/
static inline ZSEG*
_hfit( _dlink_t *pHead, size_t bbsize )
{
	ZSEG	*segp;

	for( segp = (ZSEG*)pHead->l_next; (_dlink_t*)segp != pHead; 
							segp = segp->f_next ) {
		if( bbsize <= segp->s_size ) return( segp );
	}
	return( NULL );
}

static char	*
_halloc( struct zheap *_zheap, size_t size, int type )
{
	size_t	bsize, bbsize;
	int		i, m = 0;
	register ZSEG	*segp, *bsegp, *nsegp;
	NHeapPool_t	*pPool;
	bool_t	bBest;

	bsize	= (size+sizeof(ZSEG)-1)/sizeof(ZSEG);
	bbsize	= bsize + 1;

	if( _zheap->h_flg & HEAP_BEST_FIT )	bBest = TRUE;
	else	bBest = FALSE;

	segp	= NULL;
	if( bBest ) {
		segp = NHeapBTGetAndDelete( _zheap, (long)bsize );
		if( !segp )	return( NULL );
	} else {
		if( bsize <= _zheap->h_LowHigh ) {
			segp = _hfit( &_zheap->h_freeLow, bsize );
		}
		if( !segp ) segp = _hfit( &_zheap->h_freeHigh, bsize );
		if( !segp )	return( NULL );
	}

	/* found */
	pPool	= _zheap->h_pHeapPool;
	POOL_ALLOC( pPool, bbsize );

	bsegp	= segp + 1 + segp->s_size;
	if( segp->s_size >= bbsize ) {
		nsegp	= segp + bbsize;
		bsegp->s_frwd	= nsegp;
		nsegp->s_frwd	= segp;
		nsegp->s_size	= (long)(segp->s_size - bbsize);
	} else {
		nsegp = NULL;
	}

	if( !bBest )	NDELETE( (_dlink_t*)segp );

	segp->s_size	= -((long)bsize);
	segp->d_link	= 1;
	segp->d_type	= type;

	if( nsegp && nsegp->s_size > 0 ) {
		if( bBest ) {
			NHeapBTInsert( _zheap->h_pFree, nsegp );
		} else { 
			if( nsegp->s_size <= _zheap->h_LowHigh ) {
				INSERT( (_dlink_t*)nsegp, &_zheap->h_freeLow );
			} else {
				INSERT( (_dlink_t*)nsegp, &_zheap->h_freeHigh ); 
			}
		}
	}

	_zheap->h_free -= (long)bbsize;

	/* statistics */
	if( _zheap->h_flg & STATIS ) {
		long	b;
		for( b = 1, i = 0; i < BINT; i++, b <<= 1 ) {

			if( size & b )
				m = i;
		}
		_zheap->h_statis[m]++;
	}

	return( (char *)&segp[1] );
}


/******************************************************************************
 * 関数名
 *		_hfree()
 * 機能        
 *		領域を返却する
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_hfree( ZHEAP *_zheap, ZSEG *p )
{
	register ZSEG	*fp,	*bp;
	NHeapPool_t	*pPool;
	bool_t	bBest;

	p--;

	if( --p->d_link ) {		/* decrement link count */
		return;
	}

	if( _zheap->h_flg & HEAP_BEST_FIT )	bBest = TRUE;
	else	bBest = FALSE;

	pPool	= _zheap->h_pHeapPool;

	p->s_size = - p->s_size;
	_zheap->h_free += p->s_size;
	POOL_FREE( pPool, p->s_size );

	fp = p->s_frwd;
	bp	= p + 1 + p->s_size;

	/* forward concatination */
	if( fp ) {
		if( fp->s_size >= 0 ) {
			_zheap->h_free += 1;
			POOL_FREE( pPool, 1 );
			if( fp->s_size != 0 ) {
				if( bBest )	NHeapBTDelete( _zheap->h_pFree, fp );
				else	NDELETE( (_dlink_t*)fp );
			}
			bp->s_frwd	= fp;
			fp->s_size	+= 1 + p->s_size;

			p	= fp;
		}
	}
	/* backward concatination */
	if( bp->s_size >= 0 ) {
		_zheap->h_free += 1;
		POOL_FREE( pPool, 1 );
		if( bp->s_size != 0 ) {
				if( bBest )	NHeapBTDelete( _zheap->h_pFree, bp );
				else	NDELETE( (_dlink_t*)bp );
		}
		p->s_size	+= 1 + bp->s_size;
		bp = p + 1 + p->s_size;
		bp->s_frwd	= p;
	}
	if( bBest ) {
		NHeapBTInsert( _zheap->h_pFree, p );
	} else {
		if( p->s_size <= _zheap->h_LowHigh ) {
			INSERT( (_dlink_t*)p, &_zheap->h_freeLow );
		} else {
			INSERT( (_dlink_t*)p, &_zheap->h_freeHigh ); 
		}
	}
}

#ifdef	HHH
/******************************************************************************
 * 関数名
 *		_hlink()
 * 機能        
 *		リンクをつなげる
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
static void
_hlink( p )
ZSEG	*p;		/* I アドレス	*/
{
	--p;
	p->d_link++;	
}
#endif

static	int
NHeapStatistics( ZHEAP *_zheap, int cmd )
{
	int		i,	smax;
	ZSEG		*segp;
	long		size;

	switch( cmd ) {
		case HEAP_STATIS_ON:
			_zheap->h_flg	|= STATIS;
			return( _zheap->h_flg );
		case HEAP_STATIS_OFF:
			_zheap->h_flg	&= ~STATIS;
			return( _zheap->h_flg );
		case HEAP_FREE_SIZE:
			return( _zheap->h_free );
		case HEAP_FREE_SEG:
			i = 0;
			for ( segp = (ZSEG *)&_zheap[1]; segp < _zheap->h_last;
				segp = segp + 1 + size ){
				if( segp->s_size > 0 )	i++;
				if( segp->s_size >= 0 )	size = segp->s_size;
				else					size = - segp->s_size;
			}
			return( i );
		case HEAP_FREE_MAX:
			smax	= 0;
			for ( segp = (ZSEG *)&_zheap[1]; segp < _zheap->h_last;
				segp = segp + 1 + size ){
				if( segp->s_size > smax )	smax = segp->s_size;
				if( segp->s_size >= 0 )	size = segp->s_size;
				else					size = - segp->s_size;
			}
			return( smax );
		case HEAP_ALLOC_SIZE:
			size = 0;
			smax	= 0;
			for ( segp = (ZSEG *)&_zheap[1]; segp < _zheap->h_last;
				segp = segp + 1 + size ){
				if( segp->s_size < 0 )	smax += - segp->s_size;
				if( segp->s_size >= 0 )	size = segp->s_size;
				else					size = - segp->s_size;
			}
			return( smax );
	}
	return( -1 );
}

int
NHeapPoolStatistics( NHeapPool_t *pPool, int Heap, int Cmd )
{
	int Ret;

	if( pPool->p_HeapMax <= Heap )	return( -1 );

	if( pHEAP(Heap)->h_heap ) {
		Ret	= NHeapStatistics( pHEAP(Heap)->h_heap, Cmd );
	} else {
		Ret = -1;
	}
	return( Ret );
}


void
NHeapDump( ZHEAP *_zheap )
{
	ZSEG	*segp;
	int		i;	
	long	size;
	
	Printf("### heap dump(_zheap=%p,h_last=%p,h_free=%ld) ###\n", 
				_zheap, _zheap->h_last, _zheap->h_free );
	for( i = 0; i < BINT; i++ ) {
		Printf("[%d-%d]", i, _zheap->h_statis[i] );
	}

	Printf("\n=== chain list ===\n");
	for ( i = 0, segp = (ZSEG *)&_zheap[1]; segp <= _zheap->h_last;
		segp = segp + 1 + size, i++ ){

		size = segp->s_size;
		if( size < 0 )
			size	= -size;

		Printf("<%d(%p):s_frwd=%p,s_size=%ld,", 
				i, segp, segp->s_frwd, segp->s_size );
		if( segp->s_size >= 0 ) {
			Printf("f_next=%p,f_prev=%p>\n", 
					segp->f_next, segp->f_prev );
		} else {
			Printf("d_type=%d,d_link=%d>\n", 
					segp->d_type, segp->d_link );
		}
	}
	if( _zheap->h_flg & HEAP_BEST_FIT ) {
		BTreePrint( _zheap->h_pFree );
	} else {
		Printf("=== freelist(Low) ===\n");
		for( i = 0, segp = (ZSEG*)_zheap->h_freeLow.l_next; 
 			segp && segp != (ZSEG *)&_zheap->h_freeLow; 
			segp = segp->f_next, i++ ) {
			Printf(
			  "<%d(%p):f_next=%p,f_prev=%p,s_frwd=%p,s_size=%ld>\n",
				i, segp, segp->f_next, segp->f_prev, 
				segp->s_frwd, segp->s_size );
		}
		Printf("=== freelist(High) ===\n");
		for( i = 0, segp = (ZSEG*)_zheap->h_freeHigh.l_next; 
 			segp && segp != (ZSEG *)&_zheap->h_freeHigh; 
			segp = segp->f_next, i++ ) {
			Printf(
			  "<%d(%p):f_next=%p,f_prev=%p,s_frwd=%p,s_size=%ld>\n",
				i, segp, segp->f_next, segp->f_prev, 
				segp->s_frwd, segp->s_size );
		}
	}
}

void
NHeapPoolDump( NHeapPool_t *pPool )
{
	int	i;
	register struct pool 	*hp;

	Printf(
		"=== Pool(%p) Unit=%d Max=%d Size=%zu Use=%zu(ZSEG) Max=%zu(ZSEG)\n",
		pPool, pPool->p_HeapUnit, pPool->p_HeapMax,
		pPool->p_Size, pPool->p_Use, pPool->p_Max ); 

	for( i = 0; i < pPool->p_HeapMax; i++ ) {

		hp = pHEAP(i);
		if( !hp->h_heap )
			continue;
		Printf( "####### index ######-->>  %d\n", i );
		NHeapDump(hp->h_heap);
	}
}

/**
 *	Hash
 */
static HashItem_t*
HashItemAlloc( Hash_t* pHash, void* pKey, void* pValue )
{
	HashItem_t*	pItem;

	if( pHash->h_fMalloc )	
		pItem	= (HashItem_t*)(pHash->h_fMalloc)( sizeof(*pItem) );
	else
		pItem	= (HashItem_t*)Malloc( sizeof(*pItem) );
	if( pItem ) {
		list_init( &pItem->i_Link );
		pItem->i_pKey	= pKey;
		if( (uintptr_t)pHash->h_fHashCode == HASH_KEY_U64 ) {
			pItem->i_U64	= *((uint64_t*)pKey);
		}
		pItem->i_pValue	= pValue;
	}
	return( pItem );
}

static void
HashItemFree( Hash_t* pHash, HashItem_t* pItem )
{
	if( pHash->h_fFree )	(pHash->h_fFree)( pItem );
	else					Free( pItem );
}

int
HashInit( Hash_t* pHash, int size, 
			int(*fHashCode)(void*), int(*fCompare)(void*,void*),
			void(*fPrint)(void*,void*),
			void*(fMalloc)(size_t), void(*fFree)(void*),
			void(fDestroy)(Hash_t*,void*) )
{
	int	i, l;

	pHash->h_N			= size;
	pHash->h_fHashCode	= fHashCode;
	pHash->h_fCompare	= fCompare;
	pHash->h_fPrint		= fPrint;
	pHash->h_fMalloc	= fMalloc;
	pHash->h_fFree		= fFree;
	pHash->h_fDestroy	= fDestroy;
	pHash->h_Count		= 0;
	pHash->h_CurIndex	= -1;
	
	l	= sizeof(list_t)*pHash->h_N;
	if( pHash->h_fMalloc )	pHash->h_aHeads	= 
									(list_t*)(pHash->h_fMalloc)( l );
	else					pHash->h_aHeads	= (list_t*)Malloc( l );
	for( i = 0; i < pHash->h_N; i++ ) {
		list_init( &pHash->h_aHeads[i] );
	}
	return( 0 );
}

void
HashClear( Hash_t* pHash )
{
	int	i;
	HashItem_t*	pItem;

	for( i = 0; i < pHash->h_N; i++ ) {
		while( (pItem = 
		list_first_entry(&pHash->h_aHeads[i], HashItem_t, i_Link) ) ) {
			if( pHash->h_fDestroy )	
					(pHash->h_fDestroy)( pHash, pItem->i_pValue);
			list_del( &pItem->i_Link );
			HashItemFree( pHash, pItem );
		}
	}
}

void
HashDestroy( Hash_t* pHash )
{
	HashClear( pHash );

	if( pHash->h_fFree )	(pHash->h_fFree)( pHash->h_aHeads );
	else					Free( pHash->h_aHeads );
}

long
HashStrCode( char *pKey )
{
	uchar_t	*pC = (uchar_t*)pKey;
	long	s, v;
	long	len, r, n;
	int		i;

	r = (uintptr_t)pC&(sizeof(long)-1);

	len	= (long)strlen( (char*)pC );

	s 	= 0;
	n	= len/sizeof(long);

	for( i = 0; i < n; i++ ) {
		if( r )	memcpy( &v, pC, sizeof(long) );
		else	v = *(long*)pC;
		s	+= v;
		pC	+= sizeof(long);
		len	-= sizeof(long);
	}
	if( len ) {
		v = 0;	
		memcpy( &v, pC, len );
		s	+= v;
	}
	return( s );
}

static int
HashCode( Hash_t* pHash, void* pKey )
{
	uint32_t 	Index;
	uint32_t	Key;
	uchar_t		*pC;
	int			len, n4, r;
	uint32_t	s, v;
	int			i;
	int			odd;
	uint64_t	U64;

	switch( (uintptr_t)pHash->h_fHashCode ) {
		case HASH_KEY_U64:
			U64	= *((uint64_t*)pKey);	
			Index	= (uint32_t)(U64-(U64/pHash->h_N)*pHash->h_N); 
			break;
		case HASH_KEY_PNT:
		case HASH_KEY_INT:
			Key	= (uint32_t)((uintptr_t)pKey);
			Index	= Key - (Key/pHash->h_N)*pHash->h_N; 
			break;
		case HASH_KEY_STR:
			pC	= (uchar_t*)pKey;
			odd	= (uintptr_t)pKey & 0x3;
			len	= (int)strlen( (char*)pC );
			n4	= len/sizeof(s);
			r	= len - n4*sizeof(s);
			s	= 0;
			for( i = 0; i < n4; i++, pC += sizeof(s) ) {
				if( odd )	memcpy( &v, pC, sizeof(v) );
				else		v	= *((uint32_t*)pC);
				s	+= v;
			}
			if( r ) {
				v	= 0;
				memcpy( &v, pC, r );
				s	+= v;
			}
			Index	= s/pHash->h_N;
			Index	= s - pHash->h_N*Index;
			break;
		default:
			Key	= (pHash->h_fHashCode)( pKey );
			Index	= Key - (Key/pHash->h_N)*pHash->h_N; 
			break;
	}
	return( Index );
}

static int
HashCompare( Hash_t* pHash, HashItem_t* pItem, void* pKey )
{
	void	*i_pKey = pItem->i_pKey;

	switch( (uintptr_t)pHash->h_fHashCode ) {
		case HASH_KEY_U64:
			return( pItem->i_U64 != *((uint64_t*)pKey) );	
		case HASH_KEY_PNT:
			return( i_pKey != pKey );
		case HASH_KEY_INT:
			return( (int)((uintptr_t)i_pKey - (uintptr_t)pKey) );
		case HASH_KEY_STR:
			return( strcmp( (char*)i_pKey, (char*)pKey ) );
		default:
			return( (pHash->h_fCompare)( i_pKey, pKey ) );
	}
}

HashItem_t*
HashGetItem( Hash_t* pHash, void* pKey )
{
	int	Index;
	int	Ret;
	HashItem_t	*pItem;

	Index	= HashCode( pHash, pKey );

	list_for_each_entry( pItem, &pHash->h_aHeads[Index], i_Link ) {

		Ret = HashCompare( pHash, pItem, pKey );
		if( !Ret ) {
			pHash->h_CurIndex	= Index;
			pHash->h_pCurItem	= pItem;
			return( pItem );
		}
	}
	pHash->h_CurIndex	= -1;
	return( NULL );
}

HashItem_t*
HashNextItem( Hash_t* pHash, void* pKey )
{
	HashItem_t	*pItem;
	int	Ret;

	if( pHash->h_CurIndex < 0 )	return( NULL );

	pItem	= pHash->h_pCurItem;

	list_for_each_entry_continue( pItem, 
				&pHash->h_aHeads[pHash->h_CurIndex], i_Link ) {
		Ret = HashCompare( pHash, pItem, pKey );
		if( !Ret ) {
			pHash->h_pCurItem	= pItem;
			return( pItem );
		}
	}
	pHash->h_CurIndex	= -1;
	return( NULL );
}

void*
HashGet( Hash_t* pHash, void* pKey )
{
	HashItem_t	*pItem;

	pItem	= HashGetItem( pHash, pKey );

	return( (pItem ? pItem->i_pValue : NULL) );
}

void*
HashNext( Hash_t* pHash, void* pKey )
{
	HashItem_t	*pItem;

	pItem	= HashNextItem( pHash, pKey );

	return( (pItem ? pItem->i_pValue : NULL) );
}

/*
 *	重複を許す
 *		pValueをFreeできないため
 */
int
HashPut( Hash_t* pHash, void* pKey, void* pValue )
{
	HashItem_t	*pItem;
	int			Index;

	pItem	= HashItemAlloc( pHash, pKey, pValue );
	if( !pItem )	return( -1 );
	Index	= HashCode( pHash, pKey );
	list_add_tail( &pItem->i_Link, &pHash->h_aHeads[Index] );
	pHash->h_Count++;
	return( 0 );
}

int
HashRemoveItem( Hash_t* pHash, HashItem_t* pItem )
{
	if( pItem ) {
		list_del( &pItem->i_Link );
		pHash->h_Count--;
		HashItemFree( pHash, pItem );
		return( 0 );
	} else
		return( -1 );
}

void*
HashRemove( Hash_t* pHash, void* pKey )
{
	HashItem_t*	pItem;
	void*		pValue = NULL;

	pItem	= HashGetItem( pHash, pKey );
	if( pItem ) {
		pValue	= pItem->i_pValue;
		HashRemoveItem( pHash, pItem );
	}
	return( pValue );
}


HashItem_t*
HashIsExist( Hash_t* pHash )
{
	int	i;
	HashItem_t*	pItem;

	for( i = 0; i < pHash->h_N; i++ ) {
		pItem = list_first_entry( &pHash->h_aHeads[i], HashItem_t, i_Link ); 
		if( pItem )	return( pItem );
	}
	return( NULL );
}

void
HashDump( Hash_t* pHash )
{
	int	i;
	HashItem_t*	pItem;

	Printf("=== Hash(%p) START ===\n", pHash );
	for( i = 0; i < pHash->h_N; i++ ) {
		if( !list_empty( &pHash->h_aHeads[i] ) ) {
			Printf("%d:", i );
			list_for_each_entry( pItem, &pHash->h_aHeads[i], i_Link ) {
				if( pHash->h_fPrint )
					(pHash->h_fPrint)( pItem->i_pKey, pItem->i_pValue );
				else
					Printf("[%p-%p] ", pItem->i_pKey, pItem->i_pValue );
			}
			Printf("\n");
		}
	}
	Printf("=== Hash(%p) END ===\n", pHash );
}

int
HashCount( Hash_t* pHash )
{
	return( pHash->h_Count );
}

int
DHashInit( DHash_t* pDHash, int Prim1, int Prim1Size, int Prim2, 
			int(*fHashCode)(void*), int(*fCompare)(void*,void*),
			void(*fPrint)(void*,void*),
			void*(fMalloc)(size_t), void(*fFree)(void*),
			void(fDestroy)(Hash_t*,void*) )
{
	int	Ret = -1;
	int	l;

	pDHash->d_Primary1		= Prim1;
	pDHash->d_Primary1Size 	= Prim1Size;
	pDHash->d_Primary2		= Prim2;

	l	= sizeof(DHashElm_t)*Prim1;
	if( fMalloc )	pDHash->d_aElm	= (DHashElm_t*)(fMalloc)( l );
	else			pDHash->d_aElm	= (DHashElm_t*)Malloc( l );
	if( !pDHash->d_aElm )	goto err;

	memset( pDHash->d_aElm, 0, l );

	Ret = HashInit( &pDHash->d_Hash, Prim1, 
				fHashCode, fCompare, fPrint, fMalloc, fFree, fDestroy );
err:	
	return( Ret );
}

void
DHashClear( DHash_t* pDHash )
{
	int	i;
	DHashElm_t	*pElm;

	HashClear( &pDHash->d_Hash );
	for( i = 0; i < pDHash->d_Primary1; i++ ) {
		pElm	= &pDHash->d_aElm[i];
		if( pElm->e_pHash ) {
			HashClear( pElm->e_pHash );
		}	
		pElm->e_Cnt	= 0;
	}
}

void
DHashDestroy( DHash_t* pDHash )
{
	int	i;
	DHashElm_t	*pElm;
	void	(*fFree)(void*);
	Hash_t	*pHash;

	if( pDHash->d_Hash.h_fFree )	fFree = pDHash->d_Hash.h_fFree;
	else							fFree = Free;

	HashDestroy( &pDHash->d_Hash );

	for( i = 0; i < pDHash->d_Primary1; i++ ) {
		pElm	= &pDHash->d_aElm[i];
		pHash	= pElm->e_pHash;
		if( pHash ) {
			HashDestroy( pHash );
			(*fFree)( pHash );
		}
	}
	(*fFree)( pDHash->d_aElm );
}

int
DHashPut( DHash_t* pDHash, void* pKey, void* pValue )
{
	int	Ind1;
	Hash_t		*pHash1, *pHash2;
	DHashElm_t	*pElm1;
	int			Ret = -1;
	void	(*fFree)(void*);
	void*	(*fMalloc)(size_t);

	if( pDHash->d_Hash.h_fFree )	fFree = pDHash->d_Hash.h_fFree;
	else							fFree = Free;
	if( pDHash->d_Hash.h_fMalloc )	fMalloc = pDHash->d_Hash.h_fMalloc;
	else							fMalloc = Malloc;

	pHash1	= &pDHash->d_Hash;
	Ind1	= HashCode( pHash1, pKey );
	pElm1	= &pDHash->d_aElm[Ind1];

	if( pElm1->e_Cnt < pDHash->d_Primary1Size ) {
		Ret = HashPut( pHash1, pKey, pValue );
	} else {
		pHash2	= pElm1->e_pHash;
		if( !pHash2 ) {
			pHash2	= (Hash_t*)(*fMalloc)( sizeof(Hash_t) );

			Ret = HashInit( pHash2, pDHash->d_Primary2, 
				pDHash->d_Hash.h_fHashCode, pDHash->d_Hash.h_fCompare,
				pDHash->d_Hash.h_fPrint, 
				pDHash->d_Hash.h_fMalloc, pDHash->d_Hash.h_fFree,
				pDHash->d_Hash.h_fDestroy );
			if( Ret ) {
				(*fFree)( pHash2 );
				goto  err;
			}	
			pElm1->e_pHash	= pHash2;
		}
		Ret = HashPut( pHash2, pKey, pValue );
	}
	pElm1->e_Cnt++;
	pHash1->h_Count++;

err:
	return( Ret );
}

void*
DHashGet( DHash_t* pDHash, void* pKey )
{
	int	Ind1;
	Hash_t		*pHash1, *pHash2;
	DHashElm_t	*pElm1;
	HashItem_t	*pItem = NULL;
	int	Ret;

	pHash1	= &pDHash->d_Hash;
	Ind1	= HashCode( pHash1, pKey );
	pElm1	= &pDHash->d_aElm[Ind1];

	pHash1->h_CurIndex	= Ind1;

	pHash2	= pElm1->e_pHash;
	if( pHash2 ) {
		pItem	= HashGetItem( pHash2, pKey );
		pHash1->h_pCurItem	= NULL;
	}
	if( !pItem ) {
		list_for_each_entry( pItem, &pHash1->h_aHeads[Ind1], i_Link ) {
			Ret = HashCompare( pHash1, pItem, pKey );
			if( !Ret ) {
				pHash1->h_pCurItem	= pItem;
				goto ret;
			}
		}
		goto err;
	}
ret:
	return( (pItem ? pItem->i_pValue : NULL) );
err:
	return( NULL );
}

void*
DHashNext( DHash_t* pDHash, void* pKey )
{
	int	Ind1;
	Hash_t		*pHash1, *pHash2;
	DHashElm_t	*pElm1;
	HashItem_t	*pItem = NULL;
	int	Ret;

	pHash1	= &pDHash->d_Hash;
	Ind1	= pHash1->h_CurIndex;
	if( Ind1 < 0 || pHash1->h_N <= Ind1 ) {
		goto err;
	}
	pElm1	= &pDHash->d_aElm[Ind1];
	pHash2	= pElm1->e_pHash;
	if( pHash2 ) {
		pItem	= HashNextItem( pHash2, pKey );
		pHash1->h_pCurItem	= NULL;
	}
	if( !pItem ) {
		list_for_each_entry( pItem, &pHash1->h_aHeads[Ind1], i_Link ) {
			Ret = HashCompare( pHash1, pItem, pKey );
			if( !Ret ) {
				pHash1->h_pCurItem	= pItem;
				goto ret;
			}
		}
		goto err;
	}
ret:
	return( (pItem ? pItem->i_pValue : NULL) );
err:
	return( NULL );
}

void*
DHashRemove( DHash_t* pDHash, void* pKey )
{
	HashItem_t*	pItem;
	void*		pValue = NULL;
	int	Ind1;
	Hash_t		*pHash1, *pHash2;
	DHashElm_t	*pElm1;

	pValue	= DHashGet( pDHash, pKey );
	if( !pValue )	goto err;

	pHash1	= &pDHash->d_Hash;
	Ind1	= pHash1->h_CurIndex;
	pElm1	= &pDHash->d_aElm[Ind1];
	pHash2	= pElm1->e_pHash;

	pItem	= pHash1->h_pCurItem;
	if( pItem ) {
		pValue	= pItem->i_pValue;
		HashRemoveItem( pHash1, pItem );
		pElm1->e_Cnt--;
	} else if( pHash2 ) {
		pItem	= pHash2->h_pCurItem;
		pValue	= pItem->i_pValue;
		HashRemoveItem( pHash2, pItem );
		pElm1->e_Cnt--;
	}
	return( pValue );
err:
	return( NULL );
}

int
DHashCount( DHash_t *pDHash )
{
	int	i;
	int	Total = 0;
	DHashElm_t	*pElm1;

	for( i = 0; i < pDHash->d_Primary1; i++ ) {
		pElm1	= &pDHash->d_aElm[i];
		Total	+= pElm1->e_Cnt;
	}
	return( Total );
}

void
DHashDump( DHash_t *pDHash )
{
	int	i;
	DHashElm_t	*pElm1;

	Printf("### DHash(%p) Dump Start ###\n", pDHash );

	Printf("--- 1-st Hash Start ---\n");
	HashDump( &pDHash->d_Hash );
	Printf("--- 1-st Hash End ---\n");

	for( i = 0; i < pDHash->d_Primary1; i++ ) {
		pElm1	= &pDHash->d_aElm[i];
		if( pElm1->e_Cnt == 0 && !pElm1->e_pHash )	continue;

		Printf("--- %d --- Start\n", i );
		Printf("e_Cnt=%d e_pHash=%p\n", pElm1->e_Cnt, pElm1->e_pHash );
		if( pElm1->e_pHash )	HashDump( pElm1->e_pHash );
		Printf("--- %d --- End\n", i );
	}
	Printf("### DHash(%p) Dump End ###\n", pDHash );
}

/*
 *	Btree
 */
#define	BTN	(pBTree->b_N)
#define	BTI_CMP( pL, pR ) \
	(pBTree->b_fCompare == NULL ? \
	 (char*)(pL)-(char*)(pR) : pBTree->b_fCompare((pL),(pR)))

#define	BTITEM_KEY( Item )	(Item)

#ifdef	VISUAL_CPP

inline int
BTIND_LT( BTree_t *pBTree, BTPage_t *pPage, void *pKey )
{ 
	int	i, L, R;
	L = 1; R = pPage->p_M+1;
	while( L < R ) {
		i = ( L + R )>>1;
		if(BTI_CMP(BTITEM_KEY(pPage->p_aItems[i-1]), pKey ) < 0 ){
			L = i+1;
		} else	R = i;
	}
	R--; 
	return( R );
}

inline int
BTIND_LE( BTree_t *pBTree, BTPage_t *pPage, void *pKey )
{ 
	int	i, L, R;
	L = 1; R = pPage->p_M+1;
	while( L < R ) {
		i = ( L + R )>>1;
		if(BTI_CMP(BTITEM_KEY(pPage->p_aItems[i-1]), pKey ) <= 0 ){
			L = i+1;
		} else	R = i;
	}
	R--; 
	return( R );
}

#else

#define	BTIND( pBTree, pPage, pKey, cmp ) \
({ int	i, L, R; \
	L = 1; R = pPage->p_M+1; \
	while( L < R ) { \
		i = ( L + R )>>1; \
		if(BTI_CMP(BTITEM_KEY(pPage->p_aItems[i-1]), pKey ) cmp 0 )	L = i+1; \
		else														R = i; \
	} \
	R--; \
	R; \
})
#endif

#define	BTITEM_INIT( Item, pKey )	BTITEM_KEY(Item) = (pKey)

#define	BTMOVE_ITEM( pToPage, toR, Item ) \
					(pToPage)->p_aItems[(toR)-1] = (Item);

#define	BTMOVE( pTo, toR, pFr, frR ) \
{	BTMOVE_ITEM( pTo, toR, (pFr)->p_aItems[(frR)-1] ); \
	(pTo)->p_apPages[(toR)]	= (pFr)->p_apPages[(frR)]; \
	if( (pTo)->p_apPages[(toR)] ) { \
		(pTo)->p_apPages[(toR)]->p_pParent		= (pTo); \
		(pTo)->p_apPages[(toR)]->p_ParentIndex	= (toR); \
	} \
}

#define	BTMOVE0( To, Fr )	(To)	= (Fr)

#define	BTSET( pPage, R, Item, pChild ) \
{	BTMOVE_ITEM( pPage, R,	Item ); \
	(pPage)->p_apPages[(R)]	= pChild; \
	if( (pPage)->p_apPages[(R)] ) { \
		(pPage)->p_apPages[(R)]->p_pParent		= (pPage); \
		(pPage)->p_apPages[(R)]->p_ParentIndex	= (R); \
	} \
}

#define	BTSET0( pPage, pChild ) \
{	(pPage)->p_apPages[0]	= pChild; \
	if( (pPage)->p_apPages[0] ) { \
		(pPage)->p_apPages[0]->p_pParent		= (pPage); \
		(pPage)->p_apPages[0]->p_ParentIndex	= 0; \
	} \
}

#define	BTSETN( pPage, N, pChild ) \
{	(pPage)->p_apPages[(N)]	= pChild; \
	if( (pPage)->p_apPages[(N)] ) { \
		(pPage)->p_apPages[(N)]->p_pParent		= (pPage); \
		(pPage)->p_apPages[(N)]->p_ParentIndex	= (N); \
	} \
}

int
BTCursorOpen( BTCursor_t* pCursor, BTree_t* pBTree )
{
	memset( pCursor, 0, sizeof(*pCursor) );
	pCursor->c_pBTree	= pBTree;
	return( 0 );
}

int
BTCursorClose( BTCursor_t* pCursor )
{
	return( 0 );
}

int
BTreeInit( BTree_t* pBTree, int n, 
		long(*fCompare)(void*,void*), void(*fPrint)(void*),
		void*(*fMalloc)(size_t), void(*fFree)(void*),
		int(*fDestroy)(BTree_t*,void*) )
{
	memset( pBTree, 0, sizeof(*pBTree) );
	pBTree->b_N				= n;
	pBTree->b_fCompare		= fCompare;
	pBTree->b_fPrint		= fPrint;
	pBTree->b_fMalloc		= fMalloc;
	pBTree->b_fFree			= fFree;
	pBTree->b_fDestroy		= fDestroy;
	return( 0 );
}

int
BTreeDestroy( BTree_t* pBTree )
{
	BTCursor_t	Cursor;
	int	Ret;
	void *pVoid;

	BTCursorOpen( &Cursor, pBTree );
	Ret = BTCHead( &Cursor );
	if( Ret < 0 )	goto err;

	do {
		pVoid = BTCGetKey( void*, &Cursor );
		if( pBTree->b_fDestroy ) {
			Ret = (*pBTree->b_fDestroy)( pBTree, pVoid );
			if( Ret < 0 )	goto err;
		}
		BTCDelete( &Cursor );
	} while( BTCNext(&Cursor) >= 0 );

	BTCursorClose( &Cursor );
	return( 0 );
err:
	BTCursorClose( &Cursor );
	return( -1 );
}

uint64_t
BTreeCount( BTree_t *pBTree )
{
	return( pBTree->b_Cnt );
}

static BTPage_t*
BPageAlloc( BTree_t* pBTree, int n )
{
	BTPage_t	*pPage;
	int	l;

	l =	sizeof(BTPage_t) + 2*n*sizeof(BTItem_t) + (2*n+1)*sizeof(BTPage_t*);

	if( pBTree->b_fMalloc )	pPage	= (BTPage_t*)pBTree->b_fMalloc( l );
	else 					pPage	= MallocCast( BTPage_t, l );
	memset( pPage, 0, l );

	pPage->p_aItems		= (BTItem_t*)((char*)pPage + sizeof(BTPage_t));
	pPage->p_apPages	= (BTPage_t**)((char*)pPage 
							+ sizeof(BTPage_t) + 2*n*sizeof(BTItem_t) );
	return( pPage );
}

static void
BPageFree( BTree_t* pBTree, BTPage_t *pPage )
{
	if( pBTree->b_fFree )	pBTree->b_fFree( pPage );
	else 					Free( pPage );
}

static BTPage_t*
split( BTree_t* pBTree, BTPage_t* pPage, int R, 
							BTItem_t* pV, BTPage_t* pR, BTPage_t* pL )
{
	BTPage_t*	pNewPage;
	BTItem_t		UP;
	int	i;
	int	n = BTN;

	pNewPage	= BPageAlloc( pBTree, n );
	if( !pNewPage )	return( NULL );

	BTITEM_INIT( UP, NULL );

	if( R <= n ) {

		/* 後半を移動 */
		for( i = n+1; i <= 2*n; i++ ) {
			BTMOVE( pNewPage, i-n, pPage, i );
		}
		if( R != n ) {

			/* 挿入、前半の末尾をUP */
			BTMOVE0( UP, pPage->p_aItems[n-1] );
			BTSET0( pNewPage, pPage->p_apPages[n] );

			/* 場所を空ける */
			for( i = n-1; i > R; i-- ) {
				BTMOVE( pPage, i+1, pPage, i );
			}

			/* Itemと子を挿入 */
			BTSET( pPage, R+1, *pV, pR );
			BTSETN( pPage, R, pL );
			BTMOVE0( *pV, UP );
		} else {

			/* V <= pR */
			BTSET0( pNewPage, pR );
			BTSETN( pPage, n, pL ); 
		}
	} else {

		/* 後半の先頭をUP */
		BTMOVE0(UP, pPage->p_aItems[n] );
		BTSET0( pNewPage, pPage->p_apPages[n+1] );

		/* R< 以降を移動 */
		for( i = 2*n; i > R; i-- ) {
			BTMOVE( pNewPage, i-n, pPage, i );
		}

		/* 挿入 */
		BTSET( pNewPage, R-n, *pV, pR );
		BTSETN( pNewPage, R-n-1, pL ); 

		/* Rまで移動 */
		for( i = R; i > n+1; i-- ) {
			BTMOVE( pNewPage, i-n-1, pPage, i );
		}
		BTMOVE0( *pV, UP );
	}
	pPage->p_M	= pNewPage->p_M	= n;
	
	return( pNewPage );
}

int
BTInsert( BTree_t* pBTree, void* pKey )
{
	BTPage_t	*pPage, *pP = NULL, *pNewPage = NULL;
	int	R = 0, i;
	BTItem_t	V;

	pBTree->b_Cnt++;

	pPage	= pBTree->b_pRoot;

	BTITEM_INIT( V, pKey );

	/* 葉を探す */
	while( pPage ) {

#ifdef	VISUAL_CPP
		R = BTIND_LT( pBTree, pPage, BTITEM_KEY(V) );
#else
		R = BTIND( pBTree, pPage, BTITEM_KEY(V), < );
#endif
		pP	= pPage->p_apPages[R];

		if( !pP )	break;
		pPage = pP;
	}
	pP	= NULL;
	while( 1 ) {

		/* root */
		if( pPage == NULL ) {
			pPage	= BPageAlloc( pBTree, BTN );
			BTSET0( pPage, pP );
			BTSET( pPage, 1, V, pNewPage );
			pPage->p_M			= 1;
			pBTree->b_pRoot		= pPage;
			return( 0 );
		}
		if( pPage->p_M == 2*BTN ) {

			pNewPage = split( pBTree, pPage, R, &V, pNewPage, pP );

			pP	= pPage;

			R = pPage->p_ParentIndex;
			pPage	= pPage->p_pParent;

			continue;
		}

		/* 挿入 */
		for( i = pPage->p_M; i > R; i-- ) {
			BTMOVE( pPage, i+1, pPage, i );
		}
		BTSET( pPage, R+1, V, pNewPage );
		pPage->p_M++;
		return( 0 );
	}
}

static bool_t
underflow( 
	BTree_t*	pBTree,
	BTPage_t* 	pA,	// 不足ページ
	int s  )		// 親ページでのindex
{
	BTPage_t* 	pC = pA->p_pParent; 	// 親ページ
	BTPage_t*	pB;
	int	i, k, mb, mc, n;

//ASSERT( pA->p_M == BTN-1 );
	n	= pA->p_M + 1;	// pA->p_M = BTN - 1 のはず
	mc	= pC->p_M;
	if( s < mc ) {
	/* 右の兄弟ページがある */
		s++;
		pB	= pC->p_apPages[s];
		mb	= pB->p_M;
		/* pBからpAへ移す数。収容可能をバランスする。負であれば併合できる。*/
//ASSERT( n == BTN );
		k	= ( mb - n + 1 )>>1 /* /2 */; 
		/* 親をpAの末尾に移動 */
		BTSET( pA, n, pC->p_aItems[s-1], pB->p_apPages[0] );

		if( k > 0 ) {
			/* 兄弟を移動 */
			for( i = 1; i <= k-1; i++ ) {
				BTMOVE( pA, i+n, pB, i );
			} 
			/* 親に移動 */
			BTMOVE0( pC->p_aItems[s-1], pB->p_aItems[k-1] );

			BTSET0( pB, pB->p_apPages[k] );
			mb	= mb - k;
			/* 残りを平行に移動 */
			for( i = 1; i <= mb; i++ ) {
				BTMOVE( pB, i, pB, i+k );
			}
			pB->p_M	= mb;
			pA->p_M	= n -1 + k;

			return( FALSE );
		} else {
			/* pBをpAに併合 */
			for( i = 1; i <= n; i++ ) {
				BTMOVE( pA, i+n, pB, i );
			}
			for( i = s; i <= mc -1; i++ ) {
				BTMOVE( pC, i, pC, i+1 );
			}
			pA->p_M	= 2*n;
			pC->p_M	= mc - 1;

			BPageFree( pBTree, pB );

			return( mc <= n );
		}
	} else {
		/* pBを左の兄弟ページとする */
		pB	= pC->p_apPages[s-1];
		mb	= pB->p_M + 1;
		/* pBからpAへ移す数。負であれば併合する。*/
		k	= (mb - n)/2;
		if( k > 0 ) {
			/* 移動(バランス) */
			/* 空きを作る */
			pA->p_apPages[n+k]	= pA->p_apPages[n];
			for( i = n - 1; i >= 1; i-- ) {
				BTMOVE( pA, i+k, pA, i );
			}
			/* 親を移し、ポインタも設定。親はpA以下である */
			BTSET( pA, k, pC->p_aItems[s-1], pA->p_apPages[0] );
			/* 兄弟を移動 */
			mb	= mb - k;
			for( i = k - 1; i >= 1; i-- ) {
				BTMOVE( pA, i, pB, i+mb );
			}
			BTSET0( pA, pB->p_apPages[mb] );
			/* 親に移動 */
			BTSET( pC, s, pB->p_aItems[mb-1], pA );
			pB->p_M	= mb - 1;
			pA->p_M	= n - 1 + k;

			return( FALSE );
		} else {
			/* 併合 */
			BTSET( pB, mb, pC->p_aItems[s-1], pA->p_apPages[0] );
			for( i = 1; i <= n-1; i++ ) {
				BTMOVE( pB, i+mb, pA, i );
			}
			pB->p_M	= 2*n;
			pC->p_M	= mc - 1;

			BPageFree( pBTree, pA );

			return( mc <= n );
		}
	}
}

int
BTDelete( BTree_t* pBTree, void* pKey )
{
	int	Ret;
	BTCursor_t	Cursor;

	BTCursorOpen( &Cursor, pBTree );

	Ret = BTCFind( &Cursor, pKey );
	if( !Ret )	Ret = BTCDelete( &Cursor );

	BTCursorClose( &Cursor );

	return( Ret );
}

int
_BTCNext( BTCursor_t* pCursor )
{
	BTPage_t*	pPage;
	int			Index;

	pPage	= pCursor->c_pPage;
	Index	= pCursor->c_zIndex;
	if( pPage->p_apPages[Index+1] ) {	// Down
		pPage	= pPage->p_apPages[Index+1];
		/* 葉の左端 */
		while( pPage->p_apPages[0] )	pPage = pPage->p_apPages[0];

		pCursor->c_pNextPage	= pPage;
		pCursor->c_NextIndex	= 0;
	} else if( Index + 1 < pPage->p_M ) {	// Brother
		pCursor->c_pNextPage	= pPage;
		pCursor->c_NextIndex	= ++Index;
	} else  {								// Up
		do {
			Index	= pPage->p_ParentIndex;
			pPage	= pPage->p_pParent;
		} while ( pPage && Index >= pPage->p_M );
		if( !pPage	) {
			pCursor->c_pNextPage	= NULL;
			pCursor->c_NextIndex	= -1;
			return( -1 );
		} else {
			pCursor->c_pNextPage	= pPage;
			pCursor->c_NextIndex	= Index;
		}
	}
	pCursor->c_pNextKey	= BTITEM_KEY(pPage->p_aItems[pCursor->c_NextIndex]);
	return( 0 );
}

int
BTCHead( BTCursor_t* pCursor )
{
	BTPage_t *pPage;

	pPage	= pCursor->c_pBTree->b_pRoot;
	while( pPage && pPage->p_apPages[0] ) {
		pPage	= pPage->p_apPages[0];
	}
	pCursor->c_pPage	= pPage;
	pCursor->c_zIndex	= 0;
	if( pPage ) {
		pCursor->c_pKey		= BTITEM_KEY( pPage->p_aItems[0] );
		_BTCNext( pCursor );
		return( 0 );
	} else		return( -1 );
}

int
BTCNext( BTCursor_t* pCursor )
{
	pCursor->c_pPage	= pCursor->c_pNextPage;
	pCursor->c_zIndex	= pCursor->c_NextIndex;
	pCursor->c_pKey		= pCursor->c_pNextKey;

	if( pCursor->c_pPage ) {
		_BTCNext( pCursor );
		return( 0 );
	} else {
		return( -1 );
	}
}

int
BTCPrev( BTCursor_t *pCursor )
{
	BTPage_t*	pPage, *pParent;
	int			Index;

	pPage = pCursor->c_pPage;
	Index = pCursor->c_zIndex;
	if( !pPage )	goto err;

	while( pPage )  {
		if( pPage->p_apPages[Index] ) { // Down
			pPage	= pPage->p_apPages[Index];
			Index	= pPage->p_M;
		} else {
			Index--; // move left
			if( 0 <= Index )	break;

			do {
				pParent	= pPage->p_pParent;
				if( !pParent )	goto err;

				Index	= pPage->p_ParentIndex;
				pPage	= pParent;

			} while( Index == 0 );

			Index--;
			break;
		}
	}
	if( !pPage )	goto err;

	pCursor->c_pNextPage	= pCursor->c_pPage;
	pCursor->c_NextIndex	= pCursor->c_zIndex;

	pCursor->c_pPage	= pPage;
	pCursor->c_zIndex	= Index;
	
	return( 0 );
err:
	return( -1 );
}

int
BTCFind( BTCursor_t* pCursor, void* pKey )
{
	BTree_t*	pBTree;
	int		Ret = -1;
	long	cmp;

	pBTree	= pCursor->c_pBTree;

	if( BTCFindLower( pCursor, pKey ) )	goto ret;

	do {
		cmp = (int)BTI_CMP( pCursor->c_pKey, pKey );
		if( cmp == 0 ) {
			Ret = 0;
			break;
		} else if( cmp < 0 ) {
			break;
		}
	} while( BTCNext( pCursor ) == 0 );

ret:
	return( Ret );
}

int
BTCFindLower( BTCursor_t* pCursor, void* pKey )
{
	BTree_t*	pBTree;
	BTPage_t*	pPage;
	int	R;

	pBTree	= pCursor->c_pBTree;
	pPage	= pBTree->b_pRoot;
	pCursor->c_pPage	= NULL;
	while( pPage ) {
#ifdef	VISUAL_CPP
		R = BTIND_LE( pBTree, pPage, pKey );
#else
		R = BTIND( pBTree, pPage, pKey, <= );
#endif
//DBG_PRT("pPage=%p M=%d R=%d\n", pPage, pPage->p_M, R );
		if( R == pPage->p_M )  {
			/* オーバであれば後ろを調べる */
			if( BTI_CMP( BTITEM_KEY(pPage->p_aItems[R-1]), pKey ) < 0 ) {
				pPage = pPage->p_apPages[R];
				continue;
			} else {
				R--;
			}
		}

		/* 前を調べる */
		while( R - 1 >= 0 ) {
			if( BTI_CMP( BTITEM_KEY(pPage->p_aItems[R-1]), pKey ) < 0 ) break;
			R--;
		}

		pCursor->c_pPage	= pPage;
		pCursor->c_zIndex	= R;

		pPage	= pPage->p_apPages[R];
	}
	if( pCursor->c_pPage ) {
		pCursor->c_pKey	= 
			BTITEM_KEY( pCursor->c_pPage->p_aItems[pCursor->c_zIndex] );

		_BTCNext( pCursor );

		return( 0 );
	} else
		return( -1 );
}

int
BTCFindUpper( BTCursor_t* pCursor, void* pKey )
{
	BTree_t*	pBTree;
	BTPage_t*	pPage;
	int	L;

	pBTree	= pCursor->c_pBTree;
	pPage	= pBTree->b_pRoot;
	pCursor->c_pPage	= NULL;
	while( pPage ) {
#ifdef	VISUAL_CPP
		L = BTIND_LE( pBTree, pPage, pKey );
#else
		L = BTIND( pBTree, pPage, pKey, <= );
#endif
//DBG_PRT("pPage=%p M=%d L=%d I=%p\n", pPage, pPage->p_M, L );
//printf("pPage=%p M=%d L=%d I=%p K=%p\n", 
//	pPage, pPage->p_M, L, pPage->p_aItems[L], pKey );
		if( L == 0 ) {
			if( BTI_CMP( BTITEM_KEY(pPage->p_aItems[0]), pKey ) > 0 ) {
				pPage = pPage->p_apPages[0];
				continue;
			}
		}
		pCursor->c_pPage	= pPage;
		pCursor->c_zIndex	= L - 1;

		pPage	= pPage->p_apPages[L];
	}
	if( pCursor->c_pPage ) {
		pCursor->c_pKey	= 
			BTITEM_KEY( pCursor->c_pPage->p_aItems[pCursor->c_zIndex] );

		_BTCNext( pCursor );

		return( 0 );
	} else
		return( -1 );
}

int
BTCDelete( BTCursor_t* pCursor )
{
	int	zIndex, i;
	BTPage_t	*pP, *pA, *pParent;
	BTree_t*	pBTree;

	if( pCursor->c_pPage == NULL )	return( -1 );

	pBTree	= pCursor->c_pBTree;

	pBTree->b_Cnt--;
	/* 左の葉の最大値と入れ替える */
	pA		= pCursor->c_pPage;
	zIndex	= pCursor->c_zIndex;
	pP	= pA->p_apPages[zIndex];
	if( pP ) {
		while( pP->p_apPages[pP->p_M] )	pP	= pP->p_apPages[pP->p_M];
		/*  pKeyを移動 */
		BTMOVE0( pA->p_aItems[zIndex], pP->p_aItems[pP->p_M-1]);
		pP->p_M--;

		/* 半分より少ない */
		if( pP->p_M < BTN ) {
			pParent	= pP->p_pParent;
			while( pParent && underflow( pBTree, pP, pP->p_ParentIndex ) ) {
				pP		= pParent;
				pParent	= pParent->p_pParent;
			}
		}
	} else {
		/* 葉であった */
		for( i = zIndex+1; i < pA->p_M; i++ ) {
			BTMOVE( pA, i, pA, i+1 );
		}
		pA->p_M--;
		if( pA->p_M < BTN ) {
			pP	= pA;
			pParent	= pP->p_pParent;
			while( pParent && underflow( pBTree, pP, pP->p_ParentIndex ) ) {
				pP		= pParent;
				pParent	= pParent->p_pParent;
			} 
		}
	}
	if( pBTree && pBTree->b_pRoot->p_M == 0 ) {

		pP = pBTree->b_pRoot->p_apPages[0];

		BPageFree( pBTree, pBTree->b_pRoot );

		if( pP )	pP->p_pParent	= NULL;
		pBTree->b_pRoot	= pP;
	}

	/* BTCNextの準備 */
	if( pCursor->c_pNextPage ) {
		BTCFind( pCursor, pCursor->c_pNextKey );
		pCursor->c_pNextPage	= pCursor->c_pPage;
		pCursor->c_NextIndex	= pCursor->c_zIndex;
		pCursor->c_pNextKey		= pCursor->c_pKey;
	} 

	return( 0 );
}

void
PrintCursor( BTCursor_t* pCursor )
{
	if( pCursor->c_pPage ) {
		Printf("CUR:<%p-%d> %p %p\n", 
			pCursor->c_pPage, pCursor->c_zIndex, 
			pCursor->c_pKey, 
			BTITEM_KEY(pCursor->c_pPage->p_aItems[pCursor->c_zIndex]) );
		if( pCursor->c_pNextPage ) {
		Printf("NXT:<%p-%d> %p %p\n", 
			pCursor->c_pNextPage, pCursor->c_NextIndex, 
			pCursor->c_pNextKey,
			BTITEM_KEY(pCursor->c_pNextPage->p_aItems[pCursor->c_NextIndex]) );
		}
	} else {
		Printf( "<NULL>\n");
	}
}

static void
PrintPageTree( BTree_t* pBTree, BTPage_t* pPage, int level )
{
	int	i, j;

	if( pPage != NULL ) {
		for( j = 0; j < level; j++ )	Printf("  ");
		Printf("%p[%d]-%p:",pPage->p_pParent,pPage->p_ParentIndex,pPage);
		for( i = 0; i < pPage->p_M; i++ ) { 
			if( pBTree->b_fPrint == NULL ) {
				Printf("%p(%d)",BTITEM_KEY(pPage->p_aItems[i]),
						PNT_INT32(BTITEM_KEY(pPage->p_aItems[i])) );
			 } else {
				Printf("\n");
				for( j = 0; j < level; j++ )	Printf("  ");
				Printf("%d", i );
				(*pBTree->b_fPrint)( BTITEM_KEY(pPage->p_aItems[i]) );
			}

		}
		Printf("\n");
		if( pPage->p_M > 0 ) {
			for( i = 0; i <= pPage->p_M; i++ )
				PrintPageTree( pBTree, pPage->p_apPages[i], level+1 );
		}
	}
}

void
BTreePrint( BTree_t* pBTree )
{
	Printf("===BTree(%p) START===\n", pBTree );
	PrintPageTree( pBTree, pBTree->b_pRoot, 0 );
	Printf("===BTree(%p) END===\n", pBTree );
}

long 
BTreeCheck( BTree_t *pBTree )
{
	BTCursor_t	Cursor;
	void	*pPrev, *pCur;
	long	cmp;
	long	count = 0;

	BTCursorOpen( &Cursor, pBTree );

	BTCHead( &Cursor );
	pPrev 	= BTCGetKey( void*, &Cursor );
	while( !BTCNext( &Cursor ) ) {
		pCur	= BTCGetKey( void*, &Cursor );
		cmp = (long)BTI_CMP( pPrev, pCur );
		if( cmp > 0 )	return( -1 );
		count++;
		pPrev = pCur;
	}
	BTCursorClose( &Cursor );
	return( count );
}


/*
 *	Timer
 */

static void*
TimerMalloc( Timer_t* pTimer, size_t s )
{
	if( pTimer->t_Flags & TIMER_HEAP )	return(NHeapMalloc(&pTimer->t_Heap,s));
	else if( pTimer->t_fMalloc )	return( (*pTimer->t_fMalloc)( s ) );
	else							return( Malloc(s) );
}

static void
TimerFree( Timer_t* pTimer, void* p )
{
	if( pTimer->t_Flags & TIMER_HEAP ) 	NHeapFree( p );
	else if( pTimer->t_fFree )	(*pTimer->t_fFree)( p );
	else						Free( p );
}

static long
TimerEventCompare( void* pA, void* pB )
{
	TimerEvent_t	*pAEvent	= (TimerEvent_t*)pA;
	TimerEvent_t	*pBEvent	= (TimerEvent_t*)pB;

	return((long)TIMESPECCOMPARE( pAEvent->e_Timeout, pBEvent->e_Timeout ));
}

static TimerEvent_t*
TimerEventAlloc( Timer_t* pTimer, int ms, 
				int(*fEvent)(TimerEvent_t*), void* pArg )
{
	TimerEvent_t*	pEvent;

	pEvent	= (TimerEvent_t*)TimerMalloc( pTimer, sizeof(TimerEvent_t) );
	list_init( &pEvent->e_List );
	pEvent->e_fEvent	= fEvent;
	pEvent->e_pArg		= pArg;
	pEvent->e_MS		= ms;
	
	return( pEvent );
}

static void
TimerEventFree( Timer_t* pTimer, TimerEvent_t* pEvent )
{
	TimerFree( pTimer, pEvent );
}

static int
TimerBTDestroy( BTree_t* pBTree, void* p )
{
	Timer_t*	pTimer = (Timer_t*)pBTree->b_pVoid;

	TimerFree( pTimer, p );
	return( 0 );
}

int
TimerInit( 
	Timer_t* pTimer, int Flags,
	int HeapUnit, int HeapMax, int HeapDynamic,
	void*(*fMalloc)(size_t), void(*fFree)(void*), 
	int BTreeN, void(*fBTItemPrint)(void*) )
{
	int	Ret;

	memset( (void*)pTimer, 0, sizeof(*pTimer) );

	MtxInit( &pTimer->t_Mtx );
	CndInit( &pTimer->t_Cnd );
	list_init( &pTimer->t_Events );
	QueueInit( &pTimer->t_Execs );

	pTimer->t_Flags = Flags;

	if( pTimer->t_Flags & TIMER_HEAP ) {
		Ret	= NHeapPoolInit( &pTimer->t_Heap, HeapUnit, HeapMax, 
						fMalloc, fFree, HeapDynamic );
		if( Ret )	goto err1;
	} else {
		pTimer->t_fMalloc	= fMalloc;
		pTimer->t_fFree		= fFree;
	}
	if( pTimer->t_Flags & TIMER_BTREE ) {
		Ret = BTreeInit( &pTimer->t_BTree, BTreeN, TimerEventCompare, 
				fBTItemPrint, fMalloc, Free, TimerBTDestroy );
		if( Ret )	goto err2;

		pTimer->t_BTree.b_pVoid	= (void*)pTimer;
	}
	HashInit( &pTimer->t_Hash, PRIMARY_101, HASH_CODE_PNT, HASH_CMP_PNT,
					NULL, fMalloc, fFree, NULL );	
	return( 0 );
err1:
err2:
	return( -1 );
}

void
TimerDestroy( Timer_t* pTimer )
{
	if( pTimer->t_Flags & TIMER_BTREE )	BTreeDestroy( &pTimer->t_BTree );
	if( pTimer ->t_Flags& TIMER_HEAP )	NHeapPoolDestroy( &pTimer->t_Heap );
	HashDestroy( &pTimer->t_Hash );
}

void
TimerEnable( Timer_t* pTimer )
{
	pTimer->t_Active	= 1;
}

void
TimerDisable( Timer_t* pTimer )
{
	pTimer->t_Active	= 0;
}

void*
TimerPerformerThread( void* pArg )
{
	DECLARE( Timer_t*, pTimer, pArg );
#ifdef	VISUAL_CPP
#else
//	int	ret;
#endif
	TimerEvent_t*	pEvent;

	while( pTimer->t_Active ) {
//#ifdef	VISUAL_CPP
		QueueWaitEntry(&pTimer->t_Execs, pEvent, e_List);
		if( pEvent ) {
			TIMESPEC( pEvent->e_Exec );
			(*pEvent->e_fEvent)( pEvent );
			TimerEventFree( pTimer, pEvent );
		}
//#else
/***
		ret = QueueWaitEntry( &pTimer->t_Execs, pEvent, e_List );
		if( !ret && pEvent ) {
			TIMESPEC( pEvent->e_Exec );
			(*pEvent->e_fEvent)( pEvent );
			TimerEventFree( pTimer, pEvent );
		}
***/
//#endif
	}
	pthread_exit( NULL );
	return( NULL );
}

void*
TimerThread( void*	pArg )
{
	DECLARE( Timer_t*, pTimer, pArg );
	int	Ret;
	TimerEvent_t*	pEvent;
	TimerEvent_t*	pW;
	struct timespec	Now={0,};
	BTCursor_t		Cursor;

	if( pTimer->t_Flags & TIMER_BTREE )
		BTCursorOpen( &Cursor, &pTimer->t_BTree );

	MtxLock( &pTimer->t_Mtx );

	while( pTimer->t_Active ) {

		if( pTimer->t_Flags & TIMER_BTREE ) {
			if( !BTCHead( &Cursor ) ) 	
				pEvent = BTCGetKey(TimerEvent_t*, &Cursor);
			else
				pEvent = NULL;
		} else {
			pEvent	= 
				list_first_entry( &pTimer->t_Events, TimerEvent_t, e_List );
		}

		if( pEvent == NULL ) {
			pTimer->t_OnOff		= 0;
			Ret	= CndWait( &pTimer->t_Cnd, &pTimer->t_Mtx );
			continue;
		} else {

			pTimer->t_OnOff		= 1;
			pTimer->t_Timeout	= pEvent->e_Timeout;
			pEvent->e_Mode	= TE_HEAD;

			Ret = CndTimedWait( &pTimer->t_Cnd, &pTimer->t_Mtx, 
													&pEvent->e_Timeout );
			if( Ret == ETIMEDOUT ) {

				TIMESPEC( Now );
				if( pTimer->t_Flags & TIMER_BTREE ) {
					while( !BTCHead( &Cursor ) ) {
						pEvent	= BTCGetKey( TimerEvent_t*, &Cursor );
						if( TIMESPECCOMPARE( pEvent->e_Timeout, Now ) < 0 ) {

							BTCDelete( &Cursor );

							HashRemove( &pTimer->t_Hash, pEvent );
							pEvent->e_Fired = Now;
							pEvent->e_Mode	= TE_PERFORMER;
	
							QueuePostEntry( &pTimer->t_Execs, pEvent, e_List );
						} else {
							break;
						}
					}
				} else {
					list_for_each_entry_safe( pEvent,  pW,
							&pTimer->t_Events, e_List ) {

						if( TIMESPECCOMPARE( pEvent->e_Timeout, Now ) < 0 ) {

							list_del_init( &pEvent->e_List );
							HashRemove( &pTimer->t_Hash, pEvent );

							pEvent->e_Fired = Now;
							pEvent->e_Mode	= TE_PERFORMER;

							QueuePostEntry( &pTimer->t_Execs, pEvent, e_List );
						} else {
							break;
						}
					}
				}
			} else {
				if( HashGet( &pTimer->t_Hash, pEvent ) ) {
					pEvent->e_Mode	= TE_TIMER;
				}
			}
		}
	}
	MtxUnlock( &pTimer->t_Mtx );
	pthread_exit( NULL );

	return( NULL );
}

int
TimerStart( Timer_t* pTimer )
{
	pthread_create( &pTimer->t_ThreadPerformer, NULL, 
					TimerPerformerThread, (void*)pTimer );
	pthread_create( &pTimer->t_ThreadTimer, NULL, 
					TimerThread, (void*)pTimer );
	return( 0 );
}

int
TimerStop( Timer_t* pTimer )
{
void* _TimerCancel( Timer_t* pTimer, TimerEvent_t* pEvent );

	TimerEvent_t	*pEvent;

	MtxLock( &pTimer->t_Mtx );

	TimerDisable( pTimer );

	/* purge events */
	do {
#ifdef	ZZZ
		pEvent	= (TimerEvent_t*)HashIsExist( &pTimer->t_Hash );

		if( pEvent )	_TimerCancel( pTimer, pEvent );
#else
		HashItem_t	*pItem;

		pEvent = NULL;
		pItem	= HashIsExist( &pTimer->t_Hash );
		if( pItem ) {
			pEvent	= (TimerEvent_t*)pItem->i_pValue;
			_TimerCancel( pTimer, pEvent );
		}
#endif

	} while( pEvent );

	CndSignal( &pTimer->t_Cnd );
	QueueAbort( &pTimer->t_Execs, -SIGKILL );

	MtxUnlock( &pTimer->t_Mtx );

	pthread_join( pTimer->t_ThreadTimer, NULL );
	pthread_join( pTimer->t_ThreadPerformer, NULL );

	return( 0 );
}

static int
TimerAdd( Timer_t* pTimer, TimerEvent_t* pEvent )
{
	TimerEvent_t	*pE;

	MtxLock( &pTimer->t_Mtx );

	TIMESPEC( pEvent->e_Registered );
	TIMEOUT( pEvent->e_Registered, pEvent->e_MS, pEvent->e_Timeout );


	if( pTimer->t_Flags & TIMER_BTREE ) {
		BTInsert( &pTimer->t_BTree, (void*)pEvent );
	} else {
		if( list_empty( &pTimer->t_Events ) ) {
			list_add_tail( &pEvent->e_List, &pTimer->t_Events );
		} else {
			list_for_each_entry( pE, &pTimer->t_Events, e_List ) {
				if( TIMESPECCOMPARE( pEvent->e_Timeout, pE->e_Timeout ) > 0 )
					continue;
				/* 前に挿入 */
				list_add_tail( &pEvent->e_List, &pE->e_List );
				goto next;
			}
			list_add_tail( &pEvent->e_List, &pTimer->t_Events );
next:;
		}
	}
	pEvent->e_Mode	= TE_TIMER;

	HashPut( &pTimer->t_Hash, pEvent, pEvent );

	if( pTimer->t_OnOff == 0 
		|| ( pTimer->t_OnOff == 1 
				&& TIMESPECCOMPARE( pEvent->e_Timeout, 
								pTimer->t_Timeout ) < 0 ) )	{
		CndSignal( &pTimer->t_Cnd );
	}
	MtxUnlock( &pTimer->t_Mtx );

	return( 0 );
}

void*
_TimerCancel( Timer_t* pTimer, TimerEvent_t* pEvent )
{
	void	*pArg = NULL;

/*	1.待(pArg)
 *	2.先頭でタイムアウト発行(pArg)
 *	3.実行へ引き渡し済み(NULL)
 *	4.存在しない(NULL)
 */
	if( HashGet( &pTimer->t_Hash, pEvent ) ) {
		if( pEvent->e_Mode == TE_TIMER || pEvent->e_Mode == TE_HEAD ) {
			if( pTimer->t_Flags & TIMER_BTREE )	
				BTDelete( &pTimer->t_BTree, pEvent );
			else	
				list_del_init( &pEvent->e_List );

			pArg = pEvent->e_pArg;

			HashRemove( &pTimer->t_Hash, pEvent );

			if( pEvent->e_Mode == TE_HEAD ) {
				CndSignal( &pTimer->t_Cnd );
			}

			TimerEventFree( pTimer, pEvent );
		}
	}
	return( pArg );
}

void*
TimerCancel( Timer_t* pTimer, TimerEvent_t* pEvent )
{
	void	*pArg;

	MtxLock( &pTimer->t_Mtx );
	pArg	= _TimerCancel( pTimer, pEvent );
	MtxUnlock( &pTimer->t_Mtx );
	return( pArg );
}

TimerEvent_t*
TimerSet( Timer_t* pTimer, int ms, int(*fEvent)(TimerEvent_t*), void* pArg )
{
	TimerEvent_t*	pEvent;

	pEvent	= TimerEventAlloc( pTimer, ms, fEvent, pArg );

	TimerAdd( pTimer, pEvent );

	return( pEvent );
}

/*
 *	メッセージ操作
 */

Msg_t*
MsgCreate( int s, void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	Msg_t	*pM;

	pM				= (Msg_t*)(fMalloc)( sizeof(*pM) );
	memset( pM, 0, sizeof(*pM) );
	list_init( &pM->m_Lnk );
	pM->m_aVec		= (MsgVec_t*)(fMalloc)( sizeof(MsgVec_t)*s );
	memset( pM->m_aVec, 0, sizeof(MsgVec_t)*s );
	pM->m_Size		= s;
	pM->m_N			= 0;
	pM->m_fMalloc	= fMalloc;
	pM->m_fFree		= fFree;
	pM->m_Cnt	= 1;

	return( pM );
}

int
MsgDestroy( Msg_t* pM )
{
	int	i;
	MsgVec_t	*pV;

	if( --pM->m_Cnt > 0 )	return( 0 );

	if( pM->m_pOrg ) {
		MsgDestroy( pM->m_pOrg );
	} else {	// leaf
		for( i = 0; i < pM->m_N; i++ ) {
			pV	= &pM->m_aVec[i];
			if( pV->v_Flags & VEC_MINE ) {
				(pM->m_fFree)(pV->v_pVoid );
			}
		}
	}
	if( pM->m_aVec )	(pM->m_fFree)( pM->m_aVec );
	(pM->m_fFree)( pM );
	return( 0 );
}

Msg_t*
MsgClone( Msg_t* pM )
{
	Msg_t	*pClone;
	int		i;

	pClone		= (Msg_t*)(pM->m_fMalloc)( sizeof(*pClone) );
	if( pClone ) {
		memcpy( pClone, pM, sizeof(*pClone) );
		list_init( &pClone->m_Lnk );
		pClone->m_aVec		= (MsgVec_t*)
				(pClone->m_fMalloc)( sizeof(MsgVec_t)*pM->m_Size );
		for( i = 0; i < pM->m_N; i++ ) {
			pClone->m_aVec[i]	= pM->m_aVec[i];
			pClone->m_aVec[i].v_Flags	&= ~VEC_MINE;
		}
	}
	if( !pClone->m_pOrg ) {
		pClone->m_pOrg = pM;
	}
	pClone->m_pOrg->m_Cnt++;
	pClone->m_Cnt	= 1;
	return( pClone );
}

int
MsgAdd( Msg_t* pM, MsgVec_t* pVec )
{
	MsgVec_t*	aVec;
	int				i, l;

	if( pM->m_N >= pM->m_Size ) {
		l	= sizeof(MsgVec_t)*pM->m_Size*2;
		aVec	= (MsgVec_t*) (pM->m_fMalloc)( l );
		memset( aVec, 0, l );
		for( i = 0; i < pM->m_Size; i++ ) {
			aVec[i]	= pM->m_aVec[i];
		}
		pM->m_Size	*= 2;
		(pM->m_fFree)( pM->m_aVec );
		pM->m_aVec	= aVec;
	}
	pM->m_aVec[pM->m_N++]	= *pVec;
	return( 0 );
}

int
MsgInsert( Msg_t* pM, MsgVec_t* pVec )
{
	MsgVec_t*	aVec;
	int			i;
	int			l;
	int			s;

	if( pM->m_N >= pM->m_Size ) {
		s	= pM->m_Size*2;
		l	= sizeof(MsgVec_t)*s;
		aVec	= (MsgVec_t*) (pM->m_fMalloc)( l );
		memset( aVec, 0, l );
		for( i = 0; i < pM->m_N; i++ ) {
			aVec[i+1]	= pM->m_aVec[i];
		}
		(pM->m_fFree)( pM->m_aVec );
		pM->m_Size	= s;
		pM->m_aVec	= aVec;
	} else {
		for( i = pM->m_N; 0 < i; i-- ) {
			pM->m_aVec[i]	= pM->m_aVec[i-1];
		}
	}
	pM->m_aVec[0]	= *pVec;
	pM->m_N++;
	return( 0 );
}

int
MsgAddDel( Msg_t *pM, Msg_t *pAdd )
{
	int	i;
	MsgVec_t	*pVec;

	for( i = 0; i < pAdd->m_N; i++ ) {
		pVec	= &pAdd->m_aVec[i];
		if( pVec->v_Flags & VEC_DISABLED )	continue;

		MsgAdd( pM, pVec );
		pVec->v_Flags	&= ~VEC_MINE; 
	}
	MsgDestroy( pAdd );
	return( 0 );
}

int
MsgTotalLen( Msg_t *pM )
{
	int	i, Len;
	MsgVec_t	*pVec;

	for( Len = 0, i = 0; i < pM->m_N; i++ ) {
		pVec	= &pM->m_aVec[i];
		if( !(pVec->v_Flags & VEC_DISABLED) ) {
			Len	+= pM->m_aVec[i].v_Len;
		}
	}
	return( Len );
}

int
MsgCopyToBuf( Msg_t *pM, char *pBuf, size_t n )
{
	int	i, Len = 0, l;
	MsgVec_t	*pVec;

	for( Len = 0, i = 0; n > 0 && i < pM->m_N; i++ ) {
		pVec	= &pM->m_aVec[i];
		if( !(pVec->v_Flags & VEC_DISABLED) ) {
			l	= (int)( n < pVec->v_Len ? n : pVec->v_Len );
			memcpy( pBuf, pVec->v_pStart, l );
			pBuf	+= l;
			Len		+= l;
			n		-= l;
		}
	}
	return( Len );
}

char*
MsgFirst( Msg_t *pM )
{
	int	i;
	MsgVec_t	*pVec;

	for( i = 0; i< pM->m_N; i++ ) {
		pVec	= &pM->m_aVec[i];
		if( !(pVec->v_Flags & VEC_DISABLED) ) {
			pM->m_CurIndex	= i;
			return( pVec->v_pStart );
		}
	}
	pM->m_CurIndex	= -1;
	return( NULL );
}

char*
MsgNext( Msg_t *pM )
{
	int	i;
	MsgVec_t	*pVec;

	if( pM->m_CurIndex < 0 )	return( NULL );

	for( i = pM->m_CurIndex + 1; i < pM->m_N; i++ ) {
		pVec	= &pM->m_aVec[i];
		if( !(pVec->v_Flags & VEC_DISABLED) ) {
			pM->m_CurIndex	= i;
			return( pVec->v_pStart );
		}
	}
	pM->m_CurIndex	= -1;
	return( NULL );
}

int
MsgLen( Msg_t *pM )
{
	if( pM->m_CurIndex < 0 )	return( -1 );

	return( pM->m_aVec[pM->m_CurIndex].v_Len );
}

char*
MsgTrim( Msg_t *pM, int h )
{
	MsgVec_t	*pVec;

	if( pM->m_CurIndex < 0 )	return( NULL );

	pVec	= &pM->m_aVec[pM->m_CurIndex];
	if( pVec->v_Len > h ) {
		pVec->v_Len		-= h;
		pVec->v_pStart	+= h;
		return( pVec->v_pStart );
	} else {
		pVec->v_Flags	|= VEC_DISABLED;
		h	-= pVec->v_Len;
		if( MsgNext( pM ) ) {
			return( MsgTrim( pM, h ) );
		} else {
			return(NULL );
		}
	}
}

uint64_t
MsgCksum64( Msg_t *pM, int Start, int N, int *pR )
{
	int	i, r, l;
	MsgVec_t	*pVec;
	uint64_t	sum64, w64;
	unsigned char	*pStart;
	int			carry;
	int		End;

	if( N <=0 )	N = pM->m_N;
	End	= Start + N;
	if( End > pM->m_N )	End	= pM->m_N;

	sum64	= 0;
	r 		= 0;
	if( pR )	r = *pR;

	for( i = Start; i < End; i++ ) {
		pVec	= &pM->m_aVec[i];
		if( pVec->v_Flags & VEC_DISABLED )	continue;

		pStart	= (unsigned char*)pVec->v_pStart;
		l		= pVec->v_Len;

		w64		= in_sum64( pStart, l, r );
		sum64	+= w64;
		carry	= ( w64 > sum64 );
		sum64	+= carry;

		r = (l+r) % 8;
	}
	if( pR )	*pR	= r;
	return( ~sum64 );
}


int
MsgEngine( Msg_t *pM, MsgFunc_t fNext )
{
	MsgFunc_t	fFunc;

	pM->m_Err	= 0;
	do {
		fFunc	= fNext;
	} while( (fNext = (MsgFunc_t)(fFunc)( pM )) != NULL );

	if( pM->m_Err )	return( -1 );
	else	return( 0 );
}

typedef struct MsgFuncList {
	list_t		fl_Lnk;
	MsgFunc_t	fl_fFunc;
	void		*fl_pArg;
} MsgFuncList_t;

int
MsgPushFunc( Msg_t *pMsg, MsgFunc_t fFunc, void *pArg )
{
	MsgFuncList_t	*pFL;

	pFL	= (MsgFuncList_t*)(*pMsg->m_fMalloc)( sizeof(*pFL) );
	if( !pFL )	goto err;
	list_init( &pFL->fl_Lnk );
	pFL->fl_fFunc	= fFunc;
	pFL->fl_pArg	= pArg;

	list_add( &pFL->fl_Lnk, &pMsg->m_Lnk );
	return( 0 );
err:
	return( -1 );
}

MsgFunc_t
MsgPopFunc( Msg_t *pMsg )
{
	MsgFuncList_t	*pFL;
	MsgFunc_t		fFunc = NULL;

	pFL	= list_first_entry( &pMsg->m_Lnk, MsgFuncList_t, fl_Lnk );
	if( pFL ) {
		list_del( &pFL->fl_Lnk );
		fFunc	= pFL->fl_fFunc;
		MSG_FUNC_ARG( pMsg ) = pFL->fl_pArg;
		(*pMsg->m_fFree)( pFL );
	}
	return( fFunc );
}

#ifdef	ZZZ
#else
int
MsgSendByFd( int Fd, Msg_t *pMsg )
{
	int	i;
	MsgVec_t	*pVec;
	int	Ret;

/* through */
	for( i = 0; i < pMsg->m_N; i++ ) {
		pVec	= &pMsg->m_aVec[i];
		if( pVec->v_Flags & VEC_DISABLED )	continue;

//LOG(pLog, LogDBG, "[%s]v_Len=%d", pVec->v_pStart, pVec->v_Len );
		Ret = SendStream( Fd, pVec->v_pStart, pVec->v_Len );
		if( Ret < 0 )	goto err;
	}
	return( 0 );
err:
	return( -1 );
}
#endif

static	int
IOFileRead( void* This, void *pBuf, int Len )
{
	IOFile_t	*pIOFile = (IOFile_t*)This;
	char	*pC = (char*)pBuf;
	int	Ret;
	int		n = 0;

	while( Len > 0 ) {
		Ret = (int)fread( pC, 1, Len, pIOFile->f_pFILE );
		if( Ret == 0 ) {
			if( feof( pIOFile->f_pFILE ) )	return( 0 );
			else if( ferror( pIOFile->f_pFILE ) )	return( -1 );
		}
		pC	+= Ret;
		Len	-= Ret;
		n	+= Ret;
	}
	return( n );
}

static	int
IOFileWrite( void* This, void *pBuf, int Len )
{
	IOFile_t	*pIOFile = (IOFile_t*)This;
	char	*pC = (char*)pBuf;
	int		Ret;
	int		n = 0;

	while( Len > 0 ) {
		Ret = (int)fwrite( pC, 1, Len, pIOFile->f_pFILE );
		if( Ret <= 0 )	return( -1 );
		pC	+= Ret;
		Len	-= Ret;
		n	+= Ret;
	}
	return( n );
}

IOFile_t*
IOFileCreate(const char *pPath, int Flags, mode_t Mode,
				void*(*fMalloc)(size_t), void(*fFree)(void*) )
{
	FILE	*pFILE;
	IOFile_t	*pIOFile;

	if( Flags & O_RDWR ) {
		if( Flags & O_TRUNC )	pFILE = fopen( pPath, "w+" );
		else					pFILE = fopen( pPath, "a+" );
	} else if( Flags & O_WRONLY ) {
		if( Flags & O_TRUNC )	pFILE = fopen( pPath, "w" );
		else					pFILE = fopen( pPath, "a" );
	} else {
		pFILE	= fopen( pPath, "r" );
	}
	if( !pFILE )	return( NULL );
	pIOFile	= (IOFile_t*)(*fMalloc)( sizeof(*pIOFile) );
	if( !pIOFile ) {
		return( NULL );
	}
	memset( pIOFile, 0, sizeof(*pIOFile) );
	pIOFile->f_pFILE	= pFILE;
	pIOFile->f_fMalloc	= fMalloc;
	pIOFile->f_fFree	= fFree;
	pIOFile->f_ReadWrite.IO_Read	= IOFileRead;
	pIOFile->f_ReadWrite.IO_Write	= IOFileWrite;

	return( pIOFile );
}

#ifdef	VISUAL_CPP

/* Get _get_osfhandle.  */
//# include "msvc-nothrow.h"

int
fsync (int fd)
{
  HANDLE h = (HANDLE) _get_osfhandle (fd);
  DWORD err;

  if (h == INVALID_HANDLE_VALUE)
    {
      errno = EBADF;
      return -1;
    }

  if (!FlushFileBuffers (h))
    {
      /* Translate some Windows errors into rough approximations of Unix
       * errors.  MSDN is useless as usual - in this case it doesn't
       * document the full range of errors.
       */
      err = GetLastError ();
      switch (err)
        {
        case ERROR_ACCESS_DENIED:
          /* For a read-only handle, fsync should succeed, even though we have
             no way to sync the access-time changes.  */
          return 0;

          /* eg. Trying to fsync a tty. */
        case ERROR_INVALID_HANDLE:
          errno = EINVAL;
          break;

        default:
          errno = EIO;
        }
      return -1;
    }

  return 0;
}

#endif

int
IOFileDestroy( IOFile_t *pIOFile, bool_t bSync )
{
	if( bSync )	fsync( fileno(pIOFile->f_pFILE) );
	fclose( pIOFile->f_pFILE );
	(pIOFile->f_fFree)( pIOFile );
	return( 0 );
}

static	int
IOSocketRead( void* This, void *pBuf, int Len )
{
	IOSocket_t	*pR	= (IOSocket_t*)This;
	int	Ret;

	Ret = RecvStream( pR->s_Fd, (char*)pBuf, Len );
	if( !Ret )	return( Len );
	else		return( Ret );
}

static	int
IOSocketWrite( void* This, void *pBuf, int Len )
{
	IOSocket_t	*pW	= (IOSocket_t*)This;
	int	Ret;

	Ret = SendStream( pW->s_Fd, (char*)pBuf, Len );
	if( !Ret )	return( Len );
	else		return( Ret );
}

int
IOSocketBind( IOSocket_t *pIO, int fd )
{
	memset( pIO, 0, sizeof(*pIO) );
	pIO->s_ReadWrite.IO_Read	= IOSocketRead;
	pIO->s_ReadWrite.IO_Write	= IOSocketWrite;

	pIO->s_Fd	= fd;
	return( 0 );
}

char *
getHostName(char *pCellName,int w,int no)
{
	static char server[1024];
	char fmt[256];
	char *ret = NULL;
	FILE *fp = NULL;
	// ENV
	sprintf(fmt,"%s%%0%uu",pCellName,w);
	sprintf(server,fmt,no);
	if ((ret = getenv(server)) != NULL ) {
		goto ret;
	}
	// config file
	if ((ret = getenv("HOME")) != NULL) {
		sprintf(server,"%s/.%s.conf",ret,pCellName);
		fp = fopen(server,"r");
	}
	if ( fp == NULL ) {
		sprintf(server,"/etc/%s/server.conf",pCellName);
		fp = fopen(server,"r");
		if ( fp == NULL ) {
			ret = NULL;
			goto ret;
		}
	}
	while ((ret = fgets(server,sizeof(server),fp) ) != NULL) {
		int n;
		if ( *ret == '\r' || *ret == '\n' || *ret == '#' ) {
			continue;
		}
		if ( !isdigit((int)ret[0]) ) {
			continue;
		}
		n = strtoul(ret,&ret,10);
		if ( n != no ) {
			continue;
		}
		while(isblank((int)*ret)) ret++;
		if ( *ret == '\r' || *ret == '\n' ) {
			continue;
		}
		if ( isalnum((int)*ret ) ) {
			char *p = ret;
			while(!isspace((int)*p)) p++;
			*p ='\0';
			break;
		}
	}
 ret:
	if( fp )	fclose( fp );
	return ret;
}

int
GElmCtrlInit( GElmCtrl_t *pGE, void*pTag,
		GElm_t*(*_fAlloc)(GElmCtrl_t*),
		int(*_fFree)(GElmCtrl_t*,GElm_t*pKey),
		int(*_fSet)(GElmCtrl_t*,
					GElm_t*pElm,void*pKey,void**ppKey,void*,bool_t),
														// return *ppKey in Elm
		int(*_fUnset)(GElmCtrl_t*,GElm_t*pElm,void**ppKey,bool_t),	
														// return *ppKey
		int(*_fLoop)(GElmCtrl_t*,GElm_t*pElm, void*),	// return break loop
		int	MaxCnt,

		int HashSize, 				// 素数
		int(*fHashCode)(void*), int(*fCompare)(void*,void*),
		void(*fPrint)(void*,void*),
		void*(*fMalloc)(size_t), void(*fFree)(void*),
		void(*fDestroy)(Hash_t*,void*) )
{
	memset( pGE, 0, sizeof(*pGE) );

	MtxInitRecursive( &pGE->ge_Mtx );
	CndInit( &pGE->ge_Cnd );
	list_init( &pGE->ge_Lnk );

	HashInit( &pGE->ge_Hash, HashSize, fHashCode, fCompare,
					fPrint, fMalloc, fFree, fDestroy );

	pGE->ge_MaxCnt	= MaxCnt;
	pGE->ge_pTag	= pTag;
	pGE->ge_fAlloc	= _fAlloc;
	pGE->ge_fFree	= _fFree;
	pGE->ge_fSet	= _fSet;
	pGE->ge_fUnset	= _fUnset;
	pGE->ge_fLoop	= _fLoop;

	return( 0 );
}

GElm_t*
_GElmAlloc( GElmCtrl_t *pGE )
{
	GElm_t	*pElm;

	pElm = (*pGE->ge_fAlloc)( pGE );
	ASSERT( pElm );
	GElmInit( pElm );
	pGE->ge_Cnt++;
	return( pElm );
}

void
_GElmFree( GElmCtrl_t *pGE, GElm_t *pElm )
{
	(*pGE->ge_fFree)( pGE, pElm );
	pGE->ge_Cnt--;
}

int
_GElmSet( GElmCtrl_t *pGE, GElm_t *pElm, void *pKey, void *pArg, bool_t bLoad )
{
	void	*pKeyInElm;
	int		Ret;

	Ret = (*pGE->ge_fSet)( pGE, pElm, pKey, &pKeyInElm, pArg, bLoad );
	if( Ret )	goto ret;

	list_add_tail( &pElm->e_Lnk, &pGE->ge_Lnk );
	HashPut( &pGE->ge_Hash, pKeyInElm, pElm );
ret:
	return( Ret );
}

int
_GElmUnset( GElmCtrl_t *pGE, GElm_t *pElm, bool_t bSave )
{
	void	*pOldKey;
	int		Ret;

	Ret = (*pGE->ge_fUnset)( pGE, pElm, &pOldKey, bSave );
	if( Ret )	goto ret;

	if( HashRemove( &pGE->ge_Hash, pOldKey ) ) {
		list_del_init( &pElm->e_Lnk );
		return( 0 );
	} else {
		return( -1 );
	}
ret:
	return( Ret );
}

GElm_t*
GElmGet( GElmCtrl_t *pGE, void *pKey, void *pArg, bool_t bAlloc, bool_t bLoad )
{
	GElm_t	*pElm;
	int		Ret;

	MtxLock( &pGE->ge_Mtx );

	pElm	= (GElm_t*)HashGet( &pGE->ge_Hash, pKey );
	if( !pElm && bAlloc ) {
		if( pGE->ge_MaxCnt == 0 ||  pGE->ge_Cnt < pGE->ge_MaxCnt ) {
			pElm	= _GElmAlloc( pGE );
		} else {
loop_wait:
			pElm = list_first_entry( &pGE->ge_Lnk, GElm_t, e_Lnk );
			ASSERT( pElm->e_Cnt >= 0 );
			if( pElm->e_Cnt ) {
				pGE->ge_Wait++;
				CndWait( &pGE->ge_Cnd, &pGE->ge_Mtx );
				pGE->ge_Wait--;
				goto loop_wait;
			}
			Ret = _GElmUnset( pGE, pElm, bLoad );
			if( Ret )	goto err;
		}
		pElm->e_Cnt = 0;
		Ret = _GElmSet( pGE, pElm, pKey, pArg, bLoad );
		if( Ret ) {
			_GElmFree( pGE, pElm );
			goto err;
		}
	}
	if( pElm ) {
		pElm->e_Cnt++;
		list_del_init( &pElm->e_Lnk );
		list_add_tail( &pElm->e_Lnk, &pGE->ge_Lnk );
	}
	MtxUnlock( &pGE->ge_Mtx );
	return( pElm );
err:
	MtxUnlock( &pGE->ge_Mtx );
	return( NULL );
}
 
int
GElmPut( GElmCtrl_t *pGE, GElm_t *pElm, bool_t bFree, bool_t bSave )
{
	int	Ret = 0;

//DBG_PRT("pElm=%p\n", pElm );
	MtxLock( &pGE->ge_Mtx );

	--pElm->e_Cnt;
	ASSERT( pElm->e_Cnt >= 0 );
	if( pElm->e_Cnt == 0 ) {
		if( bFree ) {
			Ret = _GElmUnset( pGE, pElm, bSave );
			if( Ret )	goto ret;
			_GElmFree( pGE, pElm );
		}
		if( pGE->ge_Wait )	CndBroadcast( &pGE->ge_Cnd );
	}
ret:
	MtxUnlock( &pGE->ge_Mtx );
	return( Ret );
}

int
GElmLoop( GElmCtrl_t *pGE, list_t *pHead, int off, void *pVoid )
{
	list_t	*pE, *pW;
	GElm_t	*pElm;
	int		Ret = 0;
	list_t	*pEntry;

	if( pHead )	pEntry = pHead;
	else		pEntry = &pGE->ge_Lnk;

	MtxLock( &pGE->ge_Mtx );

	if( pGE->ge_fLoop ) {
		list_for_each_safe( pE, pW, pEntry  ) {

			pElm = (GElm_t*)((char*)pE + off );

			if( (Ret = (*pGE->ge_fLoop)( pGE, pElm, pVoid )) != 0 )	break;
		} 
	}
	MtxUnlock( &pGE->ge_Mtx );
	return( Ret );
}

#ifdef	ZZZ
#else
int
GElmFlush( GElmCtrl_t *pGE, list_t *pHead, int off, void *pVoid, bool_t bSave )
{
	list_t	*pE, *pW;
	GElm_t	*pElm;
	int		Ret = 0;
	list_t	*pEntry;

	if( pHead )	pEntry = pHead;
	else		pEntry = &pGE->ge_Lnk;

	MtxLock( &pGE->ge_Mtx );

	list_for_each_safe( pE, pW, pEntry  ) {

		pElm = (GElm_t*)((char*)pE + off );

again:	// I'm the last.
		if( pElm->e_Cnt ) {
			pGE->ge_Wait++;
			CndWait( &pGE->ge_Cnd, &pGE->ge_Mtx );
			pGE->ge_Wait--;
			goto again;
		}
		if( pGE->ge_fLoop ) {
			if( (Ret = (*pGE->ge_fLoop)( pGE, pElm, pVoid )) != 0 )	break;
		} 
		Ret = _GElmUnset( pGE, pElm, TRUE );
		if( Ret )	goto err;
		_GElmFree( pGE, pElm );
	}
err:
	MtxUnlock( &pGE->ge_Mtx );
	return( Ret );
}
#endif
/*
 *	Fdevent
 */
#ifdef	_NOT_LIBEVENT_

int
FdEventCtrlCreate( FdEventCtrl_t *pFdEvCt )
{
	int	FdPoll;
#define	NWG_EVENT_MAX	1000

	FdPoll = epoll_create( NWG_EVENT_MAX );
	if( FdPoll < 0 )	goto err1;

	pFdEvCt->ct_EpollFd = FdPoll;
	MtxInitRecursive( &pFdEvCt->ct_Mtx );
	HashInit( &pFdEvCt->ct_Hash, PRIMARY_101, HASH_CODE_U64, HASH_CMP_U64,
					NULL, Malloc, Free, NULL );
	list_init( &pFdEvCt->ct_Lnk );
	pFdEvCt->ct_Cnt = 0;
	CndInit( &pFdEvCt->ct_Cnd );

	return( 0 );
err1:
	return( -1 );
}

int
FdEventCtrlDestroy( FdEventCtrl_t *pFdEvCt )
{
	close( pFdEvCt->ct_EpollFd );
	MtxDestroy( &pFdEvCt->ct_Mtx );
	return( 0 );
}

int
FdEventInit( FdEvent_t *pEv, int Type, int Fd, int Events,
				void *pArg, FdEventHandler_t fHandler )
{
	memset( pEv, 0, sizeof(*pEv) );
	list_init( &pEv->fd_Lnk );
	pEv->fd_Type		= Type;
	pEv->fd_Fd			= Fd;
	pEv->fd_Events		= Events;
	pEv->fd_pArg		= pArg;
	pEv->fd_fHandler	= fHandler;

	return( 0 );
}

int
FdEventAdd( FdEventCtrl_t *pFdEvCt, uint64_t Key64, FdEvent_t *pEv )
{
	pEv->fd_pFdEvCt			= pFdEvCt;

	pEv->fd_ev.events	= pEv->fd_Events;
	pEv->fd_ev.data.u64	= Key64;

	MtxLock( &pFdEvCt->ct_Mtx );

	if( epoll_ctl( pFdEvCt->ct_EpollFd, EPOLL_CTL_ADD,
					pEv->fd_Fd, &pEv->fd_ev ) < 0 ) {
//printf("pollFd=%d fd=%d\n", pFdEvCt->ct_EpollFd, pEv->fd_Fd );
		pEv->fd_errno	= errno;
		goto err1;
	}
	ASSERT( !HashGet( &pFdEvCt->ct_Hash, &pEv->fd_ev.data.u64) );
	list_add_tail( &pEv->fd_Lnk, &pFdEvCt->ct_Lnk );
	HashPut( &pFdEvCt->ct_Hash, &pEv->fd_ev.data.u64, pEv );

	pFdEvCt->ct_Cnt++;

	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}

int
FdEventMod( FdEvent_t *pEv )
{
	FdEventCtrl_t	*pFdEvCt;

	pFdEvCt	= pEv->fd_pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	if( epoll_ctl( pFdEvCt->ct_EpollFd, EPOLL_CTL_MOD,
					pEv->fd_Fd, &pEv->fd_ev ) < 0 ) {
		pEv->fd_errno	= errno;
		goto err1;
	}
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}

int
FdEventDel( FdEvent_t *pEv )
{
	FdEventCtrl_t	*pFdEvCt;

	pFdEvCt	= pEv->fd_pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	if( epoll_ctl( pFdEvCt->ct_EpollFd, EPOLL_CTL_DEL,
					pEv->fd_Fd, NULL ) < 0 ) {
		pEv->fd_errno	= errno;
		goto err1;
	}
	list_del_init( &pEv->fd_Lnk );
	HashRemove( &pFdEvCt->ct_Hash, &pEv->fd_ev.data.u64 );

	pFdEvCt->ct_Cnt--;

	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}

int
FdEventSuspend( FdEventCtrl_t *pFdEvCt )
{
	FdEvent_t	*pEv;
	struct epoll_event	ev;

	memset( &ev, 0, sizeof(ev) );

	MtxLock( &pFdEvCt->ct_Mtx );

	list_for_each_entry( pEv, &pFdEvCt->ct_Lnk, fd_Lnk ) {

		if( epoll_ctl(pFdEvCt->ct_EpollFd, EPOLL_CTL_MOD, pEv->fd_Fd, &ev) ) {
			pEv->fd_errno	= errno;
			(*pEv->fd_fHandler)( pEv, EV_SUSPEND );
		}
	}
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
}

int
FdEventResume( FdEventCtrl_t *pFdEvCt, FdEvent_t *pEv1 )
{
	FdEvent_t	*pEv = NULL;
	
	MtxLock( &pFdEvCt->ct_Mtx );

	if( pEv1 ) {

		if(epoll_ctl(pFdEvCt->ct_EpollFd,EPOLL_CTL_MOD,
						pEv1->fd_Fd, &pEv1->fd_ev)) {
			pEv1->fd_errno	= errno;
			(*pEv->fd_fHandler)( pEv1, EV_RESUME );
		}
	} else {
		list_for_each_entry( pEv, &pFdEvCt->ct_Lnk, fd_Lnk ) {

			if(epoll_ctl(pFdEvCt->ct_EpollFd,EPOLL_CTL_MOD,
						pEv->fd_Fd, &pEv->fd_ev)) {
				pEv->fd_errno	= errno;
				(*pEv->fd_fHandler)( pEv, EV_RESUME );
			}
		}
	}
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
}

int
FdEventLoop( FdEventCtrl_t *pFdEvCt, int msec )
{
	struct epoll_event	evs[FD_EVENT_MAX];
	int		n, i;
	FdEvent_t	*pEv;

	while( 1 ) {
		n = epoll_wait( pFdEvCt->ct_EpollFd, evs, FD_EVENT_MAX, msec );
		if( n < 0 ) {
			if( errno == EINTR )	continue;
			goto err;
		}
		for( i = 0; i < n; i++ ) {

			MtxLock( &pFdEvCt->ct_Mtx );

			pEv = (FdEvent_t*)HashGet(&pFdEvCt->ct_Hash, &evs[i].data.u64);

			MtxUnlock( &pFdEvCt->ct_Mtx );

			if( !pEv ) {
				errno	= ENOENT;
				goto err;
			}
			ASSERT( pEv->fd_ev.data.u64 == evs[i].data.u64 );
			pEv->fd_errno	= 0;
			if( (*pEv->fd_fHandler)( pEv, EV_LOOP ) )	goto err;

		}
	}
	return( 0 );
err:
	close( pFdEvCt->ct_EpollFd );
	return( -1 );
}

#else	// _NOT_LIBEVENT__

static void
_cb_func( evutil_socket_t Fd, short what, void *pArg )
{
	short	*pWhat = (short*)pArg;

	*pWhat	= what;
}

/*
 *	Ret = 0	 : data event
 *		= -1 : timeout event
 */
int
FdEventWait( int Fd, struct timeval *pTimeout )
{
	struct event_base *ev_base;
	struct event *ev;
	int		Flag;
	int		Ret;
	short	What;


	if( pTimeout ) 	Flag = EV_READ|EV_TIMEOUT;
	else		 	Flag = EV_READ;

	ev_base = event_base_new();
	ev = event_new(ev_base, Fd, Flag, _cb_func, &What );

	event_add(ev, pTimeout );
	event_base_dispatch(ev_base);

	if( What & EV_READ )	Ret = 0;
	else					Ret = -1;

	event_free(ev);
	event_base_free(ev_base);

	return( Ret ) ;
}

int
FdEventCtrlCreate( FdEventCtrl_t *pFdEvCt )
{
	struct event_base	*pEvBase;
#define	NWG_EVENT_MAX	1000

	pEvBase	= event_base_new();
	if( pEvBase == NULL )	goto err1;

	pFdEvCt->ct_pEvBase	= pEvBase;

	MtxInitRecursive( &pFdEvCt->ct_Mtx );

	HashListInit( &pFdEvCt->ct_HashLnk, 
					PRIMARY_101, HASH_CODE_U64, HASH_CMP_U64,
					NULL, Malloc, Free, NULL );

	pFdEvCt->ct_Cnt = 0;

	return( 0 );
err1:
	return( -1 );
}

int
FdEventCtrlDestroy( FdEventCtrl_t *pFdEvCt )
{
	event_base_free( pFdEvCt->ct_pEvBase );
	pFdEvCt->ct_pEvBase = NULL;
	HashListDestroy( &pFdEvCt->ct_HashLnk );
	MtxDestroy( &pFdEvCt->ct_Mtx );
	return( 0 );
}

void
_FdEvHandler( evutil_socket_t fd, short Events, void *pArg )
{
	FdEvent_t *pFdEv = (FdEvent_t*)pArg;

	(*pFdEv->fd_fHandler)( pFdEv, EV_LOOP );
}

int
FdEventInit( FdEvent_t *pEv, int Type, int Fd, int Events,
				void *pArg, FdEventHandler_t fHandler )
{
	memset( pEv, 0, sizeof(*pEv) );
	list_init( &pEv->fd_Lnk );
	pEv->fd_Type		= Type;
	pEv->fd_Fd			= Fd;
	pEv->fd_Events		= Events;
	pEv->fd_pArg		= pArg;
	pEv->fd_fHandler	= fHandler;

	return( 0 );
}

int
FdEventAdd( FdEventCtrl_t *pFdEvCt, uint64_t Key64, FdEvent_t *pEv )
{
	pEv->fd_pFdEvCt			= pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	ASSERT( !HashListGet( &pFdEvCt->ct_HashLnk, &Key64) );

	event_assign( &pEv->fd_Event, pFdEvCt->ct_pEvBase, 
				pEv->fd_Fd, pEv->fd_Events|EV_PERSIST, 
				_FdEvHandler, pEv );
	if( event_add( &pEv->fd_Event , NULL ) ) {
		pEv->fd_errno	= errno;
		goto err1;
	}

	pEv->fd_Key64	= Key64;
	HashListPut( &pFdEvCt->ct_HashLnk, 
			&pEv->fd_Key64, pEv, fd_Lnk );

	pFdEvCt->ct_Cnt++;
	CndSignal( &pFdEvCt->ct_Cnd );

	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}

#ifdef	ZZZ
int
FdEventMod( FdEvent_t *pEv )
{
	FdEventCtrl_t	*pFdEvCt;

	pFdEvCt	= pEv->fd_pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	if( event_del( &pEv->fd_Event ) < 0 ) {
		pEv->fd_errno	= errno;
		goto err1;
	}
	event_assign( &pEv->fd_Event, pFdEvCt->ct_pEvBase, 
				pEv->fd_Fd, pEv->fd_Events|EV_PERSIST, 
				_FdEvHandler, pEv );
	if( event_add( &pEv->fd_Event , NULL ) ) {
		pEv->fd_errno	= errno;
		goto err1;
	}

	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}
#endif

int
FdEventDel( FdEvent_t *pEv )
{
	FdEventCtrl_t	*pFdEvCt;

	pFdEvCt	= pEv->fd_pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	if( event_del( &pEv->fd_Event ) < 0 ) {
		pEv->fd_errno	= errno;
		goto err1;
	}
	HashListDel( &pFdEvCt->ct_HashLnk, &pEv->fd_Key64,
							FdEvent_t*, fd_Lnk );

	pFdEvCt->ct_Cnt--;

	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
err1:
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( -1 );
}

int
FdEventSuspend( FdEventCtrl_t *pFdEvCt )
{
	FdEvent_t	*pFdEvent;

	MtxLock( &pFdEvCt->ct_Mtx );

	list_for_each_entry( pFdEvent, &pFdEvCt->ct_HashLnk.hl_Lnk, fd_Lnk ) {

		if( event_del( &pFdEvent->fd_Event ) ) {
			pFdEvent->fd_errno	= errno;
			(*pFdEvent->fd_fHandler)( pFdEvent, EV_SUSPEND );
		}
	}
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
}

int
FdEventResume( FdEventCtrl_t *pFdEvCt, FdEvent_t *pEv1 )
{
	FdEvent_t		*pFdEvent;
	
	MtxLock( &pFdEvCt->ct_Mtx );

	if( pEv1 ) {
		if( event_add( &pEv1->fd_Event, NULL ) < 0 ) {
			pEv1->fd_errno	= errno;
			(*pEv1->fd_fHandler)( pEv1, EV_RESUME );
		}
	} else {

		list_for_each_entry( pFdEvent, &pFdEvCt->ct_HashLnk.hl_Lnk, fd_Lnk ) {

			if( event_add( &pFdEvent->fd_Event, NULL ) ) {
				pFdEvent->fd_errno	= errno;
				(*pFdEvent->fd_fHandler)( pFdEvent, EV_RESUME );
			}
		}
	}
	MtxUnlock( &pFdEvCt->ct_Mtx );
	return( 0 );
}

int
FdEventLoop( FdEventCtrl_t *pFdEvCt, int msec )
{
	struct timeval TV;
	int	Ret;
	bool_t	bFOREVER = TRUE;

	if( msec != FOREVER ) {	// set timeout
		TV.tv_sec	= msec/1000;
		TV.tv_usec	= (msec%1000)*1000;
		Ret = event_base_loopexit( pFdEvCt->ct_pEvBase, &TV );
		if( Ret < 0 ) {
			goto err;
		}
		bFOREVER = FALSE;
	}
	do {
		MtxLock( &pFdEvCt->ct_Mtx );
		while (pFdEvCt->ct_Cnt == 0) {
			CndWait( &pFdEvCt->ct_Cnd, &pFdEvCt->ct_Mtx );
		}
		MtxUnlock( &pFdEvCt->ct_Mtx );
		Ret = event_base_dispatch( pFdEvCt->ct_pEvBase );
		if( Ret < 0 ) {
			if( errno == EINTR )	continue;
			goto err;
		}
	} while( bFOREVER );
	return( 0 );
err:
	return( -1 );
}

#ifdef	ZZZ
/* Synchronous timeout read */
int
FdEventReadWithTimeout( FdEventCtrl_t *pFdEvCt, FdEvent_t FdEvent,
		struct timeval *pTimeout )
{
	pEv->fd_pFdEvCt			= pFdEvCt;

	MtxLock( &pFdEvCt->ct_Mtx );

	ASSERT( !HashListGet( &pFdEvCt->ct_HashLnk, &Key64) );

	event_assign( &pEv->fd_Event, pFdEvCt->ct_pEvBase, 
				pEv->fd_Fd, pEv->fd_Events, 
				_FdEvHandler, pEv );
	if( event_add( &pEv->fd_Event , pTimeout ) ) {
		pEv->fd_errno	= errno;
		goto err1;
	}

	MtxUnlock( &pFdEvCt->ct_Mtx );
}
#endif

#endif	// _NOT_LIBEVENT__


/***
#include	<stdint.h>
#include	<stdlib.h>
#include	<assert.h>
#include	"in_cksum.h"
***/

/*
 *	Byte order independent because of 1's complement
 *	Algorithm:
 *		Let 0 <= a(MSB=m-1) <= 2**m-1, 0 <= b(MSB=n-1) <= 2**n-1 
 *			then 0 <= a+b <= 2**m+2**n-2 ( m >= n )
 *		case no-carry
 *			a+b = c < 2**m	-> a,b <= c -> a-c <= 0, b-c <= 0
 *		case carry
 *			a+b = 2**m + c	-> a-c > 2**m - b > 0
 *					-> b-c > 2**m - a > 0
 *			2**m + c <= 2**m+2**n-2
 *			c <= 2**n-2 -> no more carry with 1 added
 *
 *	Carry の判断は、(a,b)>cでOK.
 *	キャリー発生後の足し込みはキャリーを発生しないので安心。
 *	減算は、否定をキャリー付きで足す。
 *	  
 */
uint64_t
in_sum64( void *pAddr, int nleft, int r )
{
	long		bound;
	char		*pw = (char*)pAddr;
	int			carry = 0;

	uint64_t 	sum64 = 0L;
	uint64_t	w64, c64;
	int			r1;

	ASSERT( 0 <= r && r < 8 );

	if( r ) {
		c64	= 0;
		r1	= 8 - r;
		if( r1 > nleft ) {
			r1	= nleft;
		}
		memcpy( (char*)&c64 + r, pw, r1 );
		nleft	-= r1;
		pw		+= r1;
		sum64	= *(uint64_t*)&c64;
	}
#ifdef	ZZZ
	bound	= pw - (char*)0;
#else
	bound	= (long)(uintptr_t)pw;
#endif
	while( nleft > 0 ) {

		if( nleft >= 8) {

			if( !(bound & 0x3 /*0x7*/) )	w64	= *(uint64_t*)pw;
			else {
				c64		= 0L;
 				memcpy( &c64, pw, 8 );
				w64	= *(uint64_t*)&c64;
			}
			pw		+= 8;
			bound	+= 8;
			nleft	-= 8;
		} else {
			c64	= 0;
			memcpy( &c64, pw, nleft );
			w64	= *(uint64_t*)&c64;

			nleft	= 0;
		}
		sum64	+= w64;
		carry	= ( w64 > sum64 );
		sum64	+= carry;
	}	
	return( sum64 );
}

uint64_t
in_cksum64( void *pAddr, int nleft, int r )
{
	uint64_t	sum64;

	sum64	= in_sum64( pAddr, nleft, r );
	return( ~sum64 );
}

uint16_t
in_cksum( uint16_t *pAddr, int nleft )
{
#ifdef	ZZZ
	long		bound = ((char*)pAddr-(char*)0);
#else
	long		bound = (long)(uintptr_t)pAddr;
#endif
	char		*pw = (char*)pAddr;
	uint32_t	sum32 = 0;
	uint32_t	w32 = 0;
	int			carry = 0;
	uint16_t	answer;
	
	assert( !(bound & 1 ) );

	if( 8 == sizeof(long) ) {
		uint64_t 	sum64 = 0L;
		uint64_t	w64;

		while( nleft > 1 ) {

			sum64	+= carry;

			if(!(bound & 0x7) && nleft >= 8) {
				w64	= *(uint64_t*)pw;
				sum64	+= w64;
				carry	= ( w64 > sum64 );
				pw	+= 8;
				bound	+= 8;
				nleft	-= 8;
			} else if( !(bound & 0x3) && nleft >= 4 ) {
				w64	= *(uint32_t*)pw;
				sum64	+= w64;
				carry	= ( w64 > sum64 );
				pw	+= 4;
				bound	+= 4;
				nleft	-= 4;
			} else if( !(bound & 0x1) && nleft >= 2 ) {
				w64	= *(uint16_t*)pw;
				sum64	+= w64;
				carry	= ( w64 > sum64 );
				pw	+= 2;
				bound	+= 2;
				nleft	-= 2;
			}
		}	
		sum64	+= carry;
		sum64 	= (sum64>>32) + (sum64&0xffffffffL);
		sum64 	+= (sum64>>32);
		sum32	= (uint32_t)sum64;
	} else {
		while( nleft > 1 ) {
			sum32	+= carry;

			if( !(bound & 0x3) && nleft >= 4 ) {
				w32	= *(uint32_t*)pw;
				sum32	+= w32;
				carry	= ( w32 > sum32 );
				pw	+= 4;
				bound	+= 4;
				nleft	-= 4;
			} else if( !(bound & 0x1) && nleft >= 2 ) {
				w32	= *(uint16_t*)pw;
				sum32	+= w32;
				carry	= ( w32 > sum32 );
				pw	+= 2;
				bound	+= 2;
				nleft	-= 2;
			}

		}
		sum32	+= carry;
	}
	if( nleft == 1 ) {
		w32	= *(unsigned char*)pw;
		sum32	+= w32;
		if( w32 > sum32 )	sum32 += 1;
	}
	sum32 = (sum32>>16) + (sum32&0xffff);
	sum32 += (sum32>>16);
	answer = ~sum32;
	return( answer );
}

/*
 *	Old part is removed and new part is added.
 *		RFC1624 ???
 */
uint64_t
in_cksum64_sub_add( uint64_t cksum, uint64_t sub, uint64_t add )
{
	uint64_t	sum;
	int			carry;

	sum	= ~cksum;

	sum		+= sub;
	carry	= ( sub > sum );
	sum		+= carry;

	sum		+= ~add;
	carry	= ( ~add > sum );
	sum		+= carry;

	return( ~sum );
} 

uint64_t
in_cksum64_update( uint64_t cksum, 
					void *op, int ol, int or, 
					void *np, int nl, int nr )
{
	uint64_t	osum, nsum;
	uint64_t	sum;
	int			carry;

	sum	= ~cksum;
	if( ol ) {
		osum 	= in_sum64( op, ol, or );
		sum		+= ~osum;
		carry	= ( ~osum > sum );
		sum		+= carry;
	}
	if( nl ) {
		nsum 	= in_sum64( np, nl, nr );
		sum		+= nsum;
		carry	= ( nsum > sum );
		sum		+= carry;
	}
	return( ~sum );
}

unsigned short
in_cksum_update( unsigned short cksum, unsigned short *op, int ol,
			unsigned short *np, int nl )
{
	unsigned short	osum, nsum;
	int		sum;
	unsigned short	answer;

	sum	= (~cksum & 0xffff);
	if( ol ) {
		osum 	= in_cksum( op, ol );
		sum	-= (~osum & 0xffff );
	}
	if( nl ) {
		nsum 	= in_cksum( np, nl );
		sum	+= (~nsum & 0xffff );
	}
	sum = (sum>>16) + (sum&0xffff);
	sum += (sum>>16);
	answer = ~sum;
	return( answer );
}


/*
 *	pseudo header update
 */
unsigned short
in_cksum_pseudo( unsigned short o_cksum, 
		unsigned int o_src, unsigned int o_dst,
		unsigned int n_src, unsigned int n_dst, 
		unsigned short t_len, unsigned char protocol )
/*
 *	All is network order.
 */
{
	unsigned short	cksum;
	pseudo_hd_t	old, New;
	int		ol = 0, nl =0;

	if( o_src ) {
		old.src_ip	= o_src;
		old.dst_ip	= o_dst;
		old.reserve	= 0;
		old.protocol	= protocol;
		old.tot_len	= t_len;
		ol		= sizeof(old);
	}
	if( n_src ) {
		New.src_ip	= n_src;
		New.dst_ip	= n_dst;
		New.reserve	= 0;
		New.protocol	= protocol;
		New.tot_len	= t_len;
		nl		= sizeof(New);
	}
	cksum = in_cksum_update( o_cksum, 
			(unsigned short*)&old, ol, (unsigned short*)&New, nl );
	return( cksum );
}
/***
short	buff1[] ={ 1, 2, 3, 4, 5, 0};

int
main(int ac, char *av[] )
{
	unsigned	short a, b, c, d;

	a = in_cksum( (unsigned short*)&buff1[0], 10 );
	b = in_cksum( (unsigned short*)&buff1[1], 8 );
	c = in_cksum_update( b, 0, 0, (unsigned short*)&buff1[0], 2 );
	d = in_cksum_update( a, (unsigned short*)&buff1[0], 2, 0, 0 );

	printf("a[0x%x] b[0x%x] c[0x%x] d[0x%x]\n", a, b, c, d );
	buff1[5] = a;
	a = in_cksum( (unsigned short*)&buff1[0], 12 );
	printf("a[0x%x]\n", a );
	return( 0 );
}
***/
		
#ifdef	VISUAL_CPP

#define EPOCHFILETIME (116444736000000000i64)

int 
gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        tagFileTime;
    LARGE_INTEGER   largeInt;
    __int64         val64;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&tagFileTime);

        largeInt.LowPart  = tagFileTime.dwLowDateTime;
        largeInt.HighPart = tagFileTime.dwHighDateTime;
        val64 = largeInt.QuadPart;
        val64 = val64 - EPOCHFILETIME;
        val64 = val64 / 10;
        tv->tv_sec  = (long)(val64 / 1000000);
        tv->tv_usec = (long)(val64 % 1000000);
    }

    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }

        //Visual C++ 6.0でＯＫだった・・
        //tz->tz_minuteswest = _timezone / 60;
        //tz->tz_dsttime = _daylight;

        long _Timezone = 0;
         _get_timezone(&_Timezone);
         tz->tz_minuteswest = _Timezone / 60;
         
         int _Daylight = 0;
         _get_daylight(&_Daylight);
         tz->tz_dsttime = _Daylight;
    }

    return 0;
}

int
WinInit( void )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD(2, 2);
 
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
	/*
     * 使用可能なWinSock DLLが見つからなかった事をユーザに伝える。
	 */
   	 return -1;
	}
	return( 0 );
}

#endif

