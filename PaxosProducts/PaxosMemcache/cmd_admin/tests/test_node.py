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
import re
import os.path
import unittest
import shutil
import StringIO
import yaml

from paxos.cmd.pmemcache import config, opt, node, error
from tests import utils


class _TestNode(utils._PaxosTest):

    HOST0 = 'vv-vm0'
    HOST1 = 'vv-vm1'
    HOST2 = 'vv-vm2'
    HOST3 = 'vv-vm3'
    HOSTX = 'vv-vmX'

    def setUp(self):
        self.clear_local_conf_dir()


class TestNodeRegister(_TestNode):

    def setUp(self):
        super(TestNodeRegister, self).setUp()
        self.output = StringIO.StringIO()
        sys.stderr = self.output

    def tearDown(self):
        sys.stderr = sys.__stderr__

    def test_default(self):
        self.execute('host', 'register', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, [])
        self.assertFalse(n.use_ssh_config)

    def test_user_port(self):
        self.execute(
            'host', 'register', '-u', 'paxos2', '-p', '33', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertEqual(n.port, 33)
        self.assertEqual(n.user, 'paxos2')
        self.assertEqual(n.groups, [])
        self.assertFalse(n.use_ssh_config)

    def test_use_ssh_config(self):
        self.execute('host', 'register', '--use-ssh-config', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertIsNone(n.user)
        self.assertEqual(n.groups, [])
        self.assertTrue(n.use_ssh_config)

    def test_bad_port(self):
        line = ['host', 'register', '-p', 'xxx', self.HOST1]
        with self.assertRaises(SystemExit) as cm:
            opt.parser.parse_args(line)
        self.assertEqual(cm.exception.code, 2)

    def test_group1(self):
        self.execute('host', 'register', '-g', 'grp0', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, ['grp0'])
        self.assertFalse(n.use_ssh_config)

    def test_group2(self):
        self.execute('host', 'register', '-g', 'grp0:grp1', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, ['grp0', 'grp1'])
        self.assertFalse(n.use_ssh_config)

    def test_err_exists(self):
        self.execute('host', 'register', self.HOST1)
        with self.assertRaises(error.ExistError):
            self.execute('host', 'register', self.HOST1)


class TestNodeUnregister(_TestNode):

    def setUp(self):
        super(TestNodeUnregister, self).setUp()
        self.execute('host', 'register', self.HOST1)

    def test_success(self):
        self.execute('host', 'unregister', self.HOST1)
        result = node.Node.find()
        self.assertEqual(len(result), 0)
        with self.assertRaises(error.NotFoundError):
            node.Node.get(self.HOST1)

    def test_error(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('host', 'unregister', self.HOST2)


class TestNodeModify(_TestNode):

    def setUp(self):
        super(TestNodeModify, self).setUp()
        self.execute('host', 'register', self.HOST1)
        self.execute(
            'host', 'register', '-u', 'paxos', '-p', '22', '-g', 'grp0',
            self.HOST2)
        self.execute('host', 'register', '--use-ssh-config', self.HOST3)
        self.output = StringIO.StringIO()
        sys.stderr = self.output

    def tearDown(self):
        sys.stderr = sys.__stderr__

    def test_user1(self):
        self.execute('host', 'modify', '-u', 'paxos2', self.HOST1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos2')
        self.assertEqual(n.groups, [])

    def test_user2(self):
        self.execute('host', 'modify', '-u', 'paxos2', self.HOST2)
        n = node.Node.get(self.HOST2)
        self.assertEqual(n.hostname, self.HOST2)
        self.assertEqual(n.port, 22)
        self.assertEqual(n.user, 'paxos2')
        self.assertEqual(n.groups, ['grp0'])

    def test_port1(self):
        self.execute('host', 'modify', '-p', '33', self.HOST1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertEqual(n.port, 33)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, [])

    def test_port2(self):
        self.execute('host', 'modify', '-p', '33', self.HOST2)
        n = node.Node.get(self.HOST2)
        self.assertEqual(n.hostname, self.HOST2)
        self.assertEqual(n.port, 33)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, ['grp0'])

    def test_group1(self):
        self.execute('host', 'modify', '-g', 'grp0:grp1', self.HOST1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, ['grp0', 'grp1'])

    def test_group2(self):
        self.execute('host', 'modify', '-g', 'grp0:grp1', self.HOST2)
        n = node.Node.get(self.HOST2)
        self.assertEqual(n.hostname, self.HOST2)
        self.assertEqual(n.port, 22)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, ['grp0', 'grp1'])

    def test_bad_port(self):
        line = ['host', 'modify', '-p', 'xxx', self.HOST1]
        with self.assertRaises(SystemExit) as cm:
            opt.parser.parse_args(line)
        self.assertEqual(cm.exception.code, 2)

    def test_error_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('host', 'modify', '-u', 'paxos2', self.HOSTX)

    def test_use_ssh_config(self):
        self.execute('host', 'modify', '--use-ssh-config', self.HOST1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, [])
        self.assertTrue(n.use_ssh_config)

        self.execute('host', 'modify', '--not-use-ssh-config', self.HOST1)
        n = node.Node.get(self.HOST1)
        self.assertEqual(n.hostname, self.HOST1)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, [])
        self.assertFalse(n.use_ssh_config)

    def test_not_use_ssh_config(self):
        self.execute('host', 'modify', '--not-use-ssh-config', self.HOST3)
        n = node.Node.get(self.HOST3)
        self.assertEqual(n.hostname, self.HOST3)
        self.assertIsNone(n.port)
        self.assertEqual(n.user, 'paxos')
        self.assertEqual(n.groups, [])
        self.assertFalse(n.use_ssh_config)


class TestNodeShow(_TestNode):

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def setUp(self):
        super(TestNodeShow, self).setUp()
        self.execute('host', 'register', '--use-ssh-config', self.HOST0)
        self.execute('host', 'register', '-g', 'grp0', self.HOST1)
        self.execute(
            'host', 'register', '-u', 'paxos', '-p', '22', '-g', 'grp0:grp1',
            self.HOST2)
        self.execute(
            'host', 'register', '-u', 'paxos2', '-p', '33', '-g', 'grp1',
            self.HOST3)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        self.output.close()

    def test_default(self):
        result = self.exec_and_result('host', 'show')
        self.assertEqual(len(result), 4)
        self.assertEqual(len(result[0]), 2)
        self.assertEqual(len(result[1]), 3)
        self.assertEqual(len(result[2]), 4)
        self.assertEqual(len(result[3]), 4)

    def test_user1(self):
        result = self.exec_and_result('host', 'show', '-u' 'paxos')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['user'], 'paxos')
        self.assertEqual(len(result[0]), 3)

    def test_user2(self):
        result = self.exec_and_result('host', 'show', '-u' 'paxos2')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['user'], 'paxos2')
        self.assertEqual(len(result[0]), 4)

    def test_port(self):
        result = self.exec_and_result('host', 'show', '-p' '33')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['port'], 33)
        self.assertEqual(len(result[0]), 4)

    def test_group1(self):
        result = self.exec_and_result('host', 'show', '-g' 'grp0')
        self.assertEqual(len(result), 2)
        for res in result:
            self.assertEqual(res['group'].count('grp0'), 1)

    def test_group2(self):
        result = self.exec_and_result('host', 'show', '-g' 'grp0:grp1')
        self.assertEqual(len(result), 3)
        for res in result:
            self.assertTrue(res['group'].count('grp0') +
                            res['group'].count('grp1') > 0)

    def test_host1(self):
        result = self.exec_and_result('host', 'show', self.HOST1)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['hostname'], self.HOST1)

    def test_host2(self):
        result = self.exec_and_result('host', 'show', self.HOST1, self.HOST2)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['hostname'], self.HOST1)
        self.assertEqual(result[1]['hostname'], self.HOST2)


