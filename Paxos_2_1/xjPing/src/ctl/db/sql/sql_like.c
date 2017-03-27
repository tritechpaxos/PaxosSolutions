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

#include	<stdio.h>
#include	"neo_db.h"

#define	LIKE_SKIP1	1
#define	LIKE_SKIP2	2
#define	LIKE_FIXED	3

typedef	struct like_seg {
	int		s_Type;
	int		s_N;
	unsigned char	*s_pFixed;
} like_seg_t;

typedef	struct like_head {
	unsigned char		*h_pOrg;
	unsigned char		*h_pPattern;
	int			h_N;
	like_seg_t	*h_aSeg;	
} like_head_t;
	
static	like_head_t*
like_alloc( char *pPattern, int N, char e )
{
	int			i, n, j;
	unsigned char		c, *p, *pe, *pc;
	like_head_t	*pH;

	for( i = 0, n = 0; i < N; i++ ) {
		c	= pPattern[i];
		if( c == '%' || c == '_' )	n++;
	}
	n	+= 2;
	pH	= (like_head_t*)neo_zalloc(
				sizeof(like_head_t) + sizeof(like_seg_t)*n + N*2 );
	pH->h_aSeg	= (like_seg_t*)(pH+1);
	pH->h_pOrg	= (unsigned char*)(pH->h_aSeg + n );
	pH->h_pPattern	= (pH->h_pOrg + N );
	
	memcpy( pH->h_pPattern, pPattern, N );
	memcpy( pH->h_pOrg, pPattern, N );
	j = 0;
	p 	= pH->h_pPattern;
	pe	= p + N;
	while( p < pe ) {
		if( *p == e ) {
			for( pc = p + 1 ; pc < pe; pc++ ) {
				*(pc-1)	= *pc;
			}
			N--;
		}
		switch( *p ) {
			case 0:
				p++;
				break;

			case '%':
				pH->h_aSeg[j++].s_Type	= LIKE_SKIP1;
				p++;
				break;
			case '_':
				pH->h_aSeg[j++].s_Type	= LIKE_SKIP2;
				p++;
				break;
				
			default:
				pH->h_aSeg[j].s_Type	= LIKE_FIXED;
				pH->h_aSeg[j].s_pFixed	= p;
				n = 1;
				p++;
				while( p < pe ) {
					if( *p == e ) {
						for( pc = p + 1 ; pc < pe; pc++ ) {
							*(pc-1)	= *pc;
						}
						N--;
					}
					if( *p == '%' || *p == '_' || *p == 0 )	break;
					n++;
					p++;
				}
				pH->h_aSeg[j].s_N	= n;
				j++;
				break;
		}
	}	
	pH->h_N	= j;
	return( pH );
}

static	void
like_free( like_head_t *pH )
{
	neo_free( pH );
}

static inline bool_t
_match( char *t, int tn, char *p, int pn )
{
	if( tn < pn )	return( FALSE );
	if( strncmp( t, p, pn ) )	return( FALSE );
	else	return( TRUE );
} 

/*
 * sql_like
 *
 *	match t by pattern p in t;
 *	matched info is writed in b
 *  return   0 - success
 *          -1 - fail
 */
int
sql_like(t, tn, p, pn, e, b)
	char	*t;	/* (I) string to match */
	int		tn;		/* (I) t's size */
	char	*p;	/* (I) match pattern */
	int		pn;		/* (I) p's size */
	char	*e;	/* (I) escape word */
	int	*b;		/* (O) match flag (1: matched 0: not matched) */
{
	static	like_head_t	*pH;
	int		i;
	like_seg_t	*pSeg;
	int		prev_mode = 0;

	if( !pH || memcmp( pH->h_pOrg, p, pn ) ) {
		if( pH )	like_free( pH );
		pH	= like_alloc( p, pn, (e == NULL ? 0 : *e) );
	} 
	*b	= 1;
	for( i = 0; i < pH->h_N; i++ ) {
		pSeg	= &pH->h_aSeg[i];
		switch( pSeg->s_Type ) {
			case LIKE_SKIP1:
				break;

			case LIKE_SKIP2:
				if( tn <= 0 ) {
					*b	= 0;
					return( 0 );
				}
				tn--;
				t++;
				break;

			case LIKE_FIXED:
				if( prev_mode == LIKE_SKIP1 ) {
					while( !_match(t, tn, (char*)pSeg->s_pFixed, pSeg->s_N) ) {
						t++; tn--;
						if( tn <= 0 ) {
							*b	= 0;
							return( 0 );
						}
					}
				} else {
					if( !_match(t, tn, (char*)pSeg->s_pFixed, pSeg->s_N) ) {
						*b	= 0;
						return( 0 );
					}
				}
				t	+= pSeg->s_N;
				tn	-= pSeg->s_N;
				break;
		}
		prev_mode	= pSeg->s_Type;
	}
	if( tn && *t && prev_mode != LIKE_SKIP1 )	*b	= 0;
	return( 0 );
}
