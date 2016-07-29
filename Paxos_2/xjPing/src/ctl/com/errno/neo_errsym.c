/*******************************************************************************
 * 
 *  neo_errsym.c --- xjPing (On Memory DB) error message handling programs
 * 
 *  Copyright (C) 2010 triTech Inc. All Rights Reserved.
 * 
 *  See GPL-LICENSE.txt for copyright and license details.
 * 
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version. 
 * 
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ******************************************************************************/
/* 
 *   çÏê¨            ìnï”ìTçF
 *   ééå±
 *   ì¡ãñÅAíòçÏå†    ÉgÉâÉCÉeÉbÉN
 */ 

/******************************************************************************
 *
 *      neo_errsym.c
 *
 *      ê‡ñæ    neo_errno Ç…ëŒâûÇ∑ÇÈÉGÉâÅ[ÉÅÉbÉZÅ[ÉWÇï‘Ç∑
 *
 ******************************************************************************/
#include "neo_system.h"

/*
 *      ÉGÉâÅ[ÉVÉìÉ{Éãç\ë¢ëÃ
 */
typedef struct {
        int neo_errno;  /* neo_errno */
        char* msg;      /* ÉGÉâÅ[ÉÅÉbÉZÅ[ÉW */
} _errsym_t;

extern _errsym_t _neo_errsymtab[];

/******************************************************************************
 * ä÷êîñº
 *              neo_errsym()
 * ã@î *              neo_errno Ç…ëŒâûÇ∑ÇÈÉGÉâÅ[ÉÅÉbÉZÅ[ÉWÇï‘Ç∑
 * ä÷êîíl
 *      ê≥èÌ: ÉVÉìÉ{Éãï∂éöóÒ
 *      àŸèÌ: Not defined error ÉÅÉbÉZÅ[ÉW
 * íçà”
 ******************************************************************************/
extern char*
neo_errsym()
{
        _errsym_t* t;
        static char buf[128];

        for( t = _neo_errsymtab; t->msg; t++ ){
                if( neo_errno == t->neo_errno ) {
                        sprintf( buf, "%s[%x]", t->msg, neo_errno );
                        return( buf );
                }
        }

        /*
         *      ÉÅÉbÉZÅ[ÉW Not found !!
         */
        sprintf( buf, "%s[%x]", "Not defined error", neo_errno );
        return( buf );
}

/*
 *      ÉGÉâÅ[ÉVÉìÉ{ÉãÉeÅ[ÉuÉã
 */
