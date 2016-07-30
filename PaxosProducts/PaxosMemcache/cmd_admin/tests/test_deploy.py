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
import os
import os.path
import pwd
import unittest
import StringIO
from itertools import chain
import fabric.tasks
import fabric.state

from paxos.cmd.pmemcache import config, opt, node, error
from paxos.cmd.pmemcache.util import match_hostaddr
from tests import fabfile
from tests import utils
from tests import test_conf


cfn = test_conf['node']
cfc = [c['node'] for c in test_conf['css']]
addr = [list(chain.from_iterable([(
    '{0}:{1}'.format(n['addr']['hostname'], n['uport']),
    '{0}:{1}'.format(n['addr']['hostname'], n['tport']))
    for n in c])) for c in cfc]


class TestNodeDeploy(utils._PaxosTest):

    def setUp(self):
        self.output = StringIO.StringIO()
        self.clear_local_conf_dir()
        host0 = '{0}@{1}'.format(cfn[0]['user'], cfn[0]['hostname'])
        self.clear_remote_conf_dir(host0)
        self.execute(
            'host', 'register', '-u', cfn[0]['user'], cfn[0]['hostname'])

    def test_default(self):
        self.execute_raw('deploy')

    def test_use_ssh_config(self):
        self.execute(
            'host', 'register', '-g', 'grp',
            '--use-ssh-config', cfn[0]['ssh_alias'])
        self.execute_raw('deploy', '-g', 'grp')

    def test_localhost1(self):
        self.execute('host', 'register', '-g', 'grp', '127.0.0.1', '-u',
                     pwd.getpwuid(os.getuid())[0])
        self.execute_raw('deploy', '-g', 'grp')

    def test_error(self):
        self.execute('host', 'register', cfn[10]['hostname'])
        self.execute(
            'host', 'register', cfn[1]['hostname'], '-u', cfn[1]['user'])
        with self.assertRaises(error.DeployError):
            self.execute_raw('deploy')


class TestNodeDeployClean(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        self.clear_local_conf_dir()
        for ad in ['{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
                   for n in cfc[5]]:
            self.clear_remote_conf_dir(ad)

        self.node_register(cfc[5][0]['addr'], '-g', 'css')
        self.node_register(cfc[5][1]['addr'], '-g', 'css')
        self.node_register(cfc[5][2]['addr'], '-g', 'css')
        self.execute('css', 'register', '-n', self.CSS1, *addr[5])
        self.execute('css', 'register', '-n', self.CSS2, *addr[6])
        self.execute_raw('deploy', '-g', 'css')
        self.assert_cell_conf_files([self.CSS1, self.CSS2], cfc[5])

    def test_clean(self):
        self.execute_raw('deploy', '-g', 'css', '--clean')
        self.assert_cell_conf_files([self.CSS1, self.CSS2], cfc[5])

        self.execute('css', 'unregister', '-n', self.CSS2)
        self.execute_raw('deploy', '-g', 'css', '--clean')
        self.assert_cell_conf_files([self.CSS1], cfc[5])

    def is_localhost(self, n):
        return match_hostaddr(n['addr']['hostname'])

    def test_not_clean(self):
        nodes0 = cfc[5]
        self.execute_raw('deploy', '-g', 'css', '--clean')
        self.assert_cell_conf_files([self.CSS1, self.CSS2], nodes0)

        nodes = [n for n in nodes0 if not self.is_localhost(n)]
        self.execute('css', 'unregister', '-n', self.CSS2)
        self.execute_raw('deploy', '-g', 'css')
        self.assert_cell_conf_files([self.CSS1, self.CSS2], nodes)


class TestNodeFetch(utils._PaxosTest):

    def setUp(self):
        self.clear_local_conf_dir()
        host0 = '{0}@{1}'.format(cfn[0]['user'], cfn[0]['hostname'])
        self.clear_remote_conf_dir(host0)
        self.execute(
            'host', 'register', '-u', cfn[0]['user'], cfn[0]['hostname'])
        self.execute_raw('deploy')
        self.clear_local_conf_dir()
        fabric.state.env.use_ssh_config = False
        fabric.state.env.abort_on_prompts = True

    def test_success(self):
        addr = '{0}@{1}'.format(cfn[1]['user'], cfn[1]['hostname'])
        self.execute_raw('fetch', addr)
        self.assertTrue(os.path.exists(os.path.expanduser(config.CONFIG_DIR)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.MEMCACHE_CONF_PATH)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.CONFIG_CELL_DIR)))

    def test_use_ssh_config(self):
        self.execute_raw('fetch', '--use-ssh-config', cfn[1]['ssh_alias'])
        self.assertTrue(os.path.exists(os.path.expanduser(config.CONFIG_DIR)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.MEMCACHE_CONF_PATH)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.CONFIG_CELL_DIR)))

    def test_reg_node1(self):
        self.execute(
            'host', 'register', '-u', cfn[1]['user'], cfn[1]['hostname'])
        self.execute_raw('fetch', cfn[1]['hostname'])
        self.assertTrue(os.path.exists(os.path.expanduser(config.CONFIG_DIR)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.MEMCACHE_CONF_PATH)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.CONFIG_CELL_DIR)))

    def test_reg_node2(self):
        self.execute(
            'host', 'register', '--use-ssh-config', cfn[0]['ssh_alias'])
        self.execute_raw('fetch', cfn[0]['ssh_alias'])
        self.assertTrue(os.path.exists(os.path.expanduser(config.CONFIG_DIR)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.MEMCACHE_CONF_PATH)))
        self.assertTrue(os.path.exists(os.path.expanduser(
            config.CONFIG_CELL_DIR)))
