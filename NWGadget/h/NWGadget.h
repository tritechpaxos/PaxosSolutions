/******************************************************************************
*
*  NWGadget.h 	--- header of Gadget Library
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
#ifndef	_NWGADGET_
#define	_NWGADGET_

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<time.h>
#ifndef	VISUAL_CPP
#include	<sys/time.h>
#include	<stdlib.h>
#endif
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<assert.h>
#include	<pthread.h>
#include	<signal.h>
#ifdef	ZZZ
#else
#include	<wchar.h>
#endif

#ifdef	VISUAL_CPP
#include	<winsock2.h>
#include	<windows.h>
#include	<io.h>
#define	SIGKILL	9

#define	sleep( sec )	Sleep((sec)*1000 )

#define	socklen_t	int

int	WinInit(void);

#else

#include	<sys/socket.h>
#include	<netinet/ip.h>
#include	<netinet/tcp.h>
#include	<arpa/inet.h>
#include	<unistd.h>
#include	<sys/un.h>
#include	<unistd.h>
#endif

#include	<inttypes.h>

#ifdef	_NOT_LIBEVENT_

#include	<sys/epoll.h>
#define	FD_EVENT_READ	EPOLLIN

#else

#include	<event.h>
#define	FD_EVENT_READ	EV_READ

#endif

/*
 *	Types
 */
typedef	unsigned char		uchar_t;

#ifndef __int8_t_defined
#ifndef	VISUAL_CPP
typedef	long 			int32_t;
typedef	unsigned long		uint32_t;
#endif
typedef	long long		int64_t;
typedef	unsigned long long	uint64_t;
#endif

#ifdef	ZZZ
#define	PNT_UINT32( pnt )	(uint32_t)((void*)(pnt)-(void*)0)
#define	PNT_INT32( pnt )	(int32_t)((void*)(pnt)-(void*)0)
#else
#define	PNT_UINT32( pnt )	(uint32_t)(uintptr_t)(pnt)
#define	PNT_INT32( pnt )	(int32_t)(intptr_t)(pnt)
#endif

#define	UINT32_PNT( u )	(void*)(uintptr_t)(u)

#define	INT32_PNT( u )	(void*)(intptr_t)(u)

#define	bool_t	int
#define	TRUE	1
#define	FALSE	0

/*
 *	Debug
 */
#ifdef	VISUAL_CPP

inline	long
_ThId( pthread_t th )
{
	return( *((long*)&th) );
}

#else
#define	_ThId( th )	(long)(th)

#endif

#define	ASSERT( x )	\
	{if( !(x) ) { \
		printf("ASSERT[%ld:%u] %s-%u %s:", \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__,__LINE__,__FUNCTION__);fflush(stdout);} \
		assert( x ); }

#define	ABORT() \
	{	printf("ABORT[%ld:%u] %s-%u %s\n", \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__,__LINE__,__FUNCTION__);fflush(stdout); \
		abort(); }
#define	EXIT( x )	\
	{	if(x){ printf("EXIT(%d)[%ld:%u] %s-%u %s:", \
		x,_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__,__LINE__,__FUNCTION__);fflush(stdout);} \
		exit( x ); }

#define	Printf	printf

#define	DBG_PRT(fmt, ...) \
	(printf("PRT[%ld:%u] %s-%u %s:"fmt, \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__), \
		fflush(stdout))

