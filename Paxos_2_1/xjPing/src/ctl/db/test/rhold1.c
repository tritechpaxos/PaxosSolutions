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
		printf("usage:rhold1 server TABLE sleep(sec)\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, av[2] ) ) )
		goto err1;
	/*
	 *	search condition
	 */
	if( rdb_search( td, "NAME=='2'") )
		goto err1;

	printf("FIND(LOCK)\n");
	while( ( i = rdb_find( td, 1, rp, R_EXCLUSIVE )) > 0  ) {

		DBG_A(("ret=%d,rp->r_abs=%d,NAME=<%s>\n", 
					i, rp->r_abs, rp+1 ));

		print_record( 0, rp, td->t_scm->s_n, td->t_scm->s_item );
	}

	DBG_A(("i=%d,neo_errno=0x%x\n", i, neo_errno ));

	if( i >= 0 ) {
		printf("SLEEP( %d sec) \n", atoi(av[3]) );
		neo_sleep( atoi(av[3]) );
	}

	printf("UNLOCK\n");
	if( rdb_search( td, 0 ) )
		goto err1;

	printf("SLEEP( %d sec) \n", atoi(av[3]) );
	neo_sleep( atoi(av[3]) );

	printf("CLOSE\n");

	if( rdb_close( td ) )
		goto err1;

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
