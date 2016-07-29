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

#include	<string.h>
#include	"neo_skl.h"
#include	"neo_usradm.h"

void
neo_setusradm( up, procbname, pid, myname, hostname, pd )
	usr_adm_t	*up;
	char		*procbname;
	int		pid;
	char		*myname;
	char		*hostname;
	int		pd;
{
	strncpy( up->u_procname, procbname, sizeof(up->u_procname) );
	up->u_pid	= pid;
	strncpy( up->u_portname, myname, sizeof(up->u_portname) );
	strncpy( up->u_hostname, hostname, sizeof(up->u_hostname) );
	up->u_portno	= pd;
}

void
_neo_log_open( char *pName )
{
	int	log_size;

	if( getenv("LOG_SIZE") && !neo_log ) {
		log_size = atoi( getenv("LOG_SIZE") );

#ifdef	ZZZ
		neo_log	= LogCreate( log_size, neo_malloc, neo_free, LOG_FLAG_BINARY, stderr );
#else
		neo_log	= LogCreate( log_size, neo_malloc, neo_free, LOG_FLAG_BINARY, 
							stderr, LogDBG );
#endif
	}
}

void
_neo_log_close()
{
	if( neo_log ) {
		LogDump( neo_log );
		LogDestroy( neo_log );
		neo_log	= NULL;
	}
}

