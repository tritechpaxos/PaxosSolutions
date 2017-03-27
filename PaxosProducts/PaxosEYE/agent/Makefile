################################################################################
#
# Makefile --- Makefile of probe, client and server programs of PFS library
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

PAXOS_VER=Paxos_2_1

G	= ../../../NWGadget/
P	= ../../../$(PAXOS_VER)/paxos/
S	= ../../../$(PAXOS_VER)/session/
ST	= ../../../$(PAXOS_VER)/status/
CA	= ../../../$(PAXOS_VER)/cache/
PF	= ../../../$(PAXOS_VER)/PFS/
H = $(PF)/h/
LIB = $(PF)/lib/

INC	= -I$(G)h -I$(P)h -I$(S)h -I$(CA)h -I$(H)
#LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
#			-lPaxosPFS -lPaxosSession -lPaxos -lNWGAdget
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) \
	-lPaxosPFS -lPaxosSession -lPaxos -lNWGadget -lStatus -lCache \
	-lpthread -levent

all: RasAgent

RasAgent: RasAgent.c $(H)PFS.h \
					$(H)/PFS.h $(LIB)/libPaxosPFS.a \
					$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
					$(P)h/Paxos.h $(P)lib/libPaxos.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o RasAgent $(INC) RasAgent.c $(LIBS)

clean:
	rm -f RasAgent RasAgent.o
