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

/******************************************************************************
 *
 *
 *	neo_db.h
 *
 *	説明	ヘッダファイル
 *
 ******************************************************************************/
#ifndef	_NEO_DB_
#define	_NEO_DB_

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	"neo_system.h"
#include	"neo_link.h"
#include	"neo_usradm.h"
#include	"neo_db_err.h"

/*
 *	name length
 */
#define	R_PATH_NAME_MAX		NEO_MAXPATHLEN	/* DB path 名 最大長 */
#define	R_MAN_NAME_MAX		NEO_NAME_MAX	/* データベース名最大長 */
#define	R_TABLE_NAME_MAX	128	/*NEO_NAME_MAX	 テーブル名最大長 */
#define	R_ITEM_NAME_MAX		128	/*NEO_NAME_MAX	 アイテム名最大長 */

typedef	char	r_path_name_t[R_PATH_NAME_MAX];	/* DB_MAN path 名 */
typedef	char	r_man_name_t[R_MAN_NAME_MAX];	/* DB_MAN データベース名 */
typedef	char	r_tbl_name_t[R_TABLE_NAME_MAX];	/* DB_MAN テーブル名 */
typedef	char	r_item_name_t[R_ITEM_NAME_MAX];	/* DB_MAN アイテム名 */

/*
 *	DB_MAN descriptor for client
 */
#define	R_MAN_IDENT	0x64626d6eL	/* 'dbmn' */

typedef struct r_man {
	_dlnk_t		r_lnk;		/* リンク */
	long		r_ident;	/* 'dbmn'識別子 */
	r_man_name_t	r_name;		/* DB_MAN 名 */
	p_id_t		*r_pd;		/* port descriptor */
	_dlnk_t		r_tbl;		/* open table list */
	_dlnk_t		r_result;	/* ResultSet for client */
	_dlnk_t		*r_result_next;
	_dlnk_t		r_stmt;
	usr_adm_t	r_MyId;
	int			r_Seq;
} r_man_t;


/*
 *	DB Table
 */
/*
 *	item
 *		Item should be aligned to 4-bytes boundary in record.
 */
/* attribute */
#define		R_HEX		1	/* hex type */
#define		R_INT		2	/* integer type */
#define		R_UINT		3	/* unsigned integer type */
#define		R_FLOAT		4	/* float type */
#define		R_STR		5	/* string type */
#define		R_LONG		6	/* long long type */
#define		R_ULONG		7	/* unsigned long long type */
//#define	R_DATE	    8	/* date type */
#define		R_BYTES		9	/* bytes type */

typedef struct r_item {
	r_item_name_t	i_name;		/* item name */
	char		i_type[NEO_NAME_MAX];	/* external representation */
	int		i_ind;
	int		i_attr;		/* attribute */
	int		i_len;		/* effective item length(bytes) */
	int		i_size;		/* item maximum length(bytes) */
	int		i_off;		/* offset in record */
	int		i_flag;		/* reserved for future use */
} r_item_t;

/*
 *	scheme
 */
typedef struct r_scm {
	int		s_n;		/* number of items */
	r_item_t	s_item[1];	/* iteme array */
} r_scm_t;

/*
 *	record
 */
/* flags  somes are used in table(t_stat) */
#define	R_INSERT	0x0001	/* insert record	*/
#define	R_UPDATE	0x0002	/* update record	*/
#define	R_USED		0x0004	/* used record		*/
#define	R_BUSY		0x0008	/* 変更されたがファイルに反映されていない状態 */

#define	T_TMP		0x1000	/* td is temporaly used */
#define	T_CLOSED	0x2000	/* td is already closed */

#define	R_DIRTY		(R_INSERT|R_UPDATE)

#define	R_WRITE		0x01	/* サーバ内部の排他チェックで使用 */
#define	R_READ		0x02	/* サーバ内部の排他チェックで使用 */

#define	R_EXCLUSIVE	0x0100	/* 参照不可, 更新不可 */
#define	R_SHARE		0x0200	/* 参照可,   更新不可 */

#define	R_LOCK		(R_EXCLUSIVE|R_SHARE)

#define	R_WAIT		0x0000	/* デフォルトは待ちを行う	*/
#define	R_NOWAIT	0x0400	/* 待ちを行わない		*/

/* cache buffer control flags(但し,セグメント管理は現在行っていない) */
#define	R_REC_FREE	0x00010000L	/* buff free pointer	*/
#define	R_REC_BUF	0x00020000L	/* buff attached	*/
#define	R_REC_CACHE	0x00040000L	/* data cached		*/
#define	R_REC_SWAP	0x00080000L	/* swap in buff		*/

#define	R_REC_MASK	(R_REC_FREE|R_REC_BUF|R_REC_CACHE|R_REC_SWAP)

/*
 *   record header
 */
typedef struct r_head {
	int64_t		r_rel;		/* relative number */
	int64_t		r_abs;		/* absolute number */
	int64_t		r_stat;		/* record status */
	uint64_t	r_Cksum64;	/* checksum */
} r_head_t;

