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

/** @file
 * @brief skl_mem ä÷êîåQ
 * @author nw
 */

#include	"NWGadget.h"

void
statistics( NHeapPool_t* pPool )
{
	int	i;

	for( i = 0; i < pPool->p_HeapMax; i++ ) {
		Printf("===STATISTICS[%d]===\n", i );
		Printf("FREE_SIZE[%d]\n", 
				NHeapPoolStatistics( pPool, i, HEAP_FREE_SIZE ) );
		Printf("FREE_SEG[%d]\n", 
				NHeapPoolStatistics( pPool, i, HEAP_FREE_SEG ) );
		Printf("FREE_MAX[%d]\n", 
				NHeapPoolStatistics( pPool, i, HEAP_FREE_MAX ) );
		Printf("ALLOC_SIZE[%d]\n", 
				NHeapPoolStatistics( pPool, i, HEAP_ALLOC_SIZE ) );
	}
}

int
main( int ac, char* args[] )
{
#define	NHEAP	1
	NHeapPool_t	HeapPool;
	char	*pC1, *pC2, *pC3;

	NHeapPoolInit( &HeapPool, 1024, NHEAP, malloc, free, HEAP_BEST_FIT );
	statistics( &HeapPool );
	NHeapPoolDump( &HeapPool );

	printf("### pC1 = NHeapZalloc( &HeapPool, 32 ) ###\n");
	pC1 = (char*)NHeapZalloc( &HeapPool, 32 );
	if( pC1 ) {
		*pC1	= 'A';
		statistics( &HeapPool );
		NHeapPoolDump( &HeapPool );

		printf("### NHeapFree( pC1=%p ) ###\n", pC1 );
		NHeapFree( pC1 );
		statistics( &HeapPool );
		NHeapPoolDump( &HeapPool );
	}

	printf("### pC1 = NHeapZalloc( &HeapPool, 10 ) ###\n");
	pC1 = (char*)NHeapZalloc( &HeapPool, 10 );
	if( pC1 ) {
		*pC1	= 'A';
		statistics( &HeapPool );
		NHeapPoolDump( &HeapPool );

		printf("### NHeapFree( pC1=%p ) ###\n", pC1 );
		NHeapFree( pC1 );
		statistics( &HeapPool );
		NHeapPoolDump( &HeapPool );
	}

	printf("### pC1 = NHeapZalloc( &HeapPool, 4 ) ###\n");
	pC1 = (char*)NHeapZalloc( &HeapPool, 4 );
	if( pC1 ) {
		*pC1	= 'A';
		statistics( &HeapPool );
		NHeapPoolDump( &HeapPool );

		printf("### pC2 = NHeapRealloc( &HeapPool, 6 ) ###\n");
		pC2 = (char*)NHeapRealloc( pC1, 6 );
		if( pC2 ) {
			statistics( &HeapPool );
			NHeapPoolDump( &HeapPool );

			printf("### NHeapFree( pC2=%p ) ###\n", pC2 );
			NHeapFree( pC2 );
			statistics( &HeapPool );
			NHeapPoolDump( &HeapPool );
		}
	}
	printf("### pC1 = NHeapZalloc( &HeapPool, 4 ) ###\n");
	pC1 = (char*)NHeapZalloc( &HeapPool, 4 );
	printf("### pC2 = NHeapZalloc( &HeapPool, 50 ) ###\n");
	pC2 = (char*)NHeapZalloc( &HeapPool, 50 );
	statistics( &HeapPool );
	NHeapPoolDump( &HeapPool );

	printf("### NHeapFree( pC1=%p ) ###\n", pC1 );
	NHeapFree( pC1 );
	statistics( &HeapPool );
	NHeapPoolDump( &HeapPool );

	printf("### pC3 = NHeapZalloc( &HeapPool, 30 ) ###\n");
	pC3 = (char*)NHeapZalloc( &HeapPool, 30 );
	statistics( &HeapPool );
	NHeapPoolDump( &HeapPool );

	printf("### NHeapFree( pC2=%p ) ###\n", pC2 );
	NHeapFree( pC2 );
	printf("### NHeapFree( pC3=%p ) ###\n", pC3 );
	NHeapFree( pC3 );
	statistics( &HeapPool );
	NHeapPoolDump( &HeapPool );

	NHeapPoolDestroy( &HeapPool );

	return( 0 );
}

