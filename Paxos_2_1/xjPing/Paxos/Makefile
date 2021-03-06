################################################################################
#
# Makefile --- Makefile of xjPingPaxos
#
# Copyright (C) 2011-2015 triTech Inc. All Rights Reserved.
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
#T = A B
#echo:
#	@echo $(T:=Paxos)

GCC	= gcc -Wall -g -fPIC -D_FILE_OFFSET_BITS=64#-Wuninitialized -O
AR	= ar
RANLIB	= ranlib

G	= ../../../NWGadget/
P	= ../../paxos/
S	= ../../session/
F	= ../../PFS/
ST	= ../../status/
CA	= ../../cache/
H = .
LIB = ../lib/
BIN	= ../bin/

XJPING_H	= ../h/
XJPING_L	= ../lib/
XJPING_T	= ../src/ctl/db/test/

INC	= -I. -I$(G)h -I$(P)h -I$(S)h -I$(F)h -I$(ST)h -I$(CA)h -I$(H) -I$(XJPING_H)

LIBS= -L$(S)lib -L$(F)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) \
	-lPaxosSession -lPaxosPFS -lStatus -lCache -lPaxos -lNWGadget \
	-lpthread -lneo1 -levent

LIBS2= -L$(S)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) -L. \
	-lneo2 -lPaxosSession -lStatus -lCache -lPaxos -lNWGadget -lpthread \
	-levent -lreadline #-liconv

TARGET	= xjSql xjCre xjDrop xjList find FCProbe #xjTable B1Create B1Bench

all: $(BIN)xjPingPaxos $(TARGET)

$(BIN)xjPingPaxos: xjPingPaxos.c \
				$(S)h/PaxosSession.h $(S)lib/libPaxosSession.a \
				$(P)h/Paxos.h $(P)lib/libPaxos.a $(ST)lib/libStatus.a \
				$(CA)lib/libCache.a \
				$(G)h/NWGadget.h $(G)lib/libNWGadget.a $(XJPING_L)libneo1.a
	$(GCC) -o $(BIN)xjPingPaxos $(INC) xjPingPaxos.c $(LIBS)

p_user_cell.o: xjPingPaxos.h p_user_cell.c \
				$(S)h/PaxosSession.h $(P)h/Paxos.h $(G)h/NWGadget.h
	$(GCC) -c $(INC) p_user_cell.c

$(XJPING_L)libneo1.a: $(XJPING_L)libneo.a p_user_cell.o
	cp $(XJPING_L)libneo.a $(XJPING_L)libneo1.a
	ar -dv $(XJPING_L)libneo1.a skl_main.o p_user.o p_server.o rdb_link.o
	ar -rsv $(XJPING_L)libneo1.a p_user_cell.o

$(XJPING_L)libneo2.a: $(XJPING_L)libneo.a p_user_cell.o
	cp $(XJPING_L)libneo.a $(XJPING_L)libneo2.a
	ar -dv $(XJPING_L)libneo2.a p_user.o p_server.o rdb_link.o
	ar -rsv $(XJPING_L)libneo2.a p_user_cell.o

$(TARGET):%:$(XJPING_T)%.c $(XJPING_L)libneo2.a
	$(GCC) -o $@ $(INC) $(XJPING_T)$@.c $(LIBS2) 
#	mv $@ $(BIN)$@Paxos
	cp $@ $(BIN)$@Paxos

clean:
	rm -f $(XJPING_L)libneo1.a
	rm -f $(XJPING_L)libneo2.a
	cd $(BIN);rm -f xjPingPaxos $(TARGET:=Paxos)
	rm -f $(TARGET) $(OBJ) *.o

ROOT  = ../..

install:
#	cp $(TARGET) $(BIN)
	cd $(XJPING_H); cp neo_*.h xj_*.h $(ROOT)/h 
	cp $(BIN)xjPingPaxos $(ROOT)/bin
	cd $(BIN); cp $(addsuffix Paxos, $(TARGET)) $(ROOT)/bin
	cp $(XJPING_L)libneo1.a $(ROOT)/lib
	cp $(XJPING_L)libneo2.a $(ROOT)/lib

uninstall:
	cd $(ROOT)/h; rm -f neo_*.h xj_*.h
	cd $(ROOT)/bin; rm -f xjPingPaxos $(addsuffix Paxos, $(TARGET))
	cd $(ROOT)/lib; rm -f libneo1.a libneo2.a

#
#	Test section
#
TestTotal: Test1 Test2 TestB0 TestB100k

Test1:
	$(XJPING_T)Test1.sh paxos
Test2:
	$(XJPING_T)Test2.sh paxos
TestB0:
	$(XJPING_T)TestB0.sh paxos
TestB100k:
	$(XJPING_T)TestB100k.sh paxos
