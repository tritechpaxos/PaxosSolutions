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

#ifndef	_HASH_H
#define	_HASH_H

#include	"join.h"

/* 1009, 2003, 4003, 5003, 10007, 50021, 100003 */
//#define	HASH_SIZE	PRIMARY_2003

#define	HASH_SIZE_1	PRIMARY_1009
#define	HASH_SIZE_2	PRIMARY_101
#define	HASH_LENGTH	200

typedef struct h_elm {
	int			e_ind;		/* record index */
	uint64_t	e_lValue;	/* hash value( not hash index ) */
} h_elm_t;

struct h_head;
typedef struct h_entry {
	int				ent_cnt;
	int				ent_size;
	h_elm_t			*ent_array;	/* array */
	struct h_head	*ent_hash;
} h_entry_t;

typedef struct h_head {
	r_item_name_t	h_name;
	int				h_cnt;	/* used element */
	int				h_max;
	int				h_active;
	int				h_hash_size;
	h_entry_t		h_value[1];
} h_head_t;

typedef	struct h_equal {
	_dlnk_t		eq_lnk;
	int			eq_left;
	h_head_t	*eq_left_hp;
	r_value_t	eq_left_value;
	int			eq_right;
	h_head_t	*eq_right_hp;
	r_value_t	eq_right_value;
} h_equal_t;

typedef struct h_cntl {
	_dlnk_t		c_lnk;

	r_from_tables_t	*c_from;
} h_cntl_t;

extern	int hash_add( r_tbl_t *tdp, r_item_t *itemp, int ind );
extern	int hash_del( r_tbl_t *tdp, r_item_t *itemp, int ind );
extern	void	hash_destroy( Hash_t* pHash, void* pValue );

extern	void	hash_head_free( h_head_t* );
extern	void	hash_free( h_cntl_t* );
extern	int		hash_union( h_cntl_t*, h_head_t* );
extern	int		xj_join_by_hash( r_from_tables_t* );

#endif
