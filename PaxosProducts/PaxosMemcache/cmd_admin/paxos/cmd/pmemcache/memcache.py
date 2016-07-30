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
from os import putenv
from os.path import expanduser
import sys
import socket
import logging
from string import Formatter
from subprocess import Popen, PIPE, STDOUT, CalledProcessError
from time import sleep
import itertools
import re

from . import config, util, error, model


logger = logging.getLogger(__name__)


class MemcachePair(object):

    LOCALHOST = '127.0.0.1'
    PORT = 11211
    TAG = 'pair'

    def __init__(self, shost, chost, sport, cport,):
        self.shost = shost
        self.chost = chost
        self.sport = sport
        self.cport = cport

    def client_host(self):
        return self.chost if self.chost is not None else self.LOCALHOST

    def client_port(self):
        return self.cport if self.cport is not None else self.PORT

    def client_addr(self):
        return '{chost}:{cport}'.format(chost=self.client_host(),
                                        cport=self.client_port())

    def server_host(self):
        return self.shost

    def server_port(self):
        return self.sport if self.sport is not None else self.PORT

    def server_addr(self):
        return '{shost}:{sport}'.format(shost=self.server_host(),
                                        sport=self.server_port())

    def __str__(self):
        return '["{chost}", {cport}, "{shost}", {sport}]'.format(
            chost=self.client_host(), shost=self.server_host(),
            cport=self.client_port(), sport=self.server_port())

    def norm_hostname(self, hostname):
        try:
            return socket.gethostbyname(hostname)
        except socket.gaierror:
            return hostname

    def _same_host(self, _h0, _h1):
        h0 = _h0 if _h0 is not None else self.LOCALHOST
        h1 = _h1 if _h1 is not None else self.LOCALHOST
        h0_addr = self.norm_hostname(h0)
        h1_addr = self.norm_hostname(h1)
        return h0_addr == h1_addr

    def _same_port(self, _p0, _p1):
        p0 = _p0 if _p0 is not None else self.PORT
        p1 = _p1 if _p1 is not None else self.PORT
        return int(p0) == int(p1)

    def _same_address(self, a0, a1):
        return self._same_host(a0[0], a1[0]) and self._same_port(a0[1], a1[1])

    def dup_address(self, pair):
        ret = self._same_address(
            (self.chost, self.cport), (pair.chost, pair.cport))
        if ret:
            return True
        ret = self._same_address(
            (self.shost, self.sport), (pair.shost, pair.sport))
        return ret

    def same(self, pair):
        ret = self._same_address(
            (self.chost, self.cport), (pair.chost, pair.cport))
        if not ret:
            return False
        ret = self._same_address(
            (self.shost, self.sport), (pair.shost, pair.sport))
        return ret


class Memcache(model.Model):

    TAG = 'memcache'
    KEY = 'id'

    def __init__(self, css, id=None, max_cache_item=None, checksum=None,
                 checksum_interval=None, expire_interval=None, worker=None,
                 name_space=None):
        self.css = css
        self.id = id
        self.max_cache_item = max_cache_item
        self.checksum = checksum
        self.checksum_interval = checksum_interval
        self.expire_interval = expire_interval
        self.worker = worker
        self.pair = []
        self.name_space = name_space

    def match(self, kv):
        if 'id' in kv and self.id != kv['id']:
            return False
        if 'css' in kv:
            if self.css.name != kv['css']:
                return False
        if 'name_space' in kv:
            if self.name_space != kv['name_space']:
                return False
        return True

    def add_pair(self, pair):
        if any(o.dup_address(pair) for o in self.pair):
            raise error.DuplicationError(_('address is duplicated'))
        self.pair.append(pair)

    def remove_pair(self, pair):
        result = filter(lambda o: o.same(pair), self.pair)
        if len(result) == 0:
            raise error.NotFoundError(_('not found pair'))
        assert len(result) == 1
        self.pair.remove(result[0])

    def in_use(self, css):
        from .css import Css
        if isinstance(self.css, Css):
            return self.css == css
        else:
            return self.css == css.name

    def __str__(self):
        from .css import Css
        txt = 'id: {id}\n'.format(id=self.id)
        txt += 'css: {css}\n'.format(css=self.css.name
                                     if isinstance(self.css, Css)
                                     else self.css)
        if self.max_cache_item is not None:
            txt += 'max cache item: {0}\n'.format(self.max_cache_item)
        if self.checksum is not None and not self.checksum:
            txt += 'checksum: {0}\n'.format(self.checksum)
        if self.checksum_interval is not None:
            txt += 'checksum interval: {0}\n'.format(self.checksum_interval)
        if self.expire_interval is not None:
            txt += 'expire interval: {0}\n'.format(self.expire_interval)
        if self.worker is not None:
            txt += 'worker: {0}\n'.format(self.worker)
        if self.name_space is not None:
            txt += 'name space: {0}\n'.format(self.name_space)
        txt += 'pair:\n'
        for p in self.pair:
            txt += '    - {0}\n'.format(str(p))
        return txt


