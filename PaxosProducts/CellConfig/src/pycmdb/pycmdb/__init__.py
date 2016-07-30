###########################################################################
#   pycmdb Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of pycmdb.
#
#   Pycmdb is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pycmdb is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with pycmdb.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import os
import time
import datetime
from ._pycmdb import Connection
from ._pycmdb import Warning, Error, InterfaceError, DatabaseError, DataError,\
    OperationalError, IntegrityError, InternalError, ProgrammingError,\
    NotSupportedError
from ._pycmdb import apilevel, threadsafety, paramstyle


def connect(database, conf_dir=None):
    if conf_dir is not None:
        if isinstance(conf_dir, list):
            os.environ['PAXOS_CELL_CONFIG'] = conf_dir[0] + '/'
        else:
            os.environ['PAXOS_CELL_CONFIG'] = conf_dir + '/'
    return Connection(database)

Date = datetime.date
Time = datetime.time
Timestamp = datetime.datetime


def DateFromTicks(tics):
    return Date(*time.localtime(ticks)[:3])


def TimeFromTicks(tics):
    return Time(*time.localtime(ticks)[3:6])


def TimestampFromTicks(tics):
    return Timestamp(*time.localtime(ticks)[:6])


def Binary(string):
    if isinstance(string, unicode):
        return string.encode()
    return str(string)


class _DBAPITypeObject(frozenset):
    def __eq__(self, other):
        if self is other:
            return True
        return other in self

    def __ne__(self, other):
        return not self.__eq__(other)

STRING = _DBAPITypeObject(['CHAR[5]', 'VARCHAR[5]'])
BINARY = _DBAPITypeObject(['BYTES[9]'])
NUMBER = _DBAPITypeObject(['INT[2]', 'UINT[3]', 'LONG[6]', 'ULONG[7]',
                           'NUMBER[6]', 'FLOAT[4]'])
DATETIME = _DBAPITypeObject(['DATE[8]'])
ROWID = _DBAPITypeObject()
