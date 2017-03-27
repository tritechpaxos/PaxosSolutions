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

#ifndef	_STATUS_
#define	_STATUS_

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<dirent.h>
#include	<libgen.h>
#include	"NWGadget.h"

#define	ST_MSG_SIZE	20
#define	ST_DT		10/*sec*/

#ifndef	FILENAME_MAX
#define	FILENAME_MAX	1024
#endif
#define	ST_NAME_MAX		256

typedef	struct StatusEnt {
	char	e_Name[256];
	time_t	e_Time;
#define	ST_REG	0x1
#define	ST_DIR	0x2
#define	ST_DEL	0x4
#define	ST_NON	0x8
	int		e_Type;
	off_t	e_Size;
	void	*e_pVoid;
} StatusEnt_t;

typedef	struct PathEnt {
	time_t	p_Time;
	int		p_Type;
	int		p_Len;
	off_t	p_Size;
} PathEnt_t;
// follow p_Len characters

extern	int 	STFileList( char *pPath, Msg_t *pMsg );
extern	Hash_t* STTreeCreate( Msg_t *pMsg );
extern	int 	STTreeDestroy( Hash_t *pHash );
extern	int		STTreePrint( Hash_t *pHash );
extern	StatusEnt_t*
				STTreeFind( Msg_t* pDir, StatusEnt_t *pOldS, Hash_t *pTree );
extern	StatusEnt_t* STTreeFindByPath( char *pPath, Hash_t *pTree );
extern	int STUnlink( Msg_t *pDel );
extern	int 	STDiffList( Msg_t *pMsgOld, Msg_t *pMsgNew, time_t DT,
									Msg_t *pMsgDel, Msg_t *pMsgUpd );

#endif
