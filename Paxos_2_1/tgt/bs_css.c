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

#include <pthread.h>

#include "list.h"
#include "util.h"
#include "tgtd.h"
#include "scsi.h"
#include "spc.h"
#include "bs_thread.h"

#include "paxos_css.h"

#define MAX_LUN (8)


#define FUNC_ENTER dprintf("enter.\n")
#define FUNC_EXIT dprintf("%s exit.\n",__FUNCTION__)
#define FUNC_RETURN_INT( rtn ) dprintf("%s exit(%d,0x%x).\n",__FUNCTION__,rtn,rtn)

typedef struct {
	char volname[PATH_MAX +1];
	off_t vol_size;
	size_t  file_size;
} vol_info_t;
typedef struct {
	char filename[PATH_MAX +1];
    int isExist;
	size_t  size;
} exist_info_t;


static int css_fd_index = 0;//LUN No.
#ifdef CSS_MULTI_SESSION 
static void **pSession[MAX_LUN];
static void **pFile[MAX_LUN];
static void ***pVf[MAX_LUN];
#else
static void *pSession_P = NULL;
static void *pFile_P[MAX_LUN];
static void **pVf_P[MAX_LUN];
#endif
static exist_info_t *fileExist[MAX_LUN];
static vol_info_t bs_css_volume[MAX_LUN];

//static pthread_mutex_t mutex;

#ifdef PAXOS_SESSION_SYNC
int paxos_session_sync = 1;
#else
int paxos_session_sync = 0;
#endif

static int get_thread_index(pthread_t thread_table[]);
static vol_info_t getsize(void *pS,char *name);
static size_t getfp(int lun, int thread, off_t *offset, void **ppFp);
static int bs_css_read_write(int lun, int thread, void *buf, off_t offset, int len, int isWrite);
static int bs_css_write(int lun, int thread, void *buf, off_t offset, int len);
static int bs_css_read(int lun, int thread, void *buf, off_t offset, int len);

static void set_medium_error(int *result, uint8_t *key, uint16_t *asc)
{
	*result = SAM_STAT_CHECK_CONDITION;
	*key = MEDIUM_ERROR;
	*asc = ASC_READ_ERROR;
}

static void bs_sync_sync_range(struct scsi_cmd *cmd, uint32_t length,
			       int *result, uint8_t *key, uint16_t *asc)
{
	int ret = 0;

	//ret = fdatasync(cmd->dev->fd);
	if (ret)
		set_medium_error(result, key, asc);
}

static void bs_css_request(struct scsi_cmd *cmd)
{
	int ret, fd = cmd->dev->fd;
	uint32_t length;
	int result = SAM_STAT_GOOD;
	uint8_t key;
	uint16_t asc;
	uint32_t info = 0;
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

	struct scsi_lu *lu = cmd->dev;
	struct bs_thread_info *th_info = BS_THREAD_I(lu);

	int index = fd;
	int thread_index = get_thread_index(th_info->worker_thread);

	FUNC_ENTER;

	dprintf("cmd code[%d]\n",cmd->scb[0]);

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

		ret = bs_css_read(index, thread_index, tmpbuf,
			offset, length);
		//ret = pread64(fd, tmpbuf, length, offset);

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

		ret = bs_css_read(index, thread_index, tmpbuf,
			offset, length);
		//ret = pread64(fd, tmpbuf, length, offset);

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
			info = pos;
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
		ret = bs_css_write(index, thread_index, (void *)write_buf,
                           offset, length);
		//ret = pwrite64(fd, write_buf, length,
		//	       offset);
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
			set_medium_error(&result, &key, &asc);

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
#endif
			break;
		}
		while (tl > 0) {
			blocksize = 1 << cmd->dev->blk_shift;
			tmpbuf = scsi_get_out_buffer(cmd);
			dprintf("blocksize[%ld],blk_shift[%d],tl[%d]\n",
				blocksize,cmd->dev->blk_shift,tl);

			switch(cmd->scb[1] & 0x06) {
			case 0x02: /* PBDATA==0 LBDATA==1 */
				put_unaligned_be32(offset, tmpbuf);
				break;
			case 0x04: /* PBDATA==1 LBDATA==0 */
				/* physical sector format */
				put_unaligned_be64(offset, tmpbuf);
				break;
			}

			ret = bs_css_write(index,thread_index,
				tmpbuf, offset, blocksize);
			//ret = pwrite64(fd, tmpbuf, blocksize, offset);
			if (ret != blocksize)
				set_medium_error(&result, &key, &asc);

			offset += blocksize;
			tl     -= blocksize;
		}
		break;
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
		length = scsi_get_in_length(cmd);
		ret = bs_css_read(index, thread_index,
			scsi_get_in_buffer(cmd), offset, length);
		//ret = pread64(fd, scsi_get_in_buffer(cmd), length,
		//	      offset);

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
#else
		ret = 0;
