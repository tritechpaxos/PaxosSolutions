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

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<unistd.h>

#include	"neo_db.h"
#include	"svc_logmsg.h"
#include	"join.h"
#include	"../sql/sql.h"

extern r_expr_t	*sql_getnode();
extern r_expr_t	*SQLreturn;

extern r_tbl_t	*prc_join();

int
svc_sql( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	Msg_t	*pMsg;
	int		ret;
	r_req_sql_t	*reqp;

	DBG_ENT(M_SQL,"svc_sql");

DBG_A(("hp->h_len=%d\n", hp->h_len ));

	pMsg = p_recvByMsg( pd );
	if( !pMsg )	goto err1;

	reqp	= (r_req_sql_t*)MsgFirst( pMsg );
DBG_B(("statesments[%s]\n", reqp+1 ));
LOG(neo_log,LogDBG,"[%s]", reqp+1);

	if( (ret	= prc_sql( pd, reqp, (char*)(reqp+1) )) ) {
		goto err2;
	}

	MsgDestroy( pMsg );

DBG_B(("ret(0x%x)\n", ret ));
	DBG_EXIT( 0 );
	return( 0 );

err2:	MsgDestroy( pMsg );
err1:
	err_reply( pd, hp, neo_errno );

DBG_B(("ret(%s)\n", neo_errsym()));
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
svc_sql_params( pd, hp )
	p_id_t		*pd;
	p_head_t	*hp;
{
	Msg_t	*pMsg;
	int		ret;
	r_req_sql_params_t	*reqp;

	DBG_ENT(M_SQL,"svc_sql");

DBG_A(("hp->h_len=%d\n", hp->h_len ));

	pMsg = p_recvByMsg( pd );
	if( !pMsg )	goto err1;

	reqp	= (r_req_sql_params_t *) MsgFirst( pMsg );
DBG_B(("statesments[%s]\n", reqp+1 ));

	if( (ret	= prc_sql_params( pd, reqp, (char*)(reqp+1) )) ) {
		goto err2;
	}

	MsgDestroy( pMsg );

DBG_B(("ret(0x%x)\n", ret ));
	DBG_EXIT( 0 );
	return( 0 );

err2:	MsgDestroy( pMsg );
err1:
	err_reply( pd, hp, neo_errno );

DBG_B(("ret(%s)\n", neo_errsym()));
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

/*
 *	SQL statements dispatcher
 */
int
prc_sql( pd, reqp, statements )
	p_id_t		*pd;
	r_req_sql_t	*reqp;
	char		*statements;
{
	char		*bufp;
	int		len;
	r_expr_t	*statementp;
	r_expr_t	*sp;
	int		ret;
//	r_res_sql_t	res;
	r_res_sql_t	*resp = 0;
	int		changed = 0;
	int		resflag;
	int		Seq = 0;

	DBG_ENT(M_SQL,"prc_sql");

	len = strlen(statements)+1;
	len *= 2;

	sql_mem_init();

	if( !(bufp = sql_mem_malloc( len )) ){
		goto err1;
	}
        sql_lex_start( statements, bufp, len );

        if( (ret = sqlparse()) ) {
		neo_errno = E_SQL_PARSE;
		goto err2;
	}

    DBG_AF((sqlprint( SQLreturn )));

	statementp = 0;
	while( (statementp = sql_getnode( SQLreturn, statementp, SSTATEMENT )) ) {

		resp = 0;
		changed = 0;
		resflag	= 1;
		reqp->sql_Minor = Seq++;
		if( !(sp = statementp->e_tree.t_binary.b_l) )
			continue;

		switch( sp->e_op ) {
			case SSELECT:
				if( sql_select( pd, reqp, sp ) )	goto err3;
				resflag = 0;
				break;
			case SCREATE_VIEW:
				if( sql_create_view( sp, statements ) )	
								goto err3;
				break;
			case SCREATE_TABLE:
				if( sql_create_table( sp ) )	goto err3;
				break;
			case SDROP_TABLE:
				if( sql_drop_table( sp ) )	goto err3;
				break;
			case SDROP_VIEW:
				if( sql_drop_view( sp ) )	goto err3;
				break;
			case SINSERT:
				if( sql_insert( pd, sp, (char**)&resp) )	goto err3;
				resp->sql_changed = changed;
				len = resp->sql_head.h_len + sizeof(p_head_t);
				reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

				if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
//					sql_mem_free(resp);
					goto err3;
				}
				
//				sql_mem_free(resp);
				resflag = 0;
				//end
				break;
			case SDELETE:
				if( sql_delete( pd, sp ) )	goto err3;
				break;
			case SUPDATE:
				if( sql_update( pd, sp, &changed, (char**)&resp ) )
								goto err3;
				resp->sql_changed = changed;
				len = resp->sql_head.h_len + sizeof(p_head_t);
				reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

				if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
//				    sql_mem_free(resp);
					goto err3;
				}
//				sql_mem_free(resp);
				resflag = 0;
				break;
			case SBEGIN:
				if( sql_begin( pd, sp ) )	goto err3;
				break;
			case SCOMMIT:
				if( sql_commit( pd, sp ) )	goto err3;
				break;
			case SROLLBACK:
				if( sql_rollback( pd, sp ) )	goto err3;
				break;
			default:
				neo_errno = E_SQL_PARSE;	goto err3;
		}
		if( resflag ) {
			resp = (r_res_sql_t*)neo_malloc( sizeof(*resp) );
			resp->sql_changed	= changed;
			len = sizeof(*resp);
			reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

			if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
				goto err3;
			}
		}
	}

	sql_free( SQLreturn );

	sql_mem_free( bufp );

	DBG_EXIT( 0 );
	return( 0 );

