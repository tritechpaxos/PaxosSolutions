/*******************************************************************************
 * 
 *  xjSql.c --- xjPing (On Memory DB)  Test programs
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

#define	DB_MAN_SYSTEM

#include	<stdio.h>
#include	<string.h>
#include	<time.h>

#include	"neo_db.h"
#include	"../sql/sql.h"

r_man_t	*md;
char	buff[4098];
char	statement[4098];

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_tbl_t		*td;
	int		i;
	char		*rp;
	long		s, e;

DBG_ENT(M_SQL,"neo_main");

	if( ac < 2 ) {
		printf("usage:sql dbServer {<|'sql_statemen'}\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( md->r_ident != R_MAN_IDENT ) {
		goto err1;
	}

	while( getsqllines( statement, sizeof(statement) ) ) {

		DBG_B(("INPUT:%s\n", statement ));
		if( statement[0] == '#' )
			continue;
		if( !strncmp( statement, "quit;" , 5 ) ) {
			break;
		}
		s = time( 0 );

		if( SqlExecute( md, statement ) )
			goto err2;

		e = time( 0 );

		if( neo_errno )
			printf("->%s\n", neo_errsym());

		while( SqlResultFirstOpen( md, &td ) == 0 && td ) {
			for( i = 0; i < td->t_scm->s_n; i++ ) {
				DBG_B(("[%s(%s,%d)]",
					td->t_scm->s_item[i].i_name,
					td->t_scm->s_item[i].i_type,
					td->t_scm->s_item[i].i_len ));
			}
			DBG_B(("\n"));
			for( i = 0, rp = (char*)td->t_rec; 
				i < td->t_rec_num; i++, rp += td->t_rec_size ) {
				print_record( i, (r_head_t*)rp, 
					td->t_scm->s_n, td->t_scm->s_item );
			}
			DBG_B(("Total=%d\n", td->t_rec_num ));
			SqlResultClose( td );
		}	
DBG_B(("START:%s",ctime(&s)));
DBG_B(("END  :%s",ctime(&e)));
	}
	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );

err2:	rdb_unlink( md );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return( -1 );
}

