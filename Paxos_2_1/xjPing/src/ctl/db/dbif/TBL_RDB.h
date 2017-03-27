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
 *	TBL_RDB.h
 *
 *	説明	DB i/f header
 *
 ******************************************************************************/
#ifndef	_TBL_RDB_
#define	_TBL_RDB_

#include	"neo_db.h"
#include	<fcntl.h>
#include	"../../../../../cache/h/FileCache.h"

extern	BlockCacheCtrl_t	BCNTRL;
#define	pBC	(&BCNTRL)

#define	MY_FD_MAX	40	/* max fds */

#define	MY_FD	0x01		/* fd attached */

/*
 * DB table descriptor
 */
typedef struct my_tbl {
	r_tbl_t		*m_td;		/* DB_MAN table descriptor */
	int		m_stat;		/* status */
	int		m_fd;		/* file descriptor */
	int		m_data_off;	/* data offset */
	_dlnk_t		m_fd_active;	/* fd active list */
} my_tbl_t;

#define	MYTOTD( p )	((p)->m_td)
#define	TDTOMY( p )	((my_tbl_t *)(p)->t_tag)
#define	FDTOMY( p ) \
		(my_tbl_t*)((char*)(p)-(size_t)&((my_tbl_t*)0)->m_fd_active)

/*
 * function
 */
extern	void	fd_init( char * pPath );
extern	int		fd_drop();
extern	int		fd_create( r_tbl_name_t );
extern	int		fd_delete( int );
extern	int 	fd_open( my_tbl_t* );
extern	int 	fd_close( my_tbl_t* );
extern	int		fd_unlink( r_tbl_name_t );

extern	my_tbl_t	*my_tbl_alloc();
extern	void		my_tbl_free( my_tbl_t* );
extern	int 		my_tbl_store( my_tbl_t*, int, int, char* );
extern	int 		my_tbl_load( my_tbl_t*, int, int, char* );

extern	int		my_head_write( char*, int, long, int, 
								int, r_item_t[], uint64_t *pOff );
extern	int		my_head_update( my_tbl_t* );
extern	int		my_head_read( my_tbl_t*, uint64_t *pOff );
extern	int		my_item_write( char*, int, r_item_t[], uint64_t *pOff );
extern	int		my_item_read( my_tbl_t*, uint64_t *pOff );
extern	int		my_depend_write( char*, int, r_tbl_mod_t[], uint64_t *pOff );
extern	int		my_depend_read( my_tbl_t*, uint64_t *pOff );
extern	int64_t	my_data_off( my_tbl_t* );
extern	int64_t	my_depend_off( my_tbl_t* );

/*----------------------------------------------------------------------------

  ○  ＮＥＯ／ＤＢ ファイルフォーマット

  +----------------------------------------+
  |             レコード数                 |	4 バイト（バイナリ）
  +----------------------------------------+
  |             有効レコード数             |	4 バイト（バイナリ）
  +----------------------------------------+
  |             レコード長                 |	4 バイト（バイナリ） 
  +----------------------------------------+
  |             アイテム数                 |	4 バイト（バイナリ）
  +----+-----------------------------------+
  | ア |        アイテム名                 |	32バイト（文字列）
  + イ +-----------------------------------+
  | テ |        属性                       |	4 バイト（バイナリ）
  + ム +-----------------------------------+
  | 情 |        有効アイテム長             |	4 バイト（バイナリ）
  + 報 +-----------------------------------+
  | ＊ |        アイテム幅                 |	4 バイト（バイナリ）
  + ｎ +-----------------------------------+
  |    |        レコード内オフセット値     |	4 バイト（バイナリ）
  +----+----+------------------------------+
  | ０ | ヘ |   相対番号                   |	4 バイト（バイナリ）
  + 番 + ッ +------------------------------+
  | 目 | ダ |   絶対番号                   |	4 バイト（バイナリ）
  + の + 部 +------------------------------+
  | レ |    |   状態                       |	4 バイト（バイナリ）
  + コ +----+------------------------------+
  | ｜ | ア |   0 番目のアイテムデータ     |	アイテム幅 バイト）
  | ド | イ |                              |
  + デ + テ +------------------------------+
  | ｜ | ム |   1 番目のアイテムデータ     |     .....
  | タ | デ |                              |
  +    + ｜ +------------------------------+
  |    | タ |                              |
  |    |    |    ........                  |
  +----+----+------------------------------+
                 ........



  ○  ＮＥＯ／ＤＢ  インタフェース

  (1)  データベースのオープン
	
	TBL_DBOPEN( char *データベース名 )


  (2)  データベースのクローズ
	
	TBL_DBCLOSE( char *データベース名 )


  (3)  テーブルの生成
	
	TBL_CREATE( r_tbl_name_t , int rec_num, int item_num, r_item_t* )


  (4)  テーブルの削除
	
	TBL_DROP( r_tbl_name_t )


  (5)  テーブルのオープン
	
	TBL_OPEN( r_tbl_t*, r_tbl_name_t )


  (6)  テーブルのクローズ
	
	TBL_CLOSE( r_tbl_t* )


  (7)  テーブルのロード
	
	TBL_LOAD( r_tbl_t*, int start_abs, int num, char* )


  (8)  テーブルのストア
	
	TBL_STORE( r_tbl_t*, int start_abs, int num, char* )


  (9)  テーブルのヘッダ部の変更
	
	TBL_CNTL( r_tbl_t*, int cmd, char *arg )

----------------------------------------------------------------------------*/

#endif /* _TBL_RDB_ */