#define	DBG_LCK(fmt, ...) \
	(printf("LCK[%ld] %s-%u %s:"fmt, \
		_ThId(pthread_self()), __FILE__,__LINE__, \
		__FUNCTION__,##__VA_ARGS__), \
		fflush(stdout))

#ifdef _DEBUG_

#define	DBG_ENT() \
	(printf("ENT[%ld:%u] %s-%u %s\n", \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__, __LINE__, __FUNCTION__), \
		fflush(stdout))
#define	DBG_EXT(ret) \
		(printf("EXT[%ld:%u] %s-%u %s(%d)\n", \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__, __LINE__, __FUNCTION__, ret), \
		fflush(stdout))
#define	DBG_TRC(fmt, ...) \
	(printf("TRC[%ld:%u] %s-%u %s:"fmt, \
		_ThId(pthread_self()),(uint32_t)time(0), \
		__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__), \
		fflush(stdout))
#define	RETURN( ret )	{DBG_EXT( (ret) ); return( (ret) );}

#else

#define	DBG_ENT(...)
#define	DBG_EXT(ret)
#define	DBG_TRC(fmt,...)

#define	RETURN( ret )	return( ret )

#endif

//#define	RETURN( ret )	{DBG_EXT( (ret) ); return( (ret) );}

/*
 *	Log
 */
struct	Log;

#define	LOG_FLAG_WRAP	0x1	// 指定なければバッファフル時にLogDump
#define	LOG_FLAG_BINARY	0x2	// ファイルにバッファ書出し

extern	struct Log*
		LogCreate( int Size, void*(*fMalloc)(size_t), void(*fFree)(void*), 
							int Flags, FILE*, int Level );
extern	int	LogDestroy( struct Log* pLog );
extern	int	Log( struct Log* pLog, int Level, char* pFmt, ... );
extern	int	LogDump( struct Log* pLog );
extern	int	LogDumpBuf( int Start, char* pBuf, FILE* pFile );

extern	void	LogAtExit( struct Log* );

/* Logging level */
#define	LogSYS	0
#define	LogERR	1
#define	LogWRN	2
#define	LogINF	3
#define	LogDBG	4
/*
 *	pLがNULLであれば無視
 *	バッファサイズが0であればすぐに書き出す
 */
#define	LOG( pL, L, fmt, ... ) \
		if( pL ) Log(pL, L, " %d [%ld:%u] %s-%u %s:"fmt, \
			L, _ThId(pthread_self()), time(0), \
			__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
/*
 *	Pointer
 */
#ifdef	VISUAL_CPP

#include <type_traits>
#ifdef offsetof
#undef offsetof
#endif

#define offsetof(s,m)	(size_t)&reinterpret_cast<const volatile char&>\
			((((std::remove_reference<s>::type *)0)->m))

#define container_of(ptr, T, member)	\
	(std::remove_reference<T>::type *)( ptr ? (char *)ptr - offsetof(T,member) : NULL )

#define	typeof	decltype

#else

#ifndef	offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &(((TYPE) *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({          \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif	// VISUAL_CPP

#define	DECLARE( type, name, data )	type name = (type)data
/*
 *	Compare
 */
#define	CMP( x, y )	(int)((x) - (y))

#define	SEQ_INC( x )	(++(x) == 0 ? ++(x) : (x) )
#define	SEQ_CMP( x, y ) ((x) == 0 ? \
			((y) == 0 ? 0 : -1) : \
			((y) == 0 ? 1 : (int)((x)-(y))))

/*
 *	Time
 */
#define	TIME_MAX	0x7fffffff

typedef	struct timespec	timespec_t;

#ifdef	VISUAL_CPP

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

extern	int gettimeofday(struct timeval *tv, struct timezone *tz);

inline void
_TIMESPEC( timespec_t *pTimespec )
{
	struct timeval _Now;
	gettimeofday( &_Now, NULL );

	pTimespec->tv_sec	= _Now.tv_sec;
	pTimespec->tv_nsec= _Now.tv_usec*1000;
}

#define	TIMESPEC( T )	_TIMESPEC( &(T) )

#define TIMEOUT( Now, MS, Timeout ) \
{ \
	int64_t _w;  \
 \
	_w = (int64_t)(Now).tv_sec*1000000000LL + (Now).tv_nsec; \
	_w += (int64_t)(MS)*1000000LL; \
	(Timeout).tv_sec	= _w/1000000000LL;  \
	(Timeout).tv_nsec	= (long)(_w - (int64_t)(Timeout).tv_sec*1000000000LL); \
}

#define	TIMESPECCOMPARE( A, B ) \
	( (A).tv_sec == (B).tv_sec ? \
	( (A).tv_nsec == (B).tv_nsec ? 0 : ((A).tv_nsec - (B).tv_nsec) ) \
	: ((A).tv_sec - (B).tv_sec) )

#define	IsTimespecZero( A )	\
	( (A).tv_sec == 0 && (A).tv_nsec == 0 )

#define	TimespecAddUsec( A, usec ) \
{ \
	int64_t _w; \
	int32_t _c; \
 \
	_w = (usec); _w *= 1000L; _w += (A).tv_nsec; \
	_c = (int32_t)(_w/(1000L*1000*1000)); \
	(A).tv_sec += _c; \
	(A).tv_nsec = (long)(_w - _c*(1000L*1000*1000));  \
}

#define	TimespecSubUsec( A, usec ) \
{ \
	int64_t _w; \
	int64_t _c; \
 \
	_w = (usec); _w *= 1000L; \
	_c = (A).tv_sec*(1000L*1000*1000) + (A).tv_nsec; \
	_c -= _w; \
	_w = _c/(1000L*1000*1000); \
	(A).tv_sec	= _w; \
	(A).tv_nsec	= (long)(_c - _w*(1000L*1000*1000));  \
}

#else

#define	TIMESPEC( Timespec ) \
	({struct timeval _Now;gettimeofday( &_Now, NULL ); \
 (Timespec).tv_sec	= _Now.tv_sec; (Timespec).tv_nsec	= _Now.tv_usec*1000;})

#define	TIMEOUT( Now, MS, Timeout ) \
	({int64_t _w; _w = (int64_t)(Now).tv_sec*1000000000LL + (Now).tv_nsec; \
		_w += (int64_t)(MS)*1000000LL; \
		(Timeout).tv_sec	= _w/1000000000LL; \
		(Timeout).tv_nsec	= _w - (int64_t)(Timeout).tv_sec*1000000000LL;})

#define	TIMESPECCOMPARE( A, B ) \
	( (A).tv_sec == (B).tv_sec ? \
	( (A).tv_nsec == (B).tv_nsec ? 0 : ((A).tv_nsec - (B).tv_nsec) ) \
	: ((A).tv_sec - (B).tv_sec) )

#define	IsTimespecZero( A )	\
	( (A).tv_sec == 0 && (A).tv_nsec == 0 )

#define	TimespecAddUsec( A, usec ) \
	({int64_t _w;int32_t _c; _w = (usec); _w *= 1000L; _w += (A).tv_nsec; \
		 _c = _w/(1000L*1000*1000); \
		(A).tv_sec += _c; (A).tv_nsec = _w - _c*(1000L*1000*1000);}) 

#define	TimespecSubUsec( A, usec ) \
	({int64_t _w;int64_t _c; _w = (usec); _w *= 1000L; \
		 _c = (A).tv_sec*(1000L*1000*1000) + (A).tv_nsec; _c -= _w; \
		 _w = _c/(1000L*1000*1000); \
		(A).tv_sec	= _w; \
		(A).tv_nsec = _c - _w*(1000L*1000*1000);}) 
#endif

#define	SetTimespecZero( A )	memset( &(A), 0, sizeof(A) )

/*
 *	Double Link
 */
typedef struct Dlnk {
	struct Dlnk	*q_next;	/* next link */
	struct Dlnk	*q_prev;	/* previous link */
} Dlnk_t;

#define	dlInit( qp ) \
	(((Dlnk_t*)(qp))->q_next=((Dlnk_t*)(qp))->q_prev=(Dlnk_t*)(qp))

static inline	void	
dlInsert( Dlnk_t* p, Dlnk_t* before )
{
	p->q_prev	= before->q_prev;
	p->q_next	= before;
	if( before->q_prev )
		before->q_prev->q_next = p;
	before->q_prev	= p;
}

static inline	void	
dlAppend( Dlnk_t* p, Dlnk_t* after )
{
	p->q_prev	= after;
	p->q_next	= after->q_next;
	if( after->q_next )
		after->q_next->q_prev = p;
	after->q_next	= p;
}

static inline	void	
dlRemove( Dlnk_t* p )
{
	if( p->q_prev )
		p->q_prev->q_next	= p->q_next;
	if( p->q_next )
		p->q_next->q_prev	= p->q_prev;
}

static inline	Dlnk_t*	
dlHead( Dlnk_t* ep )
{
	if( ep->q_next ) {
		if( ep != ep->q_next ) {
			return( ep->q_next );
		}
	}
	return( 0 );
}

static inline	Dlnk_t*	
dlTail( Dlnk_t* ep )
{
	if( ep->q_prev ) {
		if( ep != ep->q_prev ) {
			return( ep->q_prev );
		}
	}
	return( 0 );
}

static inline	void
dlReplace( Dlnk_t* old, Dlnk_t* New )
{
	New->q_next			= old->q_next;
	New->q_next->q_prev	= New;
	New->q_prev			= old->q_prev;
	New->q_prev->q_next	= New;
}

static inline	void
dlJoin( Dlnk_t* frHead, Dlnk_t* toHead )
{
    Dlnk_t	*first = frHead->q_next;
    Dlnk_t	*last = frHead->q_prev;
    Dlnk_t	*at = toHead->q_next;

    first->q_prev = toHead;
    toHead->q_next = first;

    last->q_next = at;
    at->q_prev = last;
}

#define	dlNext( qp )		(((Dlnk_t *)(qp))->q_next)
#define	dlPrev( qp )		(((Dlnk_t *)(qp))->q_prev)
#define	dlValid( ep, qp )	((qp)&&((Dlnk_t *)(ep)!=(Dlnk_t *)(qp)))

typedef	Dlnk_t	list_t;

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
	Dlnk_t name = LIST_HEAD_INIT(name)

#define	list_init( pEntry )				dlInit( pEntry )
#define	list_first( pHead )				dlHead( pHead )
#define	list_last( pHead )				dlTail( pHead )

#ifdef	VISUAL_CPP

#define	list_first_entry( pHead, type, member ) \
		list_entry(dlHead(pHead),type,member)
#define	list_last_entry( pHead, type, member ) \
		list_entry(dlTail(pHead),type,member)
#else

#define	list_first_entry( pHead, type, member ) \
	({Dlnk_t* p = dlHead( pHead ); \
		( p == NULL ? NULL: list_entry(p,type,member) );})
#define	list_last_entry( pHead, type, member ) \
	({Dlnk_t* p = dlTail( pHead ); \
		( p == NULL ? NULL: list_entry(p,type,member) );})
#endif

#define	list_add( pNew, pHead )			dlAppend( pNew, pHead )
#define	list_add_tail( pNew, pHead )	dlInsert( pNew, pHead )
#define	list_del( pEntry )				dlRemove( pEntry )
#define	list_del_init( pEntry ) \
	{dlRemove( pEntry ); dlInit( pEntry );}
#define	list_replace( pOld, pNew )		dlReplace( pOld, pNew )
#define	list_replace_init( pOld, pNew ) \
	{dlReplace( pOld, pNew );dlInit( pOld );}
#define	list_move( pEntry, pHead ) \
	{dlRemove( pEntry );dlAppend( pEntry, pHead );}
#define	list_move_tail( pEntry, pHead ) \
	{dlRemove( pEntry );dlInsert( pEntry, pHead );}
#define	list_is_last( pEntry, pHead )	( dlTail( pHead ) == pEntry )
#define	list_empty( pHead )				!dlHead( pHead )
#define	list_splice( pFrHead, pToHead ) \
	{if( dlHead( pFrHead ) )	dlJoin( pFrHead, pToHead);}
#define	list_splice_init( pFrHead, pToHead ) \
	{if( dlHead( pFrHead ) ) { \
		dlJoin( pFrHead, pToHead);dlInit(pFrHead); \
	 }}

#define list_for_each(pos, head) \
	for (pos = (head)->q_next;  pos != (head); \
        	pos = pos->q_next)
#define list_for_each_prev(pos, head) \
	for (pos = (head)->q_prev; pos != (head); \
        	pos = pos->q_prev)
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->q_next, n = pos->q_next; pos != (head); \
		pos = n, n = pos->q_next)

#ifdef	VISUAL_C

#define list_for_each_entry(pos, head, member)	\
	for (pos = list_entry((head)->q_next, decltype(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.q_next, decltype(*pos), member))

#define list_for_each_entry_reverse(pos, head, member, type)	\
	for (pos = list_entry((head)->q_prev, type, member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.q_prev, type, member))

#define list_prepare_entry(pos, head, member, type) \
	((pos) ? : list_entry(head, type, member))

#define list_for_each_entry_continue(pos, head, member, type) 	\
	for (pos = list_entry(pos->member.q_next, type, member);\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.q_next, type, member))

#define list_for_each_entry_from(pos, head, member, type) 		\
	for (; &pos->member != (head);	\
	     pos = list_entry(pos->member.q_next, type, member))

#define list_for_each_entry_safe(pos, n, head, member, type)		\
	for (pos = list_entry((head)->q_next, type, member),	\
		n = list_entry(pos->member.q_next, type, member);\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.q_next, type, member))

#define list_for_each_entry_safe_continue(pos, n, head, member, type)	\
	for (pos = list_entry(pos->member.q_next, type, member), \
		n = list_entry(pos->member.q_next, type, member);\
	     &pos->member != (head);					\
	     pos = n, n = list_entry(n->member.q_next, type, member))

