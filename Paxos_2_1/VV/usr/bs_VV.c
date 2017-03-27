/*
 * Synchronous I/O file backing store routine
 *
 * Copyright (C) 2006-2007 FUJITA Tomonori <tomof@acm.org>
 * Copyright (C) 2006-2007 Mike Christie <michaelc@cs.wisc.edu>
 * Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/fs.h>
#include <sys/epoll.h>

#include "list.h"
#include "util.h"
#include "tgtd.h"
#include "scsi.h"
#include "spc.h"
#include "bs_thread.h"
#include "target.h"

#include "driver.h"
#include	"bs_VV.h"

static void set_VV_write_error(int *result, uint8_t *key, uint16_t *asc)
{
	*result = SAM_STAT_CHECK_CONDITION;
	*key = HARDWARE_ERROR;
	*asc = ASC_INTERNAL_TGT_FAILURE;
eprintf("*result=%x *key=%x *acs=%x\n", *result, *key, *asc );
}

static void set_medium_error(int *result, uint8_t *key, uint16_t *asc)
{
	*result = SAM_STAT_CHECK_CONDITION;
	*key = MEDIUM_ERROR;
	*asc = ASC_READ_ERROR;
}

static void bs_sync_sync_range(struct scsi_cmd *cmd, uint32_t length,
			       int *result, uint8_t *key, uint16_t *asc)
{
#if 0
	int ret;


	ret = fdatasync(cmd->dev->fd);
	if (ret)
		set_medium_error(result, key, asc);
#endif
}

static void bs_VV_request(struct scsi_cmd *cmd)
{
#if 0
	int ret, fd = cmd->dev->fd;
#else
	int ret;
#endif
	uint32_t length;
	int result = SAM_STAT_GOOD;
	uint8_t key;
	uint16_t asc;
//	uint32_t info = 0;
	char *tmpbuf;
	size_t blocksize;
	uint64_t offset = cmd->offset;
	uint32_t tl     = cmd->tl;
	int do_verify = 0;
	int i;
	char *ptr;
	const char *write_buf = NULL;
	ret = length = 0;
	key = asc = 0;

	struct scsi_lu	*pLu = cmd->dev;
	void			*pVV = pLu->xxc_p;

	VV_LockR();

	switch (cmd->scb[0])
	{
	case ORWRITE_16:
		length = scsi_get_out_length(cmd);

		tmpbuf = malloc(length);
		if (!tmpbuf) {
			result = SAM_STAT_CHECK_CONDITION;
			key = HARDWARE_ERROR;
			asc = ASC_INTERNAL_TGT_FAILURE;
			break;
		}

//		ret = pread64(fd, tmpbuf, length, offset);
		ret = VL_read( pVV, (char*)tmpbuf, (size_t)length, (off_t)offset );

		if (ret != length) {
			set_medium_error(&result, &key, &asc);
			free(tmpbuf);
			break;
		}

		ptr = scsi_get_out_buffer(cmd);
		for (i = 0; i < length; i++)
			ptr[i] |= tmpbuf[i];

		free(tmpbuf);

		write_buf = scsi_get_out_buffer(cmd);
		goto write;
	case COMPARE_AND_WRITE:
		/* Blocks are transferred twice, first the set that
		 * we compare to the existing data, and second the set
		 * to write if the compare was successful.
		 */
		length = scsi_get_out_length(cmd) / 2;
		if (length != cmd->tl) {
			result = SAM_STAT_CHECK_CONDITION;
			key = ILLEGAL_REQUEST;
			asc = ASC_INVALID_FIELD_IN_CDB;
			break;
		}

		tmpbuf = malloc(length);
		if (!tmpbuf) {
			result = SAM_STAT_CHECK_CONDITION;
			key = HARDWARE_ERROR;
			asc = ASC_INTERNAL_TGT_FAILURE;
			break;
		}

