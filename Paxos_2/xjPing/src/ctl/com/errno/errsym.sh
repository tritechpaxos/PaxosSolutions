#!/bin/sh

################################################################################
#
# errsym.sh --- Shell Script of xjPing (On Memory DB)  errno create programs
#
# Copyright (C) 2010 triTech Inc. All Rights Reserved.
#
# See GPL-LICENSE.txt for copyright and license details.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version. 
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.
#
################################################################################

awkfile1="/tmp/errsym1.awk"
awkfile2="/tmp/errsym2.awk"
tmpfile="/tmp/unixerr.tmp"

#XJPOS=$2
#if [ ${XJPOS} = "XJPOS_LINUX" ]
#then
#sys_errno_file_1="/usr/include/asm-generic/errno-base.h"
#sys_errno_file_2="/usr/include/asm-generic/errno.h"
#sys_errno_file="$sys_errno_file_1 $sys_errno_file_2"
#else
#sys_errno_file="/usr/include/sys/errno.h"
#fi

# Cygwin
cpp -CC -dD < /usr/include/errno.h > z
#linux
if [ $? -ne 0 ] ; then cpp -C -dD < /usr/include/errno.h > z ; fi	
sys_errno_file="z"

CREATE_FILE="neo_errsym.c"
DIR=`dirname $1`
FILE1=`ls -1 $DIR/*_err.h | awk '$1 !~ /neo_unix_err.h/ {print $1}'`
FILE2=$DIR/neo_unix_err.h

make_awk_script1()
{
        cat > $awkfile1 << END_OF_LINE
{
        s = substr( \$2,1,1 );
        if( s == "E" ){
                printf( "{ %s,\"",\$2 );
                if( NF > 3 ){
                        for( i = 5; i < NF-1; i++ )
                                printf( "%s ",\$i );
                        printf( "%s\" },\n",\$(NF-1) );
                } else {
                        printf( "\" },\n");
                }
        }
}
END_OF_LINE
}

make_awk_script2()
{
        cat > $awkfile2 << END_OF_LINE
{
        s = substr( \$1,1,1 );
        if( s == "E" ){
                printf( "{ %s,\"",\$1 );
                if( NF > 2 ){
                        for( i = 5; i < NF-1; i++ )
                                printf( "%s ",\$i );
                        printf( "%s\" },\n",\$(NF-1) );
                } else {
                        printf( "\" },\n" );
                }
        }
}
END {
        printf( "{-1,0}\n" );
        printf( "};\n\n" );
}
END_OF_LINE
}

make_database_from_neo_err_h()
{
        grep "define" $FILE1 | awk -f $awkfile1 >> $CREATE_FILE
}

make_index_from_errno_h()
{
        grep "define" $sys_errno_file | \
                awk '{ if( NF > 2 ) printf("E_UNIX_%s\n",$2); }' | \
                sort -k 1,2 > $tmpfile
}

make_database_from_neo_unix_errno_h()
{
        grep "define" $FILE2 | \
        awk '{ if( NF > 2 ) print $0; }' | \
        sort -k 2,3 | \
        join -j1 2 - $tmpfile | \
        sort -n -k 3,4 | \
        awk -f $awkfile2 >> $CREATE_FILE
}

procedure()
{
        cat > $CREATE_FILE << END_OF_LINE
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
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

/******************************************************************************
 *
 *      neo_errsym.c
 *
 *      説明    neo_errno に対応するエラーメッセージを返す
 *
 ******************************************************************************/
#include "neo_system.h"

/*
 *      エラーシンボル構造体
 */
typedef struct {
        int neo_errno;  /* neo_errno */
        char* msg;      /* エラーメッセージ */
} _errsym_t;

extern _errsym_t _neo_errsymtab[];

/******************************************************************************
 * 関数名
 *              neo_errsym()
 * 機能
 *              neo_errno に対応するエラーメッセージを返す
 * 関数値
 *      正常: シンボル文字列
 *      異常: Not defined error メッセージ
 * 注意
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
         *      メッセージ Not found !!
         */
        sprintf( buf, "%s[%x]", "Not defined error", neo_errno );
        return( buf );
}

/*
 *      エラーシンボルテーブル
 */
_errsym_t _neo_errsymtab[] = {
END_OF_LINE
}

main()
{
        make_awk_script1
        make_awk_script2
        procedure
        make_database_from_neo_err_h
        make_index_from_errno_h
        make_database_from_neo_unix_errno_h
        rm -f $awkfile1 $awkfile2 $tmpfile
}

main $1



