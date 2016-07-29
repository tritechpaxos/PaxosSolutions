/*******************************************************************************
 * 
 *  xj_swap.c --- xjPing (On Memory DB) memory manage Library programs
 * 
 *  Copyright (C) 2011,2014-2016 triTech Inc. All Rights Reserved.
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

#include	<stdio.h>
#include	<fcntl.h>

#include	"neo_db.h"

/* must be 32 bits */
typedef unsigned int	xj_swap_handle_t;

#define	SET_POOL( p, pool )	(*(p) |= (pool)<<28 )
#define	SET_SEQ( p, seq )	(*(p) |= (seq) )
#define	GET_POOL( p )		(((*(p))>>28)&0xf)
#define	GET_SEQ( p )		((*(p))&0xfffff)

typedef struct xj_swap_head {
	int	p_prev;		/* previous head */
	int	p_len;		/* next head (+ used/- unused ) */
} xj_swap_head_t;

typedef struct xj_swap_file {
	int	f_fd;		/* file descriptor */
	char	*f_name;	/* file name */
} xj_swap_file_t;
	
xj_swap_file_t	xj_swap[] = {
	{ -1, "XJ_SWAP1"},
};

#define	XJ_SWAP_FILE_MAX	(sizeof(xj_swap)/sizeof(xj_swap[0]))

int
xj_swap_init()
{
	int	i, j;
	int	fd;
	xj_swap_head_t
		head;

	for( i = 0; i < XJ_SWAP_FILE_MAX; i++ ) {
		unlink( xj_swap[i].f_name );

		fd = open( xj_swap[i].f_name, O_RDWR|O_CREAT|O_EXCL, 0666 );
		if( fd < 0 ) {
			goto err1;
		}
		xj_swap[i].f_fd = fd;
		bzero( &head, sizeof(head) );
		if( write( fd, &head, sizeof(head) ) < 0 ) {
			goto err1;
		}
	}
	return( 0 );
err1:
	for( j = 0; j < i; j++ ) {
		if( xj_swap[j].f_fd >= 0 ) {
			close( xj_swap[j].f_fd );
			xj_swap[j].f_fd = -1;
		}
	}
	return( -1 );
}

void
xj_swap_term()
{
	int	j;

	for( j = 0; j < XJ_SWAP_FILE_MAX; j++ ) {
		if( xj_swap[j].f_fd >= 0 ) {
			close( xj_swap[j].f_fd );
			xj_swap[j].f_fd = -1;
		}
	}
}

int
xj_swap_out( addrp, size, hp )
	char	*addrp;
	int	size;
	xj_swap_handle_t
		*hp;
{
	int	fd;
	xj_swap_head_t head;
	int	res;
	int	seq;

//DBG_PRT("OUT\n");
	if( xj_swap[0].f_fd < 0 ) {
		if( xj_swap_init() )
			goto err1;
	}

	*hp = 0;
	SET_POOL( hp, 0 );
	fd = xj_swap[0].f_fd;

	seq = 0;
	if( lseek( fd, (off_t)0, SEEK_SET ) )
		goto err1;
	while(1) {

		if( read( fd, &head, sizeof(head) ) < 0 )
			goto err1;

		if( head.p_len == 0 ) {	/* end */

			head.p_len	= size;
			lseek( fd, (off_t)(-sizeof(head)), SEEK_CUR );
			write( fd, &head, sizeof(head) );
			SET_SEQ( hp, seq );
			if( write( fd, addrp, size ) < 0 )
				goto err1;

			/* end */
			head.p_prev	= seq;
			head.p_len	= 0;
			write( fd, &head, sizeof(head) );

			break;

		} else if( head.p_len > 0 ) {	/* used */

			lseek( fd, (off_t)head.p_len, SEEK_CUR );
			seq += head.p_len + sizeof(head);

		} else {	/* empty */
			res = -head.p_len;
			if( size + 2*sizeof(head) > res ) { /* smaller */
			/* 2*sizeof(head) avoiding terminate confusion */

				lseek( fd, (off_t)res, SEEK_CUR );
				seq += res + sizeof(head);

				continue;
			}
			/* split */
			head.p_len	= size;
			lseek( fd, (off_t)(-sizeof(head)), SEEK_CUR );
			write( fd, &head, sizeof(head) );
			SET_SEQ( hp, seq );
			write( fd, addrp, size );

			head.p_prev	= seq;
			head.p_len	= -((res - size) - sizeof(head));
			seq += sizeof(head) + size;
			write( fd, &head, sizeof(head) );

			break;
		}
	}
	/*
	 *	Free !!!
	 */
	neo_free( addrp );
	return( 0 );
err1:
	return( -1 );
}

