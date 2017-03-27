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

#define	DB_MAN_SYSTEM
#include	"neo_db.h"


r_usr_t	usr;

void print_usr( r_tbl_t*, int, r_usr_t* );

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	int		pos;
	char		*addrp;
	p_id_t		pid;
	_dlnk_t		*pep;
	r_port_t	pe;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
		printf("usage:dump_usr server TABLE\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	if( !(td = rdb_open( md, av[2] ) ) )
		goto err1;

	pos = 0;
	while( !rdb_dump_usr( td, pos, &usr, &addrp ) ) {

		print_usr( (r_tbl_t*)addrp, pos++, &usr );

		rdb_dump_mem( md, (char*)usr.u_pd, sizeof(pid), (char*)&pid );

		printf("remote port(%s/%d)\n", 
			inet_ntoa( pid.i_remote.p_addr.a_pa.sin_addr),
			ntohs( pid.i_remote.p_addr.a_pa.sin_port ) );
		printf("local port(%s/%d)\n", 
			inet_ntoa( pid.i_local.p_addr.a_pa.sin_addr),
			ntohs( pid.i_local.p_addr.a_pa.sin_port ) );
		if( (pep = &PDTOENT( &pid )->p_usr_ent) ) {
			if( rdb_dump_mem( md, (char*)pep, sizeof(pe), (char*)&pe ) < 0 )
                                        goto err2;
        printf("PORT ENTRY[%p]\n", pep );
        printf("twait		= %p\n",	pe.p_twait );
        printf("proc		= %s\n",	pe.p_clnt.u_procname );
        printf("port		= %s\n",	pe.p_clnt.u_portname );
        printf("host		= %s\n",	pe.p_clnt.u_hostname );
        printf("pid		= %d\n",	pe.p_clnt.u_pid );
        printf("port no		= 0x%x\n",	pe.p_clnt.u_portno );
        printf("tran. mode	= %d\n",	pe.p_trn );
		}
	}

	if( rdb_close( td ) )
		goto err1;

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );

err2:	rdb_close( td );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	printf("%s\n", neo_errsym() );
	rdb_unlink( md );
	return( -1 );
}

void
print_usr( td, pos, ud )
	r_tbl_t	*td;
	int	pos;
	r_usr_t	*ud;
{
	printf("\nOPEN USER-%d(%p)\n", pos, td );

	printf("stat		= 0x%x\n",	ud->u_stat );
	printf("port		= %p\n",	ud->u_pd );
	printf("my td		= %p\n", 	ud->u_mytd );
	printf("remote td	= %p\n", 	ud->u_td );
	printf("cursor		= %d\n", 	ud->u_cursor );
	printf("mode		= 0x%x\n", 	ud->u_mode );
	printf("index 		= %d\n", 	ud->u_index );
	printf("added 		= %d\n", 	ud->u_index );
	printf("insert 		= %d\n", 	ud->u_insert );
	printf("update 		= %d\n", 	ud->u_update );
	printf("delete 		= %d\n", 	ud->u_delete );
}
