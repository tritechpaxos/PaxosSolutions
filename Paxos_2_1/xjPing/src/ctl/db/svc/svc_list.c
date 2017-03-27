/*******************************************************************************
 * 
 *  svc_list.c --- xjPing (On Memory DB) Server AP programs
 * 
 *  Copyright (C) 2011,2015-2016 triTech Inc. All Rights Reserved.
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

#include	<dirent.h>
#include	<fcntl.h>

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"../dbif/TBL_RDB.h"

static int match();

int
svc_list( pd, hp )
	p_id_t		*pd;		/* 通信ポート記述子	*/
	p_head_t	*hp;		/* 要求パケットヘッダ	*/
{
	r_req_list_t	req;
	r_res_list_t	*resp;
	int		len;

//	DIR		*dir;
	struct dirent	*ent;
	int		cnt;
	char		*cp;
	int		n;
//	int		fd;
	struct dirent	*pEnt;
	int		i;

DBG_ENT(M_RDB,"svc_list");

	/*
	 * リクエストレシーブ
	 */
	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) < 0 ) {
		neo_proc_err( M_RDB, svc_myname, RDBCOMERR01, neo_errsym() );
		goto err1;
	}

/***
	dir = f_opendir();
	cnt = 0;
	while( (ent = f_readdir( dir )) ) {
		cnt++;
	}
	f_rewinddir( dir );
***/
	n	= FileCacheReadDir( pBC, ".", NULL, 0, 0 );
	if( n <= 0 ) {
		err_reply( pd, hp, E_RDB_NOEXIST );
		goto err1;
	}

	len = sizeof(r_res_list_t) + sizeof(r_res_file_t)*(n-1);
	if( !(resp = (r_res_list_t*)neo_zalloc( len ) ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
/***
		goto err2;
***/
		goto err1;
	} 
	if( !(pEnt = (struct dirent*)neo_zalloc(sizeof(struct dirent)*n) ) ) {
		err_reply( pd, hp, E_RDB_NOMEM );
		goto err3;
	} 
	FileCacheReadDir( pBC, ".", pEnt, 0, n );

	cnt = 0;
/***
	while( (ent = f_readdir( dir )) ) {
***/
	for( i = 0; i < n; i++ ) {

		ent	= &pEnt[i];

		if( match( ent ) )	continue;

/***
		f_stat( ent->d_name, &resp->l_file[cnt].f_stat );
***/
		FileCacheStat( pBC, ent->d_name, &resp->l_file[cnt].f_stat );

		strcpy( resp->l_file[cnt].f_name, ent->d_name );
		if( (cp = (char*)strrchr( ent->d_name, '.' )) ) {
			if( !strcmp( cp, ".scm" ) ) {

#ifdef	ZZZ
				if( (fd = f_open( ent->d_name/*, O_RDONLY*/ ))>=0 ) {
					if( read( fd, &n, sizeof(n) ) < 0 ) {
						goto errscm;
					}
					if( n >= 
					  sizeof(resp->l_file[cnt].f_stmt)-1 ) { 
					  n=sizeof(resp->l_file[cnt].f_stmt)-1; 
					}
					read( fd, resp->l_file[cnt].f_stmt, n );
				errscm:
					close( fd );
				}
#else
				FileCacheRead( pBC, ent->d_name, 0, sizeof(n), (char*)&n );
				if( n >= sizeof(resp->l_file[cnt].f_stmt)-1 ) { 
					  n=sizeof(resp->l_file[cnt].f_stmt)-1; 
				}
				if( n > 0 ) {
					FileCacheRead( pBC, ent->d_name, sizeof(n), 
									n, (char*)resp->l_file[cnt].f_stmt );
				}
#endif
			}
		}
		cnt++;
	}
	resp->l_n = cnt;

	/*
	 * リプライ
	 */
	len = sizeof(r_res_list_t) + sizeof(r_res_file_t)*(cnt-1);
	reply_head( (p_head_t*)&req, (p_head_t*)resp, len, 0 );

	if( p_reply( pd, (p_head_t*)resp, len ) < 0 ) {
		goto err3;
	}

	neo_free( pEnt );
	neo_free( resp );

//	f_closedir( dir );

DBG_EXIT(0);
	return( 0 );

//err4:	neo_free( pEnt );
err3:	neo_free( resp );
/***
err2:	f_closedir( dir );
***/
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

typedef struct {
	char	*e_name;
} exclude_t;

static exclude_t exclude[] = {
	{"."},
	{".."},
	{0} };

static int match( ent )
	struct dirent *ent;
{
	exclude_t	*p;

DBG_ENT(M_RDB,"match");

	for( p = exclude; p->e_name; p++ ) {
		if( !(strcmp( p->e_name, ent->d_name ) ) ) {
			DBG_EXIT( 1 );
			return( 1 );
		}
	}
	DBG_EXIT( 0 );
	return( 0 );
}
