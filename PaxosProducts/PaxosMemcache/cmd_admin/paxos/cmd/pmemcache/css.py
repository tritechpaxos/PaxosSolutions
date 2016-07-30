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
from os.path import expanduser
import logging
import os.path
import sys
import os
import re
import errno
import shutil
from subprocess import Popen, PIPE, STDOUT, check_output, CalledProcessError
from time import sleep
import itertools
import re
import signal
import fabric.tasks
import fabric.state

from . import config, util, fabfile, node, error, memcache, model


logger = logging.getLogger(__name__)


class CssServer(object):

    TAG = 'server'
    MAX_PORT = 65535

    def __init__(self, no, udp, tcp, core=True):
        self.no = no
        self.core = core
        try:
            self.uaddr, uport = udp.split(':')
            self.uport = int(uport)
            self.taddr, tport = tcp.split(':')
            self.tport = int(tport)
        except ValueError as e:
            msg = _(
                'non-numeric value has been specified for the port number')
            raise error.IllegalParamError(msg)
        if self.uport > self.MAX_PORT or self.tport > self.MAX_PORT:
            raise error.IllegalParamError(
                _('port number should be {0} or less').format(self.MAX_PORT))

    def to_conf(self):
        taddr_txt = (
            self.taddr.hostname
            if isinstance(self.taddr, node.Node) else self.taddr)
        return '{no} {uaddr} {uport} {taddr} {tport}'.format(
            no=self.no, uaddr=self.uaddr, uport=self.uport,
            taddr=taddr_txt, tport=self.tport)

    def __str__(self):
        taddr_txt = (
            self.taddr.hostname
            if isinstance(self.taddr, node.Node) else self.taddr)
        return '[{no}, "{uaddr}", {uport}, "{taddr}", {tport}]'.format(
            no=self.no, uaddr=self.uaddr, uport=self.uport,
            taddr=taddr_txt, tport=self.tport)

    def node(self):
        if isinstance(self.taddr, node.Node):
            return self.taddr
        else:
            return node.Node.get(self.taddr)

    def register_node(self, name):
        if not isinstance(self.taddr, node.Node):
            try:
                n = node.Node.get(self.taddr)
            except:
                n = node.Node(hostname=self.taddr, groups=[name], user='paxos')
                node.Node.append(n)
            self.taddr = n

    def in_use(self, nd):
        if isinstance(self.taddr, node.Node):
            return self.taddr == nd
        else:
            return self.taddr == nd.hostname

    def same_host(self, other):
        if self.uport == other.uport and self.uaddr == other.uaddr:
            return True
        if self.tport == other.tport and self.taddr == other.taddr:
            return True
        return False