/*
 *   record control
 */
typedef struct r_rec {
	int			r_stat;		/* record status */
	r_head_t	*r_head;	/* record buffer */
	struct r_usr	*r_access;	/* access/lock user */
	struct r_usr	*r_wait;	/* first wait user */
} r_rec_t;

/*
 *	table
 */
#define	R_TBL_IDENT	0x7461626cL	/* 'tabl' */
#define	R_TBL_REC_MIN	1		/* minimum record number in one table */
#define	R_ALL_REC	0		/* rdb_number()'s parameter total rec */
#define	R_USED_REC	1		/* rdb_number()'s parameter used rec  */
#define	R_UNUSED_REC	2		/* rdb_number()'s parameter unused rec*/

/* table type */
#define	R_TBL_NORMAL	0		/* normal */
#define	R_TBL_VIEW	1		/* view which has table dependency*/

typedef struct r_tbl_mod {
	r_tbl_name_t	m_name;
	long		m_time;
	int			m_version;
} r_tbl_mod_t;

typedef struct r_tbl_list {
	int		l_n;
	r_tbl_mod_t	l_tbl[1];
} r_tbl_list_t;

typedef struct r_tbl {
	_dlnk_t		t_lnk;
	long		t_ident;	/* 'tabl' */
	r_tbl_name_t	t_name;		/* table name */
	int		t_stat;		/* table status */
	int		t_version;
	r_man_t		*t_md;		/* r_man pointer(client) */
	int		t_usr_cnt;	/* number of users */
	_dlnk_t		t_usr;		/* open user list */
	struct r_usr	*t_lock_usr;	/* lock user */
	int		t_wait_cnt;	/* number of wait user */
	_dlnk_t		t_wait;		/* wait user list */
	struct r_tbl	*t_rtd;		/* remote table(client) */
	struct r_usr	*t_rud;		/* remote open user pointer(client) */
	r_scm_t		*t_scm;		/* schema */
	int		t_rec_size;	/* record size */
	int		t_rec_num;	/* total number of records */
	int		t_rec_used;	/* number of current used records */
	int		t_rec_ori_used;	/* number of original used records */
	int		t_unused_index;	/* may be minimum unused record index */
	r_rec_t		*t_rec;		/* record control array(server)/r_head_t*(client)*/
	void		*t_tag;		/* work */
	long		t_access;	/* last access time */

	long		t_modified;	/* last modified time */
	int		t_type;		/* table type */
	r_tbl_list_t	*t_depend;	/* table dependency */
	Hash_t		t_HashEqual;
	Hash_t		t_HashRecordLock;
} r_tbl_t;

#define	TDTOPD( td )	((td)->t_md->r_pd)


/*
 *	table exclusive control data
 */
#define	R_TBL_HOLD_MAX	100	/* 一度に排他を宣言できるテーブル数 */

typedef struct r_hold {
	r_tbl_name_t	h_name;
	r_tbl_t		*h_td;	/* テーブル記述子 */
	int		h_mode;	/* 排他モード */
} r_hold_t;


/*
 *	sort condition
 */
#define	R_ASCENDANT	1		/* 昇順 */
#define	R_DESCENDANT	2		/* 降順 */

typedef struct r_sort {
	r_item_name_t	s_name;		/* アイテム名 */
	int		s_order;	/* 順番 */
} r_sort_t;



/*
 *	search condition
 */
#define	R_EXPR_MAX	50	/* max expression node */
#define	R_TEXT_MAX	256	/* max length of condition string */

typedef struct r_value {

	int	v_type;	     /* value type(VSTRING or VBIN or VITEM or VBOOL) */

	union {
		r_item_t		*v_item;/* item (VITEM) */
		int			v_int;	/* binary (VBIN or VBOOL) */
		unsigned int		v_uint;	/* R_UINT */
		long long		v_long;	/* R_LONG */
		unsigned long long	v_ulong;/* R_ULONG */
		float			v_float;/* R_FLOAT */
		struct {
			int	s_len;
			char	*s_str;
		} v_str;			/* string (VSTRING) */
	} v_value;

} r_value_t;


typedef struct r_expr {

	int		e_op;		/* operation or TLEAF */

	union {
		r_value_t	t_unary; /* leaf value (when e_op's TLEAF) */
		struct {
		  struct r_expr	*b_l;	/* left  tree (when e_op's operation) */
		  struct r_expr	*b_r;	/* right tree (when e_op's operation) */
		} t_binary;
	} e_tree;
#define	e_left	e_tree.t_binary.b_l
#define	e_right	e_tree.t_binary.b_r
#define	e_value	e_tree.t_unary

} r_expr_t;


typedef struct r_search {
	r_expr_t	*s_expr;	/* expression root */
	int		s_max;		/* max expression struct(R_EXPR_MAX) */
	int		s_cnt;		/* used number */
	r_expr_t	s_elm[R_EXPR_MAX];  /* expression */
	char		s_text[R_TEXT_MAX]; /* string constant buffer */
} r_search_t;

/*
 *	open user structure
 */
