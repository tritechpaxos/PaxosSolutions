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
#include	"./sql.h"

extern r_expr_t	*sql_getnode();
extern r_expr_t	*SQLreturn;

//extern char	*sql_mem_malloc();

r_tbl_t* sql_res_open();
r_tbl_t* sql_res_sql();

int
SqlExecute( md, statement )
	r_man_t	*md;
	char	*statement;
{
	p_head_t	head;
	r_req_sql_t	*reqp;
	int		len;
	r_tbl_t		*tdp;
	char		*bufp;
	r_expr_t	*statementp;
	int		num;

DBG_ENT(M_SQL,"SqlExecute");

#ifdef	ZZZ
#else
	ASSERT( _dl_head(&(md->r_result)) == NULL );
#endif
	sql_mem_init();
	len = strlen( statement ) + 1;
	len *= 2;
	if( !(bufp = sql_mem_malloc( len ) ) ) {
		goto err3;
	}
	sql_lex_init();
	sql_lex_start( statement, bufp, len );
	if( sqlparse() ) {
		neo_errno = E_SQL_PARSE;
		goto err2;
	}
	num = 0;
	statementp = 0;
	while( (statementp = sql_getnode( SQLreturn, statementp, SSTATEMENT ))) {
		num++;
	}

	reqp = (r_req_sql_t*)neo_malloc
			(sizeof(r_req_sql_t)+strlen(statement)+1);

	strcpy( (char*)(reqp+1), statement );

	_rdb_req_head( (p_head_t*)reqp, R_CMD_SQL, 
		len = sizeof(*reqp)+strlen(statement)+1 );

	reqp->sql_Major	= md->r_Seq++;
	
	if( p_request( md->r_pd, (p_head_t*)reqp, len ) )
		goto err1;

	while( num-- ) {

 		if( p_peek( md->r_pd, (p_head_t*)&head, sizeof(head), P_WAIT ) < 0 ) {
			goto err1;
		}

     	switch( head.h_cmd ) {
			case R_CMD_SQL:
               	tdp = SqlResSql( md->r_pd, &head );
				if( neo_errno )
					goto err1;
				if( tdp ) {
					_dl_insert( tdp, &(md->r_result) );
				}
				break;
             case R_CMD_OPEN:
                tdp = SqlResOpen( md->r_pd, &head );
				tdp->t_md = md;
				_dl_insert( tdp, &(md->r_result) );
                break;
		}
	}
//end:
	neo_free( reqp );
	sql_free( SQLreturn );
	sql_mem_free( bufp );
	return( 0 );			
err1:
	neo_free( reqp );
	sql_free( SQLreturn );
err2:
	sql_mem_free( bufp );
err3:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return( -1 );
}

r_tbl_t*
SqlQuery( md, statement )
	r_man_t	*md;
	char	*statement;
{
	r_tbl_t		*tdp;

DBG_ENT(M_SQL,"SqlQuery");

	SqlExecute( md, statement );

	tdp = (r_tbl_t*)_dl_head( &(md->r_result));
	if( !tdp )
		goto err1;

	if( SqlSelect( md->r_pd, tdp ) )	/* Get ResultSet */
			goto err1;

	DBG_EXIT( tdp );
	return( tdp );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return( 0 );
}

r_tbl_t*
SqlUpdate( md, statement )
	r_man_t	*md;
	char	*statement;
{
	r_tbl_t		*tdp;

DBG_ENT(M_SQL,"SqlUpdate");

	SqlExecute( md, statement );

	tdp = (r_tbl_t*)_dl_head( &(md->r_result));
	if( !tdp )
		goto err1;

	DBG_EXIT( tdp );
	return( tdp );
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return( 0 );
}

