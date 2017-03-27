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
	printf("STDelete path del_list\n");
}

int
main( int ac, char *av[] )
{
	IOFile_t	*pF;
	int			l, Len;
	PathEnt_t	Ent;
	char		Path[FILENAME_MAX];
	int		Ret = 0;

	if( ac < 3 ) {
		usage();
		Ret = -1;
		goto err;
	}
	pF = IOFileCreate( av[2], O_RDONLY, 0, Malloc, Free );
	if( !pF )	goto err;
	strcpy( Path, av[1] );if( Path[0] )	strcat( Path, "/" );
	l = strlen( Path );
	while( IORead( (IOReadWrite_t*)pF, &Len, sizeof(Len) ) > 0 ) {
		IORead( (IOReadWrite_t*)pF, &Ent, sizeof(Ent) );
		IORead( (IOReadWrite_t*)pF, &Path[l], Len - sizeof(Ent) );
		Path[l+Len-sizeof(Ent)] = 0;

		if( Ent.p_Type & ST_DIR )	rmdir( Path );
		else						unlink( Path );
	}
	IOFileDestroy( pF, TRUE );
err:
	return( Ret );
}

