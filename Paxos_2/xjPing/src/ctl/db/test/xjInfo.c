/*******************************************************************************
 * 
 *  xjInfo.c --- xjPing (On Memory DB)  Test programs
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

#include	<time.h>
#include	"neo_db.h"
#include	"../svc/hash.h"


r_inf_t	inf;
r_tbl_t	tbl;

void print_pd( p_id_t*, p_id_t* );
void print_pe( _dlnk_t*, r_port_t*, r_man_t* );
int	 print_wait( r_man_t*, r_twait_t* );
void print_ud( r_usr_t*, r_usr_t* );
void print_tbl( r_tbl_t*, r_tbl_t* );
void print_depend( r_tbl_list_t* );
int hash_dump( r_man_t *md, h_head_t *pHead, char *indent );

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t	*md;
	_dlnk_t	*tdp;
	r_res_dump_inf_t	res;
	r_inf_t		inf;
	int		i;
	int		l_n, len;
	r_tbl_list_t	*listp;
	_dlnk_t		dlnk;
	p_id_t		pd;
	_dlnk_t		*pep;
	r_port_t	pe;
	_dlnk_t		ulnk;
	r_usr_t		ud;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 2 ) {
		printf("usage:info server\n");
		neo_exit( 1 );
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err;

	if( rdb_dump_lock( md ) < 0 )
		goto err0;

	if( rdb_dump_inf( md, &res ) < 0 )
		goto err1;
	inf = res.i_inf;
	
	printf( "Server stat is %s\n", 
		(inf.i_svc_inf.f_stat ? "R_SERVER_STOP" : "R_SERVER_ACTIVE") );
	printf( "Database path is [%s]\n", inf.i_svc_inf.f_path );

	/* Heap */
	NHeapPool_t	Pool;

	if( rdb_dump_mem( md, (char*)inf.i_svc_inf.f_pPool, 
							sizeof(Pool), (char*)&Pool ) < 0 ) {
		goto err1;
	}
	printf("NHeapPool:size=%zu use=%zu max=%zu\n", 
				Pool.p_Size, Pool.p_Use, Pool.p_Max );	
	printf("\n");

	/* port */
	if( rdb_dump_mem( md, (char*)inf.i_svc_inf.f_port, sizeof(dlnk), (char*)&dlnk ) < 0 )
				goto err1;
	if( dlnk.q_next != inf.i_svc_inf.f_port ) {
		do {
			if( rdb_dump_mem(md, (char*)dlnk.q_next, sizeof(pd), (char*)&pd)< 0 )
				goto err1;

			print_pd( (p_id_t*)dlnk.q_next, &pd );

			/* port entry and open user */
			if( (pep = &PDTOENT( &pd )->p_usr_ent) ) {
				if( rdb_dump_mem( md, (char*)pep,
						sizeof(pe), (char*)&pe ) < 0 )
					goto err1;

				print_pe( pep, &pe, md );

				bcopy( &pe.p_usr_ent, &ulnk, sizeof(ulnk) );

				if( pep != ulnk.q_next ) {
					do {
						if( rdb_dump_mem( md,
							(char*)PDTOUSR(ulnk.q_next),
							sizeof(ud), (char*)&ud ) < 0 )
								goto err1;

						print_ud( PDTOUSR(ulnk.q_next),
								 &ud );

						bcopy( &ud.u_port, &ulnk, 
								sizeof(ulnk) );
					} while( pep != ulnk.q_next );
				}
			}

			printf("\n");

			bcopy( &pd.i_lnk, &dlnk, sizeof(dlnk) );
		} while( inf.i_svc_inf.f_port != dlnk.q_next );
	}

	/* table */
	if( inf.i_svc_inf.f_opn_tbl.q_next != (_dlnk_t *)inf.i_root ) {

		bcopy( &inf.i_svc_inf.f_opn_tbl, &tbl, sizeof(_dlnk_t) );
		do {
			tdp = tbl.t_lnk.q_next;

			if( rdb_dump_mem( md, (char*)tdp, sizeof(tbl), (char*)&tbl ) < 0 )
				goto err1;

			print_tbl( (r_tbl_t*)tdp, &tbl );

			if( tbl.t_depend ) {
				if( rdb_dump_mem( md, (char*)tbl.t_depend,
						sizeof(int), (char*)&l_n ) < 0 )
					goto err1;
				len = sizeof(r_tbl_list_t) + 
					sizeof(r_tbl_mod_t)*(l_n-1);
				if(!(listp = (r_tbl_list_t*)neo_zalloc(len))<0){
					goto err1;
				}
				if( rdb_dump_mem( md, (char*)tbl.t_depend,
							len, (char*)listp ) < 0 ) {
					neo_free( listp );
					goto err1;
				}
				print_depend( listp );
				neo_free( listp );
			}
			/* HashEqual dump */
			list_t	*h_aHeads;

			printf("=== HashEqual ===\n" );
			len	= sizeof(list_t)*tbl.t_HashEqual.h_N;
			h_aHeads = (list_t*)neo_malloc( len );
			if( rdb_dump_mem( md, (char*)tbl.t_HashEqual.h_aHeads,
							len, (char*)h_aHeads ) < 0 ) {
				neo_free( h_aHeads );
				goto err1;
			}
			HashItem_t	HashItem, *pHashItem;
			h_head_t	Head, *pHead;
			int			Size;
			for( i = 0; i < tbl.t_HashEqual.h_N; i++ ) {
				LIST_FOR_EACH_ENTRY( pHashItem, &h_aHeads[i], i_Link,
										h_aHeads, tbl.t_HashEqual.h_aHeads ) {
					if( rdb_dump_mem( md, (char*)pHashItem,
							sizeof(HashItem), (char*)&HashItem ) < 0 ) {
						neo_free( h_aHeads );
						goto err1;
					}
					if( rdb_dump_mem( md, (char*)HashItem.i_pValue,
							sizeof(Head), (char*)&Head ) < 0 ) {
						neo_free( h_aHeads );
						goto err1;
					}
					Size = sizeof(*pHead) + sizeof(h_entry_t)*Head.h_hash_size;
					pHead = (h_head_t*)neo_malloc( Size );
					if( rdb_dump_mem( md, (char*)HashItem.i_pValue,
							Size, (char*)pHead ) < 0 ) {
						neo_free( pHead );
						goto err1;
					}
					hash_dump( md, pHead, "" );
					pHashItem	= &HashItem;
				}
			}

			printf("\n");
			
		} while( inf.i_root != (char *)tbl.t_lnk.q_next );
	} else {
		printf("No table is opened\n");
	}

	rdb_dump_unlock( md );

	rdb_unlink( md );
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
    printf("err1: neo_errno=0x%x\n",neo_errno);
	rdb_dump_unlock( md );
