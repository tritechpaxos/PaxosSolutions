################################################################################
#
# Makefile --- Makefile of xjPing (On Memory DB)  Library, AP and Test programs
#
# Copyright (C) 2011 triTech Inc. All Rights Reserved.
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

all:
	cd com;make
	cd skel;make
	cd link;make
	cd db;make
clean:
	rm -f ../../lib/*
	cd com;make clean
	cd skel;make clean
	cd link;make clean
	cd db;make clean

install:
	cd db;make install

uninstall:
	cd db;make uninstall
