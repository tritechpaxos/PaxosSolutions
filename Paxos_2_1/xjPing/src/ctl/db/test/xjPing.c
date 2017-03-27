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
 *	main.c
 *
 *	説明	server   メインプログラム
 * 
 ******************************************************************************/
				/* Designed by N.Watanabe */

#include	"neo_db.h"
#include	"../svc/svc_logmsg.h"
#include	"../sql/sql.h"
#include	"../dbif/TBL_RDB.h"

r_svc_inf_t	_svc_inf;
char		*svc_myname;

/*****************************************************************************
 *
 *  関数名	neo_main()
 *
 *  機能	サーバプログラムエントリ
 * 
 *  関数値
 *     		無し
 *
 ******************************************************************************/
struct Log	*pLog = NULL;

int
neo_main( ac, av )
	int	ac;	/* サーバプロセス起動時のプログラム引数の数 */
	char	*av[];	/* サーバプロセス起動時のプログラム引数 */
{
	BlockCacheCtrl_t	*pB;

DBG_ENT(M_RDB,"neo_main");
DBG_A(("sbrk(0)=0x%x\n", sbrk(0) ));
DBG_A(("sizeof(r_usr_t)=%d,sizeof(r_expr_t)=%d\n", 
		sizeof(r_usr_t), sizeof(r_expr_t)));

	pLog = LogCreate( 0, Malloc, Free, 0, stdout, 5 );
	/*
	 * 引数の数チェック
	 */
	if( ac < 3 ) {
printf("usage:xjPing Database FilePath\n");
printf("\txjPing\t\t--- load module name\n");
printf("\tDatabase\t--- database name defined by enviroment variable\n");
printf("\t\t\t\tDatabase=host:port\n");
printf("\tFilePath\t--- DBMS directory\n");
/***********************************************
printf("\tMaxMemory\t--- cache memory size\n");
***********************************************/
		goto err1;
	}

	/*
	 * 初期化
	 */
	/**************************************************
	if( ac >= 4 ) {
		m = atoi( av[3] );
	}
	if( ac >= 4 && m > 0 )	xj_heap_max( m*1024 );
	else			xj_heap_max(0);
	***************************************************/

	MtxInit( &_svc_inf.f_Mtx );
	HashInit( &_svc_inf.f_HashOpen, PRIMARY_1009, HASH_CODE_STR, HASH_CMP_STR,
					NULL, neo_malloc, neo_free, NULL );

	_dl_init( &_svc_inf.f_opn_tbl );
	_svc_inf.f_stat = R_SERVER_ACTIVE;
	strcpy(_svc_inf.f_path, av[2] ) ;
	svc_myname = av[1];
	getcwd( (char*)&_svc_inf.f_root, sizeof(_svc_inf.f_root) );

	_svc_inf.f_port	= &_neo_port;

	lex_init();
	sql_lex_init();

	if( !(pB = (BlockCacheCtrl_t*)TBL_DBOPEN( av[2] ) ) ) {
printf("Database Open Error[%s]\n", av[2] );
		goto err2;
	}
	_svc_inf.f_pFileCache	= pB;
#define	FD_MAX_20		20
#define	BLOCK_MAX_2000	2000
#define	SEG_SIZE_32K	(1024*32)
#define	SEG_NUM_16		16
#define	USEC_WB			(1000*100)
	if( FileCacheInit( pB, FD_MAX_20, av[2], BLOCK_MAX_2000, 
				SEG_SIZE_32K, SEG_NUM_16, USEC_WB, TRUE, pLog ) < 0 ) {
		goto err2;
	}

	/*
	 * サーバスケルトン
	 */
	if( p_server( av[1] ) )	{
printf("Port Open Error[%s]\n", av[1] );
		goto err1;
	}

	/*
	 *
	 */
	TBL_DBCLOSE( av[2] );
DBG_EXIT(0);
printf("### xjPing Normal EXIT\n");
	return( 0 );

err1:
	TBL_DBCLOSE( av[2] );
DBG_EXIT(1);
printf("### xjPing Error EXIT(1)\n");
	neo_exit( 1 );
err2:
DBG_EXIT(2);
printf("### xjPing Error EXIT(2)\n");
	neo_exit( 2 );
return( -1 );
}


