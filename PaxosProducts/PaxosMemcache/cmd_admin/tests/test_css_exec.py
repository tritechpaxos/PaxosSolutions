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
import logging
from itertools import chain
from fabric.state import env, output
from fabric.tasks import execute
import fabric.network

from paxos.cmd.pmemcache import config, css, error
from tests import utils, test_conf, fabfile
import paxos.cmd.pmemcache as pm


cfn = test_conf['node']
cfc = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cfc]


class TestCssStart(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    CSSZ = 'cssZ'
    CSSZ2 = 'cssZ2'

    def setUp(self):
        super(TestCssStart, self).setUp()
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.node_register(cfn[9])
        self.node_register(cfn[10])

        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute('css', 'register', '-n', self.CSS2, *addr[6])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('memcache', 'register', '-c', self.CSS1)

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[5]]
        execute(fabfile.clear_data)

        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop', '--force')
        finally:
            super(TestCssStart, self).tearDown()

    def exec_and_result(self, *cmd):
        self.output = StringIO.StringIO()
        sys.stdout = self.output
        try:
            self.execute(*cmd)
            return [x for x in yaml.load_all(self.output.getvalue())]
        finally:
            sys.stdout = sys.__stdout__

    def _regist_cssz(self):
        self.execute('css', 'register', '-n', self.CSSZ, *addr[7])
        self.execute_raw('deploy', '-g', 'css')

    def _regist_cssz2(self):
        self.execute('css', 'register', '-n', self.CSSZ2, *addr[8])
        self.execute_raw('deploy', '-g', 'css')

    def check_pfs_dirs(self, name, exists=True):
        c = css.Css.get(name)
        output['everything'] = False
        for srv in c.servers:
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            ret = execute(fabfile.check_pfs_dirs, c, srv.no, exists)
            self.assertTrue(ret[addr])

    def check_pfs_seq(self, name, seq_map):
        c = css.Css.get(name)
        output['everything'] = False
        for srv in c.servers:
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            if isinstance(seq_map, dict):
                seq = seq_map[addr]
            else:
                seq = seq_map
            if seq is not None:
                ret = execute(fabfile.check_pfs_seq, c, srv.no, 'TRUE', seq)
            else:
                ret = execute(fabfile.check_pfs_seq, c, srv.no, 'FALSE')
            self.assertTrue(ret[addr])

    def get_pfs_seq(self, name):
        c = css.Css.get(name)
        output['everything'] = False
        result = {}
        for srv in c.servers:
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            ret = execute(fabfile.get_pfs_seq, c, srv.no)
            result[addr] = ret[addr]
        return result

    def get_pfs_start(self, name):
        c = css.Css.get(name)
        output['everything'] = False
        result = {}
        for srv in c.servers:
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            ret = execute(fabfile.get_pfs_start, c, srv.no)
            result[srv.no] = ret[addr]
        return result

    def remove_shutdown(self, name, no):
        c = css.Css.get(name)
        output['everything'] = False
        for srv in c.servers:
            if srv.no not in no:
                continue
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            execute(fabfile.remove_shutdown, c, srv.no)

    def clear_data_srv(self, name, no):
        c = css.Css.get(name)
        output['everything'] = False
        for srv in c.servers:
            if srv.no not in no:
                continue
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            execute(fabfile.clear_data_srv, c, srv.no)

    def rewrite_shutdown(self, name, vals):
        c = css.Css.get(name)
        output['everything'] = False
        for srv in c.servers:
            addr = '{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname)
            env.hosts = [addr]
            execute(fabfile.rewrite_shutdown, c, srv.no, vals[addr])

    def test_start_init(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        self.execute('css', 'stop', '-n', self.CSS1)
        self.remove_shutdown(self.CSS1, [0, 1, 2])
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1, '--initialize')
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, 0)

    def test_start_000(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)
        self.check_pfs_dirs(self.CSS2, False)

    def test_start_555(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        self.execute('css', 'stop', '-n', self.CSS1)
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, seq_list)

    def test_start_55x(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        self.remove_shutdown(self.CSS1, [2])
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, seq_list)

    def test_start_5xx(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        self.remove_shutdown(self.CSS1, [1, 2])
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, seq_list)

    def test_start_xxx(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        self.remove_shutdown(self.CSS1, [0, 1, 2])
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, 0)

    def test_start_ooo(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)

    def test_start_0xx(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        self.remove_shutdown(self.CSS1, [1, 2])
        self.clear_data_srv(self.CSS1, [0])
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_seq(self.CSS1, seq_list)

    def test_start_543(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        c = css.Css.get(self.CSS1)
        vals = dict([('{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname),
                      5 - idx)
                     for idx, srv in enumerate(c.servers)])
        self.rewrite_shutdown(self.CSS1, vals)
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        time.sleep(3)
        self.check_pfs_seq(self.CSS1, seq_list)
        ret = self.exec_and_result('css', 'status', '-s', '-n', self.CSS1)
        self.assertEqual(ret[0][self.CSS1], config.PROC_STATE_RUNNING)
#        result = self.get_pfs_start(self.CSS1)
#        self.assertTrue(result[0] < result[1])
#        self.assertTrue(result[0] < result[2])
#        self.assertTrue(result[1] < result[2])

    def test_start_345(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        c = css.Css.get(self.CSS1)
        vals = dict([('{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname),
                      3 + idx)
                     for idx, srv in enumerate(c.servers)])
        self.rewrite_shutdown(self.CSS1, vals)
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        time.sleep(3)
        self.check_pfs_seq(self.CSS1, seq_list)
        ret = self.exec_and_result('css', 'status', '-s', '-n', self.CSS1)
        self.assertEqual(ret[0][self.CSS1], config.PROC_STATE_RUNNING)
#        result = self.get_pfs_start(self.CSS1)
#        self.assertTrue(result[0] > result[1])
#        self.assertTrue(result[0] > result[2])
#        self.assertTrue(result[1] > result[2])

    def test_start_34x(self):
        self.execute('css', 'start', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.check_pfs_dirs(self.CSS1)
        self.check_pfs_seq(self.CSS1, 0)
        self.execute('css', 'stop', '-n', self.CSS1)
        time.sleep(1)
        c = css.Css.get(self.CSS1)
        vals = dict([('{0}@{1}'.format(srv.taddr.user, srv.taddr.hostname),
                      3 + idx)
                     for idx, srv in enumerate(c.servers)])
        self.rewrite_shutdown(self.CSS1, vals)
        self.remove_shutdown(self.CSS1, [2])
        seq_list = self.get_pfs_seq(self.CSS1)
        self.execute('css', 'start', '-n', self.CSS1)
        time.sleep(3)
        self.check_pfs_seq(self.CSS1, seq_list)
        ret = self.exec_and_result('css', 'status', '-s', '-n', self.CSS1)
        self.assertEqual(ret[0][self.CSS1], config.PROC_STATE_RUNNING)


class TestCssStop(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    CSSX = 'cssX'
    CSSZ = 'cssZ'
    CSSZ2 = 'cssZ2'

    def setUp(self):
        super(TestCssStop, self).setUp()
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.node_register(cfn[9])
        self.node_register(cfn[10])
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute('css', 'register', '-n', self.CSS2, *addr[6])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('css', 'start', '-n', self.CSS1)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop', '--force')
        finally:
            super(TestCssStop, self).tearDown()

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def test_default(self):
        self.execute('css', 'stop')
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_STOPPED)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)

    def test_css1(self):
        self.execute('css', 'stop', '-n', self.CSS1)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_STOPPED)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)

    def test_css2(self):
        self.execute('css', 'stop', '-n', self.CSS2)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)

    def test_error_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('css', 'stop', '-n', self.CSSX)
        ret = self.exec_and_result('css', 'status', '-s')
        result = ret[0]
        self.assertEqual(len(result), 2)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)

    def _regist_cssz(self):
        self.execute('css', 'register', '-n', self.CSSZ, *addr[7])
        self.execute_raw('deploy', '-g', 'css')

    def _regist_cssz2(self):
        self.execute('css', 'register', '-n', self.CSSZ2, *addr[8])
        self.execute_raw('deploy', '-g', 'css')

    def test_bad_css1(self):
        self._regist_cssz()
        lh = logging.StreamHandler(self.output)
        lh.setLevel(logging.WARNING)
        formatter = logging.Formatter('%(levelname)s: %(message)s')
        lh.setFormatter(formatter)
        pm.logger.addHandler(lh)
        try:
            self.execute('css', 'stop', '-n', self.CSSZ)
        finally:
            pm.logger.removeHandler(lh)
        lines = self.output.getvalue().splitlines()
        self.assertEqual(len(lines), 1)
        self.assertRegexpMatches(
            lines[0], 'WARNING: {0}'.format(cfc[7][2]['addr']['hostname']))
        output.status = False
        fabric.network.disconnect_all()

    def test_bad_css2(self):
        self._regist_cssz2()
        lh = logging.StreamHandler(self.output)
        lh.setLevel(logging.WARNING)
        formatter = logging.Formatter('%(levelname)s: %(message)s')
        lh.setFormatter(formatter)
        pm.logger.addHandler(lh)
        try:
            self.execute('css', 'stop', '-n', self.CSSZ2)
        finally:
            pm.logger.removeHandler(lh)
        lines = self.output.getvalue().splitlines()
        self.assertEqual(len(lines), 1)
        self.assertRegexpMatches(
            lines[0], 'WARNING: {0}'.format(cfc[8][2]['addr']['hostname']))

    def test_twice(self):
        self.execute('css', 'stop', '-n', self.CSS1)
        self.execute('css', 'stop', '-n', self.CSS1)