#define	R_USR_IDENT	0x75736572L	/* 'user' */

#define	R_USR_RWAIT	0x01		/* wait record lock */

typedef struct r_usr {
	_dlnk_t			u_tbl;		/* table link */
	_dlnk_t			u_port;		/* port link */
	_dlnk_t			u_twait;	/* table wait link */
	_dlnk_t			u_rwait;	/* record wait link */
	long			u_ident;	/* 'user' 識別子 */
	int			u_stat;		/* status */
	p_id_t			*u_pd;		/* port descriptor */
	p_head_t		u_head;		/* wait packet header */
	r_tbl_t			*u_mytd;	/* table linked */
	r_tbl_t			*u_td;		/* client table */
	r_search_t		u_search;	/* search condition */
	int				u_cursor;	/* cursor */
	int				u_mode;		/* exclusive mode */
	int				u_index;	/* wait record index */
	int				u_added;	/* numbers of added records */

	int				u_insert;
	int				u_update;
	int				u_delete;
	struct r_usr	*u_Id;		/* for Unmarshal */
	int				u_refer;	/* reference counter */
} r_usr_t;

#define	PDTOUSR( p )	((p)?(r_usr_t*)((_dlnk_t*)(p)-1):(r_usr_t*)(p))
#define	TWTOUSR( p )	((p)?(r_usr_t*)((_dlnk_t*)(p)-2):(r_usr_t*)(p))
#define	RWTOUSR( p )	((p)?(r_usr_t*)((_dlnk_t*)(p)-3):(r_usr_t*)(p))

#define	UDTOPD( p )	((p)->u_pd)


/*
 *	functions
 */
r_tbl_t	*_r_tbl_alloc(void);


/*----------------------------------------------------------------------------*/

/* 
 *	server system information (DB_MAN server only use)
 */
#define	R_SERVER_ACTIVE 	0	/* サーバ活動中	*/
#define	R_SERVER_STOP	 	1	/* サーバ停止中	*/

typedef struct r_svc_inf {
	_dlnk_t		f_opn_tbl;	/* open table list  */
	int		f_stat;		/* server status    */
	r_path_name_t	f_path;		/* db-path name	    */
	r_path_name_t	f_root;		/* current directory*/
	_dlnk_t		*f_port;	/* port list */
	Mtx_t		f_Mtx;
	Hash_t		f_HashOpen;
	NHeapPool_t	*f_pPool;
	bool_t		f_bCksum;
	void		*f_pFileCache;
} r_svc_inf_t;


/* 
 *	dump server information (for dump command)
 */
typedef struct r_inf {
	char		*i_root;	/* &_svc_inf */
	r_svc_inf_t	i_svc_inf;	/* _svc_inf  */
} r_inf_t;


/*============================================================================*/
//#ifdef	DB_MAN_SYSTEM	/* DB_MAN システム用定義 */


/*
 *	sort index
 */
typedef struct r_sort_index {
	int		i_id;
	r_head_t	*i_head;
} r_sort_index_t;


/*
 *	table wait
 */
typedef struct r_twait {
	_dlnk_t			w_tbl;	/* table link */
	_dlnk_t			w_usr;	/* user link */
	r_tbl_t			*w_td;
	p_id_t			*w_pd;
	p_head_t		w_head;
	struct r_twait*	w_Id;
} r_twait_t;


/*
 *	port entry
 *		This entry is allocated to p_id_t.i_tag.
 */
typedef struct r_port {
	_dlnk_t		p_usr_ent;	/* entry */
	r_twait_t	*p_twait;
	usr_adm_t	p_clnt;		/* client information */
	int			p_trn;		/* transaction mode(0/1) */
	Hash_t		p_HashUser;
} r_port_t;

#define	PDTOENT( pd )	((r_port_t*)(pd)->i_tag)
#define	CLNTTOPORT( p )	((r_port_t*)((char*)(p) - (int)&((r_port_t*)0)->p_clnt))


/*
 *	Protocol
 */

#define	P_F_RDB		2
#define	R_VER0		0

#define	R_CMD_LINK			1
#define	R_CMD_UNLINK		2
#define	R_CMD_CREATE		3
#define	R_CMD_DROP			4
#define	R_CMD_OPEN			5
#define	R_CMD_CLOSE			6
#define	R_CMD_FLUSH			7
#define	R_CMD_CANCEL		8
#define	R_CMD_INSERT		9
#define	R_CMD_DELETE		10
#define	R_CMD_UPDATE		11
#define	R_CMD_SEARCH		12
#define	R_CMD_FIND			13
#define	R_CMD_REWIND		14
#define	R_CMD_SORT			15
#define	R_CMD_HOLD			16
#define	R_CMD_RELEASE		17
#define	R_CMD_HOLD_CANCEL	18
#define	R_CMD_COMPRESS		19
#define	R_CMD_EVENT			20
#define	R_CMD_EVENT_CANCEL	21
#define	R_CMD_DIRECT		22
#define	R_CMD_REC_NUMS		23
#define	R_CMD_STOP			24
#define	R_CMD_RESTART		25
#define	R_CMD_LIST			26

