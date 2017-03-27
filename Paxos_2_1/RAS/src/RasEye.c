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

#include	"Ras.h"

struct Log	*pLog;

RasEyeCtrl_t	RasEyeCtrl;

int
DirEventCallback( struct Session *pEvent, PFSHead_t *pH )
{
	PFSEventDir_t	*pE = (PFSEventDir_t*)pH;
	char	Ephemeral[PATH_NAME_MAX];
	char	Tmp[PATH_NAME_MAX];
	char	body[512];
	char	buff[1024];
	RasEyeSession_t	*pRES;
	File_t	*pF;
	int		fd;
	int		len, total = 0;
	struct Session	*pSession;
	RasVersion_t	Ver ={0,};
	time_t	Time;
	int		Diff;
	int		seq;
	int		Ret;
	char	LockName[PATH_NAME_MAX];
	PaxosSessionCell_t	*pCell;
	RasKey_t	RasKey;
	Ras_t		*pRas;
	char	Cell[FILE_NAME_MAX];

	memset( &RasKey, 0, sizeof(RasKey) );
	pCell = PaxosSessionGetCell( pEvent );

	strcpy( Tmp, pE->ed_Name );
	strcpy( Cell, basename(Tmp) );

	strncpy( RasKey.rk_CellName, pCell->c_Name, sizeof(RasKey.rk_CellName) );
	strncpy( RasKey.rk_EventDir, pE->ed_Name, sizeof(RasKey.rk_EventDir) );

	pRas = (Ras_t*)HashListGet( &RasEyeCtrl.rmc_RAS, &RasKey );
	if( !pRas ) {
		LOG( pLog, LogERR, "RasKey[%s:%s]",
			 RasKey.rk_CellName, RasKey.rk_EventDir );
		LogDump( pLog );
		return( 0 );
	}

	pRES = PaxosSessionGetTag( pEvent );
	pSession	= pRES->s_pSession;

	RwLockR( &RasEyeCtrl.rmc_RwLock );

	memset( body, 0, sizeof(body) );
	memset( Ephemeral, 0, sizeof(Ephemeral) );

	snprintf( Ephemeral, sizeof(Ephemeral),"%s/%s", 
				pE->ed_Name, pE->ed_Entry );

	memset( Tmp, 0, sizeof(Tmp) );

	switch( pE->ed_Mode ) {

	case DIR_ENTRY_CREATE:
		snprintf( body, sizeof(body), "%s[%s:%s](%u) is started. %s", 
			pCell->c_Name,pE->ed_Name, pE->ed_Entry, pE->ed_EventSeq, 
			ctime(&pE->ed_Time) );
		body[strlen(body)-1]	= 0;
		break;

	case DIR_ENTRY_DELETE:
		snprintf( body, sizeof(body), "%s[%s:%s](%u) is stopped. %s", 
			pCell->c_Name,pE->ed_Name, pE->ed_Entry, pE->ed_EventSeq, 
			ctime(&pE->ed_Time) );
		body[strlen(body)-1]	= 0;

		snprintf( Ephemeral, sizeof(Ephemeral),"%s/%s_BAK", 
				pE->ed_Name, pE->ed_Entry );
		LOG( pLog, LogDBG, "Ephemeral[%s]", Ephemeral );

		pF = PFSOpen( pSession, Ephemeral, 0 );
		if( pF ) {
			sprintf( Tmp, "%s/%s", pRES->s_CellName, pE->ed_Entry );
			fd	= open( Tmp, O_CREAT|O_RDWR, 0666 );
			if( fd > 0 ) {
				while( (len = PFSRead( pF, buff, sizeof(buff))) > 0 ) {
					write( fd, buff, len );
					total += len;
				}
				close( fd );
			} else {
				sprintf( buff, "Can't open local file[%s]", pE->ed_Entry );

				LOG( pLog, LogDBG, "%s", buff );
				strcat( body, "\n" );
				strcat( body, buff );
			}
			PFSClose( pF );
		}
		break;

	case DIR_ENTRY_UPDATE:
		snprintf( body, sizeof(body), "%s[%s:%s](%u) is updated. %s", 
			pCell->c_Name,pE->ed_Name, pE->ed_Entry, pE->ed_EventSeq, 
			ctime(&pE->ed_Time) );
		body[strlen(body)-1]	= 0;

		snprintf( Ephemeral, sizeof(Ephemeral),"%s/%s", 
				pE->ed_Name, pE->ed_Entry );
		LOG( pLog, LogDBG, "Ephemeral[%s]", Ephemeral );

		pF = PFSOpen( pSession, Ephemeral, 0 );
		if( pF ) {
			sprintf( Tmp, "%s/%s", pRES->s_CellName, pE->ed_Entry );
			fd	= open( Tmp, O_CREAT|O_RDWR, 0666 );
			if( fd > 0 ) {
				while( (len = PFSRead( pF, buff, sizeof(buff))) > 0 ) {
					write( fd, buff, len );
					total += len;
				}
				close( fd );
			} else {
				sprintf( buff, "Can't open local file[%s]", pE->ed_Entry );

				LOG( pLog, LogDBG, "%s", buff );
				strcat( body, "\n" );
				strcat( body, buff );
			}
			PFSClose( pF );
		}
		break;
	}
	memset( LockName, 0, sizeof(LockName) );

	sprintf( LockName, "%s/RasEye/%s", 
					pE->ed_Name, RasEyeCtrl.rmc_Cluster );

	LOG( pLog, LogINF, "body[%s] LockName[%s]", body, LockName );

retry:
	PFSLockW( pSession, LockName );

	pF = PFSOpen( pSession, LockName, 0 );

	if( !pF ) {
		LOG( pLog, LogERR, "Can't PFSOpen[%s]", LockName );
		goto err1;
	}
	Time	= time(0);
	Diff	= RasEyeCtrl.rmc_VerDiffSec;

	Ret = PFSRead( pF, (char*)&Ver, sizeof(Ver) );

	if( Ret > 0 && (Time - Diff < Ver.v_Time) ) {

		seq = SEQ_CMP( pE->ed_EventSeq, Ver.v_Seq );
		if( seq <= 0 ) {
			LOG(pLog, LogINF, 
				"SKIP:EventSeq=%u v_Seq=%u", pE->ed_EventSeq, Ver.v_Seq );
			goto skip;
		}

		if( seq > 1 ) {
			PFSClose( pF );
			PFSUnlock( pSession, LockName );
			LOG(pLog, LogERR, 
				"RETRY:EventSeq=%u v_Seq=%u", pE->ed_EventSeq, Ver.v_Seq );
			goto retry;	
		}
	}
	if( pRas->r_Mail[0] != 0 ) {
		sprintf( buff, "%s %s %s event \"%s\" %s >/dev/null 2>&1", 
					pRas->r_Mail, RasEyeCtrl.rmc_Cluster, RasEyeCtrl.rmc_Id,
					body, Tmp );
		LOG(pLog, LogINF, "Mail[%s]", buff );
		Ret = system( buff );
		if( Ret ) {
			LOG(pLog, LogERR, "Mail ERROR[%s]=%d", buff, Ret );
		}
	}
	if( pRas->r_Autonomic[0] != 0 ) {

		sprintf( buff, "%s %s -i %s -r %s -s %u -m %d -p %s -t %lu -C %s -I %s >/dev/null 2>&1",
			pRas->r_Autonomic, Cell, pE->ed_Entry, pRas->r_RasKey.rk_CellName,
			pE->ed_EventSeq, pE->ed_Mode, pE->ed_Name, pE->ed_Time,
			RasEyeCtrl.rmc_Cluster, RasEyeCtrl.rmc_Id );
		LOG(pLog, LogINF, "Autonomic[%s]", buff );
		Ret = system( buff );
		if( Ret ) {
			LOG(pLog, LogERR, "Autonomic ERROR[%s]=%d", buff, Ret );
		}
	}


	Ver.v_Time	= Time;
	Ver.v_Seq	= pE->ed_EventSeq;

	PFSLseek( pF, 0 );
	Ret = PFSWrite( pF, (char*)&Ver, sizeof(Ver) );	

skip:
	PFSClose( pF );
	PFSUnlock( pSession, LockName );

	RwUnlock( &RasEyeCtrl.rmc_RwLock );

	return( 0 );
err1:
	PFSUnlock( pSession, LockName );
	RwUnlock( &RasEyeCtrl.rmc_RwLock );
	return( -1 );
}

