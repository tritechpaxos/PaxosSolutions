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
import os.path
from ..paxos import Paxos
from cellconfig import syslog_conf


class RasEye(object):
    PATH_RAS_BIN = 'ras/bin'
    PATH_ENV_RAS_BIN = '.pyenv/versions/ras/bin'
    CMD_RASEYE = 'RasEye'
    CMD_RASEYEADM = 'RasEyeAdm'
    CMD_EVT_SCRIPT = 'eventh'

    def __init__(self, group, home, no):
        self.group = group
        self.home = home
        self.no = no

    def _build_cmdline(self, prog, opt='', ext_env=''):
        return 'env {ev}={conf} {ext_env} {prog} {group} {no} {opt}'.format(
            prog=os.path.join(self.home, self.PATH_RAS_BIN, prog),
            group=self.group, no=self.no, opt=opt, ext_env=ext_env,
            ev=Paxos.CELL_CONF_ENV,
            conf=os.path.join(self.home, Paxos.CELL_CONF_PATH) + '/',
        )

    def command_line(self, opt=None, ext_env=None):
        log = syslog_conf()
        if opt is None:
            opt = ' < /dev/null |& {} > /dev/null 2>&1 '.format(
                self._logger_cmd(log))
        ext_env_1 = ' LOG_SIZE=0 LOG_LEVEL={}'.format(log.level())
        if ext_env is not None:
            ext_env += ext_env_1
        else:
            ext_env = ext_env_1
        return self._build_cmdline(self.CMD_RASEYE, opt, ext_env)

    def _logger_cmd(self, log):
        srv = log.server()
        srvopt = "" if srv is None else "-n {}".format(srv)
        return '{cmd} {server_opt} -p {priority}'.format(
            cmd=Paxos.LOG_CMD, server_opt=srvopt, priority=log.priority())

    def admin(self):
        return RasEyeAdm(self)


class RasEyeAdm(object):

    def __init__(self, srv):
        self.srv = srv

    def stop(self, opt=None, ext_env=''):
        if opt is None:
            opt = ' < /dev/null'
        opt = 'stop ' + opt
        return self.srv._build_cmdline(self.srv.CMD_RASEYEADM, opt, ext_env)

    def set_event(self, ras_cell, path):
        script = os.path.join(
            self.srv.home, self.srv.PATH_ENV_RAS_BIN, self.srv.CMD_EVT_SCRIPT)
        opt = 'set {ras_cell} {path} - {script} < /dev/null'.format(
            ras_cell=ras_cell, path=path, script=script)
        return self.srv._build_cmdline(self.srv.CMD_RASEYEADM, opt)

    def unset_event(self, ras_cell, path):
        script = os.path.join(
            self.srv.home, self.srv.PATH_ENV_RAS_BIN, self.srv.CMD_EVT_SCRIPT)
        opt = 'unset {ras_cell} {path} < /dev/null'.format(
            ras_cell=ras_cell, path=path)
        return self.srv._build_cmdline(self.srv.CMD_RASEYEADM, opt)

    def regist_me(self, ras_cell):
        opt = 'regist_me {ras_cell} < /dev/null'.format(ras_cell=ras_cell)
        return self.srv._build_cmdline(self.srv.CMD_RASEYEADM, opt)

    def unregist_me(self, ras_cell):
        opt = 'unregist_me {ras_cell} < /dev/null'.format(ras_cell=ras_cell)
        return self.srv._build_cmdline(self.srv.CMD_RASEYEADM, opt)
