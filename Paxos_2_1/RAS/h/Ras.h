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

#ifndef	_RAS_H_
#define	_RAS_H_

#include	"PFS.h"


/* Version Diff Time */
#define	DEFAULT_VER_DIFF_SEC	10

/* Admin Port */
#define	PAXOS_RAS_MAIL_ADM_FMT		"/tmp/PAXOS_RAS_MAIL_ADM_%s_%s"
#define	PAXOS_RAS_EYE_ADM_FMT		"/tmp/PAXOS_RAS_EYE_ADM_%s_%s"

/* Myself*/
#define	PAXOS_RAS_EYE_MY_RAS_FMT	"RAS/RasEye/%s"

typedef	struct RasEyeSession {
	list_t			s_Lnk;
	list_t			s_LnkReg;
	int				s_Refer;
	char			s_CellName[PAXOS_CELL_NAME_MAX];
	struct Session	*s_pSession;
	struct Session	*s_pEvent;
	pthread_t		s_ThreadEvent;
} RasEyeSession_t;

typedef	struct RasKey {
	char	rk_CellName[PAXOS_CELL_NAME_MAX];
	char	rk_EventDir[PATH_NAME_MAX];
} RasKey_t;

typedef	struct Ras {
	list_t				r_Lnk;
	RasKey_t			r_RasKey;
	RasEyeSession_t		*r_pSession;
	char				r_Mail[FILE_NAME_MAX];
	char				r_Autonomic[FILE_NAME_MAX];
} Ras_t;

typedef	struct MailAddr {
	list_t				m_Lnk;
	char				m_MailAddr[PATH_NAME_MAX];
} MailAddr_t;

typedef struct RasVersion {
	time_t			v_Time;
	unsigned int	v_Seq;
} RasVersion_t;

typedef struct RasEyeCtrl {
	RwLock_t			rmc_RwLock;
	char				rmc_Cluster[FILE_NAME_MAX];
	char				rmc_Id[FILE_NAME_MAX];
	pthread_t			rmc_ThreadAdm;
	struct sockaddr_un	rmc_AdmAddr;
	HashList_t			rmc_Session;
	HashList_t			rmc_RAS;
	HashList_t			rmc_Regist;
	int					rmc_VerDiffSec;
} RasEyeCtrl_t;

typedef struct RasMailCtrl {
	RwLock_t			rmc_RwLock;
	char				rmc_Id[FILE_NAME_MAX];
	char				rmc_Cluster[FILE_NAME_MAX];
	pthread_t			rmc_ThreadAdm;
	struct sockaddr_un	rmc_AdmAddr;
	HashList_t			rmc_Mail;
} RasMailCtrl_t;

/*
 *	Protocol
 */
#define	RAS_FAMILY		(8<<16)

#define	RAS_DUMP			(RAS_FAMILY|1)
#define	RAS_STOP			(RAS_FAMILY|2)
#define	RAS_LOG				(RAS_FAMILY|3)

#define	RAS_LOCK			(RAS_FAMILY|4)
#define	RAS_UNLOCK			(RAS_FAMILY|5)

#define	RAS_EYE_CTRL		(RAS_FAMILY|11)

#define	RAS_EYE_SET			(RAS_FAMILY|12)
#define	RAS_EYE_UNSET		(RAS_FAMILY|13)
#define	RAS_EYE_UNSET_ALL	(RAS_FAMILY|14)

#define	RAS_EYE_REGIST		(RAS_FAMILY|15)
#define	RAS_EYE_UNREGIST	(RAS_FAMILY|16)

#define	RAS_EYE_ACTIVE		(RAS_FAMILY|17)

#define	RAS_MAIL_CTRL		(RAS_FAMILY|21)

#define	RAS_MAIL_SEND		(RAS_FAMILY|22)
#define	RAS_MAIL_ADD		(RAS_FAMILY|23)
#define	RAS_MAIL_DEL		(RAS_FAMILY|24)

#define	RAS_MAIL_ACTIVE		(RAS_FAMILY|25)

#define	RasStopReq_t	PaxosSessionHead_t
#define	RasStopRpl_t	PaxosSessionHead_t

#define	RasLogReq_t		PaxosSessionHead_t
#define	RasLogRpl_t		PaxosSessionHead_t

#define	RasLockReq_t	PaxosSessionHead_t
#define	RasLockRpl_t	PaxosSessionHead_t

#define	RasUnlockReq_t	PaxosSessionHead_t
#define	RasUnlockRpl_t	PaxosSessionHead_t

#define	RasEyeActiveReq_t	PaxosSessionHead_t
#define	RasEyeActiveRpl_t	PaxosSessionHead_t

#define	RasEyeUnsetAllReq_t	PaxosSessionHead_t
#define	RasEyeUnsetAllRpl_t	PaxosSessionHead_t

#define	RasMailActiveReq_t	PaxosSessionHead_t
#define	RasMailActiveRpl_t	PaxosSessionHead_t

typedef struct RasDumpReq {
	PaxosSessionHead_t	d_Head;
	void				*d_pAddr;
	int					d_Len;
} RasDumpReq_t;

typedef struct RasMailDumpRpl {
	PaxosSessionHead_t	d_Head;
	// follow data
} RasDumpRpl_t;

#define	RasMailCtrlDumpReq_t	PaxosSessionHead_t

typedef struct RasMailCtrlDumpRpl {
	PaxosSessionHead_t	c_Head;
	RasMailCtrl_t		*c_pCtrl;
} RasMailCtrlDumpRpl_t;

#define	RasEyeCtrlDumpReq_t	PaxosSessionHead_t

typedef struct RasEyeCtrlDumpRpl {
	PaxosSessionHead_t	c_Head;
	RasEyeCtrl_t		*c_pCtrl;
} RasEyeCtrlDumpRpl_t;

typedef struct RasSetReq {
	PaxosSessionHead_t	s_head;
	RasKey_t			s_RasKey;
	char				s_Mail[FILE_NAME_MAX];
	char				s_Autonomic[FILE_NAME_MAX];
} RasSetReq_t;

typedef struct RasSetRpl {
	PaxosSessionHead_t	s_head;
} RasSetRpl_t;

typedef struct MailAddReq {
	PaxosSessionHead_t	a_head;
	char				a_MailAddr[PATH_NAME_MAX];
} MailAddReq_t;

typedef struct MailAddRpl {
	PaxosSessionHead_t	a_head;
} MailAddRpl_t;

typedef struct RegistReq {
	PaxosSessionHead_t	r_head;
	char				r_CellName[PAXOS_CELL_NAME_MAX];
} RegistReq_t;

typedef struct RegistRpl {
	PaxosSessionHead_t	r_head;
} RegistRpl_t;

typedef struct RasMailSendReq {
	PaxosSessionHead_t	m_head;
	char				m_Cluster[256];
	char				m_Body[1024];
	char				m_File[FILE_NAME_MAX];
} RasMailSendReq_t;

typedef struct RasMailSendRpl {
	PaxosSessionHead_t	m_head;
} RasMailSendRpl_t;

#endif	//	_RAS_H_