#define	R_CMD_SEEK			27
#define	R_CMD_SHUTDOWN		28

#define	R_CMD_DUMP_PROC		30
#define	R_CMD_DUMP_INF		31
#define	R_CMD_DUMP_RECORD	32
#define	R_CMD_DUMP_TBL		33
#define	R_CMD_DUMP_USR		34
#define	R_CMD_DUMP_MEM		35
#define	R_CMD_DUMP_CLIENT	36

#define R_CMD_SQL			37
#define R_CMD_SQL_DONE		38
#define R_CMD_FILE		    39

#define	R_CMD_DUMP_LOCK		40
#define	R_CMD_DUMP_UNLOCK	41

#define	R_CMD_COMMIT		42
#define	R_CMD_ROLLBACK		43

#define	R_CMD_RAS			44
#define	R_CMD_CLOSE_CLIENT	45

#define R_CMD_SQL_PARAMS	46

/*
 *	commit/rollback
 */
typedef struct r_req_commit {
	p_head_t	c_head;
} r_req_commit_t;

typedef struct r_res_commit {
	p_head_t	c_head;
} r_res_commit_t;

typedef struct r_req_rollback {
	p_head_t	r_head;
} r_req_rollback_t;

typedef struct r_res_rollback {
	p_head_t	r_head;
} r_res_rollback_t;

/*
 * link/unlink
 */
typedef struct r_req_link {
	p_head_t	l_head;
	usr_adm_t	l_usr;
} r_req_link_t;

typedef struct r_res_link {
	p_head_t	l_head;
	usr_adm_t	l_usr;
} r_res_link_t;

typedef struct r_req_unlink {
	p_head_t	u_head;
} r_req_unlink_t;

typedef struct r_res_unlink {
	p_head_t	u_head;
} r_res_unlink_t;

/*
 *	table create/drop
 */
typedef struct r_req_create {
	p_head_t	c_head;
	r_tbl_name_t	c_name;		/* table name */
	r_tbl_t		*c_td;		/* client table */
	int		c_rec_num;	/* total record number */
	int		c_item_num;	/* number of items */
	r_item_t	c_item[1];	/* item list */
} r_req_create_t;

typedef struct r_res_create {
	p_head_t	c_head;
	r_tbl_t		*c_td;		/* remote table */
	r_usr_t		*c_ud;		/* remote user table */
	int		c_rec_num;
	int		c_rec_size;
	int		c_item_num;
} r_res_create_t;
/* followed by r_item_t */

typedef struct r_req_drop {
	p_head_t	d_head;
	r_tbl_name_t	d_name;
} r_req_drop_t;

typedef struct r_res_drop {
	p_head_t	d_head;
} r_res_drop_t;

/*
 *	table open/close
 */
typedef struct r_req_open {
	p_head_t	o_head;
	r_tbl_name_t	o_name;
	r_tbl_t		*o_td;			// client td
} r_req_open_t;

typedef struct r_res_open {
	p_head_t	o_head;
	r_tbl_t		*o_td;	// work
	r_usr_t		*o_ud;	// work
	r_tbl_name_t	o_name;	// for SQL
	int		o_rec_num;
	int		o_rec_used;
	int		o_rec_size;
	int		o_n;
	r_item_t	o_items[1];
} r_res_open_t;

typedef struct r_req_close {
	p_head_t	c_head;
	r_tbl_name_t	c_name;
	r_tbl_t		*c_td;
	r_usr_t		*c_ud;
} r_req_close_t;

typedef struct r_res_close {
	p_head_t	c_head;
} r_res_close_t;

typedef struct r_req_close_client {
	p_head_t	c_head;
	r_tbl_name_t	c_name;
	r_tbl_t		*c_td;
	r_usr_t		*c_ud;
} r_req_close_client_t;

typedef struct r_res_close_client {
	p_head_t	c_head;
} r_res_close_client_t;

/*
 *	flush/cancel
 */
typedef struct r_req_flush {
	p_head_t	f_head;
	r_tbl_name_t	f_name;
} r_req_flush_t;

typedef struct r_res_flush {
	p_head_t	f_head;
} r_res_flush_t;

typedef struct r_req_cancel {
	p_head_t	c_head;
	r_tbl_name_t	c_name;
} r_req_cancel_t;

typedef struct r_res_cancel {
	p_head_t	c_head;
} r_res_cancel_t;

/*
 *	record insert/delete/update
 */
typedef struct r_req_insert {
	p_head_t	i_head;
	r_tbl_name_t	i_name;
	r_tbl_t		*i_td;
	r_usr_t		*i_ud;
	int		i_mode;
	int		i_rec;
	int		i_item;
} r_req_insert_t;
/* followed by record(r_head_t+data),
		r_item_name_t		*/

typedef struct r_res_insert {
	p_head_t	i_head;
	int		i_rec_num;
	int		i_rec_used;
	r_head_t	i_rec;
} r_res_insert_t;