#endif

		if (ret != 0)
			set_medium_error(&result, &key, &asc);
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

		ret = bs_css_read(index, thread_index,
			tmpbuf, offset, length);
		//ret = pread64(fd, tmpbuf, length, offset);

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

#if 0
			if (tl > 0) {
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
			}
#endif

			length -= 16;
			tmpbuf += 16;
		}
		break;
	default:
		break;
	}

	dprintf("io done %p %x %d %u\n", cmd, cmd->scb[0], ret, length);

	scsi_set_result(cmd, result);

	if (result != SAM_STAT_GOOD) {
		eprintf("io error %p %x %d %d %" PRIu64 ", %m\n",
			cmd, cmd->scb[0], ret, length, offset);
		sense_data_build(cmd, key, asc);
	}
	FUNC_EXIT;
}

static int bs_css_open(struct scsi_lu *lu, char *path, int *fd, uint64_t *size)
{
	int index = css_fd_index -1;
#ifdef CSS_MULTI_SESSION
	int i,j;
#else
	int j;
#endif
	size_t dev_unit = 1024*1024*1024;
	int rtn = 0;
	void *pS = NULL;
#ifdef CSS_MULTI_SESSION
	char *cell = getenv("PAXOS_CELL_NAME");
#endif

	FUNC_ENTER;

	dprintf("index[%d],vol[%s]\n",index,path);
	*fd = index;
#ifdef CSS_MULTI_SESSION
	for ( i = 0; i < nr_iothreads; i++ ) {
		struct Session *pS = pSession[index][i];
		if( (pFile[index][i] = css_open(pS,path,0)) == NULL ) {
			eprintf("css_open(%s) error.\n",path);
			rtn = -1;
			goto err;
		}
		dprintf("css_open[%d][%d] volume[%s] pSession[%p] pF[%p]\n",
			index,i,path,pS,pFile[index][i]);
	}
	if ((pS= paxos_session_open(cell, MAX_LUN*nr_iothreads,
			paxos_session_sync))==NULL ) {
		eprintf("paxos_session_open() error.\n");
		rtn = -1;
		goto err;
	}
	dprintf("paxos_session_open(%d) -> %p\n",MAX_LUN*nr_iothreads,pS);
#else
#if 0
	if( (pFile_P[index] = css_open(pSession_P,path,0)) == NULL ) {
		eprintf("css_open(%s) error.\n",path);
		rtn = -1;
		goto err;
	}
#endif
	pS = pSession_P;
#endif
	dprintf("check volume\n");
	struct stat st;
	st.st_size = 0;

	int ret = css_stat(pS,path,&st);
	dprintf("st_mode[0x%06x],id directory[%d]\n",
		st.st_mode,S_ISDIR(st.st_mode));
	//if ( css_stat(pS,path,&st) ) {
	if ( ret ) {
		dprintf("path[%s] not found.\n",path);
		st.st_size = 0;
	} else if(S_ISDIR(st.st_mode)) {
		dprintf("dirctroy volume.\n");
		bs_css_volume[index]= getsize(pS,path);
		int max =  bs_css_volume[index].vol_size / 
	        bs_css_volume[index].file_size;
		// multi layers( 64 (6 bits) )
		int layers = 0;
		int value = max;
		while (value) {
			value >>= 6;
			layers++;
		}
		dprintf("max=%d,layers=%d.\n",max,layers);
		if ( (fileExist[index] = 
		      (exist_info_t *)calloc(max,sizeof(exist_info_t))) == NULL ) {
			eprintf("calloc(%lu,%d) error.\n",sizeof(exist_info_t),max);
			goto err;
		}
#ifdef CSS_MULTI_SESSION
		for ( i = 0; i < nr_iothreads; i++ ) {
		    pVf[index][i] = calloc(sizeof(void *),max);
		    if ( pVf[index][i] == NULL ) {
			eprintf("calloc(%lu,%d) error.\n",sizeof(void *),max);
			goto err;
		    }
		}
#else
		pVf_P[index] = calloc(sizeof(void *),max);
		if ( pVf_P[index] == NULL ) {
			eprintf("calloc(%lu,%d) error.\n",sizeof(void *),max);
			goto err;
		}
#endif
		for ( j = 0; j < max; j++ ) {
		    int l;
		    char vfname[PATH_MAX +1];
		    sprintf(vfname,"%s",path);
		    for ( l= 0; l < layers; l++ ) {
			char num[4];
			sprintf(num,"/%02u",(j >> 6*(layers-l-1))&0x3f);
			strcat(vfname,num);
		    }
		    strcpy(fileExist[index][j].filename,vfname);
		    fileExist[index][j].size = bs_css_volume[index].file_size; 
		    if ( css_stat(pS,vfname,&st) == 0 ) {
			if( S_ISDIR(st.st_mode) ) {
			    eprintf("file(%s) is directory.\n",
				    vfname);
				goto err;
			}
			fileExist[index][j].isExist = 1;
		    }
#ifdef CSS_MULTI_SESSION
		    for ( i = 0; i < nr_iothreads; i++ ) {
			struct Session *pSt = pSession[index][i];
			if( ( pVf[index][i][j] = css_open(pSt,vfname,0)) == NULL ) {
			    eprintf("css_open(%s) error.\n",vfname);
			    rtn = -1;
			    goto err;
			}
			dprintf("css_open[%d][%d][%d] volume[%s] pSession[%p] pF[%p]\n",
				index,i,j,vfname,pSt,pVf[index][i][j]);
		    }
#else
		    if( ( pVf_P[index][j] = css_open(pSession_P,vfname,0)) == NULL ) {
			eprintf("css_open(%s) error.\n",vfname);
			rtn = -1;
			goto err;
		    }
		    dprintf("css_open[%d][%d] volume[%s] pSession[%p] pF[%p]\n",
			index,j,vfname,pSession_P,pVf_P[index][j]);
#endif
		}
	
		dprintf("volume is directory(%s)\n",path);
			
	} else {
#ifdef CSS_MULTI_SESSION
#else
		if( (pFile_P[index] = css_open(pSession_P,path,0)) == NULL ) {
			eprintf("css_open(%s) error.\n",path);
			rtn = -1;
			goto err;
		}
#endif
		dprintf("path[%s] is reguler file.\n",path);
		strcpy(bs_css_volume[index].volname,path);
		bs_css_volume[index].vol_size = st.st_size;
		bs_css_volume[index].file_size = st.st_size;
		dprintf("volume is file(%s)\n",path);
	}
// unit 1G ???
	if ( st.st_size > 0 ) {
		*size = bs_css_volume[index].vol_size;
	} else {
		*size = dev_unit*10;
	}
	dprintf("Volume volNo[%d] size[%ld,%ld]\n",
		index, bs_css_volume[index].vol_size,*size);
	if (*fd < 0)
		rtn =  *fd;

 out:
#ifdef CSS_MULTI_SESSION
	if (pS ) {
		paxos_session_close(pS);
		dprintf("paxos_session_close(%p)\n",pS);
	}
#endif
		
	FUNC_RETURN_INT( rtn );
	return rtn;
 err:
#ifdef CSS_MULTI_SESSION
	for (  i = 0; i < nr_iothreads; i++ ) {
		if ( pFile[index][i] ) {
			css_close(pFile[index][i]);
			pFile[index][i] = NULL;
			dprintf("css_close[%d,%d]\n",index,i);
		}
		if (  bs_css_volume[index].file_size > 0 &&
		      pVf[index][i]) {
			int max =  bs_css_volume[index].vol_size / 
				bs_css_volume[index].file_size;
			for ( j = 0; j < max; j++ ) {
				if ( pVf[index][i][j] ) {
					css_close(pVf[index][i][j]);
					dprintf("css_close[%d,%d,%d]\n",
						index,i,j);
				}
			}
			free( pVf[index][j] );
			pVf[index][j]=NULL;
		}
	}
#else
	if ( pFile_P[index] ) {
		css_close(pFile_P[index]);
		pFile_P[index] = NULL;
		dprintf("css_close[%d]\n",index);
	}
	if (  bs_css_volume[index].file_size > 0 &&
	      pVf_P[index]) {
		int max =  bs_css_volume[index].vol_size / 
			bs_css_volume[index].file_size;
		for ( j = 0; j < max; j++ ) {
			if ( pVf_P[index][j] ) {
				css_close(pVf_P[index][j]);
				dprintf("css_close[%d,%d]\n",
					index,j);
			}
			free( pVf_P[index][j] );
			pVf_P[index][j]=NULL;
		}
	}
#endif
	goto out;
}

