################################################################################
#
# Makefile --- Makefile of Mirror cell
#
# Copyright (C) 2014-2015 triTech Inc. All Rights Reserved.
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
CA	= ../../cache/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(P)h -I$(S)h -I$(F)h -I$(ST)h -I$(CA)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(F)lib -L$(G)lib -L$(LIB) -L$(ST)lib \
	-L$(CA)lib \
	-lPaxosPFS -lPaxosSession -lPaxos -lNWGadget -lStatus -lCache -lpthread \
	-levent

all: $(BIN)LVServer LVClient.o

$(BIN)LVServer: LVServer.c $(H)LV.h \
					$(CA)h/FileCache.h $(CA)lib/libCache.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)LVServer $(INC) LVServer.c $(LIBS)

LVClient.o: LVClient.c $(H)LV.h \
					$(CA)h/FileCache.h \
					$(S)h/PaxosSession.h  \
					$(P)h/Paxos.h  \
					$(G)h/NWGadget.h 
	$(GCC) -c $(INC) $<

clean:
	rm -f *.o
	rm -f $(BIN)LVServer*

