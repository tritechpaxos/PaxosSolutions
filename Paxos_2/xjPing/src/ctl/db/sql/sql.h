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

#ifndef	_SQL_H_
#define	_SQL_H_

#include	"y.tab.h"
#include	"../svc/join.h"

extern	int		sql_mem_init();
extern	void	sql_mem_free(void*);
extern	void    *sql_mem_malloc(size_t);
extern	void    *sql_mem_zalloc(size_t);

extern	int		sqlparse();
extern	void	sql_lex_init();
extern	void	sql_lex_start( const char*, char*, int );
extern	void	sql_lex_pstat_start( const char*, char*, int );
extern	int		sqllex();
extern	void	sqlerror( char* );
extern	int		sqlkey(char*);
extern	void	sql_free( r_expr_t* );
extern	void	sql_free1( r_expr_t* );
extern	int		sql_write_tree( int, r_expr_t* );
extern	r_expr_t* sql_read_tree( int );

extern	int		sql_like( char*, int, char*, int, char*, int* );

extern	int		sql_create_select( r_expr_t*, r_from_tables_t*, r_expr_t* );
extern	int		sql_create_expr( r_expr_t*, r_from_tables_t* );
extern	int		sql_eval_expr( r_expr_t*, r_head_t*[] );
extern	int		sql_evaluate( r_expr_t*, r_head_t*[], r_value_t* );
extern	int		sql_item_value( r_item_t*, char*, r_value_t* );
extern	void	sql_cast( r_value_t*, r_value_t* );
extern	void	sql_cast_float( r_value_t* );
extern	void	sql_cast_ulong( r_value_t* );
extern	void	sql_cast_long( r_value_t* );
extern	void	sql_cast_uint( r_value_t* );
extern	void	sql_cast_int( r_value_t* );


extern	int 		SqlExecute( r_man_t*, char* );
extern	r_tbl_t*	SqlQuery( r_man_t*, char* );
extern	r_tbl_t*	SqlUpdate( r_man_t*, char* );
extern	r_tbl_t*	SqlResSql( p_id_t*, p_head_t* );
extern	r_tbl_t*	SqlResOpen( p_id_t*, p_head_t* );
extern	int			SqlSelect( p_id_t*, r_tbl_t* );
extern	void		SqlResultClose( r_tbl_t* );
extern	int SqlResultFirstOpen( r_man_t *mdp, r_tbl_t **tdpp );
extern	int SqlResultNextOpen( r_man_t *mdp, r_tbl_t **tdpp );

extern int sql_prepare( r_man_t *md, const char *statement, r_stmt_t **pstmt );
extern int sql_statement_execute( r_stmt_t *stmt );
extern int sql_bind_int( r_stmt_t *stmt, int pos, int val );
extern int sql_bind_uint( r_stmt_t *stmt, int pos, unsigned int val );
extern int sql_bind_long( r_stmt_t *stmt, int pos, long val );
extern int sql_bind_ulong( r_stmt_t *stmt, int pos, unsigned long val );
extern int sql_bind_double( r_stmt_t *stmt, int pos, double val );
extern int sql_bind_str( r_stmt_t *stmt, int pos, const char *val );
extern int sql_statement_reset( r_stmt_t *stmt );
extern int sql_statement_close( r_stmt_t *stmt );


#endif