#define list_for_each_entry_safe_from(pos, n, head, member, type)	\
	for (n = list_entry(pos->member.q_next, type, member);	\
	     &pos->member != (head);					\
	     pos = n, n = list_entry(n->member.q_next, type, member))

#define list_for_each_entry_safe_reverse(pos, n, head, member, type)	\
	for (pos = list_entry((head)->q_prev, type, member),	\
		n = list_entry(pos->member.q_prev, type, member);\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.q_prev, type, member))

#else

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->q_next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.q_next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->q_prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.q_prev, typeof(*pos), member))

#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.q_next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.q_next, typeof(*pos), member))
#define list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);	\
	     pos = list_entry(pos->member.q_next, typeof(*pos), member))
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->q_next, typeof(*pos), member),	\
		n = list_entry(pos->member.q_next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.q_next, typeof(*n), member))
#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_entry(pos->member.q_next, typeof(*pos), member), 		\
		n = list_entry(pos->member.q_next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = list_entry(n->member.q_next, typeof(*n), member))
#define list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = list_entry(pos->member.q_next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = list_entry(n->member.q_next, typeof(*n), member))
#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->q_prev, typeof(*pos), member),	\
		n = list_entry(pos->member.q_prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.q_prev, typeof(*n), member))

#endif
/*
 *	Remote Memory Dump
 *		LIST_FOR_EACH_ENTRY( pBody, ... ) {
 *			read into Body from remote by pBody;
 *			...
 *			pBody = &Body;
 *		}
 */
#define LIST_FOR_EACH_ENTRY(pos, head, member, now, org )			\
	for (pos = list_entry((head)->q_next, typeof(*pos), member);	\
	     (char*)&pos->member != (char*)(head)-((char*)(now)-(char*)(org)); 	\
	     pos = list_entry(pos->member.q_next, typeof(*pos), member))

#define	LIST_FIRST_ENTRY( head, type, member, now, org ) \
	({type* _p = list_entry((head)->q_next, type, member); \
     if((char*)&_p->member == \
		(char*)(head)-((char*)(now)-(char*)(org)))_p=NULL;_p;})

#define	LIST_LAST_ENTRY( head, type, member, now, org ) \
	({type* _p = list_entry((head)->q_prev, type, member); \
     if((char*)&_p->member == \
		(char*)(head)-((char*)(now)-(char*)(org)))_p=NULL;_p;})

/*
 *	Lock
 */

typedef	pthread_mutex_t		Mtx_t;
#define	MtxInit( pm )		pthread_mutex_init( (pm), 0 )
#ifndef	VISUAL_CPP
extern int	pthread_mutexattr_settype(pthread_mutexattr_t*, int);
#endif
#define	MtxInitRecursive( pm ) \
	{pthread_mutexattr_t mattr; memset(&mattr,0,sizeof(mattr)); \
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE_NP); \
	pthread_mutex_init( (pm), &mattr );}
#define	MtxDestroy( pm )	pthread_mutex_destroy( (pm) )

#ifndef _LOCK_

#define	MtxLock( pm )		pthread_mutex_lock( (pm) )
#define	MtxTrylock( pm )	pthread_mutex_trylock( (pm) )
#define	MtxUnlock( pm )		pthread_mutex_unlock( (pm) )

#else

#define	MtxLock( pm ) \
	({DBG_LCK("MtxLock[%p]\n",pm);pthread_mutex_lock( (pm) );})
#define	MtxTrylock( pm ) \
	({DBG_LCK("MtxTrylock[%p]\n",pm);pthread_mutex_trylock( (pm) );})
#define	MtxUnlock( pm ) \
	({DBG_LCK("MtxUnlock[%p]\n",pm);pthread_mutex_unlock( (pm) );})
#endif

typedef	pthread_cond_t		Cnd_t;
#define	CndInit( pc )		pthread_cond_init( (pc), 0 )
#define	CndDestroy( pc )	pthread_cond_destroy( (pc) )

//#ifdef	VISUAL_CPP

static inline int
CndTimedWait( Cnd_t *pc, Mtx_t *pm, struct timespec *pt )
{
	int	Ret;

 	Ret = pthread_cond_timedwait( pc, pm, pt );
	if( Ret )	errno = Ret;
	return( Ret );
}

static inline int
CndTimedWaitWithMs( Cnd_t *pc, Mtx_t *pm, int ms )
{	int	Ret;
	struct timespec Now, Timeout; 
	TIMESPEC( Now ); TIMEOUT( Now, ms, Timeout );
	Ret = pthread_cond_timedwait( pc, pm, &Timeout );
	if( Ret ) errno = Ret;
	return( Ret );
}

//#else
/***
#define	CndTimedWait( pc, pm, pt ) pthread_cond_timedwait( (pc), (pm), (pt) )
#define	CndTimedWaitWithMs( pc, pm, ms ) \
	({struct timespec Now, Timeout; \
		TIMESPEC( Now ); TIMEOUT( Now, ms, Timeout ); \
		pthread_cond_timedwait( (pc), (pm), &Timeout );})
***/
//#endif

#define	CndWait( pc, pm )	pthread_cond_wait( (pc), (pm) )
#define	CndSignal( pc )		pthread_cond_signal( (pc) )
#define	CndBroadcast( pc )	pthread_cond_broadcast( (pc) )

typedef	pthread_rwlock_t	RwLock_t;
#define	RwLockInit( pl )	pthread_rwlock_init( (pl), 0 )
#define	RwLockDestroy( pl )	pthread_rwlock_destroy( (pl) )

#ifndef _LOCK_

#define	RwUnlock( pl )		pthread_rwlock_unlock( (pl) )
#define	RwLockR( pl )		pthread_rwlock_rdlock( (pl) )
#define	RwLockW( pl )		pthread_rwlock_wrlock( (pl) )

#else

#define	RwUnlock( pm ) \
	({DBG_LCK("RwUnlock[%p]\n",pm);pthread_rwlock_unlock( (pm) );})
#define	RwLockR( pm ) \
	({DBG_LCK("RwLockR[%p]\n",pm);pthread_rwlock_rdlock( (pm) );})
#define	RwLockW( pm ) \
	({DBG_LCK("RwLockW[%p]\n",pm);pthread_rwlock_wrlock( (pm) );})

#endif

/*
 *	Wait queue
 */
typedef	struct Queue {
	Mtx_t	q_mtx;
	Cnd_t	q_cnd;
	Dlnk_t	q_entry;
	int		q_cnt;
	int		q_abrt;
#ifdef	ZZZ
#else
#define	QUEUE_SUSPEND	0x1
	int		q_flg;
	int		q_max;
	int		q_omitted;
	int		q_waiter;
#endif
} Queue_t;

static inline	void	
QueueInit( Queue_t* qp )
{
#ifdef	ZZZ
#else
	memset( qp, 0, sizeof(*qp) );
#endif
	MtxInit( &qp->q_mtx );
	CndInit( &qp->q_cnd );
	dlInit( &qp->q_entry );
#ifdef	ZZZ
	qp->q_cnt	= 0;
	qp->q_abrt	= 0;
#endif
}

static inline	void
QueueDestroy( Queue_t* pQ )
{
	CndDestroy( &pQ->q_cnd );
	MtxDestroy( &pQ->q_mtx );
}

#define	FOREVER	-1

