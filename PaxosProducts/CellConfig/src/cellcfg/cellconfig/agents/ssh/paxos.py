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
import re
from time import strftime, localtime
from cellconfig import syslog_conf


class Paxos(object):
    CORE_BIN_PATH = 'core/bin'
    CORE_COMMANDS = [
        'PaxosAdmin', 'PaxosSessionShutdown', 'PaxosSessionProbe', 'PFSProbe',
        'xjPingPaxos', 'PFSServer', 'LVServer', 'LVProbe', 'PFSRasCluster',
    ]
    CELL_CONF_PATH = 'cell/config'
    CELL_CONF_ENV = 'PAXOS_CELL_CONFIG'
    CELL_DATA_PATH = 'cell/data'
    CELL_LOG_PATH = 'cell/log'
    LOG_CMD = 'logger'
    RAS_CLUSTER_CMD = 'PFSRasCluster'
    SHUTDOWN = 'SHUTDOWN'

    @classmethod
    def create(cls, cell, home=None, server_no=None):
        if cell.type == 'CSS' or cell.type == 'RAS':
            return PaxosCss(cell, home, server_no)
        elif cell.type == 'CMDB':
            return PaxosCmdb(cell, home, server_no)
        elif cell.type == 'LV':
            return PaxosLv(cell, home, server_no)
        else:
            raise

    def __init__(self, cell, home=None, server_no=None):
        self.cell = cell
        self.home = home
        self.server_no = server_no

    def _prog_path(self, prog_name):
        print prog_name
        return os.path.join(self.home, self.CORE_BIN_PATH, prog_name)

    def _build_cmdline(self, prog_name, opt='', ext_env=''):
        return 'env {ev}={conf} {ext_env} '\
               '{prog} {cell_name} {server_no} {opt}'\
            .format(
                prog=self._prog_path(prog_name),
                cell_name=self.cell.name,
                server_no=self.server_no,
                opt=opt,
                ev=self.CELL_CONF_ENV,
                conf=os.path.join(self.home, self.CELL_CONF_PATH) + '/',
                ext_env=ext_env,
            )

    def shutdown_filename(self):
        return os.path.join(self.cell_data_path(), self.SHUTDOWN)

    def service_command_line(self, next_no=0):
        log = syslog_conf()
        if self.server_no < self.cell.core:
            join = 'TRUE {}'.format(next_no if next_no >= 0 else 0)
        else:
            join = 'FALSE'
        return self._build_cmdline(
            prog_name=self.program_name(),
            opt='{data_path} {join} < /dev/null |& {log_cmd} >& /dev/null'
                .format(data_path=self.cell_data_path(),
                        join=join,
                        log_cmd=self._logger_cmd(log)),
            ext_env='LOG_SIZE=0 LOG_LEVEL={}'.format(log.level()),
        )

    def _logger_cmd(self, log):
        srv = log.server()
        srvopt = "" if srv is None else "-n {}".format(srv)
        return '{cmd} {server_opt} -p {priority}'.format(
            cmd=self.LOG_CMD, server_opt=srvopt, priority=log.priority())

    def session_shutdown(self):
        return self._build_cmdline(prog_name='PaxosSessionShutdown')

    def get_probe(self, op):
        if op == 'accept':
            return PaxosSessionProbeAccept(self)
        elif op == 'FdEvent':
            return PaxosSessionProbeFdEvent(self)
        elif op == 'out_of_band':
            return PaxosSessionProbeOutOfBand(self)
        elif op == 'df':
            return PaxosDiskSpace(self)
        else:
            raise

    def get_admin(self):
        return PaxosAdmin(self)

    def parse_ps(self, ps_output):
        prog = self.program_name()
        for line in ps_output.splitlines():
            words = line.split()
            pid, cmd = words[0:2]
            args = words[2:]
            if args[0] == self.cell.name and int(args[1]) == self.server_no:
                return int(pid)
        return None

    def cell_conf_path(self):
        return os.path.join(self.home, self.CELL_CONF_PATH,
                            "{0}.conf".format(self.cell.name))

    def cell_data_path(self):
        return os.path.join(self.home, self.CELL_DATA_PATH,
                            self.cell.name, str(self.server_no))

    def cell_log_path(self):
        return os.path.join(self.home, self.CELL_LOG_PATH,
                            self.cell.name, str(self.server_no))

    def ras_cluster(self, ras_cell, path):
        opt = '{ras_cell} RAS/{path} < /dev/null'.format(
            ras_cell=ras_cell, path=path)
        return self._build_cmdline(
            prog_name=self.RAS_CLUSTER_CMD, opt=opt)


