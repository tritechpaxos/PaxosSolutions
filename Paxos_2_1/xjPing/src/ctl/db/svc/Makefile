################################################################################
#
# Makefile of xjPing (On Memory DB) db Server programs
#
# Copyright (C) 2011 triTech Inc. All Rights Reserved.
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

### ホームディレクトリ
TOP	= ../../..

### コンパイルオプション
#OPT	= -g -DDB_MAN_SYSTEM 
OPT	= -g -Wall
### デバッグフラグ
#DEBUG	= -DDEBUG

LIBNAME = $(TOP)/../lib/libneo
### ソースファイル 
SRC	= svc_misc.c svc_link.c svc_create.c svc_open.c svc_insert.c \
		svc_delete.c svc_load.c svc_mem.c svc_update.c svc_search.c \
		svc_find.c svc_sort.c svc_flush.c svc_hold.c svc_compress.c \
		svc_event.c svc_direct.c svc_number.c svc_stop.c \
		svc_dump.c svc_alternate_table.c svc_list.c \
		prc_open.c prc_find.c prc_search.c prc_join.c prc_sql.c \
		prc_insert.c prc_delete.c prc_update.c prc_sort.c prc_view.c \
		xj_swap.c xj_hash.c prc_file.c svc_commit.c

### ヘッダファイル
HEAD	= 

### ロードモジュール名 
LOAD	= xjPing

### インストールディレクトリ
INSDIR	= $(TOP)/../com

### リンクオプション 
LIBFLGS	= #-static
### ローカルライブラリ( NEO/DB I/F lib )
LCFLGS	= -I../clnt
LCLIBS	= ../dbif/libtbl.a

### Do not change under this line.
include	$(TOP)/makedef/makelib
