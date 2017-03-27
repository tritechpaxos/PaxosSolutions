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
 *      neo_link.h
 *
 *      説明		neo_link  ヘッダファイル
 *
 ******************************************************************************/
#ifndef	_NEO_LINK_
#define	_NEO_LINK_

#ifndef NULL
#define	NULL	0
#endif
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>

#include	"neo_system.h"
#include	"neo_dlnk.h"
//#include	"neo_auth.h"
#include	"neo_link_err.h"
#include	"neo_skl.h"

#ifndef neo_auth_t
typedef long	neo_auth_t;
#endif

/* port name */
#ifdef NEO_NAME_MAX
#define	P_NAME_MAX	NEO_NAME_MAX		/* max length of port name  */
#else
#define	P_NAME_MAX	32
#endif

typedef	char	p_name_t[P_NAME_MAX];		/* type define of port name */

#define	P_CNTL_ABORT	128			/* abort code		    */
#define	P_NOWAIT	0			/* peek's nowait mode       */
#define	P_WAIT		1			/* peek"s wait mode	    */

#define	P_CLIENT	1			/* act as client	    */
#define	P_SERVER	2			/* act as server	    */

/* the following constant depends on system running on */
#define	SOCK_BUF_MAX	52000			/* maxsize of socket buffer */

#define	P_MAX_DATA	17 * 1024		/* maximum  length of frame */
#define	P_MAX_RETRY	6			/* maximum  times of retry  */
#define	P_MAX_TIME	24*60*60*1000		/* maximum value of timeout */
#define	P_MIN_TIME	1*1000			/* minimum value of timeout */
#define	ALIVE_PERRIOD	72111 			/* perriod for keep alive   */
typedef struct p_addr {
	int			a_id;		/* port ID	   	    */
	struct p_id	       *a_pd;		/* port descriptor	    */
	union {
		struct sockaddr_in	u_pa;		/* physical port address    */
		struct sockaddr_un	u_un;
	} a_addr;
#define	a_pa	a_addr.u_pa
#define	a_un	a_addr.u_un
} p_addr_t;
	

typedef struct p_port {

/* port identification part */
	p_addr_t		p_addr;		/* port identification part */

/* connection information   */
	int			p_listen;	/* back log        	    */
	int			p_role;		/* client or server         */

/* channel information      */
	int			p_max_data;	/* maximum data length 	    */
	int			p_max_retry;	/* maximum retry number     */
	long			p_max_time;	/* maximum timeout value(ms)*/
	long			p_min_time;	/* minimum timeout value(ms)*/
	long			p_cur_time;	/* current timeout value(ms)*/
	long			p_rcv_time;	/* timeout value(ms)by user */
	long			p_stime;	/* most recent time of send */
	long			p_rtime;	/* most recent time of recv */
	long			p_bufsize;	/* channel buffer size 	    */

/* authentication */
	neo_auth_t		p_auth;

} p_port_t;

/* port descriptor status */
#define	P_ID_UNUSED	0x00000000L		/* port unused		    */
#define	P_ID_CONNECTED	0x00010000L		/* port connected	    */
#define	P_ID_DISCONNECT	0x00020000L		/* disconnect req   arrived */
#define	P_ID_CNTL	0x00040000L		/* contrl  code  arrived    */
#define	P_ID_DOWN	0x00080000L		/* link down		    */
#define	P_ID_READY	0x00100000L		/* for connecting port only */
#define	P_ID_EXCEPT	0x00200000L		/* for connecting port only */

#define	P_PORT_EFFECTIVE	0x706f7274L	/* port linked by _neo_port */

typedef struct p_id {

/* control */
	_dlnk_t		i_lnk;			/* link for _neo_port	    */
	void		*i_tag;			/* work for r_port_t	    */
	long		i_id;			/* port of being linked     */
	long		i_stat;			/* port status		    */
	long		i_err;			/* error code		    */
	int		i_busy;			/* refer count		    */

/* connection */
	int		i_sfd;			/* send fd 		    */
	int		i_rfd;			/* recv fd 		    */
	int		i_cfd;			/* control fd 		    */
	p_port_t	i_local;		/* local port address	    */
	p_port_t	i_remote;		/* remote port address 	    */

/* data */
	int		i_cntl;			/* control code		    */
/* statics */
	struct i_static {
		long	s_number;		/* total number		    */
		long	s_bytes;		/* total bytes 		    */
		long	s_max_bytes;		/* maximum value 	    */
		long	s_min_bytes;		/* minimum value	    */
		long	s_time;			/* total time 	  	    */
		long	s_max_time;		/* maximum time 	    */
		long	s_min_time;		/* minimum time 	    */
	}
	                i_spacket;		/* send packet		    */
	struct i_static	i_rpacket;		/* receive packet 	    */

/* extend tag */
	void	*i_pTag;
	void	*i_pTag1;

} p_id_t;


