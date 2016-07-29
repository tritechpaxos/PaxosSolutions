/*******************************************************************************
 * 
 *  TBL_IO.c --- xjPing (On Memory DB) db I/F Library programs
 * 
 *  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

#include	"TBL_RDB.h"

#ifdef	ZZZ

int	MyFdMax = MY_FD_MAX;
static int		fd_cnt;
static _dlnk_t	fd_active;
static char		PathDB[R_PATH_NAME_MAX];

char*
F( char *name )
{
	static char _f[R_PATH_NAME_MAX];

	if( PathDB[0] ) {strcpy(_f,PathDB);strcat(_f,"/");strcat(_f,name);}
	else	strcpy(_f,name );
	return( _f );
}

/*****************************************************************************
 *
 *  関数名	fd_init()
 *
 *  機能	DBテーブル記述子リスト初期化
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
void
fd_init( char *pPath )
{
	_dl_init(&fd_active);
	strcpy( PathDB, pPath );
}


/*****************************************************************************
 *
 *  関数名	fd_drop()
 *
 *  機能	古いDBテーブルをクローズする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
fd_drop()
{
	long		past;
	_dlnk_t		*dlp;
	my_tbl_t	*md;
	my_tbl_t	*find;
	r_tbl_t		*td;

DBG_ENT(M_RDB,"fd_drop");

	find	= 0;
	past	= time(0);

	/*
	 * 古いDBテーブルを捜し出す
	 */
	for( dlp = _dl_head(&fd_active); _dl_valid(&fd_active,dlp);
			dlp = _dl_next(dlp) ) {
		md = FDTOMY(dlp);
		td = MYTOTD(md);
		if( td->t_access < past ) {
			past 	= td->t_access;
			find	= md;
		}
	}

	if( !find ) {
		neo_errno	= E_RDB_FD;
		goto err1;
	}

	/*
	 * 該当DBテーブルをクローズする
	 */
	if( (neo_errno = fd_close( find )) )
		goto err1;

DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	fd_creat()
 *
 *  機能	ファイルを生成する
 * 
 *  関数値
 *     		正常	生成したファイルの記述子
 *		異常	-1
 *
 ******************************************************************************/
int
fd_create( name )
	r_tbl_name_t	name;		/* 生成するファイル名	*/
{
	int	fd;

DBG_ENT(M_RDB,"fd_create");

	/*
	 * プロセス毎のファイル記述子制限数以上だったら、
	 * fd_drop()にてある記述子をクローズする
	 */
	if( fd_cnt > MyFdMax ) {
		if( fd_drop() )
			goto err1;
	}

	/*
	 * ファイルを生成する
     *
	 * O_RDWR  : 読み書き用
     * O_CREAT : 必要に応じ、mode 指定のパーミッションでファイルを作成する
     * O_EXCL  : O_CREATと共に指定された場合、すでにファイルが存在する場合は失敗する。
     *           同じファイルを２度作成しないためのオプション。
	 */
	if( (fd = open( F(name), O_RDWR|O_CREAT|O_EXCL, 0666 )) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}

DBG_A(("fd = %d\n", fd ));
	fd_cnt++;

DBG_EXIT(fd);
	return( fd );
err1:
DBG_EXIT(-1);
	return( -1 );
}


/*****************************************************************************
 *
 *  関数名	fd_delete()
 *
 *  機能	ファイルをクローズする
 * 
 *  関数値
 *     		正常	0
 *		異常	-1
 *
 ******************************************************************************/
int
fd_delete( fd )
	int	fd;
{

DBG_ENT(M_RDB,"fd_delete");
DBG_A(("fd=%d\n", fd ));

	if( close( fd ) ) {
		neo_errno = unixerr2neo();
		goto err1;
	}

	fd_cnt--;

DBG_EXIT( 0 );
	return( 0 );

err1:
DBG_EXIT(-1);
	return( -1 );
}

