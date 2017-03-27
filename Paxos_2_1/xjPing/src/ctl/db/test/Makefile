################################################################################
#
# Makefile --- Makefile of xjPing (On Memory DB) db Manage Test programs
#
# Copyright (C) 2011,2014,2015 triTech Inc. All Rights Reserved.
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
OPT	= -g
### デバッグフラグ
#DEBUG	= -DDEBUG

### リンクオプション 
LIBFLGS	= #-static
### ローカルヘッダのディレクトリ 
#LCFLGS	= -I../dbif
### ローカルライブラリ
#LCLIBS	= ../dbif/libtbl.a

### SYS LIB
LIBDIR	= $(TOP)/lib

### インストールディレクトリ
#INSDIR	= ../../../../com
INSDIR	= ../../../../bin


INCDIRC	= $(TOP)/../h
INCDIRA	= $(TOP)/../../../NWGadget/h

LIBDIR	= $(TOP)/../lib
LIB	= $(LIBDIR)/libneo.a
GADGETLIB = $(TOP)/../../../NWGadget/lib/libNWGadget.a \
			$(TOP)/../../cache/lib/libCache.a -lpthread -lreadline -levent
CC	= gcc -Wall

DEF	= $(DEBUG) -D_FILE_OFFSET_BITS=64
INC	= -I. -I$(INCDIRC) -I$(INCDIRA)
CFLAGS	= $(OPT) $(DEF) $(INC) 


#TARGET  = dump_file
#TARGET  = cre drop append dupcheck find search update delete compress sort \
#	hold1 hold2 rhold1 rhold2 \
#	direct \
#	dump_proc dump_inf dump_table dump_tbl dump_usr dump_mem \
#	maketest link flush stop restart \
#	sql
TARGET	= xjPing xjInfo xjTable xjDump_user \
	xjCre xjDrop xjCompress xjRhold1 \
	xjStop xjShutdown xjRestart \
	xjSql \
	xjDump_swap xjList \
	xjExport xjImport \
	sort find search delete flush hold1 hold2 rhold1 rhold2 seek lock_seq \
	dupcheck commit \
	B1Create B1Bench FCProbe

INSTALL	= xjPing xjInfo xjTable xjDump_user \
	xjCre xjDrop xjCompress \
	xjStop xjShutdown xjRestart \
	xjSql \
	xjDump_swap xjList \
	xjExport xjImport \
	B1Create B1Bench FCProbe

UTL	=# dump_file

all: procc
procc: $(TARGET) $(UTL)

$(TARGET):%:%.c $(LIB) $(LCLIBS)
	$(CC) -o $@ $(CFLAGS) $(LCFLGS) $@.c $(LCLIBS) $(LIBFLGS) $(LIB) $(GADGETLIB)

$(UTL):%:%.c
	$(CC) -o $@ -g $(LIBFLGS) $@.c
	mv $@ $(INSDIR)

proclean: clean

clean:
	rm -f $(TARGET) $(OBJ) $(LLNS) $(TLOAD) DBG.* core a.out
	cd $(INSDIR);rm -f $(TARGET) $(UTL)

#install:
#	if test -n "$(INSDIR)"; then install $(INSTALL) $(INSDIR); fi	

#ROOT= ../../../../..

install:
#	cp $(INSTALL) $(ROOT)/bin
#	cp $(INCDIRC)/neo_*.h $(INCDIRC)/xj_*.h $(ROOT)/h

uninstall:
#	cd $(ROOT)/bin; rm $(INSTALL)
#	cd $(ROOT)/h; rm neo_*.h xj_*.h

