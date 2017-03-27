#!/bin/sh -x

################################################################################
#
# unixerr.sh --- Shell Script of xjPing (On Memory DB) Errno Create programs
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
#
#-------------------------------------------------------------------------------
# PROCEDURE: unixerr.sh
#   PURPOSE: create unixerr.c file
#-------------------------------------------------------------------------------
neo_errno_file=$1

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

tmpfile_1="/tmp/unixerr.1"
tmpfile_2="/tmp/unixerr.2"
target="unix_errno.c"
#
#
# PROCEDURE: make_awk_script()
#   PURPOSE: create awk script for make .c source file
#
make_awk_script()
{
        cat > $tmpfile_1 << END_OF_LINE
#!/bin/awk
BEGIN {
        printf( "uerr_t unix2neotab[] = {\n" );
}
{
        s = \$NF;
        len = length(s);
        if( len <= 4 )
                sp = "                  ";
        else if( len >= 13 )
                sp = "  ";
        else
                sp = "          ";
        printf( "{ %s,%s%s },\n",s,sp,\$1 );
}
END {
        printf( "{ 0,0 }\n" );
        printf( "};\n" );
}
END_OF_LINE
}
#
# PROCEDURE: make_index_from_errno_h()
#   PURPOSE: make relation keyword index from <sys/errno.h>
#
#    OUTPUT: tmpfile
#               neo_errno       errno
#               E_UNIX_ENOENT   ENOENT
#
#   (1) pickup #define field from <sys/errno.h>
#   (2) sort for join command order by E_UNIX_XXX
#
make_index_from_errno_h()
{
	grep "define" $sys_errno_file | \
		awk '{ if( NF > 2 ) printf("E_UNIX_%s %s\n",$2,$2); }' | \
		sort -k 1,2 > $tmpfile_2
}
#
# PROCEDURE: make_procedure_unixerr_c()
#   PURPOSE: make "unixerr.c" procedure part
#
make_procedure_unixerr_c()
{
        cat > $target << END_OF_LINE
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
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */ 

/******************************************************************************
 *
 *      unix_errno.c
 *
 *      説明    UNIX errno と xjPing neo_errno の変換
 *
 ******************************************************************************/

#include <errno.h>
#include "neo_system.h"

/*
 *      エラー番号管理構造体
 */
typedef struct {
        int unx_errno;          /* errno */
        int neo_errno;          /* neo_errno */
} uerr_t;

extern uerr_t unix2neotab[];


/******************************************************************************
 * 関数名
 *              unixerr2neo()
 * 機能
 *              UNIX errno から xjPing neo_errno への変換
 * 関数値
 *      正常: neo_errno
 *      異常: 0
 * 注意
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
 * 関数名
 *              neoerr2unix()
 * 機能
 *              xjPing neo_errno から UNIX errno への変換
 * 関数値
 *      正常: errno
 *      異常: 0
 * 注意
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
 *      エラー番号管理テーブル
 */
END_OF_LINE
}
#
# PROCEDURE: make_database_from_neo_unix_errno_h()
#   PURPOSE: make relation database file from "neo_unix_err.h"
#
#   (1) pickup #define field  from "neo_unxi_err.h"
#   (2) sort for join command order by E_UNIX_XXX
#   (3) join FILE1:pickup(errno.h) FILE2:pickup(neo_unix_err.h)
#               relation key FILE1:FIELD2 ( errno defined symbol )
#                       and only pair mach FILE1,FILE2
#   (4) sort for TABLE order bye numeric errno
#   (5) genarate "unixerr.c" data part with awk script
#
make_database_from_neo_unix_errno_h()
{
        grep "define" $neo_errno_file | \
        awk '{ if( NF > 2 ) print $0; }' | \
        sort -k 2,3 | \
        join -j1 2 - $tmpfile_2 | \
        sort -n -k 3,4 | \
        awk -f $tmpfile_1 >> $target
}

main()
{
       cat $sys_errno_file > $tmpfile_2
       make_awk_script
       make_procedure_unixerr_c
       make_index_from_errno_h
       make_database_from_neo_unix_errno_h
       rm -f $tmpfile_1 $tmpfile_2
}

main $1

