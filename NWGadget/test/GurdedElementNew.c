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
GElmCtrl_t	GE;

typedef struct Elm {
	GElm_t	e_Elm;
	char	e_Key[64];
} Elm_t;


GElm_t*
_Alloc( GElmCtrl_t *pGE )
{
	Elm_t	*pE;

	pE = (Elm_t*)Malloc( sizeof(*pE) );

	DBG_PRT("pE=%p\n", pE );
	return( &pE->e_Elm );
}

int
_Free( GElmCtrl_t *pGE, GElm_t *pElm )
{
	Free( pElm );
	DBG_PRT("pElm=%p\n", pElm );
	return( 0 );
}

int
_Unset( GElmCtrl_t *pGE, GElm_t *pElm, void **ppKey, bool_t bSave)
{
	Elm_t	*pE = (Elm_t*)pElm;

	*ppKey	= pE->e_Key;
	return( 0 );
}

int
_Set( GElmCtrl_t *pGE, GElm_t *pElm, void *pKey, void **ppKey, 
					void *pArg, bool_t bLoad )
{
	Elm_t	*pE = (Elm_t*)pElm;
	
	memset( pE->e_Key, 0, sizeof(pE->e_Key) );
	strncpy( pE->e_Key, (char*)pKey, sizeof(pE->e_Key) );
	*ppKey	= pE->e_Key;
	return( 0 );
}

int
main( int ac, char* av[] )
{
	Elm_t	*pE;

	GElmCtrlInit( &GE, NULL, _Alloc, _Free, _Set, _Unset, NULL, 3,
			PRIMARY_5, HASH_CODE_STR, HASH_CMP_STR, NULL, Malloc, Free, NULL );
	
#define	KEY1	"KEY"

	DBG_PRT("=== GElmGet(TRUE,TRUE),GElmPut(TRUE,TRUE) ===\n");
	pE =(Elm_t*) GElmGet( &GE, KEY1, NULL, TRUE, TRUE );
	DBG_PRT(" pE=%p\n", pE );
	
	GElmPut( &GE, &pE->e_Elm, TRUE, TRUE );
	
	DBG_PRT("=== GElmGet(TRUE,FALSE),GElmPut(FALSE) ===\n");
	pE = (Elm_t*)GElmGet( &GE, KEY1, NULL, TRUE, FALSE );
	DBG_PRT(" pE=%p\n", pE );
	
	GElmPut( &GE, &pE->e_Elm, FALSE, FALSE );
	
	GElmCtrlLock( &GE );
	GElmCtrlUnlock( &GE );
	return( 0 );
}
