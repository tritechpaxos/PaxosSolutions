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
rdb_commit( md )
	r_man_t		*md;
{
	p_id_t		*pd;
	r_req_commit_t	req;
	r_res_commit_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_commit");
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
	_rdb_req_head( &req.c_head, R_CMD_COMMIT, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) 
					|| (neo_errno = res.c_head.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

int
rdb_rollback( md )
	r_man_t		*md;
{
	p_id_t		*pd;
	r_req_rollback_t	req;
	r_res_rollback_t	res;
	int			len;

DBG_ENT(M_RDB,"rdb_rollback");
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
	_rdb_req_head( (p_head_t*)&req, R_CMD_ROLLBACK, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	len = sizeof(res);
	if( p_response( pd, (p_head_t*)&res, &len ) 
						|| (neo_errno = res.r_head.h_err) )
		goto	err1;

DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}
