/******************************************************************************
*
*  LogFiles.c 	--- LogFiles program of NWGadget Library
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
#include	<fcntl.h>

int
main( int ac, char* av[] )
{
	int		Size;
	int		Start;
	char	*pBuf;
	FILE*	pFile;
	char	name[256];
	int		n = 0;
	int		Max;
	int		fd;

	if( ac < 3 ) {
		printf("LogFiles file_name files\n");
		return( -1 );
	}
	Max	= atoi( av[2] );

	sprintf( name, "%s_%s", av[1], "NUM" );
	fd = open( name, O_RDWR|O_CREAT, 0666 );
	if( fd < 0 ) {
		return( -1 );
	}
	if( read( fd, &n, sizeof(n) ) != sizeof(n) ) {
		n = 0;
	}

	while( fread( &Size, sizeof(Size), 1, stdin ) > 0 ) {
if( Size > 1000000000LL ) {
	printf("LogFiles ERROR:Size=%d\n", Size );
	return( -1 );
}
		fread( &Start, sizeof(Start), 1, stdin );
		pBuf	= (char*)Malloc( Size );
		fread( pBuf, Size, 1, stdin );

		sprintf( name, "%s-%04d", av[1], n );
		pFile	= fopen( name, "w+" );

		fwrite( &Size, sizeof(Size), 1, pFile );
		fwrite( &Start, sizeof(Start), 1, pFile );
		fwrite( pBuf, Size, 1, pFile );

		fclose( pFile );

		if( n >= Max ) {
			sprintf( name, "%s-%04d", av[1], n - Max );
			unlink( name );
		}
		n++;
		lseek( fd, 0, SEEK_SET );
		write( fd, &n, sizeof(n) );

		Free( pBuf );
	}	
	return( 0 );
}
