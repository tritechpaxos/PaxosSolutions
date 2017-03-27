################################################################################
#
# Makefile --- Makefile of xjPing (On Memory DB) errno Table Library programs
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
### ホームディレクトリ
TOP	= ../../..

### コンパイルオプション
OPT	= -g
### デバッグフラグ
#DEBUG	= -DDEBUG

### ライブラリ名
LIBNAME	= $(TOP)/../lib/libneo
### 固有のヘッダディレクトリ
LCFLGS	=

### ソースファイル 
SRC	= neo_errsym.c unix_errno.c
### ヘッダファイル
HEAD    =

### Do not change under this line.
include	$(TOP)/makedef/makelib

.INIT: 
	sh unixerr.sh $(INCDIRC)/neo_unix_err.h $(XJPOS)
	sh errsym.sh $(INCDIRC)/neo_errno.h $(XJPOS)

unix_errno.c: unixerr.sh
	sh unixerr.sh $(INCDIRC)/neo_unix_err.h $(XJPOS)

neo_errsym.c: errsym.sh $(TOP)/../h/neo_*_err.h
	sh errsym.sh $(INCDIRC)/neo_errno.h $(XJPOS)
