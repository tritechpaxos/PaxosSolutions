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
	Hash_t	Hash;

	HashInit( &HashMem, 3, HASH_CODE_PNT, HASH_CMP_PNT, 
								NULL, NULL, NULL, NULL );	
	HashInit( &Hash, 3, HASH_CODE_STR, HASH_CMP_STR, 
							print, HashMalloc, HashFree, NULL );	

	HashPut( &Hash, "1", "A" );
	HashPut( &Hash, "2", "A" );
	HashPut( &Hash, "3", "A" );
	HashPut( &Hash, "4", "B" );
	HashPut( &Hash, "5", "C-5" );
	HashPut( &Hash, "6", "C-6" );

	HashDump( &Hash );
	HashDump( &HashMem );

	Printf("Value=%s\n", (char*)HASH_VALUE(HashGetItem( &Hash, "4" )) );

	HashPut( &Hash, "4", "C-4" );
	HashDump( &Hash );
	HashDump( &HashMem );
	Printf("Value=%s\n", (char*)HASH_VALUE(HashGetItem( &Hash, "4" )) );
	Printf("Value=%s\n", (char*)HASH_VALUE(HashNextItem( &Hash, "4" )) );

	HashRemove( &Hash, "4" );
	HashRemove( &Hash, "4" );
	HashDump( &Hash );
	HashDump( &HashMem );

	HashDestroy( &Hash );
	HashDump( &HashMem );

	return( 0 );
}