r_tbl_t*
SqlResSql( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	r_res_sql_t	*resp;
	int		len;
	Msg_t	*pMsg;
	r_tbl_t		*tdp = 0;
	r_scm_t		*scmp;
	r_scm_t		*sp;
	r_res_sql_update_t
			*up;
	char		*rp;
	int        *flagp;

	pMsg	= p_responseByMsg( pd );
	if( !pMsg )	goto err1;

	resp	= (r_res_sql_t*)MsgFirst( pMsg );
	len	= MsgTotalLen( pMsg );
    if( (neo_errno = resp->sql_head.h_err) ) {
		goto err2;
	}
    
	if( len > sizeof(r_res_sql_t)) {	/* updated data */

		flagp = (int*)(resp+1);
		up = (r_res_sql_update_t*)(flagp+1);
		sp = (r_scm_t*)(up+1);

		if( !(tdp = _r_tbl_alloc() ) ) {
			goto err2;
		}
		tdp->t_rec_used	= up->u_updated;
		tdp->t_rec_num	= up->u_num;
		tdp->t_rec_size	= up->u_rec_size;

		if( !(scmp = (r_scm_t*)neo_malloc( 
		(len = sizeof(r_scm_t) + sizeof(r_item_t)*(sp->s_n-1) )) )) {
			goto err3;
		}
		bcopy( sp, scmp, len );
		tdp->t_scm	= scmp;

		if( !(rp = neo_malloc( up->u_num*up->u_rec_size ) ) ) {
			goto err4;
		}
		bcopy( (char*)sp + len, rp, up->u_num*up->u_rec_size );
		tdp->t_rec = (r_rec_t*)rp;
	}
	MsgDestroy( pMsg );
	return( tdp );

err4:	neo_free( scmp );
err3:	_r_tbl_free( tdp );
err2:	MsgDestroy( pMsg );
err1:
	if( neo_errno )
		printf(":%s\n", neo_errsym() );
	return( 0 );
}

r_tbl_t*
SqlResOpen( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	r_res_open_t	*resp;
	r_scm_t		*sp;
	r_tbl_t		*td;
	Msg_t	*pMsg;
	
	pMsg	= p_responseByMsg( pd );
	if( !pMsg )	goto err1;
	resp = (r_res_open_t*)MsgFirst( pMsg );
	if( (neo_errno = resp->o_head.h_err) )	goto err2;

		if( !(td = _r_tbl_alloc() ) ) {
                neo_errno = E_RDB_NOMEM;
                goto err2;
		}
        /*
         * スキーマを作成する
         */
        if( !(sp = (r_scm_t*)neo_malloc(sizeof(r_scm_t)
                                + sizeof(r_item_t)*(resp->o_n - 1) ) ) ) {
                neo_errno = E_RDB_NOMEM;
                goto err3;
        }
        bcopy( resp->o_items, sp->s_item, sizeof(r_item_t)*resp->o_n );
        sp->s_n = resp->o_n;

        /*
         * ローカルのテーブル記述子の情報を設定して、
         * DB_MAN記述子へ登録する
         */
		strncpy( td->t_name, resp->o_name, sizeof(td->t_name) );

		td->t_stat	|= T_TMP;
        td->t_rec_num   = resp->o_rec_num;
        td->t_rec_used  = resp->o_rec_used;
        td->t_rec_size  = resp->o_rec_size;

        td->t_scm       = sp;

	MsgDestroy( pMsg );

	return( td );

err3:	_r_tbl_free( td );
err2:	MsgDestroy( pMsg );
err1:
	return( 0 );
}

int
SqlSelect( pd, td )
	p_id_t		*pd;
	r_tbl_t		*td;
{
	int		size;
	int		num;
	char		*bufp;
	int		n;

	DBG_ENT(M_RDB,"SqlSelect");

        size = rdb_size( td );
        num  = td->t_rec_used;

        DBG_A(("size=%d,num=%d\n", size, num ));
	if( num <= 0 ) {
		return(0);
	}

#ifdef XXX
	for( i = 0; i < td->t_scm->s_n; i++ ) {
		printf("[%s]", &td->t_scm->s_item[i].i_name );
	}
	printf("\n");
#endif
        /*
         *      allocate buffer
         */
        if( !(bufp = (char *)neo_zalloc( num*size ) ) )
                goto err1;
        /*
         *      find
         */
        while( (n = rdb_find( td, num, (r_head_t*)bufp, 0 ) ) > 0 ) {

                DBG_A(("n=%d\n", n ));
#ifdef XXX
				int i;
				r_head_t	*rp;
                for( i = 0, rp = (r_head_t*)bufp; i < n;
                                i++, rp = (r_head_t*)((char*)rp + size ) ) {

                        DBG_A(("rec(rel=%d,abs=%d,stat=0x%x[%s])\n",
                                rp->r_rel, rp->r_abs, rp->r_stat, rp+1 ));

                        print_record(i, rp, td->t_scm->s_n, td->t_scm->s_item);
                }
#endif
        }

	/* cast */
	td->t_rec_num	= num;
	td->t_rec	= (r_rec_t*)bufp;

	return( 0 );
err1:
	return( neo_errno );
}

