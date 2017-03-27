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

#include	"BlockCache.h"

DlgCache_t*
_BCAlloc( DlgCacheCtrl_t *pDC )
{
	C_BCtrl_t	*pBC;
	C_Block_t	*pB;

	pBC	= container_of( pDC, C_BCtrl_t, bc_Ctrl );

	pB	= (*pBC->bc_fMalloc)( sizeof(*pB) );
	memset( pB, 0, sizeof(*pB) );

	pB->b_pBuf	= (*pBC->bc_fMalloc)( C_BLOCK_SIZE(pBC) );
	memset( pB->b_pBuf, 0, C_BLOCK_SIZE(pBC) );

	return( &pB->b_Dlg );
}

int
_BCFree( DlgCacheCtrl_t *pDC, DlgCache_t *pD )
{
	C_BCtrl_t	*pBC;
	C_Block_t	*pB;

	pBC	= container_of( pDC, C_BCtrl_t, bc_Ctrl );
	pB	= container_of( pD, C_Block_t, b_Dlg );

	(*pBC->bc_fFree)( pB->b_pBuf );
	(*pBC->bc_fFree)( pB );

	return( 0 );
}

int
_BCInit( DlgCache_t *pD, void *pArg )
{
	C_Block_t		*pB;
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
//	C_BCtrl_t	*pBC;

	LOG( pDC->dc_pLog, LogINF, "[%s]", pD->d_Dlg.d_Name );

//	pBC	= container_of( pDC, C_BCtrl_t, bc_Ctrl );
	pB	= container_of( pD, C_Block_t, b_Dlg );

	pB->b_Cksum64	= 0;
	pB->b_Flags		= 0;
	memset( &pB->b_Valid, 0, sizeof(pB->b_Valid) );

	if( pArg )	*(bool_t*)pArg	= TRUE;

	return( 0 );
}

int
_BCTerm( DlgCache_t *pD, void *pArg )
{
	C_Block_t		*pB;
	DlgCacheCtrl_t	*pDC = pD->d_pCtrl;
//	C_BCtrl_t	*pBC;

	LOG( pDC->dc_pLog, LogINF, "[%s]", pD->d_Dlg.d_Name );

//	pBC	= container_of( pDC, C_BCtrl_t, bc_Ctrl );
	pB	= container_of( pD, C_Block_t, b_Dlg );

	pB->b_Flags	= 0;
	pB->b_Cksum64	= 0;

	return( 0 );
}

int
_BCLoop( DlgCache_t *pD, void *pArg )
{
	return( 0 );
}

int
BCInit( C_BCtrl_t *pBC, struct Session *pSession, DlgCacheRecall_t *pRC,
		int BlockMax, int BlockSize, bool_t bCksum, int Msec, 
		void*(*fMalloc)(size_t), void(*fFree)(void*), struct Log *pLog )
{
	int	Ret;

	pBC->bc_BlockSize	= BlockSize;
	pBC->bc_bCksum		= bCksum;
	pBC->bc_Usec		= Msec*1000L;

	pBC->bc_fMalloc		= fMalloc;
	pBC->bc_fFree		= fFree;

	Ret = DlgCacheInit( pSession, pRC, &pBC->bc_Ctrl, BlockMax,
		_BCAlloc, _BCFree, NULL, NULL, _BCInit, _BCTerm,
		_BCLoop, pLog );

	return( Ret );
}

C_Block_t*
BCLock( C_BCtrl_t *pBC, char *pName, int64_t i64, bool_t bRead, bool_t *pbIO )
{
	char	Name[BLOCK_CACHE_NAME_MAX];
	DlgCache_t	*pDlg;

	sprintf( Name, "BC_%s/%"PRIi64"", pName, i64 );

	LOG( pBC->bc_Ctrl.dc_pLog, LogINF, "[%s] bRead=%d", Name, bRead ); 

	if( bRead ) {
		pDlg	= DlgCacheLockR( &pBC->bc_Ctrl, Name, TRUE, NULL, pbIO );
	} else {
		pDlg	= DlgCacheLockW( &pBC->bc_Ctrl, Name, TRUE, NULL, pbIO );
	}
	return container_of( pDlg, C_Block_t, b_Dlg );	
}

int
BCUnlock( C_Block_t *pB )
{
	LOG( pB->b_Dlg.d_pCtrl->dc_pLog, LogINF, "[%s] Own=%d", 
			pB->b_Dlg.d_Dlg.d_Name, pB->b_Dlg.d_Dlg.d_Own  );

	DlgCacheUnlock( &pB->b_Dlg, FALSE );

	return( 0 );
}

int
BC_IO_init( BC_IO_t *pIO )
{
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );

	return( 0 );
}

