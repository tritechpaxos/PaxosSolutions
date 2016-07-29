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

/******************************************************************************
 *
 *
 *	neo_unix_err.h
 *
 *	ê‡ñæ	UNIXÉGÉâÅ[î‘çÜ
 *
 ******************************************************************************/
#ifndef _NEO_UNIX_ERRNO_
#define _NEO_UNIX_ERRNO_

#include	<sys/errno.h>
#include	"neo_class.h"

#define	E_UNIX_EPERM		1	/* Not owner */
#define	E_UNIX_ENOENT		2	/* No such file or directory */
#define	E_UNIX_ESRCH		3	/* No such process */
#define	E_UNIX_EINTR		4	/* Interrupted system call */
#define	E_UNIX_EIO		5	/* I/O error */
#define	E_UNIX_ENXIO		6	/* No such device or address */
#define	E_UNIX_E2BIG		7	/* Arg list too long */
#define	E_UNIX_ENOEXEC		8	/* Exec format error */
#define	E_UNIX_EBADF		9	/* Bad file number */
#define	E_UNIX_ECHILD		10	/* No children */
#define	E_UNIX_EAGAIN		11	/* No more processes */
#define	E_UNIX_ENOMEM		12	/* Not enough core */
#define	E_UNIX_EACCES		13	/* Permission denied */
#define	E_UNIX_EFAULT		14	/* Bad address */
#define	E_UNIX_ENOTBLK		15	/* Block device required */
#define	E_UNIX_EBUSY		16	/* Mount device busy */
#define	E_UNIX_EEXIST		17	/* File exists */
#define	E_UNIX_EXDEV		18	/* Cross-device link */
#define	E_UNIX_ENODEV		19	/* No such device */
#define	E_UNIX_ENOTDIR		20	/* Not a directory*/
#define	E_UNIX_EISDIR		21	/* Is a directory */
#define	E_UNIX_EINVAL		22	/* Invalid argument */
#define	E_UNIX_ENFILE		23	/* File table overflow */
#define	E_UNIX_EMFILE		24	/* Too many open files */
#define	E_UNIX_ENOTTY		25	/* Not a typewriter */
#define	E_UNIX_ETXTBSY		26	/* Text file busy */
#define	E_UNIX_EFBIG		27	/* File too large */
#define	E_UNIX_ENOSPC		28	/* No space left on device */
#define	E_UNIX_ESPIPE		29	/* Illegal seek */
#define	E_UNIX_EROFS		30	/* Read-only file system */
#define	E_UNIX_EMLINK		31	/* Too many links */
#define	E_UNIX_EPIPE		32	/* Broken pipe */

/* math software */
#define	E_UNIX_EDOM		33	/* Argument too large */
#define	E_UNIX_ERANGE		34	/* Result too large */

/* non-blocking and interrupt i/o */
#define	E_UNIX_EWOULDBLOCK	35	/* Operation would block */
#define	E_UNIX_EINPROGRESS	36	/* Operation now in progress */
#define	E_UNIX_EALREADY		37	/* Operation already in progress */
/* ipc/network software */

	/* argument errors */
#define	E_UNIX_ENOTSOCK		38	/* Socket operation on non-socket */
#define	E_UNIX_EDESTADDRREQ	39	/* Destination address required */
#define	E_UNIX_EMSGSIZE		40	/* Message too long */
#define	E_UNIX_EPROTOTYPE	41	/* Protocol wrong type for socket */
#define	E_UNIX_ENOPROTOOPT	42	/* Protocol not available */
#define	E_UNIX_EPROTONOSUPPORT	43	/* Protocol not supported */
#define	E_UNIX_ESOCKTNOSUPPORT	44	/* Socket type not supported */
#define	E_UNIX_EOPNOTSUPP	45	/* Operation not supported on socket */
#define	E_UNIX_EPFNOSUPPORT	46	/* Protocol family not supported */
#define	E_UNIX_EAFNOSUPPORT	47	/* Address family not supported by protocol family */
#define	E_UNIX_EADDRINUSE	48	/* Address already in use */
#define	E_UNIX_EADDRNOTAVAIL	49	/* Can't assign requested address */

	/* operational errors */
#define	E_UNIX_ENETDOWN		50	/* Network is down */
#define	E_UNIX_ENETUNREACH	51	/* Network is unreachable */
#define	E_UNIX_ENETRESET	52	/* Network dropped connection on reset */
#define	E_UNIX_ECONNABORTED	53	/* Software caused connection abort */
#define	E_UNIX_ECONNRESET	54	/* Connection reset by peer */
#define	E_UNIX_ENOBUFS		55	/* No buffer space available */
#define	E_UNIX_EISCONN		56	/* Socket is already connected */
#define	E_UNIX_ENOTCONN		57	/* Socket is not connected */
#define	E_UNIX_ESHUTDOWN	58	/* Can't send after socket shutdown */
#define	E_UNIX_ETOOMANYREFS	59	/* Too many references: can't splice */
#define	E_UNIX_ETIMEDOUT	60	/* Connection timed out */
#define	E_UNIX_ECONNREFUSED	61	/* Connection refused */

	/* */
#define	E_UNIX_ELOOP		62	/* Too many levels of symbolic links */
#define	E_UNIX_ENAMETOOLONG	63	/* File name too long */

/* should be rearranged */
#define	E_UNIX_EHOSTDOWN	64	/* Host is down */
#define	E_UNIX_EHOSTUNREACH	65	/* No route to host */
#define	E_UNIX_ENOTEMPTY	66	/* Directory not empty */

/* quotas & mush */
#ifdef	EPROCLIM
#define	E_UNIX_EPROCLIM		67	/* Too many processes */
#endif
#define	E_UNIX_EUSERS		68	/* Too many users */
#ifdef	EDQUOT
#define	E_UNIX_EDQUOT		69	/* Disc quota exceeded */
#endif

