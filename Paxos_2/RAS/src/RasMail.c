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

RasMailCtrl_t	RasMailCtrl;

int
_MailSend( char *pBody, char *pTmp, char *pSubject, char *pMail )
{
	char	cmd[4096];
	int		Ret;

	if( pTmp ) {
		sprintf( cmd, "(echo \"%s\"; uuencode %s %s.uue;)| mail -s %s %s", 
								pBody, pTmp, pTmp, pSubject, pMail );
	} else {
		sprintf( cmd, "echo \"%s\" | mail -s %s %s", 
								pBody, pSubject, pMail );
	}
	LOG( pLog, LogINF, "system(%s)", cmd );
	Ret = system( cmd );
	return( Ret );
}

int
MailSend( char *pBody, char *pFile, char *pCluster )
{
	MailAddr_t	*pMailAddr;
	int		len, total = 0;
	char	Mail[2048];

	total = 0;
	Mail[0]	= 0;
	list_for_each_entry( pMailAddr, &RasMailCtrl.rmc_Mail.hl_Lnk, m_Lnk ) {

		len	= strlen( pMailAddr->m_MailAddr );
		if( total + len >= sizeof(Mail) ) {

			_MailSend( pBody, pFile, pCluster, Mail );

			total = 0; Mail[0] = 0;
		}
		strcat( Mail, pMailAddr->m_MailAddr );
		strcat( Mail, " ");
		total += len + 1;
	}
	if( total ) {
		_MailSend( pBody, pFile, pCluster, Mail );
	}
	return( 0 );
}

