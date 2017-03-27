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

p_port_t	local_port, server_port;

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	int	i;
	int	m = 0;
	char	name[128];
DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
		printf("usage:test server TABLE [number_of_tables]\n");
		neo_exit( 1 );
	}
	if( ac > 3 )
		m = atoi( av[3] );

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( rdb_drop( md, av[2] ) )
		goto err1;
	for( i = 1; i <= m; i++ ) {
		sprintf( name, "%s_%d", av[2], i );
		rdb_drop( md, name );
	}

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	printf("%s\n", neo_errsym() );
	rdb_unlink( md );
	return( -1 );
}