static inline	int	
QueueTimedWait( Queue_t* ep, Dlnk_t** ppEntry, int ms )
{
	int			ret;
	Dlnk_t*	pEntry = 0;
	struct timespec Now={0,}, Timeout={0,};

	MtxLock( &ep->q_mtx );

#ifdef	ZZZ
	ep->q_cnt--;
#else
	ep->q_waiter++;
#endif
	if( ms >= 0 ) {	TIMESPEC( Now ); TIMEOUT( Now, ms, Timeout );}

#ifdef	ZZZ
	while( !(pEntry = dlHead( &ep->q_entry ) ) ) {
#else
	while( ep->q_flg || !(pEntry = dlHead( &ep->q_entry ) ) ) {
#endif

		if( ep->q_abrt ) {
			ret = errno = ep->q_abrt;
			goto err;
		}
		if( ms >= 0 )	ret = CndTimedWait(&ep->q_cnd, &ep->q_mtx, &Timeout );
		else			ret = CndWait(&ep->q_cnd, &ep->q_mtx);
		if( ret )	goto err;
	}
#ifdef	ZZZ
	if( pEntry )	dlRemove( pEntry );
#else
	if( pEntry ) {
		dlRemove( pEntry );
		ep->q_cnt--;
	}
	ep->q_waiter--;
#endif

	*ppEntry	= pEntry;

	MtxUnlock( &ep->q_mtx );
	return( 0 );
err:
#ifdef	ZZZ
	ep->q_cnt++;
#else
	ep->q_waiter--;
#endif
	MtxUnlock( &ep->q_mtx );
	return( ret );
}

static inline	int	
QueuePost( Queue_t* ep, Dlnk_t* qp )
{
#ifdef	ZZZ
#else
	int	Ret;
#endif
	MtxLock( &ep->q_mtx );

	dlInit( qp );
#ifdef	ZZZ
	dlInsert( qp, &ep->q_entry );
	ep->q_cnt++;
#else
	if( ep->q_max && ep->q_cnt >= ep->q_max ) {
		ep->q_omitted++;
		Ret = -1;
	} else {
		dlInsert( qp, &ep->q_entry );
		ep->q_cnt++;
		Ret = 0;
	}
#endif
	CndSignal( &ep->q_cnd );

	MtxUnlock( &ep->q_mtx );
#ifdef	ZZZ
	return( 0 );
#else
	return( Ret );
#endif
}

static inline	int	
QueueAbort( Queue_t* ep, int Abrt )
{
	if( Abrt == ETIMEDOUT )	return( -1 );

	MtxLock( &ep->q_mtx );

	ep->q_abrt	= Abrt;
	CndBroadcast( &ep->q_cnd );

	MtxUnlock( &ep->q_mtx );
	return( 0 );
}

#ifdef	ZZZ
#else
static inline	int
QueueSuspend( Queue_t *ep )
{
	MtxLock( &ep->q_mtx );

	ep->q_flg	|= QUEUE_SUSPEND;

	MtxUnlock( &ep->q_mtx );
	return( 0 );
}

static inline	int	
QueueResume( Queue_t* ep )
{
	MtxLock( &ep->q_mtx );

	ep->q_flg	&= ~QUEUE_SUSPEND;

	CndBroadcast( &ep->q_cnd );

	MtxUnlock( &ep->q_mtx );
	return( 0 );
}

static inline	int	
QueueCnt( Queue_t* ep )
{
	/* atomic */
	return( ep->q_cnt );
}

static inline	int	
QueueMax( Queue_t* ep, int Max )
{
	MtxLock( &ep->q_mtx );

	ep->q_max = Max;

	MtxUnlock( &ep->q_mtx );
	return( 0 );
}

static inline	int	
QueueOmitted( Queue_t* ep )
{
	/* atomic */
	return( ep->q_omitted );
}

#endif

#define	queue_entry(ptr, type, member)	container_of(ptr, type, member)

//#ifdef	VISUAL_CPP

#define	QueueWaitEntry( pQueue, pEntry, member ) \
	{Dlnk_t* _p; pEntry = NULL; \
		if( !QueueTimedWait(pQueue,&_p,FOREVER) ) \
			pEntry = queue_entry(_p,typeof(*pEntry),member);}

#define	QueueTimedWaitEntryWithMs( pQueue, pEntry, member, ms ) \
	{Dlnk_t* _p; pEntry = NULL; \
		if( !QueueTimedWait(pQueue,&_p,ms)  ) \
			pEntry = queue_entry(_p,typeof(*pEntry),member);}

#define	QueuePostEntry( pQueue, pEntry, member ) \
	QueuePost( pQueue, &(pEntry)->member )

//#else

/***
#define	QueueWaitEntry( pQueue, pEntry, member ) \
	({Dlnk_t* _p;int _ret; _ret = QueueTimedWait(pQueue,&_p,FOREVER); \
		if( !_ret ) pEntry = queue_entry(_p,typeof(*pEntry),member);_ret;})
#define	QueueTimedWaitEntryWithMs( pQueue, pEntry, member, ms ) \
	({Dlnk_t* _p;int _ret; _ret = QueueTimedWait(pQueue,&_p,ms); \
		if( !_ret ) pEntry = queue_entry(_p,typeof(*pEntry),member);_ret;})
#define	QueuePostEntry( pQueue, pEntry, member ) \
	({int _ret; _ret=QueuePost( pQueue, &(pEntry)->member );_ret;})
***/
//#endif

/*
 *	Memory
 */
extern	void*	Malloc(size_t);
extern	void	Free(void*);
extern	void	MallocDump(void);
extern	int		MallocCount(void);

#ifdef _DEBUG_
extern	bool_t	IsFree(void*);
#else
#define	IsFree( p )
#endif

#define	MallocCast( type, size )	(type*)Malloc( size )
#define	CallocCast( type, size )	(type*)Calloc( size )
#define	FreeCast( p )			Free( (void*)(p) )

/** 
 * 概要
 *	ヒープ管理
 * 機能
 *	  ヒープは、一次元配列上に、とられ、先頭にヒープ管理情報構造体がおかれ、
 *	残りのデータ域上には、動的にセグメントがとられる。後尾には、
 *	ターミネートセグメントがおかれる。
 *	  セグメントには、空きセグメントと使用中セグメントの別があり、s_size の
 *	正負で区別される。全セグメントはチェインリストで昇順につながれ、
 *	このうち空きセグメントはフリーリストにつながれる。
 *	  取得はファーストフィットあるいはベストフィットとしている。
 *	なお、排他制御は外側で行う。
 */
/* 
 *	NHeapPoolStatistics( NHeapPool_t *pPool, int HeapNo, int Cmd );
 *		HEAP_STATIS_ON     統計情報フラグをオンとする
 *		HEAP_STATIS_OFF    統計情報フラグをオフとする
 *		HEAP_FREE_SIZE     フリー領域の総和
 *		HEAP_FREE_SEG      フリーセグメントの数
 *		HEAP_FREE_MAX      最大フリーセグメントのサイズ
 *		HEAP_ALLOC_SIZE    使用領域の総和
*/

typedef struct NHeapPool {
	void*		p_pVoid;
	int			p_HeapUnit;
	int			p_HeapMax;		// Unit, Unit*2, ... Unit*2^(Max-1)
#define	HEAP_DYNAMIC	0x1 //  動的に取得
#define	HEAP_BEST_FIT	0x2	//　Best Fit
	int			p_Flags;
	void*		(*p_fMalloc)(size_t);
	void		(*p_fFree)(void*);
	size_t		p_Size;		// Total size
	size_t		p_Use;		// Total current use
	size_t		p_Max;		// Total max used
	void*		p_aHeaps;
} NHeapPool_t;


#define	HEAP_STATIS_ON	1
#define	HEAP_STATIS_OFF	2
#define	HEAP_FREE_SIZE	3
#define	HEAP_FREE_SEG	4
#define	HEAP_FREE_MAX	5
#define	HEAP_ALLOC_SIZE	6

int 	NHeapPoolInit( NHeapPool_t* pPool,
				int Unit, int Max, 
				void*(*fMalloc)(size_t), 
				void(*fFree)(void*),
				int Flags );
int 	NHeapPoolDestroy( NHeapPool_t* pPool );
void* 	NHeapMalloc( NHeapPool_t* pPool, size_t size );
void* 	NHeapZalloc( NHeapPool_t* pPool, size_t size );
void* 	NHeapRealloc( void* p, size_t size );
int	NHeapFree( void* p );
int	NHeapPoolStatistics( NHeapPool_t *pPool, int HeapNo, int Cmd );
void	NHeapPoolDump( NHeapPool_t *pPool );


/**
 *	ハッシュ管理
 */
typedef	struct HashItem {
	list_t		i_Link;
	void*		i_pKey;
	uint64_t	i_U64;
	void*		i_pValue;
} HashItem_t;

typedef	struct Hash {
	void*		h_pVoid;
	int			h_N;
	int			h_Count;
	int			h_CurIndex;
	HashItem_t*	h_pCurItem;
	int			(*h_fHashCode)(void*);
	int			(*h_fCompare)(void*,void*);
	void		(*h_fPrint)(void*pKey,void*pValue);
	void*		(*h_fMalloc)(size_t);
	void		(*h_fFree)(void*);
	void		(*h_fDestroy)(struct Hash*,void*);
	list_t*		h_aHeads;
} Hash_t;

#define	HASH_KEY( pItem )	((pItem)->i_pKey)
#define	HASH_VALUE( pItem )	((pItem)->i_pValue)

#define	HASH_KEY_INT	1
#define	HASH_KEY_STR	3
#define	HASH_KEY_PNT	5
#define	HASH_KEY_U64	7

/* ハッシュコード */
#define	HASH_CODE_INT	(int(*)(void*))(HASH_KEY_INT)
#define	HASH_CODE_STR	(int(*)(void*))(HASH_KEY_STR)
#define	HASH_CODE_PNT	(int(*)(void*))(HASH_KEY_PNT)
#define	HASH_CODE_U64	(int(*)(void*))(HASH_KEY_U64)

/* キー値比較 */
#define	HASH_CMP_INT	(int(*)(void*,void*))(HASH_KEY_INT)
#define	HASH_CMP_STR	(int(*)(void*,void*))(HASH_KEY_STR)
#define	HASH_CMP_PNT	(int(*)(void*,void*))(HASH_KEY_PNT)
#define	HASH_CMP_U64	(int(*)(void*,void*))(HASH_KEY_U64)

/* 素数 */
#define	PRIMARY_5		5
#define	PRIMARY_11		11
#define	PRIMARY_101		101
#define	PRIMARY_1009	1009
#define	PRIMARY_1013	1013
#define	PRIMARY_2003	2003
#define	PRIMARY_4003	4003
#define	PRIMARY_5003	5003
#define	PRIMARY_10007	10007
#define	PRIMARY_50021	50021
#define	PRIMARY_100003	100003

int			HashInit( Hash_t* pHash, 
				int size, 				// 素数
				int(*fHashCode)(void*), int(*fCompare)(void*,void*),
				void(*fPrint)(void*,void*),
				void*(*fMalloc)(size_t), void(*fFree)(void*),
				void(*fDestroy)(Hash_t*,void*) );
void		HashClear( Hash_t* pHash );
void		HashDestroy( Hash_t* pHash );
int			HashPut( Hash_t* pHash, void* pKey, void* pValue );
void*		HashGet( Hash_t* pHash, void* pKey );
void*		HashNext( Hash_t* pHash, void* pKey );
void*		HashRemove( Hash_t* pHash, void* pKey );
HashItem_t*	HashGetItem( Hash_t* pHash, void* pKey );
HashItem_t*	HashNextItem( Hash_t* pHash, void* pKey );
HashItem_t*	HashIsExist( Hash_t* pHash );
int			HashRemoveItem( Hash_t* pHash, HashItem_t* pItem );
void		HashDump( Hash_t* pHash );
int			HashCount( Hash_t* pHash );
long 		HashStrCode( char *pKey );

typedef struct HashList {
	Hash_t	hl_Hash;
	list_t	hl_Lnk;
} HashList_t;

#define	HashListInit( HL, PRM, CODE, CMP, PRNT, A, F, D ) \
	{HashInit( &(HL)->hl_Hash,(PRM),(CODE),(CMP),(PRNT),(A),(F),(D) ); \
		list_init( &(HL)->hl_Lnk );}

#define	HashListDestroy( HL )		HashDestroy( &(HL)->hl_Hash )
#define	HashListClear( HL )		{HashClear( &(HL)->hl_Hash ); \
					list_init(&(HL)->hl_Lnk);}
