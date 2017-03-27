################################################################################
#
# Makefile --- xjPing (On Memory DB) SQL Analize I/F Library programs
#
# Copyright (C) 2011,2014 triTech Inc. All Rights Reserved.
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
TOP	= ../../../
NWGADGET= ../../../../../../NWGadget

### コンパイルオプション
OPT	= -g -DDB_MAN_SYSTEM
### デバッグフラグ
#DEBUG	= -DDEBUG

LIBNAME = $(TOP)/../lib/libneo
### 固有のヘッダディレクトリ
LCFLGS  =

### ソースファイル 
#SRC	= sql_gram.c sql_lex.c sql_expr.c sql_statement.c sql_like.c
SRC	= y.tab.c sql_lex.c sql_expr.c sql_statement.c sql_like.c

### ヘッダファイル
HEAD	= sql.h

LOAD=test
#LIBFLGS= -static
Y=y.tab.c

### Do not change under this line.
include	$(TOP)/makedef/makelib
#include	$(TOP)/makedef/makeproc

y.tab.c: sql_gram.expr Makefile
#	yacc -v -g -t -d -p sql sql_gram.expr
	bison -y -v -g -t -d -p sql sql_gram.expr
#	mv y.tab.c sql_gram.c
#	mv y.tab.h sql.h
#	cp y.tab.c sql_gram.c
#	cp y.tab.h sql.h

test: test.c Makefile $(TOP)../lib/libneo.a
	gcc -g -o test test.c $(DEBUG) -I. -I$(TOP)../h  -I$(NWGADGET)/h \
			$(TOP)../lib/libneo.a $(NWGADGET)/lib/libNWGadget.a -lpthread
