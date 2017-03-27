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
 *      p_raw.c
 *
 *      説明    RAW I/F プログラム
 *
 ******************************************************************************/

#include	<string.h>
#include	"neo_link.h"
#include	<limits.h>
#include	<sys/uio.h>


/*****************************************************************************
 * 関数名
 *		_p_send()
 * 機能
 *		パケット送信及びACK受信
 * 関数値
 *		0	正常
 *		0以外   エラー番号
 * 注意
 ****************************************************************************/
int
_p_send( pd, packet, len )
	p_id_t		*pd;	 	/* ポート記述子		*/
	p_head_t	*packet;	/* 送信されるパケット	*/
	int		len;		/* パケットサイズ	*/
{
	int		err;

#ifdef	STATICS
	long		begin;
	long		time_cost;
	int		length;
#endif

DBG_ENT( M_LINK, "_p_send" );
//DBG_PRT( "_p_send\n" );

	/*
	 * ポート状態チェック
	 */
	if( pd->i_stat & P_ID_DOWN ) {
		err = E_LINK_DOWN;
		goto err1;
	}
	if( pd->i_stat & P_ID_CNTL ) {
		err = E_LINK_CNTL;
		goto err1;
	}

	/*
	 * パケット送信
	 */
	if( (err = SendStream( pd->i_rfd, (char*)packet, len ) ) ) {
		goto err1;
	}

#ifdef	STATICS
	begin	= time(0);
#endif

#ifdef STATICS

	time_cost	= time(0) - begin ;
	length		= sizeof(head);
	pd->i_rpacket.s_number ++;
	pd->i_rpacket.s_bytes += length;

	if (length > pd->i_rpacket.s_max_bytes )
		pd->i_rpacket.s_max_bytes = length;

	if (length < pd->i_rpacket.s_min_bytes )
		pd->i_rpacket.s_min_bytes = length;

	pd->i_rpacket.s_time += time_cost;

	if ( time_cost > pd->i_rpacket.s_max_time )
		pd->i_rpacket.s_max_time = time_cost ;

	if ( time_cost < pd->i_rpacket.s_min_time )
		pd->i_rpacket.s_min_time = time_cost ;

#endif // STATICS

err1:

DBG_EXIT( err );
	return( err );
}


/*****************************************************************************
 * 関数名
 *		_p_recv()
 * 機能
 *		パケット受信及びACK送信
 * 関数値
 *		0	正常
 *		0以外   エラー番号
 * 注意
 *		接続要求フレームの多重受信はここでチェックする
 ****************************************************************************/
int
_p_recv( pd, packet, lenp, mode )
	p_id_t		*pd;		/* ポート記述子		*/
	char		*packet;	/* バッファ		*/
	int		*lenp;		/* バッファサイズ	*/
	int		mode;		/* 受信パケットモード	*/
{
	int		err ,error = 0 ;

#ifdef STATICS
	long		begin;
	long		time_cost;
	int		length;
#endif // STATICS

DBG_ENT( M_LINK, "_p_recv") ;
//DBG_PRT( "_p_recv\n" );

#ifdef STATICS
	begin = time(0);
#endif // STATICS
	/*
	 * ポート状態チェック
	 */
	if( pd->i_stat & P_ID_DOWN ) {
		err = E_LINK_DOWN;
		goto err1;
	}

	/*
	 * パケット受信待ち
	 */
	p_head_t	head;

	if( pd->i_stat & P_ID_CNTL ) {
		error = E_LINK_CNTL;
		goto err1;
	}
	err = PeekStream( pd->i_rfd, (char*)&head, sizeof(head) );
//DBG_PRT("PEEK err=%d\n", err );
	if( err < 0 ) {
		err = errno;
		goto err1;
	}
	if( *lenp < sizeof(head) + head.h_len ) {
		err = E_LINK_BUFSIZE ;
//DBG_PRT("BUFF *lenp=%d h_len=%d\n", *lenp, head.h_len );
		goto err1;
	}
	err = RecvStream( pd->i_rfd, packet, sizeof(head) + head.h_len );
//DBG_PRT("RECV err=%d\n", err );
	if( err < 0 ) {
		err = errno;
		goto err1;
	}

#ifdef STATICS

	time_cost	= time(0) - begin ;

	pd->i_rpacket.s_number ++;
	pd->i_rpacket.s_bytes += length;

	if (length > pd->i_rpacket.s_max_bytes )
		pd->i_rpacket.s_max_bytes = length;

	if (length < pd->i_rpacket.s_min_bytes )
		pd->i_rpacket.s_min_bytes = length;

	pd->i_rpacket.s_time += time_cost;

	if ( time_cost > pd->i_rpacket.s_max_time )
		pd->i_rpacket.s_max_time = time_cost ;

	if ( time_cost < pd->i_rpacket.s_min_time )
		pd->i_rpacket.s_min_time = time_cost ;

#endif // STATICS

	err = error ;
err1:

DBG_EXIT( err );
	return( err );
}


/*****************************************************************************
 * 関数名
 *		_p_peek()
 * 機能
 *		パケットピーク
 * 関数値
 *		0	正常
 *		0以外   エラー番号
 * 注意
 ****************************************************************************/
int
_p_peek( pd, packet, lenp, mode )
	p_id_t	*pd;		/* ポート記述子		*/
	char	*packet;	/* バッファ		*/
	int	*lenp;		/* バッファサイズ	*/
	int	mode;		/* ピークするパケットモード*/
{
	int		err;

DBG_ENT( M_LINK, "_p_peek");
//DBG_PRT( "_p_peek\n" );

	/*
	 * ポート状態チェック
	 */
	if( pd->i_stat & P_ID_DOWN ) {
		err = E_LINK_DOWN;
		goto err1;
	}

	/*
	 * 受信キューチェック及びP_WAITモード時
	 * の受信待ち
	 */
	int	len = *lenp;

	if( pd->i_stat & P_ID_CNTL ) {
		err = E_LINK_CNTL;
		goto err1;
	}
	err = PeekStream( pd->i_rfd, packet, len );
//DBG_PRT("len=%d errno=%d\n", len, errno );
	if( err < 0 ) {
		err = errno;
		goto err1;
	}

DBG_EXIT( 0 );
	return( 0 );
err1:

DBG_EXIT( err );
	return( err );
}


