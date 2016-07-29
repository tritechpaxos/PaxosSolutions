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

#define	_PAXOS_SERVER_
#include	"Paxos.h"

#include	<netdb.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<poll.h>

int
_PaxosIsActive( int Fd, PaxosCell_t *pCell, int FrId, int ToId, 
						PaxosActive_t *pActive )
{
	msg_t	M;
	msg_active_rpl_t Rpl;
	struct sockaddr_in Fr, ToAddr;
	socklen_t	len;
	int	err;
#define	EXPIRE_TIMEOUT_MSEC	200
	timespec_t	Expire, Now;

	TIMESPEC( Expire );
	TimespecAddUsec( Expire, EXPIRE_TIMEOUT_MSEC*1000 );

	msg_init( &M, -1, MSG_IS_ACTIVE, sizeof(M) );
	M.m_fr	= FrId;
	M.m_to	= ToId;
	M.m_From	= pCell->c_aPeerAddr[FrId].a_Addr;
	ToAddr		= pCell->c_aPeerAddr[ToId].a_Addr;

	err	= sendto( Fd, (char*)&M, sizeof(M), 0, 
			(struct sockaddr*)&ToAddr, sizeof(ToAddr) );
	if( err < 0 )	goto err;

	// Check Read MSG_IS_ACTIVE
while( 1 ) {

	struct pollfd fds[1];
	nfds_t nfds = 1;
	fds[0].fd = Fd;
	fds[0].events = POLLIN;

	int Ret = poll( fds, nfds, EXPIRE_TIMEOUT_MSEC/*msec*/);

	if ( Ret != 1 || (fds[0].revents & POLLIN) == 0 ) {
		err = -1;
		goto err;
	}
	len	= sizeof(Fr);
	err = recvfrom( Fd, (char*)&Rpl, sizeof(Rpl), MSG_TRUNC, 
						(struct sockaddr*)&Fr, &len );
	if( err < 0 )	goto err;

	if( Rpl.a_head.m_cmd != MSG_IS_ACTIVE ) {

		TIMESPEC( Now );
		if( TIMESPECCOMPARE( Expire, Now ) < 0 )	goto err;

		continue;
	} else {
		*pActive	= Rpl.a_Active;
		break;
	}
}
	return( 0 );
err:
	return( -1 );
}

int
PaxosIsActiveSummary( char *pCell, int My_Id, PaxosActive_t *pSummary )
{
	PaxosCell_t	*pPaxosCell;
	int		j;
	int	Fd;
	int	Ret;
	struct sockaddr_in	InAddr;
	int		ServerMax = 0;
	int		cmp;

	pPaxosCell	= PaxosGetCell( pCell );
	ServerMax = pPaxosCell->c_Maximum;
/*
 *	UDP socket
 */
	if( (Fd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		goto err;
	}
	InAddr	= pPaxosCell->c_aPeerAddr[My_Id].a_Addr;
	if( bind( Fd, (struct sockaddr*)&InAddr, sizeof(InAddr) ) < 0 ) {
		goto err1;
	}
/*
 *	Active check
 */
	PaxosActive_t	ActiveW;
	bool_t	bFirst = TRUE;

	memset( pSummary, 0, sizeof(*pSummary) );

	for( j = 0; j < ServerMax; j++ ) {

		if( j == My_Id )	continue;

		Ret = _PaxosIsActive( Fd, pPaxosCell, My_Id, j, &ActiveW );
		if( Ret < 0 )	continue;

		if( bFirst ) {
			ServerMax	= ActiveW.a_Max;
			*pSummary	= ActiveW;
			bFirst = FALSE;
		}
		if(	(cmp = CMP(ActiveW.a_NextDecide, pSummary->a_NextDecide)) >= 0 ) {

			if( cmp > 0 || pSummary->a_Load <= 0 || 
				(pSummary->a_Load > 0 && ActiveW.a_Load < pSummary->a_Load) ) {

					pSummary->a_Target		= j;
					pSummary->a_NextDecide	= ActiveW.a_NextDecide;
			}
		}

		if( ActiveW.a_Leader >= 0 ) {
			pSummary->a_Leader	= ActiveW.a_Leader;
			pSummary->a_Epoch	= ActiveW.a_Epoch;
		}
	}
	close( Fd );
	PaxosFreeCell( pPaxosCell );

	if( bFirst )	return( -1 );
	else			 return( 0 );
err1:
	close( Fd );
err:
	PaxosFreeCell( pPaxosCell );
	return( -1 );
}

int
PaxosIsActive( char *pCell, int My_Id, int j, PaxosActive_t *pActive )
{
	PaxosCell_t	*pPaxosCell;
	struct sockaddr_in	InAddr;
	int	Fd;
	int	Ret;

	pPaxosCell	= PaxosGetCell( pCell );
/*
 *	UDP socket
 */
	if( (Fd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		goto err;
	}
	InAddr	= pPaxosCell->c_aPeerAddr[My_Id].a_Addr;
	if( bind( Fd, (struct sockaddr*)&InAddr, sizeof(InAddr) ) < 0 ) {
		goto err1;
	}
/*
 *	Active check
 */
	memset( pActive, 0, sizeof(*pActive) );

	Ret = _PaxosIsActive( Fd, pPaxosCell, My_Id, j, pActive );

	close( Fd );
	PaxosFreeCell( pPaxosCell );
	return( Ret );
err1:
	close( Fd );
err:
	PaxosFreeCell( pPaxosCell );
	return( -1 );
}

