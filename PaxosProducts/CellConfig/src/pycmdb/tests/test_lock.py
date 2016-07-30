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
import unittest
import sys
import os
import os.path
import pycmdb
import tests


class TestLock(unittest.TestCase):

    def setUp(self):
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute('create table qq0 (val int)')
        self.cursor.execute('create table qq1 (val int)')
        self.cursor.execute('create table qq2 (val int)')

    def tearDown(self):
        self.cursor.execute('drop table qq0')
        self.cursor.execute('drop table qq1')
        self.cursor.execute('drop table qq2')
        self.conn.close()

    def test_lock_str(self):
        self.conn.lock('qq0')
        self.conn.unlock('qq0')
        self.conn.lock(u'qq0')
        self.conn.unlock('qq0')
        self.conn.lock('qq0')
        self.conn.unlock(u'qq0')
        self.conn.lock(['qq0', 'qq1', 'qq2'])
        self.conn.unlock(['qq0', 'qq1', 'qq2'])
        self.conn.lock(('qq0', u'qq1', 'qq2'))
        self.conn.unlock((u'qq0', 'qq1', u'qq2'))

    def test_lock_bad_table(self):
        with self.assertRaises(pycmdb.ProgrammingError):
            self.conn.lock('pp0')

    def test_lock_bad_type(self):
        with self.assertRaises(TypeError):
            self.conn.lock(2)

    def test_unlock_bad_table(self):
        self.conn.unlock('pp0')

    def test_lock_already_lock_table(self):
        self.conn.lock('qq0')
        with self.assertRaises(pycmdb.OperationalError):
            self.conn.lock('qq0')
        self.conn.unlock('qq0')

    def test_lock_insert(self):
        self.conn.lock('qq0')
        try:
            self.cursor.executemany('insert into qq0 values(?)', [[1], [7]])
        finally:
            self.conn.unlock('qq0')

    def test_lock_drop_table(self):
        self.conn.lock('qq0')
        try:
            with self.assertRaisesRegexp(pycmdb.OperationalError,
                                         'table is busy'):
                self.cursor.execute('drop table qq0')
        finally:
            self.conn.unlock('qq0')
