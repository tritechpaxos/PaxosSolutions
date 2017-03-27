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

#include	"hash.h"
#include	"../sql/sql.h"

extern r_expr_t	*sql_getnode();
extern	int sql_not_inner_join( r_expr_t *ep );

h_elm_t* xj_toArray();

uint64_t
hash_value( bufp, len, flg )
	unsigned char	*bufp;
	int		len;
{
	uint64_t	s = 0;
	uint64_t	v;
	int		i, n8;
	int		r;
	unsigned char	*cp;

	if( flg ) {
		for( i = 0, cp = bufp; i < len ; i++, cp++ ) {
			if( *cp == 0 )	break;
		}
		len = i;
	}
	n8 = len/sizeof(s);
	r  = len - n8*sizeof(s);

	for( i = 0; i < n8; i++, bufp += sizeof(s) ) {
		v = *((uint64_t*)bufp);
		s += v;
	}
	if( r ) {
		v = 0;
		bcopy( bufp, &v, r );
		s += v;
	}
	return( s );
}

h_head_t*
hash_create( int Size )
{
	h_head_t	*pH;

	pH = (h_head_t*)neo_zalloc( sizeof(*pH)+sizeof(h_entry_t)*(Size-1) );
	if( !pH ) {
		neo_errno	= E_RDB_NOMEM;
		goto err;
	}
	pH->h_hash_size	= Size;
	return( pH );
err:
	return( NULL );
}

h_entry_t*
hash_get_ent( h_head_t *pH, uint64_t lValue )
{
	int			ind;
	h_entry_t	*pEnt;

	ind		= lValue % pH->h_hash_size;
	pEnt	= &pH->h_value[ind];
	return( pEnt );
}

h_entry_t*
hash_get_entry( h_head_t *pH, uint64_t lValue )
{
	h_entry_t	*pEnt;

	do {
		pEnt	= hash_get_ent( pH, lValue );
		pH		= pEnt->ent_hash;
	} while( pH );
	return( pEnt );
}

int
hash_put_elm( h_head_t *pH, uint64_t lValue, int rInd )
{
	int			i, i2;
	h_entry_t	*pEnt, *pEnt2;
	h_elm_t		*pElm;
	h_head_t	*pH2;
	h_elm_t		*array;
	int			ind;

	/* 1-st hash */
	pEnt	= hash_get_ent( pH, lValue );
	if( !pEnt->ent_array && !pEnt->ent_hash ) {
		pEnt->ent_array	= (h_elm_t*)neo_zalloc( sizeof(h_elm_t)*HASH_LENGTH );
		if( !pEnt->ent_array ) {
			neo_errno	= E_RDB_NOMEM;
			goto err;
		}
		pEnt->ent_size	= HASH_LENGTH;
	}
	/* 2-nd hash */
	if( pEnt->ent_cnt >= pEnt->ent_size 
				&& !pEnt->ent_hash ) {
		/* init */
		ind	= pEnt - &pH->h_value[0];
		pH2				= hash_create( HASH_SIZE_2 );
		pEnt->ent_hash	= pH2;
		if( !pH2 ) {
			neo_errno	= E_RDB_NOMEM;
			goto err;
		}
		sprintf( pH2->h_name, "%d", ind );
		for( i = 0; i < pEnt->ent_cnt; i++ ) {	// move
			pElm = &pEnt->ent_array[i];
			pEnt2	= hash_get_ent( pH2, pElm->e_lValue );
			if( !pEnt2->ent_array ) {
				pEnt2->ent_array	= (h_elm_t*)
					neo_zalloc( sizeof(h_elm_t)*HASH_LENGTH );
				if( !pEnt2->ent_array ) {
					neo_errno	= E_RDB_NOMEM;
					goto err;
				}
				pEnt2->ent_size	= HASH_LENGTH;
			}
			pEnt2->ent_array[pEnt2->ent_cnt++]	= *pElm;
		}
		neo_free( pEnt->ent_array );
		pEnt->ent_array = NULL;
		pEnt->ent_size	= 0;
	}
	if( pEnt->ent_hash ) {
		pH2	= pEnt->ent_hash;
		pEnt2	= hash_get_ent( pH2, lValue );
		if( !pEnt2->ent_array ) {
			array = (h_elm_t*)neo_zalloc(sizeof(h_elm_t)*HASH_LENGTH);
			if( !array ) {
				neo_errno	= E_RDB_NOMEM;
				goto err;
			}
			pEnt2->ent_array	= array;
			pEnt2->ent_size 	= HASH_LENGTH;
		} else {
			if( pEnt2->ent_cnt >= pEnt2->ent_size ) { // extend
				array = (h_elm_t*)neo_zalloc(sizeof(h_elm_t)*pEnt2->ent_size*2);
				if( !array ) {
					neo_errno	= E_RDB_NOMEM;
					goto err;
				}
				for( i2 = 0; i2 < pEnt2->ent_cnt; i2++ ) {
					array[i2]	= pEnt2->ent_array[i2];
				}
				neo_free( pEnt2->ent_array );
				pEnt2->ent_array	= array;
				pEnt2->ent_size		*= 2;
			}
		}
		pEnt->ent_cnt++;
		pEnt	= pEnt2;
	}
	pElm	= &pEnt->ent_array[pEnt->ent_cnt++];
	pElm->e_ind		= rInd;
	pElm->e_lValue	= lValue;

	return( 0 );
err:
	return( -1 );
}
 