int
BC_IO_destroy( BC_IO_t *pIO )
{
	BC_Req_t		*pReq;
	C_Block_t	*pB;
	int			i;

	for( i = 0; i < pIO->io_N; i++ ) {
		pB	= pIO->io_aB[i];
		if( pB )	BCUnlock( pB );
	}

	while( (pReq = list_first_entry( &pIO->io_Lnk, BC_Req_t, rq_Lnk )) ) {
		list_del( &pReq->rq_Lnk );
		Free( pReq );
	}
	Free( pIO->io_aB );
	return( 0 );	
}

int
BCReadIO( C_BCtrl_t *pBC, BC_IO_t *pIO,
	char *pName, off_t Offset, size_t Size, char *pData )
{
	int64_t	iS, iE, i, j;
	off_t	Off;
	C_Block_t	*pB;
	BC_Req_t	*pReq;
	bool_t		bIO;

	DlgLock( &pBC->bc_Ctrl );	// serialize start

	iS	= Offset/C_BLOCK_SIZE( pBC );
	iE	= (Offset + Size -1 )/C_BLOCK_SIZE(pBC);
	pIO->io_N	= iE - iS +1;
	pIO->io_aB	= (C_Block_t**)Malloc((iE-iS+1)*sizeof(C_Block_t*));

	for( i = iS, j = 0; i <= iE; i++, j++ ) {

		Off	= i*C_BLOCK_SIZE(pBC);

		bIO	= FALSE;

		pIO->io_aB[j] = pB = BCLock( pBC, pName, i, TRUE, &bIO );

		if( bIO ) {	// not Cached

			ASSERT( !(pB->b_Flags & C_BLOCK_ACTIVE) );

			pReq = Malloc( sizeof(*pReq) );

			list_init( &pReq->rq_Lnk );
			pReq->rq_Offset	= Off;
			pReq->rq_Size	= C_BLOCK_SIZE(pBC);
			pReq->rq_pData	= pB->b_pBuf;

			list_add_tail( &pReq->rq_Lnk, &pIO->io_Lnk );

			pB->b_Flags	|= C_BLOCK_DO_CKSUM;
		}
	}
	return( 0 );
}

int
BCReadDone( C_BCtrl_t *pBC, BC_IO_t *pIO,
	char *pName, off_t Offset, size_t Size, char *pData )
{
	int64_t	iS, iE, i, j;
	uint64_t	Cksum64;
	int64_t	Off, off, len;
	C_Block_t	*pB;
	timespec_t	Now;

	TIMESPEC( Now );

	iS	= Offset/C_BLOCK_SIZE( pBC );
	iE	= (Offset + Size -1 )/C_BLOCK_SIZE(pBC);

	for( i = iS, j = 0; i <= iE; i++, j++ ) {

		Off	= i*C_BLOCK_SIZE(pBC);

		if( Off < Offset )	off = Offset - Off;
		else				off = 0;

		if( Off + C_BLOCK_SIZE(pBC) < Offset + Size ) {
			len = C_BLOCK_SIZE(pBC) - off;
		} else {
			len = Offset + Size - Off - off;
		}

		pB = pIO->io_aB[j];

		// checksum check
		if( pBC->bc_bCksum ) {

			if( (pB->b_Flags & C_BLOCK_CKSUMED ) && pB->b_Cksum64
					&& (TIMESPECCOMPARE(Now, pB->b_Valid) > 0) ) {

LOG( pB->b_Dlg.d_pCtrl->dc_pLog, LogDBG,  
	"RE-CHECKSUM [%s]", pB->b_Dlg.d_Dlg.d_Name  );

				Cksum64	= in_cksum64( pB->b_pBuf, C_BLOCK_SIZE(pBC), 0 );
				// ASSERT !!!
				ASSERT( pB->b_Cksum64 == Cksum64 );

				// Set cksum valid time
				TIMESPEC( pB->b_Valid );
				TimespecAddUsec( pB->b_Valid, pBC->bc_Usec );
			}
			if( pB->b_Flags & C_BLOCK_DO_CKSUM ) {

				pB->b_Cksum64	= 
					in_cksum64( pB->b_pBuf, C_BLOCK_SIZE(pBC), 0 );

				TIMESPEC( pB->b_Valid );
				TimespecAddUsec( pB->b_Valid, pBC->bc_Usec );

				pB->b_Flags	&= ~C_BLOCK_DO_CKSUM;
				pB->b_Flags	|= (C_BLOCK_ACTIVE|C_BLOCK_CKSUMED);
			}
		}
		// read from cache
ASSERT( 0 <= off && off < pBC->bc_BlockSize );
ASSERT( 0 < len && len <= pBC->bc_BlockSize );
		memcpy( pData, pB->b_pBuf + off, len );
		pData	+= len;

	}
	DlgUnlock( &pBC->bc_Ctrl );	// serialize end

	return( 0 );
}

