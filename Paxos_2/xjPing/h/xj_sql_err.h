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

#ifndef	_XJ_SQL_ERR_
#define	_XJ_SQL_ERR_

#include	"neo_class.h"

#define	E_SQL_PARSE	(M_SQL<<24|1)	/* parse error */
#define	E_SQL_SELECT	(M_SQL<<24|2)	/* select */
#define	E_SQL_EXPR	(M_SQL<<24|3)	/* expression */
#define	E_SQL_EVAL	(M_SQL<<24|4)	/* evaluation */
#define	E_SQL_USR	(M_SQL<<24|5)	/* allocate usr */
#define	E_SQL_ATTR	(M_SQL<<24|6)	/* attrubute */
#define	E_SQL_SCHEMA	(M_SQL<<24|7)	/* shema */
#define	E_SQL_NOMEM	(M_SQL<<24|8)	/* no memory */


#endif	/* _XJ_SQL_ERR_ */