err3:	sql_free( SQLreturn );
err2:	sql_mem_free( bufp );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

static int
prc_sql_bind_params(statement, reqp)
	r_expr_t	*statement;
	r_req_sql_params_t	*reqp;
{
	int pos;
	r_sql_param_t *param;
	r_expr_t *sp;
	r_value_t *val;
	char *ptr;

	ptr = (char *) reqp + sizeof(*reqp) + reqp->sql_stmt_size;
	pos = 0;
	sp = NULL;
	while ((sp = sql_getnode( statement, sp, TLEAF ))) {
		if (sp->e_tree.t_unary.v_type == VPARAM) {
			param = (r_sql_param_t *) ptr;
			val = &sp->e_tree.t_unary;
			switch (param->sp_type) {
			case R_INT:
				val->v_type = VINT;
				val->v_value.v_int = *(int *) param->sp_data;
				break;
			case R_UINT:
				val->v_type = VUINT;
				val->v_value.v_uint =
				  *(unsigned int *) param->sp_data;
				break;
			case R_LONG:
				val->v_type = VLONG;
				val->v_value.v_long = *(long *) param->sp_data;
				break;
			case R_ULONG:
				val->v_type = VULONG;
				val->v_value.v_ulong =
				  *(unsigned long *) param->sp_data;
				break;
			case R_FLOAT:
				val->v_type = VFLOAT;
				val->v_value.v_float =
				  *(float *) param->sp_data;
				break;
			case R_STR:
				val->v_type = VSTRING_ALLOC;
				val->v_value.v_str.s_len = param->sp_size - 1;
				val->v_value.v_str.s_str =
				  neo_malloc(param->sp_size);
				if (!val->v_value.v_str.s_str) {
					return -1;
				}
				strncpy(val->v_value.v_str.s_str,
					param->sp_data, param->sp_size);
				break;
			default:
				return -1;
			}
			ptr += sizeof(*param) + param->sp_size;
			pos++;
			if (pos > reqp->sql_num_params) {
				return -1;
			}
		}
	}
	if (pos < reqp->sql_num_params) {
		return -1;
	}
	return 0;
}

/*
 *	SQL statements dispatcher
 */
int
prc_sql_params( pd, reqp, statements )
	p_id_t		*pd;
	r_req_sql_params_t	*reqp;
	char		*statements;
{
	char		*bufp;
	int		len;
	r_expr_t	*statementp;
	r_expr_t	*sp;
	int		ret;
//	r_res_sql_t	res;
	r_res_sql_t	*resp = 0;
	int		changed = 0;
	int		resflag;
	int		Seq = 0;

	DBG_ENT(M_SQL,"prc_sql");

	len = strlen(statements)+1;
	len *= 2;

	sql_mem_init();

	if( !(bufp = sql_mem_malloc( len )) ){
		goto err1;
	}
        sql_lex_pstat_start( statements, bufp, len );

        if( (ret = sqlparse()) ) {
		neo_errno = E_SQL_PARSE;
		goto err2;
	}

	ret = prc_sql_bind_params(SQLreturn, reqp);

    DBG_AF((sqlprint( SQLreturn )));

	statementp = SQLreturn;

	resp = 0;
	changed = 0;
	resflag	= 1;
	reqp->sql_Minor = Seq++;
	sp = statementp;

	switch( sp->e_op ) {
	case SSELECT:
		if( sql_select( pd, (r_req_sql_t *) reqp, sp ) ) goto err3;
		resflag = 0;
		break;
	case SCREATE_VIEW:
		if( sql_create_view( sp, statements ) )	goto err3;
		break;
	case SCREATE_TABLE:
		if( sql_create_table( sp ) )	goto err3;
		break;
	case SDROP_TABLE:
		if( sql_drop_table( sp ) )	goto err3;
		break;
	case SDROP_VIEW:
		if( sql_drop_view( sp ) )	goto err3;
		break;
	case SINSERT:
		if( sql_insert( pd, sp, (char**)&resp) )	goto err3;
		resp->sql_changed = changed;
		len = resp->sql_head.h_len + sizeof(p_head_t);
		reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );
		if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
//					sql_mem_free(resp);
			goto err3;
		}
			
//				sql_mem_free(resp);
		resflag = 0;
		//end
		break;
	case SDELETE:
		if( sql_delete( pd, sp ) )	goto err3;
		break;
	case SUPDATE:
		if( sql_update( pd, sp, &changed, (char**)&resp ) ) goto err3;
		resp->sql_changed = changed;
		len = resp->sql_head.h_len + sizeof(p_head_t);
		reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );
		if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
//				    sql_mem_free(resp);
			goto err3;
		}
//				sql_mem_free(resp);
		resflag = 0;
		break;
	case SBEGIN:
		if( sql_begin( pd, sp ) )	goto err3;
		break;
	case SCOMMIT:
		if( sql_commit( pd, sp ) )	goto err3;
		break;
	case SROLLBACK:
		if( sql_rollback( pd, sp ) )	goto err3;
		break;
	default:
		neo_errno = E_SQL_PARSE;	goto err3;
	}
	if( resflag ) {
		resp = (r_res_sql_t*)neo_malloc( sizeof(*resp) );
		resp->sql_changed	= changed;
		len = sizeof(*resp);
		reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

		if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
			goto err3;
		}
	}

	sql_free( SQLreturn );

	sql_mem_free( bufp );

	DBG_EXIT( 0 );
	return( 0 );

