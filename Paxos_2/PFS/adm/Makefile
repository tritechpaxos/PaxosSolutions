################################################################################
#
# Makefile --- Makefile of probe, client and server programs of PFS library
#
# Copyright (C) 2010 -2013 triTech Inc. All Rights Reserved.
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
ST	= ../../status/
CA	= ../../cache/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(P)h -I$(S)h -I$(CA)h -I$(H)
#LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
#			-lPaxosPFS -lPaxosSession -lPaxos -lNWGAdget
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) \
	-lPaxosPFS -lPaxosSession -lPaxos -lNWGadget -lStatus -lCache \
	-lpthread -levent

all: $(BIN)PFSProbe $(BIN)PFSRasActive $(BIN)PFSRasMonitor \
	$(BIN)PFSRasCluster

$(BIN)PFSProbe: PFSProbe.c $(H)PFS.h \
					$(H)/PFS.h $(LIB)/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a $(CA)lib/libCache.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PFSProbe $(INC) PFSProbe.c $(LIBS)

$(BIN)PFSRasCluster: PFSRasCluster.c $(H)PFS.h \
					$(H)/PFS.h $(LIB)/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PFSRasCluster $(INC) PFSRasCluster.c $(LIBS)

$(BIN)PFSRasActive: PFSRasActive.c $(H)PFS.h \
					$(H)/PFS.h $(LIB)/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PFSRasActive $(INC) PFSRasActive.c $(LIBS)

$(BIN)PFSRasMonitor: PFSRasMonitor.c $(H)PFS.h \
					$(H)/PFS.h $(LIB)/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PFSRasMonitor $(INC) PFSRasMonitor.c $(LIBS)

clean:
	rm -f $(BIN)PFSProbe*
	rm -f $(BIN)PFSAdmin*
	rm -f $(BIN)PFSRasCluster*
	rm -f $(BIN)PFSRasActive*
	rm -f $(BIN)PFSRasMonitor*