class Css(model.Model):

    TAG = 'css'
    KEY = 'name'
    NAME_PATTERN = re.compile(r'^\w+$')

    def __init__(self, name, servers):
        if not self.NAME_PATTERN.match(name):
            raise error.IllegalNameError(_('illegal css name'))
        self.name = name
        self.servers = []
        no = 0
        for stuple in itertools.izip_longest(*[iter(servers)]*2):
            server = CssServer(no, *stuple)
            self.servers.append(server)
            no = no + 1
        if self.same_host():
            m = _('the same address has been specified more than once')
            raise error.DuplicationError(m)

    def same_host(self):
        for idx, sv0 in enumerate(self.servers[:-1]):
            for sv1 in self.servers[(idx+1):]:
                if sv0.same_host(sv1):
                    return True
        return False

    def match(self, kv):
        if 'name' in kv and self.name != kv['name']:
            return False
        return True

    def register_nodes(self):
        for sv in self.servers:
            sv.register_node(self.name)

    def _make_cell_dir(self):
        util.makedirs(config.CONFIG_CELL_DIR)

    def _make_data_dirs(self, css, srv):
        for fmt in config.DATA_CELL_FORMAT_DIRS:
            util.makedirs(fmt.format(cell=css.name, no=srv.no))

    def generate_conf(self):
        self._make_cell_dir()
        fname = expanduser(config.CONFIG_CELL_FORMAT.format(cell=self.name))
        with open(fname, 'w') as f:
            for sv in self.servers:
                f.write(sv.to_conf() + '\n')

    def delete_instance(self):
        super(Css, self).delete_instance()
        fname = expanduser(config.CONFIG_CELL_FORMAT.format(cell=self.name))
        os.remove(fname)

    def find_server(self, no):
        ret = [srv for srv in self.servers if srv.no == no]
        return ret[0] if len(ret) == 1 else ret

    def in_use(self, nd):
        return any([srv.in_use(nd) for srv in self.servers])

    def add_extension(self, addr):
        srv = CssServer(len(self.servers), addr[0], addr[1], core=False)
        srv.register_node(self.name)
        self.servers.append(srv)

    def get_extension(self):
        ret = [srv for srv in self.servers if not srv.core]
        return ret[0] if len(ret) == 1 else ret

    def change_member(self, old_no, new_no):
        old_srv = self.find_server(old_no)
        logger.debug("OLD: %s", str(old_srv))
        new_srv = self.find_server(new_no)
        logger.debug("NEW: %s", str(new_srv))
        self.servers.remove(new_srv)
        idx = self.servers.index(old_srv)
        self.servers[idx] = new_srv
        new_srv.no = old_no
        new_srv.core = True
        self.generate_conf()
        self.save()
        logger.debug("CSS: %s", str(self))

    def __str__(self):
        txt = 'name: {name}\nservers:\n'.format(name=self.name)
        for s in self.servers:
            txt += '  - {0}\n'.format(str(s))
        return txt


util.register_class(Css, '!' + Css.TAG)
util.register_class(CssServer, '!' + CssServer.TAG)


class _StartStopCommand(model.Command):
    def __init__(self, name=None, *args, **kwargs):
        super(_StartStopCommand, self).__init__(*args, **kwargs)
        self.tgt = Css.find({}) if name is None else [Css.get(name)]

    def run(self):
        ret = [self.op_per_css(css) for css in self.tgt]
        return 1 if any((r is not None for r in ret)) else None

    def match_server(self, srv):
        return util.match_hostaddr(srv.uaddr) or util.match_hostaddr(srv.taddr)

    def op_per_srv(self, css, srv):
        if not self.match_server(srv):
            op = self.op_css_srv_remote
        else:
            op = self.op_css_srv_local
        return op(css, srv)

    def op_per_css(self, css):
        return [self.op_per_srv(css, srv) for srv in css.servers]

    def op_css_srv_remote(self, css, srv):
        n = srv.taddr
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        fabric.state.env.hosts = [n.hostname]
        if n.user is not None:
            fabric.state.env.user = n.user
        if n.port is not None:
            fabric.state.env.port = n.port
        return self.exec_srv_remote(css, srv)


class _LocalCommand(model.Command):
    def __init__(self, name, no, *args, **kwargs):
        super(_LocalCommand, self).__init__(*args, **kwargs)
        self.css = Css.get(name)
        self.no = no

    def run(self):
        for srv in self.css.servers:
            if srv.no == self.no and self.match_server(srv):
                self.op_css_srv_local(self.css, srv)

    def match_server(self, srv):
        return util.match_hostaddr(srv.uaddr) or util.match_hostaddr(srv.taddr)

    def get_shutdown(self, css, srv):
        data_dir = config.DATA_CELL_FORMAT.format(cell=css.name, no=srv.no)
        if not os.path.exists(data_dir):
            return 0
        path = os.path.join(data_dir, 'SHUTDOWN')
        if not os.path.exists(path):
            return None
        with open(path) as f:
            txt = f.read()
            w = txt.strip().split()
            return int(w[0])


