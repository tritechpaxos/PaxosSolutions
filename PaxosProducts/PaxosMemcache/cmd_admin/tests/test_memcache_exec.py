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
import socket
import unittest
from time import sleep
from itertools import chain
from fabric.state import env
from subprocess import check_output

from paxos.cmd.pmemcache import config, css, memcache, error
from tests import utils
from tests import test_conf


cfn = test_conf['node']
cfc = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cfc]
cfm = test_conf['memcache']


def _css_running(css_name):
    cmd = css.StateCommand(name=css_name, mode='short')
    result = cmd.exec_short()
    if result is None or css_name not in result:
        return False
    status = result[css_name]
    if isinstance(status, list):
        return config.PROC_STATE_RUNNING in status
    else:
        return status == config.PROC_STATE_RUNNING


class _TestMemcache(utils._PaxosTest):

    CSS = 'css'

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def setUp(self):
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS, *addr[5])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('css', 'start', '-n', self.CSS, '--initialize')
        sleep(3)
        self.assertTrue(_css_running(self.CSS))
        self.execute('memcache', 'register', '-c', self.CSS)
        self.execute('memcache', 'register', '-c', self.CSS, '-i', '10')
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        sys.stderr = self.output
        try:
            self.execute('memcache', 'stop', '--force')
        finally:
            os.system("killall PaxosMemcache > /dev/null 2>&1")
        try:
            self.execute('css', 'stop', '--force')
        finally:
            sys.stderr = sys.__stderr__


class TestMemcacheStart(_TestMemcache):

    CSSX = 'cssX'

    def test_default(self):
        self.execute('memcache', 'start')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[1]['status'], config.PROC_STATE_RUNNING)

    def test_id(self):
        self.execute('memcache', 'start', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['status'], config.PROC_STATE_RUNNING)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)

    def test_idX(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'start', '-i', '99')

    def test_dup(self):
        self.execute('memcache', 'start', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['status'], config.PROC_STATE_RUNNING)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)
        self.output.close()

        self.output = StringIO.StringIO()
        sys.stdout = self.output
        self.execute('memcache', 'start', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['status'], config.PROC_STATE_RUNNING)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)


class TestMemcacheStop(_TestMemcache):

    def setUp(self):
        super(TestMemcacheStop, self).setUp()
        self.execute('memcache', 'start')

    def test_default(self):
        self.execute('memcache', 'stop')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['status'], config.PROC_STATE_STOPPED)

    def test_id(self):
        self.execute('memcache', 'stop', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[1]['status'], config.PROC_STATE_STOPPED)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)

    def test_idX(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'stop', '-i', '99')

    def test_dup(self):
        self.execute('memcache', 'stop', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[1]['status'], config.PROC_STATE_STOPPED)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)
        self.output.close()

        self.output = StringIO.StringIO()
        sys.stdout = self.output
        self.execute('memcache', 'stop', '-i', '10')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[1]['status'], config.PROC_STATE_STOPPED)
        lines = check_output(["pgrep", "PaxosMemcache"]).splitlines()
        self.assertEqual(len(lines), 1)


