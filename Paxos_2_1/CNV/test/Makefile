################################################################################
#
# Makefile --- Makefile of CoC and CoS programs of converter 
#
# Copyright (C) 2016 triTech Inc. All Rights Reserved.
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

CFLAGS	= -Wall -fPIC
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
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(P)h -I$(S)h -I$(ST)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
	-lPaxosSession -lPaxos -lNWGadget -lpthread -levent -lcrypto

all: $(BIN)CoC $(BIN)CoS IdMap #$(BIN)HTTPd $(BIN)HTTPProxy

$(BIN)CoC: CoC.c $(H)CNV.h ../src/CIFS.h \
					$(LIB)libCNVClient.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)CoC $(INC) CoC.c -lCNVClient $(LIBS)

$(BIN)CoS: CoS.c $(H)CNV.h ../src/CIFS.h \
					$(LIB)libCNVServer.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)CoS $(INC) CoS.c -lCNVServer $(LIBS)

$(BIN)HTTPd: HTTPd.c $(H)CNV.h \
					$(LIB)libCNV.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)HTTPd $(INC) HTTPd.c $(LIBS)

$(BIN)HTTPProxy: proxy.c $(H)CNV.h \
					$(LIB)libCNV.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)HTTPProxy $(INC) proxy.c $(LIBS)

IdMap: IdMap.c ../src/CIFS.h $(H)CNV.h \
					$(LIB)libCNVClient.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o IdMap $(INC) -I../src/ IdMap.c -lCNVClient $(LIBS)

clean:
	rm -f $(BIN)CoC*
	rm -f $(BIN)CoS*
	rm -f $(BIN)HTTPd*
	rm -f $(BIN)HTTPProxy*
	rm -f IdMap

