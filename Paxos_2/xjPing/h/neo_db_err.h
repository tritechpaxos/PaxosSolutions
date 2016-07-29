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
 *	neo_db_err.h
 *
 *	説明	エラー番号ヘッダ
 *
 ******************************************************************************/
#ifndef	_NEO_DB_ERR_
#define	_NEO_DB_ERR_

#include	"neo_class.h"

#define	E_RDB_NOMEM	(M_RDB<<24|1)	/* no memory		*/
#define	E_RDB_TBL_BUSY	(M_RDB<<24|2)	/* table is busy		*/
#define	E_RDB_REC_BUSY	(M_RDB<<24|3)	/* record is busy		*/
#define	E_RDB_CONFUSED	(M_RDB<<24|4)	/* server status is confused	*/
#define	E_RDB_MD	(M_RDB<<24|5)	/* invalid server-ID		*/
#define	E_RDB_EXIST	(M_RDB<<24|6)	/* already exist		*/
#define	E_RDB_INVAL	(M_RDB<<24|7)	/* invalid argument		*/
#define	E_RDB_SPACE	(M_RDB<<24|8)	/* no space in disk		*/
#define	E_RDB_TD	(M_RDB<<24|9)	/* invalid table-ID		*/
#define	E_RDB_UD	(M_RDB<<24|10)	/* invalid user-ID		*/
#define	E_RDB_NOCACHE	(M_RDB<<24|11)	/* no cache space		*/
#define	E_RDB_FD	(M_RDB<<24|12)	/* file descriptor		*/
#define	E_RDB_SEARCH	(M_RDB<<24|13)	/* illegal search condition	*/
#define	E_RDB_NOITEM	(M_RDB<<24|14)	/* cannot find item name	*/
#define	E_RDB_NOEXIST	(M_RDB<<24|15)	/* no entry			*/
#define	E_RDB_RETRY	(M_RDB<<24|16)	/* retry over			*/
#define	E_RDB_STOP	(M_RDB<<24|17)	/* server stop			*/
#define	E_RDB_PROTOTYPE	(M_RDB<<24|18)	/* invalid protocol type	*/
#define	E_RDB_FILE	(M_RDB<<24|19)	/* invalid table-file type	*/
#define	E_RDB_HOLD	(M_RDB<<24|20)	/* table is locked			*/
#define	E_BYTESFILE_ERROR	(M_RDB<<24|21)	/* bytes file operation error*/
#define	E_RDB_NOHASH	(M_RDB<<24|22)	/* no hash			*/
#define	E_RDB_NOSUPPORT	(M_RDB<<24|23)	/* no support		*/

#define	E_RDB_CLOSED	(M_RDB<<24|24)	/* already closed		*/

#endif /* _NEO_DB_ERR_ */
