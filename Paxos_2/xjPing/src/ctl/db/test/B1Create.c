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
#define	ITEM_NUM	2

r_item_t	items[ITEM_NUM];

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_tbl_t	*td;
	char	*rp, *rp_buf;
	int		i, j, k, ib;
	int		key_size, data_size;
	char	*pKey;
	char	*pData;
	int		total;
	char	key_format[128];
	char	data_format[128];

DBG_ENT(M_RDB,"neo_main");

	if( ac < 6 ) {
printf("usage:B1Create server TABLE key_size data_size record_number\n");
		neo_exit( 1 );
	}
	key_size	= atoi( av[3] );
	data_size	= atoi( av[4] );
	total 		= atoi( av[5] );

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	/*
	 *	table create
	 */
	 strcpy( items[0].i_name, "KEY_DATA" );
	 items[0].i_attr	= R_STR;
	 items[0].i_len		= key_size;
	 items[0].i_size	= key_size;

	 strcpy( items[1].i_name, "VALUE_DATA" );
	 items[1].i_attr	= R_STR;
	 items[1].i_len		= data_size;
	 items[1].i_size	= data_size;

	if( !(td = rdb_create( md, av[2], total, ITEM_NUM, items ) ) )
			goto err1;

#define	BLOCK_SIZE	1000
	
	rp_buf	= neo_zalloc( td->t_rec_size*BLOCK_SIZE );
	pKey	= neo_zalloc( key_size + 1 );
	pData	= neo_zalloc( data_size + 1 );

	sprintf( key_format, "%s%d%s", "%0", key_size, "d" );
	sprintf( data_format, "%s%d%s", "%0", data_size, "d" );

ib	= (total + BLOCK_SIZE - 1 )/BLOCK_SIZE;
for( j = 0; j < ib; j++ ) {
	for( k = 0; k < BLOCK_SIZE; k++ ) {

		i = BLOCK_SIZE*j + k;
		if( i >= total )	break; 

		rp	= rp_buf + td->t_rec_size*k;	
		/*
		 *	insert
		 */
			
			sprintf( pKey, key_format, i );
			sprintf( pData, data_format, i );

			if( rdb_set( td, (r_head_t*)rp, "KEY_DATA", pKey ) )
				goto err1;
			if( rdb_set( td, (r_head_t*)rp, "VALUE_DATA", pData ) )
				goto err1;
/***
			if( rdb_insert( td, 1, (r_head_t*)rp, 0, 0, 0 ) )
				goto err1;
***/
	}
			if( rdb_insert( td, k, (r_head_t*)rp_buf, 0, 0, 0 ) )
				goto err1;
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
	printf("%s\n", neo_errsym() );
	rdb_unlink( md );
	return( -1 );
}
