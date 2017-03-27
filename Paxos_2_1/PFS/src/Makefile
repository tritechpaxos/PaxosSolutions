################################################################################
#
# Makefile --- Makefile of PFS Library
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

INC	= -I$(G)h -I$(P)h -I$(S)h -I$(ST)h -I$(CA)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
			-lPaxosPFS -lPaxosSession -lPaxos -lNWGAdget

all: $(LIB)libPaxosPFS.a

$(LIB)libPaxosPFS.a: PFSRas.c PFSClient.c PFSServer.c $(H)PFS.h \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a \
					$(ST)h/Status.h $(ST)lib/libStatus.a
	$(GCC) -c $(INC) PFSClient.c
	$(GCC) -c $(INC) PFSServer.c
	$(GCC) -c $(INC) PFSRas.c
	$(AR) rv $(LIB)libPaxosPFS.a *.o
	$(RANLIB) $(LIB)libPaxosPFS.a
	rm -f *.o

clean:
	rm -f $(LIB)libPaxosPFS.a

ROOT = ../../

install:
	cp $(LIB)libPaxosPFS.a $(ROOT)lib
	cp ../h/PFS.h $(ROOT)h

uninstall:
	rm -f $(ROOT)lib/libPaxosPFS.a
	rm -f $(ROOT)h/PFS.h


