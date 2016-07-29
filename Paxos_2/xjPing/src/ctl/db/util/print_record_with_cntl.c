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
print_record_with_cntl( i, recp, rp, n, items )
	int		i;
	r_rec_t		*recp;
	r_head_t	*rp;
	int		n;
	r_item_t	items[];
{
	int	j;
	int	*bin;
	char	*str;

	printf("%d.[%c%c%c%c%c%c,a(%p),w(%p),h(%p) ",
		i,
		(recp->r_stat&R_INSERT ? 'I':'-'),
		(recp->r_stat&R_UPDATE ? 'U':'-'),
		(recp->r_stat&R_USED ? 'D':'-'),
		(recp->r_stat&R_BUSY ? 'B':'-'),
		(recp->r_stat&R_EXCLUSIVE ? 'E':'-'),
		(recp->r_stat&R_SHARE ? 'S':'-'),
		recp->r_access, recp->r_wait, recp->r_head );

	printf("[%"PRIi64",%"PRIi64",%c%c%c%c%c%c]", rp->r_rel, rp->r_abs,
		(rp->r_stat&R_INSERT ? 'I':'-'),
		(rp->r_stat&R_UPDATE ? 'U':'-'),
		(rp->r_stat&R_USED ? 'D':'-'),
		(recp->r_stat&R_BUSY ? 'B':'-'),
		(rp->r_stat&R_EXCLUSIVE ? 'E':'-'),
		(rp->r_stat&R_SHARE ? 'S':'-'));

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
				printf(" [%s]", str );
				break;
			case 6: /* long long */
				printf(" %lld", *(long long*)bin );
				break;
			case 7: /* unsigned long long */
				printf(" %llu", *(unsigned long long*)bin );
				break;
		}
	}
	printf("\n");
}
