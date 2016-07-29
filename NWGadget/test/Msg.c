/******************************************************************************
*
*  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
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

#include	"NWGadget.h"

/* Messege Engine */
typedef struct fact {
	int	f_n;
	int	f_nn;
} fact_t;

void*
Factor( Msg_t *pM )
{
	fact_t *pF = (fact_t*)MsgFirst( pM );

	if( pF->f_n ) {
		pF->f_nn *= pF->f_n--;
		return( Factor );
	} else return( NULL );
}

int
main( int ac, char* av[] )
{
	fact_t		fact;
	MsgVec_t	Vec;
	Msg_t		*pM;

	if( ac < 2 ) {
		printf("usage:msg number\n");
		exit( -1 );
	}
	fact.f_n	= atoi(av[1]);
	fact.f_nn = 1;

	pM = MsgCreate( 1, malloc, free );
	MsgVecSet( &Vec, 0, &fact, sizeof(fact), sizeof(fact) );
	MsgAdd( pM, &Vec );

	MsgEngine( pM, Factor );

	printf("n! = %d\n", fact.f_nn );
	
	return( 0 );
}
