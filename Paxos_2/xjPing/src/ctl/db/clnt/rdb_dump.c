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

#include	<string.h>
#include	"neo_db.h"

int
rdb_dump_lock( md )
	r_man_t		*md;
{
	p_id_t		*pd;
	p_head_t	req;
	p_head_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_dump_poc");
	/*
	 *	argument check
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( &req, R_CMD_DUMP_LOCK, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, &req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(res);
	if( p_response( pd, &res, &len ) || (neo_errno = res.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_unlock( md )
	r_man_t		*md;
{
	p_id_t		*pd;
	p_head_t	req;
	p_head_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_dump_poc");
	/*
	 *	argument check
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( &req, R_CMD_DUMP_UNLOCK, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, &req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(res);
	if( p_response( pd, &res, &len ) || (neo_errno = res.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_proc( md )
	r_man_t		*md;
{
	p_id_t		*pd;
	p_head_t	req;
	p_head_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_dump_poc");
	/*
	 *	argument check
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( &req, R_CMD_DUMP_PROC, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, &req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(res);
	if( p_response( pd, &res, &len ) || (neo_errno = res.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_inf( md, resp )
	r_man_t			*md;
	r_res_dump_inf_t	*resp;
{
	p_id_t			*pd;
	r_req_dump_inf_t	req;
	int			len;

DBG_ENT(M_RDB,"rdb_dump_inf");
	/*
	 *	argument check
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DUMP_INF, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(*resp);
	if( p_response( pd, (p_head_t*)resp, &len ) 
					|| (neo_errno = resp->i_head.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_record( td, abs, recp, bufp )
	r_tbl_t		*td;
	int		abs;
	r_rec_t		*recp;
	char		*bufp;
{
	p_id_t		*pd;
	r_req_dump_record_t	req;
	r_res_dump_record_t	*resp;
	r_res_dump_record_t	head;
	int		len;

DBG_ENT(M_RDB,"rdb_dump_record");
	/*
	 *	argument check
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	if( abs < 0 ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DUMP_RECORD, sizeof(req) );
	
	strncpy( req.r_name, td->t_name, sizeof(req.r_name) );
	req.r_abs	= abs;

	/*
	 *	request
	 */
	if( p_request( pd = TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	if( p_peek( pd, (p_head_t*)&head, sizeof(head), P_WAIT ) < 0 )
		goto err1;

	len = sizeof(p_head_t) + head.r_head.h_len;
	if( !(resp = (r_res_dump_record_t *)neo_malloc( len )) ) {

		neo_errno = E_RDB_NOMEM;

		goto err1;
	}
	if( p_response( pd, (p_head_t*)resp, &len ) 
					|| (neo_errno = resp->r_head.h_err) )
		goto	err2;
	/*
	 *	data copy
	 */
	if( (len - sizeof(r_res_dump_record_t) ) != td->t_rec_size ) {

		neo_errno = E_RDB_CONFUSED;

		goto err2;
	}

	bcopy( &resp->r_rec, recp, sizeof(resp->r_rec) );
	bcopy( resp+1, bufp, td->t_rec_size );

	neo_free( resp );
	
DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( resp );
err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_tbl( td, tbl )
	r_tbl_t		*td;
	r_tbl_t		*tbl;
{
	p_id_t		*pd;
	r_req_dump_tbl_t	req;
	r_res_dump_tbl_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_dump_table");
	/*
	 *	argument check
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DUMP_TBL, sizeof(req) );
	
	strncpy( req.t_name, td->t_name, sizeof(req.t_name) );

	/*
	 *	request
	 */
	if( p_request( pd = TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len	= sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) 
					|| (neo_errno = res.t_head.h_err) )
		goto	err1;
	/*
	 *	data copy
	 */
	bcopy( &res.t_tbl, tbl, sizeof(r_tbl_t) );
	
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_usr( td, pos, usr, u_addr )
	r_tbl_t		*td;
	int		pos;
	r_usr_t		*usr;
	char		**u_addr;
{
	p_id_t		*pd;
	r_req_dump_usr_t	req;
	r_res_dump_usr_t	res;
	int		len;

DBG_ENT(M_RDB,"rdb_dump_usr");
	/*
	 *	argument check
	 */
	if( td->t_ident != R_TBL_IDENT ) {
		neo_errno = E_RDB_TD;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DUMP_USR, sizeof(req) );
	
	strncpy( req.u_name, td->t_name, sizeof(req.u_name) );
	req.u_pos	= pos;

	/*
	 *	request
	 */
	if( p_request( pd = TDTOPD(td), (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len	= sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) 
					|| (neo_errno = res.u_head.h_err) )
		goto	err1;
	/*
	 *	data copy
	 */
	bcopy( &res.u_usr, usr, sizeof(r_usr_t) );
	*u_addr = res.u_addr;
	
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_dump_mem( md, addr, size, bufp )
	r_man_t		*md;
	char		*addr;
	int		size;
	char		*bufp;
{
	p_id_t			*pd;
	r_req_dump_mem_t	req;
	r_res_dump_mem_t	*resp;
	r_res_dump_mem_t	head;
	int			len;

DBG_ENT(M_RDB,"rdb_dump_mem");
	/*
	 *	argument check
	 */
	if( md->r_ident != R_MAN_IDENT ) {
		neo_errno = E_RDB_MD;
		goto err1;
	}
	if( size < 0 ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}
	/*
	 *	setup request
	 */
	_rdb_req_head( (p_head_t*)&req, R_CMD_DUMP_MEM, sizeof(req) );
	
	req.m_addr	= addr;
	req.m_len	= size;

	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	if( p_peek( pd, (p_head_t*)&head, sizeof(head), P_WAIT ) < 0 )
		goto err1;

	len = sizeof(p_head_t) + head.m_head.h_len;
	if( !(resp = (r_res_dump_mem_t *)neo_malloc( len )) ) {

		neo_errno = E_RDB_NOMEM;

		goto err1;
	}
	if( p_response( pd, (p_head_t*)resp, &len ) 
				|| (neo_errno = resp->m_head.h_err) )
		goto	err2;
	/*
	 *	data copy
	 */
	len = resp->m_len;
	bcopy( resp+1, bufp, len );

	neo_free( resp );
	
DBG_EXIT(len);
	return( len );

err2: DBG_T("err2");
	neo_free( resp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
