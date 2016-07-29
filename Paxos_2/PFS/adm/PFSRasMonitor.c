/******************************************************************************
*
*  PFSRasMonitor.c 	--- Monitor program
*
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
 *	作成			渡辺典孝
 *	試験
 *	特許、著作権	トライテック
 */

//#define	_DEBUG_


#include	"PFS.h"
#include	<stdio.h>
#include	<semaphore.h>

int
Callback( struct Session *pSession, PFSHead_t *pF )
{
	PFSEventDir_t	*pE;
	struct stat st;


	pE	= (PFSEventDir_t*)pF;

	/*
	 * set Date
	 */
	char strtime[80];
	char M;

	switch( pE->ed_Mode ) {
		case DIR_ENTRY_CREATE:	
			M = 'C';	
			PFSStat(pSession,pE->ed_Name,&st);
			strcpy(strtime,ctime(&st.st_ctime));
			break;
		case DIR_ENTRY_UPDATE:	
			M = 'U';	
			PFSStat(pSession,pE->ed_Name,&st);
			strcpy(strtime,ctime(&st.st_ctime));
			break;
		case DIR_ENTRY_DELETE:	
			M = 'D';	
			strcpy(strtime,ctime(&pE->ed_Time));
			break;
	}
	strtime[strlen(strtime)-1] = '\0';
	printf("%c %s %s '%s'\n", M,pE->ed_Name, pE->ed_Entry, strtime );
	fflush( stdout );
	return( 0 );
}

int
initial_print(int ac, char *av[], struct Session *pSession )
{
#define SERVER_ID( name, PAXOS_SERVER_MAXIMUM ) \
	(((name)[0] == '.') ? PAXOS_SERVER_MAXIMUM : strtoul(name,NULL,0))

	int i;
	int	Maximum;
	static PFSDirent_t	*ent;

	if( !ent ) {
		Maximum	= PaxosSessionGetMaximum( pSession );
		ent = (PFSDirent_t*)Malloc( sizeof(PFSDirent_t)*(Maximum+2));
	}
	for ( i = 0; i < ac; i++ ) {
		int no = Maximum + 2;
		if ( PFSReadDir(pSession,av[i],ent,0,&no) == 0 ) {
			/**** sort *****/
			int m,n;
			for ( n = 0; n < no-1; n++ ) {
				int min = SERVER_ID(ent[n].de_Name,Maximum);
				for ( m = n+1; m < no; m++ ) {
					int val = SERVER_ID(ent[m].de_Name,Maximum);
					if ( min > val ) {
						// swap
						PFSDirent_t e=ent[m];
						ent[m] = ent[n];
						ent[n] = e;
						min = val;
					}
				}
			}
			/*** print ***/
			for ( n = 0; n < no-2; n++ ) {
				// stat
				char path[PATH_NAME_MAX];
				struct stat st;

				sprintf(path,"%s/%s",av[i],ent[n].de_Name);
				if ( PFSStat(pSession,path,&st) == 0 ) {

					char strtime[80];
					strcpy(strtime,ctime(&st.st_ctime));
					strtime[strlen(strtime)-1] = '\0';

					printf("I %s %s '%s'\n", av[i],ent[n].de_Name, strtime );
					fflush( stdout );
				}
			}
		}
	}
	return 0;
} 

int
usage()
{
	printf("PFSRasMonitor ras_cellname path1 path2 ...\n");
	return( 0 );
}

int
main( int ac, char* av[] )
{
	int	Ret;
	int	i;

	if( ac < 3 ) {
		usage();
		exit( -1 );
	}
	struct Session*	pSession;

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
			Send, Recv, NULL, NULL, av[1] );
	if( !pSession ) {
		printf("Can't get cell[%s] addr.\n", av[1] );
		exit( -1 );
	}

	Ret	= PaxosSessionOpen( pSession, PFS_SESSION_NORMAL, FALSE );
	if( Ret ) goto err;

	for( i = 2; i < ac; i++ ) {
		Ret = PFSEventDirSet( pSession, av[i], pSession );
		if( Ret ) {
			Printf("EventDirSet[%s] error.\n", av[i] );
			goto err1;
		}
	}
	initial_print(ac-2,av+2,pSession);

	int	count, omitted;
	while(PFSEventGetByCallback( pSession, &count, &omitted, Callback ) == 0) {
	}

	PaxosSessionClose( pSession );
	PaxosSessionFree( pSession );
	return( 0 );
err1:
	PaxosSessionClose( pSession );
	PaxosSessionFree( pSession );
err:
	return( -1 );
}
