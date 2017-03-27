/******************************************************************************
*
*  CIFS.h 	--- CIFS protocol
*
*  Copyright (C) 2016 triTech Inc. All Rights Reserved.
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

/*
 *	çÏê¨			ìnï”ìTçF
 *	ééå±
 *	ì¡ãñÅAíòçÏå†	ÉgÉâÉCÉeÉbÉN
 */

#ifndef	_CIFS_H_
#define	_CIFS_H_

//#define	_DEBUG_

#include	"CNV.h"
#include	<wchar.h>
#include	<locale.h>

struct IdMap;

extern	struct Log	*pLog;

/*
 *	MS-DTYP
 */
#ifndef	GUID
typedef struct {
	uint32_t	Data1;
	uint16_t	Data2, Data3;
	uint8_t		Data4[8];
} GUID, UUID, *PGUID;
#endif

typedef struct _FILETIME {
	uint32_t	dwLowDateTime;
	uint32_t	dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct OEM_String {	// uchar null-terminated
	uint8_t	NullTerminated[0];
} OEM_String_t;

typedef struct SMB_String {	// uchar/wchar null-terminated
	union {
		uint8_t	U8[0];
		wchar_t	U16[0];
	} NullTerminated;
} SMB_String_t;

typedef struct SMB_Dialect {
	uint8_t			BufferFormat;
	OEM_String_t	DialectString;
} SMB_Dialect_t;

/*
 * Snoop mode
 */
#define	SNOOP_OFF		0
#define	SNOOP_META_ON	1
#define	SNOOP_META_OFF	2
/*
 *	CIFS protocol(MS-CIFS)
 */
// SMB packet part in Vec
#define	SMB_MSG_DATA_LEN	0
#define	SMB_MSG_HEAD		1
#define	SMB_MSG_PARAM		2
#define	SMB_MSG_DATA		3

// Command
#define	SMB_COM_CLOSE					0x04

#define	SMB_COM_TRANSACTION				0x25
#define	SMB_COM_TRANSACTION_SECONDARY	0x26

#define	SMB_COM_ECHO					0x2b
#define	SMB_COM_READ_ANDX				0x2e
#define	SMB_COM_WRITE_ANDX				0x2f

#define	SMB_COM_TRANSACTION2			0x32
#define	SMB_COM_TRANSACTION2_SECONDARY	0x33

#define	SMB_COM_TREE_CONNECT			0x70	// obsoleted
#define	SMB_COM_TREE_DISCONNECT			0x71
#define	SMB_COM_NEGOTIATE				0x72
#define	SMB_COM_SESSION_SETUP_ANDX		0x73
#define	SMB_COM_SESSION_LOGOFF_ANDX		0x74
#define	SMB_COM_TREE_CONNECT_ANDX		0x75

#define	SMB_COM_NT_TRANSACT				0xa0
#define	SMB_COM_NT_TRANSACT_SECONDARY	0xa1

#define	SMB_COM_NT_CREATE_ANDX			0xa2
#define	SMB_COM_NT_CANCEL				0xa4


// Flags2
#define	SMB_FLAGS2_LONG_NAME				0x0001
#define	SMB_FLAGS2_EAS						0x0002
#define	SMB_FLAGS2_SMB_SECUTITY_SIGNATURE	0x0004
#define	SMB_FLAGS2_IS_LONG_NAME				0x0040
#define	SMB_FLAGS2_DFS						0x1000
#define	SMB_FLAGS2_PAGING_IO				0x2000
#define	SMB_FLAGS2_NT_STATUS				0x4000
#define	SMB_FLAGS2_UNICODE					0x8000

typedef struct DataBuffer {
	uint16_t	d_Length;
	uint8_t		d_Data[0];
} __attribute__((packed)) DataBuffer_t;

typedef struct SMB_ERROR {
	uint8_t		e_ErrorClass;
	uint8_t		e_Reserved;
	uint16_t	e_ErrorCode;
} __attribute__((packed)) SMB_ERROR_t;

typedef	struct SMB_Header {
	uint8_t		h_Protocol[4];
	uint8_t		h_Command;
/***
	SMB_ERROR_t	h_Status;
***/
	struct {
		uint8_t		e_ErrorClass;
		uint8_t		e_Reserved;
		uint16_t	e_ErrorCode;
	}	h_Status;
	uint8_t		h_Flags;
	uint16_t	h_Flags2;
	uint16_t	h_PIDHigh;
	union {
		uint8_t		s_SecuritySignature[8];
		struct {
						uint32_t	f_Key;
						uint16_t	f_CID;
						uint16_t	f_SequeunceNumber;
		} 			s_SecurityFeatures;
	}			h_Security;
#define	h_SecuritySignature	h_Security.s_SecuritySignature
#define	h_Key				h_Security.s_SecurityFeatures.f_Key
#define	h_CID				h_Security.s_SecurityFeatures.f_CID
#define	h_SequenceNumber	h_Security.s_SecurityFeatures.f_SequenceNumber

	uint16_t	h_Reserved;
	uint16_t	h_TID;
	uint16_t	h_PIDLow;
	uint16_t	h_UID;
	uint16_t	h_MID;
} __attribute__((packed)) SMB_Header_t;

typedef struct SMB_Parameters {
	uint8_t		p_WordCount;
	uint16_t	p_Words[0];
} __attribute__((packed)) SMB_Parameters_t;

typedef struct SMB_Data {
	uint16_t	d_ByteCount;
	uint8_t		d_Bytes[0];
} __attribute__((packed)) SMB_Data_t;

// NT LANMAN
#define	CAP_RAW_MODE			0x00000001
#define	CAP_MPX_MODE			0x00000002
#define	CAP_UNICODE				0x00000004
#define	CAP_LARGE_FILES			0x00000008
#define	CAP_NT_SMBS				0x00000010
#define	CAP_RPC_REMOTE_APIS		0x00000020
#define	CAP_STATUS32			0x00000040
#define	CAP_LEVEL_II_OPLOCKS	0x00000080
#define	CAP_LOCK_AND_READ		0x00000100
#define	CAP_NT_FIND				0x00000200
#define	CAP_BULK_TRANSFER		0x00000400
#define	CAP_COMPRESSED_DATA		0x00000800
#define	CAP_DFS					0x00001000
#define	CAP_QUADWORD_ALIGNED	0x00002000
#define	CAP_LARGE_READX			0x00004000

typedef struct SMB_Parameters_NT_LM_Res {
	uint16_t	p_DialectIndex;
	uint8_t		p_SecurityMode;
	uint16_t	p_MaxMpxCount;
	uint16_t	p_MaxNumberVcs;
	uint32_t	p_MaxBufferSize;
	uint32_t	p_MaxRawSize;
	uint32_t	p_SessionKey;
	uint32_t	p_Capabilities;
	FILETIME	p_SystemTime;
	uint16_t	p_ServerTimeZone;
	uint8_t		p_ChallengeLength;
} __attribute__((packed)) SMB_Parameters_NT_LM_Res_t;


typedef struct MetaHead {
	uint32_t	m_Length;	// including myself only Meta part
#define	META_NULL		0
#define	META_ID_RETURN	1
	uint32_t	m_Cmd;
} MetaHead_t;

typedef struct Meta_Return {
	MetaHead_t	r_Head;
	uint16_t	r_ReturnID;
} Meta_Return_t;

typedef struct Meta_NT_LM {
	MetaHead_t	m_Head;
	uint32_t	m_SessionKey;
} Meta_NT_LM_t;

// Setup
typedef struct SMB_Parameters_Setup_Req {
	uint8_t		p_AndXCommand;
	uint8_t		p_AndXReserved;
	uint16_t	p_AndXOffset;
	uint16_t	p_MaxBufferSize;
	uint16_t	p_MaxMpxCount;
	uint16_t	p_VcNumber;
	uint32_t	p_SessionKey;
	uint16_t	p_OEMPasswordLen;
	uint16_t	p_UnicodePasswordLen;
	uint32_t	p_Reserved;
	uint32_t	p_Capabilities;
} __attribute__((packed)) SMB_Parameters_Setup_Req_t;

typedef struct SMB_Parameters_Setup_Res {
	uint8_t		p_AndXCommand;
	uint8_t		p_AndXReserved;
	uint16_t	p_AndXOffset;
	uint16_t	p_Action;
} __attribute__((packed)) SMB_Parameters_Setup_Res_t;

typedef struct Meta_Setup {
	MetaHead_t	s_Head;
	uint16_t	s_UID;
} Meta_Setup_t;

// TREE_CONNECT_ANDX
#define		TREE_CONNECT_ANDX_DISCONNECT_TID	0x0001

typedef struct SMB_Parameters_Connect_Req {
	uint8_t		c_AndXCommand;
	uint8_t		c_AndXReserved;
	uint16_t	c_AndXOffset;
	uint16_t	c_Flags;
	uint16_t	c_PasswordLen;
} __attribute__((packed)) SMB_Parameters_Connect_Req_t;

typedef struct SMB_Parameters_Connect_Rpl {
	uint8_t		c_AndXCommand;
	uint8_t		c_AndXReserved;
	uint16_t	c_AndXOffset;
	uint16_t	c_OptionalSupport;
} __attribute__((packed)) SMB_Parameters_Connect_Rpl_t;

typedef struct Meta_Connect {
	MetaHead_t	c_Head;
	uint16_t	c_TID;
} Meta_Connect_t;

// CLOSE
typedef struct SMB_Parameters_Close_Req {
	uint16_t	c_FID;
	uint32_t	c_LastTimeModified;
} __attribute__((packed)) SMB_Parameters_Close_Req_t;

// CREATE_ANDX
typedef struct SMB_Parameters_CreateAndX_Req {
	uint8_t		c_AndXCommand;
	uint8_t		c_AndXReserved;
	uint16_t	c_AndXOffset;
	uint8_t		c_Reserved;
	uint16_t	c_NameLength;
	uint32_t	c_Flags;
	uint32_t	c_RootDirectoryFID;
	uint32_t	c_DesiredAccess;
	uint64_t	c_AllocationSize;
	uint32_t	c_ExtFileAttributes;
	uint32_t	c_ShareAccess;
	uint32_t	c_CreateDisposition;
	uint32_t	c_CreateOptions;
	uint32_t	c_ImpersonationLevel;
	uint8_t		c_SecurityFlags;
} __attribute__((packed)) SMB_Parameters_CreateAndX_Req_t;

typedef struct SMB_Parameters_CreateAndX_Res {
	uint8_t		c_AndXCommand;
	uint8_t		c_AndXReserved;
	uint16_t	c_AndXOffset;
	uint8_t		c_OpLockLevel;
	uint16_t	c_FID;
	uint32_t	c_CreateDisposition;
	FILETIME	c_CreateTime;
	FILETIME	c_LastAccessTime;
	FILETIME	c_LastWriteTime;
	FILETIME	c_LastChangeTime;
	uint32_t	c_ExtFileAttributes;
	uint64_t	c_AllocationSize;
	uint64_t	c_EndOfFile;
	uint16_t	c_ResourceType;
	uint16_t	c_NMPipeStatus;
	uint8_t		c_Directory;
} __attribute__((packed)) SMB_Parameters_CreateAndX_Res_t;

typedef struct Meta_Create {
	MetaHead_t	c_Head;
	uint16_t	c_FID;
} Meta_Create_t;

// TRANSACTION
typedef struct SMB_Parameters_Transaction_Req {
	uint16_t	t_TotalParameterCount;
	uint16_t	t_TotalDataCount;
	uint16_t	t_MaxParameterCount;
	uint16_t	t_MaxDataCount;
	uint8_t		t_MaxSetupCount;
	uint8_t		t_Reserved1;
	uint16_t	t_Flags;
	uint32_t	t_Timeout;
	uint16_t	t_Reserved2;
	uint16_t	t_ParameterCount;
	uint16_t	t_ParameterOffset;
	uint16_t	t_DataCount;
	uint16_t	t_DataOffset;
	uint8_t		t_SetupCount;
	uint8_t		t_Reserved3;
	uint16_t	t_Setup[0];
} __attribute__((packed)) SMB_Parameters_Transaction_Req_t;

typedef struct SMB_Parameters_Transaction_Res {
	uint16_t	t_TotalParameterCount;
	uint16_t	t_TotalDataCount;
	uint16_t	t_Reserved1;
	uint16_t	t_ParameterCount;
	uint16_t	t_ParameterOffset;
	uint16_t	t_ParameterDisplacement;
	uint16_t	t_DataCount;
	uint16_t	t_DataOffset;
	uint16_t	t_DataDisplacement;
	uint8_t		t_SetupCount;
	uint8_t		t_Reserved2;
	uint16_t	t_Setup[0];
} __attribute__((packed)) SMB_Parameters_Transaction_Res_t;

// TRANSACTION_SECONDARY	No response
typedef struct SMB_Parameters_Transaction_Secondary_Req {
	uint16_t	s_TotalParameterCount;
	uint16_t	s_TotalDataCount;
	uint16_t	s_ParameterCount;
	uint16_t	s_ParameterOffset;
	uint16_t	s_ParameterDisplacement;
	uint16_t	s_DataCount;
	uint16_t	s_DataOffset;
	uint16_t	s_DataDisplacement;
} __attribute__((packed)) SMB_Parameters_Transaction_Secondary_Req_t;

// TRAN subcommand( Setup command)
#define	TRANS_SET_NMPIPE_STATE		0x0001
#define	TRANS_RAW_READ_NMPIPE		0x0011
#define	TRANS_QUERY_NMPIPE_STATE	0x0021
#define	TRANS_QUERY_NMPIPE_INFO		0x0022
#define	TRANS_PEEK_NMPIPE			0x0023
#define	TRANS_TRANSACT_NMPIPE		0x0026
#define	TRANS_RAW_WRITE_NMPIPE		0x0031
#define	TRANS_READ_NMPIPE			0x0036
#define	TRANS_WRITE_NMPIPE			0x0037
#define	TRANS_WAIT_NMPIPE			0x0053
#define	TRANS_CALL_NMPIPE			0x0054
#define	TRANS_MAIL_SLOT_WRITE		0x0001

// TRANSACTION2
typedef struct SMB_Parameters_Transaction2_Req {
	uint16_t	t_TotalParameterCount;
	uint16_t	t_TotalDataCount;
	uint16_t	t_MaxParameterCount;
	uint16_t	t_MaxDataCount;
	uint8_t		t_MaxSetupCount;
	uint8_t		t_Reserved1;
	uint16_t	t_Flags;
	uint32_t	t_Timeout;
	uint16_t	t_Reserved2;
	uint16_t	t_ParameterCount;
	uint16_t	t_ParameterOffset;
	uint16_t	t_DataCount;
	uint16_t	t_DataOffset;
	uint8_t		t_SetupCount;
	uint8_t		t_Reserved3;
	uint16_t	t_Setup[0];
} __attribute__((packed)) SMB_Parameters_Transaction2_Req_t;

typedef struct SMB_Parameters_Transaction2_Res {
	uint16_t	t_TotalParameterCount;
	uint16_t	t_TotalDataCount;
	uint16_t	t_Reserved1;
	uint16_t	t_ParameterCount;
	uint16_t	t_ParameterOffset;
	uint16_t	t_ParameterDisplacement;
	uint16_t	t_DataCount;
	uint16_t	t_DataOffset;
	uint16_t	t_DataDisplacement;
	uint8_t		t_SetupCount;
	uint8_t		t_Reserved2;
	uint16_t	t_Setup[0];
} __attribute__((packed)) SMB_Parameters_Transaction2_Res_t;

// TRANSACTION2_SECONDARY	No response
typedef struct SMB_Parameters_Transaction2_Secondary_Req {
	uint16_t	s_TotalParameterCount;
	uint16_t	s_TotalDataCount;
	uint16_t	s_ParameterCount;
	uint16_t	s_ParameterOffset;
	uint16_t	s_ParameterDisplacement;
	uint16_t	s_DataCount;
	uint16_t	s_DataOffset;
	uint16_t	s_DataDisplacement;
	uint16_t	s_FID;
} __attribute__((packed)) SMB_Parameters_Transaction2_Secondary_Req_t;

// TRAN2 subcommand( Setup command)
#define	TRANS2_OPEN2					0x0000
#define	TRANS2_FIND_FIRST2				0x0001
#define	TRANS2_FIND_NEXT2				0x0002
#define	TRANS2_QUERY_FS_INFORMATION		0x0003
#define	TRANS2_SET_FS_INFORMATION		0x0004
#define	TRANS2_QUERY_PATH_INFORMATION	0x0005
#define	TRANS2_SET_PATH_INFORMATION		0x0006
#define	TRANS2_QUERY_FILE_INFORMATION	0x0007
#define	TRANS2_SET_FILE_INFORMATION		0x0008
#define	TRANS2_FSCTL					0x0009
#define	TRANS2_IOCTL2					0x000a
#define	TRANS2_FIND_NOTIFY_FIRST		0x000b
#define	TRANS2_FIND_NOTIFY_NEXT			0x000c
#define	TRANS2_CREATE_DIRECTORY			0x000d
#define	TRANS2_SESSION_SETUP			0x000e
#define	TRANS2_GET_DFS_REFERRAL			0x0010
#define	TRANS2_REPORT_DFS_INCONSISTENCY	0x0011

typedef struct SMB_Parameters_Trans2_Open_Req {
	uint16_t	o_Flags;
	uint16_t	o_AccessMode;
	uint16_t	o_Reserved1;
	uint32_t	o_FileAttributes;
	uint32_t	o_CreationTime;
	uint16_t	o_OpenMode;
	uint32_t	o_AllocationSize;
	uint16_t	o_Reserved[5];
	uint8_t		o_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Open_Req_t;

typedef struct SMB_Parameters_Trans2_Open_Res {
	uint16_t	o_FID;
	uint32_t	o_FileAttributes;
	uint32_t	o_CreationTime;
	uint32_t	o_FileDataSize;
	uint16_t	o_AccessMode;
	uint16_t	o_NMPipeStatus;
	uint16_t	o_ActionTaken;
	uint32_t	o_Reserved;
	uint16_t	o_ExtendedAttributeErrorOffset;
	uint32_t	o_ExtendedAttributeLength;
} __attribute__((packed)) SMB_Parameters_Trans2_Open_Res_t;

typedef struct SMB_Parameters_Trans2_Find_First2_Req {
	uint32_t	f_SearchAttributes;
	uint16_t	f_SearchCount;
	uint16_t	f_Flags;
	uint16_t	f_InformationLevel;
	uint32_t	f_SearchStrageType;
	uint8_t		f_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Find_First2_Req_t;

typedef struct SMB_Parameters_Trans2_Find_First2_Res {
	uint16_t	f_SID;
	uint16_t	f_SearchCount;
	uint32_t	f_SearchAttributes;
	uint16_t	f_EndOfSearch;
	uint16_t	f_EaErrorOffset;
	uint16_t	f_LastNameOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Find_First2_Res_t;

typedef struct SMB_Parameters_Trans2_Find_Next2_Req {
	uint16_t	n_SID;
	uint16_t	n_SearchCount;
	uint16_t	n_InformationLevel;
	uint32_t	n_ResumeKey;
	uint16_t	n_Flags;
	uint8_t		n_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Find_Next2_Req_t;

typedef struct SMB_Parameters_Trans2_Find_Next2_Res {
	uint16_t	n_SearchCount;
	uint16_t	n_EndOfSearch;
	uint16_t	n_EaErrorOffset;
	uint16_t	n_LastNameOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Find_Next2_Res_t;

typedef struct SMB_Parameters_Trans2_Query_FS_Information_Req {
	uint16_t	qf_InformationLevel;
} __attribute__((packed)) SMB_Parameters_Trans2_Query_FS_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Set_FS_Information_Req {
	uint16_t	sf_InformationLevel;
	uint32_t	sf_Reserved;
	uint8_t		sf_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Set_FS_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Set_FS_Information_Res {
	uint16_t	sf_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Set_FS_Information_Res_t;

typedef struct SMB_Parameters_Trans2_Query_Path_Information_Req {
	uint16_t	qp_InformationLevel;
	uint32_t	qp_Reserved;
	uint8_t		qp_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Query_Path_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Query_Path_Information_Res {
	uint16_t	qp_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Query_Path_Information_Res_t;

typedef struct SMB_Parameters_Trans2_Set_Path_Information_Req {
	uint16_t	sp_InformationLevel;
	uint32_t	sp_Reserved;
	uint8_t		sp_FileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Set_Path_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Set_path_Information_Res {
	uint16_t	sp_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Set_Path_Information_Res_t;

typedef struct SMB_Parameters_Trans2_Query_File_Information_Req {
	uint16_t	qf_FID;
	uint16_t	qf_InformationLevel;
} __attribute__((packed)) SMB_Parameters_Trans2_Query_File_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Query_File_Information_Res {
	uint16_t	qf_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Query_File_Information_Res_t;

typedef struct SMB_Parameters_Trans2_Set_File_Information_Req {
	uint16_t	sf_FID;
	uint16_t	sf_InformationLevel;
	uint16_t	sf_Reserved;
} __attribute__((packed)) SMB_Parameters_Trans2_Set_File_Information_Req_t;

typedef struct SMB_Parameters_Trans2_Set_File_Information_Res {
	uint16_t	sf_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Set_File_Information_Res_t;

typedef struct SMB_Parameters_Trans2_Create_Directory_Req {
	uint32_t	cd_Reserved;
	uint8_t		cd_DirectoryName;
} __attribute__((packed)) SMB_Parameters_Trans2_Create_Directory_Req_t;

typedef struct SMB_Parameters_Trans2_Create_Directory_Res {
	uint16_t	cd_EaErrorOffset;
} __attribute__((packed)) SMB_Parameters_Trans2_Create_Directory_Res_t;


typedef struct SMB_Parameters_Trans2_Get_DFS_Referral_Req {
	uint16_t	dr_MaxReferralLevel;
	uint16_t	dr_RequestFileName[];
} __attribute__((packed)) SMB_Parameters_Trans2_Get_DFS_Referral_Req_t;

typedef struct SMB_Data_Trans2_Get_DFS_Referral_Rpl {
	uint16_t	dr_PathConsumed;
	uint16_t	dr_NumberOfReferrals;
	uint32_t	dr_ReferralHeaderFlags;
} __attribute__((packed)) SMB_Data_Trans2_Get_DFS_Referral_Rpl_t;

typedef struct DFS_Referral {
#define	DFS_REFERRAL_V1	0x0001
#define	DFS_REFERRAL_V2	0x0002
#define	DFS_REFERRAL_V3	0x0003
#define	DFS_REFERRAL_V4	0x0004
	uint16_t	r_VersionNumber;
	uint16_t	r_Size;
	uint16_t	r_ServerType;
	uint16_t	r_ReferralHeaderFlags;
} __attribute__((packed)) DFS_Referral_t;

typedef struct DFS_Referral_V1 {
	DFS_Referral_t	v1_Head;
	uint16_t		v1_ShareName[];
} __attribute__((packed)) DFS_Referral_V1_t;

typedef struct DFS_Referral_V2 {
	DFS_Referral_t	v2_Head;
	uint32_t		v2_Proximity;
	uint32_t		v2_TimeToLive;
	uint16_t		v2_DFSPathOffset;
	uint16_t		v2_NetworkAddressOffset;
} __attribute__((packed)) DFS_Referral_V2_t;

typedef struct DFS_Referral_V34 {
	DFS_Referral_t	v34_Head;
	uint32_t		v34_TimeToLive;
} __attribute__((packed)) DFS_Referral_V34_t;

#define	NameListReferral	0x0002
typedef struct Referral_Entry_0 {
	uint16_t	e0_DFSPathOffset;
	uint16_t	e0_DFSAlternatePathOffset;
	uint16_t	e0_NetworkAddressOffset;
	uint8_t		e0_ServiceSiteGuid[16];
} __attribute__((packed)) Referral_Entry_0_t;

typedef struct Referral_Entry_1 {
	uint16_t	e1_SpecialNameOffset;
	uint16_t	e1_NumberOfExpandedNames;
	uint16_t	e1_ExpandedNameOffset;
} __attribute__((packed)) Referral_Entry_1_t;

// NT_TRANSACT
typedef struct SMB_Parameters_NT_Transact_Req {
	uint8_t		n_MaxSetupCount;
	uint16_t	n_Reserved1;
	uint32_t	n_TotalParameterCount;
	uint32_t	n_TotalDataCount;
	uint32_t	n_MaxParameterCount;
	uint32_t	n_MaxDataCount;
	uint32_t	n_ParameterCount;
	uint32_t	n_ParameterOffset;
	uint32_t	n_DataCount;
	uint32_t	n_DataOffset;
	uint8_t		n_SetupCount;
	uint16_t	n_Function;
	uint16_t	n_Setup[0];
} __attribute__((packed)) SMB_Parameters_NT_Transact_Req_t;

typedef struct SMB_Parameters_NT_Transact_Res {
	uint8_t		n_Reserved1[3];
	uint32_t	n_TotalParameterCount;
	uint32_t	n_TotalDataCount;
	uint32_t	n_ParameterCount;
	uint32_t	n_ParameterOffset;
	uint32_t	n_ParameterDisplacement;
	uint32_t	n_DataCount;
	uint32_t	n_DataOffset;
	uint32_t	n_DataDisplacement;
	uint8_t		n_SetupCount;
	uint16_t	n_Setup[0];
} __attribute__((packed)) SMB_Parameters_NT_Transact_Res_t;

// NT_TRANSACT_SECONDARY	No response
typedef struct SMB_Parameters_NT_Transact_Secondary_Req {
	uint8_t		s_Reserved1[3];
	uint32_t	s_TotalParameterCount;
	uint32_t	s_TotalDataCount;
	uint32_t	s_ParameterCount;
	uint32_t	s_ParameterOffset;
	uint32_t	s_ParameterDisplacement;
	uint32_t	s_DataCount;
	uint32_t	s_DataOffset;
	uint32_t	s_DataDisplacement;
	uint8_t		s_Reserved2;
} __attribute__((packed)) SMB_Parameters_NT_Transact_Secondary_Req_t;

// NT_TRANSACT subcommand( Setup command)
#define	NT_TRANSACT_CREATE				0x0001
#define	NT_TRANSACT_IOCTL				0x0002
#define	NT_TRANSACT_SET_SECURITY_DESC	0x0003
#define	NT_TRANSACT_NOTIFY_CHANGE		0x0004
#define	NT_TRANSACT_RENAME				0x0005
#define	NT_TRANSACT_QUERY_SECURITY_DESC	0x0006

typedef struct NT_Transact_IOCTL_Setup {
	uint32_t	i_FunctionCode;
	uint16_t	i_FID;
	uint8_t		i_IsFsctl;
	uint8_t		i_IsFlags;
} __attribute__((packed)) NT_Transact_IOCTL_Setup_t;

typedef struct NT_Transact_Notify_Change_Setup {
	uint32_t	n_CompletionFilter;
	uint16_t	n_FID;
	uint8_t		n_WatchTree;
	uint8_t		n_Reserved;
} __attribute__((packed)) NT_Transact_Notify_Change_Setup_t;

// WRITE_ANDX
typedef struct SMB_Parameters_WriteAndX_Req {
	uint8_t		w_AndXCommand;
	uint8_t		w_AndXReserved;
	uint16_t	w_AndXOffset;
	uint16_t	w_FID;
	uint32_t	w_Offset;
	uint32_t	w_Timeout;
	uint16_t	w_WriteMode;
	uint16_t	w_Remaining;
	uint16_t	w_Reserved;
	uint16_t	w_DataLength;
	uint16_t	w_DataOffset;
	uint16_t	w_OffseyHigh;
} __attribute__((packed)) SMB_Parameters_WriteAndX_Req_t;

typedef struct SMB_Parameters_WriteAndX_Res {
	uint8_t		w_AndXCommand;
	uint8_t		w_AndXReserved;
	uint16_t	w_AndXOffset;
	uint16_t	w_Count;
	uint16_t	w_Available;
	uint32_t	w_Reserved;
} __attribute__((packed)) SMB_Parameters_WriteAndX_Res_t;

// READ_ANDX
typedef struct SMB_Parameters_ReadAndX_Req {
	uint8_t		r_AndXCommand;
	uint8_t		r_AndXReserved;
	uint16_t	r_AndXOffset;
	uint16_t	r_FID;
	uint32_t	r_Offset;
	uint16_t	r_MaxCountOfBytesToReturn;
	uint16_t	r_MinCountOfBytesToReturn;
	uint32_t	r_Timeout;
	uint16_t	r_Remaining;
	uint16_t	r_OffseyHigh;
} __attribute__((packed)) SMB_Parameters_ReadAndX_Req_t;

typedef struct SMB_Parameters_ReadAndX_Res {
	uint8_t		r_AndXCommand;
	uint8_t		r_AndXReserved;
	uint16_t	r_AndXOffset;
	uint16_t	r_Available;
	uint16_t	r_DataCompactionMode;
	uint16_t	r_Reserved1;
	uint16_t	r_DataLength;
	uint16_t	r_DataOffset;
	uint16_t	r_Reserved2[5];
} __attribute__((packed)) SMB_Parameters_ReadAndX_Res_t;

//	Transaction mode
typedef	enum TransactionState {
	TransmittedAllRequests	= 0,
	TransactionPrimaryRequest,
	ReceivedInterimResponse,
} TransactionState_t;

//	PIDMID
typedef	struct PIDMIDHead {
	Mtx_t	h_Mtx;
	list_t	h_Lnk;
} PIDMIDHead_t;

typedef	struct PIDMID {
	list_t				pm_Lnk;
	uint32_t			pm_PID;
	uint16_t			pm_MID;
	TransactionState_t	pm_State;
	uint16_t			pm_TotalParameterCountReq;
	uint16_t			pm_TotalDataCountReq;
	uint16_t			pm_MaxParameterCountReq;
	uint16_t			pm_MaxDataCountReq;
	uint16_t			pm_TotalParameterCountRes;
	uint16_t			pm_TotalDataCountRes;
	bool_t				pm_bNotify;
} PIDMID_t;

// Connector(Client) - Adaptor(Server)
// Connector
typedef struct ConCoC {
	FdEvCoC_t		c_FdEvCoC;
//	struct Session	*c_pSession;
//	int				c_NoSession;
	Mtx_t			c_Mtx;
	uint32_t		c_SessionKeyMeta;	
	uint16_t		c_UIDMeta;	
	uint16_t		c_TIDMeta;	
	PIDMIDHead_t	c_PIDMID;		// PIDMID list
	uint16_t		c_FIDMeta;		// current
	list_t			c_LnkFID;		// Open FID
	Hash_t			c_HashFIDMeta;
	Hash_t			c_HashFIDName;
} ConCoC_t;

// Adapter
// PairFID control
// PairTID control
typedef struct PairFID {
	list_t		p_Lnk;
	int			p_Cnt;
	uint16_t	p_FIDMeta;
	uint16_t	p_FID;
	char		p_NameMD5[MD5_DIGEST_LENGTH*2+1];
	list_t		p_AdaptorEvent;
} PairFID_t;

#ifdef	ZZZ
#else
typedef struct PairTID {
	list_t		p_Lnk;
	int			p_Cnt;
	uint16_t	p_TIDMeta;
	uint16_t	p_TID;
} PairTID_t;
#endif

typedef struct PairCoS {
	Mtx_t			s_Mtx;
	Cnd_t			s_Cnd;
	list_t			s_LnkFID;
	Hash_t			s_HashFID;
	Hash_t			s_HashFIDMeta;
#ifdef	ZZZ
#else
	list_t			s_LnkTID;
	Hash_t			s_HashTID;
	Hash_t			s_HashTIDMeta;
#endif
} PairCoS_t;

typedef struct ConCoS {
	list_t			s_Lnk;
	Mtx_t			s_Mtx;
	uint32_t		s_SessionKeyMeta;	
	uint32_t		s_SessionKey;	
	uint16_t		s_UIDMeta;	
	uint16_t		s_UID;	
	uint16_t		s_TIDMeta;	
	uint16_t		s_TID;	
	uint16_t		s_TIDFlags;
	uint16_t		s_FIDMeta;	
#ifdef	ZZZ
	PairCoS_t		s_FID;
#else
	PairCoS_t		s_Pair;
#endif
	PIDMIDHead_t	s_PIDMID;		// PIDMID list
	FdEvent_t		s_FdEvent;
	struct PaxosAccept	*s_pAccept;
	struct PaxosAccept	*s_pEvent;	// Event
	int				s_CnvCmd;
pthread_t	s_ThRecv;	// FOR DEBUG
} ConCoS_t;

// Con - Control
typedef struct CtrlCoC {
	CnvCtrlCoC_t	c_Ctrl;
	struct IdMap	*c_pIdMap;	
	struct Session	*c_pEvent;
	pthread_t		c_ThreadEvent;
} CtrlCoC_t;

typedef struct CtrlCoS {
	CnvCtrlCoS_t	s_Ctrl;
} CtrlCoS_t;

extern	int IsSMBDialect( SMB_Dialect_t *pBuffer, SMB_Dialect_t **ppNext );

// Unicode
extern	int UnicodeLen( uint16_t *p16 );
extern	wchar_t* CreateUnicodeToWchar( uint16_t *p16, size_t ByteLen );
extern	void DestroyUnicodeToWchar( wchar_t *p );

// Meta
extern	int CIFSMetaHeadInsert( Msg_t *pMsg, MetaHead_t *pMetaHead );
extern	Meta_Return_t* CIFSMetaReturn( uint16_t ID );

// PIDMID
extern	int PIDMIDInit( PIDMIDHead_t *pHead );
extern	int PIDMIDTerm( PIDMIDHead_t *pHead );
extern	PIDMID_t* PIDMIDFind( PIDMIDHead_t *pHead, 
					uint32_t PID, uint16_t MID, bool_t bCreate );
extern	int PIDMIDDelete( PIDMIDHead_t *pHead, PIDMID_t *pPIDMID );

// FID,TID
extern	int FIDCmp( void *pA, void *pB );
extern	int 	FIDMetaCmp( void *pA, void *pB );
extern	int 	FIDNameCmp( void *pA, void *pB );

extern	int PairCoSInit( PairCoS_t *pPairCoS );
extern PairFID_t* PairCoSGetByFIDMeta( PairCoS_t *pPairCoS, 
										uint16_t FIDMeta, bool_t bCreate );
extern	int PairCoSSetFID(PairCoS_t *pPairCoS, PairFID_t *pPair, uint16_t FID);
#ifdef	ZZZ
extern	int PairCoSPut(PairCoS_t *pPairCoS, PairFID_t *pPair, bool_t bDestroy);
#else
extern	int PairCoSPutFID(PairCoS_t *pPairCoS, 
									PairFID_t *pPair, bool_t bDestroy);

extern PairTID_t* PairCoSGetByTIDMeta( PairCoS_t *pPairCoS, 
										uint16_t TIDMeta, bool_t bCreate );
extern	int PairCoSSetTID(PairCoS_t *pPairCoS, PairTID_t *pPair, uint16_t TID);
extern PairTID_t* PairCoSGetByTID( PairCoS_t *pPairCoS, uint16_t TID );
extern	int PairCoSPutTID(PairCoS_t *pPairCoS, 
									PairTID_t *pPair, bool_t bDestroy);
#endif

extern	Msg_t* CIFSRecvMsgByFd( int Fd, 
		void*(*fMalloc)(size_t), void(*fFree)(void*) );
extern	int CIFSSendMsgByFd( int Fd, Msg_t *pMsg );

/*
 *	uint16_t	id
 */
typedef	struct IdMap {
	void		*(*IM_fMalloc)(size_t);
	void		(*IM_fFree)(void*);	
	int32_t		IM_Size;
	uint16_t	IM_NextId;
	uint8_t		IM_aMap[0];
} IdMap_t;

extern	IdMap_t		*IdMapCreate( int Size, 
						void*(*fMalloc)(size_t), void(*fFree)(void*) );
extern	void		IdMapDestroy( IdMap_t *pIM );
extern	uint16_t	IdMapGet( IdMap_t *pIM );
extern	int			IdMapPut( IdMap_t *pIM, uint16_t Id );
extern	int			IdMapSet( IdMap_t *pIM, uint16_t Id );

#if 0
/*
 *	CIFS protocol(MS-SMB2)
 */
typedef struct smb_head {
	uint8_t		h_ProtocolId[4];
	uint16_t	h_StructureSize, h_CreditCharge;
	uint32_t	h_Status;
	uint16_t	h_Command, h_CreditRR;
	uint32_t	h_Flags;
	uint32_t	h_NextCommand;
} smb_head_t;

typedef struct smb_head_async {
	smb_head_t	a_Head;
	uint64_t	a_MessageId;
	uint64_t	a_AsyncId;
	uint64_t	a_SessionId;	
	uint8_t		s_Signature[8];
} smb_head_async_t;

typedef struct smb_head_sync {
	smb_head_t	s_Head;
	uint64_t	s_MessageId;
	uint32_t	s_Reserved,	TreeID;
	uint64_t	s_SessionId;	
	uint8_t		s_Signature[16];
} smb_head_sync_t;

#define	SMB2_NEGOTIATE			0x0000
#define	SMB2_SESSION_SETUP		0x0001
#define	SMB2_LOGOFF				0x0002
#define	SMB2_TREE_CONNECT		0x0003
#define	SMB2_TREE_DISCONNECT	0x0004
#define	SMB2_CREATE				0x0005
#define	SMB2_CLOSE				0x0006
#define	SMB2_FLUSH				0x0007
#define	SMB2_READ				0x0008
#define	SMB2_WRITE				0x0009
#define	SMB2_LOCK				0x000A
#define	SMB2_IOCTL				0x000B
#define	SMB2_CANCEL				0x000C
#define	SMB2_ECHO				0x000D
#define	SMB2_QUERY_DIRECTORY	0x000E
#define	SMB2_CHANGE_NOTIFY		0x000F
#define	SMB2_QUERY_INFO			0x0010
#define	SMB2_SET_INFO			0x0011
#define	SMB2_OPLOCK_BREAK		0x0012

#define	SMB2_FLAGS_SERVER_TO_REDIR		0x00000001
#define	SMB2_FLAGS_ASYNC_COMMAND		0x00000002
#define	SMB2_FLAGS_RELATED_OPERATIONS	0x00000004
#define	SMB2_FLAGS_SIGNED				0x00000008
#define	SMB2_FLAGS_PRIORITY_MASK		0x00000070
#define	SMB2_FLAGS_DFS_OPERATIONS		0x10000000
#define	SMB2_FLAGS_RELAY_OPERATION		0x20000000

typedef struct smb_ERROR_Response {
	uint16_t	e_StructureSize;
	uint8_t		e_ErrorContextCount, e_Reserved;
	uint32_t	e_ByteCount;
	uint8_t		e_ErrorData[0];
} smb_ERROR_Response_t;

typedef struct smb_ERROR_Context_Response {
	uint32_t	e_ErrorDataLength;
	uint32_t	e_ErrorId;
	uint8_t		e_ErrorContextData[0];
} smb_ERROR_Context_Response_t;

#define	SMB2_ERROR_ID_DEFAULT		0x00000000

typedef struct smb_ERROR_SymLink_Response {
	uint32_t	e_SymLinkLength;
	uint32_t	e_SymLinkErrorTag;
	uint32_t	e_ReparseTag;
	uint16_t	e_ReparseDataLength;
	uint16_t	e_UnparsedPathLength;
	uint16_t	e_SubstituteNameOffset;
	uint16_t	e_SubstituteNameLength;
	uint16_t	e_PrintNameOffset;
	uint16_t	e_PrintNameLength;
	uint32_t	e_Flags;
	uint8_t		e_PathBuffer[0];
} smb_ERROR_SymLink_Response_t;

#define	SMB2_SYMLINK_FLAG_RELATIVE	0x00000001

typedef struct smb_NEGOTIATE_Request {
	uint16_t	n_StructureSize;
	uint16_t	n_DialectCount;
	uint16_t	n_SecurityMode;
	uint16_t	n_Reserved;
	uint32_t	n_Capabilities;
	GUID		n_ClientGuid;
	uint8_t		n_ClientStartTime[8];
	// followed Dialects, Padding, NegotiateCintextList
} smb_NEGOTIATE_Request_t;

#define	SMB2_NEGOTIATE_SIGNING_ENABLED	0x0001
#define	SMB2_NEGOTIATE_SIGNING_REQUIRED	0x0002

#define	SMB2_GLOBAL_CAP_DFS					0x00000001
#define	SMB2_GLOBAL_CAP_LEASING				0x00000002
#define	SMB2_GLOBAL_CAP_LARGE_MTU			0x00000004
#define	SMB2_GLOBAL_CAP_MULTI_CHANNEL		0x00000008
#define	SMB2_GLOBAL_CAP_PERSISTENT_HANDLES	0x00000010
#define	SMB2_GLOBAL_CAP_DIRECTORY_LEASING	0x00000020
#define	SMB2_GLOBAL_CAP_ENCRYPTION			0x00000040
#endif

#endif	//_CIFS_H_
