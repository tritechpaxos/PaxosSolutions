################################################################################
#
# Makefile --- Makefile of test programs of session administration
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
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(P)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
		-lPaxosSession -lPaxos -lNWGadget -lpthread -levent

all: $(BIN)PaxosSessionShutdown \
		$(BIN)PaxosSessionProbe $(BIN)PaxosSessionChangeMember

$(BIN)PaxosSessionShutdown: PaxosSessionShutdown.c \
					$(H)PaxosSession.h $(LIB)libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PaxosSessionShutdown $(INC) PaxosSessionShutdown.c $(LIBS)

$(BIN)PaxosSessionProbe: PaxosSessionProbe.c \
					$(H)PaxosSession.h $(LIB)libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PaxosSessionProbe $(INC) PaxosSessionProbe.c $(LIBS)

$(BIN)PaxosSessionChangeMember: PaxosSessionChangeMember.c \
					$(H)PaxosSession.h $(LIB)libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)PaxosSessionChangeMember $(INC) PaxosSessionChangeMember.c $(LIBS)

clean:
	rm -f $(BIN)PaxosSessionShutdown*
	rm -f $(BIN)PaxosSessionProbe*
	rm -f $(BIN)PaxosSessionChangeMember*

ROOT = ../../

install:
	cp $(BIN)PaxosSessionShutdown $(ROOT)bin
	cp $(BIN)PaxosSessionProbe $(ROOT)bin
	cp $(BIN)PaxosSessionChangeMember $(ROOT)bin

uninstall:
	rm -f $(ROOT)bin/PaxosSessionShutdown
	rm -f $(ROOT)bin/PaxosSessionProbe
	rm -f $(ROOT)bin/PaxosSessionChangeMember


