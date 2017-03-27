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

#include	<ctype.h>
#include	<inttypes.h>
#include	"VV.h"
#include	"bs_VV.h"

void
usage(void)
{
printf("=== Environmental variables"
		"(CSS DB [CRC_SIZE CRC_MAX]) are needed ===\n");
printf("	CSS			--- Paxos Core\n");
printf("	DB			--- Paxos CMDB\n");
printf("	CRC_SIZE	--- Client Read Cache block Size\n");
printf("	CRC_MAX		--- Client Read Cache block Max number\n");
printf( "VVTest write IQN VolName "
		" start interval end {len(num)|data(alpha)}\n");
printf( "VVTest read IQN VolName start interval end len\n");
}

int
main( int ac, char *av[] )
{
	Vol_t	*pVl;
	int		Ret;
	bool_t	bRead = TRUE;
	char	*pBuf, *pData = NULL;
	off_t	Off, Start, End;
	size_t	Len;
	int		Interval;
	char	*pIQN, *pVolName;
	uint64_t	Size;
	int		i, h;
	char	*pCmd;

	if( ac < 7 ) {
		goto err;
	}
	if( !getenv("CSS") || !getenv("DB") ) {
		goto err;
	}
	if( !strcmp( "read", av[1] ) )	bRead = TRUE;
	else		bRead = FALSE;
	
	pCmd		= av[1];
	pIQN		= av[2];
	pVolName	= av[3];
	Start		= atoll( av[4] );
	Interval	= atoi( av[5] );
	End			= atoll( av[6] );
	if( isdigit( av[7][0] ) ) {
		Len	= atoi( av[7] );
	} else {
		Len		= strlen( av[7] );
		pData	= av[7];
	}
	if( !bRead && !pData ) {
		pData	= Malloc( Len );
		for( i = 0; i < Len; i++ ) {
			h	= i % 10;
			pData[i]	= 0x30|h;
		}
	}

	pBuf		= Malloc( Len+1 );
	pBuf[Len]	= 0;

	VV_init();

	VL_init( pIQN, 1 );

	VL_open( pIQN, pVolName, (void**)&pVl, &Size );

	if( !strcmp( "read", pCmd ) )	{
		for( Off = Start; Off <= End; Off += Interval ) {
			memset( pBuf, 0, Len );
			Ret = VL_read( pVl, pBuf, Len, Off );
			if( Ret < 0 )	goto err1;

			printf("%s", pBuf );
			fflush(stdout);
		}
	} else if( !strcmp( "write", pCmd ) )	{
		for( Off = Start; Off <= End; Off += Interval ) {
			Ret = VL_write( pVl, pData, Len, Off );
			if( Ret < 0 )	goto err1;
		}
	}
	Ret = VL_close( pVl );

	VL_exit();

	VV_exit();

	exit( 0 );
err1:	Ret = VL_close( pVolName );
		VL_exit();
		VV_exit();
err:	usage();
		exit( -1 );
}