static void bs_css_close(struct scsi_lu *lu)
{
	int index = lu->fd;
#ifdef CSS_MULTI_SESSION
	int i,j;
#else
	int j;
#endif
	FUNC_ENTER;
	dprintf("fd = %d\n",index);
#ifdef CSS_MULTI_SESSION
	for ( i = 0; i < nr_iothreads; i++ ) {
		if ( pFile[index][i] ) {
			css_close(pFile[index][i]);
			pFile[index][i] = NULL;
			dprintf("css_close[%d,%d]\n",index,i);
		}
		if (  bs_css_volume[index].file_size > 0 &&
		      pVf[index][i]) {
			int max =  bs_css_volume[index].vol_size / 
				bs_css_volume[index].file_size;
			for ( j = 0; j < max; j++ ) {
				if ( pVf[index][i][j] ) {
					css_close(pVf[index][i][j]);
					dprintf("css_close[%d,%d,%d]\n",
						index,i,j);
				}
			}
			free( pVf[index][j] );
			pVf[index][j]=NULL;
		}
	}
#else
	if ( pFile_P[index] ) {
		css_close(pFile_P[index]);
		pFile_P[index] = NULL;
		dprintf("css_close[%d]\n",index);
	}
	if (  bs_css_volume[index].file_size > 0 &&
	      pVf_P[index]) {
		int max =  bs_css_volume[index].vol_size / 
			bs_css_volume[index].file_size;
		for ( j = 0; j < max; j++ ) {
			if ( pVf_P[index][j] ) {

				css_close(pVf_P[index][j]);
				dprintf("css_close[%d,%d]\n",
					index,j);
			}
		}
		free( pVf_P[index][j] );
		pVf_P[index][j]=NULL;
	}
#endif
	FUNC_EXIT;
}

