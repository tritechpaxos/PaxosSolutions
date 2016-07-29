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
//	BTInsert( &BTreeMem, p );
	return( p );
}

void
BTFree( void* p )
{
//	BTDelete( &BTreeMem, p );
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
	BTCursor_t	Cursor1;
	BTCursor_t	Cursor2;
	int	R;
	int	i;

//	NHeapPoolInit( &Pool, 1024*10, 3, malloc, free, 0/*HEAP_DYNAMIC*/ );

//	BTreeInit( &BTreeMem, 1, NULL, PrintMem, BTMemAlloc, BTMemFree );
	BTreeInit( &BTree, 1, NULL, NULL, BTMalloc, BTFree, NULL );

	BTCursorOpen( &Cursor, &BTree );
	BTCursorOpen( &Cursor1, &BTree );
	BTCursorOpen( &Cursor2, &BTree );

	BTreePrint( &BTree );

	if((R=BTCFind( &Cursor1, INT32_PNT(20)))) printf("BTCFind ERROR 20\n");
	ASSERT( R );

	if((R=BTCFindLower(&Cursor2,INT32_PNT(20)))) {
		printf("BTCFindLower ERROR 20\n");
	}
	ASSERT( R );
	printf("\n");

	printf("Insert 20\n");
	BTInsert( &BTree, INT32_PNT(20) );
	BTreePrint( &BTree );

	if( (R=BTCFind( &Cursor1, INT32_PNT(21)))) printf("BTCFind ERROR 21\n");
	ASSERT( R );

	if( BTCFind( &Cursor1, INT32_PNT(20) )) printf("BTCFind ERROR 20\n");
	else printf("BTCFind:20=%d\n", (R=PNT_INT32(BTCGetKey( void*, &Cursor1))));
	ASSERT( 20 == R );

	if( (R=BTCFind( &Cursor1, INT32_PNT(19)))) printf("BTCFind ERROR 19\n");
	ASSERT( R );

	if((R=BTCFindLower(&Cursor2,INT32_PNT(21)))) {
		printf("BTCFindLower ERROR 21\n");
	}
	ASSERT( R );

	if(BTCFindLower(&Cursor2,INT32_PNT(20))) printf("BTCFindLower ERROR 20\n");
	else 
	  printf("BTCFindLower:20<=%d\n",(R=PNT_INT32(BTCGetKey(void*,&Cursor2))) );
	ASSERT( 20 <= R );

	if(BTCFindLower(&Cursor2,INT32_PNT(19))) printf("BTCFindLower ERROR 19\n");
	else 
	  printf("BTCFindLower:19<=%d\n", (R=PNT_INT32(BTCGetKey(void*,&Cursor2))));
	ASSERT( 19 <= R );
	printf("\n");

	printf("Insert 10\n");
	BTInsert( &BTree, (void*)10 );
	BTreePrint( &BTree );

	if( BTCFind( &Cursor1, INT32_PNT(20) )) printf("BTCFind ERROR 20\n");
	else printf("BTCFind:20=%d\n", PNT_INT32(BTCGetKey( void*, &Cursor1 )) );
	if(BTCFindLower(&Cursor2,INT32_PNT(19))) printf("BTCFindLower ERROR 19\n");
	else printf("BTCFindLower:19<=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	printf("\n");

	printf("Insert 30\n");
	BTInsert( &BTree, (void*)30 );
	BTreePrint( &BTree );

	if( BTCFind( &Cursor1, (void*)19 )) printf("BTCFind ERROR 19\n");
	else printf("BTCFind:19=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)) );
	if( BTCFindLower( &Cursor2, (void*)19 )) printf("BTCFindLower ERROR 19\n");
	else printf("BTCFindLower:19<=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));

	if( BTCFind( &Cursor1, (void*)31 )) printf("BTCFind ERROR 31\n");
	else printf("BTCFind:31=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)) );
	if( BTCFindLower( &Cursor2, (void*)31 )) printf("BTCFindLower ERROR 31\n");
	else printf("BTCFindLower:31<=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	printf("\n");

	printf("Insert 20\n");
	BTInsert( &BTree, (void*)20 );
	BTreePrint( &BTree );

	if( BTCFind( &Cursor1, (void*)20 )) printf("BTCFind ERROR 20\n");
	else printf("BTCFind:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)));
	PrintCursor( &Cursor1 );
	if( BTCNext( &Cursor1 ) ) printf("BTCNext ERROR\n");
	else printf("BTCNext:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)));

	if( BTCFindLower( &Cursor2, (void*)20 )) printf("BTCFindLower ERROR 20\n");
	else printf("BTCFindLower:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	PrintCursor( &Cursor2 );
	if( BTCNext( &Cursor2 ) ) printf("BTCNext ERROR\n");
	else printf("BTCNext:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	printf("\n");

	printf("Insert 20\n");
	BTInsert( &BTree, (void*)20 );
	printf("Insert 20\n");
	BTInsert( &BTree, (void*)20 );
	printf("Insert 20\n");
	BTInsert( &BTree, (void*)20 );
	BTreePrint( &BTree );

	if( BTCFind( &Cursor1, (void*)20 )) printf("BTCFind ERROR 20\n");
	else printf("BTCFind:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)));
	PrintCursor( &Cursor1 );
	if( BTCNext( &Cursor1 ) ) printf("BTCNext ERROR\n");
	else printf("BTCNext:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor1)));

	if( BTCFindLower( &Cursor2, (void*)20 )) printf("BTCFindLower ERROR 20\n");
	else printf("BTCFindLower:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	PrintCursor( &Cursor2 );
	if( BTCNext( &Cursor2 ) ) printf("BTCNext ERROR\n");
	else printf("BTCNext:20=%d\n", PNT_INT32(BTCGetKey(void*,&Cursor2)));
	printf("\n");

	printf("BTCDelete\n");
	BTCDelete( &Cursor2 );
	BTreePrint( &BTree );
	PrintCursor( &Cursor2 );

	BTreeDestroy( &BTree );
	printf("\n");

	BTreeInit( &BTree, 2, NULL, NULL, BTMalloc, BTFree, NULL );

	for( i = 0; i < 8; i++ ) {
		BTInsert( &BTree, (void*)20 );
	}
	BTreePrint( &BTree );
		
	BTInsert( &BTree, (void*)30 );
	BTreePrint( &BTree );
	BTInsert( &BTree, (void*)20 );
	BTreePrint( &BTree );
	BTInsert( &BTree, (void*)20 );
	BTreePrint( &BTree );

	// BTCFindLower/BTCFindUpper
	printf("\n### BTCFindLower/BTCFindUpper ###\n");
	// one page
	BTreeInit( &BTree, 10, NULL, NULL, BTMalloc, BTFree, NULL );
	for( i = 1; i <= 4; i++ ) {
		BTInsert( &BTree, (void*)(uintptr_t)i );
		BTInsert( &BTree, (void*)(uintptr_t)i );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	for( i = 0; i <= 5; i++ ) {
		if( BTCFindLower( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindLower ERROR %d\n", i );
		else 
			printf("BTCFindLower:[%p-%d]%p(%d)>=%d\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
	}
	printf("\n");
	for( i = 0; i <= 5; i++ ) {
		if( BTCFindUpper( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindUpper ERROR %d\n", i );
		else {
			printf("BTCFindUpper:[%p-%d]%p(%d)<=%d  ",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
			if( !BTCNext( &Cursor ) ) {
				printf("BTCNext:[%p-%d]%p(%d)\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)) );
			} else
				printf("\n");
		}
	}
	// tree page
	printf("\n");
	BTreeInit( &BTree, 2, NULL, NULL, BTMalloc, BTFree, NULL );
	for( i = 1; i <= 4; i++ ) {
		BTInsert( &BTree, (void*)(uintptr_t)i );
		BTInsert( &BTree, (void*)(uintptr_t)i );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	for( i = 0; i <= 5; i++ ) {
		if( BTCFindLower( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindLower ERROR %d\n", i );
		else 
			printf("BTCFindLower:[%p-%d]%p(%d)>=%d\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
	}
	printf("\n");
	for( i = 0; i <= 5; i++ ) {
		if( BTCFindUpper( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindUpper ERROR %d\n", i );
		else {
			printf("BTCFindUpper:[%p-%d]%p(%d)<=%d  ",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
			if( !BTCNext( &Cursor ) ) {
				printf("BTCNext:[%p-%d]%p(%d)\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)) );
			} else
				printf("\n");
		}
	}
	for( i = 0; i <= 5; i++ ) {
		if( BTCFindUpper( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindUpper ERROR %d\n", i );
		else {
			printf("BTCFindUpper:[%p-%d]%p(%d)<=%d  ",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
			if( !BTCPrev( &Cursor ) ) {
				printf("BTCPrev:[%p-%d]%p(%d)\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)) );
			} else
				printf("\n");
		}
	}

	BTreeInit( &BTree, 2, NULL, NULL, BTMalloc, BTFree, NULL );
	for( i = 1; i <= 19; i++ ) {
		if( i == 3 )	continue;
		BTInsert( &BTree, (void*)(uintptr_t)i );
		BTInsert( &BTree, (void*)(uintptr_t)i );
	}
	BTreePrint( &BTree );
	BTCursorOpen( &Cursor, &BTree );
	for( i = 0; i <= 20; i++ ) {
		if( BTCFindLower( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindLower ERROR %d\n", i );
		else 
			printf("BTCFindLower:[%p-%d]%p(%d)>=%d\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
	}
	printf("\n");
	for( i = 0; i <= 20; i++ ) {
		if( BTCFindUpper( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindUpper ERROR %d\n", i );
		else {
			printf("BTCFindUpper:[%p-%d]%p(%d)<=%d ",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
			if( !BTCNext( &Cursor ) ) {
				printf("BTCNext:[%p-%d]%p(%d)\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)) );
			} else
				printf("\n");
		}
	}
	for( i = 0; i <= 20; i++ ) {
		if( BTCFindUpper( &Cursor, (void*)(uintptr_t)i )) 
			printf("BTCFindUpper ERROR %d\n", i );
		else {
			printf("BTCFindUpper:[%p-%d]%p(%d)<=%d ",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)), i );
			if( !BTCPrev( &Cursor ) ) {
				printf("BTCPrev:[%p-%d]%p(%d)\n",
					Cursor.c_pPage, Cursor.c_zIndex, 
					Cursor.c_pPage->p_aItems[Cursor.c_zIndex], 
					PNT_INT32(BTCGetKey(void*,&Cursor)) );
			} else
				printf("\n");
		}
	}

	return(0);
}
