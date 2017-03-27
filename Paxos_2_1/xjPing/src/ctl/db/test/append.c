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

r_item_t	items[ITEM_NUM];
char		buff[100];

neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	r_head_t	*rp;
	int		i, n;
	char		text[50];
	float		f;
	int		start, end;
	long		st, en;

DBG_ENT(M_RDB,"neo_main");

	rp = (r_head_t *)buff;

	if( ac < 6 ) {
		printf("usage:append db_man db_local TABLE start end\n");
		neo_exit( 1 );
	}
	start	= atoi( av[4] );
	end	= atoi( av[5] );

	if( start > end || end > 100000 )
		neo_exit( 1 );

	if( !(md = rdb_link( av[2], av[1] ) ))
		goto err1;

	/*
	 *	table open
	 */
	if( !(td = rdb_open( md, av[3] ) ) )
		goto err1;
	
	st = time(0);
	/*
	 *	insert
	 */
	for( i = start; i <= end; i++ ) {

		sprintf( text, "%d", i );
		if( rdb_set( td, rp, "NAME", text ) )
			goto err1;
		if( rdb_set( td, rp, "HEX", &i ) )
			goto err1;
		if( rdb_set( td, rp, "INT", &i ) )
			goto err1;
		if( rdb_set( td, rp, "UINT", &i ) )
			goto err1;
		f = i;
		if( rdb_set( td, rp, "FLOAT", &f ) )
			goto err1;

		if( rdb_insert( td, 1, rp, 0, 0, 0 ) )
			goto err1;
DBG_A(("rec(rel=%d,abs=%d,stat=0x%x)\n", rp->r_rel, rp->r_abs, rp->r_stat ));
	}
	en = time(0);

	printf("time=%ds(%dms)\n", en-st, (en-st)*1000/(end-start+1) );

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
