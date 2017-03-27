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
 *	neo_system.h
 *
 *	説明	システム定義
 *
 ******************************************************************************/
#ifndef _NEO_SYSTEM_
#define	_NEO_SYSTEM_

#include	"NWGadget.h"

#include	"neo_class.h"
#include	"neo_errno.h"

#include	"neo_debug.h"

#define	NEO_NAME_MAX	32
#define	NEO_MAXPATHLEN	256
#define	NEO_CONF	"NEO_CONF"

#define	PING_MEM_MAX	"PING_MEM_MAX"
#define	PING_HEAP_MB	10	/* MegaBytes */

extern	void*	neo_malloc( size_t );
extern	void*	neo_zalloc( size_t );
extern	void	neo_free( void* );

extern	NHeapPool_t*	neo_GetHeapPool(void);
extern	void		neo_HeapPoolDump(void);

#endif	// _NEO_SYSTEM_
