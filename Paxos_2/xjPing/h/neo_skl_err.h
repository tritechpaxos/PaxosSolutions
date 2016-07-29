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
 *	neo_skl_err.h
 *
 *	説明	スケルトン エラー番号
 *
 ******************************************************************************/
#ifndef _NEO_SKL_ERRNO_
#define _NEO_SKL_ERRNO_

#include	"neo_class.h"

#define	E_SKL_INVARG	(M_SKL<<24|1)	/* invalid argument  */
#define	E_SKL_NOMEM	(M_SKL<<24|2)	/* no memory */
#define	E_SKL_TERM	(M_SKL<<24|3)	/* SIGTERM */

#endif
