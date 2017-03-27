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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "paxos_css.h"

// multi layers( 64 (6 bits) )
#define LYLERS_BITS (6)

// size xxxK xxM xxG xxT
size_t vol_size;
size_t file_size = 1024 * 1024 * 2;  // 2M
long mask = ~(-1L<<LYLERS_BITS);
char buf[4096];
char *name;
char *cmdName;
int isNone = 1;
int isHole = 1;
int exec_delete = 0;
int exec_size = 0;
char *cell = "css";

size_t getsize(char *av);
void usage(void);
int clearFile(void *pSession,char *file_name, int length);
int createISCSIVol(void);
size_t getISCSIVolSize(void *pSession);

typedef struct {
	char *size_name;
	size_t size_unit;
} sizetbl_t;

sizetbl_t sizetbl[] = {
	{"T",1024L*1024L*1024L*1024L},
	{"G",1024L*1024L*1024L},
	{"M",1024L*1024L},
	{"K",1024L},
	{NULL,1},
};

int
main(int ac, char *av[] )
{
	int rtn = 0;
	int argcount = 3;
	
	cmdName=av[0];
	if ( ac < 2 ) {
		usage();
		goto err;
	}
	cell = av[1];
	ac--;
	av++;

	if ( ac > 1 && strcmp(av[1],"--full") == 0 ) {
		isHole = 0;
        isNone = 1;
		ac--;
		av++;
	}
	if ( ac > 1 && strcmp(av[1],"--hole") == 0 ) {
        isNone = 0;
		ac--;
		av++;
	}
	if ( ac > 1 && strcmp(av[1],"--delete") == 0 ) {
		ac--;
		av++;
		argcount= 2;
		exec_delete=1;
	}
	if ( ac > 1 && strcmp(av[1],"--size") == 0 ) {
		ac--;
		av++;
		argcount= 2;
		exec_size=1;
	}
	if ( ac < argcount ) {
		usage();
		goto err;
	}

	name = av[1];
	if ( exec_delete ) {
		void *pSession;
		if ((pSession = paxos_session_open(cell,0,0))==NULL) {
			goto err;
		}
		css_deleteall(pSession,name);
		paxos_session_close(pSession);
		goto out;
	}
	if ( exec_size ) {
		void *pSession;
		if ((pSession = paxos_session_open(cell,0,0))==NULL) {
			goto err;
		}
		size_t size = getISCSIVolSize(pSession);
		if ( size <= 0 ) {
			goto err;
		}
		sizetbl_t *t = sizetbl;
		while ( t->size_name != NULL ) {
			if ( size / t->size_unit > 0 ) {
				break;
			}
			t++;
		}
		if ( t->size_name != NULL ) {
			printf("cell[%s] volume[%s] size -> %ld%s\n",
				cell,name,size / t->size_unit,
				t->size_name);
		}
		paxos_session_close(pSession);
		goto out;
	}
	if ( (vol_size = getsize(av[2]) ) == (size_t)-1 ) {
		goto err;
	}
	if ( ac > 3 &&  (file_size = getsize(av[3]) ) == (size_t)-1 ) {
		goto err;
	}
	if (file_size >= 1024 * 1024 * 124 ) {
		fprintf(stderr,"file size(%lu) is too large.\n", file_size);
		goto err;
	}
	if ( vol_size % file_size != 0 ) {
		fprintf(stderr,"volume size(%lu) and file size(%lu)"
			"not consistent.\n", vol_size,file_size); 
		goto err;
	}
	rtn = createISCSIVol();
 out:
	return rtn;
 err:
	rtn = -1;
	goto out;
}
int createISCSIVol(void)
{
	void *pSession = NULL;
	int rtn = 0;
	size_t offset;
	int *dirs = NULL;
	int i;
	memset(buf,'\0',sizeof(buf));

	// session open
	if ((pSession = paxos_session_open(cell,0,0))==NULL) {
		fprintf(stderr,"paxos_session_open error(%d,%s)\n",
			errno,strerror(errno));
		goto err;
	}
	if ( css_mkdir(pSession, name) != 0 ) {
		fprintf(stderr,"mkdir(%s) error(%d,%s)\n",
			name,errno,strerror(errno));
		goto err;
	}
	// multi layers( 64 (6 bits) )
	int layers = 0;
	int value = vol_size/file_size;
	while (value) {
		value >>= LYLERS_BITS;
		layers++;
	}
	printf("layers=%d\n",layers);
	if ( ( dirs = (int *)calloc(sizeof(int),layers) ) == NULL ) {
		fprintf(stderr,"calloc(%lu.%d) error(%d,%s)\n",
			sizeof(int),layers,errno,strerror(errno));
		goto err;
	}
	for(i = 0; i < layers; i++ ) {
		dirs[i] = -1;
	}
	offset = 0;
	while (vol_size > offset) {
		int j,l;
		char file_name[256];
		j= offset/file_size;
		sprintf(file_name,"%s",name);
		for ( l= 0; l < layers; l++ ) {
			char num[4];
			int dir = (j >> LYLERS_BITS*(layers-l-1))&mask;
			printf("(%d,%d)->%d\n",j,l,dir);
			sprintf(num,"/%02u",dir);
			strcat(file_name,num);
			if ( l < layers -1 && dirs[l] != dir ) {
				printf("mkdir(%s)\n",file_name);
				if ( css_r_mkdir(pSession, file_name) != 0 ) {
					fprintf(stderr,"mkdir(%s) error(%d,%s)\n",
						file_name,errno,strerror(errno));
					goto err;
				}
				dirs[l] = dir;
			}
		}
		if ( !isNone && clearFile(pSession,file_name,file_size) != 0 ) {
			goto err;
		}
		offset += file_size;
	}
 out:
	if ( dirs != NULL ) {
		free(dirs);
	}
	return rtn;
 err:
	if ( pSession ) {
		paxos_session_close(pSession);
	}
	rtn = -1;
	goto out;
}
int clearFile(void *pSession,char *file_name, int length )
{
	void *pF = NULL;
	int rtn = 0;
	// PFS file open
	if ( ( pF = css_open(pSession,file_name,0) ) == NULL ) {
		fprintf(stderr,"PFS file(%s) open error(%d,%s).\n",
			name,errno,strerror(errno));
		goto err;
	}
	if ( isHole ) {
		if ( css_locked_write(pF,buf,(off_t)-1,1,NULL) != 1 ) {
			fprintf(stderr,"PFS file(%s) write error(%d,%s).\n",
				file_name,errno,strerror(errno));
			goto err;
		}
		css_close(pF);
		pF = NULL;
		printf("truncate (%s) 0\n",file_name);
		if ( css_truncate(pSession,file_name,0) ) {
                        fprintf(stderr,"PFS file(%s,%u) truncate error(%d,%s).\n",
                                file_name,0,errno,strerror(errno));
                        goto err;
		}
		if ( css_truncate(pSession,file_name,length) ) {
                        fprintf(stderr,"PFS file(%s,%u) truncate error(%d,%s).\n",
                                file_name,length,errno,strerror(errno));
                        goto err;
		}
		goto out;
	}
	while ( length > 0 ) {
		int len = ( length > sizeof(buf) )?sizeof(buf):length;
		if ( css_locked_write(pF,buf,(off_t)-1,len,NULL) != len ) {
			fprintf(stderr,"PFS file(%s) write error(%d,%s).\n",
				file_name,errno,strerror(errno));
			goto err;
		}
		length -= len;
	}
 out:
	return rtn;
 err:
	if ( pF ) {
		css_close( pF );
	}
	rtn = -1;
	goto out;
}
void usage(void)
{
	fprintf(stderr,"usage:\t%s cell [--full|--hole] Volume_name num{K|M|G|T} [file_size{K|M}}\n",cmdName);
	fprintf(stderr,"\t%s cell [--delete] Volume_name\n",cmdName);
	fprintf(stderr,"\t%s cell [--size] Volume_name [file_size{K|M}}\n",cmdName);
}
size_t getsize(char *av)
{
	size_t size;
	int num;
	char *unit;

	num = strtoul(av,&unit,0);
	if ( num <= 0 || num > 1024) {
		fprintf(stderr,"factor(%d) is invalid.\n",num);
		goto err;
	}
	if ( ( *unit != 'K' && *unit != 'M' && *unit != 'G' && *unit != 'T' ) || 
	     *(unit + 1 ) != '\0' ) {
		fprintf(stderr,"unit(%s) is invalid.\n",unit);
		goto err;
	}
	size = num;
	switch (*unit) {
	default:
		/////// not reached
		abort();
		break;
	case 'T':
		size *= 1024;
	case 'G':
		size *= 1024;
	case 'M':
		size *= 1024;
	case 'K':
		size *= 1024;
		break;
	}
 out:
	return size;
 err:
	size = (size_t)-1;
	goto out;
}
int
r_getLeafDirNumber(void *pSession,char *dirname)
{
	int i;
	int num = 0;
	struct dirent dirent[66];

	int n = css_readdir(pSession, dirname, dirent, 0, 66);
	if ( n < 0 ) {
		fprintf(stderr,"volname(%s) readdir error(%d,%s).\n",
			name,errno,strerror(errno));
		return 0;
	}
		
	for (i = 0;i < n; i++ ) {
		char path[256];
		struct stat st;
		if ( strcmp(dirent[i].d_name,"..") == 0 ||
			 strcmp(dirent[i].d_name,".") == 0 ) {
			continue;
		}
		sprintf(path,"%s/%s",dirname,dirent[i].d_name);
		if ( css_stat(pSession, path, &st ) != 0 ) {
			fprintf(stderr,"dirname(%s) stat error(%d,%s).\n",
				path,errno,strerror(errno));
			return 0;
		}
		if ( !S_ISDIR(st.st_mode) ) {
			continue;
		}
		num += r_getLeafDirNumber(pSession,path);
	}
	if ( num == 0 ) {
		num = 1;
	}
	return num;
}
size_t
getISCSIVolSize(void *pSession)
{
	int n = r_getLeafDirNumber(pSession,name);
	return file_size*n<<LYLERS_BITS;
}
