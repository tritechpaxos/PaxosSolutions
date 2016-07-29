################################################################################
#
# Makefile --- Makefile of xjPing (On Memory DB) db Manage Library programs
#
# Copyright (C) 2011-2013 triTech Inc. All Rights Reserved.
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
OPT	= -g
### デバッグフラグ
#DEBUG	= -DDEBUG

LIBNAME = $(TOP)/../lib/libneo
### 固有のヘッダディレクトリ
LCFLGS  =

SRC	= rdb_open.c rdb_create.c rdb_link.c rdb_misc.c rdb_insert.c  \
		rdb_delete.c rdb_update.c rdb_search.c rdb_find.c \
		rdb_gram.c rdb_lex.c rdb_expr.c	rdb_set.c rdb_sort.c \
		rdb_sorter.c rdb_flush.c rdb_hold.c rdb_compress.c \
		rdb_event.c rdb_direct.c rdb_stop.c \
		rdb_dump.c time_stamp.c rdb_list.c rdb_commit.c
		### ソースファイル 

### ヘッダファイル
HEAD	= y.tab.h

Y=	rdb_gram.c

### Do not change under this line.
include	$(TOP)/makedef/makelib

rdb_gram.c:	rdb_gram.expr
#	yacc -d rdb_gram.expr
	bison -y -d rdb_gram.expr
	mv y.tab.c rdb_gram.c
