# -*- coding: utf-8 -*-
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
import time
import os
import os.path
import pycmdb
import tests


class TestCursorClose(unittest.TestCase):

    def setUp(self):
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.close()

    def tearDown(self):
        self.conn.close()

    def test_close_setinputsizes(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.setinputsizes([1])

    def test_close_setoutputsizes(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.setoutputsizes(1)
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.setoutputsizes(1, 1)

    def test_close_execute(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.execute('select * from {0}'.format(tests.TABLE))

    def test_close_executemany(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.executemany(
                "insert into {0} values(?,?,?,?)".format(tests.TABLE),
                ((1, 'aaa', 7, 7.1),
                 (2, u'あああ', 2, 2.1),
                 (3, u'あああ', False, -1.2)))

    def test_close_fetchone(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.fetchone()

    def test_close_fetchall(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.fetchall()

    def test_close_fetchmany(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.fetchmany()
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.fetchmany(2)
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed cursor'):
            self.cursor.fetchmany(size=4)


class TestConnectionClose(unittest.TestCase):

    def setUp(self):
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.conn.close()

    def test_already_close_commit(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.conn.commit()

    def test_already_close_rollback(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.conn.rollback()

    def test_already_close_cursor(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.conn.cursor()

    def test_already_close_close(self):
        self.conn.close()

    def test_already_close_setinputsizes(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.setinputsizes([1])

    def test_already_close_setoutputsizes(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.setoutputsizes(1)
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.setoutputsizes(1, 1)

    def test_already_close_execute(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.execute('select * from {0}'.format(tests.TABLE))

    def test_already_close_executemany(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.executemany(
                "insert into {0} values(?,?,?,?)".format(tests.TABLE),
                ((1, 'aaa', 7, 7.1),
                 (2, u'あああ', 2, 2.1),
                 (3, u'あああ', False, -1.2)))

    def test_already_close_fetchone(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.fetchone()

    def test_already_close_fetchall(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.fetchall()

    def test_already_close_fetchmany(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.fetchmany()
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.fetchmany(2)
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'already closed connection'):
            self.cursor.fetchmany(size=4)


class TestCloseRollback(unittest.TestCase):

    def setUp(self):
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute(
            'create table {0} (id int, txt char(64), no ulong, val float)'
            .format(tests.TABLE))
        self.recs = ((1, 'aaa', 7, 7.1),
                     (2, u'あああ', 1073741823L, 2.1),
                     (3, u'あああ', 1073741824L, 2.1),
                     (4, u'あああ', 4294967296L, -1.2))

    def tearDown(self):
        self.conn.close()
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute('drop table {0}'.format(tests.TABLE))
        self.conn.close()

    def test_insert_rollback(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)
        self.cursor.executemany(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE), self.recs)
        self.conn.close()
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)

    def test_insert_commit(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)
        self.cursor.executemany(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE), self.recs)
        self.conn.commit()
        self.conn.close()
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(len(self.recs), self.cursor.rowcount)
