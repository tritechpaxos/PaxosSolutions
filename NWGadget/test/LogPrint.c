/******************************************************************************
*
*  LogPrint.c 	--- LogPrint program of NWGadget Library
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

int
main( int ac, char* av[] )
{
	int		Size;
	int		Start;
	char	*pBuf;
	FILE*	pFile;

	pFile	= stdin;

	if( ac >= 2 ) {
		pFile = fopen( av[1], "r" );
		if( !pFile ) {
			exit( -1 );
		}
	}

	while( fread( &Size, sizeof(Size), 1, pFile ) > 0 ) {
		fread( &Start, sizeof(Start), 1, pFile );
		pBuf	= (char*)Malloc( Size );
		Size = (int)fread( pBuf, Size, 1, pFile );
		LogDumpBuf( Start, pBuf, stdout );
		Free( pBuf );
	}	
	return( 0 );
}
