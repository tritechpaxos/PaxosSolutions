################################################################################
#
# Makefile --- Makefile of Gadget Library
#
# Copyright (C) 2010,2011 triTech Inc. All Rights Reserved.
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

all: $(LIB)libNWGadget.a 

$(LIB)libNWGadget.a:	$(H)NWGadget.h NWGadget.c Makefile
	$(GCC) -c -I$(H) NWGadget.c
	$(AR) rv $(LIB)libNWGadget.a *.o
	$(RANLIB) $(LIB)libNWGadget.a
	rm *.o

clean:
	rm -f $(LIB)libNWGadget.a