int
hash_del_elm( h_head_t *pH, uint64_t lValue, int rInd )
{
	int			i, j;
	h_elm_t		*pElm;
	h_entry_t	*pEnt, *pEnt1=NULL, *pEnt2=NULL;

	pEnt1	= hash_get_ent( pH, lValue );
	if( !pEnt1 ) {
		goto err;
	}
	pEnt	= pEnt1;
	if( pEnt1->ent_hash ) {
		pEnt2 = hash_get_ent( pEnt->ent_hash, lValue );
		if( !pEnt2 ) {
			goto err;
		}
		pEnt = pEnt2;
	}
	for( i = 0; i < pEnt->ent_cnt; i++ ) {
		pElm	= &pEnt->ent_array[i];
		if( pElm->e_ind == rInd ) {
			for( j = i + 1; j < pEnt->ent_cnt; j++ ) {
				pEnt->ent_array[j-1]	= pEnt->ent_array[j];
			}
			if( pEnt1 )	pEnt1->ent_cnt--;
			if( pEnt2 )	pEnt2->ent_cnt--;
			return( 0 );
		}
	}
err:
	neo_errno	= E_RDB_NOEXIST;
	return( -1 );
}

h_head_t*
hash_make( r_tbl_t *tdp, r_item_t *itemp )
{
	int			flag;
	h_head_t	*pH, *pH2;
	h_entry_t		*pE, *pE2;
	int			i, i2;
	int			pn;
	uint64_t	lValue;
	r_head_t	*rp;

	if( (pH = (h_head_t*)HashGet(&tdp->t_HashEqual, itemp->i_name))) {
		return( pH );
	}

	if( !(pH = hash_create( HASH_SIZE_1 ) ) ) {
		neo_errno	= E_RDB_NOMEM;
		goto err;
	}
	strncpy( pH->h_name, itemp->i_name, sizeof(pH->h_name) );
	for( i = 0; i < tdp->t_rec_num; i++ ) {
		if( !(tdp->t_rec[i].r_stat & R_REC_CACHE) ) {
			pn = R_PAGE_SIZE/tdp->t_rec_size;
			svc_load( tdp, i, pn );
		}
		if( !(tdp->t_rec[i].r_stat & R_USED) )	continue;

		if( (rp = tdp->t_rec[i].r_head)->r_Cksum64 ) {
			assert( !in_cksum64( rp, tdp->t_rec_size, 0 ) );
		}
		if( itemp->i_attr == R_STR || itemp->i_attr == R_BYTES )	flag = 1;
		else	flag = 0;

		lValue	= hash_value( (char*)tdp->t_rec[i].r_head + itemp->i_off,
								itemp->i_len, flag );
		if( hash_put_elm( pH, lValue, i ) < 0 )	goto err;
	}
	for( i = 0; i < pH->h_hash_size; i++ ) {
		pE = &pH->h_value[i];
		if( pE->ent_cnt > 0 ) {
			pH->h_cnt	+= pE->ent_cnt;
			pH->h_active++;
			if( pH->h_max < pE->ent_cnt )	pH->h_max = pE->ent_cnt;
			if( pE->ent_hash ) {
				pH2	= pE->ent_hash;
				for( i2 = 0; i2 < pH2->h_hash_size; i2++ ) {
					pE2 = &pH2->h_value[i2];
					if( pE2->ent_cnt > 0 ) {
						pH2->h_cnt	+= pE2->ent_cnt;
						pH2->h_active++;
						if( pH2->h_max < pE2->ent_cnt ) {
							pH2->h_max = pE2->ent_cnt;
						}
					}
				}
			}
		}
	}
	HashPut( &tdp->t_HashEqual, pH->h_name, pH );
//DBG_PRT("[%s:%s]\n", tdp->t_name, itemp->i_name );
	return( pH );
err:
	hash_head_free( pH );
	return( NULL );
}

