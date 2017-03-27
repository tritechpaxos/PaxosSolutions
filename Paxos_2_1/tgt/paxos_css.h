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

#ifndef __PAXOS_PSS_H__
#define __PAXOS_PSS_H__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>
//#include <linux/dirent.h>
#include <dirent.h>
#include <pthread.h>
typedef enum css_event_type {
	CSS_EVENT_DIR_CREATE = 1,
	CSS_EVENT_DIR_UPDATE,
	CSS_EVENT_DIR_DELETE,
	CSS_EVENT_LOCK,
	CSS_EVENT_QUEUE_WAIT,
	CSS_EVENT_QUEUE_POST,
} css_event_t;

typedef struct css_event_name {
	css_event_t ce_type;
	char ce_name[1024];
} css_event_name_t;

void *paxos_session_open(char *cell, int session_no , int is_sync);
void paxos_session_close(void * pSession);
void *paxos_session_get_client_id(void *pSession);
void *css_open(void *pSession, char * name,int is_ephemeral);
int css_close(void *pFile);

int css_lock(void *pSession, char *lockname,int is_write);
int css_unlock(void *pSession, char *lockname);

int css_locked_create(void *pSession, char *name, int flags, char *lockname);
ssize_t css_write(void * pFile, void* vbuf, off_t offset ,size_t length);
ssize_t css_read(void * pFile, void* vbuf, off_t offset ,size_t length);
ssize_t css_locked_write(void* pFile, void* buf,
				off_t offset, size_t length,
				char *lockname);
ssize_t css_locked_read(void* pFile, void* buf,
			       off_t offset, size_t length,
			       char *lockname);
int css_locked_delete( void *pSession,char *path, char *lockname );
int css_stat(void *pSession, char *name, struct stat * pStat);
int css_truncate(void *pSession, char *name, off_t Offset);
int css_mkdir(void *pSession, char *path);
int css_r_mkdir(void *pSession, char *path);
int css_rmdir(void *pSession, char *path);
int css_deleteall( void *pSession,char *path );
int css_readdir(void *pSession, char *path, struct dirent *dirent,
		int index, int num );
int css_getevent(void* pSession, int *pCnt, int *pOmitted, 
			css_event_name_t *pEvent);
int css_seteventdir(void* pSession, char* name, void *pESession);
int css_canceleventdir(void* pSession, char* name, void *pESession);
#if PAXOS_VER >= 2
int css_copy( void *pSession, char *fromPath, char *toPath);
int css_concat( void *pSession, char *fromPath, char *toPath);
int css_trylock( void* pSession, char *lockname,int is_write );
#endif

#endif /* __PAXOS_PSS_H__ */
