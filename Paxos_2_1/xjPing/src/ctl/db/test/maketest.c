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

#define	REC_NUM		10
#define	ITEM_NUM	5


neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	p_name_t	myname;
	r_man_name_t	database;
	r_tbl_t		*td;
	r_tbl_name_t	name;
	r_item_t	items[ITEM_NUM];
	char		buff[100];
	r_sort_t	sort[10];
	r_hold_t	hds[10];
	r_item_name_t	itemname;

	r_inf_t		*infp;
	r_rec_t		*rec;
	r_usr_t		*usr;
	r_scm_t		*scm;

	r_head_t	*rp;

	float		f;
	int		i, n;
	char		text[50];
	int		m;
	char		table[128];
	int		j;
	int		total;


	if( !(md = rdb_link( myname, database )) )
		goto err1;

	if( rdb_unlink( md ) )
		goto err1;

	if( !(td = rdb_create( md, table, total, ITEM_NUM, items ) ) )
		goto err1;

	if( rdb_drop( md, name ) )
		goto err1;

	if( !(td = rdb_open( md, name )) )
		goto err1;

	if( rdb_close( td ) )
		goto err1;

	if( rdb_flush( td ) )
		goto err1;

	if( rdb_cancel( td ) )
		goto err1;

	if( rdb_insert( td, n, rp, 0, 0, R_EXCLUSIVE ) )
		goto err1;

	if( rdb_insert( td, n, rp, 0, 0, R_SHARE ) )
		goto err1;

	if( rdb_delete( td, n, rp, R_WAIT ) )
		goto err1;

	if( rdb_delete( td, n, rp, R_NOWAIT ) )
		goto err1;

	if( rdb_update( td, n, rp, R_SHARE ) )
		goto err1;

	if( rdb_search( td, text ) )
		goto err1;

	if( rdb_find( td, n, rp, (R_SHARE|R_WAIT) ) )
		goto err1;

	if( rdb_rewind( td ) )
		goto err1;

	sort[0].s_order	= R_ASCENDANT;
	sort[1].s_order	= R_DESCENDANT;
	if( rdb_sort( td, n, sort ) )
		goto err1;

	if( rdb_hold( n, hds, R_WAIT ) )
		goto err1;
	
	if( rdb_release( td ) )
		goto err1;

	if( rdb_compress( td ) )
		goto err1;

	if( rdb_set( td, rp, "FLOAT", &f ) )
		goto err1;

	if( rdb_get( td, rec, itemname, &f, &n ) )
		goto err1;

	if( (n = rdb_size( td )) < 0 )
		goto err1;

	if( (n = rdb_number( td, 0 )) < 0 )
		goto err1;

	if( rdb_direct( td, n, rp, n ) )
		goto err1;

	if( rdb_dump_proc( md ) )
		goto err1;
	
	if( rdb_dump_inf( md, infp ) )
		goto err1;

	if( rdb_dump_record( td, n, rec, buff ) )
		goto err1;

	if( rdb_dump_tbl( td, td ) )
		goto err1;

	if( rdb_dump_usr( td, n, usr ) )
		goto err1;

	if( rdb_dump_mem( md, buff, n, buff ) )
		goto err1;


	return( 0 );
err1:
	return( -1 );
}
