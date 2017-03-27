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

int
RasEyeLock( int Fd )
{
	RasLockReq_t	Req;
	RasLockRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_LOCK, 0, 0, sizeof(Req) );
	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
RasEyeUnlock( int Fd )
{
	RasUnlockReq_t	Req;
	RasUnlockRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_UNLOCK, 0, 0, sizeof(Req) );
	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

int
_RasDump( int Fd, char *pDataRemote, char *pData, int Len )
{
	int	Ret;

	RasDumpReq_t	Req;
	RasDumpRpl_t	Rpl;

	SESSION_MSGINIT( &Req, RAS_DUMP, 0, 0, sizeof(Req) );
	Req.d_pAddr	= pDataRemote;
	Req.d_Len	= Len;

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, pData, Rpl.d_Head.h_Len - sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	return( 0 );
err:
	return( -1 );
}

RasEyeCtrl_t	Ctrl, *pCtrl;

int
_RasEyeCtrlDump( int Fd )
{
	int	Ret;

	RasEyeCtrlDumpReq_t	Req;
	RasEyeCtrlDumpRpl_t	Rpl;

	SESSION_MSGINIT( &Req, RAS_EYE_CTRL, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	pCtrl	= Rpl.c_pCtrl;
	Ret = _RasDump( Fd, (char*)pCtrl, (char*)&Ctrl, sizeof(Ctrl) );
	if( Ret < 0 )	goto err;

	return( 0 );
err:
	return( -1 );
}

int
RasDump( int Fd )
{
	int	Ret;
	Ras_t	*pRas, Ras;
	int	i = 0;

	RasEyeLock( Fd );

	Ret = _RasEyeCtrlDump( Fd );
	if( Ret < 0 ) goto err;

	LIST_FOR_EACH_ENTRY( pRas, &Ctrl.rmc_RAS.hl_Lnk, r_Lnk,
							&Ctrl, pCtrl ) {
		Ret = _RasDump( Fd, (char*)pRas, (char*)&Ras, sizeof(Ras) );
		if( Ret < 0 )	goto err;

		printf("%d [%s:%s,%s,%s] RasSession=%p\n", i++,
			Ras.r_RasKey.rk_CellName, Ras.r_RasKey.rk_EventDir,
			Ras.r_Mail, Ras.r_Autonomic,
			Ras.r_pSession );

/***
		pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
					Send, Recv, Malloc, Free, Ras.r_RasKey.rk_CellName );

		Ret	= PaxosSessionOpen( pSession, 0, TRUE );
		if( Ret != 0 ) {
			PaxosSessionFree( pSession );
		} else {
			PFSDirent_t	aEntry[100];
			int	Ind = 0, Num = 100;

			while( Num == 100 ) {
				Ret = PFSReadDir( pSession, Ras.r_RasKey.rk_EventDir,
										aEntry, Ind, &Num );
				if( Ret )	break;
				if( Num == 0 )	break;

				for( i = 0; i < Num; i++ ) {
					if( aEntry[i].de_ent.d_type != DT_DIR ) {
						printf("\t[%s] type=%d\n", aEntry[i].de_Name, aEntry[i].de_ent.d_type );
					}
				}
				Ind += Num;
				Num = 100;
			}
			PaxosSessionClose( pSession);
			PaxosSessionFree( pSession );
		}
***/
		pRas	= &Ras;
	} 
	RasEyeUnlock( Fd );
	return( 0 );
err:
	RasEyeUnlock( Fd );
	return( -1 );
}

int
RasSet( int Fd, char *pCellName, char *pEventDir, 
				char *pMail, char *pAutonomic )
{
	RasSetReq_t	Req;
	RasSetRpl_t	Rpl;
	int	Ret;

	memset( &Req, 0, sizeof(Req) );

	SESSION_MSGINIT( &Req, RAS_EYE_SET, 0, 0, sizeof(Req) );

	strncpy( Req.s_RasKey.rk_CellName, pCellName, 
						sizeof(Req.s_RasKey.rk_CellName) );
	snprintf( Req.s_RasKey.rk_EventDir, 
				sizeof(Req.s_RasKey.rk_EventDir), "RAS/%s", pEventDir );
	if( pMail && strcmp("-", pMail))	strncpy( Req.s_Mail, pMail, sizeof(Req.s_Mail) );
	if( pAutonomic )	strncpy( Req.s_Autonomic, pAutonomic, 
										sizeof(Req.s_Autonomic) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.s_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RasUnset( int Fd, char *pCellName, char *pEventDir )
{
	RasSetReq_t	Req;
	RasSetRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_EYE_UNSET, 0, 0, sizeof(Req) );

	strncpy( Req.s_RasKey.rk_CellName, pCellName, 
						sizeof(Req.s_RasKey.rk_CellName) );
	snprintf( Req.s_RasKey.rk_EventDir, 
				sizeof(Req.s_RasKey.rk_EventDir), "RAS/%s", pEventDir );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.s_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
SessionDump( int Fd )
{
	int	Ret;
	RasEyeSession_t	*pRMS, RMS;
	int	i = 0;

	RasEyeLock( Fd );

	Ret = _RasEyeCtrlDump( Fd );
	if( Ret < 0 ) goto err;

	LIST_FOR_EACH_ENTRY( pRMS, &Ctrl.rmc_Session.hl_Lnk, s_Lnk,
							&Ctrl, pCtrl ) {
		Ret = _RasDump( Fd, (char*)pRMS, (char*)&RMS, sizeof(RMS) );
		if( Ret < 0 )	goto err;

		printf("%d [%s] Refer=%d\n", i++, RMS.s_CellName, RMS.s_Refer );

		pRMS	= &RMS;
	} 
	RasEyeUnlock( Fd );
	return( 0 );
err:
	RasEyeUnlock( Fd );
	return( -1 );
}

int
RegistMe( int Fd, char *pCellName )
{
	RegistReq_t	Req;
	RegistRpl_t	Rpl;
	int	Ret;


	SESSION_MSGINIT( &Req, RAS_EYE_REGIST, 0, 0, sizeof(Req) );

	strncpy( Req.r_CellName, pCellName, sizeof(Req.r_CellName) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.r_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
UnregistMe( int Fd, char *pCellName )
{
	RegistReq_t	Req;
	RegistRpl_t	Rpl;
	int	Ret;


	SESSION_MSGINIT( &Req, RAS_EYE_UNREGIST, 0, 0, sizeof(Req) );

	strncpy( Req.r_CellName, pCellName, sizeof(Req.r_CellName) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.r_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RegistDump( int Fd )
{
	int	Ret;
	RasEyeSession_t	*pRMS, RMS;
	int	i = 0;
	struct Session *pSession;

	RasEyeLock( Fd );

	Ret = _RasEyeCtrlDump( Fd );
	if( Ret < 0 ) goto err;

	LIST_FOR_EACH_ENTRY( pRMS, &Ctrl.rmc_Regist.hl_Lnk, s_LnkReg,
							&Ctrl, pCtrl ) {
		Ret = _RasDump( Fd, (char*)pRMS, (char*)&RMS, sizeof(RMS) );
		if( Ret < 0 )	goto err;

		printf("%d [%s] Refer=%d\n", i++, RMS.s_CellName, RMS.s_Refer );

		pSession = PaxosSessionAlloc( Connect, Shutdown, CloseFd, GetMyAddr,
					Send, Recv, Malloc, Free, RMS.s_CellName );

		Ret	= PaxosSessionOpen( pSession, 0, TRUE );
		if( Ret != 0 ) {
			PaxosSessionFree( pSession );
		} else {
			PFSDirent_t	aEntry[100];
			char	dir[512];
			int	Ind = 0, Num = 100;

			sprintf( dir, PAXOS_RAS_EYE_MY_RAS_FMT, Ctrl.rmc_Cluster );
			while( Num == 100 ) {
				Ret = PFSReadDir( pSession, dir, aEntry, Ind, &Num );
				if( Ret )	break;
				if( Num == 0 )	break;

				for( i = 0; i < Num; i++ ) {
					if( aEntry[i].de_ent.d_type != DT_DIR ) {
						printf("\t[%s] type=%d\n", aEntry[i].de_Name, aEntry[i].de_ent.d_type );
					}
				}
				Ind += Num;
				Num = 100;
			}
			PaxosSessionClose( pSession);
			PaxosSessionFree( pSession );
		}
		pRMS	= &RMS;
	} 
	RasEyeUnlock( Fd );
	return( 0 );
err:
	RasEyeUnlock( Fd );
	return( -1 );
}

int
RasEyeStop( int Fd )
{
	RasStopReq_t	Req;
	RasStopRpl_t	Rpl;
	int	Ret;


	SESSION_MSGINIT( &Req, RAS_STOP, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RasEyeLog( int Fd )
{
	RasLogReq_t	Req;
	RasLogRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_LOG, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RasEyeUnsetAll( int Fd )
{
	RasEyeUnsetAllReq_t	Req;
	RasEyeUnsetAllRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_EYE_UNSET_ALL, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RasEyeActive( int Fd )
{
	RasEyeActiveReq_t	Req;
	RasEyeActiveRpl_t	Rpl;
	int	Ret;
	struct timeval Timeout = { 0, 500000};

	SESSION_MSGINIT( &Req, RAS_EYE_ACTIVE, 0, 0, sizeof(Req) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = FdEventWait( Fd, &Timeout );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

void
usage(void)
{
printf("RasEyeAdm cluster id set cell path mail restart\n");
printf("RasEyeAdm cluster id unset cell path\n");
printf("RasEyeAdm cluster id ras_dump\n");
printf("RasEyeAdm cluster id regist_me cell\n");
printf("RasEyeAdm cluster id unregist_me cell\n");
printf("RasEyeAdm cluster id regist_dump\n");
printf("RasEyeAdm cluster id session_dump\n");
printf("RasEyeAdm cluster id stop\n");
printf("RasEyeAdm cluster id log\n");
printf("RasEyeAdm cluster id active\n");
printf("RasEyeAdm cluster id unset_all\n");
}

int
main( int ac, char *av[] )
{
	int	Ret;
	struct sockaddr_un	AddrAdm;
	int	Fd;

	if( ac < 4 ) {
		usage();
		goto err;
	}
	/* connect to AdmPort */
	AddrAdm.sun_family	= AF_UNIX;
	sprintf( AddrAdm.sun_path, PAXOS_RAS_EYE_ADM_FMT, av[1], av[2] );

	Fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( Fd < 0 ) {
		goto err;
	}
	if( (Ret = connect( Fd, (struct sockaddr*)&AddrAdm, sizeof(AddrAdm) ) )
					< 0 ) {
		goto err1;
	}
	if( !strcmp( "set", av[3] ) ) {
		if( ac < 6 ) {
			usage();
			goto err;
		}
		if( ac >= 8 ) Ret = RasSet( Fd, av[4], av[5], av[6], av[7] );
		else if( ac >= 7 )	Ret = RasSet( Fd, av[4], av[5], av[6], NULL );
		else if( ac >= 6 )	Ret = RasSet( Fd, av[4], av[5], NULL, NULL );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "unset", av[3] ) ) {
		if( ac < 6 ) {
			usage();
			goto err;
		}
		Ret = RasUnset( Fd, av[4], av[5] );
		if( Ret < 0 )	goto err1;
	} else if( !strcmp( "ras_dump", av[3] ) ) {
		Ret = RasDump( Fd );

	} else if( !strcmp( "session_dump", av[3] ) ) {
		Ret = SessionDump( Fd );

	} else if( !strcmp( "regist_me", av[3] ) ) {
		if( ac < 5 ) {
			usage();
			goto err;
		}
		Ret = RegistMe( Fd, av[4] );
	} else if( !strcmp( "unregist_me", av[3] ) ) {
		if( ac < 5 ) {
			usage();
			goto err;
		}
		Ret = UnregistMe( Fd, av[4] );

	} else if( !strcmp( "regist_dump", av[3] ) ) {
		Ret = RegistDump( Fd );

	} else if( !strcmp( "stop", av[3] ) ) {
		Ret = RasEyeStop( Fd );
	} else if( !strcmp( "log", av[3] ) ) {
		Ret = RasEyeLog( Fd );
	} else if( !strcmp( "acive", av[3] ) ) {
		Ret = RasEyeActive( Fd );
	} else if( !strcmp( "unset_all", av[3] ) ) {
		Ret = RasEyeUnsetAll( Fd );
	}
	close( Fd );
	return( 0 );
err1:
	close( Fd );
err:
	return( -1 );
}
