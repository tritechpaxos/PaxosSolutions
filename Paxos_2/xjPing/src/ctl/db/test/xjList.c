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

#define	DB_MAN_SYSTEM

#include	<stdio.h>
#include	"neo_db.h"

r_man_t		*md;

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_res_list_t	*resp;
	int		i;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 2 ) {
		printf("usage:sql db_man\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( rdb_list( md, &resp ) )
		goto err1;

	rdb_unlink( md );

	/*
	 *	Print out
	 */
	for( i = 0; i < resp->l_n; i++ ) {
		printf("%d.[%s] %jd \t%s\n", i,
				resp->l_file[i].f_name,
				(intmax_t)resp->l_file[i].f_stat.st_size, 
				resp->l_file[i].f_stmt );
	}

DBG_EXIT(0);
	return( 0 );

err1:
	rdb_unlink( md );
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	printf("%s\n", neo_errsym() );
	return( -1 );
}