#define	HashListPut( HL, K, P, L )	{HashPut(&(HL)->hl_Hash,(K),(P)); \
					list_add_tail(&(P)->L,&(HL)->hl_Lnk);}
#ifdef	VISUAL_CPP

#define	HashListDel( HL, K, T, L )	{void*_p; \
					_p=HashRemove(&(HL)->hl_Hash,(K)); \
					if(_p)list_del_init(&((T)_p)->L);}

#define	HashListGet( HL, K )		HashGet(&(HL)->hl_Hash,(K))

#else
#define	HashListDel( HL, K, T, L )	({void*_p; \
					_p=HashRemove(&(HL)->hl_Hash,(K)); \
					if(_p)list_del_init(&((T)_p)->L);_p;})
#define	HashListGet( HL, K )		({void*p;p=HashGet(&(HL)->hl_Hash,(K));p;})
#endif


typedef struct DHashAddElm {
	int			e_Cnt;
	Hash_t		*e_pHash;
} DHashElm_t;

typedef struct DHash {
	int			d_Primary1;
	int			d_Primary1Size;
	int			d_Primary2;
	Hash_t		d_Hash;
	DHashElm_t	*d_aElm;
} DHash_t;

int			DHashInit( DHash_t* pDHash, 
				int Prim1, int Prim1Size, int Prim2,	// Prim1,2は互いに素。
				int(*fHashCode)(void*), int(*fCompare)(void*,void*),
				void(*fPrint)(void*,void*),
				void*(fMalloc)(size_t), void(*fFree)(void*),
				void(fDestroy)(Hash_t*,void*) );
void		DHashClear( DHash_t* pDHash );
void		DHashDestroy( DHash_t* pDHash );
int			DHashPut( DHash_t* pDHash, void* pKey, void* pValue );
void*		DHashGet( DHash_t* pDHash, void* pKey );
void*		DHashNext( DHash_t* pDHash, void* pKey );
void*		DHashRemove( DHash_t* pDHash, void* pKey );
int			DHashCount( DHash_t* pDHash );
void		DHashDump( DHash_t* pDHash );

typedef struct DHashList {
	DHash_t	hl_Hash;
	list_t	hl_Lnk;
} DHashList_t;

#define	DHashListInit( HL, P1, L, P2, CODE, CMP, PRNT, A, F, D ) \
	({DHashInit( &(HL)->hl_Hash, (P1), (L), (P2), (CODE), (CMP), (PRNT), \
							(A), (F), (D) ); \
		list_init( &(HL)->hl_Lnk );})

#define	DHashListDestroy( HL )		DHashDestroy( &(HL)->hl_Hash )
#define	DHashListClear( HL )			({DHashClear( &(HL)->hl_Hash ); \
										list_init(&(HL)->hl_Lnk);})
#define	DHashListPut( HL, K, P, L )	({DHashPut(&(HL)->hl_Hash,(K),(P)); \
									list_add_tail(&(P)->L,&(HL)->hl_Lnk);})
#define	DHashListDel( HL, K, T, L )	({void*_p; \
										_p=DHashRemove(&(HL)->hl_Hash,(K)); \
										if(_p)list_del_init(&((T)_p)->L);_p;})
#define	DHashListGet( HL, K )		({void*p;p=DHashGet(&(HL)->hl_Hash,(K));p;})

/**
	B(Balance)木
		N次数が与えられた時、ページは最大2*N個の項目を有す。
		また、ページは最大2*N+1個の子供を持つ。
		2*Nを超える場合には分割され、Nより少なければ併合される。
		従って、各ページはN個以上を持ち、使用率50%以上となる。
		例えば、M個を登録する場合、
			log2*N(M）が深さであり、log2(N)が各階層の検索コストであり、
		最大、log2*N(M)*log2(N)がトータルコストとなる。

	1.挿入は、必ず葉のページに対して行う。
	2.2*Nを超えた場合は、ページを分割し、上位に伝える。
		2.1.分割の場合はページが獲得される。
	3.節の削除は、直前の葉にある項目と入れ替える。
	4.葉がNより小さければ、親の項目及び左右のページを含め平衡、併合を行う。
		4.1.併合時には、ページが返却される。
	5.併合が発生した場合には、上位に伝える。
	6.カーサを使用することより2*log2*N(M)で次項目を参照できる。
	7.削除は、カーサがあれば失敗となる。
	8.排他制御は外側で行う。
*/

struct	BTPage;

typedef	void*	BTItem_t;

typedef struct	BTPage {
	struct	BTPage*	p_pParent;
	int				p_ParentIndex;
	int				p_M;
	BTItem_t*		p_aItems;	// [0...2n-1]	R-1
	struct BTPage**	p_apPages;	// [0...2n]		R
} BTPage_t;

typedef struct BTree {
	void*			b_pVoid;
	int				b_N;
	BTPage_t*		b_pRoot;
	long			(*b_fCompare)(void*,void*);	
	void			(*b_fPrint)(void*);
	void*			(*b_fMalloc)(size_t);
	void			(*b_fFree)(void*);
	int				(*b_fDestroy)(struct BTree*,void*);
	uint64_t		b_Cnt;
} BTree_t;

typedef struct {
	BTree_t*	c_pBTree;
	BTPage_t*	c_pPage;
	int			c_zIndex;
	void*		c_pKey;
	BTPage_t*	c_pNextPage;
	int			c_NextIndex;
	void*		c_pNextKey;
} BTCursor_t;

int		BTreeInit( BTree_t* pBTree, int n, 
				long(*fCompare)(void* pR, void*pL), void(*fPrint)(void*),
				void*(*fMalloc)(size_t), 
				void(*fFree)(void*),
				int(*fDestroy)(BTree_t*,void*) );
int		BTreeDestroy( BTree_t* pBTree );
uint64_t	BTreeCount( BTree_t* pBTree );
int 		BTInsert( BTree_t* pBTree, void* pKey );
int 		BTDelete( BTree_t* pBTree, void* pKey );
int 		BTCursorOpen( BTCursor_t* pCursor, BTree_t* pBTree );
int 		BTCursorClose( BTCursor_t* pCursor );
int 		BTCHead( BTCursor_t* pCursor );
int 		BTCNext( BTCursor_t* pCursor );
int 		BTCPrev( BTCursor_t* pCursor );
int			BTCFind( BTCursor_t* pCursor, void* pKey );
int			BTCFindLower( BTCursor_t* pCursor, void* pKey ); // pKey <= Lower
int			BTCFindUpper( BTCursor_t* pCursor, void* pKey ); // Upper <= pKey
int			BTCDelete( BTCursor_t* pCursor );
void 		PrintCursor( BTCursor_t* pCursor );
long		BTreeCheck( BTree_t* pBTree );

