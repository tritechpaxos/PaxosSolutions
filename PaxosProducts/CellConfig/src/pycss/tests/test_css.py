# -*- coding: utf-8 -*-
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
import random
import os
import os.path
import stat
import errno
import pycss


class TestCss(unittest.TestCase):

    CELL = 'css00'

    def setUp(self):
        self.css = pycss.connect(TestCss.CELL)

    def tearDown(self):
        self.css = None

    def test_mkdir_rmdir_abs(self):
        path0 = '/xxx'
        path1 = ''.join([random.choice('abcdefghij') for x in range(15)])
        path = os.path.join(path0, path1)
        self.css.mkdir(path0)
        self.css.mkdir(path)
        self.assertTrue(path1 in self.css.listdir(path0))
        self.css.rmdir(path)
        self.assertTrue(path not in self.css.listdir(path0))
        self.css.rmdir(path0)

    def test_mkdir_rmdir(self):
        path0 = '/xxx'
        path1 = ''.join([random.choice('abcdefghij') for x in range(15)])
        self.css.mkdir(path0)
        self.css.chdir(path0)
        self.css.mkdir(path1)
        self.assertTrue(path1 in self.css.listdir('.'))
        self.css.rmdir(path1)
        self.assertTrue(path1 not in self.css.listdir('.'))
        self.css.rmdir(path0)

    def test_mkdir_rmdir_unicode(self):
        path = u'あいう'
        self.css.mkdir(path)
        self.assertTrue(
            path in
            [i if not isinstance(i, bytes) else i.decode('utf-8')
             for i in self.css.listdir('.')])
        self.css.rmdir(path)
        self.assertTrue(
            path not in
            [i if not isinstance(i, bytes) else i.decode('utf-8')
             for i in self.css.listdir('.')])

    def test_mkdir_twice(self):
        path = ''.join([random.choice('abcdefghij') for x in range(15)])
        self.css.mkdir(path)
        self.css.mkdir(path)
        self.css.rmdir(path)

    def test_rmdir_enoent(self):
        path = ''.join([random.choice('abcdefghij') for x in range(15)])
        with self.assertRaises(OSError) as cm:
            self.css.rmdir(path)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        self.assertEqual(cm.exception.filename, os.path.join('/', path))

    def test_chdir(self):
        paths = [''.join([random.choice('abcdefghij') for i in range(15)])
                 for j in range(3)]
        apath = '/'
        for path in paths:
            self.css.mkdir(path)
            self.css.chdir(path)
            apath = os.path.join(apath, path)
            self.assertEqual(apath, self.css.getcwd())

        (apath, name) = os.path.split(self.css.getcwd())
        while name:
            self.css.chdir('..')
            self.css.rmdir(name)
            (apath, name) = os.path.split(apath)

    def test_stat_dir(self):
        path = ''.join([random.choice('abcdefghij') for x in range(15)])
        self.css.mkdir(path)
        st = self.css.stat(path)
        self.assertTrue((st.st_mode & stat.S_IFDIR) != 0)
        self.css.rmdir(path)
