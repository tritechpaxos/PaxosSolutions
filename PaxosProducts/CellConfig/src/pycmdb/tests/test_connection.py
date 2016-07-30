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


class TestPyCmdbConnection(unittest.TestCase):

    def test_apilevel(self):
        self.assertEqual('2.0', pycmdb.apilevel)

    def test_threadsafety(self):
        self.assertEqual(0, pycmdb.threadsafety)

    def test_paramstyle(self):
        self.assertEqual('qmark', pycmdb.paramstyle)

    def test_error(self):
        conn = pycmdb.connect(tests.DATABASE)
        self.assertEqual(pycmdb.Warning, conn.Warning)
        self.assertEqual(pycmdb.Error, conn.Error)
        self.assertEqual(pycmdb.InterfaceError, conn.InterfaceError)
        self.assertEqual(pycmdb.DatabaseError, conn.DatabaseError)
        self.assertEqual(pycmdb.DataError, conn.DataError)
        self.assertEqual(pycmdb.OperationalError, conn.OperationalError)
        self.assertEqual(pycmdb.IntegrityError, conn.IntegrityError)
        self.assertEqual(pycmdb.InternalError, conn.InternalError)
        self.assertEqual(pycmdb.ProgrammingError, conn.ProgrammingError)
        self.assertEqual(pycmdb.NotSupportedError, conn.NotSupportedError)
        conn.close()

    def test_messages(self):
        conn = pycmdb.connect(tests.DATABASE)
        with self.assertRaises(AttributeError):
            a = conn.messages
        conn.close()

    def test_connection(self):
        conn = pycmdb.connect(tests.DATABASE)
        with self.assertRaises(AttributeError):
            a = conn.errorhandler
        conn.close()

    def test_connect_ok(self):
        conn = pycmdb.connect(tests.DATABASE)
        self.assertEqual(conn.database, tests.DATABASE)
        conn.close()
        conn = pycmdb.connect(tests.DATABASE)
        self.assertEqual(conn.database, tests.DATABASE)
        conn.close()

    def test_connect_ok_with_kwargs(self):
        conn = pycmdb.connect(database=tests.DATABASE)
        self.assertEqual(conn.database, tests.DATABASE)
        conn.close()

    def test_connect_error(self):
        with self.assertRaisesRegexp(pycmdb.InterfaceError,
                                     'connection failed'):
            pycmdb.connect(tests.BAD_DATABASE)

    def test_commit(self):
        conn = pycmdb.connect(tests.DATABASE)
        conn.commit()
        conn.close()

    def test_rollback(self):
        conn = pycmdb.connect(tests.DATABASE)
        conn.rollback()
        conn.close()

    def test_cursor(self):
        conn = pycmdb.connect(tests.DATABASE)
        cur = conn.cursor()
        conn.close()
        self.assertIsNotNone(cur)
