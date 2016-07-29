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

void
PrintActive( PaxosActive_t *pA )
{
	printf( "Target=%d Max=%d Maximum=%d Leader=%d Epoch=%d Load=%d "
			"NextDecide=%u BackPage=%u\n",
	pA->a_Target, pA->a_Max, pA->a_Maximum, pA->a_Leader, pA->a_Epoch,
	pA->a_Load, pA->a_NextDecide, pA->a_BackPage );
}

int
main( int ac, char *av[] )
{
	char	*pCell;
	int		My_Id, Target_Id;
	PaxosActive_t	Summary;
	int	Ret;

	if( ac < 3 ) {
		printf("PaxosIsActive cell_name my_id [target_id]\n");
		return( -2 );
	}
	pCell	= av[1];
	My_Id	= atoi(av[2]);
	if( ac < 4 ) {
		Ret = PaxosIsActiveSummary( pCell, My_Id, &Summary );
		if( Ret < 0 )	goto err;
	} else {
		Target_Id = atoi(av[3]);
		Ret = PaxosIsActive( pCell, My_Id, Target_Id, &Summary );
		if( Ret < 0 )	goto err;
	}
	PrintActive( &Summary );

	return( Ret );
err:
	printf("No Active\n");
	return( -1 );
}

