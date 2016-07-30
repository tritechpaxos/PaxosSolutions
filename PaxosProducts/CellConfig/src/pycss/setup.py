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
from setuptools import setup, Extension
from pycss import __version__


_pycss = Extension(
    'pycss._pycss',
    include_dirs=['paxos/include'],
    libraries=['PaxosPFS', 'PaxosSession', 'Paxos', 'NWGadget', 'event'],
    library_dirs=['paxos/lib'],
    sources=['src/module.c', 'src/css.c', 'src/file.c'])


setup(
    name='pycss',
    version=__version__,
    packages=['pycss'],
    ext_modules=[_pycss],
    test_suite="tests",
    install_requires=['future'],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audienve :: Developers',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C',
        'Programming Language :: Pythone :: 2.7',
        'Programming Language :: Pythone :: 3.4',
        'Programming Language :: Pythone :: 3.5',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
)
