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

// size xxxK xxM xxG xxT
size_t size;
char buf[4096];
char *name;
char *cmdName;
int isHole = 1;
char *cell = "css";

void usage(void);
int clearFile(void);

int
main(int ac, char *av[] )
{
	int num;
	char *unit;
	int rtn = 0;
	
	cmdName=av[0];
	if (ac < 2 ) {
		usage();
		goto err;
	}
	cell = av[1];
	ac--;
	av++;

	if ( ac > 1 && strcmp(av[1],"--full") == 0 ) {
		isHole = 0;
		ac--;
		av++;
	}
	if ( ac < 3 ) {
		usage();
		goto err;
	}

	name = av[1];
	num = strtoul(av[2],&unit,0);
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
	rtn = clearFile();
 out:
	return rtn;
 err:
	rtn = -1;
	goto out;
}
int clearFile(void)
{
	void *pSession = NULL;
	void *pF = NULL;
	int rtn = 0;
	memset(buf,'\0',sizeof(buf));
	// session open
	if ((pSession = paxos_session_open(cell,0,0))==NULL) {
		goto err;
	}
	// PFS file open
	if ( ( pF = css_open(pSession,name,0) ) == NULL ) {
		fprintf(stderr,"PFS file(%s) open error(%d,%s).\n",
			name,errno,strerror(errno));
		goto err;
	}
	if ( isHole ) {
		if ( css_locked_write(pF,buf,(off_t)-1,1,NULL) != 1 ) {
			fprintf(stderr,"PFS file(%s) write error(%d,%s).\n",
				name,errno,strerror(errno));
			goto err;
		}
		css_close(pF);
		pF = NULL;
		if ( css_truncate(pSession,name,0) ) {
                        fprintf(stderr,"PFS file(%s,%lu) truncate error(%d,%s).\n",
                                name,size,errno,strerror(errno));
                        goto err;
		}
		if ( css_truncate(pSession,name,size) ) {
                        fprintf(stderr,"PFS file(%s,%lu) truncate error(%d,%s).\n",
                                name,size,errno,strerror(errno));
                        goto err;
		}
		goto out;
	}
	while ( size > 0 ) {
		int len = ( size > sizeof(buf) )?sizeof(buf):size;
		if ( css_locked_write(pF,buf,(off_t)-1,len,NULL) != len ) {
			fprintf(stderr,"PFS file(%s) write error(%d,%s).\n",
				name,errno,strerror(errno));
			goto err;
		}
		size -= len;
	}
 out:
	return rtn;
 err:
	if ( pF ) {
		css_close( pF );
	}
	if ( pSession ) {
		paxos_session_close(pSession);
	}
	rtn = -1;
	goto out;
}
void usage(void)
{
	fprintf(stderr,"usage:%s cell [--full] PFS_FILE num{K|M|G|T}\n",cmdName);
}