/*****************************************************************************
 *
 *  関数名	p_proc()
 *
 *  機能	サーバスケルトンコールバック: サービス処理
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
p_proc( pd )
	p_id_t	*pd;		/* 通信ポート記述子	*/
{
	p_head_t	head;
	int		err;
	int		len;

DBG_ENT(M_RDB,"p_proc");

	/*
	 * サービス要求をピークする
	 */
	p_peek( pd, &head, sizeof(head), P_NOWAIT );

	if ( ( head.h_family != P_F_RDB ) || ( head.h_vers != R_VER0 ) ) {

		len = sizeof(head);
		p_recv( pd, &head, &len) ;
		err = E_RDB_PROTOTYPE ;
		err_reply( pd, &head, err );
		neo_rpc_log_err( M_RDB, svc_myname, &( PDTOENT(pd)->p_clnt ), 
								MAINERR01, neo_errsym() );
		goto	err1;

	}

	/*
	 * サービス要求タイプより分岐して、それぞれ
	 * の対応処理を行う
	 */
	switch( head.h_cmd ) {
		case R_CMD_LINK:	err = svc_link( pd, &head );	break;
		case R_CMD_UNLINK:	err = svc_unlink( pd, &head );	break;
		case R_CMD_CREATE:	err = svc_create( pd, &head );	break;
		case R_CMD_DROP:	err = svc_drop( pd, &head );	break;
		case R_CMD_OPEN:	err = svc_open( pd, &head );	break;
		case R_CMD_CLOSE:	err = svc_close( pd, &head );	break;
		case R_CMD_FLUSH:	err = svc_flush( pd, &head );	break;
		case R_CMD_CANCEL:	err = svc_cancel( pd, &head );	break;
		case R_CMD_INSERT:	err = svc_insert( pd, &head );	break;
		case R_CMD_DELETE:	err = svc_delete( pd, &head );	break;
		case R_CMD_UPDATE:	err = svc_update( pd, &head );	break;
		case R_CMD_SEARCH:	err = svc_search( pd, &head );	break;
		case R_CMD_FIND:	err = svc_find( pd, &head );	break;
		case R_CMD_REWIND:	err = svc_rewind( pd, &head );	break;
		case R_CMD_SEEK:	err = svc_seek( pd, &head );	break;
		case R_CMD_SORT:	err = svc_sort( pd, &head );	break;
		case R_CMD_HOLD:	err = svc_hold( pd, &head );	break;
		case R_CMD_RELEASE:	err = svc_release( pd, &head );	break;
		case R_CMD_HOLD_CANCEL:	err = svc_hold_cancel( pd, &head );
									break;
		case R_CMD_COMPRESS:	err = svc_compress( pd, &head );break;
		case R_CMD_EVENT:	err = svc_event( pd, &head );	break;
		case R_CMD_EVENT_CANCEL:err = svc_event_cancel( pd, &head );
									break;
		case R_CMD_DIRECT:	err = svc_direct( pd, &head );	break;
		case R_CMD_REC_NUMS:	err = svc_number( pd, &head );	break;
		case R_CMD_STOP:	err = svc_stop( pd, &head );	break;
		case R_CMD_SHUTDOWN:	
					err = svc_stop( pd, &head );	
					neo_exit( 0 );

		case R_CMD_RESTART:	err = svc_restart( pd, &head );	break;
		case R_CMD_LIST:	err = svc_list( pd, &head );	break;
		

		case R_CMD_DUMP_PROC:	err = svc_dump_proc( pd, &head );break;
		case R_CMD_DUMP_INF:	err = svc_dump_inf( pd, &head );break;
		case R_CMD_DUMP_RECORD:	err = svc_dump_record( pd, &head );
									break;
		case R_CMD_DUMP_TBL:	err = svc_dump_tbl( pd, &head );break;
		case R_CMD_DUMP_USR:	err = svc_dump_usr( pd, &head );break;
		case R_CMD_DUMP_MEM:	err = svc_dump_mem( pd, &head );break;
		case R_CMD_DUMP_CLIENT:	err = svc_dump_client(pd, &head);break;

		case R_CMD_SQL:		err = svc_sql(pd, &head);	break;
        case R_CMD_FILE:	err = svc_file( pd, &head );	break;
        case R_CMD_COMMIT:	err = svc_commit( pd, &head );	break;
        case R_CMD_ROLLBACK:err = svc_rollback( pd, &head );	break;
		
		/* dummy */
			case R_CMD_DUMP_LOCK:
				len = sizeof(head);
				p_recv( pd, (p_head_t*)&head, &len );
				p_reply( pd, &head, sizeof(head) );
				break;

			case R_CMD_DUMP_UNLOCK:
				len = sizeof(head);
				p_recv( pd, (p_head_t*)&head, &len );
				p_reply( pd, &head, sizeof(head) );
				break;

		default:
			len = sizeof(head);
			p_recv( pd, &head, &len );
			err_reply( pd, &head, E_RDB_INVAL );
			neo_rpc_log_err( M_RDB, svc_myname, 
						&( PDTOENT(pd)->p_clnt ),
						   MAINERR02, neo_errsym() );
			goto err1;
	}
	if( err )
		goto err1;
DBG_EXIT(0);
	return( 0 );
err1:
DBG_C(("err=0x%x[%s]\n", err, neo_errsym() ));
DBG_EXIT( err );
	return( err  );
}

/*****************************************************************************
 *
 *  関数名	p_abort()
 *
 *  機能	サーバスケルトンコールバック: アボート処理
 * 
 *  関数値
 *     		0
 *
 ******************************************************************************/
int
p_abort( pd )
	p_id_t	*pd;		/* 通信ポート記述子	*/
{
	int		cntl;

DBG_ENT(M_RDB,"p_abort");

	/*
	 * 非同期コードチェック
	 */
	cntl = p_cntl_recv( pd );

	if( cntl != -1 )
		cntl &= 0xff;

	DBG_A(("pd=0x%x,cntl=0x%x\n", pd, cntl ));
	LOG( neo_log, LogDBG, "cntl=0x%x", cntl );

	if( pd->i_tag )
		neo_rpc_log_err( M_RDB, svc_myname, 
				&(PDTOENT(pd)->p_clnt),
				MAINERR03, /*cntl,*/ neo_errsym() );

	switch( cntl ) {
		case P_CNTL_ABORT:	/* abort message */
			svc_garbage( pd );
			break;
		case -1:		/* link down */
			svc_garbage( pd );
			break;
	}

DBG_EXIT(0);
	return( 0 );
}