class TestMemcacheState(_TestMemcache):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute('css', 'register', '-n', self.CSS2, *addr[6])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('css', 'start', '-n', self.CSS1)
        self.execute('css', 'start', '-n', self.CSS2)
        sleep(3)
        self.assertTrue(_css_running(self.CSS1))
        self.assertTrue(_css_running(self.CSS2))
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache1 = memcache.Memcache.get(0)
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', cfm[0][0]['saddr'])
        self.execute_raw(
            'register', 'pair', '-i', str(self.memcache1.id),
            '-a', cfm[0][1]['saddr'], '-p', str(cfm[0][1]['sport']),
            '-A', cfm[0][1]['caddr'], '-P', str(cfm[0][1]['cport']))
        self.execute('memcache', 'register', '-c', self.CSS2, '-i', '10')
        self.memcache2 = memcache.Memcache.get(10)
        self.execute('memcache', 'start')
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache3 = memcache.Memcache.get(11)
        self.execute_raw(
            'register', 'pair', '-i', str(self.memcache3.id),
            '-a', cfm[1][0]['saddr'], '-p', str(cfm[1][0]['sport']),
            '-P', str(cfm[1][0]['cport']))
        self.output = StringIO.StringIO()
        sys.stdout = self.output
        sleep(1)

    def test_default(self):
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css1')
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[1]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[1]['id'], 10)
        self.assertEqual(result[1]['css'], 'css2')
        self.assertEqual(result[2]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[2]['id'], 11)

    def test_css(self):
        result = self.exec_and_result('memcache', 'status', '-c', self.CSS1)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css1')
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[1]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['id'], 11)

    def test_id0(self):
        result = self.exec_and_result('memcache', 'status', '-i', '0')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css1')
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2],
                         socket.gethostbyname(cfm[0][0]['saddr']))
        self.assertEqual(result[0]['pair'][0][3], 11211)
        self.assertEqual(result[0]['pair'][1][0],
                         socket.gethostbyname(cfm[0][1]['caddr']))
        self.assertEqual(result[0]['pair'][1][1], cfm[0][1]['cport'])
        self.assertEqual(result[0]['pair'][1][2],
                         socket.gethostbyname(cfm[0][1]['saddr']))
        self.assertEqual(result[0]['pair'][1][3], cfm[0][1]['sport'])

    def test_id1(self):
        result = self.exec_and_result('memcache', 'status', '-i', '10')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 10)

    def test_id2(self):
        result = self.exec_and_result('memcache', 'status', '-i', '11')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['status'], config.PROC_STATE_STOPPED)
        self.assertEqual(result[0]['id'], 11)

    def test_empty(self):
        try:
            self.execute('memcache', 'stop')
        except:
            pass
        self.execute('memcache', 'unregister', '-i', '0')
        self.execute('memcache', 'unregister', '-i', '10')
        self.execute('memcache', 'unregister', '-i', '11')
        ret = self.exec_and_result('memcache', 'status')

    def test_short(self):
        ret = self.exec_and_result('memcache', 'status', '-s')
        self.assertEqual(len(ret), 1)
        result = ret[0]
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0], config.PROC_STATE_RUNNING)
        self.assertEqual(result[10], config.PROC_STATE_RUNNING)
        self.assertEqual(result[11], config.PROC_STATE_STOPPED)

    def test_short_css(self):
        ret = self.exec_and_result('memcache', 'status', '-s', '-c', self.CSS1)
        self.assertEqual(len(ret), 1)
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0], config.PROC_STATE_RUNNING)
        self.assertEqual(result[11], config.PROC_STATE_STOPPED)

    def test_short_id0(self):
        ret = self.exec_and_result('memcache', 'status', '-s', '-i', '0')
        self.assertEqual(len(ret), 1)
        result = ret[0]
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0], config.PROC_STATE_RUNNING)

    def test_short_id2(self):
        ret = self.exec_and_result('memcache', 'status', '-s', '-i', '11')
        self.assertEqual(len(ret), 1)
        result = ret[0]
        self.assertEqual(len(result), 1)
        self.assertEqual(result[11], config.PROC_STATE_STOPPED)

    def test_quiet_stopped(self):
        ret = self.execute('memcache', 'status', '-q')
        self.assertNotEqual(ret, 0)

    def test_quiet_running(self):
        self.execute('memcache', 'unregister', '-i', '11')
        ret = self.execute('memcache', 'status', '-q')
        self.assertEqual(ret, 0)

    def test_quiet_id1(self):
        ret = self.execute('memcache', 'status', '-q', '-i', '10')
        self.assertEqual(ret, 0)

    def test_quiet_id2(self):
        ret = self.execute('memcache', 'status', '-q', '-i', '11')
        self.assertNotEqual(ret, 0)


class TestMemcachePairApply(_TestMemcache):

    def test_add_pair(self):
        self.execute('memcache', 'start')
        self.execute_raw('register', 'pair', '-i', '0',
                         '-a', cfm[0][0]['saddr'])
        self.execute_raw('apply', 'pair')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css')
        self.assertEqual(len(result[0]['pair']), 1)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2],
                         socket.gethostbyname(cfm[0][0]['saddr']))
        self.assertIsNone(result[1]['pair'])

    def test_nop_pair(self):
        self.execute('memcache', 'start')
        self.execute_raw('register', 'pair', '-i', '0',
                         '-a', cfm[0][0]['saddr'])
        self.execute_raw('apply', 'pair')
        self.execute_raw('apply', 'pair')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css')
        self.assertEqual(len(result[0]['pair']), 1)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2],
                         socket.gethostbyname(cfm[0][0]['saddr']))
        self.assertIsNone(result[1]['pair'])

    def test_remove_pair(self):
        self.execute_raw('register', 'pair', '-i', '0',
                         '-a', cfm[0][0]['saddr'])
        self.execute('memcache', 'start')
        self.execute_raw('unregister', 'pair', '-i', '0',
                         '-a', cfm[0][0]['saddr'])
        self.execute_raw('apply', 'pair')
        result = self.exec_and_result('memcache', 'status')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['status'], config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], 'css')
        self.assertIsNone(result[0]['pair'])
        self.assertIsNone(result[1]['pair'])

    def test_stopped_memcache(self):
        self.execute('memcache', 'stop', '--force')
        self.execute_raw('register', 'pair', '-i', '0',
                         '-a', cfm[0][0]['saddr'])
        self.execute_raw('apply', 'pair')
