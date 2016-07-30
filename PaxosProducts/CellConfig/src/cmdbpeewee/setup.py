###########################################################################
#   cmdbpeewee Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of cmdbpeewee.
#
#   Cmdbpeewee is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cmdbpeewee is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with cmdbpeewee.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from setuptools import setup

setup(
    name='cmdbpeewee',
    version='0.2.1',
    description='peewee class for CMDB',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audienve :: Developers',
        'Operating System :: OS Independent',
        'Programming Language :: Pythone :: 2.6',
        'Programming Language :: Pythone :: 2.7',
        'Topic :: Database',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    packages=['cmdbpeewee'],
    install_requires=['peewee', 'pycmdb'],
    test_suite="tests",
)
