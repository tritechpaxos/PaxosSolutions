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
from setuptools import setup, Extension
#from distutils.core import setup, Extension

_pycmdb = Extension('pycmdb._pycmdb',
                    include_dirs=['paxos/include'],
                    libraries=['neo2', 'PaxosSession', 'Paxos', 'NWGadget',
                               'event'],
                    library_dirs=['paxos/lib'],
                    sources=['src/module.c', 'src/connection.c',
                             'src/cursor.c', 'src/adapter.c'])

setup(
    name='pycmdb',
    version='0.2.1',
    description='It is a client library for python of CMDB.',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audienve :: Developers',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C',
        'Programming Language :: Pythone :: 2.6',
        'Programming Language :: Pythone :: 2.7',
        'Topic :: Database :: Database Engines/Servers',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    packages=['pycmdb'],
    ext_modules=[_pycmdb],
    test_suite="tests",
)
