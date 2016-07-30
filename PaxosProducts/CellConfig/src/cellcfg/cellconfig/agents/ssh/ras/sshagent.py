###########################################################################
#   Cellcfg Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of Cellcfg.
#
#   Cellcfg is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cellcfg is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Cellcfg.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import logging
from os import unlink
from tempfile import mkstemp
import sys

from cellconfig.error import DeployError
from ..sshagent import _SshAgent
from . import fabfile


logger = logging.getLogger(__name__)


class _RasOperation(_SshAgent):

    def _deploy_conf(self, cell, nodes):
        tmp = mkstemp(text=True)[1]
        cell.generate_conf(tmp)
        try:
            self._execute_fab(
                fabfile.deploy_conf, nodes=nodes, conf_path=tmp, cell=cell)
        finally:
            unlink(tmp)


class RasServerOperation(_RasOperation):

    def deploy(self):
        n = self.target.node
        try:
            self._execute_fab(
                fabfile.deploy_server, nodes=[self.target],
                os_name=n.os, dist_name=n.distribution, machine=n.machine)
        except fabfile.FabError as e:
            trace = sys.exc_info()[2]
            raise DeployError, self, trace
        self._deploy_conf(self.target.group.cell, [self.target])

    def _set_event(self, tgt):
        self._execute_fab(
            task=fabfile.set_event_server,
            nodes=[self.target],
            group=self.target.group.name,
            ras_cell=self.target.group.cell.name,
            path=tgt.path)

    def start(self):
        self._execute_fab(
            fabfile.start_server, nodes=[self.target],
            group=self.target.group.name, warn_only=False)
        for tgt in self.target.group.targets:
            self._set_event(tgt)
        from cellconfig.ras.model import RasTarget
        tgts = RasTarget.select().where(
            (RasTarget.type == 'ras') &
            (RasTarget.name == self.target.group.name) &
            RasTarget.enabled)
        for tgt in tgts:
            logger.debug("Ras RegistME {}".format(tgt.name))
            self._execute_fab(
                task=fabfile.setup_ras,
                nodes=[self.target],
                group=self.target.group.name,
                ras_cell=self.target.group.cell.name,
            )
        logger.debug("START SERVER: success")

    def stop(self):
        group = self.target.group.name
        logger.debug("STOP SERVER: {}".format(group))
        self._execute_fab(
            fabfile.stop_server, nodes=[self.target], group=group)


class RasTargetOperation(_RasOperation):

    def set_event(self, nodes=None):
        nodes = self.target.group.servers if nodes is None else nodes
        self._execute_fab(
            task=fabfile.set_event_server,
            nodes=nodes,
            group=self.target.group.name,
            ras_cell=self.target.group.cell.name,
            path=self.target.path)

    def unset_event(self, nodes=None):
        nodes = self.target.group.servers if nodes is None else nodes
        self._execute_fab(
            task=fabfile.unset_event_server,
            nodes=nodes,
            group=self.target.group.name,
            ras_cell=self.target.group.cell.name,
            path=self.target.path)

    def setup_cell(self):
        self._execute_fab(
            task=fabfile.setup_cell,
            nodes=self.target.target.nodes,
            cell=self.target.target,
            ras_cell=self.target.group.cell.name,
            path=self.target.path)

    def setup_ras(self):
        nodes = self.target.target.servers
        self._execute_fab(
            task=fabfile.setup_ras,
            nodes=nodes,
            group=self.target.target.name,
            ras_cell=self.target.target.cell.name)

    def teardown_ras(self):
        nodes = self.target.target.servers
        self._execute_fab(
            task=fabfile.teardown_ras,
            nodes=nodes,
            group=self.target.target.name,
            ras_cell=self.target.target.cell.name)