#define		BTCGetKey( type, pCursor ) \
			((pCursor)->c_pPage == NULL ? \
					(type)NULL:((type)((pCursor)->c_pKey)))
#define		BTCNextGetKey( type, pCursor ) \
			((pCursor)->c_pNextPage == NULL ? \
					(type)NULL:((type)((pCursor)->c_pNextKey)))

void		BTreePrint( BTree_t* );

/*
 *	Timer
 */

typedef struct Timer {
#define	TIMER_BTREE	0x1
#define	TIMER_HEAP	0x2
	int				t_Flags;

	Mtx_t			t_Mtx;
	Cnd_t			t_Cnd;	
	bool_t			t_Active;
	bool_t			t_OnOff;
	struct timespec	t_Timeout;
	list_t			t_Events;
	Queue_t			t_Execs;
	NHeapPool_t		t_Heap;
	BTree_t			t_BTree;
	void*			(*t_fMalloc)(size_t);
	void			(*t_fFree)(void*);
	pthread_t		t_ThreadTimer;
	pthread_t		t_ThreadPerformer;
	Hash_t			t_Hash;
} Timer_t;

typedef enum TEMode {
	TE_TIMER,
	TE_HEAD,
	TE_PERFORMER
} TEMode_t;

typedef struct TimerEvent {
	list_t				e_List;
	TEMode_t			e_Mode;
	int					e_MS;
	int					(*e_fEvent)(struct TimerEvent*);
	void*				e_pArg;
	struct timespec		e_Registered;
	struct timespec		e_Fired;
	struct timespec		e_Exec;
	struct timespec		e_Timeout;
} TimerEvent_t;

int 			TimerInit( 
					Timer_t* pTimer, int Flags,
					int HeapUnit, int HeapMax, int HeapDynamic,
					void*(*fMalloc)(size_t), void(*fFree)(void*), 
					int BTreeN, void(*fBTItemPrint)(void*) );
void 			TimerDestroy( Timer_t* pTimer );
void			TimerEnable( Timer_t* pTimer );
void			TimerDisable( Timer_t* pTimer );
int				TimerStart( Timer_t* pTimer );
int				TimerStop( Timer_t* pTimer );
void*			TimerPerformerThread( void* pTimer );
void*			TimerThread( void*	pTimer );
TimerEvent_t*	TimerSet( Timer_t* pTimer, int ms, 
					int(*fEvent)(TimerEvent_t*), void* pArg );
void*			TimerCancel( Timer_t* pTimer, TimerEvent_t* pEvent );

/*
 *	Channel(send only)
 */

typedef	struct	ChSend {
	void*				c_pVoid;
	struct sockaddr_in	c_PeerAddr;
	pthread_t			c_Sender;
	Queue_t				c_Queue;
#define	CHSEND_STOP	0x1
#define	CHSEND_DOWN	0x2
	uint32_t			c_Status;
	int					(*c_fOutbound)(struct ChSend*, void*);
	void*				(*c_fMalloc)(size_t);
	void				(*c_fFree)(void*);
} ChSend_t;

#define	ChSendInit( pC, pVoid, PeerAddr, fOutbound, fMalloc, fFree ) \
	({ QueueInit( &(pC)->c_Queue ); (pC)->c_Status = 0; \
		(pC)->c_pVoid	= (void*)pVoid; \
		(pC)->c_PeerAddr	= PeerAddr; \
		(pC)->c_fOutbound	= fOutbound; \
		(pC)->c_fMalloc	= fMalloc; \
		(pC)->c_fFree	= fFree; })

#ifdef	_SENDER_
#define	ChSend( pC, pM, dlnk )	QueuePostEntry( &(pC)->c_Queue, pM, dlnk )
#else
#define	ChSend( pC, pM, dlnk ) \
	({if( !(pC)->c_Status & CHSEND_STOP )	\
			((pC)->c_fOutbound)( pC, pM ); \
		((pC)->c_fFree)( pM );})
#endif

#define	ChOutbound( pC, dlnk )	\
	({msg_t* pM = NULL; QueueWaitEntry( &(pC)->c_Queue, pM, dlnk ); \
		if( !(pC)->c_Status & CHSEND_STOP )	\
			((pC)->c_fOutbound)( pC, pM ); \
		((pC)->c_fFree)( pM );})

#define	ChOutboundStop( pC )	((pC)->c_Status	|= CHSEND_STOP )
#define	ChOutboundRestart( pC )	((pC)->c_Status	&= ~CHSEND_STOP )
#define	ChOutboundDown( pC )	((pC)->c_Status	|= CHSEND_DOWN )
#define	isChOutboundDown( pc )	((pc)->c_Status & CHSEND_DOWN )

/**
 *	Communication
 */
#ifdef	VISUAL_CPP

#define	SEND_STREAM_BY_FUNC( Fd, pData, Len, fSend ) \
	int	n; \
	while( Len > 0 ) { \
		n = (fSend)( Fd, pData, Len, 0 ); \
		if( n < 0 )	return( n ); \
		if( n == 0 )	return( -1 ); \
		pData	+= n; \
		Len		-= n; \
	} \
	return( 0 );
#else

#define	SEND_STREAM_BY_FUNC( Fd, pData, Len, fSend ) \
	int	n; \
	while( Len > 0 ) { \
		n = (fSend)( Fd, pData, Len, MSG_NOSIGNAL ); \
		if( n < 0 )	return( n ); \
		if( n == 0 )	return( -1 ); \
		pData	+= n; \
		Len		-= n; \
	} \
	return( 0 );
#endif

#define	PEEK_STREAM_BY_FUNC( Fd, pData, Len, fRecv ) \
	int	n; \
	do { \
		n = (fRecv)( Fd, pData, Len, MSG_PEEK | MSG_NOSIGNAL ); \
		if( n < 0 )	return( n ); \
		if( n == 0 )	return( -1 ); \
	} while( n != Len ); \
	return( 0 );

#define	RECV_STREAM_BY_FUNC( Fd, pData, Len, fRecv ) \
	int	n, total = 0; \
	while( Len > 0 ) { \
		n = (fRecv)( Fd, pData, Len, 0 | MSG_NOSIGNAL ); \
		if( n < 0 )	return( n ); \
		if( n == 0 ) return(total > 0 ? total : -1 ); \
		pData	+= n; \
		Len		-= n; \
		total	+= n; \
	} \
	return( 0 );

static inline int
SendStreamByFunc( void* pFd, char *pData, int Len, 
					int(*fSend)(void*,char*,size_t,int) )
{
	SEND_STREAM_BY_FUNC( pFd, pData, Len, fSend );
}

static inline int
RecvStreamByFunc( void* pFd, char *pData, int Len, 
					int(*fRecv)(void*,char*,size_t,int) )
{
	RECV_STREAM_BY_FUNC( pFd, pData, Len, fRecv );
}

static inline int
PeekStreamByFunc( void* pFd, char *pData, int Len, 
						int(*fRecv)(void*,char*,size_t,int) )
{
	PEEK_STREAM_BY_FUNC( pFd, pData, Len, fRecv );
}

static inline int
SendStream( int Fd, char *pData, int Len )
{
	SEND_STREAM_BY_FUNC( Fd, pData, Len, send );
}

static inline int
RecvStream( int Fd, char *pData, int Len )
{
	RECV_STREAM_BY_FUNC( Fd, pData, Len, recv );
}

static inline int
PeekStream( int Fd, char *pData, int Len )
{
	PEEK_STREAM_BY_FUNC( Fd, pData, Len, recv );
}

static inline int
ReadStream( int Fd, void *pData, int Len )
{
	int	n, total = 0;
	while( Len > 0 ) {
		n = read( Fd, pData, Len );
		if( n < 0 )	return( n );
		if( n == 0 ) return(total > 0 ? total : -1 );
		pData = (char*)pData + n;//  pData	+= n;
		Len		-= n;
		total	+= n;
	}
	return( total );
}

static inline int
WriteStream( int Fd, void *pData, int Len )
{
	int	n, total = 0;
	while( Len > 0 ) {
		n = write( Fd, pData, Len );
		if( n < 0 )	return( n );
		if( n == 0 ) return( -1 );
		pData = (char*)pData + n;//  pData	+= n;
		Len		-= n;
		total	+= n;
	}
	return( total );
}

