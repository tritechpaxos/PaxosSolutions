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
#include	"svc_logmsg.h"

int
svc_dump_proc( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int		len;
	p_head_t	req;
	p_head_t	res;

DBG_ENT(M_RDB,"svc_dump_proc");

	len = sizeof(req);
	if( p_recv( pd, &req, &len ) )
		goto err1;

	/*
	 *	reply
	 */
	reply_head( &req, &res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) )
		goto err1;
	
DBG_EXIT(0);
	return( 0 );

err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_inf( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	r_req_dump_inf_t	req;
	r_res_dump_inf_t	res;

DBG_ENT(M_RDB,"svc_dump_inf");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 *	refer
	 */
	res.i_inf.i_root = (char *)&(_svc_inf);
	bcopy( &_svc_inf, &res.i_inf.i_svc_inf, sizeof(_svc_inf) );

	res.i_mem = 0;

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) )
		goto err1;
	
DBG_EXIT(0);
	return( 0 );

err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_record( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	r_req_dump_record_t	req;
	r_res_dump_record_t	*resp;
	r_tbl_t			*td;
	r_usr_t			*ud;
	int			abs;

DBG_ENT(M_RDB,"svc_dump_record");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 *	check data
	 */
	r_port_t	*pe = PDTOENT(pd);

	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.r_name);
	ud = (r_usr_t*)HashGet( &pe->p_HashUser, req.r_name );
	abs	= req.r_abs;

	if( (neo_errno = check_td_ud( td, ud )) ) {

		err_reply( pd, hp, neo_errno );

		goto err1;
	}
	if( abs < 0 || td->t_rec_num <= abs ) {

		err_reply( pd, hp, E_RDB_INVAL );

		goto err1;
	}
	/*
	 *	allocate reply buffer
	 */
	len = sizeof(r_res_dump_record_t) + td->t_rec_size;
	if( !(resp = (r_res_dump_record_t *)neo_malloc( len )) ) {
		
		err_reply( pd, hp, E_RDB_NOMEM );

		goto err1;
	}
	/*
	 *	refer
	 */
	if( !(td->t_rec[abs].r_stat & R_REC_CACHE) )	/* load */
			svc_load( td, abs, R_PAGE_SIZE/td->t_rec_size+1 );
		
	bcopy( &td->t_rec[abs], &resp->r_rec, sizeof(td->t_rec[abs]) );
	bcopy( td->t_rec[abs].r_head, resp+1, td->t_rec_size );

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)resp, len, 0 );

	if( p_reply( pd, (p_head_t*)resp, len ) )
		goto err2;
	
	neo_free( resp );
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_tbl( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	r_req_dump_tbl_t	req;
	r_res_dump_tbl_t	res;
	r_tbl_t			*td;

DBG_ENT(M_RDB,"svc_dump_tbl");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 *	check data
	 */
	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.t_name);

	if( (neo_errno = check_td( td )) ) {

		err_reply( pd, hp, neo_errno );

		goto err1;
	}
	/*
	 *	refer
	 */
	bcopy( td, &res.t_tbl, sizeof(res.t_tbl) );

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) )
		goto err1;
	
DBG_EXIT(0);
	return( 0 );

err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_usr( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	r_req_dump_usr_t	req;
	r_res_dump_usr_t	res;
	r_tbl_t			*td;
	int			pos;
	_dlnk_t			*dp;

DBG_ENT(M_RDB,"svc_dump_usr");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 *	check data
	 */
	td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, req.u_name);
	pos	= req.u_pos;

	if( (neo_errno = check_td( td )) ) {

		err_reply( pd, hp, neo_errno );

		goto err1;
	}
	if( pos < 0 ) {

		err_reply( pd, hp, E_RDB_INVAL );

		goto err1;
	}
	/*
	 *	refer
	 */
	for( dp = _dl_head( &td->t_usr );
		_dl_valid( &td->t_usr, dp ) && pos;
				dp = _dl_next( dp ), pos-- );
	
	if( !_dl_valid( &td->t_usr, dp ) || pos ) {

		err_reply( pd, hp, E_RDB_NOEXIST );

		goto err1;
	}

	res.u_addr = (char *)dp;
	bcopy( dp, &res.u_usr, sizeof(res.u_usr) );

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)&res, sizeof(res), 0 );

	if( p_reply( pd, (p_head_t*)&res, sizeof(res) ) )
		goto err1;
	
DBG_EXIT(0);
	return( 0 );

err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_mem( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	r_req_dump_mem_t	req;
	r_res_dump_mem_t	*resp;
	int			size;
	char			*addr;

DBG_ENT(M_RDB,"svc_dump_mem");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 *	check data
	 */
	addr	= req.m_addr;
	size	= req.m_len;

	/**************************************************************
	if( addr == (char*)&_neo_port ) {
		;
	} else if( CHK_ADDR( addr ) || CHK_ADDR( addr+size-1 ) ) {

		err_reply( pd, hp, E_RDB_INVAL );

		goto err1;
	}
	***************************************************************/

	/*
	 *	allocate reply buffer
	 */
	len = sizeof(r_res_dump_mem_t) + size;
	if( !(resp = (r_res_dump_mem_t *)neo_malloc( len )) ) {
		
		err_reply( pd, hp, E_RDB_NOMEM );

		goto err1;
	}
	/*
	 *	refer
	 */
	bcopy( (char *)addr, (char *)(resp+1), size );
	resp->m_len	= size;

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)resp, len, 0 );

	if( p_reply( pd, (p_head_t*)resp, len ) )
		goto err2;
	
	neo_free( resp );
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
svc_dump_client( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	int			len;
	int			clinum = 0;
	p_id_t			*ppd;
	r_req_dump_client_t	req;
	r_res_dump_client_t	*resp;
	usr_adm_t		*p_clnt;

DBG_ENT(M_RDB,"svc_dump_client");

	len = sizeof(req);
	if( p_recv( pd, (p_head_t*)&req, &len ) )
		goto err1;
	/*
	 * account connected client
	 */
	for ( ppd = ( p_id_t *)_dl_head( &_neo_port );
			_dl_valid( &_neo_port, ppd ); 
				ppd = ( p_id_t *)_dl_next(ppd)  ) {

		if ( (ppd->i_stat & P_ID_CONNECTED) && PDTOENT(ppd) )
			clinum ++;
	}

	/*
	 *	allocate reply buffer
	 */
	len = sizeof(r_res_dump_client_t) + clinum * sizeof( usr_adm_t );
	if( !(resp = (r_res_dump_client_t *)neo_malloc( len )) ) {
		
		err_reply( pd, hp, E_RDB_NOMEM );
		goto err1;
	}

	p_clnt = ( usr_adm_t * ) ( resp + 1 );
	resp->cli_num = clinum;

	/*
	 *	refer
	 */
	for ( ppd = ( p_id_t *)_dl_head( &_neo_port );
			_dl_valid( &_neo_port, ppd ); 
				ppd = ( p_id_t *)_dl_next(ppd) ) {

		if ( (ppd->i_stat & P_ID_CONNECTED) &&  PDTOENT(ppd) ) {
			
			bcopy( &( PDTOENT(ppd)->p_clnt ), p_clnt++,
						sizeof( usr_adm_t ) );
		}
	}

	/*
	 *	reply
	 */
	reply_head( (p_head_t*)&req, (p_head_t*)resp, len, 0 );

	if( p_reply( pd, (p_head_t*)resp, len ) )
		goto err2;
	
	neo_free( resp );
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