class LstartCommand(_LocalCommand):
    def __init__(self, initialize=False, *args, **kwargs):
        super(LstartCommand, self).__init__(*args, **kwargs)
        self.initialize = initialize

    def op_css_srv_local(self, css, srv):
        data_dir = config.DATA_CELL_FORMAT.format(cell=css.name, no=srv.no)
        args = [config.CMD_PAXOS_CSS, css.name, str(srv.no), data_dir]

        if self.initialize:
            try:
                shutil.rmtree(data_dir)
            except OSError as e:
                if e.errno != errno.ENOENT:
                    raise e
            seq = 0
        else:
            seq = self.get_shutdown(css, srv)
        if srv.core and seq is not None:
            args.append('TRUE')
            args.append(str(seq))
            if seq == 0:
                css._make_data_dirs(css, srv)
        else:
            css._make_data_dirs(css, srv)
            args.append('FALSE')

        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=css.name))
        env = {config.ENV_CONFIG_CELL: conf}
        logger.info('call %s', ' '.join(args))
        p0 = Popen(args, stdout=PIPE, stderr=STDOUT, env=env)
        p1 = Popen(["logger", "-t", "PFSServer-{0}#{1}".format(
            css.name, srv.no)], stdin=p0.stdout)
        p0.stdout.close()
        sleep(1)
        return p0


class StartCommand(_StartStopCommand):

    def __init__(self, initialize=False, *args, **kwargs):
        super(StartCommand, self).__init__(*args, **kwargs)
        self.initialize = initialize

    def initialize_start(self, css, srv_status):
        if any((st['status'] == config.PROC_STATE_RUNNING
                for st in srv_status)):
            raise error.NotStoppedError(_('exist server running'))
        ret = []
        for srv in css.servers:
            r = self.op_per_srv(css, srv)
            ret.append(r)
            sleep(2)
        return ret if any((r is not None for r in ret)) else None

    def op_per_css(self, css):
        cmd = StateCommand(name=css.name, mode='long', verbose=self.verbose)
        srv_status = cmd.op_per_css(css)
        logger.debug(srv_status)

        if self.initialize or all((st['next_sequence'] is None
                                   for st in srv_status
                                   if st['status'] != config.PROC_STATE_ERROR
                                   )):
            logger.info('initialize css: %s', css.name)
            self.initialize = True
            return self.initialize_start(css, srv_status)

        next_seq = dict([(int(st['No.']),
                          int(st['next_sequence'])
                          if st['next_sequence'] is not None else -1)
                        for st in srv_status
                        if st['status'] == config.PROC_STATE_STOPPED])
        ret = []
        for no, ns in sorted(next_seq.items(), key=lambda x: x[1],
                             reverse=True):
            srv = css.find_server(no)
            r = self.op_per_srv(css, srv)
            ret.append(r)
            sleep(2)
        return ret if any((r is not None for r in ret)) else None

    def exec_srv_remote(self, css, srv):
        try:
            fabric.tasks.execute(fabfile.css_start, css.name, srv.no,
                                 self.initialize)
        except error.FabError as e:
            logger.debug('', exc_info=True)
            if re.search('not found: {0}'.format(css.name), e.message):
                logger.error(
                    _('configuration file to {0} has not been deployed')
                    .format(srv.taddr.hostname))
            else:
                logger.error(
                    _('{0}: start operation to this host was not accepted')
                    .format(srv.taddr.hostname))
            return False

    def op_css_srv_local(self, css, srv):
        cmd = LstartCommand(name=css.name, no=srv.no,
                            initialize=self.initialize, verbose=self.verbose)
        cmd.run()


class LstopCommand(_LocalCommand, model.BaseStopCommand):

    def __init__(self, force=False, *args, **kwargs):
        super(LstopCommand, self).__init__(*args, **kwargs)
        self.force = force

    def op_css_srv_local(self, css, srv):
        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=css.name))
        env = {config.ENV_CONFIG_CELL: conf}
        args = [config.CMD_PAXOS_SHUTDOWN, css.name, str(srv.no)]
        logger.info('call %s', ' '.join(args))
        try:
            out = util.check_output_timeout(args, env=env, stderr=STDOUT)
        except CalledProcessError as e:
            if not self.force:
                pass
            else:
                self.force_stop(
                    'PFSServer',
                    lambda w: css.name == w[2] and str(srv.no) == w[3])
        except error.TimeoutError:
            if not self.force:
                logger.error(_("timeout: {0}").format(' '.join(args)))
            else:
                self.force_stop(
                    'PFSServer',
                    lambda w: css.name == w[2] and str(srv.no) == w[3])