//		ret = pread64(fd, tmpbuf, length, offset);
		ret = VL_read( pVV, (char*)tmpbuf, (size_t)length, (off_t)offset );

		if (ret != length) {
			set_medium_error(&result, &key, &asc);
			free(tmpbuf);
			break;
		}

		if (memcmp(scsi_get_out_buffer(cmd), tmpbuf, length)) {
			uint32_t pos = 0;
			char *spos = scsi_get_out_buffer(cmd);
			char *dpos = tmpbuf;

			/*
			 * Data differed, this is assumed to be 'rare'
			 * so use a much more expensive byte-by-byte
			 * comparasion to find out at which offset the
			 * data differs.
			 */
			for (pos = 0; pos < length && *spos++ == *dpos++;
			     pos++)
				;
//			info = pos;
			result = SAM_STAT_CHECK_CONDITION;
			key = MISCOMPARE;
			asc = ASC_MISCOMPARE_DURING_VERIFY_OPERATION;
			free(tmpbuf);
			break;
		}

#if 0
		if (cmd->scb[1] & 0x10)
			posix_fadvise(fd, offset, length,
				      POSIX_FADV_NOREUSE);
#endif

		free(tmpbuf);

		write_buf = scsi_get_out_buffer(cmd) + length;
		goto write;
	case SYNCHRONIZE_CACHE:
	case SYNCHRONIZE_CACHE_16:
		/* TODO */
		length = (cmd->scb[0] == SYNCHRONIZE_CACHE) ? 0 : 0;

		if (cmd->scb[1] & 0x2) {
			result = SAM_STAT_CHECK_CONDITION;
			key = ILLEGAL_REQUEST;
			asc = ASC_INVALID_FIELD_IN_CDB;
		} else
			bs_sync_sync_range(cmd, length, &result, &key, &asc);
		break;
	case WRITE_VERIFY:
	case WRITE_VERIFY_12:
	case WRITE_VERIFY_16:
		do_verify = 1;
	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
		length = scsi_get_out_length(cmd);
		write_buf = scsi_get_out_buffer(cmd);
write:
//		ret = pwrite64(fd, write_buf, length, offset);
		ret = VL_write( pVV, (char*)write_buf, (size_t)length, (off_t)offset );
		if (ret == length) {
			struct mode_pg *pg;

			/*
			 * it would be better not to access to pg
			 * directy.
			 */
			pg = find_mode_page(cmd->dev, 0x08, 0);
			if (pg == NULL) {
				result = SAM_STAT_CHECK_CONDITION;
				key = ILLEGAL_REQUEST;
				asc = ASC_INVALID_FIELD_IN_CDB;
				break;
			}
			if (((cmd->scb[0] != WRITE_6) && (cmd->scb[1] & 0x8)) ||
			    !(pg->mode_data[0] & 0x04))
				bs_sync_sync_range(cmd, length, &result, &key,
						   &asc);
		} else
			set_VV_write_error(&result, &key, &asc);

#if 0
		if ((cmd->scb[0] != WRITE_6) && (cmd->scb[1] & 0x10))
			posix_fadvise(fd, offset, length,
				      POSIX_FADV_NOREUSE);
#endif
		if (do_verify)
			goto verify;
		break;
	case WRITE_SAME:
	case WRITE_SAME_16:
		/* WRITE_SAME used to punch hole in file */

	{
		uint32_t vl_tl     = tl;
		off_t vl_offset = offset;
		char *vl_buf = malloc(vl_tl);
		char *pbuf = vl_buf;

		if (pbuf == NULL) {
			set_VV_write_error(&result, &key, &asc);
			break;
		}

		if (cmd->scb[1] & 0x08) {
#if 0
			ret = unmap_file_region(fd, offset, tl);
			if (ret != 0) {
				eprintf("Failed to punch hole for WRITE_SAME"
					" command\n");
				result = SAM_STAT_CHECK_CONDITION;
				key = HARDWARE_ERROR;
				asc = ASC_INTERNAL_TGT_FAILURE;
				break;
			}
			break;
#else
			memset(vl_buf, 0, vl_tl);
#endif
		}
		else {
			while (tl > 0) {
				blocksize = 1 << cmd->dev->blk_shift;
				tmpbuf = scsi_get_out_buffer(cmd);

				switch(cmd->scb[1] & 0x06) {
				case 0x02: /* PBDATA==0 LBDATA==1 */
					put_unaligned_be32(offset, tmpbuf);
					break;
				case 0x04: /* PBDATA==1 LBDATA==0 */
					/* physical sector format */
					put_unaligned_be64(offset, tmpbuf);
					break;
				}

//			ret = pwrite64(fd, tmpbuf, blocksize, offset);
#if 0
				ret = VL_write( pVV, (char*)tmpbuf, blocksize, offset );
				if (ret != blocksize)
					set_VV_write_error(&result, &key, &asc);
#else
				memcpy( pbuf, tmpbuf, blocksize );
				pbuf += blocksize;
#endif

				offset += blocksize;
				tl     -= blocksize;
			}
		}
		ret = VL_write( pVV, (char*)vl_buf, (size_t)vl_tl, (off_t)vl_offset );
		if (ret != vl_tl)
			set_VV_write_error(&result, &key, &asc);
		free(vl_buf);
		break;
	}
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
		length = scsi_get_in_length(cmd);
