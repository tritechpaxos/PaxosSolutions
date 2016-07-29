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

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdio.h>
#include    <dirent.h>

#include	"neo_db.h"
#include	"join.h"
#include	"../sql/sql.h"

int
svc_file( pd, hp )
    p_id_t   *pd;
    p_head_t *hp;
{
    int len;
    int ret;
	r_file_t *reqp;

    DBG_ENT(M_SQL,"svc_file");

DBG_A(("hp->h_len=%d\n", hp->h_len ));

	len = hp->h_len + sizeof(p_head_t);
	if( !(reqp = (r_file_t*)neo_malloc(len) )) {
		goto err1;
	}
	
	if( p_recv( pd, (p_head_t*)reqp, &len ) < 0 ) {
		goto err2;
	}
	
	if( (ret	= prc_file( pd, hp, reqp)) ) {
		goto err2;
	}

	neo_free( reqp );

DBG_B(("ret(0x%x)\n", ret ));
	DBG_EXIT( 0 );
	return( 0 );

err2:	neo_free( reqp );
err1:
	err_reply( pd, hp, neo_errno );

DBG_B(("ret(%s)\n", neo_errsym()));
	DBG_EXIT( neo_errno );
	return( neo_errno );
}


int
prc_file( pd, hp, reqp)
    p_id_t   *pd;
    p_head_t *hp;
    r_file_t *reqp;
{
	DBG_ENT(M_SQL,"prc_file");

    switch( reqp->flag ){
	    case 0:
	        //do nothing
	        break;
	    case 1://recieve and save file
	        if(file_save(pd, hp, reqp)) goto err1;
	        break;
	    case 2://load and send file
	        if(file_send(pd, hp, reqp)) goto err1;
	        break;
	    default:
    	    neo_errno = E_BYTESFILE_ERROR;
    	    goto err1;
	}
	DBG_EXIT( 0 );
    return( 0 );
err1:
    neo_errno = E_BYTESFILE_ERROR;   
    DBG_EXIT( neo_errno );
    return( neo_errno );
}


int
file_save( pd, hp, reqp)
    p_id_t   *pd;
    p_head_t *hp;
    r_file_t *reqp;
{
    
    r_res_sql_t res;
    FILE *ofp;
    int len;
    int ret;
    char *bufferhead;

	DBG_ENT(M_SQL,"file_save");
    
    if ( ( ofp = fopen(reqp->path, "w" ) ) == 0 ) {
        neo_errno = E_BYTESFILE_ERROR;
        goto err1;
	}	

	bufferhead = (char*) (reqp + 1);

    if ( (ret = fwrite( bufferhead, reqp->filelen, 1, ofp )) == 0 ) {
        neo_errno = 2;
        goto err2;
	}
	
    //return packet
    len = sizeof(res);
    reply_head( hp, (p_head_t*)&res, len, 0 );
    if( p_reply( pd, (p_head_t*)&res, len ) ) {
        goto err2;
    }
    ret = fclose( ofp );
	DBG_EXIT( 0 );
    return( 0 );
err2:
    fclose( ofp );
err1:
    neo_errno = E_BYTESFILE_ERROR;   
    DBG_EXIT( neo_errno );
    return( neo_errno );
}

int
file_send( pd, hp, reqp)
    p_id_t   *pd;
    p_head_t *hp;
    r_file_t *reqp;
{
    r_file_t *resp;
    FILE *ofp;
    struct stat statbuf; 
    int filelen;
    int len;
    int ret;
    
	DBG_ENT(M_SQL,"file_send");
    
    if ( ( ofp = fopen(reqp->path, "r" ) ) == 0 ) {
        goto err1;
	}
	
    stat(reqp->path, &statbuf);
    filelen = statbuf.st_size;
    
    len = sizeof(r_file_t) + filelen;
    if( !(resp = (r_file_t*)neo_malloc(len) )) {
		goto err2;
	}
		
    if ( fread( resp + 1, filelen, 1, ofp ) == 0 ){
        goto err3;
    }
    
    //return packet
    resp->flag = reqp->flag;
    bcopy(reqp->path, resp->path, 64);
    resp->filelen = filelen;
    
    reply_head( hp, (p_head_t*)resp, len, 0 );

    if( p_reply( pd, (p_head_t*)resp, len ) ) {
        goto err3;
    }
    ret = fclose( ofp );
    neo_free(resp);
	DBG_EXIT( 0 );
    return( 0 );
err3:
    neo_free(resp);
err2:
    fclose( ofp );
err1:
    neo_errno = E_BYTESFILE_ERROR;   
    DBG_EXIT( neo_errno );
    return( neo_errno );
}

int
file_delete( path )
    char *path;
{
    int ret;
    DBG_ENT(M_SQL,"file_delete");
    
    if((ret = unlink(path)) != 0){
        goto err1;    
    }
    
	DBG_EXIT( 0 );
    return( 0 );
err1:
    neo_errno = E_BYTESFILE_ERROR;  
    DBG_EXIT( neo_errno );
    return( neo_errno );
}

int
file_createtmp(tempfile)
    char *tempfile;
{
    int ret;
    FILE *ofp;
	DBG_ENT(M_SQL,"file_createtmp");

    tempfile = mktemp(tempfile);

	if( tempfile == NULL){
        goto err1;
    }

    if ( ( ofp = fopen(tempfile, "a" ) ) == 0 ) {
        goto err1;
	}
    
    ret = fclose(ofp);
    
    DBG_EXIT( 0 );
    return( 0 );
err1:
    neo_errno = E_BYTESFILE_ERROR;   
    DBG_EXIT( neo_errno );
    return( neo_errno );
}

int
dir_delete( path)
    char *path;
{
    DIR *dp;
    struct dirent *ep;
    struct stat st;
    int ret;
  
    DBG_ENT(M_SQL,"dir_delete");
    
    strcat(path, "/");
    
    dp = opendir (path);
    if (dp != NULL)
    {
      while ((ep = readdir (dp)) )
      {
          char newpath[128];
          strcpy(newpath, path);
          strcat(newpath, ep->d_name);
          ret = stat(newpath, &st);
        if (S_ISDIR(st.st_mode) == 0)
        {
          ret = remove(newpath);
        }
      }
      (void) closedir (dp);
    }
    ret = rmdir(path);
	DBG_EXIT( 0 );
    return( 0 );
}
