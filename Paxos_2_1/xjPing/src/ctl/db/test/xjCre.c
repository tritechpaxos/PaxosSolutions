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
#define	ITEM_NUM	7

r_item_t	items[ITEM_NUM];

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	char		*rp;
	int		i;
	char		text[50];
	float		f;
	int		m;
	char		table[128];
	int		j;
	int		total;
	int		total1;
	char		*s;
	long long	l;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
printf("usage:cre server TABLE [number_of_tables number_of_record]\n");
		neo_exit( 1 );
	}
	if( ac >= 5 )
		m = atoi( av[3] );
	else
		m = 0;
	if( m < 0 || 1000 < m )
		m = 0;

	if( ac >= 5 ) {
		total = atoi( av[4] );
		if( total < 2 )
			total = REC_NUM;
		if( 1000000 < total )
			total = 1000000;
	} else
		total = REC_NUM;


	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	/*
	 *	table create
	 */
	 strcpy( items[0].i_name, "NAME" );
	 items[0].i_attr	= R_STR;
	 items[0].i_len		= 10;

	 strcpy( items[1].i_name, "HEX" );
	 items[1].i_attr	= R_HEX;
	 items[1].i_len		= 4;

	 strcpy( items[2].i_name, "INT" );
	 items[2].i_attr	= R_INT;
	 items[2].i_len		= 4;
	
	 strcpy( items[3].i_name, "UINT" );
	 items[3].i_attr	= R_UINT;
	 items[3].i_len		= 4;
	
	 strcpy( items[4].i_name, "FLOAT" );
	 items[4].i_attr	= R_FLOAT;
	 items[4].i_len		= 4;
	
	 strcpy( items[5].i_name, "LONG" );
	 items[5].i_attr	= R_LONG;
	 items[5].i_len		= sizeof(long long);
	
	 strcpy( items[6].i_name, "ULONG" );
	 items[6].i_attr	= R_ULONG;
	 items[6].i_len		= sizeof(unsigned long long);
	
	for( j = 0; j <= m; j++ ) {

		bzero( table, sizeof(table) );
		if( j == 0 )
			sprintf( table, "%s", av[2] );
		else
			sprintf( table, "%s_%d", av[2], j );	

		if( !(td = rdb_create( md, table, total, ITEM_NUM, items ) ) )
			goto err1;
	
		/*
		 *	insert
		 */
		if( !(s = neo_zalloc( td->t_rec_size*total ) ) )
			goto err1;

		for( i = 0, rp = s; i < (total1 = total-2); 
					i++, rp += td->t_rec_size ) {

			sprintf( text, "%d", i );
			if( rdb_set( td, (r_head_t*)rp, "NAME", text ) )
				goto err1;
			if( rdb_set( td, (r_head_t*)rp, "HEX", (char*)&i ) )
				goto err1;
			if( rdb_set( td, (r_head_t*)rp, "INT", (char*)&i ) )
				goto err1;
			if( rdb_set( td, (r_head_t*)rp, "UINT", (char*)&i ) )
				goto err1;
			f = i;
			if( rdb_set( td, (r_head_t*)rp, "FLOAT", (char*)&f ) )
				goto err1;
			l = i;
			if( rdb_set( td, (r_head_t*)rp, "LONG", (char*)&l ) )
				goto err1;
			if( rdb_set( td, (r_head_t*)rp, "ULONG", (char*)&l ) )
				goto err1;

/***
			if( rdb_insert( td, 1, rp, 0, 0, 0 ) )
				goto err1;
***/
		}
		if( rdb_insert( td, total1, (r_head_t*)s, 0, 0, 0 ) )
			goto err1;

		if( rdb_close( td ) )
			goto err1;
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