/* Network File System */
#define	E_UNIX_ESTALE		70	/* Stale NFS file handle */
#define	E_UNIX_EREMOTE		71	/* Too many levels of remote in path */

/* streams */
#define	E_UNIX_ENOSTR		72	/* Device is not a stream */
#define	E_UNIX_ETIME		73	/* Timer expired */
#define	E_UNIX_ENOSR		74	/* Out of streams resources */
#define	E_UNIX_ENOMSG		75	/* No message of desired type */
#define	E_UNIX_EBADMSG		76	/* Trying to read unreadable message */

/* SystemV IPC */
#define	E_UNIX_EIDRM		77	/* Identifier removed */

/* SystemV Record Locking */
#define	E_UNIX_EDEADLK		78	/* Deadlock condition. */
#define	E_UNIX_ENOLCK		79	/* No record locks available. */

/* RFS */
#define	E_UNIX_ENONET		80	/* Machine is not on the network */
#ifdef	ERREMOTE
#define	E_UNIX_ERREMOTE		81	/* Object is remote */
#endif
#define	E_UNIX_ENOLINK		82	/* the link has been severed */
#define	E_UNIX_EADV		83	/* advertise error */
#define	E_UNIX_ESRMNT		84	/* srmount error */
#define	E_UNIX_ECOMM		85	/* Communication error on send */
#define	E_UNIX_EPROTO		86	/* Protocol error */
#define	E_UNIX_EMULTIHOP	87	/* multihop attempted */
#ifdef	EDOTDOT
#define	E_UNIX_EDOTDOT		88	/* Cross mount point (not an error) */
#endif
#define	E_UNIX_EREMCHG		89	/* Remote address changed */

/* POSIX */
#define	E_UNIX_ENOSYS		90	/* function not implemented */

/* XENIX has 135 - 142 */

/* Multi-byte character library problems */
#define	E_UNIX_EILSEQ		188	/* Illegal byte sequence. */

/*
 * Error codes in Solaris
 */
/* Error codes */
#ifdef	ECHRNG
#define	E_UNIX_ECHRNG		190	/* Channel number out of range	*/
#endif
#ifdef	EL2NSYNC
#define	E_UNIX_EL2NSYNC 	191	/* Level 2 not synchronized	*/
#endif
#ifdef	EL3HLT
#define	E_UNIX_EL3HLT		192	/* Level 3 halted		*/
#endif
#ifdef	EL3RST
#define	E_UNIX_EL3RST		193	/* Level 3 reset		*/
#endif
#ifdef	ELNRNG
#define	E_UNIX_ELNRNG		194	/* Link number out of range	*/
#endif
#ifdef	EUNATCH
#define	E_UNIX_EUNATCH		195	/* Protocol driver not attached	*/
#endif
#ifdef	ENOCSI
#define	E_UNIX_ENOCSI		196	/* No CSI structure available	*/
#endif
#ifdef	EL2HLT
#define	E_UNIX_EL2HLT		197	/* Level 2 halted		*/
#endif

/* Convergent Error Returns */
#ifdef	EBADE
#define	E_UNIX_EBADE		198	/* invalid exchange		*/
#endif
#ifdef	EBADR
#define	E_UNIX_EBADR		199	/* invalid request descriptor	*/
#endif
#ifdef	EXFULL
#define	E_UNIX_EXFULL		200	/* exchange full		*/
#endif
#ifdef	ENOANO
#define	E_UNIX_ENOANO		201	/* no anode			*/
#endif
#ifdef	EBADRQC
#define	E_UNIX_EBADRQC		202	/* invalid request code		*/
#endif
#ifdef	EBADSLT
#define	E_UNIX_EBADSLT		203	/* invalid slot			*/
#endif
#ifdef	EDEADLOCK
#define	E_UNIX_EDEADLOCK 	204	/* file locking deadlock error	*/
#endif

#ifdef	EBFONT
#define	E_UNIX_EBFONT		205	/* bad font file fmt		*/
#endif

/* stream problems */
#ifdef	ENODATA
#define	E_UNIX_ENODATA		206	/* no data (for no delay io)	*/
#endif
#ifdef	ENOPKG
#define	E_UNIX_ENOPKG		207	/* Package not installed	*/
#endif
#ifdef	EOVERFLOW
#define	E_UNIX_EOVERFLOW 	208	/* value too large to be stored in data type */
#endif
#ifdef	ENOTUNIQ
#define	E_UNIX_ENOTUNIQ 	209	/* given log. name not unique	*/
#endif
#ifdef	EBADFD
#define	E_UNIX_EBADFD		210	/* f.d. invalid for this operation */
#endif

/* shared library problems */
#ifdef	ELIBACC
#define	E_UNIX_ELIBACC		211	/* Can't access a needed shared lib. */
#endif
#ifdef	ELIBBAD
#define	E_UNIX_ELIBBAD		212	/* Accessing a corrupted shared lib. */
#endif
#ifdef	ELIBSCN
#define	E_UNIX_ELIBSCN		213	/* .lib section in a.out corrupted. */
#endif
#ifdef	ELIBMAX
#define	E_UNIX_ELIBMAX		214	/* Attempting to link in too many libs. */
#endif
#ifdef	ELIBEXEC
#define	E_UNIX_ELIBEXEC 	215	/* Attempting to exec a shared library. */
#endif
#ifdef	ERESTART
#define	E_UNIX_ERESTART 	216	/* Restartable system call	*/
#endif
#ifdef	ESTRPIPE
#define	E_UNIX_ESTRPIPE 	217	/* if pipe/FIFO, don't sleep in stream head */
#endif

#endif /* _NEO_UNIX_ERRNO_ */
