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

void
print_time( char *pTitle, timespec_t *pStart, timespec_t *pEnd )
{
	long long 	nsec, sec;

	if( pStart->tv_nsec > pEnd->tv_nsec ) {
		pEnd->tv_sec--;
		nsec	= 1000L*1000L*1000L + pEnd->tv_nsec - pStart->tv_nsec;
	} else {
		nsec	= pEnd->tv_nsec - pStart->tv_nsec;
	}
	sec	= pEnd->tv_sec - pStart->tv_sec;

	printf("Start[%u:%ld] End=[%u:%ld]\n", 
		(uint32_t)pStart->tv_sec, pStart->tv_nsec,
		(uint32_t)pEnd->tv_sec, pEnd->tv_nsec );

	printf("%s[%ld sec %ld usec]\n", pTitle, (long)sec, (long)(nsec/1000));
}

int
main( int ac, char* av[] )
{
	int size;
	char	*cp;
	int	i;
	uint64_t		cksum64, sum64, mcksum64;
	unsigned short	cksum16, sum16;
	uint32_t		sum32;
	timespec_t	Start, End;
	Msg_t		*pMsg;
	MsgVec_t	Vec;
	int		l;

	if( ac < 2 ) {
		printf("Cksum size\n");
		exit( -1 );
	}
	size	= atoi(av[1]);

	cp	= (char*)malloc( size );
	for( i = 0, srand((unsigned int)time(0)); i < size; i++ ) {
		cp[i]	= rand();
	}
	TIMESPEC( Start );
	cksum16	= in_cksum( (unsigned short*)cp, size );
	TIMESPEC( End );
	print_time( "in_cksum(8bytes sum)", &Start, &End );

	TIMESPEC( Start );
	cksum64 = in_cksum64( cp, size, 0 );
	TIMESPEC( End );
	print_time( "in_cksum64(8bytes sum)", &Start, &End );

	sum64	= ~cksum64;
	sum64	= (sum64>>32) + (sum64&0xffffffff);
	sum64	+= (sum64>>32);
	sum32	= (uint32_t)sum64;
	sum32	= (sum32>>16) + (sum32&0xffff);
	sum32	+= (sum32>>16);
	sum16	= sum32;
	sum16	= ~sum16;

	printf("[%d] cksum16=0x%x-0x%x\n", size, cksum16, sum16 );
	ASSERT( cksum16 == sum16 );

	/*
	 *
	 */
	pMsg	= MsgCreate( 3, malloc, free );

	l	= size/3;

	if( l > 0 ) {
		MsgVecSet( &Vec, 0, cp, l, l ); 
		MsgAdd( pMsg, &Vec );

		MsgVecSet( &Vec, 0, cp+l, l, l ); 
		MsgAdd( pMsg, &Vec );

		MsgVecSet( &Vec, 0, cp+2*l, size-2*l, size-2*l ); 
		MsgAdd( pMsg, &Vec );
	} else {
		MsgVecSet( &Vec, 0, cp, size, size ); 
		MsgAdd( pMsg, &Vec );
	}

	TIMESPEC( Start );
	mcksum64	= MsgCksum64( pMsg , 0, 0, NULL );
	TIMESPEC( End );
	printf("MsgCksum64[l=%d] cksum64=0x%" PRIx64" mcksum64=0x%" PRIx64"\n", 
			l, cksum64, mcksum64 );
	ASSERT( cksum64 == mcksum64 );

	/*
	 *	update
	 */
	uint64_t	old_cksum64, new_cksum64, sub, add;

	if( l > 0 ) {
		old_cksum64 = in_cksum64( cp, 2*l, 0 );
		new_cksum64 = in_cksum64( cp + l, size - l, l % 8 );

		cksum64	= in_cksum64_update( old_cksum64, 
										cp, l, 0,
										cp + 2*l, size - 2*l, (2*l) % 8 );

		printf("UPDATE: updated_cksum64=0x%" PRIx64" new_cksum64=0x%" PRIx64"\n", 
				cksum64, new_cksum64 );
		ASSERT( cksum64 == new_cksum64 );

		l	&= ~7;
		old_cksum64 = in_cksum64( cp, 2*l, 0 );
		sub	= in_cksum64( cp, l, 0 );
		add	= in_cksum64( cp + 2*l, size - 2*l, 0 );
		new_cksum64	= in_cksum64( cp + l, size - l, 0 );
		cksum64	= in_cksum64_sub_add( old_cksum64, sub, add );

		printf("\nUPDATE(l=%d): old=0x%" PRIx64" sub=0x%" PRIx64" add=0x%" PRIx64"\n"
				"new=0x%" PRIx64" ans=0x%" PRIx64"\n", 
				l, old_cksum64, sub, add, new_cksum64, cksum64 );
		ASSERT( new_cksum64 == cksum64 );
	}

	/*
	 *	End
	 */
	free( cp );

	return( 0 );
}
