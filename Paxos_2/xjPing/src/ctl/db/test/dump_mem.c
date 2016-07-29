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


r_tbl_t	tbl;

neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_tbl_t	*td;
	r_head_t	*rp;
	int	i, n;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 4 ) {
		printf("usage:dump_mem db_man db_local TABLE\n");
		neo_exit( 1 );
	}
DBG_A(("db_man=[%s],my_port=[%s],table=[%s]\n", av[1], av[2], av[3] ));

	if( !(md = rdb_link( av[2], av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, av[3] ) ) )
		goto err1;

	if( rdb_dump_mem( md, td->t_rtd, sizeof(r_tbl_t), &tbl ) < 0 )
		goto err1;

	print_tbl( &tbl );

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

print_tbl( td )
	r_tbl_t	*td;
{
	printf("TABLE NAME[%s]\n", td->t_name );

	printf("stat		= 0x%x\n",	td->t_stat );
	printf("user count	= %d\n", 	td->t_usr_cnt );
	printf("lock user	= 0x%x\n", 	td->t_lock_usr );
	printf("wait count	= %d\n", 	td->t_wait_cnt );
	printf("record size	= %d\n", 	td->t_rec_size );
	printf("total records	= %d\n", 	td->t_rec_num );
	printf("used records	= %d\n", 	td->t_rec_used );
	printf("tag		= 0x%x\n", 	td->t_tag );
	printf("access time	= %s\n", 	ctime(&td->t_access) );
}