class StopCommand(_StartStopCommand):

    def __init__(self, force=False, *args, **kwargs):
        super(StopCommand, self).__init__(*args, **kwargs)
        self.force = force

    def in_use(self, css, srv_status):
        logger.debug(srv_status)
        cond = [len(st['client_list']) > 0 for st in srv_status
                if st['status'] == config.PROC_STATE_RUNNING]
        return len(cond) > 0 and all(cond)

    def srv_loop(self, css, ev):
        ret = [self.op_per_srv(css, srv) for srv in css.servers if ev(srv.no)]
        return ret if any((r is not None for r in ret)) else None

    def op_per_css(self, css):
        cmd = StateCommand(name=css.name, mode='long')
        srv_status0 = cmd.op_per_css(css)
        if not(self.force) and self.in_use(css, srv_status0):
            raise error.InUseError(_('this css is in use.'))
        srv_status = dict(((int(st['No.']), st) for st in srv_status0))
        ret = []
        r = self.srv_loop(css, lambda no: not (
                          no in srv_status and 'master' in srv_status[no] and
                          srv_status[no]['master']))
        if r is not None:
            ret.extend(r)
        r = self.srv_loop(css, lambda no: no in srv_status and
                          'master' in srv_status[no] and
                          srv_status[no]['master'])
        if r is not None:
            ret.extend(r)
        return ret if any((r is not None for r in ret)) else None

    def op_css_srv_local(self, css, srv):
        cmd = LstopCommand(name=css.name, no=srv.no, verbose=self.verbose,
                           force=self.force)
        cmd.run()

    def exec_srv_remote(self, css, srv):
        try:
            fabric.tasks.execute(fabfile.css_stop, css.name, srv.no,
                                 self.force)
        except (error.FabError, EOFError):
            logger.warning(
                _('{0}: stop operation to this host was not accepted.')
                .format(srv.taddr.hostname))
            return False


