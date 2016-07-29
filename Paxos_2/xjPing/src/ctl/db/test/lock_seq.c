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

p_port_t	local_port, server_port;

#define	LOCK_TABLE	"XJ_LOCK"
#define	SEQUENCE_TABLE	"XJ_SEQUENCE"

#define	REC_NUM		10
char	buff[100];

char	*query = "select SEQ from %s where NAME='%s';";
char	*update = "update %s set SEQ = SEQ + 1 where NAME ='%s';commit;";

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	r_tbl_t	*td;
	r_head_t	*rp;
	int	i;
	r_tbl_t		*td_seq;
	long long	seq;
	int		len;
DBG_ENT(M_RDB,"neo_main");

	rp = (r_head_t *)buff;

	if( ac < 3 ) {
		printf("usage:sequence server sequencer\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, LOCK_TABLE ) ) )
		goto err1;
	/*
	 *	Lock --- search condition
	 */
	strcpy( buff, "NAME='");
	strcat( buff, SEQUENCE_TABLE );
	strcat( buff, "'" );
	if( rdb_search( td, buff ) )
		goto err1;

	printf("FIND(LOCK[%s])\n", buff );
	while( ( i = rdb_find( td, 1, rp, R_EXCLUSIVE )) > 0  ) {

		DBG_A(("ret=%d,rp->r_abs=%d,NAME=<%s>\n", 
					i, rp->r_abs, rp+1 ));

		print_record( 0, rp, td->t_scm->s_n, td->t_scm->s_item );
	}

	DBG_A(("i=%d,neo_errno=0x%x\n", i, neo_errno ));

SqlExecute( md, "rollback;" );

	/*
	 *	Query
	 */
	sprintf( buff, query, SEQUENCE_TABLE, av[2] );

	td_seq = SqlQuery( md, buff );	/* ResultSet */
	len = sizeof(seq);
	rdb_get( td_seq, td_seq->t_rec->r_head, "SEQ", (char*)&seq, &len );

	printf("query:SEQ=%lld\n", seq );

	SqlResultClose( td_seq );

	/*
	 *	Update
	 */
	sprintf( buff, update, SEQUENCE_TABLE, av[2] );
//	td_seq = sql_update( md, buff );
	td_seq = SqlUpdate( md, buff );
	len = sizeof(seq);
	rdb_get( td_seq, td_seq->t_rec->r_head, "SEQ", (char*)&seq, &len );

	printf("update:SEQ=%lld\n", seq );

	SqlResultClose( td_seq );
	/*
	 *	Unlock
	 */
	printf("UNLOCK\n");
	if( rdb_search( td, 0 ) )
		goto err1;

	printf("CLOSE\n");
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
