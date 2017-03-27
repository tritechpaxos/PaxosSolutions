################################################################################
#
# Makefile --- Makefile of xjPingPaxos' admin commands
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
	-lpthread -levent -lneo1 

LIBS3= -L$(S)lib -L$(P)lib -L$(G)lib -L$(ST)lib -L$(CA)lib -L$(LIB) -L. \
	-lneo3 -lPaxosSession -lStatus -lCache -lPaxos -lNWGadget -lpthread \
	-levent -lreadline

TARGET	= xjList xjInfo xjTable FCProbe \
#			xjCre xjDrop xjSql \
#			find search


all: $(TARGET)# xjRasAdmin

p_user_adm.o: p_user_adm.c \
				$(S)h/PaxosSession.h $(P)h/Paxos.h $(G)h/NWGadget.h
	$(GCC) -c $(INC) p_user_adm.c

$(XJPING_L)libneo3.a: $(XJPING_L)libneo.a p_user_adm.o
	cp $(XJPING_L)libneo.a $(XJPING_L)libneo3.a
	ar -dv $(XJPING_L)libneo3.a p_user.o p_server.o rdb_link.o
	ar -rsv $(XJPING_L)libneo3.a p_user_adm.o

$(TARGET):%:$(XJPING_T)%.c $(XJPING_L)libneo3.a
	$(GCC) -o $@ $(INC) $(XJPING_T)$@.c $(LIBS3) 
	cp $@ $(BIN)$@Admin

#xjRasAdmin:xjRasAdmin.c $(XJPING_L)libneo3.a
#	$(GCC) -o $@ $(INC) $< $(LIBS3) 
#	cp $@ $(BIN)
#
clean:
	cd $(BIN);rm -f $(TARGET:=Admin) xjRasAdmin
	rm -f $(TARGET) $(OBJ) *.o xjRasAdmin
	rm -f $(XJPING_L)libneo3.a

ROOT = ../..

install:
	cd $(BIN); cp $(addsuffix Admin, $(TARGET)) $(ROOT)/bin

uninstall:
	cd $(ROOT)/bin; rm -f $(addsuffix Admin, $(TARGET))

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
