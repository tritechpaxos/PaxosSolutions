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

#define	REC_NUM		100
#define	ITEM_NUM	16
#define	CLIENT_PORT	"dbman_client@amur"
#define	SERVER_PORT	"db_man@amur"
#define	INITTABLE	"data"

r_item_t	items[ITEM_NUM];
char		buff[520];
int		total;

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
	FILE		*fp;

DBG_ENT(M_RDB,"neo_main");


	if( ac < 2 ) {
printf("usage:y_cre TABLE_NAME [number_of_record]\n");
		neo_exit( 1 );
	}

	if ( ac == 2 )
		total = REC_NUM;
	else total = atoi(av[2]) ;


	if( !(md = rdb_link( CLIENT_PORT, SERVER_PORT ) ))
		goto err2;

	/*
	 *	table create
	 */
	
	for ( j = 0; j < ITEM_NUM; j ++ ) {

		sprintf( items[j].i_name, "IM%d", j );
	 	items[j].i_attr	= R_STR;
	 	items[j].i_len	= 64;

	}

	bzero( table, sizeof(table) );

	sprintf( table, "%s", av[1] );

	if( !(td = rdb_create( md, table, total, ITEM_NUM, items ) ) )
		goto err1;

	if ( ( fp = fopen(INITTABLE, "r") ) != NULL ) {

		init_table( td,fp );
		fclose( fp );
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
err2:
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

	for( i = 0; i < total; i++ ) {

		for ( j = 0; j < ITEM_NUM; j ++ ) {
			if (  fscanf(fp, "%s\n", data ) == EOF )
			goto	err1;

			sprintf(iname, "IM%d", j );

			if( rdb_set( td, rp, iname, data) )
				goto err1;
		}

		if( rdb_insert( td, 1, rp, 0, 0, 0 ) )
			goto err1;
	}

err1: ;

}