//		ret = pread64(fd, scsi_get_in_buffer(cmd), length,
//			      offset);
		ret = VL_read( pVV, scsi_get_in_buffer(cmd), (size_t)length, (off_t)offset );

		if (ret != length)
			set_medium_error(&result, &key, &asc);

#if 0
		if ((cmd->scb[0] != READ_6) && (cmd->scb[1] & 0x10))
			posix_fadvise(fd, offset, length,
				      POSIX_FADV_NOREUSE);
#endif

		break;
	case PRE_FETCH_10:
	case PRE_FETCH_16:
#if 0
		ret = posix_fadvise(fd, offset, cmd->tl,
				POSIX_FADV_WILLNEED);

		if (ret != 0)
			set_medium_error(&result, &key, &asc);
#endif
		break;
	case VERIFY_10:
	case VERIFY_12:
	case VERIFY_16:
verify:
		length = scsi_get_out_length(cmd);

		tmpbuf = malloc(length);
		if (!tmpbuf) {
			result = SAM_STAT_CHECK_CONDITION;
			key = HARDWARE_ERROR;
			asc = ASC_INTERNAL_TGT_FAILURE;
			break;
		}

//		ret = pread64(fd, tmpbuf, length, offset);
		ret = VL_read( pVV, (char*)tmpbuf, (size_t)length, (off_t)offset );

		if (ret != length)
			set_medium_error(&result, &key, &asc);
		else if (memcmp(scsi_get_out_buffer(cmd), tmpbuf, length)) {
			result = SAM_STAT_CHECK_CONDITION;
			key = MISCOMPARE;
			asc = ASC_MISCOMPARE_DURING_VERIFY_OPERATION;
		}

#if 0
		if (cmd->scb[1] & 0x10)
			posix_fadvise(fd, offset, length,
				      POSIX_FADV_NOREUSE);
