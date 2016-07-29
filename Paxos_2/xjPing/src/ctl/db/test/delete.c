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
#include	<inttypes.h>

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
	r_head_t	*rp;
	char		*pBufIns, *pRec;
	char		*pBuf;
	int			i;
	char		Name[64];
	int			n;
	char		search[64];

DBG_ENT(M_RDB,"neo_main");

	if( ac < 4 ) {
		printf("usage:test db_man TABLE abs_num\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "localhost", av[1] ) ))
		goto err1;

	rdb_drop( md, av[2] );
	/*
	 *	create table and insert record
	 */
	 bcopy( "NAME", items[0].i_name, 4 );
	 items[0].i_attr	= R_STR;
	 items[0].i_len		= 8;
	 items[0].i_size	= 8;
	 bcopy( "INT", items[1].i_name, 3 );
	 items[1].i_attr	= R_INT;
	 items[1].i_len		= sizeof(int);
	 items[1].i_size	= sizeof(int);
	
	if( !(td = rdb_create( md, av[2], REC_NUM, ITEM_NUM, items ) ) )
		goto err1;
	pBufIns	= malloc( td->t_rec_size*REC_NUM );
	for( i = 0, pRec = pBufIns; i < REC_NUM; i++, pRec += td->t_rec_size ) {
		sprintf( Name, "%d", i );
		rdb_set( td, (r_head_t*)pRec, "NAME", Name );
		rdb_set( td, (r_head_t*)pRec, "INT", (char*)&i );
	}
	rdb_insert( td, REC_NUM, (r_head_t*)pBufIns, 0, 0, 0 ); 
	
	pBuf	= malloc( td->t_rec_size*REC_NUM );

	n = rdb_find( td, REC_NUM, (r_head_t*)pBuf, R_SHARE );
for( i = 0, rp = (r_head_t*)pBuf; i < n;
				i++, rp = (r_head_t*)((char*)rp + td->t_rec_size) ) {
	printf("%d:rel=%"PRIi64" abs=%"PRIi64"\n", i, rp->r_rel, rp->r_abs );
}

	rp	= (r_head_t*)pBuf;
	if( rdb_delete( td, n, rp, R_WAIT ) )
		goto err2;
	n = rdb_find( td, REC_NUM, (r_head_t*)pBuf, R_SHARE );
	if( n ) {
		goto err2;
	}
	printf("=== Multi-delete is OK ===\n");
	/*
	 *	set search condition
	 */
	rdb_insert( td, REC_NUM, (r_head_t*)pBufIns, 0, 0, 0 ); 

	sprintf( search,"NAME=='%s'", av[3] ); 
	rdb_search( td, search );
	n = rdb_find( td, REC_NUM, (r_head_t*)pBuf, R_SHARE );

	rp	= (r_head_t*)pBuf;
	if( rdb_delete( td, 1, rp, R_WAIT ) )
		goto err2;

	printf("=== n=%d rel=%"PRIi64" abs=%"PRIi64" ===\n", 
			n, rp->r_rel, rp->r_abs );

	DBG_A(("abs=%d,neo_errno=0x%x\n", rp->r_abs, neo_errno ));

	rdb_close( td );

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );
err2:
	rdb_close( td );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	rdb_unlink( md );
	return( -1 );
}
