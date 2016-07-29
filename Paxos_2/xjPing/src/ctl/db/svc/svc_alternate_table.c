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
#include	"svc_logmsg.h"

#define	RECORDS	100

int
svc_alternate_table( table, item_num, itemsp )
	char		*table;
	int		item_num;
	r_item_t	*itemsp;
{
	r_tbl_t 	*tdf, *tdt;
	int		item_numf;
	r_item_t	*itemsfp;
	r_item_t	**offp;
	int		i, j, k, l;
	char		tmpname[128];
	char		*bufff, *bufft;
	char		*rf, *rt;

	DBG_ENT(M_RDB,"svc_alternate_table");
	/*
	 * テーブル記述子領域を生成する
	*/
	if( !(tdf = svc_tbl_alloc()) ) {
		neo_errno = E_RDB_NOMEM;
                goto err1;
        }

        /*
         * テーブルをオープンする
         */
        if( TBL_OPEN( tdf, table ) )
		goto err2;

	item_numf	= tdf->t_scm->s_n;
	itemsfp		= tdf->t_scm->s_item;

	if( !(offp = (r_item_t**)neo_zalloc( sizeof(r_item_t*)*item_numf ) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err3;
	}
	for( i = 0; i < item_numf; i++ ) {
		for( j = 0; j < item_num; j++ ) {
			if( !strcmp( itemsfp[i].i_name, itemsp[j].i_name) ) {
				offp[i] = &itemsp[j];
			}
		}
	}

	/* temporay table */
	sprintf( tmpname, "%s%ld", table, time(0) );
        if( TBL_CREATE( tmpname, R_TBL_NORMAL, (long)time(0),
                        tdf->t_rec_num, item_num, itemsp,
                        0, 0 ) ) {
                goto err4;
        }
	if( !(tdt = svc_tbl_alloc()) ) {
		neo_errno = E_RDB_NOMEM;
                goto err4;
        }
        if( TBL_OPEN( tdt, tmpname ) )
		goto err5;

	/* allocate buffers */
	if( !(bufff = neo_zalloc( RECORDS*tdf->t_rec_size ) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err6;
	}
	if( !(bufft = neo_zalloc( RECORDS*tdt->t_rec_size ) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err7;
	}
	for( i = 0; i < tdf->t_rec_num; i += RECORDS ) {

		if( i + RECORDS >= tdf->t_rec_num )	j = tdf->t_rec_num-i;
		else					j = RECORDS;

		if( TBL_LOAD( tdf, i, j, bufff ) ) {
			goto err8;
		}
		for( k = 0; k < j; k++ ) {
			rf = bufff + tdf->t_rec_size*k;		
			rt = bufft + tdt->t_rec_size*k;		

			bcopy( rf, rt, sizeof(r_head_t) );

			for( l = 0; l < item_numf; l++ ) {
				if( offp[l] ) {
					bcopy( rf + itemsfp[l].i_off,
						rt + offp[l]->i_off,
						itemsfp[l].i_size
							< offp[l]->i_size ? 
						itemsfp[l].i_size
							: offp[l]->i_size );
				}
			}
		}

		if( TBL_STORE( tdt, i, j, bufft ) ) {
			goto err8;
		}
	}
	tdt->t_rec_num 		= tdf->t_rec_num;
	tdt->t_rec_used		= tdf->t_rec_used;
	tdt->t_rec_ori_used	= tdf->t_rec_ori_used;
	tdt->t_unused_index	= tdf->t_unused_index;

	neo_free( bufft );
	neo_free( bufff );
	TBL_CLOSE( tdt );
	svc_tbl_free( tdt );
	neo_free( offp );
	TBL_CLOSE( tdf );
	svc_tbl_free( tdf );

	unlink( table );
	rename( tmpname, table );

DBG_EXIT(0);
	return( 0 );

err8:	neo_free( bufft );
err7:	neo_free( bufff );
err6:	TBL_CLOSE( tdt );
err5:	svc_tbl_free( tdt );
err4:	neo_free( offp );
err3:	TBL_CLOSE( tdf );
err2:	svc_tbl_free( tdf );
err1: 
DBG_EXIT(neo_errno);
	return( neo_errno );
}