_errsym_t _neo_errsymtab[] = {
{ E_RDB_NOMEM,"no memory" },
{ E_RDB_TBL_BUSY,"table is busy" },
{ E_RDB_REC_BUSY,"record is busy" },
{ E_RDB_CONFUSED,"server status is confused" },
{ E_RDB_MD,"invalid server-ID" },
{ E_RDB_EXIST,"already exist" },
{ E_RDB_INVAL,"invalid argument" },
{ E_RDB_SPACE,"no space in disk" },
{ E_RDB_TD,"invalid table-ID" },
{ E_RDB_UD,"invalid user-ID" },
{ E_RDB_NOCACHE,"no cache space" },
{ E_RDB_FD,"file descriptor" },
{ E_RDB_SEARCH,"illegal search condition" },
{ E_RDB_NOITEM,"cannot find item name" },
{ E_RDB_NOEXIST,"no entry" },
{ E_RDB_RETRY,"retry over" },
{ E_RDB_STOP,"server stop" },
{ E_RDB_PROTOTYPE,"invalid protocol type" },
{ E_RDB_FILE,"invalid table-file type" },
{ E_RDB_HOLD,"table is locked" },
{ E_BYTESFILE_ERROR,"bytes file operation" },
{ E_RDB_NOHASH,"no hash" },
{ E_RDB_NOSUPPORT,"no support" },
{ E_RDB_CLOSED,"already closed" },
{ E_LINK_PORT,"invalid port" },
{ E_LINK_FD,"invalid file descriptor" },
{ E_LINK_ADDR,"invalid address" },
{ E_LINK_NONAME,"invalid name" },
{ E_LINK_INVAL,"link invalid" },
{ E_LINK_REJECT,"rejected" },
{ E_LINK_NOMEM,"no memory" },
{ E_LINK_BUSY,"busy" },
{ E_LINK_DOWN,"down" },
{ E_LINK_PACKET,"invalid packet" },
{ E_LINK_FAMILY,"invalid family" },
{ E_LINK_BUFSIZE,"buffer size" },
{ E_LINK_TIMEOUT,"timeout" },
{ E_LINK_RETRY,"retry over" },
{ E_LINK_DUPFRAME,"dupricated" },
{ E_LINK_CNTL,"control" },
{ E_LINK_NODATA,"no data" },
{ E_LINK_CONFFILE,"configuration" },
{ E_SKL_INVARG,"invalid argument" },
{ E_SKL_NOMEM,"no memory" },
{ E_SKL_TERM,"SIGTERM" },
{ E_SQL_PARSE,"parse error" },
{ E_SQL_SELECT,"select" },
{ E_SQL_EXPR,"expression" },
{ E_SQL_EVAL,"evaluation" },
{ E_SQL_USR,"allocate usr" },
{ E_SQL_ATTR,"attrubute" },
{ E_SQL_SCHEMA,"shema" },
{ E_SQL_NOMEM,"no memory" },
{ E_UNIX_EPERM,"Not owner" },
{ E_UNIX_ENOENT,"No such file or directory" },
{ E_UNIX_ESRCH,"No such process" },
{ E_UNIX_EINTR,"Interrupted system call" },
{ E_UNIX_EIO,"I/O error" },
{ E_UNIX_ENXIO,"No such device or address" },
{ E_UNIX_E2BIG,"Arg list too long" },
{ E_UNIX_ENOEXEC,"Exec format error" },
{ E_UNIX_EBADF,"Bad file number" },
{ E_UNIX_ECHILD,"No children" },
{ E_UNIX_EAGAIN,"No more processes" },
{ E_UNIX_ENOMEM,"Not enough core" },
{ E_UNIX_EACCES,"Permission denied" },
{ E_UNIX_EFAULT,"Bad address" },
{ E_UNIX_ENOTBLK,"Block device required" },
{ E_UNIX_EBUSY,"Mount device busy" },
{ E_UNIX_EEXIST,"File exists" },
{ E_UNIX_EXDEV,"Cross-device link" },
{ E_UNIX_ENODEV,"No such device" },
{ E_UNIX_ENOTDIR,"Not a" },
{ E_UNIX_EISDIR,"Is a directory" },
{ E_UNIX_EINVAL,"Invalid argument" },
{ E_UNIX_ENFILE,"File table overflow" },
{ E_UNIX_EMFILE,"Too many open files" },
{ E_UNIX_ENOTTY,"Not a typewriter" },
{ E_UNIX_ETXTBSY,"Text file busy" },
{ E_UNIX_EFBIG,"File too large" },
{ E_UNIX_ENOSPC,"No space left on device" },
{ E_UNIX_ESPIPE,"Illegal seek" },
{ E_UNIX_EROFS,"Read-only file system" },
{ E_UNIX_EMLINK,"Too many links" },
{ E_UNIX_EPIPE,"Broken pipe" },
{ E_UNIX_EDOM,"Argument too large" },
{ E_UNIX_ERANGE,"Result too large" },
{ E_UNIX_EWOULDBLOCK,"Operation would block" },
{ E_UNIX_EINPROGRESS,"Operation now in progress" },
{ E_UNIX_EALREADY,"Operation already in progress" },
{ E_UNIX_ENOTSOCK,"Socket operation on non-socket" },
{ E_UNIX_EDESTADDRREQ,"Destination address required" },
{ E_UNIX_EMSGSIZE,"Message too long" },
{ E_UNIX_EPROTOTYPE,"Protocol wrong type for socket" },
{ E_UNIX_ENOPROTOOPT,"Protocol not available" },
{ E_UNIX_EPROTONOSUPPORT,"Protocol not supported" },
{ E_UNIX_ESOCKTNOSUPPORT,"Socket type not supported" },
{ E_UNIX_EOPNOTSUPP,"Operation not supported on socket" },
{ E_UNIX_EPFNOSUPPORT,"Protocol family not supported" },
{ E_UNIX_EAFNOSUPPORT,"Address family not supported by protocol family" },
{ E_UNIX_EADDRINUSE,"Address already in use" },
{ E_UNIX_EADDRNOTAVAIL,"Can't assign requested address" },
{ E_UNIX_ENETDOWN,"Network is down" },
{ E_UNIX_ENETUNREACH,"Network is unreachable" },
{ E_UNIX_ENETRESET,"Network dropped connection on reset" },
{ E_UNIX_ECONNABORTED,"Software caused connection abort" },
{ E_UNIX_ECONNRESET,"Connection reset by peer" },
{ E_UNIX_ENOBUFS,"No buffer space available" },
{ E_UNIX_EISCONN,"Socket is already connected" },
{ E_UNIX_ENOTCONN,"Socket is not connected" },
{ E_UNIX_ESHUTDOWN,"Can't send after socket shutdown" },
{ E_UNIX_ETOOMANYREFS,"Too many references: can't splice" },
{ E_UNIX_ETIMEDOUT,"Connection timed out" },
{ E_UNIX_ECONNREFUSED,"Connection refused" },
{ E_UNIX_ELOOP,"Too many levels of symbolic links" },
{ E_UNIX_ENAMETOOLONG,"File name too long" },
{ E_UNIX_EHOSTDOWN,"Host is down" },
{ E_UNIX_EHOSTUNREACH,"No route to host" },
{ E_UNIX_ENOTEMPTY,"Directory not empty" },
{ E_UNIX_EUSERS,"Too many users" },
{ E_UNIX_EDQUOT,"Disc quota exceeded" },
{ E_UNIX_ESTALE,"Stale NFS file handle" },
{ E_UNIX_EREMOTE,"Too many levels of remote in path" },
{ E_UNIX_ENOSTR,"Device is not a stream" },
{ E_UNIX_ETIME,"Timer expired" },
{ E_UNIX_ENOSR,"Out of streams resources" },
{ E_UNIX_ENOMSG,"No message of desired type" },
{ E_UNIX_EBADMSG,"Trying to read unreadable message" },
{ E_UNIX_EIDRM,"Identifier removed" },
{ E_UNIX_EDEADLK,"Deadlock condition." },
{ E_UNIX_ENOLCK,"No record locks available." },
{ E_UNIX_ENONET,"Machine is not on the network" },
{ E_UNIX_ENOLINK,"the link has been severed" },
{ E_UNIX_EADV,"advertise error" },
{ E_UNIX_ESRMNT,"srmount error" },
{ E_UNIX_ECOMM,"Communication error on send" },
{ E_UNIX_EPROTO,"Protocol error" },
{ E_UNIX_EMULTIHOP,"multihop attempted" },
{ E_UNIX_EDOTDOT,"Cross mount point (not an error)" },
{ E_UNIX_EREMCHG,"Remote address changed" },
{ E_UNIX_ENOSYS,"function not implemented" },
{ E_UNIX_EILSEQ,"Illegal byte sequence." },
{ E_UNIX_ECHRNG,"Channel number out of range" },
{ E_UNIX_EL2NSYNC,"Level 2 not synchronized" },
{ E_UNIX_EL3HLT,"Level 3 halted" },
{ E_UNIX_EL3RST,"Level 3 reset" },
{ E_UNIX_ELNRNG,"Link number out of range" },
{ E_UNIX_EUNATCH,"Protocol driver not attached" },
{ E_UNIX_ENOCSI,"No CSI structure available" },
{ E_UNIX_EL2HLT,"Level 2 halted" },
{ E_UNIX_EBADE,"invalid exchange" },
{ E_UNIX_EBADR,"invalid request descriptor" },
{ E_UNIX_EXFULL,"exchange full" },
{ E_UNIX_ENOANO,"no anode" },
{ E_UNIX_EBADRQC,"invalid request code" },
{ E_UNIX_EBADSLT,"invalid slot" },
{ E_UNIX_EDEADLOCK,"file locking deadlock error" },
{ E_UNIX_EBFONT,"bad font file fmt" },
{ E_UNIX_ENODATA,"no data (for no delay io)" },
{ E_UNIX_ENOPKG,"Package not installed" },
{ E_UNIX_EOVERFLOW,"value too large to be stored in data type" },
{ E_UNIX_ENOTUNIQ,"given log. name not unique" },
{ E_UNIX_EBADFD,"f.d. invalid for this operation" },
{ E_UNIX_ELIBACC,"Can't access a needed shared lib." },
{ E_UNIX_ELIBBAD,"Accessing a corrupted shared lib." },
{ E_UNIX_ELIBSCN,".lib section in a.out corrupted." },
{ E_UNIX_ELIBMAX,"Attempting to link in too many libs." },
{ E_UNIX_ELIBEXEC,"Attempting to exec a shared library." },
{ E_UNIX_ERESTART,"Restartable system call" },
{ E_UNIX_ESTRPIPE,"if pipe/FIFO, don't sleep in stream head" },
{-1,0}
};