class LstateCommand(_LocalCommand):

    CTRL_RE0 = re.compile(
        'MyId\[(-?\d+)\]\s+IamMaster\[(-?\d+)\]' +
        '\s+MyRnd\[(-?\d+)\]\s+epoch\[(-?\d+)\]')
    CTRL_RE1 = re.compile(
        '\s*Elected\[(-?\d+)\]\s+Reader\[(-?\d+)\]\s+Vote\[(-?\d+)\]\s+' +
        'Birth\[(\d+)\]\s+Suppress\[(-?\d+)\]')
    CTRL_RE2 = re.compile('NextSeq\[(\d+)\]')

    CTRL_RE3 = re.compile(
        '(\d+):\[([^:]+):(\d+)\]\s+(\S+)\s+Leader\[-?\d+\]\s+Epoch\[\d+\]' +
        '\s+Birth\[(\d+)\]\s+Load\[\d+\]')

    CTRL_RE4 = re.compile(
        'ClientId\s+\[([^-]+)-\d+\]\s+Pid=(\d+)-\d+' +
        '\s+Seq=\d+\s+Reuse=\d+\s+Try=\d+')

    def __init__(self, mode=None, *args, **kwargs):
        super(LstateCommand, self).__init__(*args, **kwargs)
        if mode is None:
            self.mode = 'normal'
        else:
            self.mode = mode

    def _proc_info(self, css, srv):
        args = ['ps', '-C', 'PFSServer', '-o', 'pid,args=', '--no-headers']
        logger.info('call %s', ' '.join(args))
        try:
            out = util.check_output_timeout(args)
            lines = out.splitlines()
            ret = []
            for line in lines:
                w = line.strip().split()
                if w[2] == css.name and int(w[3]) == srv.no:
                    ret.append(int(w[0]))
            return ret[0] if len(ret) == 1 else ret
        except CalledProcessError:
            return []
        except error.TimeoutError:
            logger.error(_("timeout: {0}").format(' '.join(args)))
            return []

    def _exec_admin_cmd(self, css, srv):
        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=css.name))
        env = {config.ENV_CONFIG_CELL: conf}
        args = [config.CMD_PAXOS_ADMIN, css.name, str(srv.no), 'control']
        logger.info('call %s', ' '.join(args))
        try:
            lines = util.check_output_timeout(args, env=env,
                                              stderr=STDOUT).splitlines()
        except error.TimeoutError as e:
            logger.error(_("timeout: {0}").format(' '.join(args)))
            raise e
        header = lines[:4]
        mat = self.CTRL_RE0.match(header[1])
        no = int(mat.group(1))
        is_master = int(mat.group(2)) != 0
        mat = self.CTRL_RE1.match(header[2])
        birth = int(mat.group(4))
        mat = self.CTRL_RE2.match(header[3])
        next_seq = int(mat.group(1))

        conn = []
        lines[:4] = []
        for stuple in itertools.izip_longest(*[iter(lines)]*3):
            line = stuple[0]
            mat = self.CTRL_RE3.match(line)
            n = int(mat.group(1))
            addr0 = mat.group(2)
            if n == srv.no and util.match_hostaddr(addr0):
                addr = addr0
                port = int(mat.group(3))
            state = int(mat.group(4), 16)
            if state == 3:
                conn.append(n)

        ret = {'No.': srv.no, 'address': addr, 'port': port,
               'status': config.PROC_STATE_RUNNING}
        if self.mode == 'long':
            ret['master'] = is_master
            ret['birth'] = birth
            ret['next_sequence'] = next_seq
            ret['connected'] = conn
            ret['pid'] = self._proc_info(css, srv)
            ret['client_list'] = self.get_clients(css, srv)
        return ret

    def get_clients(self, css, srv):
        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=css.name))
        env = {config.ENV_CONFIG_CELL: conf}
        args = [config.CMD_PAXOS_PFSPROBE, css.name, str(srv.no), 'accept']
        logger.info('call %s', ' '.join(args))
        try:
            lines = util.check_output_timeout(args, env=env,
                                              stderr=STDOUT).splitlines()
        except error.TimeoutError as e:
            logger.error(_("timeout: {0}").format(' '.join(args)))
            raise e
        result = {}
        for line in lines:
            mat = self.CTRL_RE4.match(line)
            if not mat:
                continue
            addr = mat.group(1)
            pid = int(mat.group(2))
            tp = (addr, pid)
            result[tp] = 1
        return [tp[0] for tp in result.keys()]

    def exec_admin_cmd(self, css, srv):
        try:
            return self._exec_admin_cmd(css, srv)
        except CalledProcessError:
            ret = {'No.': srv.no, 'address': srv.uaddr, 'port': srv.uport,
                   'status': config.PROC_STATE_STOPPED}
            if self.mode == 'long':
                ret['next_sequence'] = self.get_shutdown(css, srv)
            return ret

    def op_css_srv_local(self, css, srv):
        result = self.exec_admin_cmd(css, srv)
        for k, v in result.iteritems():
            print '{0}: {1}'.format(k, v if v is not None else '')