int
BCWriteIO( C_BCtrl_t *pBC, BC_IO_t *pIO,
	char *pName, off_t Offset, size_t Size, char *pData )
{
	int64_t	iS, iE, i, j;
	off_t	Off;
	C_Block_t	*pB;
	BC_Req_t	*pReq;

	iS	= Offset/C_BLOCK_SIZE( pBC );
	iE	= (Offset + Size - 1 )/C_BLOCK_SIZE(pBC);
	pIO->io_N	= iE - iS + 1;
	pIO->io_aB	= (C_Block_t**)Malloc( (iE-iS+1)*sizeof(C_Block_t*));

	// get write delegs.
	for( i = iS, j = 0; i <= iE; i++, j++ ) {

		Off	= i*C_BLOCK_SIZE(pBC);

		pIO->io_aB[j] = pB = BCLock( pBC, pName, i, FALSE, NULL );

		// Both ends(Copy On Write)
		if( (i == iS && Offset % C_BLOCK_SIZE(pBC) )
			|| (i == iE && iS != iE 
				&& (Offset+Size) % C_BLOCK_SIZE(pBC)) ) {

			pReq = Malloc( sizeof(*pReq) );

			list_init( &pReq->rq_Lnk );
			pReq->rq_Offset	= Off;
			pReq->rq_Size	= C_BLOCK_SIZE(pBC);
			pReq->rq_pData	= pB->b_pBuf;

			list_add_tail( &pReq->rq_Lnk, &pIO->io_Lnk );

			pB->b_Flags	|= C_BLOCK_DO_CKSUM;
		}

	}
	return( 0 );
}

int
BCWriteDone( C_BCtrl_t *pBC, BC_IO_t *pIO,
	char *pName, off_t Offset, size_t Size, char *pData )
{
	int64_t	iS, iE, i, j;
	int64_t	Off, off, len, offB, lenB;
	C_Block_t	*pB;
	uint64_t	sub, add, new;
	int	r;

	iS	= Offset/C_BLOCK_SIZE( pBC );
	iE	= (Offset + Size - 1 )/C_BLOCK_SIZE(pBC);

	for( i = iS, j = 0; i <= iE; i++, j++ ) {

		pB	= pIO->io_aB[j];

		Off	= i*C_BLOCK_SIZE(pBC);

		if( Off < Offset )	off = Offset - Off;
		else				off = 0;

		if( Off + C_BLOCK_SIZE(pBC) < Offset + Size ) {
			len = C_BLOCK_SIZE(pBC) - off;
		} else {
			len = Offset + Size - Off - off;
		}

		// write to cache
ASSERT( 0 <= off && off < pBC->bc_BlockSize );
ASSERT( 0 < len && len <= pBC->bc_BlockSize );
		if( pB->b_Flags & C_BLOCK_DO_CKSUM ) {	// Both End

			pB->b_Cksum64	= 
					in_cksum64( pB->b_pBuf, C_BLOCK_SIZE(pBC), 0 );

			TIMESPEC( pB->b_Valid );
			TimespecAddUsec( pB->b_Valid, pBC->bc_Usec );

			pB->b_Flags	&= ~C_BLOCK_DO_CKSUM;
			pB->b_Flags	|= C_BLOCK_CKSUMED;
		}
		if( pBC->bc_bCksum && 
				pB->b_Cksum64 && (pB->b_Flags & C_BLOCK_CKSUMED) ) {

			r		= off & 7;
			offB	= off - r;
			lenB	= len + r;

			sub	= in_cksum64( pB->b_pBuf + offB, lenB, 0 );
		}
		memcpy( pB->b_pBuf + off, pData, len );
		pData	+= len;

		pB->b_Flags	|= C_BLOCK_ACTIVE;

		// checksum check
		if( pBC->bc_bCksum ) {

			if( pB->b_Cksum64 && pB->b_Flags & C_BLOCK_CKSUMED ) {

				add	= in_cksum64( pB->b_pBuf + offB, lenB, 0 );

				new = in_cksum64_sub_add( pB->b_Cksum64, sub, add );

//printf("C_BLOCK_SIZE(pBC)=%d offB=%"PRIi64" lenB=%"PRIi64"\n"
//"b_Cksum64=%"PRIx64" sub=%"PRIx64" add=%"PRIx64" new=%"PRIx64"\n",
//C_BLOCK_SIZE(pBC), offB, lenB, pB->b_Cksum64, sub, add, new );

				pB->b_Cksum64	= new;
			} else {
				pB->b_Cksum64	= in_cksum64( pB->b_pBuf, C_BLOCK_SIZE(pBC), 0 );
			}
			// Set cksum valid time
			TIMESPEC( pB->b_Valid );
			TimespecAddUsec( pB->b_Valid, pBC->bc_Usec );

			pB->b_Flags	|= C_BLOCK_CKSUMED;
		}
	}
	return( 0 );
}

