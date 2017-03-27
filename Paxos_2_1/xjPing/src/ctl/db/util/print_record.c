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

#include	"neo_db.h"
#include	<inttypes.h>

void
print_record( i, rp, n, items )
	int		i;
	r_head_t	*rp;
	int		n;
	r_item_t	items[];
{
	int	j;
	int	*bin;
	char	*str;
	char	*cp;
	int	k;

	DBG_B(("%d.[%"PRIi64",%"PRIi64",%c%c%c(0x%"PRIx64")]", 
		i, rp->r_rel, rp->r_abs,
		(rp->r_stat&0x01 ? 'I':'-'),
		(rp->r_stat&0x02 ? 'R':'-'),
		(rp->r_stat&0x04 ? 'U':'-'),
		rp->r_stat ));
	for( j = 0; j < n; j++ ) {
		str = (char *)((char*)rp+items[j].i_off);
		bin = (int *)((char*)rp+items[j].i_off);
		switch( items[j].i_attr ) {
			case 1:	/* hex */
				printf(" 0x%x", *bin );
				break;
			case 2: /* int */
				printf(" %d", *bin );
				break;
			case 3: /* unsigned int */
				printf(" %u", *(unsigned int *)bin );
				break;
			case 4: /* float */
				printf(" %f", *(float*)bin );
				break;
			case 5: /* str */
				putchar(' '); putchar('[');
				for( k = 0, cp = str; k < items[j].i_len; 
								k++, cp++ ) {
					if( *cp == 0 )	break;
					else		putchar( *cp );
				}
				putchar(']');
/***
				if( k < items[j].i_len ) {
					printf("-%x", *++cp & 0xff );
				}
				printf(" [%s]", str );
***/
				break;
			case 6: /* long long */
				printf(" %lli", *(long long*)bin );
				break;
			case 7: /* unsigned long long */
				printf(" %llu", *(unsigned long long*)bin );
				break;
			case 9: /* bytes */
				printf(" %s", str);
				break;
		}
	}
	printf("\n");
}
