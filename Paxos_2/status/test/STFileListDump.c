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


#include	"Status.h"

void
usage()
{
	printf("STFileListDump file_list\n");
}

int
main( int ac, char *av[] )
{
	StatusEnt_t	S;
	IOFile_t	*pF;
	int		Ret = 0;

	if( ac < 2 ) {
		usage();
		Ret = -1;
		goto err;
	}
	pF = IOFileCreate( av[1], O_RDONLY, 0, Malloc, Free );
	if( !pF )	goto err;

	while( IORead( (IOReadWrite_t*)pF, &S, sizeof(S) ) > 0 ) {
		printf("[%s]\n", S.e_Name );
	}
	IOFileDestroy( pF, TRUE );
err:
	return( Ret );
}