class PaxosCmdb(Paxos):
    def cell_data_subdirs(self):
        return 'DATA DB'

    def program_name(self):
        return 'xjPingPaxos'


class PaxosCss(Paxos):
    def cell_data_subdirs(self):
        return 'DATA PFS'

    def program_name(self):
        return 'PFSServer'

    def get_probe(self, op):
        if op == 'path':
            return PaxosPfsProbePath(self)
        elif op == 'lock':
            return PaxosPfsProbeLock(self)
        elif op == 'queue':
            return PaxosPfsProbeQueue(self)
        elif op == 'meta':
            return PaxosPfsProbeMeta(self)
        elif op == 'fd':
            return PaxosPfsProbeFd(self)
        elif op == 'block':
            return PaxosPfsProbeBlock(self)
        else:
            return super(PaxosCss, self).get_probe(op)


class PaxosLv(Paxos):
    def cell_data_subdirs(self):
        return 'DATA LVS'

    def program_name(self):
        return 'LVServer'

    def get_probe(self, op):
        if op == 'meta':
            return PaxosLvProbeMeta(self)
        elif op == 'fd':
            return PaxosLvProbeFd(self)
        elif op == 'block':
            return PaxosLvProbeBlock(self)
        else:
            return super(PaxosLv, self).get_probe(op)


class PaxosCommand(object):

    CLIENT_ID_PATTERN = re.compile(
        'ClientId\s+\[([^-]+)-(\d+)\]\s+Pid=(\d+)-(\d+)\s+Seq=(\d+)\s+'
        'Reuse=(\d+)\s+Try=(\d+)')

    ACCEPT_PATTERN = re.compile(
        'Cnt=(\d+)\s+pFd=(\S+)\s+Opened=(\d+)\s+Start=(\d+)\s+End=(\d+)\s+'
        'Reply=(\d+)\s+Cmd=(0x\w+)\s+Events=(\d+)\((\d+)\)\s+Create=(\d+)\s+'
        'Access=(\d+)')

    FD_PATTERN = re.compile('Fd=(\d+)\s+events=(0x\w+)')

    def __init__(self, paxos, prog=None, opt=None):
        self.paxos = paxos
        self.prog = prog
        self.opt = opt

    def command(self):
        return self.paxos._build_cmdline(prog_name=self.prog, opt=self.opt)

    def to_ftime(self, txt):
        if not str.isdigit(txt):
            return '-'
        v = int(txt)
        if v > 0:
            return strftime("%Y-%m-%d %H:%M:%S", localtime(v))
        else:
            return '-'

    def parse(self, lines):
        ret = []
        value = None
        for line in lines.splitlines():
            mat = re.match(self.pat, line)
            if mat:
                value = self._init_list()
                value.extend(mat.groups())
                ret.append(value)
                continue
        return ret

    def _init_list(self):
        no = self.paxos.server_no
        cnode = self.paxos.cell.find_by_server_no(no)
        hostname = cnode.node.hostname if cnode else ''
        return [no, hostname]

    def _parse_accept(self, line, header, value, result):
        mat = re.match(self.CLIENT_ID_PATTERN, line)
        if mat:
            value = header[:]
            value.extend(mat.groups())
            result.append(value)
            return value
        mat = re.match(self.ACCEPT_PATTERN, line)
        if mat:
            value.extend(mat.groups())
            for idx in (-1, -2):
                value[idx] = self.to_ftime(value[idx])
            return value
        mat = re.match(self.FD_PATTERN, line)
        if mat:
            value.extend(mat.groups())
            return value
        return value

    def _padding(self, result, num):
        for r in result:
            if len(r) < num:
                r.extend(['-'] * (num - len(r)))


class PaxosSessionProbeAccept(PaxosCommand):

    def __init__(self, paxos):
        super(PaxosSessionProbeAccept, self)\
            .__init__(paxos, prog='PaxosSessionProbe', opt='accept')

    def parse(self, lines):
        ret = []
        header = None
        value = None
        for line in lines.splitlines():
            if line.strip() == '=== Accept ===':
                header = self._init_list()
                continue
            value = self._parse_accept(line, header, value, ret)
        return ret


class PaxosSessionProbeFdEvent(PaxosCommand):

    pat = re.compile('fd=(\d+)\s+\w+=(0x\d+)')

    def __init__(self, paxos):
        super(PaxosSessionProbeFdEvent, self)\
            .__init__(paxos, prog='PaxosSessionProbe', opt='FdEvent')