/*****************************************************************************
 *
 *  関数名	fd_open()
 *
 *  機能	DBテーブル記述子をオープンする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
fd_open( md )
	my_tbl_t	*md;		/* DBテーブル記述子	*/
{
	int	fd;
DBG_ENT(M_RDB,"fd_open");

	/*
	 * ファイル記述子が既にDBテーブル記述子
	 * にアタッチされている場合は、処理しない
	 */
	if( md->m_stat & MY_FD )
		goto exit;

	/*
	 * プロセス毎のファイル記述子制限数以上だったら、
	 * fd_drop()にてある記述子をクローズする
	 */
	if( fd_cnt > MyFdMax ) {
		if( fd_drop() )
			goto err1;
	}

	/*
	 * ファイルをオープンする
     *
	 * O_RDWR  : 読み書き用
     * O_BINARY : Windows系でバイナリ、テキストのモード指定をするのに使用する
	 */
	if( (fd = open( F(MYTOTD(md)->t_name), O_RDWR )) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
DBG_A(("fd=%d\n", fd ));

	/*
	 * DBテーブル記述子のファイルアタッチ済状態
	 * をたつ、ファイル記述子を中に設定する
	 */
	fd_cnt++;
	md->m_fd	= fd;
	md->m_stat	|= MY_FD;

	/*
	 * 該当DBテーブル記述子をリストに登録する
	 */
	_dl_insert( &md->m_fd_active, &fd_active );

exit:
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(-1);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	fd_close()
 *
 *  機能	DBテーブル記述子をクローズする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
fd_close( md )
	my_tbl_t	*md;		/* DBテーブル記述子	*/
{

DBG_ENT(M_RDB,"fd_close");

	/*
	 * ファイル記述子が既にDBテーブル記述子から
	 * ディタッチされた場合には、処理しない
	 */
	if( !(md->m_stat & MY_FD ) )
		goto exit;
DBG_A(("fd=%d\n", md->m_fd ));

	/*
	 * ファイルをクローズする
	 */
	if( close( md->m_fd ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}

	/*
	 * DBテーブル記述子をリストから外して、
	 * アタッチ済ビットをクリアする
	 */
	_dl_remove( &md->m_fd_active );
	md->m_stat &= ~MY_FD;

	fd_cnt--;
DBG_A(("fd=%d,fd_cnt=%d\n", md->m_fd, fd_cnt ));
exit:
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
fd_unlink( r_tbl_name_t name )
{
	int	Ret;

	Ret = unlink( F(name) );
	return( Ret );
}

#endif	// ZZZ

/*****************************************************************************
 *
 *  関数名	my_tbl_alloc()
 *
 *  機能	DBテーブル記述子用エリアを生成する
 * 
 *  関数値
 *     		正常	生成したエリアのアドレス
 *		異常	NULL
 *
 ******************************************************************************/
my_tbl_t	*
my_tbl_alloc()
{
	my_tbl_t *md;
DBG_ENT(M_RDB,"my_tbl_alloc");

	md = (my_tbl_t *)neo_zalloc(sizeof(my_tbl_t) );
DBG_EXIT(md);
	return( md );
}


/*****************************************************************************
 *
 *  関数名	my_tbl_free()
 *
 *  機能	DBテーブル記述子用エリアを解放する
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
void
my_tbl_free( md )
	my_tbl_t	*md;		/* DBテーブル記述子 */
{
	r_tbl_t	*td;

DBG_ENT(M_RDB,"my_tbl_free");
DBG_A(("md=0x%x\n",md));

	/*
	 * スキーマ持っていれば、解放する
	 */
	td = MYTOTD( md );
	if( td ) {
		if( td->t_scm ) {
			neo_free( td->t_scm );
			td->t_scm = NULL;
		}
		if( td->t_depend ) {
			neo_free( td->t_depend );
			td->t_depend = NULL;
		}
	}
	neo_free( md );

DBG_EXIT(0);
}


/*****************************************************************************
 *
 *  関数名	my_head_write()
 *
 *  機能	レコード情報をファイルヘッダ部
 *		に書き込む
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
my_head_write( pName, type, modified, rec_num, item_num, items, pOff )
	char *pName;
	int		type;
	long		modified;
	int		rec_num;	/* レコードの数		*/
	int		item_num;	/* アイテムの数		*/
	r_item_t	items[];	/* アイテム情報エリア	*/
	uint64_t	*pOff;
{
	int	size;
	int	i;
	int	used = 0;
	uint64_t	Off;

DBG_ENT(M_RDB,"my_head_write");

	/*
	 * レコードサイズを算出する
	 */
	size	= sizeof(r_head_t);
	for( i = 0; i < item_num; i++ ) {
		size += items[i].i_size;
	}
DBG_A(("rec_num=%d,item_num=%d,size=%d\n", rec_num, item_num, size ));

/***
	if( lseek( fd, (off_t)0, 0 ) < 0 ) {
		goto err1;
	}
***/
	Off	= 0;

/***
	if( write( fd, &type, sizeof(type) ) < 0  )
		goto err1;
	if( write( fd, &modified, sizeof(modified) ) < 0  )
		goto err1;
***/

	if( FileCacheWrite( pBC, pName, Off, 
			sizeof(type), (char*)&type ) < 0 )
		goto err1;
	Off	+= sizeof(type);
	if( FileCacheWrite( pBC, pName, Off, 
			sizeof(modified), (char*)&modified ) < 0 )
		goto err1;
	Off	+= sizeof(modified);
	/*
	 * レコード総数、USEDレコード数、レコードサイズ
	 * 情報をファイルヘッダ部に書き込む
	 */
/***
	if( write( fd, &rec_num, sizeof(rec_num) ) < 0  )
		goto err1;
	if( write( fd, &used, sizeof(used) ) < 0  )
		goto err1;
	if( write( fd, &size, sizeof(size) ) < 0  )
		goto err1;
***/

	if( FileCacheWrite( pBC, pName, Off, 
				sizeof(rec_num), (char*)&rec_num ) < 0 )
		goto err1;
	Off	+= sizeof(rec_num);
	if( FileCacheWrite( pBC, pName, Off, 
				sizeof(used), (char*)&used ) < 0 )
		goto err1;
	Off	+= sizeof(used);
	if( FileCacheWrite( pBC, pName, Off, 
				sizeof(size), (char*)&size ) < 0 )
		goto err1;
	Off	+= sizeof(size);

	*pOff	= Off;
DBG_EXIT(0);
	return( 0 );

err1:
	neo_errno	= unixerr2neo();
DBG_EXIT(neo_errno);
	return( neo_errno );
}



/*****************************************************************************
 *
 *  関数名	my_head_update()
 *
 *  機能	ファイルヘッダ部の情報を更新する
 *		(modified,used)
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
my_head_update( md )
	my_tbl_t	*md;		/* DBテーブル記述子	*/
{
	r_tbl_t	*td;
	uint64_t	len;
	uint64_t	Off;

DBG_ENT(M_RDB,"my_head_update");

	td = MYTOTD( md );

/***
	if( (neo_errno = fd_open( md )) )
		goto err1;

	if( lseek( md->m_fd, (off_t)sizeof(int), 0 ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
***/
	Off	= sizeof(int);

/****
	if( write( md->m_fd, &td->t_modified, sizeof(td->t_modified) ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}

	if( write(md->m_fd, &td->t_rec_num, sizeof(td->t_rec_num)) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
DBG_A( (" file's used record information updated to %d\n", td->t_rec_used ) );

	if( write(md->m_fd, &td->t_rec_ori_used, sizeof(td->t_rec_used)) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
***/
	if( FileCacheWrite( pBC, td->t_name, Off, 
			sizeof(td->t_modified), (char*)&td->t_modified ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
	Off	+= sizeof(td->t_modified);

	if( FileCacheWrite( pBC, td->t_name, Off,
			sizeof(td->t_rec_num), (char*)&td->t_rec_num ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
	Off	+= sizeof(td->t_rec_num);

	if( FileCacheWrite( pBC, td->t_name, Off,
			sizeof(td->t_rec_used), (char*)&td->t_rec_ori_used ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
	Off	+= sizeof(td->t_rec_used);

	if( td->t_depend ) {
/***
		if( lseek( md->m_fd, (off_t)my_depend_off( md ), 0 ) < 0 ) {
			neo_errno = unixerr2neo();
			goto err1;
		}
		if( write(md->m_fd, td->t_depend, 
			sizeof(r_tbl_list_t)
			+ sizeof(r_tbl_mod_t)*(td->t_depend->l_n - 1 ) ) < 0 ) {
			neo_errno = unixerr2neo();
			goto err1;
		}
***/
		Off	= my_depend_off( md );
		len	= sizeof(r_tbl_list_t)+sizeof(r_tbl_mod_t)*(td->t_depend->l_n-1);

		if( FileCacheWrite( pBC, td->t_name, Off, 
					len, (char*)td->t_depend) < 0 ) {
			neo_errno = unixerr2neo();
			goto err1;
		}
	}

	len	= my_data_off( md ) + td->t_rec_size*td->t_rec_num;
/***
	if( ftruncate(md->m_fd, len ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
***/
	if( FileCacheTruncate( pBC, td->t_name, len ) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
DBG_EXIT(0);
	return( 0 );

err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	my_head_read()
 *
 *  機能	DBファイルのヘッダ部読み出す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 *  (注)	DBファイルのフォーマットを参照すること
 ******************************************************************************/
int
my_head_read( myd, pOff )
	my_tbl_t	*myd;		/* DBテーブル記述子	*/
	uint64_t	*pOff;
{
	int	type;
	long	modified;
	int	rec_num;
	int	size;
	int	used;
	r_tbl_t	*td;
	uint64_t	Off;

DBG_ENT(M_RDB,"my_head_read");

	td = MYTOTD( myd );
	/*
	 * DBテーブルをオープンする
	 */
/***
	if( (neo_errno = fd_open( myd )) )
		goto err1;
***/

	/*
	 * ファイルヘッダにあるレコード総数、USEDレコード数,
	 * レコードサイズ情報を読み出す
	 */
/***
	if( lseek( myd->m_fd, (off_t)0, 0 ) < 0 ) 
		goto err2;
	if( read( myd->m_fd, &type, sizeof(type) ) < 0  )
		goto err2;
	if( read( myd->m_fd, &modified, sizeof(modified) ) < 0  )
		goto err2;
	if( read( myd->m_fd, &rec_num, sizeof(rec_num) ) < 0  )
		goto err2;
	if( read( myd->m_fd, &used, sizeof(used) ) < 0  )
		goto err2;

	if ( used > rec_num ) {
		neo_errno = E_RDB_FILE;
		goto	err1;
	}

	if( read( myd->m_fd, &size, sizeof(size) ) < 0  )
		goto err2;
***/
	Off	= 0;

	if( FileCacheRead( pBC, td->t_name, Off,
				sizeof(type), (char*)&type ) < 0  )
		goto err2;
	Off	+= sizeof(type);

	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(modified), (char*)&modified ) < 0  )
		goto err2;
	Off	+= sizeof(modified);

	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(rec_num), (char*)&rec_num ) < 0  )
		goto err2;
	Off	+= sizeof(rec_num);

	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(used), (char*)&used ) < 0  )
		goto err2;
	Off	+= sizeof(used);

	if ( used > rec_num ) {
		neo_errno = E_RDB_FILE;
		goto	err1;
	}

	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(size), (char*)&size ) < 0  )
		goto err2;
	Off	+= sizeof(size);

	/*
	 * 上記情報をテーブル記述子に設定する
	 */
	td->t_type		= type;
	td->t_modified		= modified;
	td->t_rec_size		= size;
	td->t_rec_num		= rec_num;
	td->t_rec_used		= used;
	td->t_rec_ori_used	= used;

	*pOff	= Off;

DBG_A(("rec_num=%d,used=%d,size=%d\n", rec_num, used, size ));
DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_errno = unixerr2neo();
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}



/*****************************************************************************
 *
 *  関数名	my_item_write()
 *
 *  機能	アイテム情報をファイルへ書き込む
 * 
 *  関数値
 *     		正常	自ポート記述子のアドレス
 *		異常	0
 *
 ******************************************************************************/
int
my_item_write( pName, item_num, items, pOff )
	char	*pName;
	int		item_num;	/* アイテムの数		  */
	r_item_t	items[];	/* アイテム情報格納エリア */
	uint64_t	*pOff;
{
	int		off;
	int		i;
	uint64_t	Off, Len;

DBG_ENT(M_RDB,"my_item_write");

	/*
	 * 各アイテムのレコード内のオフセットを算出する
	 */
	for( i = 0, off = sizeof(r_head_t); i < item_num; i++ ) {
		items[i].i_off	= off;
		off		+= items[i].i_size;
DBG_A(("i=%d,off=%d,size=%d\n", i, items[i].i_off, items[i].i_size));
	}

	/*
	 * アイテム数情報、各アイテム情報をファイルヘッダ部に書き込む
	 */
/***
	if( write( fd, &item_num, sizeof(item_num) ) < 0 ) {
		goto err1;
	}
	if( write( fd, items, item_num*sizeof(r_item_t) ) < 0 ) {
		goto err1;
	}
***/
	Off	= *pOff;

	if( FileCacheWrite( pBC, pName, Off,
			sizeof(item_num), (char*)&item_num ) < 0 ) {
		goto err1;
	}
	Off	+= sizeof(item_num);

	Len = item_num*sizeof(r_item_t);
	if( FileCacheWrite( pBC, pName, Off, Len, (char*)items ) < 0 ) {
		goto err1;
	}
	Off	+= Len;

	*pOff	= Off;

DBG_EXIT(0);
	return( 0 );
err1:
		neo_errno	= unixerr2neo();
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	my_item_read()
 *
 *  機能	DBファイルヘッダ部にあるアイテム
 *		情報を読み出す
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 *  (注)	DBファイルのフォーマットを参照すること
 *
 ******************************************************************************/
int
my_item_read( myd, pOff )
	my_tbl_t	*myd;		/* DBテーブル記述子	*/
	uint64_t	*pOff;
{
	r_scm_t		*sp;
	int		item_num;
	struct	stat	buf;
	int		file_size;
	int		off_set;
	int		i;
	r_tbl_t	*td;
	uint64_t	Off, Len;

DBG_ENT(M_RDB,"my_item_read");

	td = MYTOTD( myd );

	/*
	 * DBテーブル記述子をオープンする
	 */
/***
	if( (neo_errno = fd_open( myd )) )
		goto err1;
***/


	/*
	 * ファイルヘッダ部のアイテム数情報を読み出す
	 */
/***
	if( read( myd->m_fd, &item_num, sizeof(item_num) ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
***/
	Off	= *pOff;

	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(item_num), (char*)&item_num ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
	Off	+= sizeof(item_num);

	/*
	 * ファイルサイズチェック
	 */
/***
	 if ( fstat( myd->m_fd, &buf ) ) {
		neo_errno = unixerr2neo();
		goto	err1;
	}
****/
	 if ( FileCacheStat( pBC, td->t_name, &buf ) ) {
		neo_errno = unixerr2neo();
		goto	err1;
	}

	file_size = td->t_rec_size * td->t_rec_used ;
	file_size += ( 16 + item_num * sizeof(r_item_t) );

	if ( (file_size > buf.st_size) || ( file_size <= 0 ) ) {
		neo_errno = E_RDB_FILE;
		goto	err1;
	}

	/*
	 * スキーマ用エリアを生成する
	 */
	if( !(sp = (r_scm_t*)neo_malloc(sizeof(r_scm_t)
				+ (item_num - 1 )*sizeof(r_item_t)) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * スキーマをテーブル記述子に設定する
	 */
	sp->s_n	= item_num;
	td->t_scm = sp;

	/*
	 * ファイルヘッダ部のアイテム情報を上記
	 * スキーマエリアに読み込む
	 */
/***
	if( read( myd->m_fd, sp->s_item, item_num*sizeof(r_item_t) ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
***/
	Len	= item_num*sizeof(r_item_t);
	if( FileCacheRead( pBC, td->t_name, Off,
			Len, (char*)sp->s_item ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
	Off	+= Len;

	/*
	 * アイテム情報チェック
	 */
/***
	 off_set = 12 ;
***/
	 off_set = sizeof(r_head_t) ;
	 for ( i = 0; i < item_num; i ++ ) {

		if ( sp->s_item[i].i_off != off_set ) {
			neo_errno = E_RDB_FILE;
			goto	err1;
		}

		off_set += sp->s_item[i].i_size;
	 }

	 if ( off_set != td->t_rec_size ) {
		neo_errno = E_RDB_FILE;
		goto	err1;
	}

	*pOff	= Off;

DBG_A(("item_num=%d\n", item_num));
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
my_depend_write( pName, num, depends, pOff )
	char	*pName;
	int		num;
	r_tbl_mod_t	depends[];
	uint64_t	*pOff;
{
	uint64_t	Off, Len;

DBG_ENT(M_RDB,"my_depend_write");

	Off	= *pOff;

/***
	if( write( fd, &num, sizeof(num) ) < 0 ) {
		goto err1;
	}
	if( num > 0 ) {
		if( write( fd, depends, sizeof(r_tbl_mod_t)*num ) < 0 ) {
			goto err1;
		}
	}
***/
	if( FileCacheWrite( pBC, pName, Off,
			sizeof(num), (char*)&num ) < 0 ) {
		goto err1;
	}
	Off	+= sizeof(num);

	if( num > 0 ) {
/***
		if( write( fd, depends, sizeof(r_tbl_mod_t)*num ) < 0 ) {
			goto err1;
***/
		Len	= sizeof(r_tbl_mod_t)*num;
		if( FileCacheWrite( pBC, pName, Off,
				Len, (char*)depends ) < 0 ) {
			goto err1;
		}
		Off	+= Len;
	}
	*pOff	= Off;

DBG_EXIT( 0 );
	return( 0 );
err1:
	neo_errno	= unixerr2neo();
DBG_EXIT(neo_errno);
	return( neo_errno );
}

int
my_depend_read( myd, pOff )
	my_tbl_t	*myd;		/* DBテーブル記述子	*/
	uint64_t	*pOff;
{
	int		num;
	r_tbl_list_t	*dp;
	r_tbl_t	*td;
	uint64_t	Off, Len;

DBG_ENT(M_RDB,"my_depend_write");

	td = MYTOTD( myd );

/***
	if( (neo_errno = fd_open( myd )) )
		goto err1;
***/
	Off	= *pOff;

/***
	if( read( myd->m_fd, &num, sizeof(num) ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
***/
	if( FileCacheRead( pBC, td->t_name, Off,
			sizeof(num), (char*)&num ) < 0 ) {
		neo_errno	= unixerr2neo();
		goto err1;
	}
	Off	+= sizeof(num);

	if( num > 0 ) {
		if( !(dp = (r_tbl_list_t*)neo_malloc(sizeof(r_tbl_list_t)
				+ (num - 1 )*sizeof(r_tbl_mod_t)) ) ) {
			neo_errno = E_RDB_NOMEM;
			goto err1;
		}
/***
		if( read( myd->m_fd, dp->l_tbl, sizeof(r_tbl_mod_t)*num ) < 0 ) {
			neo_errno	= unixerr2neo();
			goto err1;
		}
***/
		Len	= sizeof(r_tbl_mod_t)*num;
		if( FileCacheRead( pBC, td->t_name, Off,
				Len, (char*)dp->l_tbl ) < 0 ) {
			neo_errno	= unixerr2neo();
			goto err1;
		}
		Off	+= Len;

		dp->l_n	= num;
		td->t_depend = dp;
	}
DBG_EXIT( 0 );
	return( 0 );
err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

/*****************************************************************************
 *
 *  関数名	my_data_off()
 *
 *  機能	ファイルデータ部の先頭オフセットを算出する
 * 
 *  関数値
 *     		オフセット値
 *
 ******************************************************************************/
int64_t
my_data_off( myd )
	my_tbl_t	*myd;		/* DBテーブル記述子	*/
{
	int64_t	off;

DBG_ENT(M_RDB,"my_data_off");

	off 	= my_depend_off( myd );
	off	+= sizeof(int);
	if( MYTOTD(myd)->t_depend ) {
		off	+= MYTOTD(myd)->t_depend->l_n*sizeof(r_tbl_mod_t);
	}
DBG_EXIT(off);
	return( off );
}

int64_t
my_depend_off( myd )
	my_tbl_t	*myd;		/* DBテーブル記述子	*/
{
	int64_t	off;

DBG_ENT(M_RDB,"my_data_off");

	off	= sizeof(MYTOTD(myd)->t_type);
	off	+= sizeof(MYTOTD(myd)->t_modified);
	off	+= sizeof(MYTOTD(myd)->t_rec_num);
	off	+= sizeof(MYTOTD(myd)->t_rec_used);
	off	+= sizeof(MYTOTD(myd)->t_rec_size);
	off	+= sizeof(MYTOTD(myd)->t_scm->s_n);
	off	+= MYTOTD(myd)->t_scm->s_n * sizeof(r_item_t);

DBG_EXIT(off);
	return( off );
}


/*****************************************************************************
 *
 *  関数名	my_tbl_store()
 *
 *  機能	テーブルをファイルへ書き換え
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
my_tbl_store( md, s, n, bufp )
	my_tbl_t	*md;		/* DBテーブル記述子	*/
	int		s;		/* 開始レコード番号	*/
	int		n;		/* レコード数		*/
	char		*bufp;		/* レコード格納エリア	*/
{
	r_tbl_t	*td;
	uint64_t	Off, Len;

DBG_ENT(M_RDB,"my_tbl_store");

	td = MYTOTD( md );

	/* 
	 * DBテーブルをオープンする
	 */
/***
	if( (neo_errno = fd_open( md )) )
		goto err1;
***/

	/*
	 * ファイル記述子ポインタを調整して、書き込みを行う
	 */
/***
	if( lseek( md->m_fd, (off_t)(md->m_data_off+s*td->t_rec_size), 0) < 0 )
		goto err2;;

	if( write( md->m_fd, bufp, n*td->t_rec_size ) < 0 )
		goto err2;;
***/
	Off	= md->m_data_off + s*td->t_rec_size;
	Len	= n*td->t_rec_size;

	if( FileCacheWrite( pBC, td->t_name, Off, Len, bufp ) < 0 )
		goto err2;

DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_errno = unixerr2neo();
//err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}


/*****************************************************************************
 *
 *  関数名	my_tbl_load()
 *
 *  機能	ファイルからテーブルにレコードをロードする
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
my_tbl_load( md, s, n, bufp )
	my_tbl_t	*md;		/* DBテーブル記述子	*/
	int		s;		/* 開始レコード番号	*/
	int		n;		/* 読み出すレコード数	*/
	char		*bufp;		/* レコード格納用エリア */
{
	r_tbl_t	*td;
	int	len;
	uint64_t	Off, Len;
DBG_ENT(M_RDB,"my_tbl_load");

	td = MYTOTD( md );

	/* 
	 * DBテーブル記述子をオープンする
	 */
/***
	if( (neo_errno = fd_open( md )) )
		goto err1;
***/

	/*
	 * ファイル記述子のシークポインタを調整して、
	 * 読み出しを行う
	 */
/***
	if( lseek( md->m_fd, (off_t)(md->m_data_off+s*td->t_rec_size), 0 ) < 0 )
		goto err2;

	len = read( md->m_fd, bufp, n*td->t_rec_size );
***/
	Off	= md->m_data_off+s*td->t_rec_size;
	Len	= n*td->t_rec_size;

	len	= FileCacheRead( pBC, td->t_name, Off, Len, bufp );

	if ( len < 0 )
		goto err2;
	else if ( len < n*td->t_rec_size )
		bzero( bufp + len , n*td->t_rec_size - len ) ;

DBG_EXIT(0);
	return( 0 );
err2: DBG_T("err2");
	neo_errno	= unixerr2neo();
//err1:
DBG_EXIT(neo_errno);
	return( neo_errno );
}

char*
F( char *name )
{
	static char _f[R_PATH_NAME_MAX];

	sprintf( _f, "%s/%s", pBC->bc_FdCtrl.fc_Root, name );
	return( _f );
}

int
f_create( char* pFile )
{
	int	fd;

	if( (fd = open( F(pFile), O_RDWR|O_CREAT|O_EXCL|O_TRUNC, 0666 )) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
	return( fd );
err1:
	return( -1 );
}

int
f_open( char* pFile )
{
	int	fd;

	if( (fd = open( F(pFile), O_RDWR )) < 0 ) {
		neo_errno = unixerr2neo();
		goto err1;
	}
	return( fd );
err1:
	return( -1 );
}

int
f_unlink( char* pFile )
{
	int	Ret;

	Ret = unlink( F(pFile) );
	return( Ret );
}

#ifdef	ZZZ
DIR*
f_opendir()
{
	DIR	*pDir;

	pDir	= opendir( PathDB );
	return( pDir );
}

int
f_closedir( DIR *pDir )
{
	return( closedir( pDir ) );
}

struct dirent*
f_readdir( DIR *pDir )
{
	return( readdir( pDir ) );
}

void
f_rewinddir( DIR *pDir )
{
	return( rewinddir( pDir ) );
}

int
f_stat( char *pName, struct stat *pStat )
{
	return( stat( F(pName), pStat ) );
}
#endif	// ZZZ