int
SqlResultFirstOpen( r_man_t *mdp, r_tbl_t **tdpp )
{
	r_tbl_t	*tdp;

	tdp = list_first_entry( &mdp->r_result, r_tbl_t, t_lnk );
	*tdpp	= tdp;
	if( !tdp )	return( 0 );

	if( tdp->t_lnk.q_next != &mdp->r_result )	
			mdp->r_result_next	= tdp->t_lnk.q_next;
	else	 mdp->r_result_next	= NULL;

	if( tdp->t_stat & T_TMP ) {
		if( SqlSelect( mdp->r_pd, tdp ) )	return( -1 );
	}
	return( 0 );
}

int
SqlResultNextOpen( r_man_t *mdp, r_tbl_t **tdpp )
{
	r_tbl_t	*tdp;

	*tdpp	= tdp = (r_tbl_t*)mdp->r_result_next;
	if( !tdp )	return( 0 );

	if( tdp->t_lnk.q_next != &mdp->r_result )	
			mdp->r_result_next	= tdp->t_lnk.q_next;
	else	 mdp->r_result_next	= NULL;

	if( tdp->t_stat & T_TMP ) {
		if( SqlSelect( mdp->r_pd, tdp ) )	return( -1 );
	}
	return( 0 );
}

void
SqlResultClose( tdp )
	r_tbl_t	*tdp;
{
	if( tdp->t_rec )	neo_free( tdp->t_rec );

	if( tdp->t_md ) {
		rdb_close( tdp );
	} else {
		_dl_remove( tdp );
		_r_tbl_free( tdp );
	}
}

int
sql_prepare( md, sql, pstmt )
	r_man_t	*md;
	const char	*sql;
	r_stmt_t **pstmt;
{
	int		len;
	char		*bufp;
	r_expr_t	*statementp;
	r_stmt_t	*stmt;
	int		num;
	size_t		size;
	char		*ptr;

DBG_ENT(M_SQL,"sql_prepare");

	ASSERT( _dl_head(&(md->r_result)) == NULL );
	sql_mem_init();
	len = strlen( sql ) + 1;
	len *= 2;
	if( !(bufp = sql_mem_malloc( len ) ) ) {
		goto err0;
	}
	sql_lex_init();
	sql_lex_pstat_start( sql, bufp, len );
	if( sqlparse() ) {
		neo_errno = E_SQL_PARSE;
		goto err0;
	}

	num = 0;
	statementp = 0;
	while ((statementp = sql_getnode( SQLreturn, statementp, TLEAF ))) {
		if (statementp->e_tree.t_unary.v_type == VPARAM) {
			num++;
		}
	}

	size = sizeof(*stmt) + strlen(sql) + 1 + sizeof(r_value_t) * num;
	stmt = (r_stmt_t *) neo_malloc(size);
	if (!stmt) {
		goto err1;
	}
	memset(stmt, 0, size);
	stmt->st_num_params = num;

	ptr = (char *) stmt + sizeof(*stmt);
	stmt->st_statement = ptr;
	strcpy((char *) stmt->st_statement, sql);
	stmt->st_stmt_size = strlen(sql) + 1;

	ptr += stmt->st_stmt_size;
	stmt->st_params = (r_value_t *) ptr;
	
	stmt->st_md = md;
	_dl_insert(stmt, &md->r_stmt);

	*pstmt = stmt;
	
	sql_free( SQLreturn );
	sql_mem_free( bufp );
	return( 0 );
err1:
	sql_free( SQLreturn );
	sql_mem_free( bufp );
err0:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return( -1 );
}

