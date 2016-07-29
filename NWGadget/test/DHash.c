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
 * @brief Hash ÇÃä÷êîåQ
 */

#include	"NWGadget.h"

void
print( void* pKey, void* pValue )
{
//	Printf("[%d %s] ", (int)pKey, (char*)pValue );
	Printf("[%s %s] ", (char*)pKey, (char*)pValue );
}

Hash_t	HashMem;

void*
HashMalloc( size_t s )
{
	void*	p;

	p = Malloc( s );
	HashPut( &HashMem, p, p );
	return( p );
}
void
HashFree( void* p )
{
	Free( p );
	HashRemove( &HashMem, p );
}

int
main( int ac, char* av[] )
{
	DHash_t	DHash;
	void	*pVoid;

	HashInit( &HashMem, 3, HASH_CODE_PNT, HASH_CMP_PNT, 
				NULL, NULL, NULL, NULL );	

	HashDump( &HashMem );
	DHashInit( &DHash, 2, 2, 3,
				HASH_CODE_STR, HASH_CMP_STR, 
				print, HashMalloc, HashFree, NULL );	

	DHashPut( &DHash, "0", "A-0" );
	DHashPut( &DHash, "1", "A-1" );
	DHashPut( &DHash, "2", "A-2" );
	DHashPut( &DHash, "3", "A-3" );
	DHashPut( &DHash, "4", "A-4-0" );
	DHashPut( &DHash, "4", "A-4-1" );
	DHashPut( &DHash, "4", "A-4-2" );

	HashDump( &HashMem );
	printf("# Total=%d #\n", DHashCount( &DHash ) );
	DHashDump( &DHash );

	printf("# DHashGet/Next(%s) Start #\n", "4");
	pVoid = DHashGet( &DHash, "4" );
	while( pVoid ) {
		printf("[%s]\n", (char*)pVoid );
		pVoid = DHashNext( &DHash, "4" );
	}
	printf("# DHashGet/Next(%s) End #\n", "4");

	printf("\n### REMOVE(%s) ###\n", "4" );
	DHashRemove( &DHash, "4" );
	printf("# Total=%d #\n", DHashCount( &DHash ) );
	DHashDump( &DHash );

	printf("# DHashGet/Next(%s) Start #\n", "4");
	pVoid = DHashGet( &DHash, "4" );
	while( pVoid ) {
		printf("[%s]\n", (char*)pVoid );
		pVoid = DHashNext( &DHash, "4" );
	}
	printf("# DHashGet/Next(%s) End #\n", "4");

	printf("\n### REMOVE(%s) ###\n", "0" );
	DHashRemove( &DHash, "0" );
	printf("# Total=%d #\n", DHashCount( &DHash ) );
	DHashDump( &DHash );

	printf("# DHashGet/Next(%s) Start #\n", "4");
	pVoid = DHashGet( &DHash, "4" );
	while( pVoid ) {
		printf("[%s]\n", (char*)pVoid );
		pVoid = DHashNext( &DHash, "4" );
	}
	printf("# DHashGet/Next(%s) End #\n", "4");

	printf("\n### REMOVE(%s) ###\n", "1" );
	DHashRemove( &DHash, "1" );
	printf("### REMOVE(%s) ###\n", "2" );
	DHashRemove( &DHash, "2" );
	printf("### REMOVE(%s) ###\n", "3" );
	DHashRemove( &DHash, "3" );
	printf("### REMOVE(%s) ###\n", "4" );
	DHashRemove( &DHash, "4" );
	printf("### REMOVE(%s) ###\n", "4" );
	DHashRemove( &DHash, "4" );

	HashDump( &HashMem );
	printf("# Total=%d #\n", DHashCount( &DHash ) );
	DHashDump( &DHash );

	printf("\n### Destroy ###\n");
	DHashDestroy( &DHash );
	HashDump( &HashMem );

	return( 0 );
}

