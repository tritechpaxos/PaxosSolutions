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

#include	"Paxos.h"
#include	<netdb.h>

static char*
StrTok( char *pS )
{
	char	*p;

	p	= pS;
	while( *p != ' ' && *p != ':' 
			&& *p != '\r' && *p != '\n' && *p != '\t'
			&& *p != 0 ) {
		p++;
	}
	if( *p == '\r' || *p == '\n' ) {
		*p++	 = 0;
		p = NULL;
	} else {
		*p++	 = 0;
		while( *p == ' ' )	p++;
	}
	return( p );
}

char*
PaxosGetArgFromConfig( char *pCellName, char *pId, int ArgNo )
{
	FILE	*fp = NULL;
	char	*pC, *pS;
	static char server[1024];

	// config file
#ifdef TTT
	if ((pS = getenv("HOME")) != NULL) {
		sprintf(server,"%s/_%s.conf", pS, pCellName);
		fp = fopen(server,"r");
	}
#else
	if ((pS = getenv("PAXOS_CELL_CONFIG")) != NULL) {
	  if (pS[strlen(pS)-1]  == '/') {
		/* path */
		sprintf(server,"%s%s.conf", pS, pCellName);
	  }
	  else {
		/* file */
		sprintf(server,"%s", pS);
	  }
	  fp = fopen(server,"r");
	}
	if ( fp == NULL && (pS = getenv("HOME")) != NULL) {
		sprintf(server,"%s/_%s.conf", pS, pCellName);
		fp = fopen(server,"r");
	}
#endif
#ifdef KKK
	if ( fp == NULL ) {
		sprintf(server,"%s/.config/cell_config/%s.conf", pS, pCellName);
		fp = fopen(server,"r");
	}
#else
	if ( fp == NULL && pS != NULL ) {
		sprintf(server,"%s/.config/cell_config/%s.conf", pS, pCellName);
		fp = fopen(server,"r");
	}
#endif
	if ( fp == NULL ) {
		sprintf(server,"/etc/cell_config/%s.conf", pCellName);
		fp = fopen(server,"r");
	}
	if ( fp == NULL && pS != NULL ) {
		sprintf(server,"/etc/%s/server.conf", pCellName);
		fp = fopen(server,"r");
		if ( fp == NULL ) {
			pS = NULL;
			goto ret;
		}
	}
	while ((pS = fgets(server,sizeof(server),fp) ) != NULL) {
		if ( *pS == '\r' || *pS == '\n' || *pS == '#' ) {
			continue;
		}
		pC = StrTok( pS );
		if ( pC == NULL || strcmp( pId, pS ) ) {
			continue;
		}
		while( pC && ArgNo-- > 0 ) {
			pS = pC;
			pC = StrTok( pS );
		}
		if( ArgNo > 0 )	pS = NULL;
		break;
	}
	fclose( fp );
ret:
	return( pS );
}

PaxosCell_t*
PaxosGetCell( char *pCell )
{
	PaxosCell_t	*pPaxosCell;
	int	j;
/*
 *	Address
 */
    struct addrinfo hints;
    struct addrinfo *result;
    int     s;
	struct in_addr	pin_addr;
	char	*pHostName, *pPort;
	int		Port;
	char	Id[4];
	int		ServerMax = 0;

    while( 1 ) {

		sprintf( Id, "%d", ServerMax );

		pHostName	= PaxosGetArgFromConfig( pCell, Id, 1 );
		pPort	= PaxosGetArgFromConfig( pCell, Id, 2 );

		if( !pHostName || !pPort )	break;
		ServerMax++;
	}
	if( ServerMax == 0 )	{
		errno	= ENOENT;
		goto err;
	}

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_INET;
    hints.ai_socktype   = SOCK_DGRAM;
    hints.ai_flags      = 0;
    hints.ai_protocol   = 0;
    hints.ai_canonname  = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

	pPaxosCell	= (PaxosCell_t*)
		Malloc( sizeof(PaxosCell_t) + sizeof(PaxosAddr_t)*ServerMax );

	memset( pPaxosCell, 0, 
			sizeof(PaxosCell_t)+sizeof(PaxosAddr_t)*ServerMax);
	pPaxosCell->c_Maximum	= ServerMax;

	strcpy( pPaxosCell->c_Name, pCell );
    for( j = 0; j < ServerMax; j++ ) {
		sprintf( Id, "%d", j );
		pHostName	= PaxosGetArgFromConfig( pCell, Id, 1 );
		pPort	= PaxosGetArgFromConfig( pCell, Id, 2 );
		if( !pHostName || !pPort ) {
			errno	= ENOENT;
			goto err1;
		}
		Port = atoi( pPort );
    	s = getaddrinfo( pHostName, NULL, &hints, &result);
	    if( s < 0 ) goto err1;

   		pin_addr    = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
    	freeaddrinfo( result );

		strncpy( pPaxosCell->c_aPeerAddr[j].a_Host, pHostName,
					sizeof(pPaxosCell->c_aPeerAddr[j].a_Host) );
        pPaxosCell->c_aPeerAddr[j].a_Active   = TRUE;
        pPaxosCell->c_aPeerAddr[j].a_Addr.sin_family   = AF_INET;
        pPaxosCell->c_aPeerAddr[j].a_Addr.sin_addr = pin_addr;
        pPaxosCell->c_aPeerAddr[j].a_Addr.sin_port = htons( Port );
    }
	return( pPaxosCell );
err1:
	Free( pPaxosCell );
err:
	return( NULL );
}

void
PaxosFreeCell( PaxosCell_t *pCell )
{
	Free( pCell );
}
