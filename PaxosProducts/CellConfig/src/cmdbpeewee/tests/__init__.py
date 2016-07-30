###########################################################################
#   cmdbpeewee Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of cmdbpeewee.
#
#   Cmdbpeewee is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cmdbpeewee is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with cmdbpeewee.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import logging

logger = logging.getLogger('peewee')
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
logger.addHandler(ch)