char*
xj_swap_in( hp, lenp )
	xj_swap_handle_t
		*hp;
	int	*lenp;
{
	char	*addrp;
	int	fd;
	int	seq, next_seq;
	xj_swap_head_t
		head, headw;

	fd = xj_swap[GET_POOL( hp )].f_fd;
	seq = GET_SEQ( hp );

	if( lseek( fd, (off_t)seq, SEEK_SET ) < 0 ) {
		goto err1;
	}
	if( read( fd, &head, sizeof(head) ) < 0 ) {
		goto err1;
	}
	if( !(addrp = neo_malloc( head.p_len ) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}
	if( read( fd, addrp, head.p_len ) < 0 ) {
		goto err2;
	}
	*lenp = head.p_len;

	next_seq = seq + sizeof(head) + head.p_len;

	/* prev concatination */
	head.p_len = -head.p_len;
	if( seq ) {
		if( lseek( fd, (off_t)head.p_prev, SEEK_SET ) < 0 ) {
			goto err2;
		}
		if( read( fd, &headw, sizeof(headw) ) < 0 ) {
			goto err2;
		}
		if( headw.p_len < 0 ) {
			seq	= head.p_prev;
			head.p_len += - sizeof(head) + headw.p_len;
		} 
	}

	/* next concatination */
	if( lseek( fd, (off_t)next_seq, SEEK_SET ) < 0 ) {
		goto err2;
	}
	if( read( fd, &headw, sizeof(headw) ) < 0 ) {
		goto err2;
	}
	if( headw.p_len < 0 ) {
		head.p_len	+= -sizeof(headw) + headw.p_len;
		if( lseek( fd, (off_t)(-headw.p_len), SEEK_CUR ) < 0 )
			goto err2;
		if( read( fd, &headw, sizeof(headw) ) < 0 ) {
			goto err2;
		}
	}
	headw.p_prev = seq;
	lseek( fd, (off_t)(-sizeof(headw)), SEEK_CUR );
	write( fd, &headw, sizeof(headw) );

	/* complete */
	lseek( fd, (off_t)seq, SEEK_SET );
	write( fd, &head, sizeof(head) );

	return( addrp );

err2:	neo_free( addrp );
err1:
	return( 0 );
}

int
xj_swap_find_free( td, s )
	r_tbl_t	*td;
	int	s;
{
	int	i;

	for( i = s; i < td->t_rec_num; i++ ) {
		if( td->t_rec[i].r_stat & R_REC_FREE )
			return( i );
	}
	return( -1 );
}

int
xj_swap_schedule( mytd )
	r_tbl_t*	mytd;
{
	r_tbl_t	*td;
	r_tbl_t	*nd;
	int	s, e, ee;
	int	h;
	int	i;

	for( td = (r_tbl_t*)_dl_head(&(_svc_inf.f_opn_tbl));
		_dl_valid(&(_svc_inf.f_opn_tbl),td); td = nd ) {

		nd      = (r_tbl_t *)_dl_next( td );

		if( td == mytd )
			continue;

		s = xj_swap_find_free( td, 0 );
		if( s < 0 )
			continue;
		do {
			e = xj_swap_find_free( td, s+1 );
			if( e < 0 )	ee = td->t_rec_num;
			else		ee = e;
			if( xj_swap_out( td->t_rec[s].r_head, 
					td->t_rec_size*(ee-s), &h ) < 0 ) {
				goto err1;
			}
			td->t_rec[s].r_head	= (r_head_t*)h;
			for( i = s; i < ee; i++ ) {
				td->t_rec[i].r_stat &= ~(R_REC_BUF|R_REC_CACHE);
				td->t_rec[i].r_stat |= R_REC_SWAP;
			}
			s = e;
		} while ( e > 0 );
	}
	return( 0 );

err1:
	return( -1 );

}

int
xj_swap_in_td( td, s, e )
	r_tbl_t	*td;
	int	s;
	int	e;
{
	int	i, j;
	int	len;
	char	*buffp;
	r_head_t	*rp;

	if( !(td->t_rec[s].r_stat & R_REC_FREE) ) {	/* search free point */
		for( ; s >= 0; s-- ) {
			if( td->t_rec[s].r_stat & R_REC_FREE )
				break;
		}
	}
	for( i = s; i <= e;) {
		if( td->t_rec[i].r_stat & R_REC_FREE
			&& td->t_rec[i].r_stat & R_REC_SWAP ) {

			buffp = xj_swap_in( &td->t_rec[i].r_head, &len );
			if( !buffp )
				goto err1;

			for( j = 0; j < len/td->t_rec_size; 
					j++, i++, buffp += td->t_rec_size ) {

				td->t_rec[i].r_head = (r_head_t*)buffp;
				td->t_rec[i].r_stat &= ~R_REC_SWAP;
				td->t_rec[i].r_stat |= R_REC_BUF|R_REC_CACHE;
				if( td->t_rec[i].r_stat & R_USED
						&& (rp = td->t_rec[i].r_head)->r_Cksum64 ) {
					assert( !in_cksum64( rp, td->t_rec_size, 0 ) );
				} 
			}
		} else {
			i++;
		}
	}
	return( 0 );
err1:
	return( -1 );
}
