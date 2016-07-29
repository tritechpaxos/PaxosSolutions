/*******************************************************************************
 * 
 *  unix_errno.c --- errno Convert UNIX <-> xjPing programs
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
 *      unix_errno.c
 *
 *      ê‡ñæ    UNIX errno Ç∆ xjPing neo_errno ÇÃïœä∑
 *
 ******************************************************************************/

#include <errno.h>
#include "neo_system.h"

/*
 *      ÉGÉâÅ[î‘çÜä«óùç\ë¢ëÃ
 */
typedef struct {
        int unx_errno;          /* errno */
        int neo_errno;          /* neo_errno */
} uerr_t;

extern uerr_t unix2neotab[];


/******************************************************************************
 * ä÷êîñº
 *              unixerr2neo()
 * ã@î *              UNIX errno Ç©ÇÁ xjPing neo_errno Ç÷ÇÃïœä∑
 * ä÷êîíl
 *      ê≥èÌ: neo_errno
 *      àŸèÌ: 0
 * íçà”
 ******************************************************************************/
extern int
unixerr2neo()
{
        uerr_t  *t;

        for( t = unix2neotab; t->unx_errno; t++ ){
                if( t->unx_errno == errno ) {
                        return( t->neo_errno );
                }
        }

        return( (M_UNIX << 24)|0 );
}


/******************************************************************************
 * ä÷êîñº
 *              neoerr2unix()
 * ã@î *              xjPing neo_errno Ç©ÇÁ UNIX errno Ç÷ÇÃïœä∑
 * ä÷êîíl
 *      ê≥èÌ: errno
 *      àŸèÌ: 0
 * íçà”
 ******************************************************************************/
extern int
neoerr2unix()
{
        uerr_t  *t;

        for( t = unix2neotab; t->unx_errno; t++ ){
                if( t->neo_errno == neo_errno ) {
                        return( t->unx_errno );
                }
        }

        return( 0 );
}


/*
 *      ÉGÉâÅ[î‘çÜä«óùÉeÅ[ÉuÉã
 */
