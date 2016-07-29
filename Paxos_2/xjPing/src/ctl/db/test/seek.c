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

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	r_head_t	*rp;
	char		*bufp;
	int		i, n;
	int		size, num;
DBG_ENT(M_RDB,"neo_main");

	if( ac < 4 ) {
		printf("usage:test dbServer1 CLIENT TABLE\n");
		neo_exit( 1 );
	}
	DBG_A(("db_man=[%s],my_port=[%s],table=[%s]\n", av[1], av[2], av[3] ));

	if( !(md = rdb_link( av[2], av[1] ) ))
		goto err1;

	/*
	 *	table open
	 */
	if( !(td = rdb_open( md, av[3] ) ) )
		goto err1;

	/*
	 *	parameters
	 */
	size = rdb_size( td );
	num  = rdb_number( td, 1 );
	DBG_A(("size=%d,num=%d\n", size, num ));
	/*
	 *	allocate buffer
	 */
	if( !(bufp = (char *)neo_malloc( num*size ) ) )
		goto err1;
	/*
	 *	find
	 */
	while( (n = rdb_find( td, num, (r_head_t*)bufp, 0 ) ) > 0 ) {

		DBG_A(("n=%d\n", n ));
		for( i = 0, rp = (r_head_t*)bufp; i < n;
				i++, rp = (r_head_t*)((char*)rp + size ) ) {
	
			DBG_A(("rec(rel=%d,abs=%d,stat=0x%x[%s])\n", 
				rp->r_rel, rp->r_abs, rp->r_stat, rp+1 ));

			print_record(i, rp, td->t_scm->s_n, td->t_scm->s_item);
		}
	}
	DBG_A(("n=%d,neo_errno = 0x%x\n", n, neo_errno ));
	/*
	 *	seek
	 */
	printf("seek:\n");
	rdb_seek( td, 10 );
	if( (n = rdb_find( td, 10, (r_head_t*)bufp, 0 ) ) > 0 ) {

		DBG_A(("n=%d\n", n ));
		for( i = 0, rp = (r_head_t*)bufp; i < n;
				i++, rp = (r_head_t*)((char*)rp + size ) ) {
	
			DBG_A(("rec(rel=%d,abs=%d,stat=0x%x[%s])\n", 
				rp->r_rel, rp->r_abs, rp->r_stat, rp+1 ));

			print_record(i, rp, td->t_scm->s_n, td->t_scm->s_item);
		}
	}
	/*
	 *	close
	 */
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