static void*
ThreadAdm( void *pArg )
{
	RasMailCtrl_t	*pRasMailCtrl = (RasMailCtrl_t*)pArg;
	struct sockaddr_un AdmAddr;
	int	FdAdm, ad;
	int	Ret;
	bool_t	bLock = FALSE;

	PaxosSessionHead_t	M, *pM;
	PaxosSessionHead_t	Rpl;

	MailAddReq_t	*pReqAdd;
	MailAddRpl_t	RplAdd;
	MailAddr_t		*pMailAddr, *pMAW;
	RasMailSendReq_t	*pReqMail;
	RasMailSendRpl_t	RplMail;

	/* 	Admin Port */
	AdmAddr	= pRasMailCtrl->rmc_AdmAddr;

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
					RwUnlock( &pRasMailCtrl->rmc_RwLock );
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
					RwLockW( &pRasMailCtrl->rmc_RwLock );
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
					RwUnlock( &pRasMailCtrl->rmc_RwLock );
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

			case RAS_MAIL_CTRL:
				{RasMailCtrlDumpRpl_t RplCtrl;
   				SESSION_MSGINIT( &RplCtrl, RAS_MAIL_CTRL, 
										0, 0, sizeof(RplCtrl) );
				RplCtrl.c_pCtrl = &RasMailCtrl;
				SendStream( ad, (char*)&RplCtrl, sizeof(RplCtrl) );
				}
				break;


			case RAS_MAIL_ADD:
				RwLockW( &pRasMailCtrl->rmc_RwLock );

				pReqAdd	= (MailAddReq_t*)pM;

				pMailAddr = (MailAddr_t*)HashListGet( &RasMailCtrl.rmc_Mail, 
											pReqAdd->a_MailAddr );
				if( pMailAddr ) {
					errno = EEXIST;
				} else {
					pMailAddr	= (MailAddr_t*)Malloc( sizeof(*pMailAddr) );
					memset( pMailAddr, 0, sizeof(*pMailAddr) );

					list_init( &pMailAddr->m_Lnk );
					strncpy( pMailAddr->m_MailAddr, pReqAdd->a_MailAddr,
							sizeof( pMailAddr->m_MailAddr ) );

					HashListPut( &RasMailCtrl.rmc_Mail, pMailAddr->m_MailAddr,
									pMailAddr, m_Lnk );
				}
				SESSION_MSGINIT( &RplAdd, RAS_MAIL_ADD, 0, errno, 
										sizeof(RplAdd) );
				SendStream( ad, (char*)&RplAdd, sizeof(RplAdd) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
				break;

			case RAS_MAIL_DEL:
				RwLockW( &pRasMailCtrl->rmc_RwLock );

				pReqAdd	= (MailAddReq_t*)pM;

				pMailAddr = (MailAddr_t*)HashListGet( &RasMailCtrl.rmc_Mail, 
											pReqAdd->a_MailAddr  );
				if( !pMailAddr ) {
					errno = ENOENT;
				} else {
					HashListDel( &RasMailCtrl.rmc_Mail, pMailAddr->m_MailAddr,
										MailAddr_t*, m_Lnk );
					Free( pMailAddr );
				}
				SESSION_MSGINIT( &RplAdd, RAS_MAIL_DEL, 0, errno, 
									sizeof(RplAdd) );
				SendStream( ad, (char*)&RplAdd, sizeof(RplAdd) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
				break;

			case RAS_MAIL_SEND:
				RwLockW( &pRasMailCtrl->rmc_RwLock );

				pReqMail	= (RasMailSendReq_t*)pM;

				if( pReqMail->m_File[0] == 0 ) {
					Ret = MailSend( pReqMail->m_Body, NULL,
												pReqMail->m_Cluster );
				} else {
					Ret = MailSend( pReqMail->m_Body, pReqMail->m_File,
												pReqMail->m_Cluster );
				}

				SESSION_MSGINIT( &RplMail, RAS_MAIL_SEND, 0, errno, 
								sizeof(RplMail) );

				SendStream( ad, (char*)&RplMail, sizeof(RplMail) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
				break;

			case RAS_LOG:
				RwLockW( &pRasMailCtrl->rmc_RwLock );

				LogDump( pLog );

				SESSION_MSGINIT( &Rpl, RAS_LOG, 
									0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
				break;

			case RAS_STOP:
				RwLockW( &pRasMailCtrl->rmc_RwLock );

				errno = 0;

				/* Mail */
				list_for_each_entry_safe( pMailAddr, pMAW,
						&RasMailCtrl.rmc_Mail.hl_Lnk, m_Lnk ) {

					HashListDel( &RasMailCtrl.rmc_Mail, pMailAddr->m_MailAddr,
										MailAddr_t*, m_Lnk );
					Free( pMailAddr );
				}
				LogDump( pLog );

				SESSION_MSGINIT( &Rpl, RAS_STOP, 0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
				exit( 0 );

			case RAS_MAIL_ACTIVE:
				RwLockR( &pRasMailCtrl->rmc_RwLock );

				SESSION_MSGINIT( &Rpl, RAS_MAIL_ACTIVE, 
									0, errno, sizeof(Rpl) );
				SendStream( ad, (char*)&Rpl, sizeof(Rpl) );

				RwUnlock( &pRasMailCtrl->rmc_RwLock );
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


int
RasMailLock( int Fd )
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
RasMailUnlock( int Fd )
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

RasMailCtrl_t	Ctrl, *pCtrl;

int
_RasMailCtrlDump( int Fd )
{
	int	Ret;

	RasMailCtrlDumpReq_t	Req;
	RasMailCtrlDumpRpl_t	Rpl;

	SESSION_MSGINIT( &Req, RAS_MAIL_CTRL, 0, 0, sizeof(Req) );

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
MailDump( int Fd )
{
	int	Ret;
	MailAddr_t	*pMail, Mail;
	int	i = 0;

	RasMailLock( Fd );

	Ret = _RasMailCtrlDump( Fd );
	if( Ret < 0 ) goto err;

	LIST_FOR_EACH_ENTRY( pMail, &Ctrl.rmc_Mail.hl_Lnk, m_Lnk,
							&Ctrl, pCtrl ) {
		Ret = _RasDump( Fd, (char*)pMail, (char*)&Mail, sizeof(Mail) );
		if( Ret < 0 )	goto err;

		printf("%d [%s]\n", i++, Mail.m_MailAddr );

		pMail	= &Mail;
	} 
	RasMailUnlock( Fd );
	return( 0 );
err:
	RasMailUnlock( Fd );
	return( -1 );
}

int
MailAdd( int Fd, char *pMailAddr )
{
	MailAddReq_t	Req;
	MailAddRpl_t	Rpl;
	int	Ret;

	SESSION_MSGINIT( &Req, RAS_MAIL_ADD, 0, 0, sizeof(Req) );

	strncpy( Req.a_MailAddr, pMailAddr, sizeof(Req.a_MailAddr) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.a_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
MailDel( int Fd, char *pMailAddr )
{
	MailAddReq_t	Req;
	MailAddRpl_t	Rpl;
	int	Ret;


	SESSION_MSGINIT( &Req, RAS_MAIL_DEL, 0, 0, sizeof(Req) );

	strncpy( Req.a_MailAddr, pMailAddr, sizeof(Req.a_MailAddr) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.a_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

int
RasMailStop( int Fd )
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
RasMailLog( int Fd )
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
RasMailActive( int Fd )
{
	RasMailActiveReq_t	Req;
	RasMailActiveRpl_t	Rpl;
	int	Ret;
	struct timeval	Timeout = { 0, 500000};

	SESSION_MSGINIT( &Req, RAS_MAIL_ACTIVE, 0, 0, sizeof(Req) );

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

int
RasMailSend( int Fd, char *pCluster, char *pBody, char *pFile )
{
	RasMailSendReq_t	Req;
	RasMailSendRpl_t	Rpl;
	int	Ret;


	SESSION_MSGINIT( &Req, RAS_MAIL_SEND, 0, 0, sizeof(Req) );
	strncpy( Req.m_Cluster, pCluster, sizeof(Req.m_Cluster) );
	strncpy( Req.m_Body, pBody, sizeof(Req.m_Body) );
	if( pFile )	strncpy( Req.m_File, pFile, sizeof(Req.m_File) );

	Ret = SendStream( Fd, (char*)&Req, sizeof(Req) );
	if( Ret < 0 ) {
		goto err;
	}
	Ret = RecvStream( Fd, (char*)&Rpl, sizeof(Rpl) );
	if( Ret < 0 ) {
		goto err;
	}
	errno	= Rpl.m_head.h_Error;
	if( errno )	goto err;

	return( 0 );
err:
	perror("");
	return( -1 );
}

void
usage()
{
	printf("RasMail cluster id		... daemon\n");
	printf("RasMail cluster id {add|del} MailAddress\n");
	printf("RasMail cluster id dump\n");
	printf("RasMail cluster id stop\n");
	printf("RasMail cluster id active\n");
	printf("RasMail cluster id event body [file]\n");
}

int
main( int ac, char **av )
{
	int	Fd = 0;
	int	Ret;

	if( ac < 3 ) {
		usage();
		exit( -1 );
	}
#if 0
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
#endif

	strncpy( RasMailCtrl.rmc_Cluster, av[1], sizeof(RasMailCtrl.rmc_Cluster) );
	strncpy( RasMailCtrl.rmc_Id, av[2], sizeof(RasMailCtrl.rmc_Id) );

	/* Log */
	if( getenv("LOG_SIZE") ) {

		int	log_level	= LogINF;
		int	log_size	= 0;

		log_size	= atoi( getenv("LOG_SIZE") );
		if( getenv("LOG_LEVEL") ) log_level	= atoi( getenv("LOG_LEVEL") );
		pLog = LogCreate( log_size, Malloc, Free, LOG_FLAG_BINARY, 
							stderr, log_level );
	}

	/* Admin Port  */
	RasMailCtrl.rmc_AdmAddr.sun_family	= AF_UNIX;
	sprintf( RasMailCtrl.rmc_AdmAddr.sun_path, PAXOS_RAS_MAIL_ADM_FMT,
				RasMailCtrl.rmc_Cluster, RasMailCtrl.rmc_Id );

	LOG( pLog, LogINF, "AdmPath[%s]", RasMailCtrl.rmc_AdmAddr.sun_path );

	if( ac == 3 ) {
		RwLockInit( &RasMailCtrl.rmc_RwLock );

		HashListInit( &RasMailCtrl.rmc_Mail, PRIMARY_101, 
				HASH_CODE_STR, HASH_CMP_STR, NULL, Malloc, Free, NULL );

	/* Daemon */
		ThreadAdm( &RasMailCtrl );
	} else {
		if( (Fd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 ) {
			goto err;
		}
		if( (Ret = connect( Fd, (struct sockaddr*) &RasMailCtrl.rmc_AdmAddr,
								sizeof(RasMailCtrl.rmc_AdmAddr) ) ) < 0 ) {
			goto err;
		}
		if( !strcmp( "add", av[3] ) ) {
			if( ac < 5 ) {
				usage();
				goto err;
			}
			Ret = MailAdd( Fd, av[4] );
			if( Ret < 0 )	goto err;
		} else if( !strcmp( "del", av[3] ) ) {
			if( ac < 5 ) {
				usage();
				goto err;
			}
			Ret = MailDel( Fd, av[4] );
			if( Ret < 0 )	goto err;
		} else if( !strcmp( "dump", av[3] ) ) {
			if( ac < 4 ) {
				usage();
				goto err;
			}
			Ret = MailDump( Fd );
			if( Ret < 0 )	goto err;
		} else if( !strcmp( "stop", av[3] ) ) {
			Ret = RasMailStop( Fd );
		} else if( !strcmp( "log", av[3] ) ) {
			Ret = RasMailLog( Fd );
		} else if( !strcmp( "active", av[3] ) ) {
			Ret = RasMailActive( Fd );
		} else if( !strcmp( "event", av[3] ) ) {
			char *pFile = NULL;
			if( ac < 5 ) {
				usage();
				goto err;
			}
			if( ac >= 6 )		pFile = av[5];
			Ret = RasMailSend( Fd, av[1], av[4], pFile );
			if( Ret < 0 )	goto err;
		}
err:
		if( Fd > 0 )	close( Fd );
	}

	return( Ret );
}
