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
#define	ITEM_NUM	2

r_item_t	items[ITEM_NUM];
char	buff[100];
r_sort_t sort[2];

neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_tbl_t	*td;
	r_head_t	*rp;
	char	*cp;
	int	i, n;
	char	text[50];
DBG_ENT(M_RDB,"neo_main");

	rp = (r_head_t *)buff;

	if( ac < 4 ) {
		printf("usage:test db_man db_local TABLE\n");
		neo_exit( 1 );
	}
DBG_A(("db_man=[%s],my_port=[%s],table=[%s]\n", av[1], av[2], av[3] ));

	if( !(md = rdb_link( av[2], av[1] ) ))
		goto err1;

	/*
	 *	table create
	 */
	 bcopy( "NAME", items[0].i_name, 4 );
	 items[0].i_attr	= R_STR;
	 items[0].i_len		= 8;
	 items[0].i_size	= 8;
	 bcopy( "123", items[1].i_name, 3 );
	 items[1].i_attr	= R_HEX;
	 items[1].i_len		= 4;
	 items[1].i_size	= 4;
	
	if( !(td = rdb_open( md, av[3] ) ) )
		goto err1;
	if( rdb_hold( 1, &td ) )
		goto err1;

	neo_sleep( 20 );

	if( rdb_release( td ) )
		goto err1;
	goto end;
	/*
	 *	search condition
	 */
	if( rdb_search( td, "NAME=='[1]'") )
		goto err1;
	if( ( i = rdb_find( td, rp, 1, 0 ) < 0 ) )
		goto err1;
	DBG_A(("ret=%d,rp->r_abs=%d,NAME=<%s>\n", i, rp->r_abs, rp+1 ));

	if( rdb_rewind( td ) )
		goto err1;

	strcpy( sort[0].s_name, "NAME");
	/***
	sort[0].s_order = R_ASCENDANT;
	***/
	sort[0].s_order = R_DESCENDANT;
	n = 1;
	if( rdb_sort( td, n, sort ) )
		goto err1;
	if( rdb_cancel( td ) )
		goto err1;
	if( rdb_flush( td ) )
		goto err1;
	if( rdb_close( td ) )
		goto err1;
end:
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