uerr_t unix2neotab[] = {
{ EPERM,          E_UNIX_EPERM },
{ ENOENT,          E_UNIX_ENOENT },
{ ESRCH,          E_UNIX_ESRCH },
{ EINTR,          E_UNIX_EINTR },
{ EIO,                  E_UNIX_EIO },
{ ENXIO,          E_UNIX_ENXIO },
{ E2BIG,          E_UNIX_E2BIG },
{ ENOEXEC,          E_UNIX_ENOEXEC },
{ ECHILD,          E_UNIX_ECHILD },
{ EAGAIN,          E_UNIX_EAGAIN },
{ ENOMEM,          E_UNIX_ENOMEM },
{ EACCES,          E_UNIX_EACCES },
{ EFAULT,          E_UNIX_EFAULT },
{ ENOTBLK,          E_UNIX_ENOTBLK },
{ EBUSY,          E_UNIX_EBUSY },
{ EEXIST,          E_UNIX_EEXIST },
{ EXDEV,          E_UNIX_EXDEV },
{ ENODEV,          E_UNIX_ENODEV },
{ ENOTDIR,          E_UNIX_ENOTDIR },
{ EISDIR,          E_UNIX_EISDIR },
{ EINVAL,          E_UNIX_EINVAL },
{ ENFILE,          E_UNIX_ENFILE },
{ EMFILE,          E_UNIX_EMFILE },
{ ENOTTY,          E_UNIX_ENOTTY },
{ ETXTBSY,          E_UNIX_ETXTBSY },
{ EFBIG,          E_UNIX_EFBIG },
{ ENOSPC,          E_UNIX_ENOSPC },
{ ESPIPE,          E_UNIX_ESPIPE },
{ EROFS,          E_UNIX_EROFS },
{ EMLINK,          E_UNIX_EMLINK },
{ EPIPE,          E_UNIX_EPIPE },
{ EDOM,                  E_UNIX_EDOM },
{ ERANGE,          E_UNIX_ERANGE },
{ EWOULDBLOCK,          E_UNIX_EWOULDBLOCK },
{ EINPROGRESS,          E_UNIX_EINPROGRESS },
{ EALREADY,          E_UNIX_EALREADY },
{ ENOTSOCK,          E_UNIX_ENOTSOCK },
{ EDESTADDRREQ,          E_UNIX_EDESTADDRREQ },
{ EMSGSIZE,          E_UNIX_EMSGSIZE },
{ EPROTOTYPE,          E_UNIX_EPROTOTYPE },
{ ENOPROTOOPT,          E_UNIX_ENOPROTOOPT },
{ EPROTONOSUPPORT,  E_UNIX_EPROTONOSUPPORT },
{ ESOCKTNOSUPPORT,  E_UNIX_ESOCKTNOSUPPORT },
{ EOPNOTSUPP,          E_UNIX_EOPNOTSUPP },
{ EPFNOSUPPORT,          E_UNIX_EPFNOSUPPORT },
{ EAFNOSUPPORT,          E_UNIX_EAFNOSUPPORT },
{ EADDRINUSE,          E_UNIX_EADDRINUSE },
{ EADDRNOTAVAIL,  E_UNIX_EADDRNOTAVAIL },
{ ENETDOWN,          E_UNIX_ENETDOWN },
{ ENETUNREACH,          E_UNIX_ENETUNREACH },
{ ENETRESET,          E_UNIX_ENETRESET },
{ ECONNABORTED,          E_UNIX_ECONNABORTED },
{ ECONNRESET,          E_UNIX_ECONNRESET },
{ ENOBUFS,          E_UNIX_ENOBUFS },
{ EISCONN,          E_UNIX_EISCONN },
{ ENOTCONN,          E_UNIX_ENOTCONN },
{ ESHUTDOWN,          E_UNIX_ESHUTDOWN },
{ ETOOMANYREFS,          E_UNIX_ETOOMANYREFS },
{ ETIMEDOUT,          E_UNIX_ETIMEDOUT },
{ ECONNREFUSED,          E_UNIX_ECONNREFUSED },
{ ELOOP,          E_UNIX_ELOOP },
{ ENAMETOOLONG,          E_UNIX_ENAMETOOLONG },
{ EHOSTDOWN,          E_UNIX_EHOSTDOWN },
{ EHOSTUNREACH,          E_UNIX_EHOSTUNREACH },
{ ENOTEMPTY,          E_UNIX_ENOTEMPTY },
{ EUSERS,          E_UNIX_EUSERS },
{ EDQUOT,          E_UNIX_EDQUOT },
{ ESTALE,          E_UNIX_ESTALE },
{ EREMOTE,          E_UNIX_EREMOTE },
{ ENOSTR,          E_UNIX_ENOSTR },
{ ENOSR,          E_UNIX_ENOSR },
{ ENOMSG,          E_UNIX_ENOMSG },
{ EBADMSG,          E_UNIX_EBADMSG },
{ EIDRM,          E_UNIX_EIDRM },
{ EDEADLK,          E_UNIX_EDEADLK },
{ ENOLCK,          E_UNIX_ENOLCK },
{ ENONET,          E_UNIX_ENONET },
{ ENOLINK,          E_UNIX_ENOLINK },
{ EADV,                  E_UNIX_EADV },
{ ESRMNT,          E_UNIX_ESRMNT },
{ ECOMM,          E_UNIX_ECOMM },
{ EPROTO,          E_UNIX_EPROTO },
{ EMULTIHOP,          E_UNIX_EMULTIHOP },
{ EDOTDOT,          E_UNIX_EDOTDOT },
{ EREMCHG,          E_UNIX_EREMCHG },
{ ENOSYS,          E_UNIX_ENOSYS },
{ EILSEQ,          E_UNIX_EILSEQ },
{ ECHRNG,          E_UNIX_ECHRNG },
{ EL2NSYNC,          E_UNIX_EL2NSYNC },
{ EL3HLT,          E_UNIX_EL3HLT },
{ EL3RST,          E_UNIX_EL3RST },
{ ELNRNG,          E_UNIX_ELNRNG },
{ EUNATCH,          E_UNIX_EUNATCH },
{ ENOCSI,          E_UNIX_ENOCSI },
{ EL2HLT,          E_UNIX_EL2HLT },
{ EBADE,          E_UNIX_EBADE },
{ EBADR,          E_UNIX_EBADR },
{ EXFULL,          E_UNIX_EXFULL },
{ ENOANO,          E_UNIX_ENOANO },
{ EBADRQC,          E_UNIX_EBADRQC },
{ EBADSLT,          E_UNIX_EBADSLT },
{ EDEADLOCK,          E_UNIX_EDEADLOCK },
{ EBFONT,          E_UNIX_EBFONT },
{ ENODATA,          E_UNIX_ENODATA },
{ ENOPKG,          E_UNIX_ENOPKG },
{ EOVERFLOW,          E_UNIX_EOVERFLOW },
{ ENOTUNIQ,          E_UNIX_ENOTUNIQ },
{ EBADFD,          E_UNIX_EBADFD },
{ ELIBACC,          E_UNIX_ELIBACC },
{ ELIBBAD,          E_UNIX_ELIBBAD },
{ ELIBSCN,          E_UNIX_ELIBSCN },
{ ELIBMAX,          E_UNIX_ELIBMAX },
{ ELIBEXEC,          E_UNIX_ELIBEXEC },
{ ERESTART,          E_UNIX_ERESTART },
{ ESTRPIPE,          E_UNIX_ESTRPIPE },
{ 0,0 }
};
