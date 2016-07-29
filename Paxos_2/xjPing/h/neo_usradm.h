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
 *	neo_usradm.h
 *
 *	説明	サーバ用ユーザ管理 アクセスライブラリ
 *
 ******************************************************************************/
#ifndef	_NEO_USRADM_
#define	_NEO_USRADM_

#include "neo_system.h"

/*
 * client adm structure for server
 */
typedef struct usr_adm {
	char	u_procname[NEO_NAME_MAX];	/* svc/client name	*/
	int	u_pid;				/* svc/client pid	*/
	char	u_portname[NEO_NAME_MAX];	/* svc/client portname	*/
	char	u_hostname[NEO_NAME_MAX];	/* svc/client hostname	*/
	int	u_portno;			/* svc/client portno	*/
} usr_adm_t;

extern	void	neo_setusradm( usr_adm_t*, char*, int, char*, char*, int );

#endif	/* _NEO_USRADM_ */
