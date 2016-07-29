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

#ifndef __BS_PAXOS_H__
#define __BS_PAXOS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>
#include <dirent.h>

#define	WORKER_MAX_TGTD	4	//4
#define	WORKER_MAX_IO	4	//4

int	VV_init(void);
int	VV_exit(void);
int	VV_LockR(void);
int	VV_Unlock(void);

int	VL_init(char *pTarget, int Worker );
int	VL_exit(void);
int	VL_open( char *pTarget, char *pPath, void **ppVV, uint64_t *pSize);
int	VL_read( void *pVV, char *pBuff, size_t Length, off_t Offset );
int	VL_write( void *pVV, char *pBuff, size_t Length, off_t Offset );
int	VL_close( void *pVV );

#endif 
