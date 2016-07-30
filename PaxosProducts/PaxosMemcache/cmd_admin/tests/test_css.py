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
import os.path
import yaml
import StringIO
import sys
import copy
from itertools import chain
import fabric.state

from paxos.cmd.pmemcache import config, css, error
from tests import utils, test_conf


cf = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cf]
cf0 = cf[0]
addr0 = addr[0]


class TestCssRegister(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        self.clear_local_conf_dir()

    def test_success(self):
        self.execute('css', 'register', '-n', self.CSS1, *addr0)
        result = css.Css.find()
        self.assertEqual(len(result), 1)
        css1 = css.Css.get(self.CSS1)
        self.assertEqual(css1.name, self.CSS1)
        self.assertEqual(css1.servers[0].uport, cf0[0]['uport'])
        self.assertEqual(css1.servers[0].tport, cf0[0]['tport'])
        self.assertEqual(css1.servers[1].uport, cf0[1]['uport'])
        self.assertEqual(css1.servers[1].tport, cf0[1]['tport'])
        self.assertEqual(css1.servers[2].uport, cf0[2]['uport'])
        self.assertEqual(css1.servers[2].tport, cf0[2]['tport'])
        conf_path = os.path.expanduser(
            config.CONFIG_CELL_FORMAT.format(cell=css1.name))
        self.assertTrue(os.path.exists(conf_path))
        with open(conf_path) as f:
            lines = f.read().splitlines()
        self.assertEqual(len(lines), 3)
        srv = [line.split() for line in lines]
        self.assertEqual(int(srv[0][0]), 0)
        self.assertEqual(srv[0][1], cf0[0]['addr']['hostname'])
        self.assertEqual(int(srv[0][2]), cf0[0]['uport'])
        self.assertEqual(srv[0][3], cf0[0]['addr']['hostname'])
        self.assertEqual(int(srv[0][4]), cf0[0]['tport'])
        self.assertEqual(int(srv[1][0]), 1)
        self.assertEqual(srv[1][1], cf0[1]['addr']['hostname'])
        self.assertEqual(int(srv[1][2]), cf0[1]['uport'])
        self.assertEqual(srv[1][3], cf0[1]['addr']['hostname'])
        self.assertEqual(int(srv[1][4]), cf0[1]['tport'])
        self.assertEqual(int(srv[2][0]), 2)
        self.assertEqual(srv[2][1], cf0[2]['addr']['hostname'])
        self.assertEqual(int(srv[2][2]), cf0[2]['uport'])
        self.assertEqual(srv[2][3], cf0[2]['addr']['hostname'])
        self.assertEqual(int(srv[2][4]), cf0[2]['tport'])

    def test_err_exists(self):
        self.execute('css', 'register', '-n', self.CSS1, *addr0)
        with self.assertRaises(error.ExistError):
            self.execute('css', 'register', '-n', self.CSS1, *addr0)

    def test_bad_name(self):
        with self.assertRaises(error.IllegalNameError):
            self.execute('css', 'register', '-n', 'xxx:100', *addr0)

    def test_bad_port1(self):
        with self.assertRaises(error.IllegalParamError):
            bad_addr = copy.copy(addr0)
            bad_addr[1] = '{0}:{1}'.format(cf0[0]['addr']['hostname'], '1400x')
            self.execute('css', 'register', '-n', 'xxxxxx', *bad_addr)

    def test_bad_port2(self):
        with self.assertRaises(error.IllegalParamError):
            bad_addr = copy.copy(addr0)
            bad_addr[0] = '{0}:{1}'.format(cf0[0]['addr']['hostname'], '1400x')
            self.execute('css', 'register', '-n', 'xxxxxx', *bad_addr)

    def test_bad_port3(self):
        with self.assertRaises(error.IllegalParamError):
            bad_addr = copy.copy(addr0)
            bad_addr[0] = cf0[0]['addr']['hostname']
            self.execute('css', 'register', '-n', 'xxxxxx', *bad_addr)

    def test_bad_port4(self):
        with self.assertRaises(error.IllegalParamError):
            bad_addr = copy.copy(addr0)
            bad_addr[1] = cf0[0]['addr']['hostname']
            self.execute('css', 'register', '-n', 'xxxxxx', *bad_addr)

    def test_same_server(self):
        self.execute('css', 'register', '-n', 'css1', *addr[2])
        self.execute('css', 'register', '-n', 'css2', *addr[3])
        with self.assertRaises(error.DuplicationError):
            self.execute('css', 'register', '-n', 'xxxxxx', *addr[4])


class TestCssUnregister(utils._PaxosTest):

    CSS1 = 'css1'
    CSSX = 'cssX'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute('css', 'register', '-n', self.CSS1, *addr0)

    def test_success(self):
        self.execute('css', 'unregister', '-n', self.CSS1)
        conf_path = os.path.expanduser(
            config.CONFIG_CELL_FORMAT.format(cell=self.CSS1))
        self.assertFalse(os.path.exists(conf_path))
        self.assertFalse(css.Css.exist(self.CSS1))

    def test_error_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('css', 'unregister', '-n', self.CSSX)


class TestCssShow(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    CSSX = 'cssX'

    def setUp(self):
        super(TestCssShow, self).setUp()
        self.clear_local_conf_dir()
        self.execute('css', 'register', '-n', self.CSS1, *addr0)
        self.execute('css', 'register', '-n', self.CSS2, *addr[1])
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def test_default(self):
        result = self.exec_and_result('css', 'show')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['name'], self.CSS1)
        self.assertEqual(result[1]['name'], self.CSS2)

    def test_name1(self):
        result = self.exec_and_result('css', 'show', '-n', self.CSS1)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['name'], self.CSS1)

    def test_name2(self):
        result = self.exec_and_result('css', 'show', '-n', self.CSS2)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['name'], self.CSS2)

    def test_name0(self):
        result = self.exec_and_result('css', 'show', '-n', self.CSSX)
        self.assertEqual(len(result), 0)


class TestCssShow(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        super(TestCssShow, self).setUp()
        self.clear_local_conf_dir()
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return self.output.getvalue().splitlines()

    def _regist_css(self):
        self.execute('css', 'register', '-n', self.CSS1, *addr0)
        self.execute('css', 'register', '-n', self.CSS2, *addr[1])

    def test_default(self):
        self._regist_css()
        result = self.exec_and_result('css', 'list')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0], self.CSS1)
        self.assertEqual(result[1], self.CSS2)

    def test_empty(self):
        result = self.exec_and_result('css', 'list')
        self.assertEqual(len(result), 0)
