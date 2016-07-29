################################################################################
#
# Makefile --- Makefile of probe of VirtualVolume 
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

GCC	= gcc -Wall -g -fPIC -D_FILE_OFFSET_BITS=64 #-Wuninitialized -O
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
LIBS= -L$(F)lib -L$(S)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) \
	-lPaxosPFS -lPaxosSession -lPaxos -lNWGadget -lStatus -lCache -lpthread \
	-levent

TARGET	= LVProbe VVProbe

all: $(TARGET)

$(TARGET):%:%.c $(H)VV.h \
				$(F)h/PFS.h $(F)lib/libPaxosPFS.a \
				$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
				$(P)h/Paxos.h $(P)lib/libPaxos.a $(CA)lib/libCache.a \
				$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $@ $(INC) $@.c $(LIBS) 
	cp $@ $(BIN)

clean:
	cd $(BIN);rm -f $(TARGET)
	rm -f $(TARGET) $(OBJ) *.o
