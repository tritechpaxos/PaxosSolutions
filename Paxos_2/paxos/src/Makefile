################################################################################
#
# Makefile --- Makefile of Paxos Library
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
H = ../h/
LIB = ../lib/
INC	= -I$(G)h -I$(H)

all: $(LIB)libPaxos.a

$(LIB)libPaxos.a:	$(H)Paxos.h Paxos.c PaxosCellAddr.c PaxosIsActive.c \
					$(G)h/NWGadget.h $(G)lib/libNWGadget.a
	$(GCC) -c $(INC) Paxos.c
	$(GCC) -c $(INC) PaxosCellAddr.c
	$(GCC) -c $(INC) PaxosIsActive.c
	$(AR) rv $(LIB)libPaxos.a *.o
	$(RANLIB) $(LIB)libPaxos.a
	rm *.o

clean:
	rm -f $(LIB)libPaxos.a