class StateCommand(_StartStopCommand):

    KEYS_EVAL = ['master', 'client_list']

    def __init__(self, mode=None, *args, **kwargs):
        super(StateCommand, self).__init__(*args, **kwargs)
        if mode is None:
            self.mode = 'normal'
        else:
            self.mode = mode

    def run(self):
        if self.mode == 'short':
            self.run_short()
        elif self.mode == 'quiet':
            return self.run_quiet()
        else:
            self.run_normal()

    def run_normal(self):
        for css in self.tgt:
            print '---'
            print 'name: {0}'.format(css.name)
            print 'server:'
            svs = self.op_per_css(css)
            for sv in svs:
                self.print_srv_normal(sv)

    def print_srv_normal(self, srv, indent='  '):
        print '{0}-'.format(indent)
        for k, v in srv.iteritems():
            print '{0}  {1}: {2}'.format(indent, k, v)

    def exec_short(self):
        ret = {}
        for css in self.tgt:
            svs = self.op_per_css(css)
            ret[css.name] = self.print_srv_short(svs)
        return ret

    def run_short(self):
        result = self.exec_short()
        logger.debug(result)
        for k, v in result.iteritems():
            print '{0}: {1}'.format(k, v)

    def run_quiet(self):
        result = self.exec_short()
        logger.debug(result)
        for status in result.values():
            if status != config.PROC_STATE_RUNNING:
                return 1

    def print_srv_short(self, srvs):
        status = list(set([sv['status'] for sv in srvs]))
        if len(status) == 1:
            return status[0]
        else:
            return '[{0}]'.format(', '.join(status))

    def op_css_srv_local(self, css, srv):
        cmd = LstateCommand(name=css.name, no=srv.no, mode=self.mode,
                            verbose=self.verbose)
        return cmd.exec_admin_cmd(css, srv)

    def exec_srv_remote(self, css, srv):
        try:
            result = fabric.tasks.execute(fabfile.css_state, css.name, srv.no,
                                          self.mode)
            lines = result[srv.taddr.hostname].splitlines()
            ret = dict()
            for line in lines:
                k, v = line.split(': ', 2)
                ret[k] = ((v if k not in StateCommand.KEYS_EVAL else eval(v))
                          if isinstance(v, str) and v != '' else None)
            return ret
        except (error.FabError, EOFError):
            return {'No.': srv.no, 'address': srv.uaddr, 'port': srv.uport,
                    'status': config.PROC_STATE_ERROR}


class LreplaceCommand(_LocalCommand):

    def __init__(self, mode, old=None, *args, **kwargs):
        super(LreplaceCommand, self).__init__(*args, **kwargs)
        self.mode = mode
        self.old_no = old

    def remove_dirs(self, css, srv):
        data_dir = config.DATA_CELL_FORMAT.format(cell=css.name, no=srv.no)
        try:
            shutil.rmtree(data_dir)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise e

    def move_dirs(self, css, srv):
        data0_dir = config.DATA_CELL_FORMAT.format(cell=css.name,
                                                   no=self.old_no)
        data_dir = config.DATA_CELL_FORMAT.format(cell=css.name, no=srv.no)
        shutil.move(data0_dir, data_dir)

    def change_server(self, css, srv):
        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=css.name))
        env = {config.ENV_CONFIG_CELL: conf}
        args = [config.CMD_PAXOS_CHANGE_MEMBER, css.name, str(srv.no),
                srv.taddr.hostname, str(srv.tport)]
        logger.info('call %s', ' '.join(args))
        try:
            out = util.check_output_timeout(args, env=env, stderr=STDOUT)
        except CalledProcessError as e:
            pass
        except error.TimeoutError:
            logger.error(_("timeout: {0}").format(' '.join(args)))

    def op_css_srv_local(self, css, srv):
        if self.mode == 'rm':
            self.remove_dirs(css, srv)
        elif self.mode == 'mv':
            self.move_dirs(css, srv)
        elif self.mode == 'srv':
            self.change_server(css, srv)