/*	Session */
struct Session*
RasSessionOpen( char *pCellName, int SessionNo, bool_t IsSync )
{
	struct Session *pSession = NULL; 
	int	Ret;
	char	Mkdir[512];

	pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
								Send, Recv, Malloc, Free, pCellName );
	if( !pSession )	goto err;

	if( pLog )	PaxosSessionSetLog( pSession, pLog );

	Ret	= PaxosSessionOpen( pSession, SessionNo, IsSync );
	if( Ret != 0 ) {
		errno = EACCES;
		goto err1;
	}
	sprintf( Mkdir, "mkdir -p %s", pCellName );
	system( Mkdir );

	return( pSession );
err1:
	PaxosSessionFree( pSession );
err:
	return( NULL );
}

void
RasSessionClose( struct Session *pSession )
{
	PaxosSessionClose( pSession );
	PaxosSessionFree( pSession );
}

void
RasSessionAbort( struct Session *pSession )
{
	PaxosSessionAbort( pSession );
	PaxosSessionFree( pSession );
}

void*
ThreadEvent( void *pArg )
{
	RasEyeSession_t *pRES = (RasEyeSession_t*)pArg;
	int	count, omitted;

	while( PFSEventGetByCallback( pRES->s_pEvent, &count, &omitted, 
							DirEventCallback) == 0) {
	}
	pthread_exit( 0 );
}