typedef struct r_req_delete {
	p_head_t	d_head;
	r_tbl_name_t	d_name;
//	r_head_t	d_rec;
} r_req_delete_t;
/* followed by record(r_head_t+data) */

typedef struct r_res_delete {
	p_head_t	d_head;
	int		d_rec_used;
} r_res_delete_t;

typedef struct r_req_update {
	p_head_t	u_head;
	r_tbl_name_t	u_name;
	int		u_mode;
	int		u_numbers;
} r_req_update_t;
/* followed by record(r_head_t+data) */

typedef struct r_res_update {
	p_head_t	u_head;
	r_head_t	u_rec;
} r_res_update_t;

/*
 *	set search condition
 */
typedef struct r_req_search {
	p_head_t	s_head;
	r_tbl_name_t	s_name;
	r_tbl_t		*s_td;
	r_usr_t		*s_ud;
	int		s_n;
} r_req_search_t;
/* followed by search string */

typedef struct r_res_search {
	p_head_t	s_head;
} r_res_search_t;

/*
 *	find/rewind/seek
 */
typedef struct r_req_find {
	p_head_t	f_head;
	r_tbl_name_t	f_name;
	r_tbl_t		*f_td;
	r_usr_t		*f_ud;
	int		f_num;		/* request number of records */
	int		f_mode;		/* access mode */
} r_req_find_t;

typedef struct r_res_find {
	p_head_t	f_head;
	int     bytesflag;//0 mean no bytes data, 2 means have bytes data to send to JDBC
	int		f_num;		/* number of records after this head */
	int		f_more;
} r_res_find_t;
/* followed by records */

/* REMARKS:
	 When the client requests records by n, the server tries to reply 
	n records as possible. Tere are three cases.
		1) The server can reply n records as request.
			 Even though there may be other records, reply is 
			one time and find protocol is completed.
			The f_more will be equal to 0.
		2) The server cannot reply whole n records as request.
			 The server replies less than n records to the client.
			In this case, as the f_more is not equal to 0 and
			the client does response again, there should be multi 
			reply-response on the contrast to one request-recv.
			 This multi reply-response will terminate when the
			f_more is equal to 0.
		3) The server finds only less than n records.
			 The f_more will be equal to 0.
*/

typedef struct r_req_rewind {
	p_head_t	r_head;
	r_tbl_name_t	r_name;
	r_tbl_t		*r_td;
	r_usr_t		*r_ud;
} r_req_rewind_t;

typedef struct r_res_rewind {
	p_head_t	r_head;
} r_res_rewind_t;

typedef struct r_req_seek {
	p_head_t	r_head;
	r_tbl_name_t	r_name;
	int		r_seek;
} r_req_seek_t;

typedef struct r_res_seek {
	p_head_t	r_head;
} r_res_seek_t;

/*
 *	sort
 */
typedef struct r_req_sort {
	p_head_t	s_head;
	r_tbl_name_t	s_name;
	int		s_n;		/* number of conditions */
} r_req_sort_t;
/* followed by r_sort_t */

typedef struct r_res_sort {
	p_head_t	s_head;
} r_res_sort_t;
		
   
/*
 *	table hold/release/cancel
 */
typedef struct r_req_hold {
	p_head_t	h_head;
	int		h_wait;
	int		h_n;
} r_req_hold_t;
/* followed by ( r_hold_t ) */

typedef struct r_res_hold {
	p_head_t	h_head;

	/* only use of E_RDB_REC_BUSY */
	r_tbl_t		*h_td;
	int		h_abs;		/* record abs number */
} r_res_hold_t;

typedef struct r_req_release {
	p_head_t	r_head;
	r_tbl_name_t	r_name;
} r_req_release_t;

typedef struct r_res_release {
	p_head_t	r_head;
} r_res_release_t;

typedef struct r_req_hold_cancel {
	p_head_t	h_head;
} r_req_hold_cancel_t;

typedef struct r_res_hold_cancel {
	p_head_t	h_head;
} r_res_hold_cancel_t;

/*
 *	table compression
 *		executed on no-one access except me.
 */
typedef struct r_req_compress {
	p_head_t	c_head;
	r_tbl_name_t	c_name;
} r_req_compress_t;

typedef struct r_res_compress {
	p_head_t	c_head;
	int		c_rec_num;
} r_res_compress_t;

/*
 *	event wait/cancel
 */
#define	R_EVENT_REC	1

typedef struct r_req_event {
	p_head_t	e_head;
	int		e_class;
	int		e_code;
	r_tbl_name_t	e_name;
} r_req_event_t;

typedef struct r_res_event {
	p_head_t	e_head;
} r_res_event_t;

typedef struct r_req_event_cancel {
	p_head_t	e_head;
	int		e_class;
	int		e_code;
	r_tbl_name_t	e_name;
} r_req_event_cancel_t;

typedef struct r_res_event_cancel {
	p_head_t	e_head;
} r_res_event_cancel_t;

/*
 *	direct refer
 */
typedef struct r_req_direct {
	p_head_t	d_head;
	r_tbl_name_t	d_name;
	int		d_mode;
	int		d_abs;
} r_req_direct_t;

