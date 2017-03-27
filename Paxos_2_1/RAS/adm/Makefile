################################################################################
#
# Makefile --- Makefile of RasMailAdm
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
CA	= ../../cache/
PFS	= ../../PFS/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

#INC	= -I$(G)h -I$(P)h -I$(S)h -I$(ST)h -I$(CA)h -I$(H)
INC	= -I$(G)h -I$(P)h -I$(S)h -I$(ST)h -I$(PFS)h -I$(CA)h -I$(H)
LIBS= -L$(PFS)lib -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
	-lPaxosPFS -lPaxosSession -lPaxos -lNWGadget -lpthread -levent

all: $(BIN)RasEyeAdm

$(BIN)RasEyeAdm: RasEyeAdm.c \
					$(H)Ras.h \
					$(PFS)h/PFS.h $(PFS)lib/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o RasEyeAdm $(INC) RasEyeAdm.c $(LIBS)
	cp RasEyeAdm $(BIN)
	rm -f *.o

clean:
	rm -f RasEyeAdm
	rm -f $(BIN)RasEyeAdm

ROOT = ../../

install:
	cp $(BIN)RasEyeAdm $(ROOT)bin

uninstall:
	rm -f $(ROOT)bin/RasEyeAdm