class ReplaceCommand(_StartStopCommand):

    def __init__(self, name, no, address, cold=False, force=False,
                 *args, **kwargs):
        super(ReplaceCommand, self).__init__(*args, **kwargs)
        self.css = Css.get(name)
        self.no = no
        self.address = address
        self.cold_start = cold
        self.force = force
        logger.debug("name=%s no=%s %s", name, str(no), str(address))

    def running_check(self):
        cmd = StateCommand(name=self.css.name)
        srv_st = {}
        for st in cmd.op_per_css(self.css):
            srv_st[int(st['No.'])] = st['status']
        logger.debug(srv_status)
        return result

    def state_check(self):
        cmd = StateCommand(name=self.css.name)
        sts = cmd.op_per_css(self.css)
        status = dict(((int(st['No.']), st) for st in sts))
        self.error_node = (status[self.no]['status'] ==
                           config.PROC_STATE_ERROR)
        if all((st['status'] == config.PROC_STATE_RUNNING
               for st in sts if int(st['No.']) != self.no)):
            self.running = True
            return self.force or not self.error_node
        if all((st['status'] == config.PROC_STATE_STOPPED for st in sts)):
            self.running = False
            self.cold_start = True
            return True
        if self.force and all((st['status'] != config.PROC_STATE_RUNNING
                               for st in sts)):
            self.running = False
            self.cold_start = True
            return True
        return False

    def paxos_installed(self, srv):
        n = srv.node()
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        fabric.state.env.hosts = [n.hostname]
        if n.user is not None:
            fabric.state.env.user = n.user
        if n.port is not None:
            fabric.state.env.port = n.port
        ret = fabric.tasks.execute(fabfile.paxos_installed)
        return (n.hostname in ret) and ret[n.hostname]

    def precheck(self):
        sv3 = CssServer(3, self.address[0], self.address[1], core=False)
        if any((sv3.same_host(sv) for sv in self.css.servers)):
            m = _('the same address has been specified more than once')
            raise error.IllegalStateError(m)
        if not self.state_check():
            msg = _('cannot be members change because not running servers')
            raise error.IllegalStateError(msg)
        if not self.paxos_installed(sv3):
            msg = _('it does not properly install PAXOS to {0}').format(
                sv3.node().hostname)
            raise error.IllegalStateError(msg)

    def run(self):
        self.precheck()
        self.update_conf_file()
        if not self.cold_start:
            self.update_ext_server()
        self.remove_old_server()
        self.replace_server()

    def update_conf_file(self):
        self.css.add_extension(self.address)
        logger.debug("CSS=%s", str(self.css))
        self.css.generate_conf()
        self.css.save()
        srv = self.css.get_extension()
        self.old_no = srv.no
        if not self.cold_start:
            logger.debug("server=%s", str(srv))
            cmd = node.DeployCommand(hostname=srv.taddr.hostname)
            cmd.run()

    def start_ext_server(self):
        cmd = StartCommand(name=self.css.name)
        srv = self.css.get_extension()
        logger.debug("start_ext_server: {0}".format(srv))
        cmd.op_per_srv(self.css, srv)

    def stop_ext_server(self):
        cmd = StopCommand(name=self.css.name)
        srv = self.css.get_extension()
        logger.debug("stop_ext_server: {0}".format(srv))
        cmd.op_per_srv(self.css, srv)

    def catch_up(self):
        srv = self.css.get_extension()
        logger.debug("catch_up: {0}".format(srv))
        cmd = StateCommand(name=self.css.name, mode='long',
                           verbose=self.verbose)
        srv_status = cmd.op_per_css(self.css)
        seq = None
        for st in srv_status:
            if srv.no == int(st['No.']):
                if st['status'] != config.PROC_STATE_RUNNING:
                    return False
                return seq == int(st['next_sequence'])
            else:
                if seq is None and st['status'] == config.PROC_STATE_RUNNING:
                    seq = int(st['next_sequence'])
        return False

    def wait_until_catch_up(self, loop=10):
        for i in range(loop):
            if self.catch_up():
                logger.debug("catch_up ok")
                return
            sleep(1)
        raise error.IllegalStateError(_('cannot start the css server'))

    def update_ext_server(self):
        logger.debug("update_ext_server")
        self.start_ext_server()
        self.wait_until_catch_up()
        self.stop_ext_server()

    def stop_old_server(self):
        cmd = StopCommand(name=self.css.name)
        srv = self.css.find_server(self.no)
        logger.debug("SERVER: %s", srv)
        cmd.op_per_srv(self.css, srv)
        logger.debug("STOP old server")

    def remove_old_dirs(self):
        logger.debug("remove_old_dir: {0}".format(self.no))
        srv = self.css.find_server(self.no)
        n = srv.taddr
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        fabric.state.env.hosts = [n.hostname]
        if n.user is not None:
            fabric.state.env.user = n.user
        if n.port is not None:
            fabric.state.env.port = n.port
        fabric.tasks.execute(fabfile.css_replace, self.css.name, srv.no,
                             '--remove-dir')

    def remove_old_server(self):
        logger.debug("remove_old_server")
        if not self.error_node:
            self.stop_old_server()
            self.remove_old_dirs()
            old_srv = self.css.find_server(self.no)
        srv = self.css.get_extension()
        logger.debug("NEW: %s", str(srv))
        self.css.change_member(self.no, srv.no)
        logger.debug("CSS: %s", str(self.css))
        for srv in self.css.servers:
            cmd = node.DeployCommand(hostname=srv.taddr.hostname)
            cmd.run()
        if not self.error_node:
            node.DeployCommand(hostname=old_srv.taddr.hostname).run()

    def replace_css_dir(self):
        logger.debug("replace_css_dir: {0}".format(self.no))
        srv = self.css.find_server(self.no)
        n = srv.taddr
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        fabric.state.env.hosts = [n.hostname]
        if n.user is not None:
            fabric.state.env.user = n.user
        if n.port is not None:
            fabric.state.env.port = n.port
        fabric.tasks.execute(
            fabfile.css_replace, self.css.name, srv.no,
            '--replace-dir', '--old', str(self.old_no))

    def start_new_server(self):
        cmd = StartCommand(name=self.css.name)
        srv = self.css.find_server(self.no)
        logger.debug("start new server: %s", str(srv))
        cmd.op_per_srv(self.css, srv)

    def exec_change_member(self):
        for srv in self.css.servers:
            if srv.no != self.no:
                continue
            n = srv.taddr
            if self.verbose == 0:
                fabric.state.output['everything'] = False
                fabric.state.output['aborts'] = False
            fabric.state.env.hosts = [n.hostname]
            if n.user is not None:
                fabric.state.env.user = n.user
            if n.port is not None:
                fabric.state.env.port = n.port
            fabric.tasks.execute(fabfile.css_replace, self.css.name, srv.no,
                                 '--change-srv')

    def replace_server(self):
        logger.debug("replace_server")
        if not self.cold_start:
            self.replace_css_dir()
        if self.running:
            self.start_new_server()
            self.exec_change_member()