class PaxosSessionProbeOutOfBand(PaxosCommand):

    pat0 = re.compile('===\s+OutOfBand\s+Validate\[(\d+)\]\s+'
                      'Seq\[(\d+)\]\s+pData\[([^]]+)\]')
    pat1 = re.compile('ClientId\s+\[([^-]+)-(\d+)\]\s+Pid=(\d+)-(\d+)\s'
                      'Seq=(\d+)\s+Reuse=(\d+)\s+Try=(\d+)')

    def __init__(self, paxos):
        super(PaxosSessionProbeOutOfBand, self)\
            .__init__(paxos, prog='PaxosSessionProbe', opt='out_of_band')

    def parse(self, lines):
        ret = []
        value = None
        val0 = None
        for line in lines.splitlines():
            mat = re.match(self.pat0, line)
            if mat:
                val0 = mat.groups()
                continue
            mat = re.match(self.pat1, line)
            if val0 is not None and mat:
                value = self._init_list()
                value.extend(mat.groups())
                value.append(val0[0])
                value.append(val0[2])
                value.append(val0[1])
                ret.append(value)
                val0 = None
        return ret


class PaxosPfsProbePath(PaxosCommand):

    pat = re.compile('===\s+Path\s*\[([^]]+)\]\s+RefCnt=(-?\d)\s+===')

    def __init__(self, paxos):
        super(PaxosPfsProbePath, self)\
            .__init__(paxos, prog='PFSProbe', opt='path')

    def parse(self, lines):
        ret = []
        header = None
        value = None
        for line in lines.splitlines():
            mat = re.match(self.pat, line)
            if mat:
                header = self._init_list()
                header.extend(mat.groups())
                continue
            value = self._parse_accept(line, header, value, ret)
        self._padding(ret, 24)
        return ret


class PaxosPfsProbeLock(PaxosCommand):

    pat0 = re.compile('===\s+LOCK\s+\[([^]]+)\]\s+Cnt=(-?\d)\s+===')
    pat1 = re.compile('->(Holders|Waiters|Event Waiters)')

    def __init__(self, paxos):
        super(PaxosPfsProbeLock, self)\
            .__init__(paxos, prog='PFSProbe', opt='lock')

    def parse(self, lines):
        ret = []
        header0 = None
        header = None
        value = None
        for line in lines.splitlines():
            mat = re.match(self.pat0, line)
            if mat:
                header0 = self._init_list()
                header0.extend(mat.groups())
                header = None
                value = None
                continue
            mat = re.match(self.pat1, line)
            if mat:
                header = header0[:]
                header.extend(mat.groups())
                value = None
                continue
            value = self._parse_accept(line, header, value, ret)
        self._padding(ret, 25)
        return ret


class PaxosPfsProbeQueue(PaxosCommand):

    pat0 = re.compile('===\s+QUEUE\s+\[([^]]+)\]\s+Cnt=(\d)\s+Peek=(\d+)===')
    pat1 = re.compile('\[(\d+)\]\s+(\d+):\s+v_Len=(\d+)')
    pat2 = re.compile('->(Waiters|Event Waiters)')
    lbl2mode = dict({'Waiters': 'post', 'Event Waiters': 'event'})

    def __init__(self, paxos):
        super(PaxosPfsProbeQueue, self)\
            .__init__(paxos, prog='PFSProbe', opt='queue')

    def parse(self, lines):
        ret0 = []
        ret1 = []
        header0 = None
        header = None
        value = None
        for line in lines.splitlines():
            mat = re.match(self.pat0, line)
            if mat:
                header0 = self._init_list()
                header0.extend(mat.groups())
                continue
            mat = re.match(self.pat1, line)
            if mat:
                value = header0[:]
                value.append(mat.group(3))
                ret0.append(value)
                continue
            mat = re.match(self.pat2, line)
            if mat:
                header = header0[:]
                header.append(self.lbl2mode[mat.group(1)])
                continue
            value = self._parse_accept(line, header, value, ret1)
        self._padding(ret1, 26)
        return (ret0, ret1)


class PaxosProbeMeta(PaxosCommand):
    pat = re.compile('(\S+)\s+e_Cnt=(\d+)\s+Name\[([^]]+)\]\s+'
                     'Fd\[(\d+)\]\s+st_size\[(\d+)\]')