/*
 *	ベクトル配列型メッセージ操作
 */
typedef struct MsgVec {

#define	VEC_HEAD		0x1
#define	VEC_REPLY		0x2
#define	VEC_MINE		0x100
#define	VEC_DISABLED	0x200

	int			v_Flags;
	void		*v_pVoid;
	int			v_Size;
	char		*v_pStart;
	int			v_Len;
	uint64_t	v_CheckSum;
} MsgVec_t;

typedef struct Msg {
	list_t		m_Lnk;
	void		*m_pTag0;
	void		*m_pTag1;
	void		*m_pTag2;
	timespec_t	m_Timespec;
	int			m_Err;
	void*		(*m_fMalloc)(size_t);
	void		(*m_fFree)(void*);
	int			m_Size;
	int			m_N;
	int			m_CurIndex;
	MsgVec_t	*m_aVec;
	int			m_Cnt;
	struct Msg	*m_pOrg;
	uint64_t	m_CheckSum;
} Msg_t;

#define	MsgVecSet( pVec, mine, pVoid, Size, Len ) \
	{(pVec)->v_Flags = (mine); \
	(pVec)->v_pVoid = (void*)(pVoid); \
	(pVec)->v_pStart =(char*)(pVoid); \
	(pVec)->v_Size = ((int)(Size));(pVec)->v_Len = ((int)(Len));}

#define	MsgVecTrim( pVec, h ) \
	{(pVec)->v_pStart += h;(pVec)->v_Len -= h; \
	if((pVec)->v_Len <= 0 ) (pVec)->v_Flags |= VEC_DISABLED;}

extern	Msg_t*	MsgCreate(int s,void*(*fMalloc)(size_t),void(*fFree)(void*));
extern	int 	MsgDestroy( Msg_t* pM );
extern	Msg_t* 	MsgClone( Msg_t* pM );

#define	MsgMalloc( pM, s )	((pM)->m_fMalloc)( s )
#define	MsgFree( pM, p )	((pM)->m_fFree)( p )

extern	int 	MsgAdd( Msg_t* pM, MsgVec_t* pVec );
extern	int 	MsgInsert( Msg_t* pM, MsgVec_t* pVec );
extern	int 	MsgAddDel( Msg_t* pM, Msg_t *pAddDel );

extern	int		MsgTotalLen( Msg_t *pM );
extern	int		MsgCopyToBuf( Msg_t *pM, char *pBuf, size_t Size );

extern	char*	MsgFirst( Msg_t *pM );
extern	char*	MsgNext( Msg_t *pM );
extern	int		MsgLen( Msg_t *pM );
extern	char*	MsgTrim( Msg_t *pM, int h );
extern	uint64_t	MsgCksum64( Msg_t *pM, int Start, int N, int *pR );
#ifdef	ZZZ
#else
extern	int		MsgSendByFd( int Fd, Msg_t *pMsg );
#endif

/*
 *	スタックを使用せずに、関数列で実行する手法
 */
typedef	void*(*MsgFunc_t)(Msg_t*);

extern	int	MsgEngine( Msg_t *pM, MsgFunc_t fStart );
extern	int MsgPushFunc( Msg_t *pMsg, MsgFunc_t fFunc, void *pArg );
#define	MSG_FUNC_ARG( pMsg )	((pMsg)->m_pTag2)
extern	MsgFunc_t MsgPopFunc( Msg_t *pMsg );

/*
 *	ReadWiteの仮想型
 */
typedef struct IOReadWrite {
	int		(*IO_Read)( void* This, void*, int );
	int		(*IO_Write)( void* This, void*, int );
	int32_t	Residual;
} IOReadWrite_t;

#define	IORead( pIO, pBuff, Len ) \
		((pIO)->IO_Read)( (pIO), (pBuff), (Len) )
#define	IOWrite( pIO, pBuff, Len ) \
		((pIO)->IO_Write)( (pIO), (pBuff), (Len) )

#define	IO_MARSHAL_END	-1

#ifdef	VISUAL_CPP

inline int
IOMarshal( IOReadWrite_t *pIO, char *pBuff, int Len )
{
	int32_t _l = Len;
	int	Ret;

	Ret = (*pIO->IO_Write)( pIO, &_l, sizeof(_l) );
	Ret = (*pIO->IO_Write)( pIO, pBuff, Len );

	return( Ret );
}

inline int
IOMarshalEnd( IOReadWrite_t *pIO )
{
	int32_t	_end = IO_MARSHAL_END;
	int Ret;

	Ret = (*pIO->IO_Write)( pIO, &_end, sizeof(_end) );

	return( Ret );
}

inline int
IOUnmarshal( IOReadWrite_t *pIO, char *pBuff, int Len )
{
	int _n, _ret;

	if( pIO->Residual <= 0 ) {
		(*pIO->IO_Read)((pIO),&pIO->Residual,sizeof(pIO->Residual));
	}
	if(pIO->Residual == IO_MARSHAL_END )	_ret = -1;
	else {
		_n = ( Len > pIO->Residual ? pIO->Residual : Len );
		_ret = (*pIO->IO_Read)( pIO, pBuff, _n );
		pIO->Residual -= _n;
	} 
	return( _ret );
}

#else

#define	IOMarshal( pIO, pBuff, Len ) \
		({int32_t _l = (Len); \
		((pIO)->IO_Write)( (pIO), &_l, sizeof(_l) ); \
		((pIO)->IO_Write)( (pIO), (pBuff), (Len) );})
#define	IOMarshalEnd( pIO ) \
		({int32_t	_end = IO_MARSHAL_END; \
		((pIO)->IO_Write)( (pIO), &_end, sizeof(_end) );})

#define	IOUnmarshal( pIO, pBuff, Len ) \
		({int _n, _ret; \
		if( (pIO)->Residual <= 0 ) \
			((pIO)->IO_Read)((pIO),&(pIO)->Residual,sizeof((pIO)->Residual)); \
		if((pIO)->Residual == IO_MARSHAL_END )	_ret = -1; \
		else { \
			_n = ( (Len) > (pIO)->Residual ? (pIO)->Residual : (Len) ); \
			_ret = ((pIO)->IO_Read)( (pIO), (pBuff), _n ); \
			(pIO)->Residual -= _n; \
		} _ret;})
#endif

/* struct FILE */
typedef struct IOFile {
	IOReadWrite_t	f_ReadWrite;
	void*		(*f_fMalloc)(size_t);
	void		(*f_fFree)(void*);
	FILE		*f_pFILE;
} IOFile_t;

#ifndef	mode_t
#define	mode_t int
#endif

extern	IOFile_t*	
IOFileCreate(const char *pathname, int flags, mode_t mode,  
	void*(*fMalloc)(size_t), void(*fFree)(void*));
extern	int			
IOFileDestroy( IOFile_t*, bool_t bSync );

/* socket */
typedef	struct IOSocket {
	IOReadWrite_t	s_ReadWrite;
	int				s_Fd;
} IOSocket_t;

extern	int	IOSocketBind( IOSocket_t*, int fd );

extern char *getHostName(char *sysName,int w,int no);


/*
 *	資源の保護的管理
 *  GET   alloc
 *          |
 *             -set---
 *  write-CACHE-write-
 *  read-      -read-- HD
 *             -unset-
 *          |
 *  PUT    free
 *
 *		複数の人が異なる要素を更新できる。
 *		要素はリスト・ハッシュ管理されている。
 *		前者は要素にread/write排他。
 *		後者はデッドロックを回避するためにカウンタ管理。
 */

typedef	struct GElm {
	list_t		e_Lnk;
	int			e_Cnt;
	RwLock_t	e_RwLock;
} GElm_t;

#define	GElmInit( pE ) \
	{list_init(&(pE)->e_Lnk);(pE)->e_Cnt=0;RwLockInit(&(pE)->e_RwLock); \
		(pE)->e_Cnt=0;}
#define	GElmRwLockW( pE )	RwLockW( &(pE)->e_RwLock )
#define	GElmRwLockR( pE )	RwLockR( &(pE)->e_RwLock )
#define	GElmRwUnlock( pE )	RwUnlock( &(pE)->e_RwLock )

typedef	struct GElmCtrl {
	Mtx_t	ge_Mtx;
	Cnd_t	ge_Cnd;
	list_t	ge_Lnk;
	Hash_t	ge_Hash;
	int		ge_MaxCnt;
	int		ge_Cnt;
	int		ge_Wait;
	void	*ge_pTag;
	GElm_t*	(*ge_fAlloc)(struct GElmCtrl*);
	int		(*ge_fFree)(struct GElmCtrl*,GElm_t*pElm);
	int	(*ge_fSet)(struct GElmCtrl*,GElm_t*,void*,void**,void*,bool_t bLoad);
	int	(*ge_fUnset)(struct GElmCtrl*,GElm_t*pElm,void**,bool_t bSave);
	int		(*ge_fLoop)(struct GElmCtrl*,GElm_t*pElm,void*);
} GElmCtrl_t;