static tgtadm_err bs_css_init(struct scsi_lu *lu, char *bsopts)
{
	struct bs_thread_info *info = BS_THREAD_I(lu);

#ifdef CSS_MULTI_SESSION
	int i;
#endif
	tgtadm_err rtn = TGTADM_SUCCESS;
	char *cell = getenv("PAXOS_CELL_NAME");

	FUNC_ENTER;
	//pthread_mutex_init(&mutex,NULL);
	dprintf("cell[%s], index[%d]\n",cell, css_fd_index);
	if ( css_fd_index >= MAX_LUN ) {
		rtn = TGTADM_NOMEM;
		eprintf("to may devices(%d) error.\n",css_fd_index);
		goto out;
	}
#ifdef CSS_MULTI_SESSION
	pSession[css_fd_index] = calloc(nr_iothreads,sizeof(void**));
	if ( pSession[css_fd_index] == NULL ) {
		rtn = TGTADM_NOMEM;
		eprintf("pSession alloc error.\n");
		goto out;
	}
	pFile[css_fd_index] = calloc(nr_iothreads,sizeof(void**));
	if ( pFile[css_fd_index] == NULL ) {
		rtn = TGTADM_NOMEM;
		eprintf("pFile alloc error.\n");
		goto out;
	}
	pVf[css_fd_index] = calloc(nr_iothreads,sizeof(void***));
	if ( pVf[css_fd_index] == NULL ) {
		rtn = TGTADM_NOMEM;
		eprintf("pVf alloc error.\n");
		goto out;
	}
	// session open
	for ( i = 0; i < nr_iothreads; i++ ) {
		dprintf("session no(%d)\n",css_fd_index*nr_iothreads+i);
		pSession[css_fd_index][i] = paxos_session_open(cell,
			css_fd_index*nr_iothreads+i,paxos_session_sync);
		if ( pSession[css_fd_index][i] == NULL ) {
			int j = 0;
			while( j < i ) {
				paxos_session_close(pSession[css_fd_index][j]);
				dprintf("paxos_session_close(%p)\n",
					pSession[css_fd_index][j]);
				pSession[css_fd_index][j] = NULL;
				j++;
			}
			eprintf("paxos_session_open(%d) error.\n",
				css_fd_index*nr_iothreads+i);
			switch (errno) {
			default:
				rtn = TGTADM_NO_TARGET;
				break;
			case 0:
				rtn = TGTADM_NO_TARGET;
				break;
			case ENOMEM:
				rtn = TGTADM_NOMEM;
				break;
			}
			goto out;
		}
		dprintf("paxos_session_open(%d) -> %p\n",
			css_fd_index*nr_iothreads+i,
			pSession[css_fd_index][i]);
	}
#else
	pFile_P[css_fd_index] = calloc(1,sizeof(void*));
	if ( pFile_P[css_fd_index] == NULL ) {
		rtn = TGTADM_NOMEM;
		eprintf("pFile alloc error.\n");
		goto out;
	}
	pVf_P[css_fd_index] = calloc(1,sizeof(void**));
	if ( pVf_P[css_fd_index] == NULL ) {
		rtn = TGTADM_NOMEM;
		eprintf("pVf alloc error.\n");
		goto out;
	}
	dprintf("session no(%d)\n",0);
	if ( pSession_P == NULL ) {
		pSession_P = paxos_session_open(cell,0,paxos_session_sync);
		if ( pSession_P == NULL ) {
			eprintf("paxos_session_open error.\n");
			switch (errno) {
			default:
				rtn = TGTADM_NO_TARGET;
				break;
			case 0:
				rtn = TGTADM_NO_TARGET;
				break;
			case ENOMEM:
				rtn = TGTADM_NOMEM;
				break;
			}
			goto out;
		}
	}
#endif
	css_fd_index++;
	rtn =  bs_thread_open(info, bs_css_request, nr_iothreads);
	if ( rtn != TGTADM_SUCCESS ) {
		eprintf("bs_thread_open error.\n");
	}
		
 out:
	FUNC_RETURN_INT( rtn );
	return rtn;
}

