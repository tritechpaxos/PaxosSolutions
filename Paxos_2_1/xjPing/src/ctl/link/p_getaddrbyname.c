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


#include	<string.h>

#include	"neo_link.h"

char	*getenv();

/*
 *	サービス名からポートアドレス、制御情報を取得する。
 *		暫定的にテーブルから取得する
 *
 */
int
p_getaddrbyname( name, port )
	p_name_t	name;
	p_port_t	*port;
{
	char		*sp;
	struct hostent	*hent;
	short		port_num;
	char		*host_name;

	DBG_ENT( M_LINK, "p_getaddrbyname");

	if( !name )	goto err1;

	if( (host_name = getenv( name )) ) {
		if( (sp = strchr( host_name, ':' )) ) {
			*sp = 0;
			port_num = atoi( sp+1 );
		} else {
			port_num = 4989;
		}
		if( !(hent = gethostbyname( host_name ) ) ) {
			goto err1;
		}
		bcopy( hent->h_addr, &port->p_addr.a_pa.sin_addr, hent->h_length );
	} else {
		port->p_addr.a_pa.sin_addr.s_addr	= htonl(INADDR_ANY);
		port_num = 0;
	}
DBG_A(("host_name[%s],port[%d]\n", host_name, port_num ));
	
DBG_A(("port_num=%x, htons(port_num)=%x\n", port_num, htons(port_num) ));
	port->p_addr.a_pa.sin_port	= htons(port_num);
	port->p_addr.a_pa.sin_family	= AF_INET;

	port->p_role		= P_CLIENT;	/* default */
	port->p_listen		= 5;		/* default */
	port->p_bufsize		= SOCK_BUF_MAX;
	port->p_max_data	= P_MAX_DATA;
	port->p_max_time	= P_MAX_TIME;
	port->p_min_time	= P_MIN_TIME;
	port->p_rcv_time	= P_MAX_TIME;
	port->p_max_retry	= P_MAX_RETRY;

	DBG_EXIT( 0 );
	return( 0 );
err1:
	neo_errno = unixerr2neo();
	DBG_EXIT( -1 );
	return( -1 );
}
