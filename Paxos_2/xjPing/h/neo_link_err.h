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

#ifndef _NEO_LINK_ERRNO_
#define _NEO_LINK_ERRNO_

#include        "neo_class.h"

#define	E_LINK_PORT	(M_LINK<<24|1)	/* invalid port */
#define	E_LINK_FD	(M_LINK<<24|2)	/* invalid file descriptor */
#define	E_LINK_ADDR	(M_LINK<<24|3)	/* invalid address */
#define	E_LINK_NONAME	(M_LINK<<24|4)	/* invalid name */
#define	E_LINK_INVAL	(M_LINK<<24|5)	/* link invalid */
#define	E_LINK_REJECT	(M_LINK<<24|6)	/* rejected */
#define	E_LINK_NOMEM	(M_LINK<<24|7)	/* no memory */
#define	E_LINK_BUSY	(M_LINK<<24|8)	/* busy */
#define	E_LINK_DOWN	(M_LINK<<24|9)	/* down */
#define	E_LINK_PACKET	(M_LINK<<24|10)	/* invalid packet */
#define	E_LINK_FAMILY	(M_LINK<<24|11)	/* invalid family */
#define	E_LINK_BUFSIZE	(M_LINK<<24|12)	/* buffer size */
#define	E_LINK_TIMEOUT	(M_LINK<<24|13)	/* timeout */
#define	E_LINK_RETRY	(M_LINK<<24|14)	/* retry over */
#define	E_LINK_DUPFRAME	(M_LINK<<24|15)	/* dupricated */
#define	E_LINK_CNTL	(M_LINK<<24|16)	/* control */
#define	E_LINK_NODATA	(M_LINK<<24|17)	/* no data */
#define	E_LINK_CONFFILE	(M_LINK<<24|18)	/* configuration */

#endif