util.register_class(Memcache, '!' + Memcache.TAG)
util.register_class(MemcachePair, '!' + MemcachePair.TAG)


def _running(memcache):
    cmd = StateCommand(id=memcache.id, css=None, mode='short')
    result = cmd.get_result()
    if result is None or memcache.id not in result:
        return False
    status = result[memcache.id]['status']
    return status == config.PROC_STATE_RUNNING


class RegisterCommand(model.Command):
    def __init__(self, css, id, max_cache_item, checksum, checksum_interval,
                 expire_interval, worker, name_space, *args, **kwargs):
        super(RegisterCommand, self).__init__(*args, **kwargs)
        from .css import Css
        m_css = Css.get(css)
        self.opt = {'id': id, 'css': m_css, 'name_space': name_space}
        if max_cache_item is not None:
            self.opt['max_cache_item'] = max_cache_item
        if checksum is not None:
            self.opt['checksum'] = checksum
        if checksum_interval is not None:
            self.opt['checksum_interval'] = checksum_interval
        if expire_interval is not None:
            self.opt['expire_interval'] = expire_interval
        if worker is not None:
            self.opt['worker'] = worker

    def run(self):
        if Memcache.exist(self.opt['id']):
            raise error.DuplicationError(_('memcache id is duplicated'))
        memcache = Memcache(**self.opt)
        Memcache.append(memcache)


class UnregisterCommand(model.Command):
    def __init__(self, id, *args, **kwargs):
        super(UnregisterCommand, self).__init__(*args, **kwargs)
        self.id = id

    def run(self):
        memcache = Memcache.get(self.id)
        if _running(memcache):
            raise error.NotStoppedError(_('this memcache is running.'))
        memcache.delete_instance()


class ModifyCommand(model.Command):
    def __init__(self, css, id, max_cache_item, checksum, checksum_interval,
                 expire_interval, worker, name_space, *args, **kwargs):
        super(ModifyCommand, self).__init__(*args, **kwargs)
        self.id = id
        self.opt = {}
        if css is not None:
            from .css import Css
            self.opt['css'] = Css.get(css)
        if name_space is not None:
            self.opt['name_space'] = name_space
        if max_cache_item is not None:
            self.opt['max_cache_item'] = max_cache_item
        if checksum is not None:
            self.opt['checksum'] = checksum
        if checksum_interval is not None:
            self.opt['checksum_interval'] = checksum_interval
        if expire_interval is not None:
            self.opt['expire_interval'] = expire_interval
        if worker is not None:
            self.opt['worker'] = worker

    def run(self):
        memcache = Memcache.get(self.id)
        for k, v in self.opt.iteritems():
            setattr(memcache, k, v)
        memcache.save()


class ShowCommand(model.Command):
    def __init__(self, id, css, name_space, *args, **kwargs):
        super(ShowCommand, self).__init__(*args, **kwargs)
        self.search = {}
        if id is not None:
            self.search['id'] = id
        if css is not None:
            self.search['css'] = css
        if name_space is not None:
            self.search['name_space'] = name_space

    def run(self):
        result = Memcache.find(self.search)
        for obj in result:
            print '---'
            print str(obj).strip()


class ListCommand(model.Command):
    def run(self):
        result = Memcache.find({})
        for obj in result:
            print obj.id


class PairAddCommand(model.Command):
    def __init__(self, id, server_address, client_address,
                 server_port, client_port, *args, **kwargs):
        super(PairAddCommand, self).__init__(*args, **kwargs)
        self.id = id
        self.pair = MemcachePair(
            server_address, client_address, server_port, client_port)

    def run(self):
        memcache = Memcache.get(self.id)
        memcache.add_pair(self.pair)
        memcache.save()


