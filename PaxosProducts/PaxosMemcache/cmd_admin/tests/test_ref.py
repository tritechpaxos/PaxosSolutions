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
import sys
import StringIO
import yaml
from itertools import chain
from fabric.state import env

from paxos.cmd.pmemcache import css, error
from tests import utils
from tests import test_conf


cfn = test_conf['node']
cfc = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cfc]


class TestCssRef(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute('css', 'register', '-n', self.CSS1, *addr[0])

    def test_unregister_node(self):
        with self.assertRaises(error.InUseError):
            self.execute('host', 'unregister', cfc[0][0]['addr']['hostname'])


class TestCssRunning(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.node_register(cfn[9])
        self.node_register(cfn[10])
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('css', 'start', '-n', self.CSS1)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        sys.stderr = self.output
        try:
            self.execute('css', 'stop', '--force')
        finally:
            sys.stderr = sys.__stderr__

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def test_unregister(self):
        with self.assertRaises(error.NotStoppedError):
            self.execute('css', 'unregister', '-n', self.CSS1)


class TestMemcacheRef(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute('css', 'register', '-n', self.CSS1, *addr[0])
        self.css = css.Css.get(self.CSS1)
        self.execute('memcache', 'register', '-c', self.CSS1)

    def test_unregister_css(self):
        with self.assertRaises(error.InUseError):
            self.execute('css', 'unregister', '-n', self.CSS1)


class TestMemcacheRunning(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute_raw('deploy', '-g', 'css')
        self.css = css.Css.get(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.execute('memcache', 'start', '-i', '0')

    def tearDown(self):
        self.execute('memcache', 'stop', '--force')
        self.execute('css', 'stop', '--force')

    def test_unregister_memcache(self):
        with self.assertRaises(error.NotStoppedError):
            self.execute('memcache', 'unregister', '-i', '0')

    def test_css_stop(self):
        with self.assertRaises(error.InUseError):
            self.execute('css', 'stop')