typedef struct r_res_direct {
	p_head_t	d_head;
} r_res_direct_t;
/* followed by a record */

/*
 *	inquiry of records numbers
 */
typedef	struct	r_req_rec_nums {
	p_head_t	r_head;
	r_tbl_name_t	r_name;
} r_req_rec_nums_t;

typedef	struct	r_res_rec_nums {
	p_head_t	r_head;
	int		r_total;
	int		r_used;
} r_res_rec_nums_t;

/*
 *	stop/restart/shutdown
 *		shutdown uses the stop structure
 */
typedef struct r_req_stop {
	p_head_t	s_head;
} r_req_stop_t;

typedef struct r_res_stop {
	p_head_t	s_head;
} r_res_stop_t;

typedef struct r_req_restart {
	p_head_t	s_head;
	r_path_name_t	s_path;
} r_req_restart_t;

typedef struct r_res_restart {
	p_head_t	s_head;
} r_res_restart_t;

/*
 *	list
 */
typedef struct r_req_list {
	p_head_t	l_head;
} r_req_list_t;

typedef struct r_res_file {
	r_tbl_name_t	f_name;
	struct stat	f_stat;
	char		f_stmt[1024];
} r_res_file_t;

typedef struct r_res_list {
	p_head_t	l_head;
	int		l_n;
	r_res_file_t	l_file[1];
} r_res_list_t;

/*
 *	dump
 */
/* dump information */
typedef struct r_req_dump_inf {
	p_head_t	i_head;
} r_req_dump_inf_t;

typedef struct r_mem_inf {
	int	m_alloc;
	int	m_free;
} r_mem_inf_t;

typedef struct r_res_dump_inf {
	p_head_t	i_head;
	r_inf_t		i_inf;
	int		i_mem;
	r_mem_inf_t	i_mem_inf[16];
} r_res_dump_inf_t;

/* dump record */
typedef struct r_req_dump_record {
	p_head_t	r_head;
	r_tbl_name_t	r_name;
	int		r_abs;
} r_req_dump_record_t;

typedef struct r_res_dump_record {
	p_head_t	r_head;
	r_rec_t		r_rec;
} r_res_dump_record_t;
/* followed by a record */

/* dump tbl */
typedef struct r_req_dump_tbl {
	p_head_t	t_head;
	r_tbl_name_t	t_name;
} r_req_dump_tbl_t;

typedef struct r_res_dump_tbl {
	p_head_t	t_head;
	r_tbl_t		t_tbl;
} r_res_dump_tbl_t;

/* dump usr */
typedef struct r_req_dump_usr {
	p_head_t	u_head;
	r_tbl_name_t	u_name;
	int		u_pos;
} r_req_dump_usr_t;

typedef struct r_res_dump_usr {
	p_head_t	u_head;
	r_usr_t		u_usr;
	char		*u_addr;
} r_res_dump_usr_t;

/* dump memory */
typedef struct r_req_dump_mem {
	p_head_t	m_head;
	char		*m_addr;
	int		m_len;
} r_req_dump_mem_t;

typedef struct r_res_dump_mem {
	p_head_t	m_head;
	int		m_len;
} r_res_dump_mem_t;
/* followed by contents of memory */

/* dump connected client */
typedef struct r_req_dump_client {
	p_head_t	cli_head;
} r_req_dump_client_t;

typedef struct r_res_dump_client {
	p_head_t	cli_head;
	int		cli_num;
} r_res_dump_client_t;
/* followed by information of connected client */

/* SQL command */
typedef struct r_req_sql {
	p_head_t	sql_head;
	int			sql_Major;
	int			sql_Minor;
} r_req_sql_t;

/* SQL params command */
typedef struct r_req_sql_params {
	p_head_t	sql_head;
	int			sql_Major;
	int			sql_Minor;
	size_t			sql_stmt_size;
	int			sql_num_params;
} r_req_sql_params_t;

/*file command*/
typedef struct r_file{
    p_head_t	sql_head;
    int flag;
    char path[64];
    int filelen;
}r_file_t;
/* followed by information of connected client */

typedef struct {
	_dlnk_t		st_lnk;
	r_man_t *	st_md;		/* r_man pointer(client) */
	int		st_stat;
	size_t		st_stmt_size;
	int		st_num_params;
	const char *	st_statement;
	r_value_t *	st_params;
} r_stmt_t;

typedef struct {
    int sp_type;
    unsigned int sp_size;
    char sp_data[0];
} r_sql_param_t;

/* followed by SQL statements */

/*
 *	This response be sent after update/delete
 */
typedef struct r_res_sql {
	p_head_t	sql_head;
	int		sql_changed;
} r_res_sql_t;
/* some data will be followed
	update	number of updated(int)
		number of return records
		record size
		schema
		records
*/
typedef struct r_res_sql_update {
	int	u_updated;
	int	u_num;
	int	u_rec_size;
} r_res_sql_update_t;

/*	This response be sent after all SQLs are done */

