###########################################################################
#   pycss Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of pycss.
#
#   Pycss is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pycss is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with pycss.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from __future__ import absolute_import
from os import environ

__version__ = '0.2.4'


def set_conf_dir(path=None):
    if path is not None:
        environ['PAXOS_CELL_CONFIG'] = path + '/'
    else:
        del(environ['PAXOS_CELL_CONFIG'])


def connect(cell):
    from .css import Css
    return Css(cell)
