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

from ..sshagent import _SshAgent
from . import fabfile


logger = logging.getLogger(__name__)


class CellRasOperation(_SshAgent):

    def _start(self, cell, nodes):
        from cellconfig.ras.model import RasTarget
        tgts = RasTarget.select().where(
            (RasTarget.type == 'cell') & (RasTarget.name == cell.name) &
            RasTarget.enabled)
        for tgt in tgts:
            logger.debug("RasCluster {}".format(tgt.name))
            self._execute_fab(
                task=fabfile.setup_cell, nodes=nodes, cell=cell,
                ras_cell=tgt.group.cell.name, path=tgt.path, warn_only=False)

    def start(self, node, params=None):
        logger.debug("START RasCluster {}".format(self.target.name))
        self._start(self.target, [node])

    def satrt_all(self):
        logger.debug("START ALL RasCluster {}".format(self.target.name))
        self._start(self.target, [self.target.nodes])
