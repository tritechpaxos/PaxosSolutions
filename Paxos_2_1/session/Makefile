################################################################################
#
# Makefile --- Makefile of session Library and test programs
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

all:
	mkdir -p lib bin
	cd src;make
	cd adm;make
	cd test;make

clean:
	cd src;make clean
	cd adm;make clean
	cd test;make clean

install:
	cd src;make install
	cd adm;make install

uninstall:
	cd src;make uninstall
	cd adm;make uninstall
