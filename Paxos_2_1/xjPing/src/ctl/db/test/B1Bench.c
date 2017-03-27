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
#include	"../sql/sql.h"
#include	"stdlib.h"

#define	REC_NUM		10
#define	ITEM_NUM	2

r_item_t	items[ITEM_NUM];

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	int		i;
	int		total;
	int		record_size;
	int		number_of_records;
	char	format[512];
	char	statement[512];
	time_t	s, e;
	int	r;
	r_scm_t		*pScm;
	r_item_t	*pItem;
	int	key_len, data_len;
	char	*pStatement;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 4 ) {
		printf("usage:B1Bench server TABLE try_number\n");
		neo_exit( 1 );
	}
	total 		= atoi( av[3] );

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, av[2] )) ) {
		goto err1;
	}
	record_size	= rdb_size( td );
	number_of_records	= rdb_number( td, 1 );
	printf("size=%d number=%d\n", record_size, number_of_records );
	pScm	= td->t_scm;
	for( i = 0; i < pScm->s_n; i++ ) {
		pItem	= &pScm->s_item[i];
		printf("[%s] len=%d size=%d\n", 
			pItem->i_name, pItem->i_len, pItem->i_size  );
		if( !strcmp( pItem->i_name, "KEY_DATA" ) ) {
			key_len		= pItem->i_len;
		}
		if( !strcmp( pItem->i_name, "VALUE_DATA" ) ) {
			data_len	= pItem->i_len;
		}
	}

	rdb_close( td );
	pStatement	= neo_malloc( data_len + 512 );
	if( !pStatement ) {
		exit( -1 );
	}

	/* Only selec t */
	s = time( 0 );
	for( i = 0; i < total; i++ ) {

		r	=  random();
		r	-= 	(r/number_of_records)*number_of_records;

		sprintf( format, "%s%d%s",
				"select * from %s where KEY_DATA='%0", key_len, "d';" );
			
		sprintf( statement, format, av[2], r );

		if( !(td = SqlQuery( md, statement )) ) {
			goto err1;
		}
if( td->t_rec_num != 1 ) {
	printf("%d [%s]\n", td->t_rec_num, statement );
}
ASSERT(td->t_rec_num == 1 );
		SqlResultClose( td );

	}
	e = time( 0 );
	printf( "SELECT: %lu sec/%d = %f sec\n", 
			e - s, total, ((float)(e-s))/total ); 

	/* Only update */
	s = time( 0 );
	for( i = 0; i < total; i++ ) {

		r	=  random();
		r	-= 	(r/number_of_records)*number_of_records;

		sprintf( format, "%s%d%s%s%d%s",
				"update %s set VALUE_DATA='%0", data_len, "d'",
				" where KEY_DATA='%0", key_len, "d';" );
			
		sprintf( pStatement, format, av[2], 9, r );
		if( !(td = SqlUpdate( md, pStatement )) ) {
			goto err1;
		}
if( td->t_rec_num != 1 ) {
	printf("%d [%s]\n", td->t_rec_num, statement );
}
ASSERT(td->t_rec_num == 1 );
		SqlResultClose( td );
	}
	e = time( 0 );
	printf( "UPDATE: %lu sec/%d = %f sec\n", 
				e - s, total, ((float)(e-s))/total ); 

	/* Select, Update and Commit */
	s = time( 0 );
	for( i = 0; i < total; i++ ) {

		r	=  random();
		r	-= 	(r/number_of_records)*number_of_records;

		sprintf( format, "%s%d%s",
				"select * from %s where KEY_DATA='%0", key_len, "d';" );
			
		sprintf( statement, format, av[2], r );

		if( !(td = SqlQuery( md, statement )) ) {
			goto err1;
		}
if( td->t_rec_num != 1 ) {
	printf("%d [%s]\n", td->t_rec_num, statement );
}
ASSERT(td->t_rec_num == 1 );
		SqlResultClose( td );

		r	=  random();
		r	-= 	(r/number_of_records)*number_of_records;

		sprintf( format, "%s%d%s%s%d%s",
				"update %s set VALUE_DATA='%0", data_len, "d'",
				" where KEY_DATA='%0", key_len, "d';" );
			
		sprintf( pStatement, format, av[2], 9, r );
		if( !(td = SqlUpdate( md, pStatement )) ) {
			goto err1;
		}
if( td->t_rec_num != 1 ) {
	printf("%d [%s]\n", td->t_rec_num, statement );
}
ASSERT(td->t_rec_num == 1 );
		SqlResultClose( td );
	}
	SqlExecute( md, "commit;" );
	e = time( 0 );
	printf( "SELECT/UPDATE AND COMMIT: %lu sec/%d = %f sec\n", 
				e - s, total, ((float)(e-s))/total ); 
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
