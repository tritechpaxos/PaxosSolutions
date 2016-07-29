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
BDestroy( BTree_t* pBTree, void* p )
{
	printf("Destroy:%d\n", PNT_INT32(p) );
	return( 0 );
}


int
main( int ac, char* pArgs[] )
{
	BTree_t		BTree;
	BTCursor_t	Cursor;
	BTCursor_t	Cursor1;
	BTCursor_t	Cursor2;

	NHeapPoolInit( &Pool, 1024*10, 3, malloc, free, 0/*HEAP_DYNAMIC*/ );

	BTreeInit( &BTreeMem, 2, NULL, PrintMem, BTMemAlloc, BTMemFree, NULL );
	BTreeInit( &BTree, 2, NULL, NULL, BTMalloc, BTFree, NULL );

	BTCursorOpen( &Cursor, &BTree );
	BTCursorOpen( &Cursor1, &BTree );
	BTCursorOpen( &Cursor2, &BTree );


printf("\nINSERT\n");
BTreePrint(&BTree);printf("Insert 20\n");
	BTInsert( &BTree, (void*)20 );

//	BTCFind( &Cursor1, (void*)20 );

BTreePrint(&BTree);printf("Insert 40\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)40 );
BTreePrint(&BTree);printf("Insert 10\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)10 );
BTreePrint(&BTree);printf("Insert 30\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)30 );
BTreePrint(&BTree);printf("Insert 15\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)15 );

BTreePrint(&BTree);printf("Insert 35\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)35 );
BTreePrint(&BTree);printf("Insert 7\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)7 );
BTreePrint(&BTree);printf("Insert 26\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)26 );
BTreePrint(&BTree);printf("Insert 18\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)18 );
BTreePrint(&BTree);printf("Insert 22\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)22 );

BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)5 );
printf("\nINSERT 5\n");
BTreePrint( &BTree );BTreePrint(&BTreeMem);

	BTInsert( &BTree, (void*)42 );
BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)13 );
BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)46 );
BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)27 );
BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)8 );
BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)32 );
printf("\nINSERT 32\n");
BTreePrint( &BTree );BTreePrint(&BTreeMem);

	BTreePrint(&BTree);printf("Insert 38\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)38 );
	BTreePrint(&BTree);printf("Insert 24\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)24 );
	BTreePrint(&BTree);printf("Insert 45\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)45 );
	BTreePrint(&BTree);printf("Insert 25\n");BTreePrint(&BTreeMem);
	BTInsert( &BTree, (void*)25 );


//	Insert( &BTree, (void*)15 );
printf("\nINSERT 25\n");
BTreePrint( &BTree );BTreePrint(&BTreeMem);

//	BTCFind( &Cursor2, (void*)25 );

	BTCHead( &Cursor );
	PrintCursor( &Cursor );
	while( !BTCNext( &Cursor ) ) PrintCursor( &Cursor );

BTreePrint( &BTree );BTreePrint(&BTreeMem);
printf("\nDELETE 25\n");
	if( !BTCFind( &Cursor, (void*)25 ) )	BTCDelete( &Cursor );
	BTreePrint( &BTree ); PrintCursor( &Cursor );BTreePrint(&BTreeMem);
printf("\nDELETE 45\n");
	if( !BTCFind( &Cursor, (void*)45 ) )	BTCDelete( &Cursor );
	BTreePrint( &BTree ); PrintCursor( &Cursor );BTreePrint(&BTreeMem);
printf("\nDELETE 24\n");
	if( !BTCFind( &Cursor, (void*)24 ) )	BTCDelete( &Cursor );
	BTreePrint( &BTree ); PrintCursor( &Cursor );BTreePrint(&BTreeMem);
printf("\nDELETE 38\n");
	if( !BTCFind( &Cursor, (void*)38 ) )	BTCDelete( &Cursor );
	BTreePrint( &BTree ); PrintCursor( &Cursor );BTreePrint(&BTreeMem);

	BTDelete( &BTree, (void*)25 );
	BTDelete( &BTree, (void*)45 );
	BTDelete( &BTree, (void*)24 );

	BTreePrint( &BTree );BTreePrint(&BTreeMem);
	BTCHead( &Cursor );
	PrintCursor( &Cursor );
	while( !BTCNext( &Cursor ) ) PrintCursor( &Cursor );

printf("\nFIND 8\n");
	if( !BTCFind( &Cursor, (void*)8 ) )	PrintCursor( &Cursor);

printf("\nFIND 20<=\n");
	if( !BTCFindLower( &Cursor, (void*)20 ) )	PrintCursor( &Cursor );
	while( !BTCNext( &Cursor ) ) PrintCursor( &Cursor );

	BTDelete( &BTree, (void*)38 );
	BTDelete( &BTree, (void*)32 );

	BTDelete( &BTree, (void*)8 );
	BTDelete( &BTree, (void*)27 );
	BTDelete( &BTree, (void*)46 );
	BTDelete( &BTree, (void*)13 );
	BTDelete( &BTree, (void*)42 );

	BTDelete( &BTree, (void*)5 );
	BTDelete( &BTree, (void*)22 );
	BTDelete( &BTree, (void*)18 );
	BTDelete( &BTree, (void*)26 );

	BTDelete( &BTree, (void*)7 );
	BTDelete( &BTree, (void*)35 );
	BTDelete( &BTree, (void*)15 );

	BTreePrint( &BTree );BTreePrint(&BTreeMem);

//	BTDelete( &BTree, (void*)20 );
//	BTDelete( &BTree, (void*)40 );
//	BTDelete( &BTree, (void*)10 );
//	BTDelete( &BTree, (void*)30 );

/***
printf("END\n");
	BTreePrint( &BTree );
printf("HEAD\n");
	BTCHead( &Cursor );PrintCursor( &Cursor );
printf("NEXT\n");
	printf("%d\n",BTCDeleteAndNext( &Cursor ));PrintCursor( &Cursor );
printf("Key=%d\n", BTCGetKey( int, &Cursor ) );
***/

	BTreePrint( &BTree );
printf("=== MEMORY LEAK ===\n");
	BTreePrint( &BTreeMem );

	NHeapPoolDump( &Pool );


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

	int	Ret;

	for( i = 0; i < 10; i++ ) {
		BTInsert( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	while( BTCHead( &Cursor ) == 0 ) {
		printf("DELETE(%p)\n", BTCGetKey( void*, &Cursor ) );
		Ret = BTCDelete( &Cursor );
	}
	BTCursorClose( &Cursor );
	BTreePrint( &BTree );

	BTreeInit( &BTreeMem, 3, NULL, PrintMem, BTMemAlloc, BTMemFree, NULL );
	BTreeInit( &BTree, 5, NULL, NULL, BTMalloc, BTFree, NULL );
	for( i = 206; i < 285; i++ ) {
		BTInsert( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	while( BTCHead( &Cursor ) == 0 ) {
		printf("DELETE(%d)\n", PNT_INT32(BTCGetKey( void*, &Cursor )) );
		Ret = BTCDelete( &Cursor );
		BTreePrint( &BTree );
	}
	BTCursorClose( &Cursor );
	BTreePrint( &BTree );
printf("=== MEMORY LEAK ===\n");
	BTreePrint( &BTreeMem );

	NHeapPoolDestroy( &Pool );
	NHeapPoolInit( &Pool, 1024*10, 10, malloc, free, 0/*HEAP_DYNAMIC*/ );
	BTreeInit( &BTreeMem, 5, NULL, NULL, BTMemAlloc, BTMemFree, NULL );
	for( i = 206; i < 285; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(i) );
	}
	BTreePrint( &BTreeMem );
	BTCursorOpen( &Cursor, &BTreeMem );
	while( BTCHead( &Cursor ) == 0 ) {
		printf("DELETE(%d)\n", PNT_INT32(BTCGetKey( void*, &Cursor )) );
		Ret = BTCDelete( &Cursor );
		BTreePrint( &BTreeMem );
	}
	BTCursorClose( &Cursor );
	BTreePrint( &BTreeMem );
//	BTreeDestroy( &BTreeMem );
	NHeapPoolDump( &Pool );

//#define	RAND_ARRAY	151
#define	RAND_ARRAY	10000
	int	aRand[RAND_ARRAY];

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
		Ret = BTDelete( &BTreeMem, INT32_PNT(aRand[i]) );
		ASSERT( !Ret );
	}
	BTreePrint( &BTreeMem );
	NHeapPoolDump( &Pool );

	for( i = 0; i < RAND_ARRAY; i++ ) {
		aRand[i]	= rand();
	}
	for( i = 0; i < RAND_ARRAY; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(aRand[i]) );
	}
	for( i = 0; i < RAND_ARRAY; i++ ) {
		Ret = BTDelete( &BTreeMem, INT32_PNT(aRand[i]) );
		ASSERT( !Ret );
	}
	BTreePrint( &BTreeMem );
	NHeapPoolDump( &Pool );

	for( i = 0; i < 100; i++ ) {
		BTInsert( &BTreeMem, INT32_PNT(i) );
	}
	BTCursorOpen( &Cursor, &BTreeMem );
	for( Ret = BTCHead( &Cursor ); Ret == 0; Ret = BTCNext( &Cursor ) ) {
		i = PNT_INT32(BTCGetKey( void*, &Cursor ));
printf("%d\n", i );
		BTCDelete( &Cursor );
	}
	// Destroy
	BTreeInit( &BTree, 1, NULL, NULL, BMalloc, BFree, BDestroy );
	for( i = 0; i < 20; i++ ) {
		BTInsert( &BTree, INT32_PNT(i) );
	}
	BTreePrint( &BTree );

	BTreeDestroy( &BTree );

	BTreePrint( &BTree );

	return( 0 );
}