int
hash_add( r_tbl_t *tdp, r_item_t *itemp, int ind )
{
	int	flag;
	h_head_t	*pH;
	int			pn;
	uint64_t	lValue;

	pH = (h_head_t*)HashGet(&tdp->t_HashEqual, itemp->i_name);
	if( !pH ) {
		neo_errno	= E_RDB_NOHASH;
		goto err;
	}
	if( !(tdp->t_rec[ind].r_stat & R_USED) ) {
		neo_errno	= E_RDB_NOEXIST;
		goto err;
	}
	if( !(tdp->t_rec[ind].r_stat & R_REC_CACHE) ) {
		pn = R_PAGE_SIZE/tdp->t_rec_size;
		svc_load( tdp, ind, pn );
	}
	if( itemp->i_attr == R_STR || itemp->i_attr == R_BYTES )	flag = 1;
	else	flag = 0;

	lValue	= hash_value( (char*)tdp->t_rec[ind].r_head + itemp->i_off,
								itemp->i_len, flag );
	if( hash_put_elm( pH, lValue, ind ) < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

/*
 *	アイテムのEqualHashからindを削除する
 */
int
hash_del( r_tbl_t *tdp, r_item_t *itemp, int ind )
{
	int			flag;
	h_head_t	*pH;
	int			pn;
	uint64_t	lValue;

	pH = (h_head_t*)HashGet(&tdp->t_HashEqual, itemp->i_name);
	if( !pH ) {
		neo_errno	= E_RDB_NOHASH;
		goto err;
	}
	if( !(tdp->t_rec[ind].r_stat & R_USED) ) {
		neo_errno	= E_RDB_NOEXIST;
		goto err;
	}
	if( !(tdp->t_rec[ind].r_stat & R_REC_CACHE) ) {
		pn = R_PAGE_SIZE/tdp->t_rec_size;
		svc_load( tdp, ind, pn );
	}
	if( itemp->i_attr == R_STR || itemp->i_attr == R_BYTES )	flag = 1;
	else	flag = 0;

	lValue	= hash_value( (char*)tdp->t_rec[ind].r_head + itemp->i_off,
								itemp->i_len, flag );
	if( hash_del_elm( pH, lValue, ind ) < 0 )	goto err;

	return( 0 );
err:
	neo_errno	= E_RDB_NOEXIST;
	return( -1 );
}

void
hash_head_free( h_head_t *hp )
{
	int	i;
	h_entry_t	*he;

//DBG_PRT("[%s]\n", hp->h_name );
	for( i = 0; i < hp->h_hash_size; i++ ) {
		he = &hp->h_value[i];
		if( he->ent_array )	neo_free( he->ent_array );
		if( he->ent_hash )	hash_head_free( he->ent_hash );
	}
	neo_free( hp );
}

void
hash_destroy( Hash_t* pHash, void* pValue )
{
	hash_head_free( (h_head_t*)pValue );
}


h_cntl_t*
hash_alloc( tblp )
	r_from_tables_t	*tblp;
{
	h_cntl_t	*cp;
	r_expr_t	*wp;
	r_expr_t	*lp, *rp;
	int		lind, rind;
	r_value_t	lv, rv;
	r_tbl_t		*lt, *rt;
	r_item_t	*li, *ri;
	h_head_t	*lhp, *rhp;
	h_equal_t	*eqp;

	DBG_ENT(M_HASH,"hash_alloc");

	if( !(cp = (h_cntl_t*)neo_zalloc( sizeof(h_cntl_t) ) ) ) {
		neo_errno = E_RDB_NOMEM;
		goto err1;
	}
	_dl_init( &cp->c_lnk );
	cp->c_from = tblp;

	if( !tblp->f_wherep )
		goto ret;

	if( sql_not_inner_join( tblp->f_wherep ) )
		goto ret;
//	goto ret;

	for( wp = 0; (wp = sql_getnode(tblp->f_wherep, wp, SEQ));) {

		lp	= NULL;
		if( wp->e_left->e_op == '.' ) {
			lp	= wp->e_left;
			lind	= lp->e_left->e_value.v_value.v_int;
			li		= lp->e_right->e_value.v_value.v_item;
			lt = tblp->f_t[lind].f_tdp;

			lhp = hash_make( lt, li );

			if( !lhp ) {
				neo_errno = E_RDB_NOMEM;
				goto err2;
			}
		} else {
			lv	= wp->e_left->e_value;
		}
		rp	= NULL;
		if( wp->e_right->e_op == '.' ) {
			rp	= wp->e_right;
			rind	= rp->e_left->e_value.v_value.v_int;
			ri		= rp->e_right->e_value.v_value.v_item;
			rt = tblp->f_t[rind].f_tdp;

			rhp = hash_make( rt, ri );

			if( !rhp ) {
				if( lhp )	hash_head_free( lhp );
				neo_errno = E_RDB_NOMEM;
				goto err2;
			}
		} else {
			rv	= wp->e_right->e_value;
		}
		if( !(eqp = (h_equal_t*)neo_zalloc( sizeof(h_equal_t) ) ) ) {
			neo_errno = E_RDB_NOMEM;
			goto err2;
		}
		_dl_init( &eqp->eq_lnk );	
		if( lp ) {
			eqp->eq_left		= lind;
			eqp->eq_left_hp		= lhp;
		} else {
			eqp->eq_left_value	= lv;
		}
		if( rp ) {
			eqp->eq_right		= rind;
			eqp->eq_right_hp	= rhp;
		} else {
			eqp->eq_right_value	= rv;
		}
		
		_dl_insert( &eqp->eq_lnk, &cp->c_lnk );
	}

ret:
	DBG_EXIT( cp );
	return( cp );

err2:	hash_free( cp );
err1:
	DBG_EXIT( 0 );
	return( 0 );
}

void
hash_free( cp )
	h_cntl_t	*cp;
{
	h_equal_t	*eqp;

	while( (eqp = (h_equal_t*)_dl_head( &cp->c_lnk )) ) {

		_dl_remove( &eqp->eq_lnk );

		neo_free( eqp );
	}
	neo_free( cp );
}

/*
 *	cp->c_from->f_joinp[i] = -1;
 */
int
hash_loop( prev, cp )
	h_equal_t	*prev;
	h_cntl_t	*cp;
{
	int		i, i2;
	h_equal_t	*eqp;
	h_head_t	*lhp, *lhp2, *rhp, *rhp2, *whp;
	h_entry_t	*ep, *lep, *lep2, *rep, *rep2;
	h_elm_t		*lp, *rp;
	int		*lindp, *rindp, *windp;
	int		lsave, rsave, wsave;
	int		l, l2, r, r2;
	uint64_t	lValue;
	r_value_t	*pv;
static int nest;

	DBG_ENT(M_HASH,"hash_loop");

	++nest;

	if( prev ) {
		eqp = (h_equal_t*)_dl_next( &prev->eq_lnk );
		if( &eqp->eq_lnk == &cp->c_lnk )	eqp = 0;
	} else {
		eqp = (h_equal_t*)_dl_head( &cp->c_lnk );
	}
//DBG_PRT("prev=%p eqp=%p nest=%d\n", prev, eqp, nest );
	if( !eqp  ) {	/* terminate */
		/* evaluate */
		if( prc_join_loop( 0, cp->c_from ) ) {
			goto err1;
		}
//DBG_PRT("TERMINATE nest=%d\n", nest );
		nest--;
		DBG_EXIT( 0 );
		return( 0 );
	} 
	lhp	= eqp->eq_left_hp;
	if( lhp ) {
		lindp = &cp->c_from->f_joinp[eqp->eq_left];
		lsave = *lindp;
	}
	rhp	= eqp->eq_right_hp;
	if( rhp ) {
		rindp = &cp->c_from->f_joinp[eqp->eq_right];
		rsave = *rindp;
	}
	if( !lhp && !rhp ) {
//DBG_PRT("VALUE=VALUE skip\n");
		hash_loop( eqp, cp );
	} else {
		if( !lhp ) {
//DBG_PRT("VALUE=ITEM rsave=%d\n", rsave );
				pv	= &eqp->eq_left_value;
				switch( pv->v_type ) {
				case VSTRING:
				case VSTRING_ALLOC:
					lValue = hash_value( pv->v_value.v_str.s_str,
								pv->v_value.v_str.s_len, 1 ); 
					break;
				case VLONG:
				case VULONG:
					lValue = hash_value( &pv->v_value, sizeof(int64_t), 0 );
					break;
				default:
					lValue = hash_value( &pv->v_value, sizeof(int), 0 );
					break;
				}
				ep	= hash_get_entry( rhp, lValue );
				for( r = 0; r < ep->ent_cnt; r++ ) {
					rp = &ep->ent_array[r];
					if( rsave >= 0 && rsave != rp->e_ind )	continue;

					*rindp = rp->e_ind;
					hash_loop( eqp, cp );
				}
		} else if( !rhp ) {
//DBG_PRT("ITEM=VALUE lsave=%d\n", lsave );
				pv	= &eqp->eq_right_value;
				switch( pv->v_type ) {
				case VSTRING:
				case VSTRING_ALLOC:
					lValue = hash_value( pv->v_value.v_str.s_str,
								pv->v_value.v_str.s_len, 1 ); 
					break;
				case VLONG:
				case VULONG:
					lValue = hash_value( &pv->v_value, sizeof(int64_t), 0 );
					break;
				default:
					lValue = hash_value( &pv->v_value, sizeof(int), 0 );
					break;
				}
				ep	= hash_get_entry( lhp, lValue );

				for( l = 0; l < ep->ent_cnt; l++ ) {
					lp = &ep->ent_array[l];
					if( lsave >= 0 && lsave != lp->e_ind )	continue;

					*lindp = lp->e_ind;
					hash_loop( eqp, cp );
				}
		} else {
//DBG_PRT("ITEM=ITEM lsave=%d rsave=%d\n", lsave, rsave );
			if( lhp->h_cnt > rhp->h_cnt ) {
				whp		= lhp;
				wsave	= lsave;
				windp	= lindp;

				lhp		= rhp;
				lsave	= rsave;
				lindp	= rindp;

				rhp		= whp;
				rsave	= wsave;
				rindp	= windp;
			}
	
			for( i = 0; i < HASH_SIZE_1; i++ ) {

				lep = &lhp->h_value[i];
				rep = &rhp->h_value[i];

				lhp2	= lep->ent_hash;
				rhp2	= rep->ent_hash;
				if( lhp2 && rhp2 ) {
					for( i2 = 0; i2 < HASH_SIZE_2; i2++ ) {
						lep2	= &lhp2->h_value[i2];	
						rep2	= &rhp2->h_value[i2];	

						for( l = 0; l < lep2->ent_cnt; l++ ) {
							lp = &lep2->ent_array[l];
							if( lsave >= 0 && lsave != lp->e_ind )
								continue;

							*lindp = lp->e_ind;

							for( r = 0; r < rep2->ent_cnt; r++ ) {
								rp = &rep2->ent_array[r];
								if( rsave >= 0 && rsave != rp->e_ind )
									continue;

								*rindp	= rp->e_ind;

								hash_loop( eqp, cp );
							}
						}
					}
				} else if( lhp2 && !rhp2 ) {
				/* left check */
					for( l2 = 0; l2 < HASH_SIZE_2; l2++ ) {
						lep2	= &lhp2->h_value[l2];	

						for( l = 0; l < lep2->ent_cnt; l++ ) {
							lp = &lep2->ent_array[l];
							if( lsave >= 0 && lsave != lp->e_ind )	
								continue;

							*lindp = lp->e_ind;

							/* right check */
							for( r = 0; r < rep->ent_cnt; r++ ) {
								rp = &rep->ent_array[r];
								if( rsave >= 0 && rsave != rp->e_ind )	
									continue;

								*rindp	= rp->e_ind;

								hash_loop( eqp, cp );
							}
						}
					}
				} else if( !lhp2 && rhp2 ) {
					/* left check */
					for( l = 0; l < lep->ent_cnt; l++ ) {
						lp = &lep->ent_array[l];
						if( lsave >= 0 && lsave != lp->e_ind )
							continue;

						*lindp = lp->e_ind;

						/* right check */
						for( r2 = 0; r2 < HASH_SIZE_2; r2++ ) {
							rep2	= &rhp2->h_value[r2];
							for( r = 0; r < rep2->ent_cnt; r++ ) {
								rp = &rep2->ent_array[r];
								if( rsave >= 0 && rsave != rp->e_ind )
									continue;
								*rindp	= rp->e_ind;

								hash_loop( eqp, cp );
							}
						}
					}
				} else {
					/* left check */
					for( l = 0; l < lep->ent_cnt; l++ ) {
						lp = &lep->ent_array[l];
						if( lsave >= 0 && lsave != lp->e_ind )
							continue;

						*lindp = lp->e_ind;

						/* right check */
						for( r = 0; r < rep->ent_cnt; r++ ) {
							rp = &rep->ent_array[r];
							if( rsave >= 0 && rsave != rp->e_ind )
								continue;

							*rindp	= rp->e_ind;

							hash_loop( eqp, cp );
						}
					}
				}
			}
		}
	}
	if( lhp ) {
		*lindp	= lsave;
	}
	if( rhp ) {
		*rindp	= rsave;
	}
//DBG_PRT("RETURN nest=%d\n", nest );
	nest--;
	DBG_EXIT( 0 );
	return( 0 );

err1:
	DBG_EXIT( neo_errno );
	return( neo_errno );
}

int
xj_join_by_hash( tblp )
	r_from_tables_t	*tblp;
{
	h_cntl_t	*cp;

	if( !(cp = hash_alloc( tblp ) ) ) {
		goto err;
	}
	if( hash_loop( 0, cp ) ) {
		goto err1;
	}
	hash_free( cp );
//DBG_PRT("f_num=%d\n", tblp->f_num );

	return( 0 );

err1:	hash_free( cp );
err:
	return( -1 );
}

