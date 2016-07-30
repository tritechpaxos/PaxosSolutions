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


class TestPyCmdbFetch(unittest.TestCase):

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
        self.cursor.executemany(
            "insert into {0} values(?,?,?,?)".format(tests.TABLE), self.recs)
        self.conn.commit()

    def tearDown(self):
        self.conn.rollback()
        self.cursor.execute('drop table {0}'.format(tests.TABLE))
        self.conn.close()

    def test_fetchall(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        res = self.cursor.fetchall()
        self.assertEquals(len(self.recs), self.cursor.rowcount)
        self.assertEquals(len(self.recs), len(res))
        for itt in zip(self.recs, res):
            self.assertEqual(len(itt[0]), len(itt[1]))
            for it in zip(*itt):
                self.assertAlmostEqual(*it, places=6)
        with self.assertRaises(pycmdb.ProgrammingError):
            self.cursor.fetchall()

    def test_fetchone(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        for rec in self.recs:
            res = self.cursor.fetchone()
            self.assertEqual(len(res), len(rec))
            for it in zip(rec, res):
                self.assertAlmostEqual(*it, places=6)
        self.assertIsNone(self.cursor.fetchone())

    def test_fetchmany(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        res = self.cursor.fetchmany(2)
        self.assertEquals(2, len(res))
        for itt in zip(self.recs[0:2], res):
            self.assertEqual(len(itt[0]), len(itt[1]))
            for it in zip(*itt):
                self.assertAlmostEqual(*it, places=6)
        res = self.cursor.fetchmany()
        self.assertEquals(2, len(res))
        for itt in zip(self.recs[2:4], res):
            self.assertEqual(len(itt[0]), len(itt[1]))
            for it in zip(*itt):
                self.assertAlmostEqual(*it, places=6)
        self.assertEqual([], self.cursor.fetchmany())

    def test_arraysize(self):
        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.cursor.arraysize = 2
        res = self.cursor.fetchmany()
        self.assertEquals(2, len(res))
        for itt in zip(self.recs[0:2], res):
            self.assertEqual(len(itt[0]), len(itt[1]))
            for it in zip(*itt):
                self.assertAlmostEqual(*it, places=6)
        res = self.cursor.fetchmany()
        self.assertEquals(2, len(res))
        for itt in zip(self.recs[2:4], res):
            self.assertEqual(len(itt[0]), len(itt[1]))
            for it in zip(*itt):
                self.assertAlmostEqual(*it, places=6)
        self.assertEqual([], self.cursor.fetchmany())

        self.cursor.execute('select * from {0}'.format(tests.TABLE))
        self.cursor.arraysize = 4
        res = self.cursor.fetchmany()
        self.assertEquals(4, len(res))
        self.assertEqual([], self.cursor.fetchmany())