static void bs_css_exit(struct scsi_lu *lu)
{
	struct bs_thread_info *info = BS_THREAD_I(lu);

	bs_thread_close(info);
	//pthread_mutex_destroy(&mutex);
}

static struct backingstore_template css_bst = {
	.bs_name		= "css",
	.bs_datasize		= sizeof(struct bs_thread_info),
	.bs_open		= bs_css_open,
	.bs_close		= bs_css_close,
	.bs_init		= bs_css_init,
	.bs_exit		= bs_css_exit,
	.bs_cmd_submit		= bs_thread_cmd_submit,
	.bs_oflags_supported    = O_SYNC | O_DIRECT,
};

__attribute__((constructor)) static void bs_css_constructor(void)
{
	unsigned char sbc_opcodes[] = {
		ALLOW_MEDIUM_REMOVAL,
		COMPARE_AND_WRITE,
		FORMAT_UNIT,
		INQUIRY,
		MAINT_PROTOCOL_IN,
		MODE_SELECT,
		MODE_SELECT_10,
		MODE_SENSE,
		MODE_SENSE_10,
		ORWRITE_16,
		PERSISTENT_RESERVE_IN,
		PERSISTENT_RESERVE_OUT,
		PRE_FETCH_10,
		PRE_FETCH_16,
		READ_10,
		READ_12,
		READ_16,
		READ_6,
		READ_CAPACITY,
		RELEASE,
		REPORT_LUNS,
		REQUEST_SENSE,
		RESERVE,
		SEND_DIAGNOSTIC,
		SERVICE_ACTION_IN,
		START_STOP,
		SYNCHRONIZE_CACHE,
		SYNCHRONIZE_CACHE_16,
		TEST_UNIT_READY,
		UNMAP,
		VERIFY_10,
		VERIFY_12,
		VERIFY_16,
		WRITE_10,
		WRITE_12,
		WRITE_16,
		WRITE_6,
		WRITE_SAME,
		WRITE_SAME_16,
		WRITE_VERIFY,
		WRITE_VERIFY_12,
		WRITE_VERIFY_16
	};
	bs_create_opcode_map(&css_bst, sbc_opcodes, ARRAY_SIZE(sbc_opcodes));
	register_backingstore_template(&css_bst);

}
static int get_thread_index(pthread_t thread_table[])
{
	int thread_index;
	pthread_t my = pthread_self();
	for( thread_index = 0; thread_index < nr_iothreads; thread_index++ ) {
		if ( my == thread_table[thread_index] ) {
			goto out;
		}
	}
	goto err;
 out:
	return thread_index;
 err:
	thread_index = -1;
	goto out;
}
static vol_info_t getsize(void *pS, char *name)
{
	struct stat st;
	struct dirent ent[1024];
	vol_info_t info = {{0},(off_t)-1,(off_t)-1};
	int bcount = sizeof(ent)/sizeof(ent[0]);
	int index = 0;
	int n = 0;
	int isOpen = 0;
	char file_name[PATH_MAX +1];
	char *cell = getenv("PAXOS_CELL_NAME");
	dprintf("getsize(%s) enter\n",name);

	strcpy(info.volname,name);
	if ( pS == NULL ) {
		pS = paxos_session_open(cell, MAX_LUN*nr_iothreads,paxos_session_sync);
		dprintf("paxos_session_open(%d) -> %p\n"
			,MAX_LUN*nr_iothreads,pS);

		isOpen = 1;
	} else {
		dprintf("already open session(%p).\n",pS);
	}
	if ( pS == NULL ) {
		goto out;
	}
	dprintf("start readdir\n");
	while (1) {
		int i;
		int count = css_readdir(pS,name,ent,index,bcount);
		if ( count < 0 ) {
			eprintf("css_readdir(%d,%d) error(%d,%s).\n",
                    index,bcount,errno,strerror(errno));
			break;
		}
		dprintf("%s entry count:%d\n",name,count);
		for ( i = 0; i < count; i++ ) {
			int val;
			if (ent[i].d_name[0] == '.') {
				continue;
			}
			val = strtoul(ent[i].d_name,NULL,10);
			sprintf(file_name,"%s/%02u",name,val);
			if ( css_stat(pS,file_name,&st) != 0 ) {
				eprintf("css_stat(%s) error(%d,%s).\n",
                        file_name,errno,strerror(errno));
				break;
			} else if (S_ISDIR(st.st_mode)) {
				vol_info_t subinfo = getsize(pS, file_name);
				if ( info.file_size == (off_t) -1 ) {
					info.file_size = subinfo.file_size;
				} else if (info.file_size != subinfo.file_size) {
					eprintf("file size is invalid (%ld != %ld).\n",
                            info.file_size,subinfo.file_size);
					break;
				}
				if ( info.vol_size == (off_t)-1) {
					info.vol_size = 0;
				}
				info.vol_size +=  subinfo.vol_size;
				dprintf("file_size(%lu),vol_size(%lu)\n",
                        info.file_size, info.vol_size);
				
			}
			if ( val > n ) {
				n = val;
			}
		}
		if ( count < bcount ) {
			break;
		}
		index += count;
	}
	sprintf(file_name,"%s/%02u",name,n);
	if ( css_stat(pS,file_name,&st) != 0 ) {
	    dprintf("css_stat(%s) err[%d]\n",file_name,errno);
            if ( n == 0 ) {
                // file not found -> default
                info.file_size = 2*1024*1024;
                info.vol_size = 64 * info.file_size;
                dprintf("default file no(%d),file_size(%lu),vol_size(%lu)\n",
                    64,info.file_size, info.vol_size);
            } else {
	        eprintf("css_stat(%s) err[%d] exists in dirctory\n",file_name,errno);
	    }
	} else if (S_ISDIR(st.st_mode)) {
		// nop
	} else {
            if ( n < 63 ) {
                n = 63;
            }
	    info.file_size = st.st_size;
	    info.vol_size = (n +1) * st.st_size;
	    dprintf("max file no(%d),file_size(%lu),vol_size(%lu)\n",
                n,info.file_size, info.vol_size);
	}
 out:
	if (isOpen && pS) {
		paxos_session_close(pS);
		dprintf("paxos_session_close(%p)\n",pS);
	}
	dprintf("getsize(%s)->(%ld,%ld) exit.\n",
            name,info.vol_size,info.file_size);
	return info;
}
static size_t getfp(int lun, int thread, off_t *offset, void **ppFp)
{
	size_t len;
	if ( lun >= css_fd_index ) {
		eprintf("lun(%d) >= lun No(%d).\n",lun,css_fd_index );
		goto err;
	}
	if ( thread >= nr_iothreads ) {
		eprintf("thread(%d) >= thread No(%d).\n",
			thread,nr_iothreads);
		goto err;
	}
	if ( bs_css_volume[lun].vol_size <= *offset ) {
		eprintf("volume(%d) size(%lu) <= request offset(%lu).\n",
                lun,bs_css_volume[lun].vol_size,*offset);
		goto err;
	}
#ifdef CSS_MULTI_SESSION
	if ( pVf[lun][thread] ) {
		int i = *offset / bs_css_volume[lun].file_size;
		*offset = *offset % bs_css_volume[lun].file_size; 
		*ppFp = pVf[lun][thread][i];
		dprintf("directory volume(%u) %u's pFile(%p,%lu).\n",
                lun,i,*ppFp,*offset);
	} else if (pFile[lun][thread]) {
		*ppFp = pFile[lun][thread] ;
		dprintf("file volume(%u)  pFile(%p,%lu).\n",
                lun,*ppFp,*offset);
#else
	if ( pVf_P[lun] ) {
		int i = *offset / bs_css_volume[lun].file_size;
		*offset = *offset % bs_css_volume[lun].file_size; 
		*ppFp = pVf_P[lun][i];
		dprintf("directory volume(%u) %u's pFile(%p,%lu).\n",
                lun,i,*ppFp,*offset);
	} else if ( pFile_P[lun] ){
		*ppFp = pFile_P[lun] ;
		dprintf("file volume(%u)  pFile(%p,%lu).\n",
                lun,*ppFp,*offset);
#endif
	} else {
		eprintf("fp(lun=%d,thread[%d],offset[%lu] is not found.\n",
			lun,thread,*offset);
		goto err;
	}
	len =  bs_css_volume[lun].file_size - *offset;
	dprintf("file length(%lu).\n",len);
 ret:
	return len;
 err:
	*ppFp = NULL;
	len = (size_t)-1;
	goto ret;
}
static int bs_css_read_write(int lun, int thread, void *buf, off_t offset, int len, int isWrite)
{
	int rtn = 0;
	int err_trno = 0;
	void *pF;
	int fileSize = bs_css_volume[lun].file_size;
	char *cell = getenv("PAXOS_CELL_NAME");
	exist_info_t *isExist;
	dprintf("volume(%u) thread(%u) (%lu,%u,%d).\n",
            lun,thread,offset,len,isWrite);
#ifdef CSS_MULTI_SESSION
	if ( pSession[lun][thread] == NULL ) {
		pSession[lun][thread] = paxos_session_open(cell,
				lun*nr_iothreads+thread,paxos_session_sync);
		if (  pSession[lun][thread] == NULL ) {
			eprintf("paxos_session_open() error.\n");
			err_trno = 1;
			goto err;
		}
		dprintf("paxos_session_open(%d,0) -> %p\n",
			lun*nr_iothreads+thread,
			pSession[lun][thread]);
		if ( pVf[lun][thread] ) {
			int i;
			for ( i = 0; 
				i < 
				bs_css_volume[lun].vol_size/
				bs_css_volume[lun].file_size;
			      i++ 				) {
				char *fname = (fileExist[lun] + i)->filename;
				if ( pVf[lun][thread][i] ) {
					css_close(pVf[lun][thread][i]);
				}
				pVf[lun][thread][i] = 
					css_open(pSession[lun][thread],fname,0);
				if (  pVf[lun][thread][i] == NULL ) {
					goto err;
				}
			}
		} else {
			if (pFile[lun][thread]) {
				css_close(pFile[lun][thread]);
			}
			char *fname = fileExist[lun][0].filename;
			pFile[lun][thread] = css_open(pSession[lun][thread],fname,0);
			if ( pFile[lun][thread] == NULL ) {
				goto err;
			}
		}
	}
#else
	if ( pSession_P == NULL ) {
		pSession_P = paxos_session_open(cell,0,paxos_session_sync);
		if (  pSession_P == NULL ) {
			eprintf("paxos_session_open() error.\n");
			err_trno = 1;
			goto err;
		}
		dprintf("paxos_session_open -> %p\n",
			pSession_P);
		if ( pVf_P[lun] ) {
			int i;
			for ( i = 0; 
				i < 
				bs_css_volume[lun].vol_size/
				bs_css_volume[lun].file_size;
			      i++ 				) {
				char *fname = (fileExist[lun] + i)->filename;
				if ( pVf_P[lun][i] ) {
					css_close(pVf_P[lun]);
				}
				pVf_P[lun][i] = 
					css_open(pSession_P,fname,0);
				if (  pVf_P[lun][i] == NULL ) {
					goto err;
				}
			}
		} else {
			if (pFile_P[lun]) {
				css_close(pFile_P[lun]);
			}
			char *fname = fileExist[lun][0].filename;
			pFile_P[lun] = css_open(pSession_P,fname,0);
			if ( pFile_P[lun] == NULL ) {
				goto err;
			}
		}
	}
#endif
	while ( len > 0) {
		off_t off = offset + rtn;
		fprintf(stderr,"off [%ld]\n",off);
		size_t l = getfp(lun,thread,&off,&pF);
		ssize_t ll = 0;
		if ( l == (size_t)-1 || pF == NULL ) {
			eprintf("l(%lu) or pF(%p) err.\n",l,pF);
			fprintf(stderr,"l(%lu) or pF(%p) err.\n",l,pF);
			fflush(stderr);
			abort();
			err_trno = 2;
			goto err;
		}
		isExist = fileExist[lun] + (offset+rtn)/fileSize;
		l = (l < len)?l:len;
		dprintf("pF(%p,%lu,%lu,%d)\n",
                pF,off,l,isWrite);
		if ( isWrite ) {
#ifdef DEBUG_ZERO_LENGTH
			if ( off == 0 ) {
				size_t ii;
				for (ii=0;ii<l;ii++) {
					if ( ((char *)buf)[ii] != '\0' ) {
						break;
					}
				}
				dprintf("write zero length[%ld]\n",ii);
			}
#endif
#ifdef CSS_MULTI_SESSION
 		    ll = css_locked_write(pF,buf,off,l,bs_css_volume[lun].volname);
		    if ( ll == l && isExist->isExist == 0 ) {
                    	if ( css_truncate(pSession[lun][thread],
                                  isExist->filename,fileSize) ) {
				eprintf("PFS file(%s,%u) truncate error(%d,%s).\n",
				isExist->filename,fileSize,errno,strerror(errno));
				err_trno = 3;
				goto err;
			}
#else
#if 1
 		    ll = css_locked_write(pF,buf,off,l,isExist->filename);
#else
 		    ll = css_write(pF,buf,off,l);
#endif
		    if ( ll == l && isExist->isExist == 0 ) {
				// truncate
			if ( css_truncate(pSession_P,
				  isExist->filename,fileSize) ) {
			    eprintf("PFS file(%s,%u) truncate error(%d,%s).\n",
			    isExist->filename,fileSize,errno,strerror(errno));
			    err_trno = 3;
			    goto err;
			}
#endif
			isExist->isExist = 1;
		    }
		} else {
			if ( isExist->isExist == 0 ) {
				memset(buf,'\0',l);
				ll = l;
			} else {
#ifdef CSS_MULTI_SESSION
				ll = css_locked_read(pF,buf,off,l,bs_css_volume[lun].volname);
#else
#if 1
				ll = css_locked_read(pF,buf,off,l,isExist->filename);
#else
				ll = css_read(pF,buf,off,l);
#endif
#endif
			}
#ifdef DEBUG_ZERO_LENGTH
			if ( off == 0 ) {
				size_t ii;
				for (ii=0;ii<l;ii++) {
					if ( ((char *)buf)[ii] != '\0' ) {
						break;
					}
				}
				dprintf("read zero length[%ld]\n",ii);
			}
#endif
		}			
		if (ll != l ) {
			err_trno = 4;
			goto err;
		}
		buf += ll;
		len -= ll;
		off += ll;
		rtn += ll;
		l -= ll;
	}
 out:
	dprintf("rtn code(%d).\n",rtn);
	return rtn;
 err:
#ifdef CSS_MULTI_SESSION
	if ( pSession[lun][thread] ) {
		if ( pVf[lun][thread] ) {
			int i;
			for ( i = 0; 
				i < 
				bs_css_volume[lun].vol_size/
				bs_css_volume[lun].file_size;
			      i++ 				) {
				css_close(pVf[lun][thread][i]);
				pVf[lun][thread][i] = NULL;
			}
		} else if (pFile[lun][thread]) {
			css_close(pFile[lun][thread]);
			pFile[lun][thread] = NULL;
		} else {
			abort();
		}
		paxos_session_close(pSession[lun][thread]);
		dprintf("paxos_session_close(%p)\n",
			pSession[lun][thread]);
		pSession[lun][thread] = NULL;
	}
#else
	if ( pSession_P ) {
		if ( pVf_P[lun] ) {
			int i;
			for ( i = 0; 
				i < 
				bs_css_volume[lun].vol_size/
				bs_css_volume[lun].file_size;
			      i++ 				) {
				css_close(pVf_P[lun][i]);
				pVf_P[lun][i] = NULL;
			}
		} else if (pFile_P[lun]) {
			css_close(pFile_P[lun]);
			pFile_P[lun] = NULL;
		} else {
			abort();
		}
		paxos_session_close(pSession_P);
		dprintf("paxos_session_close(%p)\n",
			pSession_P);
		pSession_P = NULL;
	}
#endif
	eprintf("err(%d) exit\n",err_trno);
	rtn = -1;
	goto out;
}
static int bs_css_write(int lun, int thread, void *buf, off_t offset, int len)
{
	//pthread_mutex_lock(&mutex);
	int ret = bs_css_read_write(lun,thread,buf,offset,len,1);
	//pthread_mutex_unlock(&mutex);
	return ret;
}
static int bs_css_read(int lun, int thread, void *buf, off_t offset, int len)
{
	//pthread_mutex_lock(&mutex);
	int ret = bs_css_read_write(lun,thread,buf,offset,len,0);
	//pthread_mutex_unlock(&mutex);
	return ret;
}