class RegisterCommand(model.Command):
    def __init__(self, name, servers, *args, **kwargs):
        super(RegisterCommand, self).__init__(*args, **kwargs)
        self.name = name
        self.servers = servers

    def run(self):
        if Css.exist(self.name):
            raise error.ExistError(_('css already exists'))
        css = Css(self.name, self.servers)
        css.register_nodes()
        Css.append(css)
        css.generate_conf()


class UnregisterCommand(model.Command):
    def __init__(self, name, *args, **kwargs):
        super(UnregisterCommand, self).__init__(*args, **kwargs)
        self.name = name

    def in_use(self, css):
        return any([mc.in_use(css) for mc in memcache.Memcache.find()])

    def running(self, css):
        cmd = StateCommand(name=css.name, mode='short', verbose=self.verbose)
        result = cmd.exec_short()
        if result is None or css.name not in result:
            return False
        status = result[css.name]
        if isinstance(status, list):
            return config.PROC_STATE_RUNNING in status
        else:
            return status == config.PROC_STATE_RUNNING

    def run(self):
        css = Css.get(self.name)
        if self.in_use(css):
            raise error.InUseError(_('this css is in use.'))
        if self.running(css):
            raise error.NotStoppedError(_('this css is running.'))
        css.delete_instance()


class ShowCommand(model.Command):
    def __init__(self, name, *args, **kwargs):
        super(ShowCommand, self).__init__(*args, **kwargs)
        self.search = {}
        if name is not None:
            self.search['name'] = name

    def run(self):
        for css in Css.find(self.search):
            txt = '---\n'
            txt += 'name: {0}\n'.format(css.name)
            txt += 'server:\n'
            for srv in css.servers:
                taddr = (srv.taddr.hostname
                         if isinstance(srv.taddr, node.Node) else srv.taddr)
                txt += '  -\n'
                txt += '    {0}: {1}\n'.format('No.', srv.no)
                txt += '    {0}: {1}\n'.format('address_udp', srv.uaddr)
                txt += '    {0}: {1}\n'.format('port_udp', srv.uport)
                txt += '    {0}: {1}\n'.format('address_tcp', taddr)
                txt += '    {0}: {1}\n'.format('port_tcp', srv.tport)
            print txt,


class ListCommand(model.Command):
    def run(self):
        result = Css.find({})
        for obj in result:
            print obj.name