class TestCssState(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    CSSX = 'cssX'
    CSSZ = 'cssZ'

    def setUp(self):
        super(TestCssState, self).setUp()
        self.clear_local_conf_dir()
        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')

        output['everything'] = False
        env.hosts = ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                     for n in cfc[5]]
        execute(fabfile.clear_data)

        self.node_register(cfn[9])
        self.node_register(cfn[10])
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute('css', 'register', '-n', self.CSS2, *addr[6])
        self.execute('css', 'register', '-n', self.CSSZ, *addr[9])
        self.execute_raw('deploy', '-g', 'css')
        self.execute('memcache', 'register', '-c', self.CSS2)
        self.execute('css', 'start', '-n', self.CSS1)

        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        try:
            self.execute('css', 'stop', '--force')
        finally:
            super(TestCssState, self).tearDown()

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def test_default(self):
        result = self.exec_and_result('css', 'status')
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0]['name'], self.CSS1)
        self.assertEqual(result[0]['server'][0]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][0]['address'],
                         cfc[5][0]['addr']['hostname'])
        self.assertEqual(result[0]['server'][0]['port'], cfc[5][0]['uport'])
        self.assertEqual(result[0]['server'][0]['No.'], 0)
        self.assertEqual(result[0]['server'][1]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][1]['address'],
                         cfc[5][1]['addr']['hostname'])
        self.assertEqual(result[0]['server'][1]['port'], cfc[5][1]['uport'])
        self.assertEqual(result[0]['server'][1]['No.'], 1)
        self.assertEqual(result[0]['server'][2]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][2]['address'],
                         cfc[5][2]['addr']['hostname'])
        self.assertEqual(result[0]['server'][2]['port'], cfc[5][2]['uport'])
        self.assertEqual(result[0]['server'][2]['No.'], 2)
        self.assertEqual(result[1]['name'], self.CSS2)
        self.assertEqual(result[1]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][0]['address'],
                         cfc[6][0]['addr']['hostname'])
        self.assertEqual(result[1]['server'][0]['port'], cfc[6][0]['uport'])
        self.assertEqual(result[1]['server'][0]['No.'], 0)
        self.assertEqual(result[1]['server'][1]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][1]['address'],
                         cfc[6][1]['addr']['hostname'])
        self.assertEqual(result[1]['server'][1]['port'], cfc[6][1]['uport'])
        self.assertEqual(result[1]['server'][1]['No.'], 1)
        self.assertEqual(result[1]['server'][2]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][2]['address'],
                         cfc[6][2]['addr']['hostname'])
        self.assertEqual(result[1]['server'][2]['port'], cfc[6][2]['uport'])
        self.assertEqual(result[1]['server'][2]['No.'], 2)
        self.assertEqual(result[2]['name'], self.CSSZ)
        self.assertEqual(result[2]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[2]['server'][0]['address'],
                         cfc[9][0]['addr']['hostname'])
        self.assertEqual(result[2]['server'][0]['port'], cfc[9][0]['uport'])
        self.assertEqual(result[0]['server'][0]['No.'], 0)
        self.assertEqual(result[2]['server'][1]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][1]['address'],
                         cfc[9][1]['addr']['hostname'])
        self.assertEqual(result[2]['server'][1]['port'], cfc[9][1]['uport'])
        self.assertEqual(result[0]['server'][1]['No.'], 1)
        self.assertEqual(result[2]['server'][2]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][2]['address'],
                         cfc[9][2]['addr']['hostname'])
        self.assertEqual(result[2]['server'][2]['port'], cfc[9][2]['uport'])
        self.assertEqual(result[0]['server'][2]['No.'], 2)

    def test_css1(self):
        result = self.exec_and_result('css', 'status', '-n', self.CSS1)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['name'], self.CSS1)
        self.assertEqual(result[0]['server'][0]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][1]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][2]['status'],
                         config.PROC_STATE_RUNNING)

    def test_css2(self):
        result = self.exec_and_result('css', 'status', '-n', self.CSS2)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['name'], self.CSS2)
        self.assertEqual(result[0]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[0]['server'][1]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[0]['server'][2]['status'],
                         config.PROC_STATE_STOPPED)

    def test_error_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('css', 'status', '-n', self.CSSX)

    def test_short(self):
        ret = self.exec_and_result('css', 'status', '-s')
        self.assertEqual(len(ret), 1)
        result = ret[0]
        self.assertEqual(len(result), 3)
        self.assertEqual(result[self.CSS1], config.PROC_STATE_RUNNING)
        self.assertEqual(result[self.CSS2], config.PROC_STATE_STOPPED)
        self.assertEqual(result[self.CSSZ][0], config.PROC_STATE_STOPPED)
        self.assertEqual(result[self.CSSZ][1], config.PROC_STATE_ERROR)

    def test_quiet1(self):
        ret = self.execute('css', 'status', '-q')
        self.assertIsNotNone(ret)

    def test_quiet2(self):
        self.execute('memcache', 'unregister', '-i', '0')
        self.execute('css', 'unregister', '-n', self.CSS2)
        self.execute('css', 'unregister', '-n', self.CSSZ)
        self.execute_raw('deploy', '-g', 'css')
        ret = self.execute('css', 'status', '-q')
        self.assertIsNone(ret)

    def test_long1(self):
        time.sleep(3)
        result = self.exec_and_result('css', 'status', '-l')
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0]['name'], self.CSS1)
        self.assertEqual(result[0]['server'][0]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][0]['address'],
                         cfc[5][0]['addr']['hostname'])
        self.assertEqual(result[0]['server'][0]['port'], cfc[5][0]['uport'])
        self.assertEqual(result[0]['server'][0]['No.'], 0)
        self.assertTrue('pid' in result[0]['server'][0])
        self.assertTrue('birth' in result[0]['server'][0])
        self.assertTrue('master' in result[0]['server'][0])
        self.assertEqual(result[0]['server'][0]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][0]['connected'], [1, 2])
        self.assertEqual(result[0]['server'][1]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][1]['address'],
                         cfc[5][1]['addr']['hostname'])
        self.assertEqual(result[0]['server'][1]['port'], cfc[5][1]['uport'])
        self.assertEqual(result[0]['server'][1]['No.'], 1)
        self.assertTrue('pid' in result[0]['server'][1])
        self.assertTrue('birth' in result[0]['server'][1])
        self.assertTrue('master' in result[0]['server'][1])
        self.assertEqual(result[0]['server'][1]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][1]['connected'], [0, 2])
        self.assertEqual(result[0]['server'][2]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][2]['address'],
                         cfc[5][2]['addr']['hostname'])
        self.assertEqual(result[0]['server'][2]['port'], cfc[5][2]['uport'])
        self.assertEqual(result[0]['server'][2]['No.'], 2)
        self.assertTrue('pid' in result[0]['server'][2])
        self.assertTrue('birth' in result[0]['server'][2])
        self.assertTrue('master' in result[0]['server'][2])
        self.assertEqual(result[0]['server'][2]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][2]['connected'], [0, 1])
        self.assertEqual(result[1]['name'], self.CSS2)
        self.assertEqual(result[1]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][0]['address'],
                         cfc[6][0]['addr']['hostname'])
        self.assertEqual(result[1]['server'][0]['port'], cfc[6][0]['uport'])
        self.assertEqual(result[1]['server'][0]['No.'], 0)
        self.assertEqual(result[1]['server'][0]['next_sequence'], 0)
        self.assertEqual(result[1]['server'][1]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][1]['address'],
                         cfc[6][1]['addr']['hostname'])
        self.assertEqual(result[1]['server'][1]['port'], cfc[6][1]['uport'])
        self.assertEqual(result[1]['server'][1]['No.'], 1)
        self.assertEqual(result[1]['server'][1]['next_sequence'], 0)
        self.assertEqual(result[1]['server'][2]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][2]['address'],
                         cfc[6][2]['addr']['hostname'])
        self.assertEqual(result[1]['server'][2]['port'], cfc[6][2]['uport'])
        self.assertEqual(result[1]['server'][2]['No.'], 2)
        self.assertEqual(result[1]['server'][2]['next_sequence'], 0)
        self.assertEqual(result[2]['name'], self.CSSZ)
        self.assertEqual(result[2]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[2]['server'][0]['address'],
                         cfc[9][0]['addr']['hostname'])
        self.assertEqual(result[2]['server'][0]['port'], cfc[9][0]['uport'])
        self.assertEqual(result[2]['server'][0]['No.'], 0)
        self.assertEqual(result[2]['server'][0]['next_sequence'], 0)
        self.assertEqual(result[2]['server'][1]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][1]['address'],
                         cfc[9][1]['addr']['hostname'])
        self.assertEqual(result[2]['server'][1]['port'], cfc[9][1]['uport'])
        self.assertEqual(result[2]['server'][1]['No.'], 1)
        self.assertEqual(result[2]['server'][2]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][2]['address'],
                         cfc[9][2]['addr']['hostname'])
        self.assertEqual(result[2]['server'][2]['port'], cfc[9][2]['uport'])
        self.assertEqual(result[2]['server'][2]['No.'], 2)

    def test_long2(self):
        self.execute('css', 'start', '-n', self.CSS2)
        time.sleep(1)
        self.execute('memcache', 'start')
        time.sleep(1)
        self.execute('memcache', 'stop')
        time.sleep(1)
        self.execute('css', 'stop', '-n', self.CSS2)
        time.sleep(1)
        result = self.exec_and_result('css', 'status', '-l')
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0]['name'], self.CSS1)
        self.assertEqual(result[0]['server'][0]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][0]['address'],
                         cfc[5][0]['addr']['hostname'])
        self.assertEqual(result[0]['server'][0]['port'], cfc[5][0]['uport'])
        self.assertEqual(result[0]['server'][0]['No.'], 0)
        self.assertTrue('pid' in result[0]['server'][0])
        self.assertTrue('birth' in result[0]['server'][0])
        self.assertTrue('master' in result[0]['server'][0])
        self.assertEqual(result[0]['server'][0]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][0]['connected'], [1, 2])
        self.assertEqual(result[0]['server'][1]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][1]['address'],
                         cfc[5][1]['addr']['hostname'])
        self.assertEqual(result[0]['server'][1]['port'], cfc[5][1]['uport'])
        self.assertEqual(result[0]['server'][1]['No.'], 1)
        self.assertTrue('pid' in result[0]['server'][1])
        self.assertTrue('birth' in result[0]['server'][1])
        self.assertTrue('master' in result[0]['server'][1])
        self.assertEqual(result[0]['server'][1]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][1]['connected'], [0, 2])
        self.assertEqual(result[0]['server'][2]['status'],
                         config.PROC_STATE_RUNNING)
        self.assertEqual(result[0]['server'][2]['address'],
                         cfc[5][2]['addr']['hostname'])
        self.assertEqual(result[0]['server'][2]['port'], cfc[5][2]['uport'])
        self.assertEqual(result[0]['server'][2]['No.'], 2)
        self.assertTrue('pid' in result[0]['server'][2])
        self.assertTrue('birth' in result[0]['server'][2])
        self.assertTrue('master' in result[0]['server'][2])
        self.assertEqual(result[0]['server'][2]['next_sequence'], 0)
        self.assertEqual(result[0]['server'][2]['connected'], [0, 1])
        self.assertEqual(result[1]['name'], self.CSS2)
        self.assertEqual(result[1]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][0]['address'],
                         cfc[6][0]['addr']['hostname'])
        self.assertEqual(result[1]['server'][0]['port'], cfc[6][0]['uport'])
        self.assertEqual(result[1]['server'][0]['No.'], 0)
        self.assertTrue(result[1]['server'][0]['next_sequence'] > 0)
        self.assertEqual(result[1]['server'][1]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][1]['address'],
                         cfc[6][1]['addr']['hostname'])
        self.assertEqual(result[1]['server'][1]['port'], cfc[6][1]['uport'])
        self.assertEqual(result[1]['server'][1]['No.'], 1)
        self.assertTrue(result[1]['server'][1]['next_sequence'] > 0)
        self.assertEqual(result[1]['server'][2]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[1]['server'][2]['address'],
                         cfc[6][2]['addr']['hostname'])
        self.assertEqual(result[1]['server'][2]['port'], cfc[6][2]['uport'])
        self.assertEqual(result[1]['server'][2]['No.'], 2)
        self.assertTrue(result[1]['server'][2]['next_sequence'] > 0)
        self.assertEqual(result[2]['name'], self.CSSZ)
        self.assertEqual(result[2]['server'][0]['status'],
                         config.PROC_STATE_STOPPED)
        self.assertEqual(result[2]['server'][0]['address'],
                         cfc[9][0]['addr']['hostname'])
        self.assertEqual(result[2]['server'][0]['port'], cfc[9][0]['uport'])
        self.assertEqual(result[2]['server'][0]['No.'], 0)
        self.assertEqual(result[2]['server'][0]['next_sequence'], 0)
        self.assertEqual(result[2]['server'][1]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][1]['address'],
                         cfc[9][1]['addr']['hostname'])
        self.assertEqual(result[2]['server'][1]['port'], cfc[9][1]['uport'])
        self.assertEqual(result[2]['server'][1]['No.'], 1)
        self.assertEqual(result[2]['server'][2]['status'],
                         config.PROC_STATE_ERROR)
        self.assertEqual(result[2]['server'][2]['address'],
                         cfc[9][2]['addr']['hostname'])
        self.assertEqual(result[2]['server'][2]['port'], cfc[9][2]['uport'])
        self.assertEqual(result[2]['server'][2]['No.'], 2)
