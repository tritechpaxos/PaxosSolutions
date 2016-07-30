###########################################################################
#   PaxosMemcache Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of PaxosMemcache.
#
#   PaxosMemcache is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   PaxosMemcache is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with PaxosMemcache.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from setuptools import setup
from paxos.cmd.pmemcache import __version__

setup(
    name='paxos_memcached_cmd',
    version=__version__,
    packages=['paxos', 'paxos.cmd', 'paxos.cmd.pmemcache'],
    entry_points={'console_scripts': ['paxos=paxos.cmd.pmemcache.main:main']},
    include_package_data=True,
    test_suite="tests",
    setup_requires=['Babel'],
)
