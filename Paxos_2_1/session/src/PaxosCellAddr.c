/******************************************************************************
*
*  PaxosCellAddr.c 	--- 
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
//#define	_LOCK_

#include	"PaxosSession.h"

struct in_addr
PaxosGetAddrInfo( char *pNode )
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
	struct in_addr	pin_addr = {0,};
	int	s;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family 	= AF_INET;
    hints.ai_socktype 	= SOCK_STREAM;
    hints.ai_flags 		= 0;
    hints.ai_protocol 	= 0;
    hints.ai_canonname 	= NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

   	s = getaddrinfo( pNode, NULL, &hints, &result);
	if( s < 0 || !result )	goto err;

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

err:
	return( pin_addr );
}

static int
PaxosGetCellAddr(char *pCellName, 
				int	Maximum,
				PaxosSessionCell_t	*pSessionCell, 
				PaxosCell_t			*pPaxosCell )
{
	int	j;
	int	s;
    struct addrinfo hintsTCP, hintsUDP;
    struct addrinfo *result = NULL;
	struct in_addr	pin_addr;
	char	Id[4];
	char	*pHostTCP, *pPortTCP;
	char	*pHostUDP, *pPortUDP;

/*
 *	Address
 */
    memset(&hintsTCP, 0, sizeof(hintsTCP));
    hintsTCP.ai_family 	= AF_INET;
    hintsTCP.ai_socktype 	= SOCK_STREAM;
    hintsTCP.ai_flags 		= 0;
    hintsTCP.ai_protocol 	= 0;
    hintsTCP.ai_canonname 	= NULL;
    hintsTCP.ai_addr = NULL;
    hintsTCP.ai_next = NULL;

    memset(&hintsUDP, 0, sizeof(hintsUDP));
    hintsUDP.ai_family 	= AF_INET;
    hintsUDP.ai_socktype 	= SOCK_DGRAM;
    hintsUDP.ai_flags 		= 0;
    hintsUDP.ai_protocol 	= 0;
    hintsUDP.ai_canonname 	= NULL;
    hintsUDP.ai_addr = NULL;
    hintsTCP.ai_next = NULL;
/*
 *	Paxos, Client
 */
#define	UDP_HOST	1
#define	UDP_PORT	2
#define	TCP_HOST	3
#define	TCP_PORT	4

	if( pSessionCell )	strcpy( pSessionCell->c_Name, pCellName );
	if( pPaxosCell )	strcpy( pPaxosCell->c_Name, pCellName );

	for( j = 0; j < Maximum; j++ ) {

		sprintf( Id, "%d", j );

		if( pSessionCell ) {
			pHostTCP	= PaxosGetArgFromConfig( pCellName, Id, TCP_HOST );
			if( !pHostTCP )	return( -1 );

   		 	s = getaddrinfo( pHostTCP, NULL, &hintsTCP, &result);
			if( s < 0 || !result )	return( -1 );

			pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
			freeaddrinfo( result );

			pPortTCP	= PaxosGetArgFromConfig( pCellName, Id, TCP_PORT );
			if( !pPortTCP )	return( -1 );

			strcpy( pSessionCell->c_aPeerAddr[j].a_Host, pHostTCP );
			pSessionCell->c_aPeerAddr[j].a_Active	= TRUE;
			pSessionCell->c_aPeerAddr[j].a_Addr.sin_family	= AF_INET;
			pSessionCell->c_aPeerAddr[j].a_Addr.sin_addr = pin_addr;
			pSessionCell->c_aPeerAddr[j].a_Addr.sin_port 
													= htons(atoi(pPortTCP)); 
		}
		if( pPaxosCell ) {
			pHostUDP	= PaxosGetArgFromConfig( pCellName, Id, UDP_HOST );
			if( !pHostUDP )	return( -1 );

   		 	s = getaddrinfo( pHostUDP, NULL, &hintsUDP, &result);
			if( s < 0 || !result )	return( -1 );

			pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
			freeaddrinfo( result );

			pPortUDP	= PaxosGetArgFromConfig( pCellName, Id, UDP_PORT );
			if( !pPortUDP )	return( -1 );

			strcpy( pPaxosCell->c_aPeerAddr[j].a_Host, pHostUDP );
			pPaxosCell->c_aPeerAddr[j].a_Active	= TRUE;
			pPaxosCell->c_aPeerAddr[j].a_Addr.sin_family	= AF_INET;
			pPaxosCell->c_aPeerAddr[j].a_Addr.sin_addr = pin_addr;
			pPaxosCell->c_aPeerAddr[j].a_Addr.sin_port = htons(atoi(pPortUDP)); 
		}
	}
	return( 0 );
}

static int
PaxosGetCellAddrNumber( char *pCellName )
{
	char	Id[4];
	int		j;
	char	*pHostName;

	for( j = 0; 1; j++ ) {
		sprintf( Id, "%d", j );
		pHostName	= PaxosGetArgFromConfig( pCellName, Id, 1 );
		if( !pHostName )	break;
	}
	return( j );
}

PaxosCell_t*
PaxosCellGetAddr( char *pCellName, void*(*fMalloc)(size_t) )
{
	int	Maximum;
	PaxosCell_t	*pCell;
	int	l;

	Maximum = PaxosGetCellAddrNumber( pCellName );
	if( Maximum <= 0 )	goto err;
	l = PaxosCellSize( Maximum );
	pCell	= (PaxosCell_t*)(*fMalloc)( l );
	memset( pCell, 0, l );
	PaxosGetCellAddr( pCellName, Maximum, NULL, pCell );
	pCell->c_Max = pCell->c_Maximum	= Maximum;
	return( pCell );
err:
	return( NULL );
}

PaxosCell_t*
PaxosSessionCellGetAddr( char *pCellName, void*(*fMalloc)(size_t) )
{
	int	Maximum;
	PaxosSessionCell_t	*pCell;
	int	l;

	Maximum = PaxosGetCellAddrNumber( pCellName );
	if( Maximum <= 0 )	goto err;
	l = PaxosCellSize( Maximum );
	pCell	= (PaxosCell_t*)(*fMalloc)( l );
	memset( pCell, 0, l );
	PaxosGetCellAddr( pCellName, Maximum, pCell, NULL );
	pCell->c_Max = pCell->c_Maximum	= Maximum;
	return( pCell );
err:
	return( NULL );
}