RasEyeSession_t*
_GetRasEyeSession( char *pCellName )
{
	RasEyeSession_t	*pRES;

	pRES = (RasEyeSession_t*)HashListGet(&RasEyeCtrl.rmc_Session, pCellName);
	if( !pRES ) {
		pRES	= (RasEyeSession_t*)Malloc( sizeof(*pRES) );
		memset( pRES, 0, sizeof(*pRES) );

		strncpy( pRES->s_CellName, pCellName, sizeof(pRES->s_CellName) );

		pRES->s_pSession	= RasSessionOpen( pCellName, 0, TRUE );
		if( !pRES->s_pSession )	goto err;

		pRES->s_pEvent		= RasSessionOpen( pCellName, 1, TRUE );
		if( !pRES->s_pEvent )	goto err;

		PaxosSessionSetTag( pRES->s_pEvent, pRES );

		list_init( &pRES->s_Lnk );
		list_init( &pRES->s_LnkReg );
		HashListPut( &RasEyeCtrl.rmc_Session, pRES->s_CellName, pRES, s_Lnk );

		/* GetEvent thread */
		pthread_create( &pRES->s_ThreadEvent, NULL, ThreadEvent, (void*)pRES );
	}
	pRES->s_Refer++;

	return( pRES );
err:
	if( pRES->s_pEvent )	RasSessionClose( pRES->s_pEvent );
	if( pRES->s_pSession )	RasSessionClose( pRES->s_pSession );
	Free( pRES );
	return( NULL );
}

int
_PutRasEyeSession( RasEyeSession_t *pRES )
{
	if( --pRES->s_Refer == 0 ) {

		RasSessionAbort( pRES->s_pEvent );
		RasSessionAbort( pRES->s_pSession );

		HashListDel( &RasEyeCtrl.rmc_Session, pRES->s_CellName, 
									RasEyeSession_t*, s_Lnk );
		Free( pRES );
	}
	return( 0 );
}

int
_RasCode( void *pVoid )
{
	RasKey_t	*pRasKey = (RasKey_t*)pVoid;
	unsigned int	Code;
	long	CodeCellName;
	long	CodeDirName;
	unsigned long	CodeLong;

	CodeCellName	= HashStrCode( pRasKey->rk_CellName );
	CodeDirName		= HashStrCode( pRasKey->rk_EventDir );
	
	CodeLong	= (unsigned long)(CodeCellName + CodeDirName);

	Code	= ((CodeLong&0xffffffff00000000LL )>>32) + (CodeLong&0xffffffffLL);	

	return( (int)Code );
}

