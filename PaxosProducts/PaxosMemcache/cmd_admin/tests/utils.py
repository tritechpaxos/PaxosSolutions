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
import unittest
import shutil
import fabric.tasks
import fabric.state
import logging

from paxos.cmd.pmemcache import config, opt, node
import paxos.cmd.pmemcache as pm
from tests import fabfile


class _PaxosTest(unittest.TestCase):

    def setUp(self):
        pm.ch.setLevel(logging.CRITICAL)

    def tearDown(self):
        pm.ch.setLevel(logging.WARNING)

    def clear_local_conf_dir(self):
        conf_dir = os.path.expanduser(config.CONFIG_DIR)
        if os.path.exists(conf_dir):
            shutil.rmtree(conf_dir)
        node.Node.conf.load(force=True)

    def clear_remote_conf_dir(self, hostname):
        fabric.state.output['everything'] = False
        fabric.state.env.hosts = [hostname]
        fabric.tasks.execute(fabfile.clear_config)

    def execute_raw(self, *cmd):
        conf = opt.parser.parse_args(cmd)
        v = vars(conf)
        return conf.cmd(**v).run()

    def execute(self, *cmd0):
        cmd = list(cmd0)
        if len(cmd) > 1:
            cmd[0], cmd[1] = cmd[1], cmd[0]
        return self.execute_raw(*cmd)

    def node_register(self, ninfo, *args):
        opts = ['host', 'register', '-u', ninfo['user']]
        opts.extend(args)
        opts.append(ninfo['hostname'])
        self.execute(*opts)

    def assert_cell_conf_files(self, css_list, nodes):
        fabric.state.output['everything'] = False
        fabric.state.env.hosts = [
            '{0}@{1}'.format(n['addr']['user'], n['addr']['hostname'])
            for n in nodes]
        results = fabric.tasks.execute(fabfile.check_cell_conf, css_list)
        for k, v in results.iteritems():
            self.assertTrue(v, msg='error in {0}'.format(k))
