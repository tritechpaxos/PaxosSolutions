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
 * @brief BTree ÇÃä÷êîåQ
 */
#include	"NWGadget.h"


NHeapPool_t	Pool;
BTree_t		BTreeMem;

void
PrintMem( void* p )
{
	Printf("%p", p );
}

void*
BTMemAlloc( size_t s )
{
	void	*p;

	p = NHeapMalloc( &Pool, s );
	ASSERT( p );
	return( p );
}

void
BTMemFree( void* p )
{
	NHeapFree( p );
}

void*
BTMalloc( size_t s )
{
	void	*p;
	p = malloc( s );
	memset( p, 0, s );
	BTInsert( &BTreeMem, p );
	return( p );
}

void
BTFree( void* p )
{
	BTDelete( &BTreeMem, p );
	free( p );
}

void*
BMalloc( size_t s )
{
	return( malloc( s ) );
}

void
BFree( void* p )
{
	free( p );
}


int
main( int ac, char* pArgs[] )
{
	BTree_t		BTree;
	BTCursor_t	Cursor;
	int	Ret;

	NHeapPoolInit( &Pool, 1024*20, 5, malloc, free, 0/*HEAP_DYNAMIC*/ );

	BTreeInit( &BTreeMem, 5, NULL, NULL, BTMemAlloc, BTMemFree, NULL );
	BTreeInit( &BTree, 2, NULL, NULL, BTMalloc, BTFree, NULL );

	BTreeInit( &BTree, 1, NULL, NULL, BMalloc, BFree, NULL );
	int	i;
	for( i = 0; i < 100; i++ ) {
		BTInsert( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );
	for( i = 0; i < 100; i++ ) {
		BTDelete( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );

	for( i = 0; i < 100; i++ ) {
		BTInsert( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	for( Ret = BTCHead( &Cursor ); Ret == 0; Ret = BTCNext( &Cursor ) ) {
		printf("DELETE(%d)\n", i = PNT_INT32(BTCGetKey( void*, &Cursor )) );
		if( i != 10 ) {
			Ret = BTCDelete( &Cursor );
		}
	}
	BTreePrint( &BTree );

//#define	RAND_ARRAY	151
#define	RAND_ARRAY	10000
	int	aRand[RAND_ARRAY];
int j;
for( j = 0; j < 100; j++ ) {
	for( i = 0; i < RAND_ARRAY; i++ ) {
		aRand[i]	= rand();
	}
	for( i = 0; i < RAND_ARRAY; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(aRand[i]) );
	}
	BTreePrint( &BTreeMem );

//	BTInsert( &BTreeMem, INT32_PNT(rand()) );
//	BTreePrint( &BTreeMem );

	BTCursorOpen( &Cursor, &BTreeMem );
	for( Ret = BTCHead( &Cursor ); Ret == 0; Ret = BTCNext( &Cursor ) ) {
		Ret = BTCDelete( &Cursor );
	}
	BTreePrint( &BTreeMem );
	NHeapPoolDump( &Pool );
}
for( j = 0; j < 100; j++ ) {
	for( i = 0; i < RAND_ARRAY; i++ ) {
		aRand[i]	= rand();
	}
	for( i = 0; i < RAND_ARRAY; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(aRand[i]) );
	}
	BTreePrint( &BTreeMem );

//	BTInsert( &BTreeMem, INT32_PNT(rand()) );
//	BTreePrint( &BTreeMem );

	for( i = 0; i < RAND_ARRAY; i++ ) {
		BTDelete( &BTreeMem, INT32_PNT(aRand[i]) );
	}
	BTreePrint( &BTreeMem );
	NHeapPoolDump( &Pool );
}

	for( i = 0; i < 100; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(i) );
	}
	BTDelete( &BTreeMem, INT32_PNT(99) );
	BTreePrint( &BTreeMem );

	return( 0 );
}