typedef struct r_res_sql_done {
	p_head_t	sql_head;
} r_res_sql_done_t;
/*
 *	functions
 */
extern	r_tbl_t	*_search_tbl;
extern	r_usr_t	*_search_usr;
extern	r_svc_inf_t _svc_inf;
extern	char	*svc_myname;

extern	void	lex_init(void);
extern	void	lex_start( char*, char*, int );
extern	int		yyparse(void);

extern	void	sql_lex_init(void);

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

extern	r_man_t	*rdb_link( p_name_t, r_man_name_t );
extern	int		rdb_unlink( r_man_t* );
extern	r_tbl_t	*rdb_create( r_man_t*, r_tbl_name_t, int, int, r_item_t[] );
extern	int		rdb_drop( r_man_t*, r_tbl_name_t );
extern	r_tbl_t	*rdb_open( r_man_t*, r_tbl_name_t );
extern	int		rdb_close( r_tbl_t* );
extern	int		rdb_close_client( r_tbl_t* );
extern	int		rdb_commit( r_man_t* );
extern	int		rdb_rollback( r_man_t* );
extern	int		rdb_size( r_tbl_t* );
extern	int		rdb_number( r_tbl_t*, int );
extern	int		rdb_insert( r_tbl_t*, int, r_head_t*, int, 
										r_item_name_t[], int );
extern	int		rdb_sort( r_tbl_t*, int, r_sort_t[] );
extern	int		rdb_search( r_tbl_t*, char* );
extern	int		rdb_find( r_tbl_t*, int, r_head_t*, int );
extern	int		rdb_seek( r_tbl_t*, int );
extern	int		rdb_update( r_tbl_t*, int, r_head_t*, int );
extern	int		rdb_delete( r_tbl_t*, int, r_head_t*, int );
extern	int		rdb_flush( r_tbl_t* );
extern	int		rdb_cancel( r_tbl_t* );
extern	int		rdb_hold( int, r_hold_t*, int );
extern	int		rdb_release( r_tbl_t* );
extern	int		rdb_event( r_tbl_t*, int, int );
extern	int		rdb_evaluate( r_expr_t*, char*, r_value_t*);
extern	int		rdb_set( r_tbl_t*, r_head_t*, char*, char* );
extern	int		rdb_get( r_tbl_t*, r_head_t*, char*, char*, int* );
extern	int		rdb_list( r_man_t*, r_res_list_t** );

extern	int		rdb_stop( r_man_t* );
extern	int		rdb_shutdown( r_man_t* );
extern	int		rdb_restart( r_man_t*, r_path_name_t );
extern	int		rdb_compress( r_tbl_t* );

extern	int	rdb_dump_lock( r_man_t* );
extern	int	rdb_dump_unlock( r_man_t* );
extern	int	rdb_dump_proc( r_man_t* );
extern	int	rdb_dump_inf( r_man_t*, r_res_dump_inf_t* );
extern	int	rdb_dump_record( r_tbl_t*, int, r_rec_t*, char* );
extern	int	rdb_dump_tbl( r_tbl_t*, r_tbl_t* );
extern	int	rdb_dump_usr( r_tbl_t*, int, r_usr_t*, char** );
extern	int	rdb_dump_mem( r_man_t*, char*, int, char* );

extern	int		_rdb_sort_open( r_tbl_t*, int, r_sort_t* );
extern	int		_rdb_sort_close(void);
extern	int		_rdb_sort_start( r_sort_index_t*, int );
extern	int		_rdb_arrange_data( r_sort_index_t*, r_sort_index_t*, int, int );

extern	void	_rdb_req_head( p_head_t*, int, int );

extern	void*	TBL_DBOPEN( char* );
extern	int		TBL_DBCLOSE( char* );
extern	int		TBL_CREATE( r_tbl_name_t, int, long, int, 
						int, r_item_t[], int, r_tbl_mod_t[] );
extern	int		TBL_DROP( r_tbl_name_t );
extern	int		TBL_OPEN( r_tbl_t*, r_tbl_name_t );
extern	int		TBL_OPEN_SET( r_tbl_t* );
extern	int		TBL_CLOSE( r_tbl_t* );
extern	int		TBL_CNTL( r_tbl_t*, int, char* );
extern	int		TBL_LOAD( r_tbl_t*, int, int, char* );
extern	int		TBL_STORE( r_tbl_t*, int, int, char* );
extern	int		f_create( char* );
extern	int		f_open( char* );
extern	int		f_unlink( char* );

extern	DIR*	f_opendir(void);
extern	int		f_closedir( DIR* );
extern	struct dirent*	f_readdir( DIR* );
extern	void	f_rewinddir( DIR* );
extern	int		f_stat( char*, struct stat* );

extern	int		svc_load( r_tbl_t*, int, int );
extern	int		svc_store( r_tbl_t*, int, int, r_usr_t* );
extern	int		rec_cntl_alloc( r_tbl_t* );
extern	int		rec_cntl_free( r_tbl_t* );
extern	int		rec_buf_alloc( r_tbl_t* );
extern	int		rec_buf_free( r_tbl_t* );

