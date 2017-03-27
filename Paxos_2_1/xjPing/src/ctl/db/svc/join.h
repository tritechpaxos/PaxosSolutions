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

#ifndef	_JOIN_H
#define	_JOIN_H

#include	"neo_db.h"

#define	R_VIEW_SCHEME_SUFFIX	".scm"

typedef struct r_join_item {
	int		j_ind;
	r_item_t	j_item;
} r_join_item_t;

typedef struct r_view_scm {
	r_expr_t	*v_root;	/* free pointer */
} r_view_scm_t;

typedef struct r_from_table {
	r_tbl_t	*f_tdp;
	r_usr_t	*f_udp;
	char	f_name[R_TABLE_NAME_MAX];
} r_from_table_t;

typedef struct r_from_tables {
	r_tbl_t		*f_tdp;
	r_head_t	**f_recpp;
	r_expr_t	*f_selectp;
	r_expr_t	*f_wherep;
	int		f_n;
	int		f_num;
	int		f_result_size;
	int		*f_resultp;
	int		*f_joinp;
	r_from_table_t	f_t[1];
} r_from_tables_t;

extern	int		prc_join_loop( int, r_from_tables_t* );

extern	int 	prc_view_file( char* );
extern	int		sql_create_view( r_expr_t*, char* );
extern	int		sql_drop_view( r_expr_t* );
extern	r_view_scm_t* prc_view_scm_alloc( r_tbl_name_t );
extern	int		prc_view_update( r_tbl_t* );
extern	int		prc_view_file( char* );
extern	int		prc_view_create( r_tbl_name_t, r_expr_t* );
extern	r_tbl_t*	prc_view_concatinate( r_tbl_t*, r_tbl_t* );

extern	int		prc_init_from_tables( r_from_tables_t *tblp, int n );
extern	void	prc_destroy_from_tables( r_from_tables_t *tblp );
extern	r_from_tables_t* prc_open_from_tables( r_expr_t* );
extern	int		prc_close_from_tables( r_from_tables_t*, int );
extern	int 	prc_view_scm_free( r_view_scm_t* );
extern	r_tbl_t* prc_join( r_expr_t*, r_expr_t*, r_expr_t*, r_expr_t* );
extern	int		prc_join_loop( int, r_from_tables_t* );
extern	int		prc_join_evaluate( r_from_tables_t*, int[], r_expr_t* );

extern	r_tbl_t*	prc_get_td( char* );
extern	int 	prc_open( r_req_open_t*, r_res_open_t**, int*, int );
extern	int 	prc_close( r_req_close_t*, r_res_close_t*, int );

extern	int		prc_sql( p_id_t*, r_req_sql_t*, char* );
extern	int		prc_sql_params( p_id_t*, r_req_sql_params_t*, char* );
extern	int		sql_find_user( p_id_t*, char*, r_tbl_t*, r_from_table_t* );
extern	int		sql_open( char*, r_from_table_t* );
extern	int		sql_close( r_from_table_t* );
extern	int		sql_select( p_id_t*, r_req_sql_t*, r_expr_t* );
extern	int		sql_create_table( r_expr_t* );
extern	int		sql_drop_table( r_expr_t* );
extern	int		sql_insert( p_id_t*, r_expr_t*, char** );
extern	int		sql_delete( p_id_t*, r_expr_t* );
extern	int		sql_update( p_id_t*, r_expr_t*, int*, char** );
extern	int		sql_vitem( r_scm_t*, r_expr_t* );
extern	int		sql_set_item( r_item_t*, r_expr_t*, char* );
extern	int		sql_begin( p_id_t*, r_expr_t* );
extern	int		sql_commit( p_id_t*, r_expr_t* );
extern	int		sql_rollback( p_id_t*, r_expr_t* );
extern	int		sql_usr( p_id_t*, r_usr_t*, int );

extern	int		svc_file( p_id_t*, p_head_t* );
extern	int 	prc_file( p_id_t*, p_head_t*, r_file_t* );
extern	int 	file_save( p_id_t*, p_head_t*, r_file_t* );
extern	int 	file_send( p_id_t*, p_head_t*, r_file_t* );
extern	int		file_delete( char* );
extern	int		file_createtmp( char* );
extern	int		dir_delete( char* );

extern	int		prc_insert( r_req_insert_t*, r_res_insert_t* );

extern	int		prc_update( r_from_tables_t*, r_expr_t*, r_expr_t*, 
						int, int*, char** );

extern	int		prc_delete( r_from_tables_t*, r_expr_t* );

extern	int		prc_view_sort( r_tbl_t*, r_expr_t* );

extern	int		prc_search( r_req_search_t*, r_res_search_t* );

extern	int prc_find( r_req_find_t*, r_res_find_t*, int* );
extern	int prc_rewind( r_req_rewind_t*, r_res_rewind_t* );

//extern	int		xj_swap_init();
//extern	void 	xj_swap_term();
//extern	int		xj_swap_out( char*, int, xj_swap_handle_t* );
//extern	char*	xj_swap_in( xj_swap_handle_t*, int* );
//extern	int		xj_swap_find_free( r_tbl_t*, int );
extern	int		xj_swap_schedule( r_tbl_t* );
extern	int		xj_swap_in_td( r_tbl_t*, int, int );

#endif
