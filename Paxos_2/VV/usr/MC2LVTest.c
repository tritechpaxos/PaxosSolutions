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

#include	"VV.h"
#include	<inttypes.h>

void
usage(void)
{
	printf("MCTest MCMax cell LVname segment\n");
}

char	Buf[VV_LS_SIZE];
char	Buf1[VV_LS_SIZE];

MCCtrl_t	MCCtrl;

int
main( int ac, char *av[] )
{
	LS_t	LS;
	LV_IO_t	*pIO;
	list_t	Ent;
	int		Ret;

	if( ac < 4 ) {
		usage();
		exit( -1 );
	}
	MCInit( atoi(av[1]), &MCCtrl);

	list_init( &Ent );

	memset( &LS, 0, sizeof(LS) );
	strncpy( LS.ls_LV, av[3], sizeof(LS.ls_LV) );
	LS.ls_LS = atoi( av[4] );
	strncpy( LS.ls_Cell, av[2], sizeof(LS.ls_Cell) );

	/*
	 *	Read
	 */
	printf("0.=== MCReadIO:[%s:%d](0-%d) ===\n", 
				LS.ls_LV, LS.ls_LS, VV_LS_SIZE ); 

	memset( Buf, 0, sizeof(Buf) );

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf;
	pIO->io_Off	= 0;
	pIO->io_Len	= VV_LS_SIZE;

	Ret = MCReadIO( &LS, &Ent );
	printf("Read:Ret=%d\n", Ret );
	if( Ret < 0 )	goto err;

	/*
	 *	Read
	 */
	printf("1.=== MCReadIO:[%s:%d](10-19)(512-521) ===\n", 
				LS.ls_LV, LS.ls_LS ); 

	memset( Buf, 0, sizeof(Buf) );
	memset( Buf1, 0, sizeof(Buf1) );

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf;
	pIO->io_Off	= 10;
	pIO->io_Len	= 10;

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf1;
	pIO->io_Off	= 512;
	pIO->io_Len	= 10;

	Ret = MCReadIO( &LS, &Ent );

	printf("Read:Ret=%d\n", Ret );
	if( Ret < 0 )	goto err;
	printf("Buf[%s]\n", Buf );
	printf("Buf1[%s]\n", Buf1 );

	while( (pIO = list_first_entry( &Ent, LV_IO_t, io_Lnk )) ) {
		list_del( &pIO->io_Lnk );
		Free( pIO );
	} 
	/*
	 *	Write
	 */
	printf("2.=== MCWriteIO:%s(10-19)(512-521) ===\n", LS.ls_LV ); 
	strcpy( Buf, "0123456789" );
	strcpy( Buf1, "abcdefghij" );

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf;
	pIO->io_Off	= 10;
	pIO->io_Len	= 10;

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf1;
	pIO->io_Off	= 512;
	pIO->io_Len	= 10;

	Ret = MCWriteIO( &LS, &Ent );

	printf("Write:Ret=%d\n", Ret );
	if( Ret < 0 )	goto err;

	while( (pIO = list_first_entry( &Ent, LV_IO_t, io_Lnk )) ) {
		list_del( &pIO->io_Lnk );
		Free( pIO );
	} 
	/*
	 *	Read
	 */
	printf("3.=== MCReadIO:%s(10-19)(512-521) ===\n", LS.ls_LV ); 
	memset( Buf, 0, sizeof(Buf) );
	memset( Buf1, 0, sizeof(Buf1) );

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf;
	pIO->io_Off	= 10;
	pIO->io_Len	= 10;

	pIO	= (LV_IO_t*)Malloc( sizeof(*pIO) );
	memset( pIO, 0, sizeof(*pIO) );
	list_init( &pIO->io_Lnk );
	list_add_tail( &pIO->io_Lnk, &Ent );

	pIO->io_pBuf	= Buf1;
	pIO->io_Off	= 512;
	pIO->io_Len	= 10;

	Ret = MCReadIO( &LS, &Ent );

	printf("Read:Ret=%d\n", Ret );
	if( Ret < 0 )	goto err;
	printf("Buf[%s]\n", Buf );
	printf("Buf1[%s]\n", Buf1 );

	while( (pIO = list_first_entry( &Ent, LV_IO_t, io_Lnk )) ) {
		list_del( &pIO->io_Lnk );
		Free( pIO );
	} 
	/*
	 *	Delete
	 */
	Ret = MCDelete( &LS );
	printf("Delete:Ret=%d\n", Ret );

	return( 0 );
err:
	return( -1 );
}
