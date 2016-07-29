################################################################################
#
# Makefile --- Makefile of xjPing (On Memory DB) Library, AP and Test programs
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
	$(MAKE) -C class
	$(MAKE) -C dbg 
#	$(MAKE) -C dlink
	cd  errno && $(MAKE) .INIT
	$(MAKE) -C errno
#	$(MAKE) -C txt

clean:
	cd  class && $(MAKE) clean
	cd  dbg && $(MAKE) clean
#	cd  dlink && $(MAKE) clean
	cd  errno && $(MAKE) clean
#	cd  txt && $(MAKE) clean