extern int GElmCtrlInit( GElmCtrl_t *pGE, void*pTag,
			GElm_t*(*_fAlloc)(GElmCtrl_t*),
			int(*_fFree)(GElmCtrl_t*,GElm_t*pElm),
			int(*_fSet)(GElmCtrl_t*,GElm_t*pElm,
						void*pKey,void**ppKey,void*pArg,bool_t bLoad),
			int(*_fUnset)(GElmCtrl_t*,GElm_t*pElm,void**pKey,bool_t bSave),
			int(*_fLoop)(GElmCtrl_t*,GElm_t*pElm,void*),
			int	MaxCnt,
			int HashSize, 				// 素数
			int(*fHashCode)(void*), int(*fCompare)(void*,void*),
			void(*fPrint)(void*,void*),
			void*(*fMalloc)(size_t), void(*fFree)(void*),
			void(*fDestroy)(Hash_t*,void*) );

extern	GElm_t* GElmGet(GElmCtrl_t *pGE,void *pKey,
							void *pArg, bool_t bAlloc,bool_t bLoad);

extern	int	GElmPut( GElmCtrl_t*, GElm_t* pElm, bool_t bFree, bool_t bSave );
extern	int	GElmPutByKey( GElmCtrl_t*, void* pKey, bool_t bFree, bool_t bSave );

#define	GElmCtrlLock( pGE )			MtxLock( &(pGE)->ge_Mtx )
#define	GElmCtrlUnlock( pGE )		MtxUnlock( &(pGE)->ge_Mtx )
#define	GElmCtrlCndWait( pGE )		CndWait( &(pGE)->ge_Cnd, &(pGE)->ge_Mtx )
#define	GElmCtrlCndBroadcast( pGE )	CndBroadcast( &(pGE)->ge_Cnd )

extern	int GElmLoop( GElmCtrl_t *pGE, list_t *pHead, int off, void* pVoid);
#ifdef	ZZZ
#else
extern	int GElmFlush( GElmCtrl_t *pGE, list_t *pHead, int off, 
								void* pVoid, bool_t bSave );
#endif

extern	GElm_t* _GElmAlloc( GElmCtrl_t *pGE );
extern	void _GElmFree( GElmCtrl_t *pGE, GElm_t *pElm );
extern	int _GElmSet( GElmCtrl_t *pGE, GElm_t *pElm, void *pKey, 
							void *pArg, bool_t bLoad );
extern	int _GElmUnset( GElmCtrl_t *pGE, GElm_t *pElm, bool_t bSave );

/*
 *	Fd event
 */
#define	FD_EVENT_MAX	100

#ifdef	_NOT_LIBEVENT_

//	epollによるイベントループ
typedef struct FdEventCtrl {
	int			ct_EpollFd;
	Mtx_t		ct_Mtx;
	int			ct_Cnt;
	Hash_t		ct_Hash;
	list_t		ct_Lnk;
	pthread_t	ct_Th;
} FdEventCtrl_t;

typedef enum FdEvMode {
	EV_LOOP = 0,
	EV_SUSPEND,
	EV_RESUME
} FdEvMode_t;

#define	FdEvCnt( pCtrl )	((pCtrl)->ct_Cnt)

#define	FdEvCtLock( pC )		MtxLock( &(pC)->ct_Mtx )
#define	FdEvCtUnlock( pC )		MtxUnlock( &(pC)->ct_Mtx )

struct	FdEvent;
typedef	int	(*FdEventHandler_t)(struct FdEvent*, FdEvMode_t );
	
typedef struct FdEvent {
	list_t				fd_Lnk;
	FdEventCtrl_t		*fd_pFdEvCt;
	int					fd_Type;
	int					fd_Fd;
	int					fd_Events;
	void*				fd_pArg;
	FdEventHandler_t	fd_fHandler;
	struct epoll_event	fd_ev;
	int					fd_errno;
} FdEvent_t;

#else	// _NOT_LIBEVENT_

typedef struct FdEventCtrl {
	struct event_base	*ct_pEvBase;
	Mtx_t				ct_Mtx;
	int					ct_Cnt;
	Cnd_t				ct_Cnd;
	HashList_t			ct_HashLnk;
	pthread_t			ct_Th;
} FdEventCtrl_t;

typedef enum FdEvMode {
	EV_LOOP = 0,
	EV_SUSPEND,
	EV_RESUME
} FdEvMode_t;

#define	FdEvCnt( pCtrl )	((pCtrl)->ct_Cnt)

#define	FdEvCtLock( pC )		MtxLock( &(pC)->ct_Mtx )
#define	FdEvCtUnlock( pC )		MtxUnlock( &(pC)->ct_Mtx )

struct	FdEvent;
typedef	int	(*FdEventHandler_t)(struct FdEvent*, FdEvMode_t );
	
typedef struct FdEvent {
	list_t			fd_Lnk;
	FdEventCtrl_t*		fd_pFdEvCt;
	int			fd_Type;
	int			fd_Fd;
	void*			fd_pArg;
	uint64_t		fd_Key64;
	int			fd_errno;

	FdEventHandler_t	fd_fHandler;
	int			fd_Events;
	struct event		fd_Event;
} FdEvent_t;

/*
 *	Ret = 0	 : data event
 *		= -1 : timeout event
 */
extern	int FdEventWait( int Fd, struct timeval *pTimeout );

#endif	// _NOT_LIBEVENT_

extern	int	FdEventCtrlCreate( FdEventCtrl_t* );
extern	int	FdEventCtrlDestroy( FdEventCtrl_t* );

extern	int	FdEventInit( FdEvent_t* pEv, int Type, int Fd, int Events,
			void* pArg, int (*fHandler)(FdEvent_t *pEv, FdEvMode_t Mode) );
extern	int FdEventAdd( FdEventCtrl_t*, uint64_t Key, FdEvent_t* );
#ifdef	ZZZ
extern	int FdEventMod( FdEvent_t* );
#endif
extern	int FdEventDel( FdEvent_t* );

extern	int	FdEventSuspend( FdEventCtrl_t* );
extern	int	FdEventResume( FdEventCtrl_t*, FdEvent_t* ); 

extern	int FdEventLoop( FdEventCtrl_t *pFdEvCt, int msec );

#endif	//_NWGADGET_


#ifndef	_IN_CKSUM_
#define	_IN_CKSUM_

/***
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
***/

/*
 *	Byte order independent because of 1's complement
 */
extern	uint64_t in_sum64( void *addr, int len, int r );
extern	uint64_t in_cksum64( void *addr, int len , int r);
extern	uint64_t in_cksum64_update(uint64_t cksum,
					void *op,int ol, int or, void *np, int nl, int nr );
extern	uint64_t in_cksum64_sub_add( uint64_t cksum, 
						uint64_t sub, uint64_t add );

extern	unsigned short
in_cksum( unsigned short *addr, int len );
/*
 *	Old part is removed and new part is added.
 */
extern	unsigned short
in_cksum_update( unsigned short cksum, unsigned short *op, int ol,
			unsigned short *np, int nl );
/*
 *	pseudo header update
 */
extern	unsigned short
in_cksum_pseudo( unsigned short o_cksum, 
		unsigned int o_src, unsigned int o_dst,
		unsigned int n_src, unsigned int n_dst, 
		unsigned short t_len, unsigned char protocol );

typedef struct {
	unsigned int	src_ip;
	unsigned int	dst_ip;
	unsigned char	reserve;
	unsigned char	protocol;
	unsigned short	tot_len;
} pseudo_hd_t;	// TCP,UDP

#endif	//_IN_CKSUM_

#ifdef	ZZZ
#else

#ifndef	_MD5__
#define	_MD5__

#include	<openssl/md5.h>

static inline	void	
MD5Hash( unsigned char *pBuff, long Size, unsigned char *pHash )
{
	MD5_CTX	ctx;

	MD5_Init( &ctx );
	MD5_Update( &ctx, pBuff, Size );
	MD5_Final( pHash, &ctx );
}

static inline	void	
MD5HashStr( unsigned char *pBuff, long Size, unsigned char *pHash )
{
	unsigned char	Hash[MD5_DIGEST_LENGTH];
	int		i;

	MD5Hash( pBuff, Size, Hash );
	for( i = 0; i < MD5_DIGEST_LENGTH; i++ ) {
		sprintf( (char*)&pHash[2*i], "%.2x", Hash[i] );
	}
}

#endif	//_MD5_


#endif
