################################################################################
#
# Makefile --- Makefile of test programs of session Library
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

GCC	= gcc -Wall -g #-Wuninitialized -O
AR	= ar
RANLIB	= ranlib

G	= ../../../NWGadget/
P	= ../../paxos/
H = ../h/
LIB = ../lib/
BIN	= ../bin/

INC	= -I$(G)h -I$(H)
LIBS	= -L$(G)lib -L$(LIB) \
		-lNWGadget -lStatus -lpthread -levent

all: $(BIN)test $(BIN)STFileList $(BIN)STDiffList $(BIN)STDelete \
		$(BIN)STFileListDump $(BIN)STFindByPath

$(BIN)test: test.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)test $(INC) test.c $(LIBS)

$(BIN)STFileList: STFileList.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)STFileList $(INC) STFileList.c $(LIBS)

$(BIN)STDiffList: STDiffList.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)STDiffList $(INC) STDiffList.c $(LIBS)

$(BIN)STDelete: STDelete.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)STDelete $(INC) STDelete.c $(LIBS)

$(BIN)STFileListDump: STFileListDump.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)STFileListDump $(INC) STFileListDump.c $(LIBS)

$(BIN)STFindByPath: STFindByPath.c $(H)Status.h $(LIB)libStatus.a \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -o $(BIN)STFindByPath $(INC) STFindByPath.c $(LIBS)

clean:
	rm -f $(BIN)test*
	rm -f $(BIN)STFileList*
	rm -f $(BIN)STDiffList*
	rm -f $(BIN)STFileListDump*
	rm -f $(BIN)STDelete*
	rm -f $(BIN)STFindByPath*

install:

uninstall:

