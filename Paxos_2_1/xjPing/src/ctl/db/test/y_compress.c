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

#include	<stdio.h>
#include	"neo_db.h"

#define	REC_NUM		1000
#define	ITEM_NUM	16
#define	CLIENT_PORT	"dbman_client@amur"
#define	SERVER_PORT	"db_man@amur"
#define	INITTABLE	"data"

r_item_t	items[ITEM_NUM];
char		buff[520];

neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	int		i, n;
	float		f;
	int		m;
	char		table[128];
	int		j;
	int		total;
	int		num,cases;
	FILE		*fp;
	r_head_t	rec;

DBG_ENT(M_RDB,"neo_main");


	if( ac < 2 ) {
printf("usage:y_compress TABLE_NAME  numofspacerecord case \n");
		neo_exit( 1 );
	}

	total = REC_NUM;

	if( !(md = rdb_link( CLIENT_PORT, SERVER_PORT ) ))
		goto err1;

	/*
	 *	table create
	 */
	
	for ( j = 0; j < ITEM_NUM; j ++ ) {

		sprintf( items[j].i_name, "ITEM_%d", j );
	 	items[j].i_attr	= R_STR;
	 	items[j].i_len	= 16;

	}

	bzero( table, sizeof(table) );

	sprintf( table, "%s", av[1] );

	if( !(td = rdb_create( md, table, total, ITEM_NUM, items ) ) )
		goto err1;

	if ( ( fp = fopen(INITTABLE, "r") ) != NULL ) {

		init_table( td,fp );
		fclose( fp );
	}
	
	num = atoi(av[2]);
	cases = atoi(av[3]);

	/*
	 * cross
	 */
	if ( cases == 1 ) {

	 	for ( i = 0; i < num; i ++ ) {

			rec.r_rel = rec.r_abs = 2*i;
			
			if ( rdb_delete( td, 1, &rec, 0 ) )
				goto	err1;
		}
	} else {

		for ( i = 300, j = 0; j < num; i ++, j++ ) {

			rec.r_rel = rec.r_abs = i;

			if ( rdb_delete( td, 1, &rec, 0 ) )
				goto	err1;
		}
	}

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

int
init_table( td, fp )
r_tbl_t	*td;
FILE	*fp;
{
	int		i, j;
	r_head_t	*rp;
	char		iname[20];
	char		data[16];

	rp = (r_head_t *)buff;

	for( i = 0; i < REC_NUM; i++ ) {

		for ( j = 0; j < ITEM_NUM; j ++ ) {
			if (  fscanf(fp, "%s\n", data ) == EOF )
			goto	err1;

			sprintf(iname, "ITEM_%d", j );

			if( rdb_set( td, rp, iname, data) )
				goto err1;
		}

		if( rdb_insert( td, 1, rp, 0, 0, 0 ) )
			goto err1;
	}

err1: ;

}
