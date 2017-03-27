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

#ifndef	_BLOCK_CACHE_
#define	_BLOCK_CACHE_

#include	"NWGadget.h"
#include	"DlgCache.h"

#ifndef	PATH_MAX
#define	PATH_MAX	256
#endif

#define	C_BLOCK_NAME_MAX	PATH_MAX

typedef	struct C_BCtrl {
	DlgCacheCtrl_t	bc_Ctrl;
	int				bc_BlockSize;
	bool_t			bc_bCksum;
	uint32_t		bc_Usec;
	void			*(*bc_fMalloc)(size_t);
	void			(*bc_fFree)(void*);
} C_BCtrl_t;

#define	C_BLOCK_SIZE( pBC )		((pBC)->bc_BlockSize)

#define	C_BLOCK_ACTIVE		0x1
#define	C_BLOCK_CKSUMED		0x2
#define	C_BLOCK_DO_CKSUM	0x4

typedef	struct C_Block {
	DlgCache_t		b_Dlg;
	int				b_Flags;
	char*			b_pBuf;			// [BLOCK_CACHE_SIZE]
	timespec_t		b_Valid;
	uint64_t		b_Cksum64;
} C_Block_t;

typedef	struct BC_Req {
	list_t	rq_Lnk;
	off_t	rq_Offset;
	size_t	rq_Size;
	char	*rq_pData;
} BC_Req_t;

typedef struct BC_IO_t {
	list_t		io_Lnk;
	int		io_N;
	C_Block_t	**io_aB;	
} BC_IO_t;

extern	int
	BCInit( C_BCtrl_t *pBC, struct Session *pSession, DlgCacheRecall_t *pRC,
		int BlockMax, int BlockSize, bool_t bCksum, int Msec, 
		void*(*fMalloc)(size_t), void(*fFree)(void*), struct Log *pLog );

extern	int BC_IO_init( BC_IO_t *pIO );
extern	int BC_IO_destroy( BC_IO_t *pIO );

extern	int BCReadIO( C_BCtrl_t *pBC, BC_IO_t *pIO,
		char *pName, off_t Offset, size_t Size, char *pData );
extern	int BCReadDone( C_BCtrl_t *pBC, BC_IO_t *pIO,
		char *pName, off_t Offset, size_t Size, char *pData );

extern	int BCWriteIO( C_BCtrl_t *pBC, BC_IO_t *pIO,
		char *pName, off_t Offset, size_t Size, char *pData );
extern	int BCWriteDone( C_BCtrl_t *pBC, BC_IO_t *pIO,
		char *pName, off_t Offset, size_t Size, char *pData );

#endif	//	_BLOCK_CACHE_
