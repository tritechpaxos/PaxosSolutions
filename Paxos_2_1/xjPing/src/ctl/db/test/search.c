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

#define	REC_NUM		10
char	buff[100];

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_tbl_t	*td;
	r_head_t	*rp;
	int	i;
DBG_ENT(M_RDB,"neo_main");

	rp = (r_head_t *)buff;

	if( ac < 4 ) {
		printf("usage:test db_man db_local TABLE\n");
		neo_exit( 1 );
	}
DBG_A(("db_man=[%s],my_port=[%s],table=[%s]\n", av[1], av[2], av[3] ));

	if( !(md = rdb_link( av[2], av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, av[3] ) ) )
		goto err1;
	/*
	 *	search condition
	 */
	if( rdb_search( td, "LONG==3") )
		goto err1;

	while( ( i = rdb_find( td, 1, rp, 0 ) > 0 ) ) {

		printf("ret=%d,rp->r_abs=%"PRIi64",NAME=<%s>\n", 
					i, rp->r_abs, (char*)(rp+1) );
	}

	printf("i=%d,neo_errno=0x%x\n", i, neo_errno );

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	rdb_unlink( md );
	return( -1 );
}