static int
is_sql_stmt_valid(stmt)
	r_stmt_t *stmt;
{
	if (stmt->st_stat & T_CLOSED) {
		return -1;
	}
	return 0;
}

static int
sql_stmt_clear_param(stmt, pos)
	r_stmt_t *stmt;
	int pos;
{
	r_value_t *param;

	if (pos < 0 || pos >= stmt->st_num_params) {
		return -1;
	}
	param = stmt->st_params + pos;
	if (param->v_type == R_STR) {
		neo_free(param->v_value.v_str.s_str);
	}
	memset(param, 0, sizeof(*param));
	return 0;
}

int
sql_bind_int( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	int val;
{
	r_value_t *param;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}
	param = stmt->st_params + pos;
	param->v_type = R_INT;
	param->v_value.v_int = val;

	return 0;
}

int
sql_bind_uint( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	unsigned int val;
{
	r_value_t *param;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}
	param = stmt->st_params + pos;
	param->v_type = R_UINT;
	param->v_value.v_uint = val;

	return 0;
}

int
sql_bind_long( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	long val;
{
	r_value_t *param;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}
	param = stmt->st_params + pos;
	param->v_type = R_LONG;
	param->v_value.v_long = val;

	return 0;
}

int
sql_bind_ulong( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	unsigned long val;
{
	r_value_t *param;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}
	param = stmt->st_params + pos;
	param->v_type = R_ULONG;
	param->v_value.v_ulong = val;

	return 0;
}

int
sql_bind_double( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	double val;
{
	r_value_t *param;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}
	param = stmt->st_params + pos;
	param->v_type = R_FLOAT;
	param->v_value.v_float = val;

	return 0;
}

int
sql_bind_str( stmt, pos, val )
	r_stmt_t *stmt;
	int pos;
	const char *val;
{
	r_value_t *param;
	char *str;
	size_t len;

	if (is_sql_stmt_valid(stmt) < 0 ||
	    sql_stmt_clear_param(stmt, pos) < 0) {
		return -1;
	}

	len = strlen(val) + 1;
	str = neo_malloc(len);
	if (!str) {
		return -1;
	}
	strncpy(str, val, len);
	
	param = stmt->st_params + pos;
	param->v_type = R_STR;
	param->v_value.v_str.s_str = str;
	param->v_value.v_str.s_len = len - 1;

	return 0;
}

static int
sql_calc_params_size( stmt )
	r_stmt_t *stmt;
{
	int size = 0;
	int i;
	r_value_t *param;

	for (i = 0, param = stmt->st_params; i < stmt->st_num_params;
	     i++, param++) {
		switch (param->v_type) {
		case R_INT:
		case R_UINT:
			size += sizeof(param->v_type) + sizeof(size_t);
			size += sizeof(param->v_value.v_int);
			break;
		case R_LONG:
		case R_ULONG:
			size += sizeof(param->v_type) + sizeof(size_t);
			size += sizeof(param->v_value.v_long);
			break;
		case R_FLOAT:
			size += sizeof(param->v_type) + sizeof(size_t);
			size += sizeof(param->v_value.v_float);
			break;
		case R_STR:
			size += sizeof(param->v_type) + sizeof(size_t);
			size += param->v_value.v_str.s_len + 1;
			break;
		default:
			return -1;
		}
	}
	return size;
}

