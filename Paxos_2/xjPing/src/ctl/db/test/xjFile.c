/*******************************************************************************
 * 
 *  xjFile.c --- xjPing (On Memory DB) Test programs
 * 
 *  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
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

#include	<fcntl.h>
#include	<time.h>

#include	"neo_db.h"

char	*attr[]	= {
	"",
	"HEX",
	"INT",
	"UINT",
	"FLOAT",
	"STRING",
	"LONG",
	"ULONG"
};

char	statement[2096];

int
main( ac, av )
	int	ac;
	char	*av[];
{
	int		fd;
	int		type;
	long		modified;
	int		rec_num;
	int		rec_used;
	int		rec_size;
	int		item_num;
	int		i;
	r_item_t	*items;
	int		depend_num;
	r_tbl_mod_t	*depends;
	r_head_t	*rp;
	int		n;
	int		flg;
	char		*cp;

	if( ac < 2 ) {
		printf("usage:file filePath [h]\n");
		exit( 1 );
	}
	if( ac >= 3 )	flg = 0;
	else		flg = 1;

	if( (cp = (char*)strrchr( av[1], '.' )) ) {
		if( !strcmp( cp, ".scm" ) ) {

			if( ( fd = open( av[1], O_RDONLY ) ) < 0 ) {
				printf("open error[%s]\n", av[1] );
				exit( 1 );
			}
			if( read( fd, &n, sizeof(n) ) < 0 ) {
				printf("read error(n)\n");
				exit( 1 );
			}
			if( read( fd, statement, n ) < 0 ) {
				printf("read error(statement)\n");
				exit( 1 );
			}
			close( fd );
			printf("[%s]\n", statement );
			exit( 0 );
		}
	}
	if( ( fd = open( av[1], O_RDONLY ) ) < 0 ) {
		printf("open error[%s]\n", av[1] );
		exit( 1 );
	}

	if( read( fd, &type, sizeof(type) ) < 0 ) {
		printf("read error(type)\n");
		exit( 1 );
	}
	if( read( fd, &modified, sizeof(modified) ) < 0 ) {
		printf("read error(modified)\n");
		exit( 1 );
	}
	if( read( fd, &rec_num, sizeof(rec_num) ) < 0 ) {
		printf("read error(rec_num)\n");
		exit( 1 );
	}
	if( read( fd, &rec_used, sizeof(rec_used) ) < 0 ) {
		printf("read error(rec_used)\n");
		exit( 1 );
	}
	if( read( fd, &rec_size, sizeof(rec_size) ) < 0 ) {
		printf("read error(rec_size)\n");
		exit( 1 );
	}
	if( read( fd, &item_num, sizeof(item_num) ) < 0 ) {
		printf("read error(item_num)\n");
		exit( 1 );
	}
	if( !(items = (r_item_t *)malloc( item_num*sizeof(r_item_t) ) ) )  {
		printf("malloc error(items)\n");
		exit( 1 );
	}
	if( read( fd, items, item_num*sizeof(r_item_t) ) < 0 ) {
		printf("read error(items)\n");
		exit( 1 );
	}
	if( read( fd, &depend_num, sizeof(depend_num) ) < 0 ) {
		printf("read error(depend_num)\n");
		exit( 1 );
	}
	if( depend_num > 0 ) {
		if( !(depends = (r_tbl_mod_t *)malloc( 
			depend_num*sizeof(r_tbl_mod_t) ) ) )  {
			printf("malloc error(depends)\n");
			exit( 1 );
		}
		if( read( fd, depends, 
				depend_num*sizeof(r_tbl_mod_t) ) < 0 ) {
			printf("read error(depends)\n");
			exit( 1 );
		}
	}

	printf("TYPE                   	= %d\n", type );
	printf("LAST MODIFIED TIME     	= %s", ctime(&modified) );
	printf("NUMBER OF TOTAL RECORDS	= %d\n", rec_num );
	printf("NUMBER OF USED RECORDS	= %d\n", rec_used );
	printf("SIZE OF RECORD		= %d\n", rec_size );
	printf("NUMBER OF ITEMS		= %d\n", item_num );
	for ( i = 0; i < item_num; i++ ) {
		printf("%3d.[%-32s](%s) \tlen(%3d) size(%3d) off(%3d)\n",
			i, items[i].i_name, attr[items[i].i_attr],
			items[i].i_len, items[i].i_size, items[i].i_off );
	}
	for( i = 0; i < depend_num; i++ ) {
		printf("%3d.[%s] %s", 
		i, depends[i].m_name, ctime( &depends[i].m_time) );
	}
	if( !(rp = (r_head_t *)malloc( rec_size ) ) ) {
		printf("malloc error(rp)\n");
		exit( 1 );
	}
	if( flg ) {
		for( i = 0; i < rec_num; i++ ) {
			if( (n = read( fd, rp, rec_size ) ) < 0 ) {
				printf("read error(record)\n");
				exit( 1 );
			}
			if( n == 0 )
				break;
			print_record( i, rp, item_num, items );
		}
		if( read( fd, rp, rec_size ) != 0 ) {
			printf("There are some data !!!\n");
		}
	}

	close( fd );
	return( 0 );
}

