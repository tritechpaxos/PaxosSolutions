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
 *	time_stamp.c
 *
 *	説明	各種計測用
 * 
 ******************************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "neo_db.h"

extern	int	neo_set_epilog();

#define MAXTIME 40000

typedef struct {
	int    action;
	int    class;
	int    cmd;
	struct timeval t;
} time_stamp_t;

static int first = 1;
static int times = 0;
static time_stamp_t tlog[MAXTIME];
static struct timeval o_tm;

extern char* _neo_procname;
extern neo_class_sym_t	_neo_class_tbl[];

static char*
getclass( class )
{
	neo_class_sym_t* t = _neo_class_tbl;
	while( t -> c_sym ){
		if( t -> c_class == class ) return t -> c_sym;
		t++;
	}
	return "?????";
}

static char*
db_man_cmd( cmd )
{
	switch( cmd ){
	case R_CMD_LINK	:		return	"R_CMD_LINK";
	case R_CMD_UNLINK:		return	"R_CMD_UNLINK";
	case R_CMD_CREATE:		return	"R_CMD_CREATE";
	case R_CMD_DROP:		return	"R_CMD_DROP";
	case R_CMD_OPEN:		return	"R_CMD_OPEN";
	case R_CMD_CLOSE:		return	"R_CMD_CLOSE";
	case R_CMD_FLUSH:		return	"R_CMD_FLUSH";
	case R_CMD_CANCEL:		return	"R_CMD_CANCEL";
	case R_CMD_INSERT:		return	"R_CMD_INSERT";
	case R_CMD_DELETE:		return	"R_CMD_DELETE";
	case R_CMD_UPDATE:		return	"R_CMD_UPDATE";
	case R_CMD_SEARCH:		return	"R_CMD_SEARCH";
	case R_CMD_FIND:		return	"R_CMD_FIND";
	case R_CMD_REWIND:		return	"R_CMD_REWIND";
	case R_CMD_SORT:		return	"R_CMD_SORT";
	case R_CMD_HOLD:		return	"R_CMD_HOLD";
	case R_CMD_RELEASE:		return	"R_CMD_RELEASE";
	case R_CMD_HOLD_CANCEL:		return	"R_CMD_HOLD_CANCEL";
	case R_CMD_COMPRESS:		return	"R_CMD_COMPRESS";
	case R_CMD_EVENT:		return	"R_CMD_EVENT";
	case R_CMD_EVENT_CANCEL:	return	"R_CMD_EVENT_CANCEL";
	case R_CMD_DIRECT:		return	"R_CMD_DIRECT";
	case R_CMD_STOP:		return	"R_CMD_STOP";
	case R_CMD_RESTART:		return	"R_CMD_RESTART";
	default: 			break;
	}
	return "F_???";
}

static char*
getcmd( class,cmd )
{
	switch( class ){
	case M_RDB: 	return db_man_cmd( cmd );
	default:	break;
	}
	return "???????";
}

static char*
time_diff( dt,st ) struct timeval* dt; struct timeval* st;
{
	struct timeval m;
	int s,us,sec,min,hour;
	static char buf[80];

	m.tv_sec  = dt -> tv_sec  - st -> tv_sec;
	m.tv_usec = dt -> tv_usec - st -> tv_usec;
	if( m.tv_usec < 0 ){
		m.tv_sec--;
		m.tv_usec += 1000000;
	}
	s  = m.tv_sec;
	us = m.tv_usec;
	sec  = s % 60;
	min  = (s/60) % 60;
	hour = s/3600;
	sprintf( buf,"%02d:%02d:%02d.%06d",hour,min,sec,us );
	return buf;
}

/******************************************************************************
 * 関数名 
 * 		output()
 * 機能
 *		記録されている時刻のファイルへの出力
 * 関数値
 *      なし
 * 注意
 ******************************************************************************/
static void output()
{
	if( times > 0 ){

		char buf[256];
		FILE* fp;
		int i;
		static char* proc = 0;
		static int count = 0;
		static int pid = 0;
		static char* p = 0;

		if( pid == 0 ){
			pid = getpid();
			proc = (char*)rindex( _neo_procname,'/' );
			if( proc ) proc++; else proc = _neo_procname;
		}
		if( !p ){
			p = (char*)getenv( "SAVEDIR" );
			if( !p ) p = ".";
		}
		sprintf( buf,"%s/TIME.%s.%d-%d",p,proc,pid,count++ );
		fp = fopen( buf,"w" );
		if( fp ){
fprintf( fp,"#filename: %s\n",buf );
fprintf( fp,"#IO class RPCcommand absolute-time\n" );
fprintf( fp,"#----------------------------------------\n" );
			for( i = 0; i < times; i++ ){
				fprintf( fp,"%s %-6s %-10s ",
					( tlog[i].action == 0 ) ? "-->":"<--",
					getclass( tlog[i].class ),
					getcmd( tlog[i].class,tlog[i].cmd ) );
				fprintf( fp,"%s\n",
					time_diff( &tlog[i].t,&o_tm ) );
			}
			fclose( fp );
		}
		times = 0;
	}
}

/******************************************************************************
 * 関数名 
 * 		time_stamp()
 * 機能
 *		時刻の記録
 * 関数値
 *      なし
 * 注意
 ******************************************************************************/
void
time_stamp( action,class,cmd )
	register int action; 	/* 0 RECV, 1 REPLY 	*/
	register int class;	/* LOG CLASS 		*/
	register int cmd;	/* RPC COMMAND CODE 	*/
{
	register time_stamp_t* t = &tlog[times];

	if( first ){
		neo_signal( NEO_SIGUSR2,output );
		neo_set_epilog( class,output );
	}
	t -> action = action;
	t -> class  = class;
	t -> cmd    = cmd;
	gettimeofday( &t -> t,0 );
	if( first ){
		o_tm = t -> t;
		first = 0;
	}
	times++;
	if( action ) return;
	if( times > MAXTIME - 100 ) output();
}

