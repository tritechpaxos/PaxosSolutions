/*******************************************************************************
 * 
 *  p_server.c --- xjPing (On Memory DB)  Server programs
 * 
 *  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
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
/******************************************************************************
 *	p_server.c
 *
 *      説明	サーバ専用プログラム
 ******************************************************************************/
#include	<stdio.h>
#include	<string.h>
#include	"neo_link.h"

int _p_ServerTCP( p_id_t* ad );

/*****************************************************************************
 * 関数名
 *		p_server()
 * 機能
 *		サーバプロトタイプ
 * 関数値
 *		 正常 	0
 *      	 異常  -1
 * 注意
 ******************************************************************************/
int
p_server( namep )
	p_name_t	namep;		/* サーバ用ポートネーム	*/
{
	p_id_t 		*ap;
	p_port_t	addr;
	int		err;

DBG_ENT( M_LINK, "p_server");

	/*
	 * 該当サーバの接続ポートのアドレスをネーム
	 * サーバに問い合わせ、取得する
	 */
	if( p_getaddrbyname( namep, &addr ) )
		goto err1;

	addr.p_role = P_SERVER;
	/*
	 * 接続用ポートを生成する
	 */
	if( !(ap = p_open( &addr ) ) )
		goto err1;
	
	/*
	 * 通信タイプにより分岐し、各自のサーバ処理を行う
	 */
	err = _p_ServerTCP( ap );
	if( err && err != E_SKL_TERM )
		goto	err2;

	/*
	 * 接続ポートをクローズする
	 */
	if( p_close( ap ) )
		goto err1;

DBG_EXIT( 0 );
	return( 0 );
err2:
	p_close( ap );
	neo_errno	= err;
err1:
DBG_EXIT( -1 );
	return( -1 );
}

static	fd_set	rfds,
		efds;

static struct {
	p_id_t	*s_pd;
} pds[FD_SETSIZE];


/*****************************************************************************
 * 関数名
 *		_p_ServerTCP()
 * 機能
 * 関数値
 *		 正常 	0
 *      	 異常  -1
 * 注意
 ******************************************************************************/
