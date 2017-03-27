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

//#define _GNU_SOURCE < set make parameter
#include <poll.h>
#include <ctype.h>

#include "paxos_css.h"
#include "PFS.h"

//#define DEBUG

typedef struct {
	list_t mtx_list;
	pthread_mutex_t mtx_mutex;
	int mtx_count;
	char mtx_key[PATH_MAX + 1];
} css_mutex_t;

typedef struct {
	pthread_mutex_t pf_mtx;
	struct File *pf_fp;
} css_fp_t;

#ifdef DEBUG
static int debug = 1;
#else
static int debug = 0;
#endif

LIST_HEAD(css_mtx_feelist);

struct {
	Hash_t g_hash;
	int g_init;
} css_global;

static pthread_mutex_t css_mtx_lock = PTHREAD_MUTEX_INITIALIZER;

#define eprintf(fmt, args...)						\
do {									\
	fprintf(stderr, "%s %d: " fmt, __FUNCTION__, __LINE__, ##args);	\
} while (0)

#define dprintf(fmt, args...)						\
do {									\
	if (debug)							\
		fprintf(stderr, "%s %d: " fmt,				\
			__FUNCTION__, __LINE__, ##args);		\
} while (0)

/**
 *
 * mutex operation
 *
 */
static int css_mutex_lock( char *lockname )
{
	int ret = 0;
	if ( (ret = pthread_mutex_lock( &css_mtx_lock ) )) {
		eprintf("css_mtx_lock lock error\n");
		return ret;
	}
	if ( css_global.g_init == 0 ) {
		HashInit( &css_global.g_hash,PRIMARY_101,
			HASH_CODE_STR,HASH_CMP_STR,NULL,NULL,NULL,NULL);
		css_global.g_init = 1;
	}
	
	css_mutex_t *mtx = (css_mutex_t*)HashGet(&css_global.g_hash,lockname);
	if ( mtx == NULL ) {
		mtx = (css_mutex_t *)list_first(&css_mtx_feelist);
		if ( mtx == NULL ) {
			mtx = (css_mutex_t *)calloc(1,sizeof(css_mutex_t));
			if ( mtx == NULL ) {
				int err = errno;
				eprintf("calloc(css_mutex_t) error(%d)\n",
					err);
				ret = err;
				goto err;
			}
			pthread_mutex_init(&mtx->mtx_mutex,NULL);
			list_init((list_t *)mtx);
		}  else {
			list_del_init((list_t *)mtx);
		}
		mtx->mtx_count = 0;
		strcpy(mtx->mtx_key,lockname);
		HashPut(&css_global.g_hash,mtx->mtx_key,mtx);
	}
	mtx->mtx_count++;
	dprintf("lock [%s] mutex[0x%p],thread[%ld],\n",
		lockname,&mtx->mtx_mutex,pthread_self());
	pthread_mutex_unlock( &css_mtx_lock );
	pthread_mutex_lock( &mtx->mtx_mutex );
	return ret;
 err:
	pthread_mutex_unlock( &css_mtx_lock );
	return ret;
}

static int css_mutex_unlock( char *lockname )
{
	int ret = 0;
	if ( (ret = pthread_mutex_lock( &css_mtx_lock )) ) {
		eprintf("css_mtx_lock lock error\n");
		return ret;
	}
	if ( css_global.g_init == 0 ) {
		eprintf("css_mtx_lock not init\n");
		ret = -1;
		goto out;
	}
	css_mutex_t *mtx = (css_mutex_t*)HashGet(&css_global.g_hash,lockname);
	if ( mtx == NULL ) {
		eprintf("HashGet(%s) not found.\n",lockname);
		ret = -1;
		goto out;
	}
	dprintf("unlock [%s] mutex[0x%p],thread[%ld],\n",
		lockname,&mtx->mtx_mutex,pthread_self());
	pthread_mutex_unlock( &mtx->mtx_mutex );
	mtx->mtx_count--;
	if ( mtx->mtx_count == 0 ) {
		HashRemove(&css_global.g_hash,lockname);
		list_add_tail((list_t *)mtx,&css_mtx_feelist);
	}
 out:
	pthread_mutex_unlock( &css_mtx_lock );
	return ret;
}


/**
 *
 * PFS fuction define
 *
 */
static void*
PFSConnect( struct Session* pSession, int j )
{
	int	fd;
	PaxosSessionCell_t*	pCell;

	pCell	= PaxosSessionGetCell( pSession );

	if( (fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		return( NULL );
	}

#ifdef VER_0
	if( connect( fd, (struct sockaddr*)&pCell->c_aPeerAddr[j], 
		     sizeof(struct sockaddr_in)) < 0 ) {
#else
	if( connect( fd, (struct sockaddr*)&pCell->c_aPeerAddr[j].a_Addr, 
		     sizeof(struct sockaddr_in)) < 0 ) {
#endif
		close( fd );
		return( NULL );
	}
	return( (void*)(uint64_t)fd );
}

static int
PFSShutdown( void* pVoid, int how )
{
	return( shutdown( (int)(uint64_t)pVoid, how ) );
}

static int
PFSCloseFd( void* pVoid )
{
	return( close( (int)(uint64_t)pVoid ) );
}

static int
PFSGetMyAddr( void* pVoid, struct sockaddr_in* pMyAddr )
{
	socklen_t 	len = sizeof(struct sockaddr_in);

	getsockname( (int)(uint64_t)pVoid, (struct sockaddr*)pMyAddr, &len );
	return( 0 );
}

static int
PFSSendTo( void* pVoid, char* pBuf, size_t Len, int Flags )
{
	int	Ret = 0;
	int	len = 0;
	int	count = 0;
	int fd = (int)(uint64_t)pVoid;

	Flags |= MSG_DONTWAIT;
	while ( Ret < Len ) {
		struct pollfd fds[1] = { {fd,POLLOUT|POLLHUP|POLLERR|POLLNVAL,0}};
		int err;
		if ( (err = poll(fds,1,-1)) <= 0) {
			if ( errno == EINTR ) {
				continue;
			}
			Ret = err;
			break;
		}
		if ( fds[0].revents & POLLOUT ) {
			len = send( fd, pBuf + Ret , Len - Ret, 
				    Flags|MSG_NOSIGNAL );
			if ( len < 0 ) {
				if ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
					continue;
				}
				count++;
				if ( errno == EPIPE ) {
					Ret = len;
					break;
				}
				if ( Ret == 0 ) {
					Ret = len;
				}
				break;
			}
		}
		if ( fds[0].revents & (POLLHUP|POLLERR|POLLNVAL) ) {
			Ret = 0;
			break;
		}
		Ret += len;
	}
	return( Ret );
}
static int
PFSRecvFrom( void* pVoid, char* pBuf, size_t Len, int Flags )
{
	size_t len = Len;
	char *p = pBuf;
	int Ret = 0;
	int fd = (int)(uint64_t)pVoid;

	Flags |= MSG_DONTWAIT;
	while ( len > 0 ) {
		struct pollfd fds[1] = { {fd,POLLIN|POLLRDHUP|POLLNVAL,0}};
		int err;
		if ( (err = poll(fds,1,-1)) <= 0) {
			if ( errno == EINTR ) {
				continue;
			}
			Ret = err;
			break;
		}
		if ( fds[0].revents & POLLIN ) {
			int l  = recv( fd, p, len, Flags );
			
			if ( l <= 0 && !( l == 0 && errno == EINTR ) ) {
				Ret = l;
#if 0
				if (l < 0 && errno == ECONNRESET ) {

					Ret = 0;
				}
#endif
				break;
			}
			if ( ( Flags & MSG_PEEK ) && ( l != len ) ) {
				l = 0;
			}
			p += l;
			len -= l;
			Ret += l;
		}
		if ( fds[0].revents & (POLLRDHUP|POLLNVAL) ) {
			Ret = 0;
			break;
		}
	}
	return( Ret );
}
static char  _DirEventCreateName[1024][1024];
static int _DirEventCreateNum = 0;
static int _DirEventCreateMax = sizeof(_DirEventCreateName)/
		sizeof(_DirEventCreateName[0]);
static char  _DirEventDeleteName[1024][1024];
static int _DirEventDeleteNum = 0;
static int _DirEventDeleteMax = sizeof(_DirEventDeleteName)/
		sizeof(_DirEventDeleteName[0]);
static char  _DirEventUpdateName[1024][1024];
static int _DirEventUpdateNum = 0;
static int _DirEventUpdateMax = sizeof(_DirEventUpdateName)/
		sizeof(_DirEventUpdateName[0]);
static char  _LockEventName[1024][1024];
static int _LockEventNum = 0;
static int _LockEventMax = sizeof(_LockEventName)/
		sizeof(_LockEventName[0]);
static char  _QueueEventWaitName[1024][1024];
static int _QueueEventWaitNum = 0;
static int _QueueEventWaitMax = sizeof(_QueueEventWaitName)/
		sizeof(_QueueEventWaitName[0]);
static char  _QueueEventPostName[1024][1024];
static int _QueueEventPostNum = 0;
static int _QueueEventPostMax = sizeof(_QueueEventPostName)/
		sizeof(_QueueEventPostName[0]);

static int
PFSEventDirCallback( struct Session* pSession, PFSEventDir_t* pEvent )
{
	dprintf("PFSEventDir:DirEntryMode(%s,%s,%s)\n",
			pEvent->ed_Name,
			pEvent->ed_Entry,
			pEvent->ed_Mode==DIR_ENTRY_CREATE?"Created":
			(pEvent->ed_Mode==DIR_ENTRY_DELETE?"Deleted":
			 (pEvent->ed_Mode==DIR_ENTRY_UPDATE?"Updated":"Unkwon")));
	switch (pEvent->ed_Mode) {
	default:
		break;
	case DIR_ENTRY_CREATE:
		if ( _DirEventCreateNum < _DirEventCreateMax ) {
			sprintf(_DirEventCreateName[_DirEventCreateNum],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		} else {
			sprintf(_DirEventCreateName[_DirEventCreateNum % 
					_DirEventCreateMax ],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		}
		_DirEventCreateNum++;
		break;
	case DIR_ENTRY_DELETE:
		if ( _DirEventDeleteNum < _DirEventDeleteMax ) {
			sprintf(_DirEventDeleteName[_DirEventDeleteNum],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		} else {
			sprintf(_DirEventDeleteName[_DirEventDeleteNum % 
					_DirEventDeleteMax ],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		}
		_DirEventDeleteNum++;
		break;
	case DIR_ENTRY_UPDATE:
		if ( _DirEventUpdateNum < _DirEventUpdateMax ) {
			sprintf(_DirEventUpdateName[_DirEventUpdateNum],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		} else {
			sprintf(_DirEventUpdateName[_DirEventUpdateNum % 
					_DirEventUpdateMax ],
				"%s/%s",pEvent->ed_Name,
				pEvent->ed_Entry);
		}
		_DirEventUpdateNum++;
		break;
	}
	return 0;
}
static int
PFSEventLockCallback( struct Session* pSession, PFSEventLock_t* pEvent )
{
	dprintf("PFSEventLock:LockName[%s]\n", pEvent->el_Name );
	if ( _LockEventNum < _LockEventMax ) {
		strcpy(_LockEventName[_LockEventNum],
			pEvent->el_Name);
	} else {
		strcpy(_LockEventName[_LockEventNum % 
				_LockEventMax ],
			pEvent->el_Name);
	}
	_LockEventNum ++;
	return 0;
}
static int
PFSEventQueueCallback( struct Session* pSession, PFSEventQueue_t* pEvent )
{
	dprintf("PFSEventQueue:QueueName[%s] [%s]\n", pEvent->eq_Name,
		       pEvent->eq_PostWait?"Wait":"Post");
	if (pEvent->eq_PostWait) {
		if ( _QueueEventWaitNum < _QueueEventWaitMax ) {
			strcpy(_QueueEventWaitName[_QueueEventWaitNum],
				pEvent->eq_Name);
		} else {
			strcpy(_QueueEventWaitName[_QueueEventWaitNum % 
					_QueueEventWaitMax ],
				pEvent->eq_Name);
		}
		_QueueEventWaitNum++;
	} else {
		if ( _QueueEventPostNum < _QueueEventPostMax ) {
			strcpy(_QueueEventPostName[_QueueEventPostNum],
				pEvent->eq_Name);
		} else {
			strcpy(_QueueEventPostName[_QueueEventPostNum % 
					_QueueEventPostMax ],
				pEvent->eq_Name);
		}
		_QueueEventPostNum++;
	}
	return 0;
}
static int
PFSEventCallback( struct Session *pSession, PFSHead_t *pF )
{
	switch( (int)pF->h_Head.h_Cmd ) {
		case PFS_EVENT_DIR:
			PFSEventDirCallback(pSession,(PFSEventDir_t*)pF);
			break;

		case PFS_EVENT_LOCK:
			PFSEventLockCallback(pSession,(PFSEventLock_t*)pF );
			break;

		case PFS_EVENT_QUEUE:
			PFSEventQueueCallback(pSession,(PFSEventQueue_t*)pF );
			break;

		default: 
			eprintf("cmd(%d,0x%x) is invalid.\n",
					(int)pF->h_Head.h_Cmd,
					(int)pF->h_Head.h_Cmd	       );
			break;
	}
	return( 0 );
}

void *
paxos_session_open(char *cellname, int session_no, int is_sync)
{
	struct addrinfo *result = NULL;
#if PAXOS_VER < 2
	PaxosSessionCell_t cell;
#endif
	struct Session *pSession = NULL; 
	int sys_rtn;
	int serr;

#if PAXOS_VER < 2

	PaxosGetCellAddr( cellname, &cell, NULL ); 

	pSession = calloc(1,(size_t) PaxosSessionGetSize());

	if ( pSession == NULL ) {
		eprintf("memory allocate error.\n");
		goto err;
	}
	sys_rtn = PaxosSessionInit( pSession,
			PFSConnect, PFSShutdown, PFSCloseFd, PFSGetMyAddr,
			PFSSendTo, PFSRecvFrom,
			NULL, NULL, &cell );

	if ( sys_rtn != 0 ) {
		eprintf("PaxosSessionInit error(%d,%s).\n",errno,strerror(errno));
		goto err;
	}
#else
	pSession = PaxosSessionAlloc( PFSConnect, PFSShutdown, PFSCloseFd,
			PFSGetMyAddr,PFSSendTo,PFSRecvFrom, NULL, NULL,
			cellname);
	if ( pSession == NULL ) {
		eprintf("PaxosSessionAlloc error.\n");
		goto err;
	}
#endif
	if( getenv("LOG_SIZE") ) {
		static struct Log* sessionLog = NULL;
		if ( sessionLog == NULL ) {
			int	log_size;
			log_size = atoi( getenv("LOG_SIZE") );

			PaxosSessionLogCreate( pSession, log_size, LOG_FLAG_BINARY, stderr );
			sessionLog = PaxosSessionGetLog( pSession );
		} else {
			PaxosSessionSetLog( pSession, sessionLog );
		}
	}
	sys_rtn = PaxosSessionOpen(pSession,session_no,(bool_t)is_sync);
	if ( sys_rtn != 0 ) {
		eprintf("PaxosSessionOpen error(%d,%s).\n",errno,strerror(errno));
		goto err;
	}
 out:
	return (void *)pSession;
 err:
	serr= errno;
	if ( result ) {
		freeaddrinfo( result );
		result = NULL;
	}
	if ( pSession ) {
		free(pSession);
		pSession = NULL;
	}
	errno = serr;
	goto out;
}
void *paxos_session_get_client_id(void *pSession)
{
	return (void *)PaxosSessionGetClientId(pSession);
}
void paxos_session_close(void * pSession)
{
	PaxosSessionClose((struct Session *)pSession);
	free(pSession);
}
void *css_open(void *pSession, char * name,int is_ephemeral)
{
	css_fp_t *fp = calloc(1,sizeof(*fp));
	if ( fp == NULL ) {
		eprintf("calloc(%ld),err(%d)", sizeof(*fp),errno);
		goto err;
	}
	
	fp->pf_fp = PFSOpen((struct Session*)pSession, name,is_ephemeral);

	if ( fp->pf_fp == NULL ) {
		eprintf("PFSOpen(%s),err(%d)", name,errno);
		goto err;
	}
	int err = pthread_mutex_init(&fp->pf_mtx,NULL);
	if ( err != 0 ) {
		eprintf("pthread_mutex_init err(%d,%d)", err,errno);
		goto err;
	}
	return fp;
err: 
	if ( fp != NULL ) {
		free(fp);
	}
	return NULL;
}
int css_close(void *pFile)
{
	css_fp_t *fp = (css_fp_t *)pFile;
	int ret = pthread_mutex_destroy(&fp->pf_mtx);
	if ( ret == EBUSY ) {
		return ret;
	}
	ret = PFSClose((File_t*)fp->pf_fp);
	if ( ret != 0 ) {
		return ret;
	}
	free(fp);
	return(ret);
}
int css_lock(void *pSession, char *lockname,int is_write)
{
	int ret = 0;
	css_mutex_lock(lockname);
	if ( is_write ) {
		ret = PFSLockW((struct Session *)pSession, lockname);
	} else {
		ret = PFSLockR((struct Session *)pSession, lockname);
	}
	return ret;
}
int css_unlock(void *pSession, char* lockname)
{
	int ret = 0;
	ret = PFSUnlock((struct Session *)pSession, lockname);
	css_mutex_unlock(lockname);
	return ret;
}
int css_locked_create(void *pSession,char *name, int flags,char *lockname)
{
	int ret = 0;
	if ( lockname ) {
		if ( (ret = css_lock(pSession, lockname, 1)) != 0 ) {
			eprintf("css_lock(%s) error(%d,%s).\n",lockname,errno,strerror(errno));
			goto out;
		}
	}
	ret = PFSCreate((struct Session*)pSession, name,flags);
	if ( lockname ) {
		css_unlock(pSession, lockname);
	}
 out:
	return ret;
}
ssize_t css_write(void* pFile, void* buf, off_t offset, size_t length )
{
	ssize_t ret = 0;
	css_fp_t *fp = (css_fp_t *)pFile;
	ret = pthread_mutex_lock(&fp->pf_mtx);
	if ( ret != 0 ) {
		if ( ret > 0 ) ret = -ret;
		return ret;
	}
	if ( offset != (off_t)-1) {
		ret = PFSLseek(fp->pf_fp, offset);
		if ( ret < 0 ) {
			goto out;
		}
	}
	ret = PFSWrite((File_t*)fp->pf_fp,(char *)buf,length);
	if ( ret < 0 ) {
		eprintf("PFSWrite(%lu,%ld) error(%d,%s).\n",offset,length,errno,strerror(errno));
	}
out:
	pthread_mutex_unlock(&fp->pf_mtx);
	return ret;
}
ssize_t css_read(void* pFile, void* buf, off_t offset, size_t length )
{
	ssize_t ret = 0;
	css_fp_t *fp = (css_fp_t *)pFile;
	ret = pthread_mutex_lock(&fp->pf_mtx);
	if ( ret != 0 ) {
		if ( ret > 0 ) ret = -ret;
		return ret;
	}
	if ( offset != (off_t)-1) {
		ret = PFSLseek(fp->pf_fp, offset);
		if ( ret < 0 ) {
			goto out;
		}
	}
	ret = PFSRead((File_t*)fp->pf_fp,(char *)buf,length);
	if ( ret < 0 ) {
		eprintf("PFSRead(%lu,%ld) error(%d,%s).\n",offset,length,errno,strerror(errno));
	}
out:
	pthread_mutex_unlock(&fp->pf_mtx);
	return ret;
}
ssize_t css_locked_write(void* pFile, void* buf,
				off_t offset, size_t length,
				char *lockname)
{
	ssize_t ret = 0;
	int serr;
	css_fp_t *fp = (css_fp_t *)pFile;
	File_t* pF = (File_t*)fp->pf_fp;
	struct Session* pSession = pF->f_pSession;
	dprintf("lockW(%s)\n",lockname);
	if ( lockname ) {
		if ( (ret = css_lock(pSession, lockname, 1)) != 0 ) {
			eprintf("css_lock(%s) error(%d,%s).\n",lockname,errno,strerror(errno));
			goto out;
		}
	}
	ret = css_write(pFile,buf,offset,length);
	if ( ret != length ) {
		eprintf("css_write(%s,%lu,%ld) error(%d,%s).\n",lockname,offset,length,errno,strerror(errno));
		goto err;
	}
	dprintf("unlock(%s)\n",lockname);
	if ( lockname ) {
		css_unlock(pSession, lockname);
	}
 out:
	return ret;

 err:
	serr = errno;
	if ( lockname ) {
		css_unlock(pSession, lockname);
	}
	errno = serr;
	goto out;
}
ssize_t css_locked_read(void* pFile, void* buf,
			       off_t offset, size_t length,
			       char *lockname)
{
	ssize_t ret = 0;
	int serr;
	css_fp_t *fp = (css_fp_t *)pFile;
	File_t* pF = (File_t*)fp->pf_fp;
	struct Session* pSession = pF->f_pSession;
	if ( lockname ) {
		if ( (ret = css_lock(pSession, lockname, 0)) != 0 ) {
			eprintf("css_lock(%s) error(%d,%s).\n",lockname,errno,strerror(errno));
			goto out;
		}

	}
	ret = css_read(pFile,(char *)buf,offset,length);
	if ( ret < 0 ) {
		eprintf("css_read(%s,%lu,%ld) error(%d,%s).\n",lockname,offset,length,errno,strerror(errno));
		goto err;
	}
	if ( lockname ) {
		css_unlock(pSession, lockname);
	}
 out:
	return ret;
 err:
	serr = errno;
	if ( lockname  ) {
		css_unlock(pSession, lockname);
	}
	errno = serr;
	goto out;
}
int css_locked_delete( void *pSession,char *path, char *lockname )
{
	int ret = 0;
	int serr;
	if ( lockname ) {
		if ( (ret = css_lock(pSession, lockname,1)) != 0 ) {
			eprintf("PFSLockW(%s) error(%d,%s).\n",lockname,errno,strerror(errno));
			goto out;
		}
	}
	if( (ret = PFSDelete((struct Session *)pSession, path ) ) != 0 ) {
		goto err;
	}
	if ( lockname ) {
		css_unlock((struct Session *)pSession, lockname);
	}
 out:
	return ret;
 err:
	serr = errno;
	if ( lockname ) {
		css_unlock((struct Session *)pSession, lockname);
	}
	errno = serr;
	goto out;
}
int css_stat(void *pSession, char *name, struct stat *pStat)
{
	return PFSStat((struct Session *)pSession, name, pStat);
}
int css_truncate(void *pSession, char *name, off_t Offset)
{
	return PFSTruncate((struct Session *)pSession, name, Offset);
}
int css_mkdir(void *pSession, char *path)
{
	return PFSMkdir((struct Session *)pSession, path);
}
int css_r_mkdir(void *pSession, char *path)
{
	int rtn = 0;
	char buf[PATH_NAME_MAX];
	char *dir = buf;
	strcpy(buf,path);
	while (dir) {
		if (*dir == '\0' ) {
			*dir = '/';
		}
		dir++;
		dir = index(dir,'/');
		if (dir) {
			*dir = '\0';
		}
		rtn = PFSMkdir(pSession,buf);
		if ( rtn != 0 && errno == EEXIST ) {
			rtn = 0;
		}
		if (rtn) {
		eprintf("PFSMkdir(%s) error(%u,%s).\n",buf,errno,strerror(errno));
		break;
		}
	}
	return rtn;
}
int css_rmdir(void *pSession, char *path)
{
	return PFSRmdir((struct Session *)pSession, path);
}
int css_deleteall( void *pSession,char *path )
{
	PFSDirent_t dirent[1024];
        char p[PATH_NAME_MAX];
        int i,n;
	//int index;
        int rtn = 0;
        /*
         * get dirent
         */
	//index = 0;
	while ( 1 ) {
		n = sizeof(dirent)/sizeof(dirent[0]);
		if ( PFSReadDir((struct Session *)pSession, path, dirent, 0, &n ) != 0 || n <= 0 ) {
			// regular file
			return PFSDelete( (struct Session *)pSession, path );
		}
		if ( n == 2 ) {
			break;
		}
		/*
		 * recursive delete
		 */
		for ( i = 0; i < n; i++ ) {
			if ( strcmp( dirent[i].de_Name, "." ) == 0 ||
			     strcmp( dirent[i].de_Name, ".." ) == 0  ) {
				continue;
			}
			dprintf("path=%s,ent=%s\n",path,dirent[i].de_Name);
			sprintf( p, "%s/%s", path, dirent[i].de_Name );
			if ( ( rtn = css_deleteall( pSession, p ) ) != 0 ) {
				eprintf("css_deleteall(%s) error\n",p);
				return rtn;
			}
		}
	}
	if ( strcmp(path,".") != 0 ) {
		rtn = PFSRmdir( (struct Session *)pSession, path );
	}
        return rtn;
}
int css_readdir(void *pSession, char *path, struct dirent *dirent,
		int index, int num )
{
	int count = 0;
	PFSDirent_t dent[100];
	int bcount = sizeof(dent)/sizeof(dent[0]);
	int serr;

	while ( num > 0 ) {
		int i;
		int n = (num > bcount)?bcount:num;
		int m = n;
		if ( PFSReadDir((struct Session *)pSession,path,
				dent, index, &n ) != 0 ) {
			eprintf("PFSReadDir(%d,%d->%d) error(%d,%s).\n",
				index,m,n,errno,strerror(errno));
			goto err;
		}
		if ( n == 0 ) {
			break;
		}
		for ( i = 0; i < n; i++ ) {
			strcpy(dirent[count+i].d_name,dent[i].de_Name);
			dirent[count+i].d_reclen = strlen(dirent[count+i].d_name);
			dirent[count+i].d_off = index + i;
		}
		count += n;
		num -= n;
		index += n;
		if ( n < m ) {
			break;
		}
	}

 out:
	return count;
 err:
	serr = errno;
	count = -1;
	errno = serr;
	goto out;
}
int
css_getevent(void* pSession, int *pCnt, int *pOmitted,
	css_event_name_t *pEvent )
{
	int i;
	int ret;
	int count = *pCnt;
	_DirEventCreateNum = _DirEventDeleteNum = _DirEventUpdateNum =
	_LockEventNum = _QueueEventWaitNum = _QueueEventPostNum = 0;
	if ( ( ret = PFSEventGetByCallback(pSession,pCnt,pOmitted,
						PFSEventCallback) ) != 0 ) {
		return ret;
	}
	*pCnt = 0;
	for ( i = 0; i < _DirEventCreateNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_DIR_CREATE;
		strcpy(pEvent[*pCnt].ce_name,_DirEventCreateName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	for ( i = 0; i < _DirEventDeleteNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_DIR_DELETE;
		strcpy(pEvent[*pCnt].ce_name,_DirEventDeleteName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	for ( i = 0; i < _DirEventUpdateNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_DIR_UPDATE;
		strcpy(pEvent[*pCnt].ce_name,_DirEventUpdateName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	for ( i = 0; i < _LockEventNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_LOCK;
		strcpy(pEvent[*pCnt].ce_name,_LockEventName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	for ( i = 0; i < _QueueEventWaitNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_QUEUE_WAIT;
		strcpy(pEvent[*pCnt].ce_name,_QueueEventWaitName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	for ( i = 0; i < _QueueEventPostNum; i++ ) {
		pEvent[*pCnt].ce_type = CSS_EVENT_QUEUE_POST;
		strcpy(pEvent[*pCnt].ce_name,_QueueEventPostName[i]);
		(*pCnt)++;
		if ( *pCnt >= count ) return 0;
	}
	return 0;
}
int
css_seteventdir(void* pSession, char* name, void* pESession)
{
	return PFSEventDirSet((struct Session *)pSession, name,
				(struct Session*) pESession);
}
int
css_canceleventdir(void* pSession, char* name, void* pESession)
{
	return PFSEventDirCancel((struct Session *)pSession, name,
				(struct Session*) pESession);
}
#if PAXOS_VER >= 2
int
css_copy( void *pSession, char *fromPath, char *toPath)
{
	return PFSCopy((struct Session *)pSession, fromPath, toPath);
}
int
css_concat( void *pSession, char *fromPath, char *toPath)
{
	return PFSConcat((struct Session *)pSession, fromPath, toPath);
}
int
css_trylock( void* pSession, char *lockname,int is_write )
{
	int ret = 0;
	css_mutex_lock( lockname );

	if ( is_write ) {
		ret =  PFSTryLockW((struct Session*)pSession,lockname);
	} else {
		ret = PFSTryLockR((struct Session*)pSession,lockname);
	}
	if ( ret != 0 ) {
		css_mutex_unlock( lockname );
	}
	return ret;
}
#endif
