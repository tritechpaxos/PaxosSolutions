/******************************************************************************
*
*  admin.c 	--- probe program of Paxos Library
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

#define	_PAXOS_SERVER_
#include	"Paxos.h"
#include	<netdb.h>
#ifdef KKK
#else
#include	<poll.h>
#endif

int	FdIn, FdOut;

int DumpPaxos( void *remote_addr, int l, void* my_addr  );

void
print_server( Server_t* ps )
{
	char	alive[26], you[26], token[26];
	struct tm *pTm;

printf("%d:[%s:%d] 0x%x Leader[%d] Epoch[%d] Birth[%u] Load[%d]\n" 
		"  NextDecide[%u] BackPage[%u] Auto[%ld]\n",
		ps->s_ID, 
		inet_ntoa( ps->s_ChSend.c_PeerAddr.sin_addr), 
		ntohs( ps->s_ChSend.c_PeerAddr.sin_port ), 
		ps->s_status, 
		ps->s_leader, ps->s_epoch, (uint32_t)ps->s_Election.tv_sec, ps->s_Load,
		ps->s_NextDecide, ps->s_BackPage, ps->s_AutonomicLast );

		pTm = localtime(&ps->s_Alive.tv_sec);	
		strftime( alive, sizeof(alive), "%H %M %S", pTm );
		pTm = localtime(&ps->s_YourTime.tv_sec);	
		strftime( you, sizeof(you), "%H %M %S", pTm );
		pTm = localtime(&ps->s_Token.tv_sec);	
		strftime( token, sizeof(token), "%H %M %S", pTm );
printf( "  Alive[%s] You[%s] Token[%s]\n", alive, you, token );
}

void
print_admin( Paxos_t* pPaxos )
{
	int	i;
	int	l;
	Server_t	*pServer;

printf("MallocCount=%d\nMyId[%d] IamMaster[%d] MyRnd[%d] epoch[%d] \n \
Elected[%d] Reader[%d] Vote[%d] Birth[%u]  Suppress[%d]\nNextSeq[%u]\n", 
			pPaxos->p_MallocCount,
			pPaxos->p_MyId,
			IamMaster,
			isMyRnd,
			Epoch,
			Elected,
			EReader,
			ELeader,
			(uint32_t)EBirth.tv_sec,
			pPaxos->p_bSuppressAccept,
			pPaxos->p_NextDecide );
	l = sizeof(Server_t)*pPaxos->p_Maximum;
	pServer	= (Server_t*)Malloc(l );
	DumpPaxos( pPaxos->p_aServer, l, pServer );
	for( i = 0; i < pPaxos->p_Maximum; i++ ) {
		print_server( &pServer[i] );
	}
}

#define	V( v )	((v).v_opaque)

void
print_multi( PaxosMulti_t *pM )
{
	int	k;

	printf("=== Page %u ===\n", pM->m_PageHead.p_Page );
	for( k = 0; k < PAXOS_MAX; k++ ) {
		printf("%d: %d ", k, TO_SEQ( pM->m_PageHead.p_Page, k ) );
		printf(
"Mode[%d-%d] Init[0x%x %d] Rnd[0x%x %d %d] Ballot[%d %u] Acc[0x%x] Dead[%u]\n",
			pM->m_aPaxos[k].p_Mode,	pM->m_aPaxos[k].p_Suppressed,
			pM->m_aPaxos[k].p_InitValue.v_Flags,
			pM->m_aPaxos[k].p_InitValue.v_Server,
			pM->m_aPaxos[k].p_RndValue.v_Flags,
			pM->m_aPaxos[k].p_RndValue.v_Server,
			pM->m_aPaxos[k].p_RndValue.v_From,
			pM->m_aPaxos[k].p_RndBallot.r_id,
			pM->m_aPaxos[k].p_RndBallot.r_seq,
			pM->m_aPaxos[k].p_RndAccQuo.q_member,
			(uint32_t)pM->m_aPaxos[k].p_Deadline.tv_sec );
	}
}

void
print_paxos( Paxos_t *pPaxos, uint32_t FirstPage, uint32_t LastPage )
{
	printf( "IamMaster[%d] MyRnd[%d] Mode[%d] Cur[%d %u] Inf[0x%x]\
\nPage[%u(%u-%u)%u] NextK[%d] NextSeq[%u]\
\nSuccessCount[%d] Now[%u]\n",
		IamMaster,
		isMyRnd,
		AMode, 
		CurRnd.r_id, CurRnd.r_seq,
		RndInfQuo.q_member,
		Page, LastPage, FirstPage, BackPage,
		NextK,
		NextDecide,
		SuccessCount,
		(uint32_t)time(0) );
}

void
print_gap( int index, Gap_t *pGap )
{
	printf("GAP: %d Seq[%u]\n", index, pGap->g_Seq );
}

int
GetpPaxos( Paxos_t** ppPaxos )
{
	msg_control_t	Control;
	int	err;

	msg_init( &Control.c_head, -1, MSG_CONTROL_START, sizeof(Control) );

	err = WriteStream( FdIn, (void*)&Control, sizeof(Control) );
	if( err < 0 )	goto ret;

	err = ReadStream( FdOut, (char*)ppPaxos, sizeof(*ppPaxos) );
	if( err < 0 )	goto ret;
ret:
	return( err );
}

int
LogPaxos( void )
{
	msg_t	M;
	int	err;

	msg_init( &M, -1, MSG_LOG_DUMP, sizeof(M) );
	err = WriteStream( FdIn, (void*)&M, sizeof(M) );
	return( err );
}

int
LockPaxos()
{
	msg_t	M;
	int	err;

	msg_init( &M, -1, MSG_LOCK, sizeof(M) );
	err = WriteStream( FdIn,(void*)&M, sizeof(M) );
	return( err );
}

int
UnlockPaxos( void )
{
	msg_t	M;
	int	err;

	msg_init( &M, -1, MSG_UNLOCK, sizeof(M) );
	err = WriteStream( FdIn, (void*)&M, sizeof(M) );
	return( err );
}

int
DumpPaxos( void *remote_addr, int l, void* my_addr  )
{
	msg_dump_t	Dump;
	int	err;

	msg_init( &Dump.d_head, -1, MSG_DUMP, sizeof(Dump) );
	Dump.d_addr	= remote_addr;
	Dump.d_len	= l;
	err = WriteStream( FdIn, (void*)&Dump, sizeof(Dump) );
	if( err < 0 )	goto ret;

	err = ReadStream( FdOut, my_addr, l );
	if( err != l ) {
		err = -1;
		goto ret;
	}
ret:
	return( err );
}

int
IsActive(void)
{
	msg_t	M;
	int	err;
	msg_active_rpl_t	Rpl;

	msg_init( &M, -1, MSG_IS_ACTIVE, sizeof(M) );
	err = WriteStream( FdIn, (void*)&M, sizeof(M) );
	if( err < 0 ) {
		goto err1;
	}
#ifdef	ZZZ
	// Check Read
	struct pollfd fds[1];
	nfds_t nfds = 1;
	fds[0].fd = FdIn;
	fds[0].events = POLLIN;
	int Ret = poll( fds, nfds, 500);
	if ( Ret != 1 || (fds[0].revents & POLLIN) == 0 ) {
		err = -1;
		goto err1;
	}
#else
	struct timeval	Timeout = { 0, 500000};
	err = FdEventWait( FdIn, &Timeout );
	if( err < 0 ) {
		goto err1;
	}
#endif
	err = ReadStream( FdOut, (char*)&Rpl, sizeof(Rpl) );
	if( err < 0 ) {
		goto err1;
	}
	return( 0 );
err1:
	return( err );
}

#ifdef	ZZZ
#else
int
IsLeader( int Id )
{
	msg_t	M;
	int	err;
	msg_active_rpl_t	Rpl;
	struct timeval	Timeout = { 0, 500000};

	msg_init( &M, -1, MSG_IS_ACTIVE, sizeof(M) );
	err = WriteStream( FdIn, (void*)&M, sizeof(M) );
	if( err < 0 ) {
		goto err1;
	}
	// Check Read
	err = FdEventWait( FdIn, &Timeout );
	if( err < 0 ) {
		goto err1;
	}
	err = ReadStream( FdOut, (char*)&Rpl, sizeof(Rpl) );
	if( err < 0 ) {
		goto err1;
	}
	if( Id != Rpl.a_Active.a_Leader ) {
		goto err1;
	}
	return( 0 );
err1:
	return( err );
}

#endif

int
main( int ac, char* av[] )
{
	int	id;
	int	err;
	Paxos_t	Paxos, *pPaxos;
	int	l;
	msg_t*	pm;
	PaxosMulti_t	Mul, *pMul;
	uint32_t	FirstPage = 0, LastPage = 0;
	bool_t	Found;

	if( ac < 4 ) {
usage:
		printf("PaxosAdmin cell id log\n");
		printf("PaxosAdmin cell id control\n");
		printf("PaxosAdmin cell id paxos [page]\n");
		printf("PaxosAdmin cell id {stop|restart} to\n");
		printf("PaxosAdmin cell id down\n");
		printf("PaxosAdmin cell id command string no_k\n");
		printf("PaxosAdmin cell id newround\n");
		printf("PaxosAdmin cell id begin no_k\n");
		printf("PaxosAdmin cell id recvstop msg {0|1}\n");
		printf("PaxosAdmin cell id birth\n");
		printf("PaxosAdmin cell id suppress DT(sec)\n");
		printf("PaxosAdmin cell id catchupstop {0|1}\n");
		printf("PaxosAdmin cell id removepage\n");
		printf("PaxosAdmin cell id ras down_id\n");
		printf("PaxosAdmin cell id is_active\n");
#ifdef	ZZZ
#else
		printf("PaxosAdmin cell id is_leader\n");
#endif
		exit( -1 );
	}

	id	= atoi( av[2] );
/*
 *	Open port and bind it to channel
 */
	char	PATH[PAXOS_CELL_NAME_MAX];
	struct sockaddr_un	AdmAddr;

	memset( &AdmAddr, 0, sizeof(AdmAddr) );
	sprintf( PATH, PAXOS_ADMIN_PORT_FMT, av[1], id );
	AdmAddr.sun_family	= AF_UNIX;
	strcpy( AdmAddr.sun_path, PATH );
	
	FdOut = FdIn  = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( FdOut < 0 ) {
		perror("socket");
		exit( -1 );
	}
	if( connect( FdIn, (struct sockaddr*)&AdmAddr, sizeof(AdmAddr) ) < 0 ) {
		perror("connect");
		exit( -1 );
	}

	if( !strcmp( av[3], "log" ) ) {

		err	= LogPaxos();
		if( err < 0 )	goto err1;

		return( 0 );

	} else if( !strcmp( av[3], "control" ) ) {

		UnlockPaxos();

		err = GetpPaxos( &pPaxos );
		if( err < 0 )	goto err1;

		err	= LockPaxos();
		if( err < 0 )	goto err1;

		err = DumpPaxos( pPaxos, sizeof(Paxos), &Paxos );
		if( err < 0 ) goto ret1;
ret1:
		err	= UnlockPaxos();
		if( err < 0 )	goto err1;

		print_admin( &Paxos );

		return( 0 );

	} else if( !strcmp( av[3], "paxos" ) ) {

		UnlockPaxos();

		err = GetpPaxos( &pPaxos );
		if( err < 0 )	goto err1;

		err	= LockPaxos();
		if( err < 0 )	goto err1;

		err = DumpPaxos( pPaxos, sizeof(Paxos), &Paxos );
		if( err < 0 ) goto ret2;

		pMul	= LIST_FIRST_ENTRY( &Paxos.p_MultiLnk ,PaxosMulti_t, m_Lnk,
									&Paxos, pPaxos );
		if( pMul ) {
			err = DumpPaxos( pMul, sizeof(Mul), &Mul );
			if( err < 0 ) goto ret2;
			FirstPage	= Mul.m_PageHead.p_Page;
		}
		pMul	= LIST_LAST_ENTRY( &Paxos.p_MultiLnk, PaxosMulti_t, m_Lnk,
									&Paxos, pPaxos );
		if( pMul ) {
			err = DumpPaxos( pMul, sizeof(Mul), &Mul );
			if( err < 0 ) goto ret2;
			LastPage	= Mul.m_PageHead.p_Page;
		}
		if( ac >= 5 ) {
#ifdef	ZZZ
			int	page = atoi( av[5] );
#else
			int	page = atoi( av[4] );
#endif

			LIST_FOR_EACH_ENTRY( pMul, &Paxos.p_MultiLnk, m_Lnk,
													&Paxos, pPaxos ) {
				err = DumpPaxos( pMul, sizeof(Mul), &Mul );
				if( err < 0 ) goto ret2;
				if( Mul.m_PageHead.p_Page == page ) {
					Found	= TRUE;
					goto skip2;
				}
				pMul = &Mul;
			}
			Found	= FALSE;
skip2:;
		} else {
			Mul	= Paxos.p_Multi;
			Found	= TRUE;
		}
ret2:
		print_paxos( &Paxos, FirstPage, LastPage );
		if( Found )	print_multi( &Mul );

		HashItem_t	*pHashItem, HashItem;
		int	i;
		list_t	*aHeads;
		Gap_t	Gap;

		aHeads	= (list_t*)Malloc( sizeof(list_t)*Paxos.p_Gaps.h_N );
		err = DumpPaxos( Paxos.p_Gaps.h_aHeads, sizeof(list_t)*Paxos.p_Gaps.h_N, aHeads );

		for( i = 0; i < Paxos.p_Gaps.h_N; i++ ) {
			LIST_FOR_EACH_ENTRY( pHashItem, &aHeads[i], i_Link, 
									aHeads, Paxos.p_Gaps.h_aHeads ) {
				err = DumpPaxos( pHashItem, sizeof(HashItem), &HashItem );
				if( err < 0 ) goto ret2;

				err = DumpPaxos( HashItem.i_pValue, sizeof(Gap), &Gap );
				if( err < 0 ) goto ret2;
				print_gap( i, &Gap );

				pHashItem	= &HashItem;
			}
		}

		err	= UnlockPaxos();
		if( err < 0 )	goto err1;

		return( 0 );
	} else if( !strcmp( av[3], "is_active" ) ) {
		err = IsActive();
		return( err );
#ifdef	ZZZ
#else
	} else if( !strcmp( av[3], "is_leader" ) ) {
		err = IsLeader( id );
		return( err );
#endif

	} else if( !strcmp( av[3], "stop" ) ) {
		if(ac<6) goto usage;
		l	= sizeof(msg_outofband_stop_t);
		pm	= msg_alloc(NULL, id, MSG_OUTBOUND_STOP, l );
		((msg_outofband_stop_t*)pm)->s_server	= atoi(av[5]);
	} else if( !strcmp( av[3], "restart" ) ) {
		if(ac<6) goto usage;
		l	= sizeof(msg_outofband_restart_t);
		pm	= msg_alloc(NULL, id, MSG_OUTBOUND_RESTART, l );
		((msg_outofband_restart_t*)pm)->r_server	= atoi(av[5]);
	} else if( !strcmp( av[3], "down" ) ) {
		l	= sizeof(msg_down_t);
		pm	= msg_alloc(NULL, id, MSG_DOWN, l );
	} else if( !strcmp( av[3], "birth" ) ) {
		l	= sizeof(msg_t);
		pm	= msg_alloc(NULL, id, MSG_BIRTH, l );
	} else if( !strcmp( av[3], "command" ) ) {
		if(ac<6) goto usage;
		l	= sizeof(msg_test_command_t) + 128;
		pm	= msg_alloc(NULL, id, MSG_TEST_COMMAND, l );
		((msg_test_command_t*)pm)->c_cmd.p_Len	= sizeof(PaxosHead_t)+128;
		strcpy( (char*)(((msg_test_command_t*)pm)+1), av[5] );
		((msg_test_command_t*)pm)->c_k	= atoi(av[6]);

	} else if( !strcmp( av[3], "newround" ) ) {
		l	= sizeof(msg_test_newround_t);
		pm	= msg_alloc(NULL, id, MSG_TEST_NEWROUND, l );
	} else if( !strcmp( av[3], "begin" ) ) {
		if(ac<6) goto usage;
		l	= sizeof(msg_test_begincast_t);
		pm	= msg_alloc(NULL, id, MSG_TEST_BEGINCAST, l );
		((msg_test_begincast_t*)pm)->b_k	= atoi(av[5]);
	} else if( !strcmp( av[3], "recvstop" ) ) {
		if(ac<6) goto usage;
		l	= sizeof(msg_recv_stop_t);
		pm	= msg_alloc(NULL, id, MSG_RECV_STOP, l );
		((msg_recv_stop_t*)pm)->s_Cmd	= atoi(av[4]);
		((msg_recv_stop_t*)pm)->s_OnOff	= atoi(av[5]);
	} else if( !strcmp( av[3], "suppress" ) ) {
		if(ac<4) goto usage;
		l	= sizeof(msg_catchup_req_t);
		pm	= msg_alloc(NULL, id, MSG_TEST_SUPPRESS, l );
#ifdef	ZZZ
		((msg_catchup_req_t*)pm)->c_RecvLastDT	= atoi(av[5]);
#else
		((msg_catchup_req_t*)pm)->c_RecvLastDT	= atoi(av[4]);
#endif
	} else if( !strcmp( av[3], "catchupstop" ) ) {
		if(ac<4) goto usage;
		l	= sizeof(msg_catchup_t);
		pm	= msg_alloc(NULL, id, MSG_TEST_CATCHUP, l );
#ifdef	ZZZ
		((msg_catchup_t*)pm)->c_OnOff	= atoi(av[5]);
#else
		((msg_catchup_t*)pm)->c_OnOff	= atoi(av[4]);
#endif
	} else if( !strcmp( av[3], "removepage" ) ) {
		l	= sizeof(msg_catchup_t);
		pm	= msg_alloc(NULL, id, MSG_TEST_REMOVE_PAGE, l );
	} else if( !strcmp( av[3], "ras" ) ) {
		if(ac<4) goto usage;
		l	= sizeof(msg_ras_t);
		pm	= msg_alloc(NULL, id, MSG_RAS, l );
		pm->m_fr = -1;
	} else {
		printf("No command\n");
		goto err1;
	}
	err = WriteStream( FdIn, (void*)pm, l );
	if( err < 0 ) {
			printf("Can't WriteStream\n");
			goto err1;
	}

	return( 0 );
err1:
	return( err );
}
