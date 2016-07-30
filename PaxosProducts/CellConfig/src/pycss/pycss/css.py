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
from past.builtins import basestring
from builtins import bytes
import os
import os.path
import stat
import errno
from time import sleep
from ._pycss import Css as _Css


class Css(object):

    no = 0

    def __init__(self, cell):
        self._css = _Css(cell, Css.no)
        self._cwd = os.sep
        Css.no += 1

    @property
    def cell(self):
        return self._css.cell

    def getcwd(self):
        return self._cwd

    def _normpath(self, path):
        return os.path.normpath(os.path.join(self._cwd, path))

    def _isdir(self, path):
        return (self.stat(path).st_mode & stat.S_IFDIR) != 0

    def _exists(self, path):
        try:
            self.stat(path)
            return True
        except OSError as e:
            if e.errno == errno.ENOENT:
                return False
            else:
                raise e

    def chdir(self, path):
        p = self._normpath(path)
        if self._isdir(p):
            self._cwd = p
        else:
            raise OSError(errno.ENOTDIR, os.strerror(errno.ENOTDIR), path)

    def mkdir(self, path):
        self._css.mkdir(self._normpath(path))

    def rmdir(self, path):
        self._css.rmdir(self._normpath(path))

    def listdir(self, path):
        return self._css.listdir(self._normpath(path))

    def stat(self, path):
        return self._css.stat(self._normpath(path))

    def remove(self, path):
        return self._css.remove(self._normpath(path))

    def unlink(self, path):
        return self._css.unlink(self._normpath(path))

    def open(self, path, mode='r'):
        if not isinstance(path, basestring):
            raise TypeError(
                "need str or unicode, {} found".format(path.__class__))
        ret = self._css.open(self._normpath(path), mode, self)
        try:
            sleep(0)
        except EnvironmentError as e:
            if e.errno == errno.ENOENT:
                pass
            else:
                raise e
        return ret