class PairRemoveCommand(model.Command):
    def __init__(self, id, server_address, client_address,
                 server_port, client_port, *args, **kwargs):
        super(PairRemoveCommand, self).__init__(*args, **kwargs)
        self.id = id
        self.pair = MemcachePair(
            server_address, client_address, server_port, client_port)

    def run(self):
        memcache = Memcache.get(self.id)
        memcache.remove_pair(self.pair)
        memcache.save()


class PairApplyCommand(model.Command):
    def __init__(self, id=None, *args, **kwargs):
        super(PairApplyCommand, self).__init__(*args, **kwargs)
        self.cmd = StateCommand(id, css=None)

    def pair_op(self, id, pair, op):
        pair_args = [
            config.CMD_PAXOS_MEMCACHE_ADM, str(id), op,
            pair.client_addr(), pair.server_addr()]
        logger.info("call %s", ' '.join(pair_args))
        try:
            util.check_call_timeout(pair_args)
        except CalledProcessError:
            logger.warning(
                _("not be able to {1} a pair (id={0})").format(id, op))
        except error.TimeoutError:
            logger.error(_("timeout: {0}").format(' '.join(pair_args)))

    def pair_add(self, id, pair):
        self.pair_op(id, pair, 'add')

    def pair_del(self, id, pair):
        self.pair_op(id, pair, 'del')

    def pair_apply(self, model, state):
        for p1 in model.pair:
            if all([not(p1.same(p0)) for p0 in state.pair]):
                self.pair_add(model.id, p1)
        for p0 in state.pair:
            if all([not(p0.same(p1)) for p1 in model.pair]):
                self.pair_del(model.id, p0)

    def run(self):
        current = self.cmd.get_result()
        for k, v in current.iteritems():
            if v['status'] != 'running':
                continue
            self.pair_apply(v['model'], v['state_obj'])


class StartCommand(model.Command):
    def __init__(self, id, retry=3, *args, **kwargs):
        super(StartCommand, self).__init__(*args, **kwargs)
        if id is None:
            self.tgt = Memcache.find({})
        else:
            self.tgt = [Memcache.get(id)]
        self.retry = retry

    def run(self):
        for memcache in self.tgt:
            if not _running(memcache):
                self.start_memcache(memcache)

    def _start_memcache(self, m):
        args = [config.CMD_PAXOS_MEMCACHE]
        if m.max_cache_item is not None:
            args.extend(['-I', str(m.max_cache_item)])
        if m.checksum is not None:
            args.extend(['-s', '1' if m.checksum else '0'])
        if m.checksum_interval is not None:
            args.extend(['-C', str(m.checksum_interval)])
        if m.expire_interval is not None:
            args.extend(['-E', str(m.expire_interval)])
        if m.worker is not None:
            args.extend(['-W', str(m.worker)])
        args.extend([str(m.id), m.css.name, m.name_space])
        conf = expanduser(config.CONFIG_CELL_FORMAT.format(cell=m.css.name))
        env = {config.ENV_CONFIG_CELL: conf, config.ENV_LOG_SIZE: '0'}
        logger.info('call %s', ' '.join(args))
        p0 = Popen(args, stdout=PIPE, stderr=STDOUT, env=env)
        p1 = Popen(["logger", "-t", "PaxosMemcache#{0}".format(m.id)],
                   stdin=p0.stdout)
        p0.stdout.close()
        return p0

    def _add_pair(self, m):
        for pair in m.pair:
            pair_args = [
                config.CMD_PAXOS_MEMCACHE_ADM, str(m.id), 'add',
                pair.client_addr(), pair.server_addr()]
            logger.info("call %s", ' '.join(pair_args))
            try:
                util.check_call_timeout(pair_args)
            except CalledProcessError:
                logger.warning(
                    _("not be able to add a pair (id={0})").format(m.id))
            except error.TimeoutError:
                logger.error(_("timeout: {0}").format(' '.join(pair_args)))

    def start_memcache(self, m):
        for r in range(self.retry):
            p = self._start_memcache(m)
            sleep(1)
            if p.poll() is None:
                break
            logger.warning(_("failed to start PAXOS memcache"))

        self._add_pair(m)


