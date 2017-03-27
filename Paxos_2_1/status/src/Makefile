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

GCC	= gcc -Wall -g -fPIC -D_FILE_OFFSET_BITS=64#-Wuninitialized -O
AR	= ar
RANLIB	= ranlib

G	= ../../../NWGadget/
P	=../ ../paxos/
H = ../h/
LIB = ../lib/
#BIN	= ../bin/

INC	= -I$(G)h -I$(H)
LIBS	= -L$(S)lib -L$(P)lib -L$(G)lib -L$(LIB) \
			-lNWGadget -lpthread 

all: $(LIB)libStatus.a

$(LIB)libStatus.a: $(H)Status.h Status.c \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -c $(INC) Status.c
	$(AR) rv $(LIB)libStatus.a *.o
	$(RANLIB) $(LIB)libStatus.a
	rm *.o

clean:
	rm -f $(LIB)libStatus.a

ROOT = ../../

install:
	cp $(LIB)libStatus.a $(ROOT)lib
	cp ../h/Status.h $(ROOT)h

uninstall:
	rm -f $(ROOT)lib/libStatus.a
	rm -f $(ROOT)h/Status.h


