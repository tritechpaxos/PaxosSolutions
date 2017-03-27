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
 *
 *	neo_dlnk.h
 *
 *	ê‡ñæ	ÉLÉÖÅ[ä«óù
 *
 ******************************************************************************/
#ifndef	_NEO_DLNK_
#define	_NEO_DLNK_

#include "neo_system.h"

#define	_dlnk_t	Dlnk_t
#define	_dl_init( qp )	dlInit( (Dlnk_t*)(qp) )
#define	_dl_insert( this, before )	dlInsert((Dlnk_t*)(this), (Dlnk_t*)(before))
#define	_dl_append( this, after )	dlAppend( (Dlnk_t*)(this), (Dlnk_t*)(after))
#define	_dl_remove( p )	dlRemove( (Dlnk_t*)(p) )
#define	_dl_head( ep )	dlHead( (Dlnk_t*)(ep) )
#define	_dl_tail( ep )	dlTail( (Dlnk_t*)(ep) )
#define	_dl_next( qp )	dlNext( (Dlnk_t*)(qp) )
#define	_dl_prev( qp )	dlPrev( (Dlnk_t*)(qp) )
#define	_dl_valid( ep, qp )	dlValid( (Dlnk_t*)(ep), (Dlnk_t*)(qp) )

#endif  // _NEO_DLNK_	/* _NEO_DLNK_ */
