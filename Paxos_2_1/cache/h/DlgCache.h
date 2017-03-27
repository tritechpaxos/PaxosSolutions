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

#ifndef	_DLG_CACHE_
#define	_DLG_CACHE_

#include	"PFS.h"

struct	DlgCache;

typedef struct DlgCacheRecall {
	list_t				rc_LnkCtrl;
	struct Session		*rc_pEvent;
	uint64_t			rc_EventCount;
	pthread_t			rc_Thread;
} DlgCacheRecall_t;

typedef	struct DlgCacheCtrl {
	list_t				dc_Lnk;
	GElmCtrl_t			dc_Ctrl;
	struct	Session		*dc_pSession;
	struct	Session		*dc_pEvent;
	struct	DlgCache	*(*dc_fAlloc)(struct DlgCacheCtrl*);
	int					(*dc_fFree)(struct DlgCacheCtrl*,struct DlgCache* );
	int					(*dc_fSet)(struct DlgCacheCtrl*,struct DlgCache* );
	int					(*dc_fUnset)(struct DlgCacheCtrl*,struct DlgCache*);
	int 				(*dc_fInit)(struct DlgCache*,void *pArg);
	int 				(*dc_fTerm)(struct DlgCache*,void *pArg);
	int 				(*dc_fLoop)(struct DlgCache*,void *pArg);
	struct Log			*dc_pLog;
} DlgCacheCtrl_t;

typedef	struct DlgCache {
	GElm_t			d_GE;
	DlgCacheCtrl_t	*d_pCtrl;
	timespec_t		d_Timespec;
	PFSDlg_t		d_Dlg;
	void			*d_pVoid;

#define	DLG_INIT	0x1

#define	DlgInitOn( pD )		( (pD)->d_Flags |= DLG_INIT )
#define	DlgInitOff( pD )	( (pD)->d_Flags &= ~DLG_INIT )
#define	DlgInitIsOn( pD )	( (pD)->d_Flags & DLG_INIT )

#define	DlgIsR( pD )		( (pD)->d_Dlg.d_Own == PFS_DLG_R )
#define	DlgIsW( pD )		( (pD)->d_Dlg.d_Own == PFS_DLG_W )

	int				d_Flags;
} DlgCache_t;

extern	int DlgCacheRecallStart(DlgCacheRecall_t *pRC, struct Session *pEvent);
extern	int DlgCacheRecallStop( DlgCacheRecall_t *pRC );

extern	int DlgCacheInit( struct Session *pSession, DlgCacheRecall_t *pRC,
				DlgCacheCtrl_t *pDC, int MaxD, 
				DlgCache_t	*(*fAlloc)(DlgCacheCtrl_t*), 
				int		(*fFree)(DlgCacheCtrl_t*,DlgCache_t*), 
				int		(*fSet)(DlgCacheCtrl_t*,DlgCache_t*),
				int		(*fUnset)(DlgCacheCtrl_t*,DlgCache_t*),
				int 	(*fInit)(DlgCache_t*,void *pArg),
				int 	(*fTerm)(DlgCache_t*,void *pArg),
				int		(*fLoop)(DlgCache_t*,void *pArg),
				struct Log *pLog );
extern	int DlgCacheInitBy( struct Session *pSession, DlgCacheRecall_t *pRC,
				DlgCacheCtrl_t *pDC, int MaxD, 
				DlgCache_t	*(*fAlloc)(DlgCacheCtrl_t*), 
				int		(*fFree)(DlgCacheCtrl_t*,DlgCache_t*), 
				int		(*fSet)(DlgCacheCtrl_t*,DlgCache_t*),
				int		(*fUnset)(DlgCacheCtrl_t*,DlgCache_t*),
				int 	(*fInit)(DlgCache_t*,void *pArg),
				int 	(*fTerm)(DlgCache_t*,void *pArg),
				int		(*fLoop)(DlgCache_t*,void *pArg),
				struct Log *pLog );

#define	DlgLock( pDC )		GElmCtrlLock( &(pDC)->dc_Ctrl )
#define	DlgUnlock( pDC )	GElmCtrlUnlock( &(pDC)->dc_Ctrl )

extern	int DlgCacheDestroy( DlgCacheCtrl_t *pDC );
extern	int DlgCacheLoop( DlgCacheCtrl_t *pDC, void *pArg );
extern	int DlgCacheFlush( DlgCacheCtrl_t *pDC, void *pArg, bool_t bSave );

extern	DlgCache_t* DlgCacheGet( DlgCacheCtrl_t *pDC, char *pName, 
						bool_t bCreate, void *pTag );
extern	int DlgCachePut( DlgCache_t *pD, bool_t bFree );

extern	DlgCache_t* DlgCacheLockR(DlgCacheCtrl_t*, char *pName, 
						bool_t bCreate, void *pTag, void *pArg );
extern	DlgCache_t* DlgCacheLockW(DlgCacheCtrl_t*, char *pName, 
						bool_t bCreate, void *pTag, void *pArg );
extern	DlgCache_t* DlgCacheTryLockR(DlgCacheCtrl_t*, char *pName, 
						bool_t bCreate, void *pTag, void *pArg );
extern	DlgCache_t* DlgCacheTryLockW(DlgCacheCtrl_t*, char *pName, 
						bool_t bCreate, void *pTag, void *pArg );


extern	int DlgCacheUnlock( DlgCache_t *pD, bool_t bFree );
extern	int DlgCacheUnlockByKey( DlgCacheCtrl_t*, char *pName );

extern	int DlgCacheReturn( DlgCache_t *pD, void *pTerm );

extern	DlgCache_t* DlgCacheLockRBy(DlgCacheCtrl_t*, char *pName, 
				bool_t bCreate, void *pTag, void *pArg, void *pHolder );
extern	DlgCache_t* DlgCacheLockWBy(DlgCacheCtrl_t*, char *pName, 
				bool_t bCreate, void *pTag, void *pArg, void *pHolder );
extern	int DlgCacheUnlockBy( DlgCache_t *pD, bool_t bFree, void *pHolder );

#endif	//	_DLG_CACHE_