static void
sql_copy_params(stmt, dest)
	r_stmt_t *stmt;
	char *dest;
{
	int i;
	r_value_t *param;
	r_sql_param_t *pdest = (r_sql_param_t *) dest;

	for (i = 0, param = stmt->st_params; i < stmt->st_num_params;
	     i++, param++) {
		switch (param->v_type) {
		case R_INT:
			pdest->sp_type = R_INT;
			pdest->sp_size = sizeof(param->v_value.v_int);
			memcpy(pdest->sp_data, &param->v_value.v_int,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		case R_UINT:
			pdest->sp_type = R_UINT;
			pdest->sp_size = sizeof(param->v_value.v_uint);
			memcpy(pdest->sp_data, &param->v_value.v_uint,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		case R_LONG:
			pdest->sp_type = R_LONG;
			pdest->sp_size = sizeof(param->v_value.v_long);
			memcpy(pdest->sp_data, &param->v_value.v_long,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		case R_ULONG:
			pdest->sp_type = R_ULONG;
			pdest->sp_size = sizeof(param->v_value.v_ulong);
			memcpy(pdest->sp_data, &param->v_value.v_ulong,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		case R_FLOAT:
			pdest->sp_type = R_FLOAT;
			pdest->sp_size = sizeof(param->v_value.v_float);
			memcpy(pdest->sp_data, &param->v_value.v_float,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		case R_STR:
			pdest->sp_type = R_STR;
			pdest->sp_size = param->v_value.v_str.s_len + 1;
			memcpy(pdest->sp_data, param->v_value.v_str.s_str,
			       pdest->sp_size);
			pdest =
			  (r_sql_param_t *) ((char *) pdest + sizeof(*pdest)
					     + pdest->sp_size);
			break;
		}
	}
}


int
sql_statement_execute( stmt )
	r_stmt_t *stmt;
{
	p_head_t head;
	r_req_sql_params_t *reqp;
	int param_size;
	size_t size;
	char *ptr;
	r_tbl_t *tdp;

	if (is_sql_stmt_valid(stmt) < 0) {
		goto err2;
	}
	ASSERT( _dl_head(&(stmt->st_md->r_result)) == NULL );

	param_size = sql_calc_params_size(stmt);
	if (param_size < 0) {
		goto err2;
	}

	size = sizeof(r_req_sql_params_t) + stmt->st_stmt_size + param_size;
	reqp = (r_req_sql_params_t *) neo_malloc(size);

	_rdb_req_head((p_head_t*) reqp, R_CMD_SQL_PARAMS, size);
	reqp->sql_Major = stmt->st_md->r_Seq++;
	reqp->sql_stmt_size = stmt->st_stmt_size;
	reqp->sql_num_params = stmt->st_num_params;

	ptr = (char *) reqp;
	ptr += sizeof(*reqp);
	strcpy(ptr, stmt->st_statement);
	ptr += stmt->st_stmt_size;
	sql_copy_params(stmt, ptr);

	if (p_request(stmt->st_md->r_pd, (p_head_t*) reqp, size)) {
		goto err1;
	}

	if (p_peek(stmt->st_md->r_pd, (p_head_t *) &head, sizeof(head),
		   P_WAIT) < 0 ) {
		goto err1;
	}

     	switch (head.h_cmd) {
	case R_CMD_SQL_PARAMS:
		tdp = SqlResSql(stmt->st_md->r_pd, &head);
		if (neo_errno) {
			goto err1;
		}
		if (tdp) {
			_dl_insert(tdp, &(stmt->st_md->r_result));
		}
		break;
	case R_CMD_OPEN:
		tdp = SqlResOpen(stmt->st_md->r_pd, &head);
		tdp->t_md = stmt->st_md;
		_dl_insert(tdp, &(stmt->st_md->r_result));
                break;
	}
	neo_free( reqp );

	return 0;
err1:
	neo_free( reqp );
err2:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
	return -1;
}

int
sql_statement_reset( stmt )
	r_stmt_t *stmt;
{
	int i;
	for (i = 0; i < stmt->st_num_params; i++) {
		sql_stmt_clear_param(stmt, i);
	}
	return 0;
}

int
sql_statement_close( stmt )
	r_stmt_t *stmt;
{
	int i;
	for (i = 0; i < stmt->st_num_params; i++) {
		sql_stmt_clear_param(stmt, i);
	}
	_dl_remove(&stmt->st_lnk);
	neo_free(stmt);
	return 0;
}
