################################################################################
#
# Makefile --- Makefile of test programs of NWGadget library
#
# Copyright (C) 2010-2015 triTech Inc. All Rights Reserved.
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

GCC	= gcc $(CFLAGS) -D_DEBUG_ -fPIC 
AR	= ar
RANLIB	= ranlib

H = ../h/
LIB = ../lib/
BIN = ../bin/
INC	= -I$(H)
LIBS = -L$(LIB) -lNWGadget -lpthread -levent

all: $(BIN)skl_mem $(BIN)skl_mem_best $(BIN)Hash $(BIN)DHash $(BIN)timer \
	$(BIN)BTree $(BIN)BTree1 $(BIN)BTree2 \
	$(BIN)Msg $(BIN)GurdedElementNew $(BIN)Cksum $(BIN)Queue $(BIN)FdEvent \
	$(BIN)LogPrint $(BIN)LogFiles

$(BIN)skl_mem: skl_mem.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)skl_mem $(INC) skl_mem.c $(LIBS)

$(BIN)skl_mem_best: skl_mem_best.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)skl_mem_best $(INC) skl_mem_best.c $(LIBS)

$(BIN)Hash: Hash.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)Hash $(INC) Hash.c $(LIBS)

$(BIN)timer: timer.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)timer $(INC) timer.c $(LIBS)

$(BIN)BTree: BTree.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)BTree $(INC) BTree.c $(LIBS)

$(BIN)BTree1: BTree1.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)BTree1 $(INC) BTree1.c $(LIBS)

$(BIN)BTree2: BTree2.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)BTree2 $(INC) BTree2.c $(LIBS)

$(BIN)Msg: Msg.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)Msg $(INC) Msg.c $(LIBS)

$(BIN)GurdedElementNew: GurdedElementNew.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)GurdedElementNew $(INC) GurdedElementNew.c $(LIBS)

$(BIN)Cksum: Cksum.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)Cksum $(INC) Cksum.c $(LIBS)

$(BIN)Queue: Queue.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)Queue $(INC) Queue.c $(LIBS)

$(BIN)LogPrint: LogPrint.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)LogPrint $(INC) LogPrint.c $(LIBS)

$(BIN)LogFiles: LogFiles.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)LogFiles $(INC) LogFiles.c $(LIBS)

$(BIN)DHash: DHash.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)DHash $(INC) DHash.c $(LIBS)

$(BIN)FdEvent: FdEvent.c $(LIB)libNWGadget.a
	$(GCC) -o $(BIN)FdEvent $(INC) FdEvent.c $(LIBS)

clean:
	rm -f $(BIN)skl_mem*
	rm -f $(BIN)Hash*
	rm -f $(BIN)DHash*
	rm -f $(BIN)timer*
	rm -f $(BIN)BTree*
	rm -f $(BIN)Msg*
	rm -f $(BIN)GurdedElement*
	rm -f $(BIN)Cksum*
	rm -f $(BIN)Queue*
	rm -f $(BIN)FdEvent*
	rm -f $(BIN)LogPrint*
	rm -f $(BIN)LogFiles*