int
_p_ServerTCP( ad )
	p_id_t	*ad;		/* 接続用ポート記述子	*/
{
	int		afd;	/* connection descriptor */
	int		i;
	int		width, n;
	p_id_t		*new;
	p_head_t	head;
	int		headlen;
	char		cntlcode;
	int		err = 0 ;
	int		proc_err = 0 ;

DBG_ENT( M_LINK, "_p_ServerTCP" );

	/*
	 * ポート列リストの初期値として、接続ポートを登録する
	 */
	bzero( pds, sizeof(pds) );

	afd		= ad->i_rfd;
	pds[afd].s_pd	= ad;

loop:
	FD_ZERO( &rfds );
	FD_ZERO( &efds );
	for( i = 0; i < FD_SETSIZE; i++ ) {
		if( pds[i].s_pd ) {
			width = i;
			FD_SET( i, &rfds );

			/*
			 * ソケット記述子の緊急状態がまだクリアされていない
			 * と、例外チェックしない (tcpソケット記述子緊急状態
			 * は次回の受信イベントが発生しない限りに維持される)
			 */
			if ( !(pds[i].s_pd->i_stat & P_ID_EXCEPT) )
				FD_SET( i, &efds );
		}
	}

	/*
	 * ポート列リストの全てのポートで受信
	 * イベント及び例外イベントを待つ
	 */
	n = select( width+1, &rfds, 0, &efds, 0 );

	/*
	 * 待っているところでシグナルが発生する
	 * TERM以外のシグナルだったら、無視する
	 */
	if ( n < 0 && errno == EINTR )  {

DBG_C( ("_p_ServerTCP: EINTR\n") );
		if ( neo_isterm() ) {
			err = neo_errno;
			goto	err1;
		}

		goto  loop;
	
	} else if ( n < 0 )  {
		err = unixerr2neo();
		goto  err1;
	}

	/*
	 * 接続ポートで受信イベントが発生したら
	 * p_accept()にて新ポートを生成し、ポート列リストに登録する
	 */
	if( FD_ISSET( afd, &rfds ) ) {	/* connection */

DBG_C( ("_p_ServerTCP: accept\n") );
		if (  ( new = p_accept( ad ) ) != 0 )
		
			pds[new->i_rfd].s_pd	= new;

		FD_CLR( afd, &rfds );
	}

	/*
	 * 何れかのポートで例外イベントが発生したら、
	 * recv(... MSG_OOB)にて非同期コードを受け取って、
	 * アボート処理をしてから、ポートをクローズし、
	 * ポート列リストから削除する
	 */
	for( i = 0; i <= width; i++ ) {	/* asynchroneous */

		if( FD_ISSET( i, &efds ) ) {
			err = recv( pds[i].s_pd->i_rfd, &cntlcode, 1, MSG_OOB); 

			if ( err != 1 ) {
				err = unixerr2neo();
				goto	err1;
			}

DBG_C( ("_p_ServerTCP: CNTL setting\n") );

			pds[i].s_pd->i_stat |= P_ID_CNTL | P_ID_EXCEPT;
			pds[i].s_pd->i_cntl = cntlcode;

			p_abort( pds[i].s_pd ) ;

			pds[i].s_pd->i_stat &= ~P_ID_CONNECTED;
			p_close( pds[i].s_pd );
			pds[i].s_pd = 0 ;

			FD_CLR( i, &rfds );

		}
	}

	/*
	 * 接続ポート以外の何れかのポートで受信イベントが発生
	 * したら、受信パケットの種類によって、それぞれの処理を行う
	 */
	for( i = 0; i <= width; i++ ) {	/* receive data */
		if( FD_ISSET( i, &rfds ) ) {

			headlen = sizeof(head);

			/*
			 * 受信パケットのヘッダ部をピークする
			 */
			headlen = p_peek( pds[i].s_pd, &head, headlen, P_WAIT );

			if ( headlen < 0 ) {
				p_abort( pds[i].s_pd ) ;

				pds[i].s_pd->i_stat &= ~P_ID_CONNECTED;
DBG_C( ("_p_ServerTCP:_p_peek[%d]\n", err ));
				p_close(pds[i].s_pd);
				pds[i].s_pd = 0;
				continue;
			}

			/*
			 * RPC リンクファミリーの切断要求だったら、
			 * p_close()にてポートをクローズして、
			 * ポート列リストからポートを削除する
			 */
			if( (head.h_mode & P_PKT_REQUEST)
				&& (head.h_family == P_F_LINK ) ) {
				switch( head.h_cmd ) {
					case P_LINK_DIS:

						pds[i].s_pd->i_stat |= P_ID_DISCONNECT;
						p_close( pds[i].s_pd );
DBG_C( ("_p_ServerTCP:P_LINK_DIS p_close()\n"));

						pds[i].s_pd	= 0;
						break;
				}

			/*
			 * RPC リンクファミリー以外の要求パケット
			 * だったら、p_proc()で要求処理を行う
			 */
			} else if( head.h_mode & P_PKT_REQUEST ) {
DBG_C( ("_p_ServerTCP:P_PKT_REQUEST\n"));

				proc_err = p_proc( pds[i].s_pd ) ;

				if ( neo_errno == E_LINK_CNTL
					|| neo_errno == E_UNIX_EPIPE 
					|| neo_errno == E_UNIX_ECONNRESET  ) {

DBG_C( ("_p_ServerTCP:P_PKT_REQUEST[%s]\n", neo_errsym(neo_errno) ));
					p_abort(pds[i].s_pd);
					pds[i].s_pd->i_stat &= ~P_ID_CONNECTED;
					p_close(pds[i].s_pd);
					pds[i].s_pd = 0;

				}

			}
		}
	}

	goto loop;

err1:
DBG_EXIT(err);
	return err;
}

#ifdef	STATICS_OUT

sum_up( pd, ap )
p_id_t	*pd;
p_id_t	*ap;
{
	p_id_t	*p;

	sum_item_up( &pd->i_spacket, &ap->i_spacket);
	sum_item_up( &pd->i_rpacket, &ap->i_rpacket);
//	sum_item_up( &pd->i_sframe, &ap->i_sframe);
//	sum_item_up( &pd->i_rframe, &ap->i_rframe);

	for ( p = (p_id_t *)_dl_head( &_neo_port );
			_dl_valid( &_neo_port, p );
				p = (p_id_t *)_dl_next(p) ) {
		
		if ( p->i_stat & P_ID_CONNECTED )
			goto	ret;
	}

	static_out( ap );
ret:
	return;

}

sum_item_up( b, r )
struct i_static *b;
struct i_static *r;
{
	r->s_number += b->s_number;

	r->s_bytes  += b->s_bytes;

	r->s_time   += b->s_time;

	if ( r->s_max_bytes < b->s_max_bytes )
		r->s_max_bytes = b->s_max_bytes;

	if ( r->s_min_bytes > b->s_min_bytes )
		r->s_min_bytes = b->s_min_bytes;

	if ( r->s_max_time < b->s_max_time )
		r->s_max_time = b->s_max_time;

	if ( r->s_min_time > b->s_min_time )
		r->s_min_time = b->s_min_time;
}
#endif //STATICS_OUT

#ifdef MEMORY

mem_out()
{
	p_id_t	*p;

	for ( p = (p_id_t *)_dl_head( &_neo_port );
			_dl_valid( &_neo_port, p );
				p = (p_id_t *)_dl_next(p) ) {
		
		if ( p->i_stat & P_ID_CONNECTED )
			goto	ret;
	}

	printf("After running Allocated memory (%d)\n", _neo_debug_halloc() );
ret:
	return;

}
#endif