#endif

		free(tmpbuf);
		break;
	case UNMAP:
		if (!cmd->dev->attrs.thinprovisioning) {
			result = SAM_STAT_CHECK_CONDITION;
			key = ILLEGAL_REQUEST;
			asc = ASC_INVALID_FIELD_IN_CDB;
			break;
		}

		length = scsi_get_out_length(cmd);
		tmpbuf = scsi_get_out_buffer(cmd);

		if (length < 8)
			break;

		length -= 8;
		tmpbuf += 8;

		while (length >= 16) {
			offset = get_unaligned_be64(&tmpbuf[0]);
			offset = offset << cmd->dev->blk_shift;

			tl = get_unaligned_be32(&tmpbuf[8]);
			tl = tl << cmd->dev->blk_shift;

			if (offset + tl > cmd->dev->size) {
				eprintf("UNMAP beyond EOF\n");
				result = SAM_STAT_CHECK_CONDITION;
				key = ILLEGAL_REQUEST;
				asc = ASC_LBA_OUT_OF_RANGE;
				break;
			}

			if (tl > 0) {
#if 0
				if (unmap_file_region(fd, offset, tl) != 0) {
					eprintf("Failed to punch hole for"
						" UNMAP at offset:%" PRIu64
						" length:%d\n",
						offset, tl);
					result = SAM_STAT_CHECK_CONDITION;
					key = HARDWARE_ERROR;
					asc = ASC_INTERNAL_TGT_FAILURE;
					break;
				}
#else
				blocksize = 1 << cmd->dev->blk_shift;
				tmpbuf = malloc(blocksize);
				memset(tmpbuf, 0, blocksize);
				while (tl > 0) {
					ret = VL_write( pVV, (char*)tmpbuf, (size_t)blocksize, (off_t)offset );
					if (ret != blocksize) {
						result = SAM_STAT_CHECK_CONDITION;
						key = HARDWARE_ERROR;
						asc = ASC_INTERNAL_TGT_FAILURE;
						break;
					}

					offset += blocksize;
					tl     -= blocksize;
				}
				free(tmpbuf);
#endif
			}

			length -= 16;
			tmpbuf += 16;
		}
		break;
	default:
		break;
	}
	VV_Unlock();

	dprintf("io done %p %x %d %u\n", cmd, cmd->scb[0], ret, length);

	scsi_set_result(cmd, result);

	if (result != SAM_STAT_GOOD) {
		eprintf("io error %p %x %d %d %" PRIu64 ", %m\n",
			cmd, cmd->scb[0], ret, length, offset);
		sense_data_build(cmd, key, asc);
	}
}

/*
 *	dtd_load_unload(target.c)
 * (target id,lun)->(pTarget,pLu)
 *	INPUT: lu, path
 *	OUTPUT: *fd, *size
 */
static int bs_VL_open(struct scsi_lu *lu, char *path, int *fd, uint64_t *size)
{
	int Ret;

	Ret = VL_open( lu->tgt->name, path, &lu->xxc_p, size );
	if( Ret < 0 )	*fd	= -1;
	else			*fd	= 0;

	return( Ret );
}

static void bs_VL_close(struct scsi_lu *lu)
{
	VL_close(lu->xxc_p);
}


extern void	VVLogDump(void);
tgtadm_err	(*fShow)(int,int,uint64_t,uint32_t,uint64_t,struct concat_buf*);
tgtadm_err
VV_show( int mode, int tid, 
		uint64_t sid, uint32_t cid, uint64_t lun, struct concat_buf *pBuf )
{
//eprintf("###VV_show(%p)###\n", fShow );
	VVLogDump();
	if( fShow )	return (*fShow)(mode,tid,sid,cid,lun,pBuf);
	else	return( TGTADM_SUCCESS );
}

static tgtadm_err bs_VL_init(struct scsi_lu *lu)
{
	struct bs_thread_info *info = BS_THREAD_I(lu);
	int	ind;
	struct tgt_driver *pDriver;

	ind = get_driver_index( "iscsi" );
	pDriver	= tgt_drivers[ind];
	fShow	= pDriver->show;
	pDriver->show = VV_show;

	if( VL_init( lu->tgt->name, WORKER_MAX_IO ) )	goto err;

if( nr_iothreads > WORKER_MAX_TGTD )
	nr_iothreads = WORKER_MAX_TGTD;

	return bs_thread_open(info, bs_VV_request, nr_iothreads);
err:
	return( TGTADM_UNKNOWN_ERR );
}

static void bs_VL_exit(struct scsi_lu *lu)
{
	struct bs_thread_info *info = BS_THREAD_I(lu);

	bs_thread_close(info);

	VL_exit();
}

static struct backingstore_template VV_bst = {
	.bs_name		= "VV",
	.bs_datasize	= sizeof(struct bs_thread_info),
	.bs_open		= bs_VL_open,
	.bs_close		= bs_VL_close,
	.bs_init		= bs_VL_init,
	.bs_exit		= bs_VL_exit,
	.bs_cmd_submit		= bs_thread_cmd_submit,
	.bs_oflags_supported    = O_SYNC | O_DIRECT,
};

__attribute__((constructor)) static void bs_VV_constructor(void)
{
	register_backingstore_template(&VV_bst);
	VV_init();
}
__attribute__((destructor)) static void bs_VV_destructor(void)
{
	VV_exit();
}