class StopCommand(model.Command, model.BaseStopCommand):
    def __init__(self, id=None, force=False, *args, **kwargs):
        super(StopCommand, self).__init__(*args, **kwargs)
        if id is None:
            self.tgt = Memcache.find({})
        else:
            self.tgt = [Memcache.get(id)]
        self.force = force

    def run(self):
        for memcache in self.tgt:
            self.stop_memcache(memcache)

    def stop_memcache(self, m):
        args = [config.CMD_PAXOS_MEMCACHE_ADM, str(m.id), 'stop']
        logger.info("call %s", ' '.join(args))
        try:
            util.check_call_timeout(args)
        except CalledProcessError as e:
            logger.info("failed to stop PAXOS memcache", exc_info=True)
            if _running(m):
                if not self.force:
                    logger.warning(_("failed to stop PAXOS memcache"))
                else:
                    self.force_stop('PaxosMemcache',
                                    lambda w: str(m.id) == w[2])
        except error.TimeoutError:
            if not self.force:
                logger.error(_("timeout: {0}").format(' '.join(args)))
            else:
                self.force_stop('PaxosMemcache', lambda w: str(m.id) == w[2])


class StateCommand(model.Command):
    def __init__(self, id, css, short=None, quiet=None, *args, **kwargs):
        super(StateCommand, self).__init__(*args, **kwargs)
        self.search = {}
        if id is not None:
            self.search['id'] = id
        if css is not None:
            self.search['css'] = css
        self.tgt = Memcache.find(self.search)
        if quiet is not None and quiet:
            self.mode = 'quiet'
        elif short is not None and short:
            self.mode = 'short'
        else:
            self.mode = 'normal'
        self.pair_re = re.compile(
            '\d+\.\s+Client\[([^:]+):(\d+)\]\s+-\s+Server\[([^:]+):(\d+)\]')

    def get_result(self):
        result = {}
        for m in self.tgt:
            result[m.id] = {'model': m, 'status': config.PROC_STATE_STOPPED}
        for proc in self.state_memcache():
            m = self.parse_ps(proc)
            if m is None:
                continue
            if self.mode == 'normal':
                self.state_pair(m)
            if m.id in result:
                result[m.id]['status'] = config.PROC_STATE_RUNNING
                result[m.id]['state_obj'] = m
        return result

    def run(self):
        result = self.get_result()
        ret = True
        if self.mode == 'normal':
            self.normal_print(result)
        elif self.mode == 'short':
            self.short_print(result)
        ret = True if len(result) > 0 else False
        for s in result.values():
            if s['status'] != config.PROC_STATE_RUNNING:
                ret = False
        return 0 if ret else 1

    def parse_ps(self, ps):
        logger.debug("memcache state=%s", ps)
        w = ps.split()
        name_space = w.pop()
        if name_space == '<defunct>':
            return None
        css = w.pop()
        id = int(w.pop())
        cmd = w.pop(0)
        m = Memcache(id=id, css=css, name_space=name_space)
        for tup in itertools.izip_longest(*[iter(w)]*2):
            opt = tup[0]
            val = int(tup[1])
            if opt == '-I':
                m.max_cache_item = val
            elif opt == '-s':
                m.checksum = True if val == 1 else False
            elif opt == '-C':
                m.checksum_interval = val
            elif opt == '-E':
                m.expire_interval = val
            elif opt == '-W':
                m.worker = val
        return m

    def normal_print(self, result):
        for id, s in result.iteritems():
            print '---'
            print "status: {state}".format(state=s['status'])
            if 'state_obj' in s:
                print str(s['state_obj'])
            else:
                print 'id: {id}'.format(id=id)

    def short_print(self, result):
        for id, s in result.iteritems():
            print "{id}: {state}".format(id=id, state=s['status'])

    def state_pair(self, m):
        args = [config.CMD_PAXOS_MEMCACHE_ADM, str(m.id), 'pair']
        logger.info("call %s", ' '.join(args))
        try:
            out = util.check_output_timeout(args)
        except CalledProcessError:
            logger.warning(
                _("state of 'pair' can not be obtained (id={0})").format(m.id))
            return
        except error.TimeoutError:
            logger.error(_("timeout: {0}").format(' '.join(args)))
            out = '\n'
        lines = out.splitlines()
        lines.pop(0)
        for line in lines:
            mat = self.pair_re.match(line)
            pair = MemcachePair(chost=mat.group(1), cport=mat.group(2),
                                shost=mat.group(3), sport=mat.group(4))
            m.add_pair(pair)

    def state_memcache(self):
        args = ['ps', '-C', 'PaxosMemcache', '-o', 'command', '--no-headers']
        logger.info("call %s", ' '.join(args))
        try:
            out = util.check_output_timeout(args)
            return out.splitlines()
        except CalledProcessError:
            return []
        except error.TimeoutError:
            logger.error(_("timeout: {0}").format(' '.join(args)))
            return []
