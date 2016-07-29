################################################################################
#
# Makefile --- Makefile of test programs of cache Library
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
PFS	= ../../PFS/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(H) -I$(G)h -I$(P)h -I$(S)h -I$(PFS)h
LIBS	=  -L$(G)lib -L$(P)lib -L$(S)lib -L$(PFS)lib -L$(LIB) \
-lCache -lCacheDlg -lNWGadget -lPaxosSession -lPaxosPFS -lPaxos -lpthread -levent

TARGET	= TestFile TestDlgCache TestDlgCacheBy

all: $(TARGET)

$(TARGET):%:%.c $(H)FileCache.h $(H)DlgCache.h \
				$(LIB)libCache.a $(LIB)libCacheDlg.a \
				$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $@ $(INC) $< $(LIBS)

clean:
	rm -f $(TARGET) 