err0:
	rdb_unlink( md );
err:
	return( -1 );
}

int
hash_dump( r_man_t *md, h_head_t *pHead, char *indent )
{
	int	i;
	h_elm_t	*array;
	int	n;
	int	j;
	h_head_t	Head, *pHead1, *pHead2;
	int	Size;

	printf("%s[%s] cnt=%d acive=%d max_len=%d average_len=%f\n", indent, 
			pHead->h_name, pHead->h_cnt, pHead->h_active, pHead->h_max,
				(float)pHead->h_cnt/pHead->h_active );

//	if( pHead->h_cnt > 100 ) goto ret;

	for( i = 0; i < pHead->h_hash_size; i++ ) {
//if( pHead->h_cnt < 100 ) {
		if( (n = pHead->h_value[i].ent_cnt) > 0 ) {
			printf("%s%d %d %d:", indent, i, n, pHead->h_value[i].ent_size );
			if( pHead->h_value[i].ent_array ) {
				array	= (h_elm_t*)neo_malloc( sizeof(h_elm_t)*n );
				if( rdb_dump_mem( md, (char*)pHead->h_value[i].ent_array,
							sizeof(h_elm_t)*n, (char*)array ) < 0 ) {
					neo_free( array );
					goto err1;
				}
				for( j = 0; j < n; j++ ) {
					printf(" %d", array[j].e_ind );
				}
				neo_free( array );
			}
			printf("\n");
		}
//}
		if( (pHead1 = pHead->h_value[i].ent_hash ) ) {
			if( rdb_dump_mem( md, (char*)pHead1, 
						sizeof(Head), (char*)&Head ) < 0 ) {
				goto err1;
			}
			Size = sizeof(*pHead2) + sizeof(h_entry_t)*(Head.h_hash_size-1);
			pHead2= (h_head_t*)neo_malloc( Size );
			if( rdb_dump_mem( md, (char*)pHead1, Size, (char*)pHead2 ) < 0 ) {
				goto err1;
			}
			hash_dump( md, pHead2, "  " );
		}
	}
//ret:
	return( 0 );
err1:
	return( -1 );
}

void
print_pd( pdp, pd )
	p_id_t	*pdp;
	p_id_t	*pd;
{
	printf("PORT[%p]\n", pdp );
	printf("i_id	= 0x%lx\n",	pd->i_id );
	printf("i_stat	= 0x%lx\n",	pd->i_stat );
	printf("i_err	= %ld\n",	pd->i_err );
	printf("i_busy	= %d\n",	pd->i_busy );
	printf("i_sfd	= %d\n",	pd->i_sfd );
	printf("i_rfd	= %d\n",	pd->i_rfd );
	printf("i_cfd	= %d\n",	pd->i_cfd );
	printf("remote port(%s/%d)\n",
		inet_ntoa( pd->i_remote.p_addr.a_pa.sin_addr),
		ntohs( pd->i_remote.p_addr.a_pa.sin_port ) );
	printf("local port(%s/%d)\n",
		inet_ntoa( pd->i_local.p_addr.a_pa.sin_addr),
		ntohs( pd->i_local.p_addr.a_pa.sin_port ) );
}

