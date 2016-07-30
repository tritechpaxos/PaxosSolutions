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
import unittest
import pycss


class TestEmptyFile(unittest.TestCase):

    CELL = 'css00'
    FILE = 'zzz'

    def setUp(self):
        self.css = pycss.connect(self.CELL)
        with self.css.open(self.FILE, 'w') as f:
            f.write('')

    def tearDown(self):
        self.css.unlink(self.FILE)
        self.css = None

    def test_read(self):
        st = self.css.stat(self.FILE)
        self.assertEqual(0, st.st_size)
        with self.css.open(self.FILE) as f:
            f.read()


class TestOverwriteFile(unittest.TestCase):

    CELL = 'css00'
    FILE = 'zz00'

    def setUp(self):
        self.css = pycss.connect(self.CELL)
        with self.css.open(self.FILE, 'w') as f:
            f.write('x' * 100)

    def tearDown(self):
        self.css.unlink(self.FILE)
        self.css = None

    def test_write(self):
        with self.css.open(self.FILE, 'w') as f:
            f.write('aaa')

        with self.css.open(self.FILE) as f:
            line = f.read()

        self.assertEqual('aaa', line)
