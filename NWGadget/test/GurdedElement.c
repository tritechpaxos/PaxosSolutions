/******************************************************************************
*
*  Copyright (C) 2010-2016 triTech Inc. All Rights Reserved.
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

#include	"NWGadget.h"

/* Gurd Control */
GrdElm_t	GE;
Hash_t	HashElm;

typedef struct Elm {
	int		e_Cnt;
	char	e_Key[64];
} Elm_t;

int
_Inc( GrdElm_t *pGE, void *pVoid )
{
	Elm_t	*pE = (Elm_t*)pVoid;

	DBG_PRT("pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	return( ++pE->e_Cnt );
}

int
_Dec( GrdElm_t *pGE, void *pVoid )
{
	Elm_t	*pE = (Elm_t*)pVoid;

	DBG_PRT("pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	return( --pE->e_Cnt );
}

int
_Cnt( GrdElm_t *pGE, void *pVoid )
{
	Elm_t	*pE = (Elm_t*)pVoid;

	DBG_PRT("pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	return( pE->e_Cnt );
}

int
_Add( GrdElm_t *pGE, void *pVoid )
{
	Elm_t	*pE = (Elm_t*)pVoid;

	DBG_PRT("pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	HashPut( &HashElm, pE->e_Key, pE );
	return( 0 );
}

int
_Del( GrdElm_t *pGE, void *pVoid )
{
	Elm_t	*pE = (Elm_t*)pVoid;

	DBG_PRT("FREE:pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	HashRemove( &HashElm, pE->e_Key );

	free( pE );

	return( 0 );
}

void*
_Find( GrdElm_t *pGE, void *pKey )
{
	Elm_t	*pE;

	pE = (Elm_t*)HashGet( &HashElm, pKey );
	if( pE ) DBG_PRT("pE=%p e_Cnt=%d\n", pE, pE->e_Cnt );
	else	DBG_PRT("pE=%p\n", pE );
	return( pE );
}

void*
_Create( GrdElm_t *pGE, void *pKey )
{
	Elm_t	*pE;

	pE = (Elm_t*)Malloc( sizeof(*pE) );
	memset( pE, 0, sizeof(*pE) );
	strcpy( pE->e_Key, pKey );

	HashPut( &HashElm, pE->e_Key, pE );

	DBG_PRT("pE=%p\n", pE );
	return( pE );
}

int
main( int ac, char* av[] )
{
	Elm_t	*pE;

	GrdElmInit( &GE, NULL, _Inc, _Dec, _Cnt, _Add, _Del, _Find, _Create );
	HashInit( &HashElm, PRIMARY_5, HASH_CODE_STR, HASH_CMP_STR, NULL,
				malloc, free, NULL );
	
#define	KEY1	"KEY"

	DBG_PRT("=== GEGet(TRUE),GEPut(TRUE) ===\n");
	pE = GrdElmGet( &GE, KEY1, TRUE );
	DBG_PRT(" pE=%p\n", pE );
	
	GrdElmPut( &GE, pE, TRUE );
	
	DBG_PRT("=== GEGet(TRUE),GEPut(FALSE) ===\n");
	pE = GrdElmGet( &GE, KEY1, TRUE );
	DBG_PRT(" pE=%p\n", pE );
	
	GrdElmPut( &GE, pE, TRUE );
	
	GrdElmLock( &GE );
	GrdElmUnlock( &GE );
	return( 0 );
}
