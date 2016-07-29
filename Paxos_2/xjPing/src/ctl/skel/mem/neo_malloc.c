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

#include	"neo_db.h"

static	bool_t		bInit;
static	NHeapPool_t	HeapPool;
static	bool_t		bPool;
static	Mtx_t		neo_mallocMtx;

NHeapPool_t*
neo_GetHeapPool()
{
	return( &HeapPool );
}

void
neo_HeapPoolDump()
{
	if( bPool ) {
		MtxLock( &neo_mallocMtx );
		NHeapPoolDump( &HeapPool );
		MtxUnlock( &neo_mallocMtx );
	}
}

static int
neo_malloc_init()
{
	long	MemMax;
	char	*pMemMax;
	char	*pC;
	int		Flag;

	MtxInit( &neo_mallocMtx );

	pMemMax	= getenv( PING_MEM_MAX );
	if( !pMemMax )	return( 0 );

	Flag = 0;
	for( pC = pMemMax; *pC; pC++ ) {
		if( '0' <= *pC && *pC <= '9' )	continue;
		*pC = 0;
		Flag |= HEAP_BEST_FIT;
		break;
	}
	MemMax	= atol( pMemMax );
	if( NHeapPoolInit( &HeapPool, MemMax*(1024*1024), 
								1, Malloc, Free, Flag ) ) {
		neo_errno	= E_RDB_NOMEM;
		return( -1 );
	}
	bPool	= TRUE;
	return( 0 );
}

void*
neo_malloc( size_t size )
{
	void	*p;

	if( !bInit ) {
		if( neo_malloc_init() ) {
			neo_errno	= E_RDB_NOMEM;
			return( NULL );
		}	
		bInit	= TRUE;
	}

	if( bPool ) {
		MtxLock( &neo_mallocMtx );
		p = NHeapMalloc( &HeapPool, size );
		MtxUnlock( &neo_mallocMtx );
	} else {
		p = Malloc( size );
	}
	return( p );
}

void
neo_free( void *p )
{
	if( bPool ) {
		MtxLock( &neo_mallocMtx );
		NHeapFree( p );
		MtxUnlock( &neo_mallocMtx );
	} else {
		Free( p );
	}
}

void*
neo_zalloc( size_t size )
{
	void	*p;

	p = neo_malloc( size );
	if( p )	memset( p, 0, size );
	return( p );
}