err3:	sql_free( SQLreturn );
err2:	sql_mem_free( bufp );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_find_user( pd, name, rtd, tp )
	p_id_t		*pd;
	char		*name;
	r_tbl_t		*rtd;
	r_from_table_t	*tp;	/* Allocated by upper-caller */
{
	r_port_t	*pe;
	_dlnk_t		*dp;
	r_usr_t		*ud;

	if( (pe = PDTOENT( pd )) ) {
		for( dp = _dl_head(&pe->p_usr_ent);
			_dl_valid(&pe->p_usr_ent,dp); dp = _dl_next(dp) ) {

			ud = PDTOUSR( dp );

			if( /* ud->u_td == rtd
			       && */ !strcmp( name, ud->u_mytd->t_name ) ) {

				tp->f_tdp = ud->u_mytd;
				tp->f_udp = ud;

				goto find;
			}
		}
	}
	return( -1 );
find:
	return( 0 );
}

int
sql_open( table, tp )
	char		*table;
	r_from_table_t	*tp;	/* Allocated by upper-caller */
{
	r_req_open_t	openReq;
	r_res_open_t	*openResp;
	int		res_len;

	DBG_ENT(M_SQL,"sql_open");

	if( strlen( table ) >= R_TABLE_NAME_MAX ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

	bzero( &openReq, sizeof(openReq) );
	strncpy( (char*)&openReq.o_name, table, sizeof(openReq.o_name) );

	if( prc_open( &openReq, &openResp, &res_len, 1 ) ) {
		goto err1;
	}
	neo_rpc_log_msg( "SQL_OPEN", M_RDB, svc_myname, "", OPENLOG01, table);

	tp->f_tdp = openResp->o_td;
	tp->f_udp = openResp->o_ud;

	neo_free( openResp );

	DBG_EXIT( 0 );
	return( 0 );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_close( tp )
	r_from_table_t	*tp;
{
	r_req_close_t	closeReq;
	r_res_close_t	closeRes;

	DBG_ENT(M_SQL,"sql_close");

	closeReq.c_td = tp->f_tdp;
	closeReq.c_ud = tp->f_udp;

	prc_close( &closeReq, &closeRes, 0 );

	if( !neo_errno ) {
		neo_rpc_log_msg( "SQL_CLOSE", M_RDB, svc_myname, 
							"", OPENLOG02, tp->f_tdp->t_name );
	}

	DBG_EXIT( neo_errno );
	return( neo_errno );
}

/*
 *	Return r_res_open_t
 */
int
sql_select( pd, reqp, rp )
	p_id_t		*pd;
	r_req_sql_t	*reqp;
	r_expr_t	*rp;
{
	r_expr_t	*sp;
	r_expr_t	*wp;
	r_expr_t	*fp;
	r_expr_t	*op;
	r_tbl_t		*tdp;
	r_usr_t		*udp;
	int		len;
	r_res_open_t	*resp;
	r_port_t	*pe = PDTOENT(pd);

	DBG_ENT(M_SQL,"sql_select");

DBG_AF((sqlprint( SQLreturn )));

	if(!(fp = sql_getnode( rp, 0, SFROM ))) {
		goto err1;
	}
	if(!(sp = sql_getnode( rp, 0, '*' ))) {
		goto err1;
	}
	wp = sql_getnode( rp, 0, SWHERE );
	op = sql_getnode( rp, 0, SORDER );
	/*
	 *	Create cache table
	 */
	if( !(tdp = prc_join( fp, wp, sp, op ) ) ) {
		goto err1;
	}
	sprintf( tdp->t_name, "%s_%d_%d_%d", 
			inet_ntoa(pd->i_remote.p_addr.a_pa.sin_addr), 
			pe->p_clnt.u_pid, reqp->sql_Major, reqp->sql_Minor );
	tdp->t_access	= time(0);
	tdp->t_stat	|= T_TMP;

	neo_rpc_log_msg( "SEL_OPEN", M_RDB, svc_myname, 
		   &( PDTOENT(pd)->p_clnt), OPENLOG01, tdp->t_name );
	/*
	 *	Register into _open_tbl
	 */
	if( !(udp = _r_usr_alloc() ) ) {
		neo_errno = E_SQL_USR;
		goto err2;
	}
	_dl_insert( tdp, &_svc_inf.f_opn_tbl );
	HashPut( &_svc_inf.f_HashOpen, tdp->t_name, tdp );

	tdp->t_usr_cnt++;
	_dl_insert( udp, &tdp->t_usr );

	_dl_insert( &udp->u_port, &pe->p_usr_ent );

	HashPut( &pe->p_HashUser, tdp->t_name, udp );

	udp->u_pd	= pd;
	udp->u_mytd	= tdp;
	/*
	 *	Reply
	 */
        len = sizeof(r_res_open_t) + sizeof(r_item_t)*(tdp->t_scm->s_n - 1 );
        if( !(resp = (r_res_open_t*)neo_malloc( len ) ) ) {
                goto err4;
        }

        reply_head( (p_head_t*)reqp, (p_head_t*)resp, len, 0 );

        resp->o_head.h_cmd	= R_CMD_OPEN;

		strncpy( resp->o_name, tdp->t_name, sizeof(resp->o_name) );
        resp->o_rec_num         = tdp->t_rec_num;
        resp->o_rec_used        = tdp->t_rec_used;
        resp->o_rec_size        = tdp->t_rec_size;
        bcopy( &tdp->t_scm->s_n, &resp->o_n,
                sizeof(tdp->t_scm->s_n) + sizeof(r_item_t)*tdp->t_scm->s_n );

DBG_B(("reply:td[0x%x]ud[0x%]\n", tdp, udp ));

        if( p_reply_free( pd, (p_head_t*)resp, len ) ) {
DBG_B(("reply error:%s\n", neo_errsym() ));
                goto err5;
        }

//        neo_free( resp );

	DBG_EXIT( 0 );
	return( 0 );

err5:	neo_free( resp );

err4:	/*_dl_remove( tdp );
	tdp->t_usr_cnt--;
	_dl_remove( udp );
	_dl_remove( &udp->u_port );*/

//err3:	/*_r_usr_free(udp);*/close_user( udp );

err2:	/*rec_buf_free( tdp );
	rec_cntl_free( tdp );
	svc_tbl_free( tdp );*/drop_td( tdp );

err1:	DBG_EXIT( neo_errno );
	return( neo_errno );
}

#define	CREATE_REC_NUM	10

typedef struct create_type {
	char	*type;
	int	attr;
	int	len;		/* length is effective. */
	int	size;
} create_type_t;
static create_type_t c_type[] = {
	{"HEX",		R_HEX,		0,	4 },
	{"INT",		R_INT,		0,	4 },
	{"UINT",	R_UINT,		0,	4 },
	{"FLOAT",	R_FLOAT,	0,	4 },
	{"CHAR",	R_STR,		1,	1 },
	{"DATE",	R_LONG,		0,	sizeof(long long) },
	{"VARCHAR",	R_STR,		1,	1 },
	{"NUMBER",	R_LONG,		0,	sizeof(long long) },
	{"LONG",	R_LONG,		0,	sizeof(long long) },
	{"ULONG",	R_ULONG,	0,	sizeof(long long) },
	{"BYTES",	R_BYTES,	1,	64},
};

create_type_t*
sql_getcreatetype( name )
	char	*name;
{
	int	i;
        char    buf[NEO_NAME_MAX];

        for( i = 0; i < strlen(name); i++ ) {
                buf[i] = (char)toupper( (int)name[i] );
        }
        buf[i] = 0;
	for( i = 0; i < sizeof(c_type)/sizeof(c_type[0]); i++ ) {
		if( !strcmp( buf, c_type[i].type ) )
			return( &c_type[i] );
	}
	neo_errno	= E_RDB_NOEXIST;
	return( 0 );
}

int
sql_create_table( root )
	r_expr_t	*root;
{
	r_tbl_t		*td;
	r_expr_t	*tablep;
	r_expr_t	*ep;
	r_expr_t	*tp;
	char		*table;
	int		item_num;
	r_item_t	*itemsp;
	int		i;
	create_type_t	*typep;
	int		offset;
	int		size;
	int		len;
	int     ret;
	char    dirname[64];

	DBG_ENT(M_RDB,"sql_create_table");

    DBG_AF((sqlprint( SQLreturn )));

	tablep = sql_getnode( root, 0, TLEAF );
	table = tablep->e_tree.t_unary.v_value.v_str.s_str;

	if( strlen( table ) >= R_TABLE_NAME_MAX ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

        /*
         * テーブル名チェック
         */
	if( (td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, table)) ) {
		if( td->t_usr_cnt ) {
           	neo_errno = E_RDB_TBL_BUSY;
	        goto err1;
		} else {
			drop_td( td );
		}
	}
	for( item_num = 0, ep = root; 
		(ep = sql_getnode( root, ep, 'E')); item_num++ );

	if( !(itemsp = (r_item_t*)neo_zalloc(sizeof(r_item_t)*item_num))) {
		goto err1;
	}
	offset = sizeof(r_head_t);
	for( i = 0, ep = root; i < item_num; i++ ) {

		ep = sql_getnode( root, ep, 'E' );

		strncpy( (char*)&itemsp[i].i_name, 
		ep->e_tree.t_binary.b_l->e_tree.t_unary.v_value.v_str.s_str,
		sizeof(itemsp[i].i_name) );
		
		tp = sql_getnode( ep, 0, 'T' );

		typep = sql_getcreatetype(
		tp->e_tree.t_binary.b_l->e_tree.t_unary.v_value.v_str.s_str );
		if( !typep ) {
			goto err2;
		}
		if( typep->len && tp->e_tree.t_binary.b_r ) {
			len = tp->e_tree.t_binary.b_r->
					e_tree.t_unary.v_value.v_int;
		} else
			len = typep->size;

		size = (len+3)/4*4;

		strcpy( itemsp[i].i_type, typep->type );
		itemsp[i].i_attr	= typep->attr;
		itemsp[i].i_len		= len;
		itemsp[i].i_size	= size;
		itemsp[i].i_off		= offset;
    
		offset += size;
	}
        /*
         * テーブル(ファイル)クリエイト
         *  既にファイルが存在した場合は、新規に作り直される
         */
		strcpy( dirname, _svc_inf.f_path );
		strcat( dirname, "/" );
        strcat(dirname, table);
        strcat(dirname, ".bytes");
        if((ret = mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0 ){
			if( errno != EEXIST )	goto err2;
	    }
        
        if( TBL_CREATE( table, R_TBL_NORMAL, (long)time(0),
                        CREATE_REC_NUM, item_num, itemsp,
			0, 0 ) ) {
		    
/***
	        if( svc_alternate_table( table, item_num, itemsp ) )
***/
	            goto err2;
    
        }
LOG(neo_log,LogDBG,"table[%s]", table );

	neo_free( itemsp );

	DBG_EXIT( 0 );
	return( 0 );

err2:	neo_free( itemsp );
err1:
	DBG_EXIT(neo_errno);
LOG(neo_log,LogDBG,"errno=%d:neo_errno=%d", errno, neo_errno );
	return( neo_errno );
}

int
sql_drop_table( root )
	r_expr_t	*root;
{
	r_tbl_t		*td;
	r_expr_t	*tablep;
	char		*table;
	char        dirname[256];

	DBG_ENT(M_RDB, "sql_drop_table");

DBG_AF((sqlprint( SQLreturn )));

	tablep = sql_getnode( root, 0, TLEAF );
	table = tablep->e_tree.t_unary.v_value.v_str.s_str;

	if( strlen( table ) >= R_TABLE_NAME_MAX ) {
		neo_errno = E_RDB_INVAL;
		goto err1;
	}

        /*
         * テーブルチェック
         */
	if( (td = (r_tbl_t*)HashGet(&_svc_inf.f_HashOpen, table)) ) {
		if( td->t_usr_cnt ) {
           	neo_errno = E_RDB_TBL_BUSY;
	        goto err1;
		} else {
			drop_td( td );
		}
	}
        /*
         * テーブル(ファイル)削除
         */
		strcpy( dirname, _svc_inf.f_path );
		strcat( dirname, "/" );
        strcat(dirname, table);
        strcat(dirname, ".bytes");
		dir_delete( dirname );

        if( TBL_DROP( table ) ) {
                goto err1;
        }

	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_insert( pd, root, buffpp )
    p_id_t		*pd;
    r_expr_t	*root;
    char		**buffpp;
{
	r_expr_t	*wp;
	r_from_tables_t	t;
	r_tbl_t		*tdp;
	r_usr_t		*udp;
	r_expr_t	*cp;
	r_expr_t	*vp;
	r_expr_t	*cp1;
	r_expr_t	*vp1;
	char		*rp;
	r_req_insert_t	*insertReqp;
	r_res_insert_t	insertRes;
	int		i, j;
	r_item_t	*ip;
	int		find;
	int		werr;
    int     len;
    char    *buffp;
    char    *returnrecp = 0;
	char    tempfile[64];
	r_res_sql_update_t *ud;
    
	DBG_ENT(M_RDB, "sql_insert");

DBG_AF((sqlprint( SQLreturn )));
	/*
	 *	Open table
	 */
	wp = root->e_tree.t_binary.b_l;
	t.f_n = 1;

	if( (find = sql_find_user(pd,wp->e_tree.t_unary.v_value.v_str.s_str,
					0, &t.f_t[0] ) ) < 0 ) {

		if( sql_open( wp->e_tree.t_unary.v_value.v_str.s_str, 
						&t.f_t[0] ) ) {
			goto err1;
		}
	}

	tdp = t.f_t[0].f_tdp;
	udp = t.f_t[0].f_udp;

	/*
	 *prepare memory area
	 */
	
	if( !(buffp = (char*)sql_mem_malloc(
			len = (
			sizeof(r_res_sql_t)	/* response head */
			+ sizeof(int) /* whether have bytes field: 1 means have, 0 means no */
			+ sizeof(r_res_sql_update_t)
			+ sizeof(r_scm_t)
			+ sizeof(r_item_t)*(tdp->t_scm->s_n-1)
			+ tdp->t_rec_size )	/* the first found record */
        ))){
		goto err1;
	}
	((p_head_t*)buffp)->h_len = len - sizeof(p_head_t);
	
	
    *(int *)(buffp+sizeof(r_res_sql_t)) = 0; //0 means no bytes field
	ud	= (r_res_sql_update_t*)(buffp+sizeof(r_res_sql_t) + sizeof(int));
	bcopy( tdp->t_scm, (char*)(ud+1),
			(len = sizeof(r_scm_t)+sizeof(r_item_t)*(tdp->t_scm->s_n-1) ) );
	returnrecp = (char*)(ud+1) + len; 
	
	/*
	 *	Set data into request
	 */
	if( !(insertReqp = (r_req_insert_t*)neo_zalloc(
			sizeof(r_req_insert_t) + tdp->t_rec_size ) ) ) {
		goto err2;
	}
	rp = (char*)(insertReqp+1);

	cp1 = sql_getnode( root, wp, SVALUES );
	if( cp1 ) {
		vp1 = cp1->e_tree.t_binary.b_r;
		cp1 = cp1->e_tree.t_binary.b_l;
	} else {
		vp1 = root->e_tree.t_binary.b_r;
	}
	
	
	for( i = 0, cp = 0, vp = 0; (vp = sql_getnode(vp1,vp,TLEAF)); i++ ) {
		if( i >= tdp->t_scm->s_n ) {
			neo_errno = E_SQL_SCHEMA;
			goto err3;
		}
		if( cp1 ) {
			if( !(cp = sql_getnode(cp1,cp,TLEAF))) {
				goto err3;
			}
			for( j = 0; j < tdp->t_scm->s_n; j++ ) {
				ip = &tdp->t_scm->s_item[j];

				if( !strcmp( (char*)&ip->i_name,
					cp->e_tree.t_unary.
						v_value.v_str.s_str ) ){
					goto find;
				}
			}
			neo_errno = E_SQL_SCHEMA;
			goto err3;
		find:;
		} else {
			ip = &tdp->t_scm->s_item[i];

		}
		switch( ip->i_attr ) {
			case R_STR:
				if( vp->e_tree.t_unary.v_type != VSTRING_ALLOC
				&& vp->e_tree.t_unary.v_type != VSTRING ) {
					goto err3;
				}
				strncpy( rp+ip->i_off,
					vp->e_tree.t_unary.v_value.v_str.s_str,
					ip->i_len );
				break;
			case R_LONG:
				sql_cast_long( &vp->e_tree.t_unary/*.v_value*/ );
				bcopy( &vp->e_tree.t_unary.v_value.v_long,
					rp+ip->i_off, ip->i_len );
				break;
			case R_ULONG:
				sql_cast_ulong( &vp->e_tree.t_unary/*.v_value*/ );
				bcopy( &vp->e_tree.t_unary.v_value.v_ulong,
					rp+ip->i_off, ip->i_len );
				break;
			case R_INT:
				sql_cast_int( &vp->e_tree.t_unary/*.v_value*/ );
				bcopy( &vp->e_tree.t_unary.v_value.v_int,
					rp+ip->i_off, ip->i_len );
				break;
			case R_HEX:
			case R_UINT:
				sql_cast_uint( &vp->e_tree.t_unary/*.v_value*/ );
				bcopy( &vp->e_tree.t_unary.v_value.v_uint,
					rp+ip->i_off, ip->i_len );
				break;
			case R_FLOAT:
				sql_cast_float( &vp->e_tree.t_unary/*.v_value*/ );
				bcopy( &vp->e_tree.t_unary.v_value.v_float,
					rp+ip->i_off, ip->i_len );
				break;
			case R_BYTES:
                *(int *)(buffp+sizeof(r_res_sql_t)) = 1; //1 means having bytes field
			    if( vp->e_tree.t_unary.v_type != VSTRING_ALLOC
				&& vp->e_tree.t_unary.v_type != VSTRING ) {
					goto err3;
				}
				
				len = strlen(vp->e_tree.t_unary.v_value.v_str.s_str);
                if (len >= 4){
			        strncpy( rp + ip->i_off, 
				        vp->e_tree.t_unary.v_value.v_str.s_str,
				        4);
		        }
		strcpy( tempfile, _svc_inf.f_path );
		strcat( tempfile, "/" );
        strcat(tempfile, tdp->t_name);
                strcat(tempfile, ".bytes/bytes-XXXXXX");
				file_createtmp(tempfile);
				strcpy( rp+ip->i_off + 4, tempfile);
				break;
			default:
				DBG_A(("attr(0x%x)\n", ip->i_attr ));
				neo_errno = E_SQL_ATTR;
				goto err3;
		}
	}
	/*
	 *	Insert
	 */
	
	insertReqp->i_td = tdp;
	insertReqp->i_ud = udp;

	if( prc_insert( insertReqp, &insertRes ) ) {
		goto err3;
	}
    //if( returnrecp ) {
		bcopy( insertReqp + 1, returnrecp, tdp->t_rec_size );
        ud->u_updated = 1;
        ud->u_num = 1;
        ud->u_rec_size = tdp->t_rec_size;
    //}
    *buffpp	 = buffp;

	neo_free( insertReqp );
	/*
	 *	Regist usr
	 */
	if( sql_usr( pd, t.f_t[0].f_udp, find ) ) {
		goto err2;
	}

	DBG_EXIT( 0 );
	return( 0 );

err3:	neo_free( insertReqp );
err2:	
        sql_mem_free( buffp );
        werr=neo_errno;
        sql_close( &t.f_t[0] );
        neo_errno=werr;
err1:
    DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_delete( pd, root )
	p_id_t		*pd;
	r_expr_t	*root;
{
	r_from_tables_t	t;
	r_expr_t	*wp;
	int		find;
	int		werr;

	DBG_ENT(M_RDB, "sql_delete");

DBG_AF((sqlprint( SQLreturn )));

	wp = sql_getnode( root, 0, SWHERE );

	if( prc_init_from_tables( &t, 1 ) ) {
		goto err1;
	}

	if( (find = sql_find_user(pd, root->e_tree.t_binary.b_l->
			e_tree.t_unary.v_value.v_str.s_str,
					0, &t.f_t[0] ) ) < 0 ) {
		if( sql_open( root->e_tree.t_binary.b_l->
			e_tree.t_unary.v_value.v_str.s_str, &t.f_t[0] ) ) {
			goto err2;
		}
	}
	if( prc_delete( &t, wp ) ) {
		goto err3;
	}
	/*
	 *	Regist usr
	 */
	if( sql_usr( pd, t.f_t[0].f_udp, find ) ) {
		goto err3;
	}
	prc_destroy_from_tables( &t );

	DBG_EXIT( 0 );
	return( 0 );

err3:	werr=neo_errno;sql_close( &t.f_t[0] );neo_errno=werr;
err2:
	prc_destroy_from_tables( &t );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_update( pd, root, changedp, buffpp )
	p_id_t		*pd;
	r_expr_t	*root;
	int		*changedp;
	char		**buffpp;
{
	r_from_tables_t	t;
	r_expr_t	*sp;
	r_expr_t	*wp;
	int		updated;
	int		find;
	int		werr;

	DBG_ENT(M_RDB, "sql_update");


DBG_AF((sqlprint( SQLreturn )));

	/*
	 *	Open table
	 */
	if( prc_init_from_tables( &t, 1 ) ) {
		goto err1;
	}
	if( (find = sql_find_user(pd, root->e_tree.t_binary.b_l->
			e_tree.t_unary.v_value.v_str.s_str,
					0, &t.f_t[0] ) ) < 0 ) {
		if( sql_open( root->e_tree.t_binary.b_l->
			e_tree.t_unary.v_value.v_str.s_str, &t.f_t[0] ) ) {
			goto err2;
		}
	}
	/*
	 *	Adjust set
	 */
	sp = root;
	while( (sp = sql_getnode( root, sp, SSET )) ) {
		if( sql_vitem( t.f_t[0].f_tdp->t_scm,
				sp->e_tree.t_binary.b_l ) ) {
			goto err3;
		}
	}
	/*
	 *	Get where and adjust
	 */
	wp = sql_getnode( root, 0, SWHERE );

	if( wp && sql_create_expr( wp, &t ) ) {
		goto err3;
	}
	/*
	 *	Update table
	 */
	if( prc_update( &t, root, wp, 0, &updated, buffpp ) ) {
		goto err3;
	}

	*changedp += updated;
	/*
	 *	Regist usr
	 */
	if( sql_usr( pd, t.f_t[0].f_udp, find ) ) {
		goto err3;
	}
	prc_destroy_from_tables( &t );

	DBG_EXIT( 0 );
	return( 0 );

err3:	werr=neo_errno;sql_close( &t.f_t[0] );neo_errno=werr;
err2:
	prc_destroy_from_tables( &t );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_vitem( scmp, ep )
	r_scm_t		*scmp;
	r_expr_t	*ep;
{
	r_item_t	*ip, *vp;
	int		i;

	DBG_ENT(M_SQL, "sql_vitem");

	for( i = 0; i < scmp->s_n; i++ ) {
		ip = &scmp->s_item[i];
		if( !strcmp( (char*)&ip->i_name,
				ep->e_tree.t_unary.v_value.v_str.s_str ) ){
			goto find;
		}
	}
	neo_errno = E_RDB_NOEXIST;
	goto err1;
find:
	if( !(vp = (r_item_t*)sql_mem_zalloc( sizeof(r_item_t) )) ) {
		goto err1;
	}
	bcopy( ip, vp, sizeof(*vp) );

	if( ep->e_tree.t_unary.v_type == VSTRING_ALLOC ) {
		sql_mem_free( ep->e_tree.t_unary.v_value.v_str.s_str );
	}
	ep->e_tree.t_unary.v_type = VITEM_ALLOC;
	ep->e_tree.t_unary.v_value.v_item = vp;	

	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_set_item( ip, ep, rp )
	r_item_t	*ip;
	r_expr_t	*ep;
	char		*rp;
{
	r_value_t	v;
	int         len;

	DBG_ENT(M_SQL, "sql_set_item");

	switch( ip->i_attr ) {
		case R_STR:
			if( ep->e_tree.t_unary.v_type != VSTRING_ALLOC
				&& ep->e_tree.t_unary.v_type != VSTRING ) {

				neo_errno = E_RDB_INVAL;
				goto err1;
			}
			strncpy( rp + ip->i_off, 
				ep->e_tree.t_unary.v_value.v_str.s_str,
				ip->i_len );
			break;
		case R_BYTES:
		    if( ep->e_tree.t_unary.v_type != VSTRING_ALLOC
				&& ep->e_tree.t_unary.v_type != VSTRING ) {

				neo_errno = E_RDB_INVAL;
				goto err1;
			}
			len = strlen(ep->e_tree.t_unary.v_value.v_str.s_str);
            if (len >= 4){
			    strncpy( rp + ip->i_off, 
				    ep->e_tree.t_unary.v_value.v_str.s_str,
				    4);
		    }
			break;	
		default:
			sql_evaluate( ep, (r_head_t**)&rp, &v );
			bcopy( (char*)&v.v_value,
				rp + ip->i_off,
				ip->i_len );
			/***
			bcopy( (char*)&ep->e_tree.t_unary.v_value,
				rp + ip->i_off,
				ip->i_len );
			***/
			break;
	}

	DBG_EXIT( 0 );
	return( 0 );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_begin( pd, root )
	p_id_t		*pd;
	r_expr_t	*root;
{
	r_port_t	*pe;

	DBG_ENT(M_RDB,"sql_begin");

DBG_AF((sqlprint( SQLreturn )));

	if( (pe = PDTOENT( pd )) ) {
		if( pe->p_trn != 0 ) {
			sql_commit( pd, root );
		}
		pe->p_trn	= 1;
	}

	DBG_EXIT( 0 );
	return( 0 );
//err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_commit( pd, root )
	p_id_t		*pd;
	r_expr_t	*root;
{
	r_port_t	*pe;
	r_twait_t	*twaitp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	_dlnk_t		*dp;
	int		i;

	DBG_ENT(M_RDB,"sql_commit");

DBG_AF((sqlprint( SQLreturn )));
	/*
	 * ポートエントリがある場合には後処理をする
	 */
	if( (pe = PDTOENT( pd )) ) {

		/*
		 * 待ちテーブルがあれば、該当テーブルの
		 * 待ちユーザリストから該当ユーザを外して、領域を解放する
		 */
		if( (twaitp = pe->p_twait) ) {

			/* remove twait from td */
			td = twaitp->w_td;
			td->t_wait_cnt--;
LOG(neo_log, LogDBG, "t_name[%s]", td->t_name );
			_dl_remove( twaitp );

			/*  unlink usr from twaitp */
			while( (dp = _dl_head(&twaitp->w_usr)) )
				_dl_remove( dp );

DBG_A(("free twaitp=0x%x\n", twaitp ));
			neo_free( twaitp );
		}

		/*
		 * オープンユーザ
		 */
		for( dp = _dl_head(&pe->p_usr_ent);
			_dl_valid(&pe->p_usr_ent,dp);) {

			ud = PDTOUSR( dp );
			dp = _dl_next( dp );	/* may be removed under */
#ifdef XXX
		while( ud = PDTOUSR(_dl_head( &pe->p_usr_ent )) ) {
#endif

			td = ud->u_mytd;

			if( td->t_stat & T_TMP ) {
				;
			} else if( svc_store( td, 0, td->t_rec_num, ud ) ) {
				goto err1;
			}
			/*
			 * レコードについての後処理
			 */
			for( i = 0; i < td->t_rec_num; i++ ) {

				if( td->t_rec[i].r_access != ud )
					continue;

			}
			/*
			 *	Set modified time
			 */
			td->t_stat	|= R_UPDATE;
			td->t_modified = time(0);
			td->t_version++;

DBG_A(("close_user(ud=0x%x)\n", ud ));
			if( !ud->u_td ) {
				if( !(td->t_stat & T_TMP) )
					close_user( ud );
/***
				if( td->t_stat & T_TMP ) drop_td( td );
***/
			}
#ifdef	ZZZ
#else
			else if( ud->u_refer == 0 ) {
				close_user( ud );
			}
#endif
		}
DBG_A(("free pe=0x%x\n", pe ));
		
		pe->p_trn	= 0;
	}

	DBG_EXIT( 0 );
	return( 0 );
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
sql_rollback( pd, root )
	p_id_t		*pd;
	r_expr_t	*root;
{
	r_port_t	*pe;
	r_twait_t	*twaitp;
	r_tbl_t		*td;
	r_usr_t		*ud;
	_dlnk_t		*dp;

	DBG_ENT(M_RDB,"sql_rollback");

DBG_AF((sqlprint( SQLreturn )));

	if( (pe = PDTOENT( pd )) ) {

		/*
		 * 待ちテーブルがあれば、該当テーブルの
		 * 待ちユーザリストから該当ユーザを外して、領域を解放する
		 */
		if( (twaitp = pe->p_twait) ) {

			/* remove twait from td */
			td = twaitp->w_td;
			td->t_wait_cnt--;

			_dl_remove( twaitp );

			/*  unlink usr from twaitp */
			while( (dp = _dl_head(&twaitp->w_usr)) )
				_dl_remove( dp );

DBG_A(("free twaitp=0x%x\n", twaitp ));
			neo_free( twaitp );
		}

		/*
		 * オープンユーザ
		 */
		for( dp = _dl_head(&pe->p_usr_ent);
			_dl_valid(&pe->p_usr_ent,dp);) {

			ud = PDTOUSR( dp );
			dp = _dl_next( dp );	/* may be removed under */

			td = ud->u_mytd;

			if( td->t_stat & T_TMP )
				goto skip;
			svc_rollback_td( td, ud );
skip:
			ud->u_added = 0;
			/*
			 *	Rollback is recognized as modification
			 */
			td->t_modified = time(0);
			td->t_version++;

			if( !ud->u_td ) {
				if( !(td->t_stat & T_TMP) )
					close_user( ud );
/***
				if( td->t_stat & T_TMP ) drop_td( td );
***/
			}
#ifdef	ZZZ
#else
			else if( ud->u_refer == 0 ) {
				close_user( ud );
			}
#endif
		}
		pe->p_trn	= 0;
	}	 

	DBG_EXIT( 0 );
	return( 0 );
/***
err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
***/
}

int
sql_usr( pd, ud, notfind )
	p_id_t	*pd;
	r_usr_t	*ud;
	int	notfind;
{
	r_port_t *pe;

	DBG_ENT(M_RDB,"sql_usr");

	if( !(pe = PDTOENT( pd ) ) ) {
		goto err1;
	}

	if( notfind ) {
		/*
		 * 該当オープンユーザをポートエントリに登録する
		 */
		_dl_insert( &ud->u_port, &(pe->p_usr_ent) );

		/*
		 * 該当オープンユーザの情報を設定する
		 */
		ud->u_pd	= pd;
	}

	/* Implicit begin */
	pe->p_trn	= 1;

	DBG_EXIT( 0 );
	return( 0 );
err1:
	DBG_EXIT(-1);
	return(-1);
}

