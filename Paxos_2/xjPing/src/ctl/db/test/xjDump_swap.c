/*******************************************************************************
 * 
 *  xjDump_swap.c --- xjPing (On Memory DB)  Test programs
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

#include	<stdio.h>
#include	<fcntl.h>
#include	<unistd.h>

typedef struct xj_swap_head {
        int     p_prev;         /* previous head */
        int     p_len;          /* next head (+ used/- unused ) */
} xj_swap_head_t;

int
main( ac, av )
	int	ac;
	char	*av[];
{
	int	fd;
	int	seq;
	xj_swap_head_t	head;

	if( (fd = open( av[1], O_RDWR, 0666 ) ) < 0 ) {
		printf("Can't open [%s]\n", av[1] );
		goto err1;
	}

	seq = 0;
	lseek( fd, (off_t)seq, SEEK_SET );

	while( 1 ) {
		if( read( fd, &head, sizeof(head) ) < 0 )
			goto err1;
		printf("%d:prev(%d)", seq, head.p_prev );
		if( head.p_len >= 0 ) {
			printf("used(%d)\n", head.p_len );
			if( head.p_len == 0 )
				break;
			lseek( fd, (off_t)head.p_len, SEEK_CUR );
			seq += sizeof(head) + head.p_len;
		} else {
			printf("empt(%d)\n", -head.p_len );
			lseek( fd, (off_t)(-head.p_len), SEEK_CUR );
			seq += sizeof(head) - head.p_len;
		}
	}
err1:;
	return( 0 );
}
