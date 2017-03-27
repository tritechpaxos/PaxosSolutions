################################################################################
#
# Makefile --- Makefile of Converter Library
#
# Copyright (C) 2010,12 triTech Inc. All Rights Reserved.
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
			-lPaxosSession -lPaxos -lNWGAdget

all: $(LIB)libCNVClient.a $(LIB)libCNVServer.a

$(LIB)libCNVClient.a: CIFS.c CIFSClient.c $(H)CNV.h CIFS.h #\
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -c $(INC) CIFS.c
	$(GCC) -c $(INC) CIFSClient.c
	$(AR) rv $(LIB)libCNVClient.a *.o
	$(RANLIB) $(LIB)libCNVClient.a
	rm -f *.o

$(LIB)libCNVServer.a: CIFS.c CIFSServer.c $(H)CNV.h CIFS.h #\
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -c $(INC) CIFS.c
	$(GCC) -c $(INC) CIFSServer.c
	$(AR) rv $(LIB)libCNVServer.a *.o
	$(RANLIB) $(LIB)libCNVServer.a
	rm -f *.o

clean:
	rm -f $(LIB)libCNVClient.a
	rm -f $(LIB)libCNVServer.a

