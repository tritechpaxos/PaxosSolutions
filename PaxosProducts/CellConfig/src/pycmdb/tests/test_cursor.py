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


class TestPyCmdbCursor(unittest.TestCase):

    def setUp(self):
        self.conn = pycmdb.connect(tests.DATABASE)
        self.cursor = self.conn.cursor()
        self.cursor.execute(
            'create table {0} (id int, txt char(64), no ulong, val float)'
            .format(tests.TABLE))

    def tearDown(self):
        self.conn.rollback()
        self.cursor.execute('drop table {0}'.format(tests.TABLE))
        self.conn.close()

    def test_setinputsizes(self):
        self.cursor.setinputsizes([1])
        with self.assertRaises(TypeError):
            self.cursor.setinputsizes()

    def test_setoutputsizes(self):
        self.cursor.setoutputsizes(1)
        self.cursor.setoutputsizes(1, 1)
        with self.assertRaises(TypeError):
            self.cursor.setoutputsizes()
        with self.assertRaises(TypeError):
            self.cursor.setoutputsizes([])
        with self.assertRaises(TypeError):
            self.cursor.setoutputsizes(1, 'aa')

    def test_nextset(self):
        with self.assertRaises(AttributeError):
            self.cursor.nextset()

    def test_callproc(self):
        with self.assertRaises(AttributeError):
            self.cursor.callproc('procname')

    def test_cursor_errorhandler(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.errorhandler

    def test_cursor_lastrowid(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.lastrowid

    def test_cursor_messages(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.messages

    def test_cursor_next(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.next()

    def test_cursor_iter(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.__iter__()

    def test_cursor_scroll(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.scroll(1)

    def test_cursor_rownumber(self):
        with self.assertRaises(AttributeError):
            a = self.cursor.rownumber

    def test_cursor_connection(self):
        self.assertEqual(self.conn, self.cursor.connection)

    def test_execute(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)

    def test_description(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)
        descs = self.cursor.description
        exp = (('id', pycmdb.NUMBER, None, 4, None, None, None),
               ('txt', pycmdb.STRING, None, 64, None, None, None),
               ('no', pycmdb.NUMBER, None, 8, None, None, None),
               ('val', pycmdb.NUMBER, None, 4, None, None, None))
        self.assertEqual(exp, descs)

    def test_description_none(self):
        self.cursor.execute(
            'create table zzz (id int, txt char(64), no ulong, val float)')
        self.assertIsNone(self.cursor.description)
        self.cursor.execute('drop table zzz')
        self.assertIsNone(self.cursor.description)

    def test_rowcount(self):
        self.cursor.execute(
            'create table zzz (id int, txt char(64), no ulong, val float)')
        self.assertEqual(-1, self.cursor.rowcount)
        self.cursor.execute('drop table zzz')
        self.assertEqual(-1, self.cursor.rowcount)
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(0, self.cursor.rowcount)
        self.cursor.execute(
            u"insert into {0} values(1, 'あああ', 2, 2.1)".format(tests.TABLE))
        self.assertEquals(1, self.cursor.rowcount)
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.assertEquals(1, self.cursor.rowcount)

    def test_execute_unicode(self):
        self.cursor.execute(
            u"insert into {0} values(1, 'あああ', 2, 2.1)".format(tests.TABLE))
        self.cursor.execute(
            "insert into {0} values(2, 'aaa', 7, 7.1)".format(tests.TABLE))
        self.cursor.execute(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE),
            (3, 'aaa', 7, 7.1))
        self.cursor.execute(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE),
            (4, u'あああ', 111111111111L, 2.1))
        self.cursor.execute(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE),
            (5, u'あああ', False, -1.2))
        self.conn.commit()
        self.cursor.execute(
            "delete from {0} where txt=?".format(tests.TABLE), (u'あああ', ))
        self.cursor.execute(
            u"update {0} set no = no + 1 where val > ?".format(tests.TABLE),
            (3.0, ))

    def test_overflow(self):
        with self.assertRaises(OverflowError):
            self.cursor.execute(
                "insert into {0} values(?,?,?,?)".format(tests.TABLE),
                (1, u'あああ', 18446744073709551615L, 2.1))
        with self.assertRaises(OverflowError):
            self.cursor.execute(
                "insert into {0} values(?,?,?,?)".format(tests.TABLE),
                (1, u'あああ', -18446744073709551615L, 2.1))

    def test_executemany(self):
        self.cursor.executemany(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE),
            ((1, 'aaa', 7, 7.1),
             (2, u'あああ', 2, 2.1),
             (3, u'あああ', False, -1.2)))
        self.conn.commit()