void
print_pe( pep, pe, md )
	_dlnk_t *pep;
	r_port_t *pe;
	r_man_t	*md;
{
	printf("\tPORT ENTRY[%p]\n", pep );
	printf("\ttwait(%p)\n",		pe->p_twait );
		if( pe->p_twait ) print_wait( md, pe->p_twait );
	printf("\tproc[%s]\n",			pe->p_clnt.u_procname );
	printf("\tport[%s]\n", 			pe->p_clnt.u_portname );
	printf("\thost[%s]\n", 			pe->p_clnt.u_hostname );
	printf("\tpid(%d)\n",			pe->p_clnt.u_pid );
	printf("\tport no(0x%x)\n",		pe->p_clnt.u_portno );
	printf("\ttransaction mode(%d)\n",	pe->p_trn );
}

int
print_wait( md, twaitp )
	r_man_t		*md;
	r_twait_t	*twaitp;
{
	r_twait_t	wait;
	_dlnk_t		dlnk;
	r_tbl_t		t;
	r_usr_t		u;

	if( rdb_dump_mem( md, (char*)twaitp, sizeof(wait), (char*)&wait ) < 0 )
			goto err1;
	/* wait a table */
	if( rdb_dump_mem( md, (char*)wait.w_td, sizeof(t), (char*)&t ) < 0 )
			goto err1;
	printf("\t\tTABLE[%s(%p)] USER", t.t_name, wait.w_td );

	/* wait users */
	dlnk = wait.w_usr;
	while( &(twaitp->w_usr) != dlnk.q_next ) {
		if( rdb_dump_mem( md, (char*)TWTOUSR(dlnk.q_next), 
						sizeof(u), (char*)&u ) < 0 )
			goto err1;
		printf("[%p] ", dlnk.q_next );
		dlnk = u.u_twait;
	}
	printf("\n");

	return( 0 );
err1:
	return( -1 );
}

void
print_ud( udp, ud )
	r_usr_t	*udp;
	r_usr_t	*ud;
{
	printf("\t\tUSR[%p]\n", udp );
	printf("\t\t\tstat(0x%x)\n", 		ud->u_stat );
	printf("\t\t\tport(%p)\n", 		ud->u_pd );
	printf("\t\t\tmy td(%p)\n", 		ud->u_mytd );
	printf("\t\t\tremote td(%p)\n",	ud->u_td );
	printf("\t\t\tsearch[%s]\n",		ud->u_search.s_text );
	printf("\t\t\tcursor[%d]\n",		ud->u_cursor );
	printf("\t\t\texclusive mode[0x%x]\n",	ud->u_mode );
	printf("\t\t\twait index[%d]\n",	ud->u_index );
	printf("\t\t\tadded records[%d]\n",	ud->u_added );
	printf("\t\t\tinsert records[%d]\n",	ud->u_insert );
	printf("\t\t\tupdate records[%d]\n",	ud->u_update );
	printf("\t\t\tdelete records[%d]\n",	ud->u_delete );
}

void
print_tbl( tdp, td )
	r_tbl_t	*tdp;
	r_tbl_t	*td;
{
	printf("TABLE NAME[%s(%p)]\n", td->t_name, tdp );

	printf("type		= 0x%x\n", 	td->t_type );
	printf("stat		= 0x%x\t",	td->t_stat );
		if( td->t_stat & R_INSERT )	printf(" INSERT");
		if( td->t_stat & R_UPDATE )	printf(" UPDATE");
		if( td->t_stat & R_USED )	printf(" USED");
		if( td->t_stat & R_BUSY )	printf(" BUSY");
		if( td->t_stat & R_EXCLUSIVE )	printf(" EXCLUSIVE");
		if( td->t_stat & R_SHARE )	printf(" SHARE");
		if( td->t_stat & T_TMP )	printf(" TMP");
	printf("\nuser count	= %d\n", 	td->t_usr_cnt );
	printf("lock user	= %p\n", 	td->t_lock_usr );
	printf("wait count	= %d\n", 	td->t_wait_cnt );
	printf("record size	= %d\n", 	td->t_rec_size );
	printf("total records	= %d\n", 	td->t_rec_num );
	printf("used records	= %d\n", 	td->t_rec_used );
	printf("in file		= %d\n", 	td->t_rec_ori_used );
	printf("tag		= %p\n", 	td->t_tag );
	printf("modified time	= %s", 		ctime(&td->t_modified) );
	printf("access time	= %s", 		ctime(&td->t_access) );
}

void
print_depend( listp )
	r_tbl_list_t	*listp;
{
	int	i;

	for( i = 0; i < listp->l_n; i++ ) {
		printf("\t[%s]\t%s", 
			listp->l_tbl[i].m_name,
			ctime(&listp->l_tbl[i].m_time) );
	}
}
