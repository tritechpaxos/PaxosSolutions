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

#define	_DEBUG_
#define	_PAXOS_SERVER_

#include	"PFS.h"

int
PFSRasSend( struct Session *pSession, PFSHead_t *pF )
{
	PFSEventDir_t	*pEvent;
	int	DownId;
	msg_ras_t	Ras;
	int	err;
	PFSRasTag_t	*pRas;
	char	PATH[PAXOS_CELL_NAME_MAX];
	struct sockaddr_un	AdmAddr;
	int		Fd;

	pRas	= (PFSRasTag_t*)PaxosSessionGetTag( pSession );
	pEvent	= (PFSEventDir_t*)pF;

	DownId	= atoi( pEvent->ed_Entry );

	memset( PATH, 0 , sizeof(PATH) );
	memset( &AdmAddr, 0, sizeof(AdmAddr) );

	sprintf( PATH, PAXOS_ADMIN_PORT_FMT, pRas->r_Cell, pRas->r_Id );
	AdmAddr.sun_family	= AF_UNIX;
	strcpy( AdmAddr.sun_path, PATH );
	
	LOG( PaxosSessionGetLog( pSession ), LogINF, "PATH[%s]", AdmAddr.sun_path );

	if( (Fd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
		goto	err;
	}
	if( connect( Fd, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		LOG( PaxosSessionGetLog( pSession ),LogINF,  
					"errno=%d PATH[%s]", errno, AdmAddr.sun_path );
		goto	err1;
	}

	msg_init( &Ras.r_head, pRas->r_Id, MSG_RAS, sizeof(msg_ras_t) );
	Ras.r_head.m_fr	= DownId;

	err = SendStream( Fd, (void*)&Ras, sizeof(Ras) );
	if( err < 0 )	goto err1;

	LOG( PaxosSessionGetLog(pSession),LogINF,  
			"Fd=%d err=%d DownId[%d]", Fd, err, DownId );

	close( Fd );

	return( 0 );
err1:
	close( Fd );
err:
	return( -1 );
}

int
_PFSRasSend( struct Session *pSession, PFSHead_t *pF )
{
	PFSEventDir_t	*pEvent;
	PFSRasTag_t	*pRas;

	pRas	= (PFSRasTag_t*)PaxosSessionGetTag( pSession );
	pEvent	= (PFSEventDir_t*)pF;

LOG( PaxosSessionGetLog(pSession), LogINF, "mod[%d] path[%s] entry[%s]",
pEvent->ed_Mode, pEvent->ed_Name, pEvent->ed_Entry );

	if( pEvent->ed_Mode != DIR_ENTRY_DELETE )	return( -1 );
	if( strcmp( pEvent->ed_Name, pRas->r_Path ) )	return( -1 );

	PFSRasSend( pSession, pF );

	return( 0 );
}

/*
 *	RASセルへの登録
 */
int
PFSRasRegister( struct Session *pSession, char *pPath, char *pEntry )
{
	char			PathName[PATH_NAME_MAX];
	struct stat		Stat;
	int	Ret;
/*
 *	Register
 */
	PFSMkdir( pSession, pPath );

	sprintf( PathName, "%s/%s", pPath, pEntry );
	Ret = PFSStat( pSession, PathName, &Stat );
	if( !Ret ) {
		LOG( PaxosSessionGetLog(pSession), LogINF, "Exist[%s]", PathName );
	}
	PFSCreate( pSession, PathName, FILE_EPHEMERAL );
	LOG( PaxosSessionGetLog(pSession), LogINF, "Regist[%s]", PathName );
	return( 0 );
}

int
PFSRasUnregister( struct Session *pSession, char *pPath, char *pEntry )
{
	char			PathName[PATH_NAME_MAX];
	int	Ret;

	sprintf( PathName, "%s/%s", pPath, pEntry );

	Ret = PFSDelete( pSession, PathName );

	LOG( PaxosSessionGetLog(pSession), LogINF, "Unregist[%s]", PathName );
	return( Ret );
}

void*
PFSRasThread( void *pArg )
{
	struct Session *pSession = (struct Session*)pArg;
	int32_t	count, omitted;
	PFSRasTag_t	*pRas;
	// Session Open	in indpendent thread
static	int	No;

	pthread_detach( pthread_self() );

	pRas	= (PFSRasTag_t*)PaxosSessionGetTag( pSession );

LOG( PaxosSessionGetLog(pSession), LogDBG, "SessionOpen:BEFORE(%p)", pSession);
	if( PaxosSessionOpen( pSession, ++No, TRUE ) ) {
		LOG( PaxosSessionGetLog(pSession), LogERR, "Can't open RasCell[%s]", 
				PaxosSessionGetCell( pSession )->c_Name);
		errno	= ENOENT;
		goto err;
	}
LOG( PaxosSessionGetLog(pSession), LogDBG, "SessionOpen:AFTER");
	// Register myself
	if( PFSRasRegister( pSession,  pRas->r_Path, pRas->r_Ephem ) ) {
		LOG( PaxosSessionGetLog(pSession), LogERR, 
		"Can't register[%s:%s].", pRas->r_Path, pRas->r_Ephem );
		errno = EIO;
		goto err1;
	}
LOG( PaxosSessionGetLog(pSession), LogDBG, "PFSRasRegister:AFTER");

	LOG( PaxosSessionGetLog(pSession), LogINF, "RAS[%s] path[%s] file[%s]", 
	PaxosSessionGetCell( pSession )->c_Name, pRas->r_Path, pRas->r_Ephem );
/*
 *	Wait event
 */
	if( PFSEventDirSet( pSession, pRas->r_Path, pSession ) ) {
		goto err;
	}
LOG( PaxosSessionGetLog(pSession), LogDBG, "AFTER PFSEventDitrSet" );
	while(PFSEventGetByCallback( pSession, &count, &omitted, 
							pRas->r_fCallback ) == 0) {
	}
	LOG( PaxosSessionGetLog(pSession), LogINF, "Disconnected -> Register again");
err1:
	PaxosSessionGetfFree( pSession )( pRas );

	PaxosSessionClose( pSession );
	PaxosSessionFree( pSession );
err:

	LOG( PaxosSessionGetLog(pSession), LogINF, "exit");
	pthread_exit( 0 );
	return( NULL );
}

/*
 *	登録し、他のダウン通知で、リーダ選び(for Cell Severs)
 */
int
PFSRasThreadCreate( char *pCellName,
			struct Session *pSession, char *pPath, int Id,
			int (*fCallback)(struct Session*, PFSHead_t*) )
{
	pthread_t	th;
	PFSRasTag_t	*pRas;
	int Ret;

	pRas = (PFSRasTag_t*)PaxosSessionGetfMalloc(pSession)(sizeof(*pRas));
	memset( pRas, 0, sizeof(*pRas));

	strncpy( pRas->r_Path, RootOmitted(pPath), sizeof(pRas->r_Path) );
	sprintf( pRas->r_Ephem, "%d", Id );
	pRas->r_Id			= Id;
	pRas->r_fCallback	= fCallback;

	strncpy(pRas->r_Cell,pCellName, sizeof(pRas->r_Cell) );
	PaxosSessionSetTag(	pSession, pRas );

	Ret = pthread_create( &th, NULL, PFSRasThread, (void*)pSession );
	return( Ret );
}

