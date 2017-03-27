################################################################################
#
# Makefile --- Makefile of cache Library
#
# Copyright (C) 2014 triTech Inc. All Rights Reserved.
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

CFLAGS	= -Wall -fPIC -D_FILE_OFFSET_BITS=64
ifeq ($(OPT),)
CFLAGS += -g -O0 -ggdb -rdynamic
else
CFLAGS += -g -O2 -fno-strict-aliasing
endif

GCC	= gcc $(CFLAGS)
AR	= ar
RANLIB	= ranlib

G	= ../../../NWGadget/
P	= ../../paxos/
S	= ../../session/
F	= ../../PFS/
ST	= ../../status/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(P)h -I$(F)h -I$(S)h -I$(ST)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
			-lPaxosPFS -lPaxosSession -lPaxos -lNWGAdget

all: $(LIB)libCache.a $(LIB)libCacheDlg.a $(LIB)libCacheBlock.a

$(LIB)libCache.a: FileCache.c $(H)FileCache.h DlgCache.c $(H)DlgCache.h \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a \
					$(ST)h/Status.h $(ST)lib/libStatus.a
	$(GCC) -c $(INC) FileCache.c
	$(AR) rv $(LIB)libCache.a FileCache.o
	$(RANLIB) $(LIB)libCache.a
	rm -f *.o

$(LIB)libCacheDlg.a: DlgCache.c $(H)DlgCache.h \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a \
					$(ST)h/Status.h $(ST)lib/libStatus.a
	$(GCC) -c $(INC) DlgCache.c
	$(AR) rv $(LIB)libCacheDlg.a DlgCache.o
	$(RANLIB) $(LIB)libCacheDlg.a
	rm -f *.o

$(LIB)libCacheBlock.a: BlockCache.c $(H)BlockCache.h $(H)DlgCache.h \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a \
					$(ST)h/Status.h $(ST)lib/libStatus.a
	$(GCC) -c $(INC) BlockCache.c
	$(AR) rv $(LIB)libCacheBlock.a BlockCache.o
	$(RANLIB) $(LIB)libCacheBlock.a
	rm -f *.o

clean:
	rm -f $(LIB)libCache.a
	rm -f $(LIB)libCacheDlg.a
	rm -f $(LIB)libCacheBlock.a

ROOT = ../../

install:
	cp $(LIB)libCache.a $(ROOT)lib
	cp $(LIB)libCacheDlg.a $(ROOT)lib
	cp $(LIB)libCacheBlock.a $(ROOT)lib
	cp ../h/DlgCache.h $(ROOT)h
	cp ../h/FileCache.h $(ROOT)h
	cp ../h/BlockCache.h $(ROOT)h

uninstall:
	rm -f $(ROOT)lib/libCache.a
	rm -f $(ROOT)lib/libCacheDlg.a
	rm -f $(ROOT)lib/libCacheBlock.a
	rm -f $(ROOT)h/DlgCache.h
	rm -f $(ROOT)h/FileCache.h
	rm -f $(ROOT)h/BlockCache.h


