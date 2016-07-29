/*******************************************************************************
 * 
 *  xjTable.c --- xjPing (On Memory DB)  Test programs
 * 
 *  Copyright (C) 2011,2014-2016 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

#include	<time.h>
#include	"neo_db.h"

char	*attr[] = {
	"",
	"HEX",
	"INT",
	"UINT",
	"FLOAT",
	"STRING",
	"LONG",
	"ULONG"
};

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	r_tbl_t		tbl;
	r_head_t	*rp;
	int		i;
	int		type;
	long		modified;
	int		rec_size, rec_num, item_num;
	int		rec_used;
	r_item_t	*items;
	r_rec_t		rec;
	int		start, end;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
printf("usage:table server TABLE [start_rec end_rec]\n");
		neo_exit( 1 );
	}
	if( ac >= 4 )
		start = atoi( av[3] );
	else
		start = 0;
	if( ac >= 5 )
		end = atoi( av[4] );
	else
		end = 0;

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	/*
	 *	table open
	 */
	if( !(td = rdb_open( md, av[2] ) ) ) {
		printf("%s is not found.\n", av[2] );
		goto err1;
	}

	if( rdb_dump_tbl( td, &tbl ) )
		goto err1;
	/*
	 *	parameters
	 */
	type		= tbl.t_type;
	modified	= tbl.t_modified;
	rec_size	= td->t_rec_size;
	rec_num		= td->t_rec_num;
	rec_used	= td->t_rec_used;
	item_num	= td->t_scm->s_n;
	items		= td->t_scm->s_item;

	printf("TABLE[%s]\n\n", av[2] );
	printf("TYPE             	   = %d\n", type );
	printf("STAT                       = 0x%x", tbl.t_stat );
                if( tbl.t_stat & R_INSERT )     printf(" INSERT");
                if( tbl.t_stat & R_UPDATE )     printf(" UPDATE");
                if( tbl.t_stat & R_USED )       printf(" USED");
                if( tbl.t_stat & R_BUSY )       printf(" BUSY");
                if( tbl.t_stat & R_EXCLUSIVE )  printf(" EXCLUSIVE");
                if( tbl.t_stat & R_SHARE )      printf(" SHARE");
                if( tbl.t_stat & T_TMP )        printf(" TMP");
		printf("\n");
	printf("MODIFIED TIME    	   = %s", ctime(&modified) );
	printf("NUMBER OF RECORDS	   = %d\n", rec_num );
	printf("NUMBER OF ORI USED RECORDS = %d\n", tbl.t_rec_ori_used );
	printf("NUMBER OF USED RECORDS	   = %d\n", rec_used );
	printf("SIZE OF RECORD		   = %d\n", rec_size );
	printf("NUMBER OF ITEMS		   = %d\n\n", item_num );

	for ( i = 0; i < item_num; i++ ) {
		printf("%3d.[%-32s](%c) len(%3d) size(%3d) off(%3d)\n",
			i, items[i].i_name, attr[items[i].i_attr][0],
			items[i].i_len, items[i].i_size, items[i].i_off );
	}
	printf("\n");
	/*
	 *	allocate buffer
	 */
	if( !(rp = (r_head_t *)neo_malloc( rec_size ) ) )
		goto err1;
	/*
	 *	dump
	 */
	if( start <= 0 || rec_num <= start )
		start = 0;
	if( end <= 0 || rec_num <= end )
		end = rec_num - 1;

	for( i = start; i <= end; i++ ) {

		if( rdb_dump_record( td, i, &rec, (char*)rp ) )
			goto err2;
	
		uint64_t	Cksum64;

		if( rp->r_Cksum64 ) {
			Cksum64	= in_cksum64( rp, rec_size, 0 );
			if( Cksum64 ) {
				printf("CKSUM ERROR\n");
			}
		}
		print_record_with_cntl( i, &rec, rp, item_num, items );
	}
	/*
	 *	close
	 */
	if( rdb_close( td ) )
		goto err2;

	rdb_unlink( md );

	neo_free( rp );
DBG_EXIT(0);
	return( 0 );

err2:
	neo_free( rp );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	rdb_unlink( md );
	return( -1 );
}