extern	int		svc_event( p_id_t*, p_head_t* );
extern	int		svc_rec_wait( r_tbl_t*, r_usr_t*, int );
extern	int		svc_rec_post( r_tbl_t*, int );
extern	int		svc_event_cancel( p_id_t*, p_head_t* );
extern	int		svc_cancel_rwait( r_tbl_t*, r_usr_t*, int );

extern	int		svc_hold( p_id_t*, p_head_t* );
extern	int		svc_release( p_id_t*, p_head_t* );
extern	int		svc_hold_cancel( p_id_t*, p_head_t* );
extern	int		svc_tbl_post( r_tbl_t* );
extern	int		_check_rec_lock( r_tbl_t*, r_usr_t* );

extern	int		 svc_open( p_id_t*, p_head_t* );
extern	int		 svc_close( p_id_t*, p_head_t* );
extern	int		 svc_close_client( p_id_t*, p_head_t* );
extern	int		close_user( r_usr_t* );

extern	int		check_td_ud( r_tbl_t*, r_usr_t* );
extern	int		check_td( r_tbl_t* );
extern	int		check_ud( r_usr_t* );
extern	int		check_lock( r_tbl_t*, r_usr_t*, int );
extern	void	reply_head( p_head_t*, p_head_t*, int, int );
extern	int		err_reply( p_id_t*, p_head_t*, int );
extern	int		drop_table(void);
extern	int		drop_td( r_tbl_t* );
extern	int		svc_switch( r_tbl_t*, int, int );
extern	r_tbl_t	*svc_tbl_alloc(void);
extern	void	svc_tbl_free( r_tbl_t* );

extern	int		svc_alternate_table( char*, int, r_item_t* );

extern	int		svc_list( p_id_t*, p_head_t* );

extern	int		svc_dump_proc( p_id_t*, p_head_t* );
extern	int		svc_dump_inf( p_id_t*, p_head_t* );
extern	int		svc_dump_record( p_id_t*, p_head_t* );
extern	int		svc_dump_tbl( p_id_t*, p_head_t* );
extern	int		svc_dump_usr( p_id_t*, p_head_t* );
extern	int		svc_dump_mem( p_id_t*, p_head_t* );
extern	int		svc_dump_client( p_id_t*, p_head_t* );

extern	int		svc_shutdown( void );
extern	int		svc_stop( p_id_t*, p_head_t* );
extern	int		svc_restart( p_id_t*, p_head_t* );

extern	int		svc_number( p_id_t*, p_head_t* );
extern	int		svc_direct( p_id_t*, p_head_t* );
extern	int		svc_compress( p_id_t*, p_head_t* );
extern	int		svc_flush( p_id_t*, p_head_t* );
extern	int		svc_cancel( p_id_t*, p_head_t* );
extern	int		svc_sort( p_id_t*, p_head_t* );
extern	int		svc_find( p_id_t*, p_head_t* );
extern	int		svc_rewind( p_id_t*, p_head_t* );
extern	int		svc_seek( p_id_t*, p_head_t* );
extern	int		svc_search( p_id_t*, p_head_t* );
extern	int		svc_update( p_id_t*, p_head_t* );
extern	int		svc_delete( p_id_t*, p_head_t* );
extern	int		svc_insert( p_id_t*, p_head_t* );
extern	int		svc_create( p_id_t*, p_head_t* );
extern	int		svc_drop( p_id_t*, p_head_t* );
extern	int		svc_link( p_id_t*, p_head_t* );
extern	int		svc_unlink( p_id_t*, p_head_t* );
extern	int		svc_rollback_td( r_tbl_t*, r_usr_t* );
extern	int		svc_commit( p_id_t*, p_head_t* );
extern	int		svc_rollback( p_id_t*, p_head_t* );
extern	int		svc_garbage( p_id_t* );
extern	int		svc_sql( p_id_t*, p_head_t* );
extern	int		svc_sql_params( p_id_t*, p_head_t* );

extern	char	*getsqllines( char*, int );
extern	void	print_record( int, r_head_t*, int, r_item_t[] );
extern	void	print_record_with_cntl( int, r_rec_t*, r_head_t*, 
							int, r_item_t[] );

r_tbl_t	*_r_tbl_alloc(void);
void	_r_tbl_free(r_tbl_t*);
r_usr_t	*_r_usr_alloc(void);
void	_r_usr_free(r_usr_t*);


#define	R_DROP_TABLE_TIME	5*60	/* sec */

#define	R_PAGE_SIZE	(1024*1024*2)	/* 2M */

#define	end	_rdb_end
extern	char	*neo_edata, *neo_end;

#define	CHK_ADDR( a )	( (neo_edata <= (char*)(a) && (char*)(a) < neo_end ) ? 0 : 1 )

#define	malloc	neo_malloc
#define	realloc	neo_realloc
#define	free	neo_free


//#endif /* DB_MAN_SYSTEM   */
/*============================================================================*/

#endif /* _NEO_DB_ */
