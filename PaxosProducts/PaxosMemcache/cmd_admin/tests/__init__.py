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
import logging
import unittest
import yaml
import fabric.state

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

fh = logging.FileHandler('debug.log')
fh.setLevel(logging.DEBUG)
formatter = logging.Formatter(
    '%(asctime)s %(levelname)s:%(filename)s:%(lineno)d: %(message)s')
fh.setFormatter(formatter)
logging.getLogger().addHandler(fh)

unittest.installHandler()

with open('devel.yaml') as f:
    test_conf = yaml.load(f)

fabric.state.env.abort_on_prompts = True
for it in test_conf['node']:
    addr = '{0}@{1}:{2}'.format(it['user'], it['hostname'], 22)
    fabric.state.env.passwords[addr] = it['password']
