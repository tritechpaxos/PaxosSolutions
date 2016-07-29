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

int
rdb_list( md, respp )
	r_man_t		*md;
	r_res_list_t	**respp;
{
	p_id_t		*pd;
	p_head_t	head;
	r_req_list_t	req;
	r_res_list_t	*resp;
	int		len;

DBG_ENT(M_RDB,"rdb_list");
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
	_rdb_req_head( (p_head_t*)&req, R_CMD_LIST, sizeof(req) );
	
	/*
	 *	request
	 */
	if( p_request( pd = md->r_pd, (p_head_t*)&req, sizeof(req) ) )
		goto	err1;
	/*
	 *	response
	 */
	if( p_peek( pd, &head, sizeof(head), P_WAIT ) < 0 )
		goto err1;

	len = sizeof(p_head_t) + head.h_len;
	if( !(resp = (r_res_list_t *)neo_malloc( len )) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	if( p_response( pd, (p_head_t*)resp, &len ) 
					|| (neo_errno = resp->l_head.h_err) )
		goto	err2;

	*respp = resp;

DBG_EXIT(0);
	return( 0 );

err2:	neo_free( resp );
err1:
DBG_EXIT(-1);
	return( -1 );
}