class PaxosPfsProbeMeta(PaxosProbeMeta):
    def __init__(self, paxos):
        super(PaxosPfsProbeMeta, self)\
            .__init__(paxos, prog='PFSProbe', opt='meta')


class PaxosLvProbeMeta(PaxosProbeMeta):
    def __init__(self, paxos):
        super(PaxosLvProbeMeta, self)\
            .__init__(paxos, prog='LVProbe', opt='meta')


class PaxosProbeFd(PaxosCommand):
    pat = re.compile('(\S+)\s+e_Cnt=(\d+)\s+Name\[([^]]+)\]\s+'
                     'Dir\[([^]]+)\]\s+Base\[([^]]+)\]\s+Fd\[(\d+)\]')


class PaxosPfsProbeFd(PaxosProbeFd):
    def __init__(self, paxos):
        super(PaxosPfsProbeFd, self)\
            .__init__(paxos, prog='PFSProbe', opt='fd')


class PaxosLvProbeFd(PaxosProbeFd):
    def __init__(self, paxos):
        super(PaxosLvProbeFd, self)\
            .__init__(paxos, prog='LVProbe', opt='fd')


class PaxosProbeBlock(PaxosCommand):

    pat0 = re.compile('###\s+BlockCache\s+Cnt=(\d+)\s+Max=(\d+)\s+'
                      'SegSize=(\d+)\s+SegNum=(\d+)\s+bCksum=(\d+)\s+'
                      'UsecWB=(\d+)')
    pat1 = re.compile('(0x\w+)\s+e_Cnt=(\d+)\s+Name\[([^]]+)\]\s+' +
                      'Off\[(\d+)\]\s+Len=(\d+)\s+Flags=(\w+)')
    pat2 = re.compile('Cksum64\[(\d+)\]=(\d+)\s+\[([^]]+)\]\s+(.+)$')

    def parse(self, lines):
        ret = []
        header = None
        value = None
        for line in lines.splitlines():
            if not header:
                mat = re.match(self.pat0, line)
                if mat:
                    header = mat.groups()
                    continue
            mat = re.match(self.pat1, line)
            if mat:
                value = self._init_list()
                value.extend(mat.groups())
                ret.append(value)
                continue
        return ret


class PaxosPfsProbeBlock(PaxosProbeBlock):
    def __init__(self, paxos):
        super(PaxosPfsProbeBlock, self)\
            .__init__(paxos, prog='PFSProbe', opt='block')


class PaxosLvProbeBlock(PaxosProbeBlock):
    def __init__(self, paxos):
        super(PaxosLvProbeBlock, self)\
            .__init__(paxos, prog='LVProbe', opt='block')


class PaxosDiskSpace(PaxosCommand):

    def command(self):
        return 'df -h {home}'.format(home=self.paxos.home)

    def parse(self, lines):
        value = self._init_list()
        value.extend(lines.splitlines()[1].split())
        return [value]


class PaxosAdmin(PaxosCommand):
    def __init__(self, paxos):
        super(PaxosAdmin, self)\
            .__init__(paxos, prog='PaxosAdmin', opt='control')

    def parse(self, lines):
        params = dict()
        server = dict()
        values = []
        for line in lines.splitlines():
            if line.startswith(' '):
                values[-1] = values[-1] + line.rstrip()
            else:
                values.append(line.rstrip())
        srv_pat = re.compile(r'(\d+):\[([^:]+):(\d+)\]\s+(\w+)')
        wd_pat = re.compile(r'(\w+)\[(\d+)\]')
        for val in values:
            mat = re.match(srv_pat, val)
            if mat:
                if int(mat.group(3)) > 0:
                    server[int(mat.group(1))] = dict(
                        no=int(mat.group(1)),
                        host=mat.group(2),
                        port=int(mat.group(3)),
                        state=True if int(mat.group(4), 0) == 3 else False)
            else:
                for word in val.split():
                    mat = re.match(wd_pat, word)
                    if mat:
                        params[mat.group(1)] = int(mat.group(2))

        ret = dict()
        if 'IamMaster' in params and params['IamMaster'] == 1:
            ret['ismaster'] = True
        else:
            ret['ismaster'] = False

        if 'NextSeq' in params:
            ret['consensus'] = params['NextSeq']
        else:
            ret['consensus'] = None

        if 'Birth' in params:
            ret['birth'] = strftime("%Y/%m/%d %H:%M:%S",
                                    localtime(params['Birth']))
        else:
            ret['birth'] = None

        ret['connection'] = dict(
            [(idx, server[idx]['state']) for idx in server.keys()])

        return ret