#define	P_HEAD_TAG	0x70636b74L		/* pckt : packet head flag  */

/*packet mode */
#define	P_PKT_DATA	0x01			/* data packet		    */
#define	P_PKT_REQUEST	0x02			/* request packet	    */
#define	P_PKT_REPLY	0x04			/* reply packet		    */
#define	P_PKT_ACK	0x08	 		/* ack packet	    	*/
#define	P_PKT_NACK	0x10			/* nack 		    */

typedef struct p_head {
	long		h_tag;			/* head identifier	    */
	long		h_mode;			/* packet mode    	    */
	int		h_id;			/* packet ID 		    */
	int		h_seq;			/* packet sequence number   */
	int		h_err;			/* error code		    */
	int		h_len;			/* data length		    */
	int		h_family;		/* RPC family		    */
	int		h_vers;			/* RPC version 		    */
	int		h_cmd;			/* RPC command 		    */
	neo_auth_t	h_auth;			/* RPC authentication 	    */
} p_head_t;

/* macros */
#define	P_HEAD( bp )	((p_head_t*)(bp))
#define	P_DATA( bp )	(char*)((char*)(bp)+sizeof(p_head_t))

#define	P_F_MASK	0xffff0000L		/*only lower 2-byte for user*/

/* LINK protocol */
#define	P_F_LINK	0x6c696e6bL		/* link family		    */
#define	P_LINK_VER0	0			/* version for link family  */
#define	P_LINK_CON	(M_LINK<<24|1)		/* connection  command 	    */
#define	P_LINK_DIS	(M_LINK<<24|2)		/* disconnection command    */
#define	P_LINK_ABRT	(M_LINK<<24|3)		/* abnormal disconnection   */

/* connection protocol */
/* request */
typedef struct p_req_con {
	p_head_t	c_head;			/* header		    */
	p_port_t	c_local;		/* local port address	    */
	p_port_t	c_remote;		/* remote port address 	    */
} p_req_con_t;

/* response */
typedef struct p_res_con {
	p_head_t	c_head;			/* header		    */
	p_port_t	c_local;		/* local port address       */
	p_port_t	c_remote;		/* remote port address	    */
} p_res_con_t;


/* disconnect protocol */
typedef struct p_req_dis {
	p_head_t	d_head;			/* header		    */
	p_port_t	d_client;		/* local port address 	    */
	p_port_t	d_remote;		/* remote port address 	    */
} p_req_dis_t;

typedef struct p_res_dis {
	p_head_t	d_head;			/* header		    */
} p_res_dis_t;

#define	CONF_DB		"neo_conf.db"		/* configuration file name  */
#define	PORT_SERVER	"PORT_SERVER"		/* port name of name server */

#define	P_CLASS_PACKET	P_HEAD_TAG		/* packet class		    */
#define	P_CLASS_FRAME	P_FRAME			/* frame class		    */

//char	*_p_alloc(), *_p_zalloc();
//int	_p_free();

extern	int 	p_getaddrbyname( p_name_t, p_port_t* );

extern	int 	p_connect( p_id_t*, p_port_t* );
extern	int 	p_disconnect( p_id_t* );
extern	p_id_t	*p_open( p_port_t* );
extern	int		p_close( p_id_t *);
extern	p_id_t	*p_accept( p_id_t*);
extern	int		p_request( p_id_t*, p_head_t*, int );
extern	int		p_recv( p_id_t*, p_head_t*, int* );
extern	Msg_t*	p_recvByMsg( p_id_t* );
extern	int		p_reply( p_id_t*, p_head_t*, int );
extern	int		p_reply_free( p_id_t*, p_head_t*, int );
extern	int		p_response( p_id_t*, p_head_t*, int* );
extern	Msg_t*	p_responseByMsg( p_id_t* );
extern	int		p_write( p_id_t*, p_head_t*, int );
extern	int		p_read( p_id_t*, p_head_t*, int* );
extern	int		p_peek( p_id_t*, p_head_t*, int, int );
extern	int		p_cntl_send( p_id_t*, char );
extern	int		p_cntl_recv( p_id_t* );
extern	int		p_set_time( p_id_t*, long );
extern	int		p_select( p_id_t**, int, fd_set*, long );
extern	int		p_exit(void);

extern	int		_p_send( p_id_t*, p_head_t*, int );
extern	int		_p_recv( p_id_t*, char*, int*, int );
extern	int		_p_peek( p_id_t*, char*, int*, int );

extern _dlnk_t	_neo_port;
extern	int	_link_lock;


#define	_p_alloc	neo_malloc
#define	_p_zalloc	neo_zalloc
#define	_p_free		neo_free

extern	int	p_proc(p_id_t*);
extern	int	p_abort(p_id_t*);

extern	int	p_server(p_name_t);

#endif
