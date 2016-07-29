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

#define	REC_NUM		5
#define	ITEM_NUM	2

r_item_t		items[ITEM_NUM];
r_item_name_t	check[] = {"STR","INT"};

#define	TABLE	"DUP_CHECK"

char*
AllocRec( r_tbl_t *pTbl )
{
	char	*pBuf, *pRec;
	char	buf[16];
	int		i;

	pBuf = neo_zalloc( pTbl->t_rec_size*REC_NUM );

	for( i = 0, pRec = pBuf; i < REC_NUM; i++, pRec += pTbl->t_rec_size ) {
		memset( buf, 0, sizeof(buf) );
		sprintf( buf, "%d", i );
		rdb_set( pTbl, (r_head_t*)pRec, "STR", buf );
		rdb_set( pTbl, (r_head_t*)pRec, "INT", (char*)&i );
	}
	return( pBuf );
}

int
table( r_man_t *pMan, bool_t bCommit )
{
	r_tbl_t	*pTbl;
	char	*pBuf;
	int		Ret;

	rdb_drop( pMan, TABLE );

	strcpy( items[0].i_name, "STR" );
	items[0].i_attr	= R_STR;
	items[0].i_size	= 16;
	items[0].i_len	= 16;

	strcpy( items[1].i_name, "INT" );
	items[1].i_attr	= R_INT;
	items[1].i_size	= 4;
	items[1].i_len	= 4;

	if( !(pTbl = rdb_create( pMan, TABLE, REC_NUM, ITEM_NUM, items ) ) ) {
			goto err1;
	}
	pBuf	= AllocRec( pTbl );

	Ret = rdb_insert( pTbl, REC_NUM, (r_head_t*)pBuf, 0, 0, R_EXCLUSIVE );

	neo_free( pBuf );

	rdb_close_client( pTbl );

	if( bCommit )	rdb_commit( pMan );
	else			rdb_rollback( pMan );

	return( 0 );
err1:
	return( -1 );
}

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;

	if( ac < 4 ) {
		printf("usage:test db_man db_local {commit|rollback}\n");
		return( 1 );
	}
	printf("db_man=[%s],my_name=[%s],cmd=[%s]\n", av[1], av[2], av[3] );

	if( !(md = rdb_link( av[2]/*"CLIENT"*/, av[1] ) ))
		goto err1;

	if( !strcmp( "commit", av[3] ) ) {
		table( md, TRUE );
	} else {
		table( md, FALSE );
	}
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