int
_RasCmp( void *pA, void *pB )
{
	int	Ret;

	RasKey_t	*pRasKeyA = (RasKey_t*)pA;
	RasKey_t	*pRasKeyB = (RasKey_t*)pB;

	Ret = strncmp( pRasKeyA->rk_CellName, pRasKeyB->rk_CellName,
									sizeof( pRasKeyA->rk_CellName) );	
	if( Ret ) {
		Ret = strncmp(pRasKeyA->rk_EventDir, pRasKeyB->rk_EventDir,
								sizeof( pRasKeyA->rk_EventDir));	
	}
	return( Ret );
}

static void*
ThreadAdm( void *pArg )
{
	RasEyeCtrl_t	*pRasEyeCtrl = (RasEyeCtrl_t*)pArg;
	struct sockaddr_un AdmAddr;
	int	FdAdm, ad;
	int	Ret;
	bool_t	bLock = FALSE;

	RasEyeSession_t	*pRasEyeSession, *pRESW;
	PaxosSessionHead_t	M, *pM;
	PaxosSessionHead_t	Rpl;

	RasSetReq_t		*pReqSet;
	RasSetRpl_t		RplSet;
	Ras_t			*pRas, *pRasW;

	RegistReq_t		*pReqRegist;
	RegistRpl_t		RplRegist;

	char	buff[1024];

	/* 	Admin Port */
	AdmAddr	= pRasEyeCtrl->rmc_AdmAddr;

	unlink( AdmAddr.sun_path ); 

	if( (FdAdm = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
		goto err;
	}
	if( bind( FdAdm, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		goto err1;
	}
	listen( FdAdm, 1 );

	while( 1 ) {

		ad	= accept( FdAdm, NULL, NULL );
		if( ad < 0 ) {
			LOG( pLog, LogERR, "accept ERROR" );
			continue;
		}
		LOG( pLog, LogINF, "accepted(%d)", ad );
		while( 1 ) {
			Ret = PeekStream( ad, (char*)&M, sizeof(M) );
			if( Ret ) {
				if( bLock ) {
					bLock = FALSE;
					RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				}
				LOG( pLog, LogERR, "peek ERROR", Ret, M.h_Len );
				break;
			}

			pM	= Malloc( M.h_Len );
			ASSERT( pM );

			if( (Ret = RecvStream( ad, (char*)pM, M.h_Len ) ) ) {
				LOG( pLog, LogERR, "recv ERROR Ret=%d h_Len=%d", Ret, M.h_Len );
				Free( pM );
				break;
			}
			errno = 0;

			switch( (int)pM->h_Cmd ) {

			case RAS_LOCK:

				if( bLock ) {
					errno = EBUSY;	
				} else {
					RwLockW( &pRasEyeCtrl->rmc_RwLock );
					bLock	= TRUE;
				}

   				SESSION_MSGINIT( &Rpl, RAS_LOCK, 0, errno, sizeof(Rpl) );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				LOG( pLog, LogINF, "RAS_LOCK");
				break;

			case RAS_UNLOCK:
				if( bLock ) {
					errno = ENOENT;
				} else {
					bLock	= FALSE;
					RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				}
   				SESSION_MSGINIT( &Rpl, RAS_UNLOCK, 0, errno, sizeof(Rpl) );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				LOG( pLog, LogINF, "RAS_UNLOCK");
				break;

			case RAS_DUMP:
				{RasDumpReq_t *pReqDump = (RasDumpReq_t*)pM;
   				SESSION_MSGINIT( &Rpl, RAS_DUMP, 
								0, 0, sizeof(Rpl) + pReqDump->d_Len );

				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );
				SendStream( ad, (char*)pReqDump->d_pAddr, 
								pReqDump->d_Len );
				}
				break;

			case RAS_EYE_CTRL:
				{RasEyeCtrlDumpRpl_t RplCtrl;
   				SESSION_MSGINIT( &RplCtrl, RAS_EYE_CTRL, 
										0, 0, sizeof(RplCtrl) );
				RplCtrl.c_pCtrl = &RasEyeCtrl;
				SendStream( ad, (char*)&RplCtrl, sizeof(RplCtrl) );
				}
				break;

			case RAS_EYE_SET:
				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				pReqSet	= (RasSetReq_t*)pM;

				pRas = (Ras_t*)HashListGet( &RasEyeCtrl.rmc_RAS, 
											&pReqSet->s_RasKey  );
				if( pRas ) {
					errno = EEXIST;
					LOG( pLog, LogERR, "[%s:%s] exists",
							pReqSet->s_RasKey.rk_CellName, 
							pReqSet->s_RasKey.rk_EventDir );
				} else {

					pRasEyeSession	= _GetRasEyeSession( 
									pReqSet->s_RasKey.rk_CellName );	

					if( pRasEyeSession ) {

						Ret = PFSEventDirSet( pRasEyeSession->s_pSession, 
									pReqSet->s_RasKey.rk_EventDir, 
									pRasEyeSession->s_pEvent );
						if( Ret == 0 ) {
							pRas	= (Ras_t*)Malloc( sizeof(*pRas) );
							memset( pRas, 0, sizeof(*pRas) );

							list_init( &pRas->r_Lnk );
							strncpy( pRas->r_RasKey.rk_CellName, 
								pReqSet->s_RasKey.rk_CellName,
								sizeof(pRas->r_RasKey.rk_CellName) );
							strncpy( pRas->r_RasKey.rk_EventDir, 
								pReqSet->s_RasKey.rk_EventDir,
								sizeof(pRas->r_RasKey.rk_EventDir) );
							if( pReqSet->s_Mail[0] ) {
								strncpy( pRas->r_Mail, pReqSet->s_Mail,
												sizeof(pRas->r_Mail) );
							}
							if( pReqSet->s_Autonomic[0] ) {
								strncpy( pRas->r_Autonomic, 
										pReqSet->s_Autonomic,
										sizeof(pRas->r_Autonomic) );
							}

							pRas->r_pSession	= pRasEyeSession;

							HashListPut( &RasEyeCtrl.rmc_RAS, 
										&pRas->r_RasKey, pRas, r_Lnk );
						} else {

							_PutRasEyeSession( pRasEyeSession );
						}
						LOG( pLog, LogINF, "PFSEventDirSet[%s:%s] Ret=%d",
							pReqSet->s_RasKey.rk_CellName, 
							pReqSet->s_RasKey.rk_EventDir, Ret );
					}
				}
				SESSION_MSGINIT( &RplSet, RAS_EYE_SET, 0, errno, 
											sizeof(RplSet) );
				SendStream( ad, (char*)&RplSet, sizeof(RplSet) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_EYE_UNSET:
				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				pReqSet	= (RasSetReq_t*)pM;

				pRas = (Ras_t*)HashListGet( &RasEyeCtrl.rmc_RAS, 
											&pReqSet->s_RasKey  );
				if( !pRas ) {
					errno = ENOENT;
					LOG( pLog, LogERR, "[%s:%s] not exists",
								pReqSet->s_RasKey.rk_CellName, 
								pReqSet->s_RasKey.rk_EventDir );
				} else {
					Ret = PFSEventDirCancel( pRas->r_pSession->s_pSession, 
									pRas->r_RasKey.rk_EventDir, 
									pRas->r_pSession->s_pEvent );
					if( Ret == 0 ) {

						_PutRasEyeSession( pRas->r_pSession );
						HashListDel( &RasEyeCtrl.rmc_RAS, &pRas->r_RasKey,
										Ras_t*, r_Lnk );
						Free( pRas );
					}
					LOG( pLog, LogINF, "PFSEventDirCancel[%s:%s] Ret=%d",
							pReqSet->s_RasKey.rk_CellName, 
							pReqSet->s_RasKey.rk_EventDir, Ret );
				}
				SESSION_MSGINIT( &RplSet, RAS_EYE_UNSET, 0, errno, 
								sizeof(RplSet) );
				SendStream( ad, (char*)&RplSet, sizeof(RplSet) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_EYE_REGIST:
				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				pReqRegist	= (RegistReq_t*)pM;

				if( HashListGet( &RasEyeCtrl.rmc_Regist, 
										pReqRegist->r_CellName ) ) {
					errno = EEXIST;
					LOG( pLog, LogERR, "[%s] exists", pReqRegist->r_CellName );
				} else {
					pRasEyeSession = 
								_GetRasEyeSession( pReqRegist->r_CellName );
					if( !pRasEyeSession ) {
						errno = EACCES;
						LOG( pLog, LogERR, "_GetRasEyeSession[%s]", 
									pReqRegist->r_CellName );
					} else {
						memset( buff, 0, sizeof(buff) );
						sprintf( buff, PAXOS_RAS_EYE_MY_RAS_FMT,
										RasEyeCtrl.rmc_Cluster );

						Ret = PFSRasRegister( pRasEyeSession->s_pSession,
												buff, RasEyeCtrl.rmc_Id );
						if( Ret < 0 ) {
							errno = EIO;
						} else {
							HashListPut( &RasEyeCtrl.rmc_Regist, 
								pRasEyeSession->s_CellName, pRasEyeSession,
								s_LnkReg );
						}
						LOG( pLog, LogINF, "PFSRasRegister[%s] Ret=%d", 
											buff, Ret );
					}
				}
				SESSION_MSGINIT( &RplRegist, RAS_EYE_REGIST, 
									0, errno, sizeof(RplRegist) );
				SendStream( ad, (char*)&RplRegist, sizeof(RplRegist) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_EYE_UNREGIST:
				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				pReqRegist	= (RegistReq_t*)pM;

				if( !(pRasEyeSession = HashListGet( &RasEyeCtrl.rmc_Regist, 
										pReqRegist->r_CellName ) ) ) {
					errno = ENOENT;
					LOG( pLog, LogERR, "[%s] not exists",
								pReqRegist->r_CellName );
				} else {
					memset( buff, 0, sizeof(buff) );
					sprintf( buff, PAXOS_RAS_EYE_MY_RAS_FMT,
										RasEyeCtrl.rmc_Cluster );
					Ret = PFSRasUnregister( pRasEyeSession->s_pSession,
												buff, RasEyeCtrl.rmc_Id );

					if( Ret < 0 ) {
						errno = EIO;
					} else {
						HashListDel( &RasEyeCtrl.rmc_Regist, 
								pRasEyeSession->s_CellName,
								RasEyeSession_t*, s_LnkReg );

						_PutRasEyeSession( pRasEyeSession );
					}
					LOG( pLog, LogINF, "PFSRasUnregsiter[%s] Ret=%d", 
										buff, Ret );
				}
				SESSION_MSGINIT( &RplRegist, RAS_EYE_UNREGIST, 
									0, errno, sizeof(RplRegist) );
				SendStream( ad, (char*)&RplRegist, sizeof(RplRegist) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_LOG:
				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				LogDump( pLog );

				SESSION_MSGINIT( &Rpl, RAS_LOG, 0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_EYE_UNSET_ALL:

				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				errno = 0;

				/* unset RAS */
				list_for_each_entry_safe( pRas, pRasW, 
						&RasEyeCtrl.rmc_RAS.hl_Lnk, r_Lnk ) {

					Ret = PFSEventDirCancel( pRas->r_pSession->s_pSession, 
									pRas->r_RasKey.rk_EventDir, 
									pRas->r_pSession->s_pEvent );

					if( Ret ) {
						LOG( pLog, LogERR, "[%s:%s]", 
							pRas->r_RasKey.rk_CellName, 
							pRas->r_RasKey.rk_EventDir );
						continue;
					}
					_PutRasEyeSession( pRas->r_pSession );

					HashListDel( &RasEyeCtrl.rmc_RAS, &pRas->r_RasKey,
									Ras_t*, r_Lnk );
					Free( pRas );
				}
				SESSION_MSGINIT( &Rpl, RAS_EYE_UNSET_ALL, 
									0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			case RAS_STOP:

				RwLockW( &pRasEyeCtrl->rmc_RwLock );

				errno = 0;

				/* unset RAS */
				list_for_each_entry_safe( pRas, pRasW, 
						&RasEyeCtrl.rmc_RAS.hl_Lnk, r_Lnk ) {

					Ret = PFSEventDirCancel( pRas->r_pSession->s_pSession, 
									pRas->r_RasKey.rk_EventDir, 
									pRas->r_pSession->s_pEvent );

					if( Ret ) {
						LOG( pLog, LogERR, "[%s:%s]", 
							pRas->r_RasKey.rk_CellName, 
							pRas->r_RasKey.rk_EventDir );
						continue;
					}
					_PutRasEyeSession( pRas->r_pSession );

					HashListDel( &RasEyeCtrl.rmc_RAS, &pRas->r_RasKey,
									Ras_t*, r_Lnk );
					Free( pRas );
				}
				/* unregist me */
				list_for_each_entry_safe( pRasEyeSession, pRESW, 
							&RasEyeCtrl.rmc_Regist.hl_Lnk, s_LnkReg ) {

					Ret = PFSRasUnregister( pRasEyeSession->s_pSession,
												buff, RasEyeCtrl.rmc_Id );
					HashListDel( &RasEyeCtrl.rmc_Regist, 
								pRasEyeSession->s_CellName,
								RasEyeSession_t*, s_LnkReg );

					_PutRasEyeSession( pRasEyeSession );
				}
				LogDump( pLog );

				SESSION_MSGINIT( &Rpl, RAS_STOP, 0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				exit( 0 );

			case RAS_EYE_ACTIVE:
				RwLockR( &pRasEyeCtrl->rmc_RwLock );

				SESSION_MSGINIT( &Rpl, RAS_EYE_ACTIVE, 0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasEyeCtrl->rmc_RwLock );
				break;

			default:
				break;
			}
		}
		close( ad );
	}
	return( 0 );
err1:
	close( FdAdm );
err:
	return( (void*)-1 );
}

void
usage()
{
	printf("RasEye [-d dd] cluster id\n");
	printf("\t-d	... stale time(%d sec)\n", DEFAULT_VER_DIFF_SEC );
}

int
main( int ac, char **av )
{
	int	VerDiffSec	= DEFAULT_VER_DIFF_SEC;
	int	j;

	if( ac < 3 ) {
		usage();
		exit( -1 );
	}
	/*
	 * Opts
	 */
	j = 1;
	while( j < ac ) {
		char	*pc;
		int		l;

		pc = av[j++];
		if( *pc != '-' )	break;

		l = strlen( pc );
		if( l < 2 )	continue;

		switch( *(pc+1) ) {
		case 'd':	 VerDiffSec = atoi(av[j++]);	break;
		}
	}

	if( j > 2 ) {
		ac -= j - 2;
		av	= &av[j-2];
	}

	strncpy( RasEyeCtrl.rmc_Cluster, av[1], sizeof(RasEyeCtrl.rmc_Cluster) );
	strncpy( RasEyeCtrl.rmc_Id, av[2], sizeof(RasEyeCtrl.rmc_Id) );

	RasEyeCtrl.rmc_VerDiffSec	= VerDiffSec;

	/* Log */
	if( getenv("LOG_SIZE") ) {

		int	log_level	= LogINF;
		int	log_size	= 0;

		log_size	= atoi( getenv("LOG_SIZE") );
		if( getenv("LOG_LEVEL") ) log_level	= atoi( getenv("LOG_LEVEL") );
		pLog = LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
							stderr, log_level );
	}

	RwLockInit( &RasEyeCtrl.rmc_RwLock );

	HashListInit( &RasEyeCtrl.rmc_Session, 
		PRIMARY_101, HASH_CODE_STR, HASH_CMP_STR, NULL, Malloc, Free, NULL );

	HashListInit( &RasEyeCtrl.rmc_RAS, 
		PRIMARY_1009, _RasCode, _RasCmp, NULL, Malloc, Free, NULL );

	HashListInit( &RasEyeCtrl.rmc_Regist, 
		PRIMARY_11, HASH_CODE_STR, HASH_CMP_STR, NULL, Malloc, Free, NULL );

	/* Admin thread  */
	RasEyeCtrl.rmc_AdmAddr.sun_family	= AF_UNIX;
	sprintf( RasEyeCtrl.rmc_AdmAddr.sun_path, PAXOS_RAS_EYE_ADM_FMT,
				RasEyeCtrl.rmc_Cluster, RasEyeCtrl.rmc_Id );

	LOG( pLog, LogINF, "AdmPath[%s]", RasEyeCtrl.rmc_AdmAddr.sun_path );

	ThreadAdm( &RasEyeCtrl );

	return( 0 );
}
