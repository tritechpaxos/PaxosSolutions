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


#define	F_ULOCK 0
#define	F_LOCK 1
#define	F_TLOCK 2
#include	<fcntl.h>
#include	<unistd.h>

/******************************************************************************
 *
 *         
 *	skl_lock.c
 *
 *	説明	スケルトン 排他制御
 * 
 ******************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include	"skl_lock.h"

/*
 *	排他制御識別子 管理リスト エントリ
 */
static _dlnk_t	_lock_entry;

int _neo_lockf( lk_id_t*pt, int cmd );

/******************************************************************************
 * 関数名
 *		_neo_init_lock()
 * 機能        
 *		排他制御 初期化
 * 関数値
 *	無し
 * 注意
 ******************************************************************************/
void
_neo_init_lock()
{
DBG_ENT( M_SKL, "_neo_init_lock" );

	_dl_init( &_lock_entry );

DBG_EXIT(0);
}


/******************************************************************************
 * 関数名
 *		neo_create_lock()
 * 機能        
 *		排他制御識別子の作成
 * 関数値
 *      正常: !NULL
 *      異常: NULL
 * 注意
 ******************************************************************************/
lk_id_t *
neo_create_lock( resource )
char	*resource;	/* I ファイル名	*/
{
	lk_id_t	*pt;
	int	fd;
	char	path[NEO_MAXPATHLEN+20];
	char	file[NEO_MAXPATHLEN], *pc;

DBG_ENT( M_SKL, "neo_create_lock" );
DBG_C(( "resource = (%s)\n", resource ));

	/*
	 *	引数チェック
	 */
	if( !resource || !(*resource) || (strlen(resource) > NEO_MAXPATHLEN) ) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	/*
	 *	ファイル名作成
	 */
	for( pc = file; *resource; pc++, resource++ ) {
		if( *resource == '/' )
			*pc = '#';
		else
			*pc = *resource;
	}
	*pc = '\0';

	sprintf( path, "%s/%s/%s", getenv("HOME"), NEO_LOCKDIR, file );

        for( pt = (lk_id_t *)_dl_head( &_lock_entry );
			_dl_valid( &_lock_entry, pt );
			pt = (lk_id_t *)_dl_next( pt ) ) {

		if( !strcmp( pt->resource, path ) ) {
			neo_errno = E_SKL_INVARG;
			goto err;
		}
	}

	/*
	 *	排他識別子の作成
	 */
	if( !(pt = (lk_id_t *)neo_malloc( sizeof(lk_id_t) )) ) {
		goto err;
	}

	/*
	 *	ロックファイルのオープン
	 */
	while( 1 ) {
		if( (fd = open( path, O_RDWR|O_CREAT|O_SYNC, 0644 )) < 0 ) {
			if( errno == EINTR ) {
				continue;
			} else {
				goto uerr;
			}
		}
		break;
	}

	/*
	 *	ロックファイルへのダミー書き込み
	 */
	while( 1 ) {
		if( write( fd, "dummy", 5 ) < 0 ) {
			if( errno == EINTR ) {
				continue;
			} else {
				goto uerr;
			}
		}
		break;
	}

	strcpy( pt->resource, path );
	pt->fd   = fd;

        _dl_insert( pt, &_lock_entry );

DBG_EXIT(pt);
	return( pt );
uerr:;
	/* 
	 * UNIX ERR 
	 */
	neo_errno = unixerr2neo();
	neo_free( pt );
err:;
DBG_EXIT(0);
	return( (lk_id_t *)NULL );
}


/******************************************************************************
 * 関数名
 *		neo_delete_lock()
 * 機能        
 *		排他制御識別子の削除
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_delete_lock( lk_id )
lk_id_t	*lk_id;		/* I 排他制御識別子	*/
{
	lk_id_t	*pt;

DBG_ENT( M_SKL, "neo_delete_lock" );
DBG_C(( "lk_id = (%#x)\n", lk_id ));

        for( pt = (lk_id_t *)_dl_head( &_lock_entry );
			_dl_valid( &_lock_entry, pt );
			pt = (lk_id_t *)_dl_next( pt ) ) {

		if( pt == lk_id ) {
			_dl_remove( pt );
			while( close( pt->fd ) ) {
				if( errno == EINTR ) {
					continue;
				} else {
					neo_errno = unixerr2neo();
					goto err;
				}
			}
			neo_free( pt );
			goto ok;
		}
	}

	neo_errno = E_SKL_INVARG;
	goto err;

ok:;
DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}


/******************************************************************************
 * 関数名
 *		neo_hold_lock()
 * 機能        
 *		排他の宣言
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_hold_lock( lk_id, wflag )
lk_id_t	*lk_id;		/* I 排他制御識別子	*/
int	wflag;		/* I 待ちフラグ		*/
{
	lk_id_t	*pt;

DBG_ENT( M_SKL, "neo_hold_lock" );
DBG_C(( "lk_id = (%#x), waitflag = (%d)\n", lk_id, wflag ));

	/*
	 *	引数のチェック
	 */
	if( wflag != NEO_WAIT && wflag != NEO_NOWAIT ) {
		neo_errno = E_SKL_INVARG;
		goto err;
	}

	/*
	 *	ロック
	 */
        for( pt = (lk_id_t *)_dl_head( &_lock_entry );
			_dl_valid( &_lock_entry, pt );
			pt = (lk_id_t *)_dl_next( pt ) ) {

		if( pt == lk_id ) {
			lseek( pt->fd, 0L, SEEK_SET );
			/*
			 *	ロック(待ち有り)
			 */
			if( wflag == NEO_WAIT ) {
				if( _neo_lockf(pt, F_LOCK) )
					goto err;
			/*
			 *	ロック(待ち無し)
			 */
			} else {
				if( _neo_lockf(pt, F_TLOCK) )
					goto err;
			}
			goto ok;
		}
	}

	neo_errno = E_SKL_INVARG;
	goto err;
	
ok:;
DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}

int
_neo_lockf( pt, cmd )
lk_id_t	*pt;
int	cmd;
{
	int	ret = 0;
	while( lockf( pt->fd, cmd, 0L ) ) {
		if( errno == EINTR ) {
			if( neo_isterm() )
				goto err;
			else
				continue;
		} else {
			neo_errno = unixerr2neo();
			goto err;
		}
	}

end:;
	return( ret );
err:;
	ret = -1;
	goto end;
}

/******************************************************************************
 * 関数名
 *		neo_release_lock()
 * 機能        
 *		排他の解除
 * 関数値
 *      正常: 0
 *      異常: -1
 * 注意
 ******************************************************************************/
int
neo_release_lock( lk_id )
lk_id_t	*lk_id;		/* I 排他制御識別子	*/
{
	lk_id_t	*pt;

DBG_ENT( M_SKL, "neo_release_lock" );
DBG_C(( "lk_id = (%#x)\n", lk_id ));

	/*
	 *	アンロック
	 */
        for( pt = (lk_id_t *)_dl_head( &_lock_entry );
			_dl_valid( &_lock_entry, pt );
			pt = (lk_id_t *)_dl_next( pt ) ) {

		if( pt == lk_id ) {
			lseek( pt->fd, 0L, SEEK_SET );
			while( lockf( pt->fd, F_ULOCK, 0L ) ) {
				if( errno == EINTR ) {
					continue;
				} else {
					neo_errno = unixerr2neo();
					goto err;
				}
			}
			goto ok;
		}
	}

	neo_errno = E_SKL_INVARG;
	goto err;
	
ok:;
DBG_EXIT(0);
	return( 0 );
err:;
DBG_EXIT(-1);
	return( -1 );
}

