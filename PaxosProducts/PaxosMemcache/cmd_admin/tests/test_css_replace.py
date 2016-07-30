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
import time
import unittest
from itertools import chain
from fabric.state import env, output
from fabric.tasks import execute

from paxos.cmd.pmemcache import config, css, error
from paxos.cmd.pmemcache import fabfile as pfabfile
from tests import utils
from tests import fabfile
from tests import test_conf


cfn = test_conf['node']
cfc = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cfc]


class TestCssReplace(utils._PaxosTest):

    CSS1 = 'css1'

    def setUp(self):
        super(TestCssReplace, self).setUp()
        env.password = 'paxos00'
        self.clear_local_conf_dir()
        self.node_register(cfc[10][0]['addr'], '-g', 'css')
        self.node_register(cfc[10][1]['addr'], '-g', 'css')
        self.node_register(cfc[10][2]['addr'], '-g', 'css')
        self.node_register(cfc[10][3]['addr'], '-g', 'css')
        self.node_register(cfn[10])
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute_raw('deploy', '-g', 'css')

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[10]]
        execute(fabfile.clear_data)
        self.execute('css', 'start', '-n', self.CSS1)

        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop')
        except:
            pass
        finally:
            super(TestCssReplace, self).tearDown()

    def _lstop(self, name, no):
        c = css.Css.get(name)
        srv = c.find_server(no)
        n = srv.taddr
        output['everything'] = False
        output['aborts'] = False
        env.hosts = [n.hostname]
        env.user = n.user
        try:
            execute(pfabfile.css_stop, name, no)
        except error.FabError as e:
            pass

    def exec_and_result(self, *cmd):
        self.output = StringIO.StringIO()
        sys.stdout = self.output
        try:
            self.execute(*cmd)
            return [x for x in yaml.load_all(self.output.getvalue())]
        finally:
            sys.stdout = sys.__stdout__

    def test_replace_Ooo(self):
        self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                         addr[10][6], addr[10][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[10][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[1]['address'], cfc[10][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[2]['address'], cfc[10][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_RUNNING)

    def test_replace_Xoo(self):
        self._lstop(self.CSS1, 0)
        self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                         addr[10][6], addr[10][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[10][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[1]['address'], cfc[10][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[2]['address'], cfc[10][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_RUNNING)

    def test_replace_Oxo(self):
        self._lstop(self.CSS1, 1)
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[10][6], addr[10][7])

    def test_replace_Xxo(self):
        self._lstop(self.CSS1, 0)
        self._lstop(self.CSS1, 1)
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[10][6], addr[10][7])

    def test_replace_Oxx(self):
        self._lstop(self.CSS1, 1)
        self._lstop(self.CSS1, 2)
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[10][6], addr[10][7])

    def test_replace_Xxx(self):
        self.execute('css', 'stop', '-n', self.CSS1)
        self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                         addr[10][6], addr[10][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[10][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(
            sv_result[1]['address'], cfc[10][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(
            sv_result[2]['address'], cfc[10][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_STOPPED)

    def test_replace_Abc_A(self):
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[10][0], addr[10][1])

    def test_replace_Abc_b(self):
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[10][2], addr[10][3])


class TestCssReplace2(utils._PaxosTest):

    CSS1 = 'css'

    def setUp(self):
        super(TestCssReplace2, self).setUp()
        env.password = 'paxos00'
        self.clear_local_conf_dir()
        self.node_register(cfc[12][0]['addr'])
        self.node_register(cfc[12][1]['addr'], '-g', 'css')
        self.node_register(cfc[12][2]['addr'], '-g', 'css')
        self.node_register(cfc[12][3]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[11])
        self.execute_raw('deploy', '-g', 'css')

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[12][1:]]
        execute(fabfile.clear_data)
        self.execute('css', 'start', '-n', self.CSS1)

        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop')
        except:
            pass
        finally:
            super(TestCssReplace2, self).tearDown()

    def _lstop(self, name, no):
        c = css.Css.get(name)
        srv = c.find_server(no)
        n = srv.taddr
        output['everything'] = False
        output['aborts'] = False
        env.hosts = [n.hostname]
        env.user = n.user
        try:
            execute(pfabfile.css_stop, name, no)
        except error.FabError as e:
            pass

    def exec_and_result(self, *cmd):
        self.output = StringIO.StringIO()
        sys.stdout = self.output
        try:
            self.execute(*cmd)
            return [x for x in yaml.load_all(self.output.getvalue())]
        finally:
            sys.stdout = sys.__stdout__

    def test_replace_eOo(self):
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '1', '-a',
                             addr[12][6], addr[12][7])

    def test_replace_eXo(self):
        self._lstop(self.CSS1, 1)
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '1', '-a',
                             addr[12][6], addr[12][7])

    def test_replace_Eoo(self):
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[12][6], addr[12][7])
        servers = css.Css.get(self.CSS1).servers
        self.assertEqual(len(servers), 3)

    def _stop_css(self):
        self.execute('css', 'stop', '-n', self.CSS1)

    def test_replace_Exx(self):
        self._stop_css()
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[12][6], addr[12][7])
        servers = css.Css.get(self.CSS1).servers
        self.assertEqual(len(servers), 3)

    def test_replace_eXx(self):
        self._stop_css()
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '1', '-a',
                             addr[12][6], addr[12][7])
        servers = css.Css.get(self.CSS1).servers
        self.assertEqual(len(servers), 3)

    def test_replace_Eoo_force(self):
        self.execute_raw('replace', '--force', '-n', self.CSS1, '-N', '0',
                         '-a', addr[12][6], addr[12][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[12][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[1]['address'], cfc[12][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[2]['address'], cfc[12][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_RUNNING)

    def test_replace_Exx_force(self):
        self._stop_css()
        self.execute_raw('replace', '--force', '-n', self.CSS1, '-N', '0',
                         '-a', addr[12][6], addr[12][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[12][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(
            sv_result[1]['address'], cfc[12][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(
            sv_result[2]['address'], cfc[12][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_STOPPED)

    def test_replace_eXx_force(self):
        self._stop_css()
        self.execute_raw('replace', '--force', '-n', self.CSS1, '-N', '1',
                         '-a', addr[12][6], addr[12][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[12][0]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_ERROR)
        self.assertEqual(
            sv_result[1]['address'], cfc[12][3]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(
            sv_result[2]['address'], cfc[12][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_STOPPED)


class TestCssReplace3(utils._PaxosTest):

    CSS1 = 'css'

    def setUp(self):
        super(TestCssReplace3, self).setUp()
        env.password = 'paxos00'
        self.clear_local_conf_dir()
        self.node_register(cfc[12][0]['addr'])
        self.node_register(cfc[12][1]['addr'], '-g', 'css')
        self.node_register(cfc[12][2]['addr'], '-g', 'css')
        self.node_register(cfc[12][3]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[12][2:])
        self.execute_raw('deploy', '-g', 'css')

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[12][1:]]
        execute(fabfile.clear_data)

        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop')
        except:
            pass
        finally:
            super(TestCssReplace3, self).tearDown()

    def test_replace_oooE(self):
        self.execute('css', 'start', '-n', self.CSS1)
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[12][0], addr[12][1])
        servers = css.Css.get(self.CSS1).servers
        self.assertEqual(len(servers), 3)

    def test_replace_xxxE(self):
        with self.assertRaises(error.IllegalStateError):
            self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                             addr[12][0], addr[12][1])
        servers = css.Css.get(self.CSS1).servers
        self.assertEqual(len(servers), 3)


class TestCssReplace4(utils._PaxosTest):

    CSS1 = 'css'

    def setUp(self):
        super(TestCssReplace4, self).setUp()
        env.password = 'paxos00'
        self.clear_local_conf_dir()
        self.node_register(cfc[13][0]['addr'], '-g', 'css')
        self.node_register(cfc[13][1]['addr'], '-g', 'css')
        self.node_register(cfc[13][2]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[13][:6])
        self.execute_raw('deploy', '-g', 'css')

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[13]]
        execute(fabfile.clear_data)

        self.execute('css', 'start', '-n', self.CSS1)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop')
        except:
            pass
        finally:
            super(TestCssReplace4, self).tearDown()

    def exec_and_result(self, *cmd):
        self.output = StringIO.StringIO()
        sys.stdout = self.output
        try:
            self.execute(*cmd)
            return [x for x in yaml.load_all(self.output.getvalue())]
        finally:
            sys.stdout = sys.__stdout__

    def test_replace_Abc_a(self):
        self.execute_raw('replace', '-n', self.CSS1, '-N', '0', '-a',
                         addr[13][6], addr[13][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[13][3]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[1]['address'], cfc[13][1]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[2]['address'], cfc[13][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_RUNNING)

    def test_replace_aBc_A(self):
        self.execute_raw('replace', '-n', self.CSS1, '-N', '1', '-a',
                         addr[13][6], addr[13][7])
        ret = self.exec_and_result('css', 'status', '--long')
        result = ret[0]
        sv_result = result['server']
        self.assertEqual(len(sv_result), 3)
        self.assertEqual(
            sv_result[0]['address'], cfc[13][0]['addr']['hostname'])
        self.assertEqual(sv_result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[1]['address'], cfc[13][3]['addr']['hostname'])
        self.assertEqual(sv_result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(
            sv_result[2]['address'], cfc[13][2]['addr']['hostname'])
        self.assertEqual(sv_result[2]['status'], config.PROC_STATE_RUNNING)