class TestNodeList(_TestNode):

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return self.output.getvalue().splitlines()

    def setUp(self):
        super(TestNodeList, self).setUp()
        self.execute('host', 'register', self.HOST0)
        self.execute('host', 'register', '-g', 'grp0', self.HOST1)
        self.execute(
            'host', 'register', '-u', 'paxos', '-p', '22', '-g', 'grp0:grp1',
            self.HOST2)
        self.execute(
            'host', 'register', '-u', 'paxos2', '-p', '33', '-g', 'grp1',
            self.HOST3)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__
        self.output.close()

    def test_default(self):
        result = self.exec_and_result('host', 'list')
        self.assertEqual(len(result), 4)

    def test_user1(self):
        result = self.exec_and_result('host', 'list', '-u' 'paxos')
        self.assertEqual(len(result), 3)
        self.assertEqual(result.count(self.HOST0), 1)
        self.assertEqual(result.count(self.HOST1), 1)
        self.assertEqual(result.count(self.HOST2), 1)

    def test_user2(self):
        result = self.exec_and_result('host', 'list', '-u' 'paxos2')
        self.assertEqual(len(result), 1)
        self.assertEqual(result.count(self.HOST3), 1)

    def test_port(self):
        result = self.exec_and_result('host', 'list', '-p' '33')
        self.assertEqual(len(result), 1)
        self.assertEqual(result.count(self.HOST3), 1)

    def test_group1(self):
        result = self.exec_and_result('host', 'list', '-g' 'grp0')
        self.assertEqual(len(result), 2)
        self.assertEqual(result.count(self.HOST1), 1)
        self.assertEqual(result.count(self.HOST2), 1)

    def test_group2(self):
        result = self.exec_and_result('host', 'list', '-g' 'grp0:grp1')
        self.assertEqual(len(result), 3)
        self.assertEqual(result.count(self.HOST1), 1)
        self.assertEqual(result.count(self.HOST2), 1)
        self.assertEqual(result.count(self.HOST3), 1)
