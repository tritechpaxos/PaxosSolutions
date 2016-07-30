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
import sys
import logging
import blinker
import ConfigParser
from fabric.tasks import execute
from fabric.state import env
from fabric.exceptions import NetworkError
import gevent
import gevent.pool
import gevent.queue
import gevent.event

from cellconfig import conf
from cellconfig.error import DeployError
from . import fabfile
from .. import _event_name2task


logger = logging.getLogger(__name__)
fabqueue = gevent.queue.Queue()


def fab_executor():
    while True:
        g = fabqueue.get()
        g.start()
        g.join()


gevent.spawn(fab_executor)


class _SshAgent(object):
    def __init__(self, target):
        self.target = target

    @staticmethod
    def _execute(task, nodes, warn_only=True, **kwargs):
        logger.debug("TASK={}, nodes={}".format(
            task, [str(n.node) for n in nodes]))
        ret = {}
        hosts = []
        home = {}
        server_no = {}
        for node in nodes:
            addr = str(node.node)
            hosts.append(addr)
            env.passwords[addr] = node.node.password
            home[addr] = node.node.paxos_home
            if addr in server_no:
                server_no[addr].append(node.no)
            else:
                server_no[addr] = [node.no]
        env.hosts = list(set(hosts))
        logger.debug("WARN ONLY: {}".format(warn_only))
        env.warn_only = warn_only
        env.skip_bad_hosts = warn_only
        try:
            result = execute(task, home=home, server_no=server_no, **kwargs)
            ret = dict([(k, v) for k, v in result.iteritems()
                        if not isinstance(v, NetworkError)])
        finally:
            env.warn_only = False
            env.skip_bad_hosts = False
        return ret

    def _execute_fab(self, task, nodes, **kwargs):
        g = gevent.Greenlet(_SshAgent._execute, task, nodes, **kwargs)
        fabqueue.put(g)
        return g.get()


class _SshCellAgent(_SshAgent):

    def _execute_fab(self, task, nodes, **kwargs):
        return super(_SshCellAgent, self)._execute_fab(
            task, nodes, cell=self.target, **kwargs)

    def execute_fab(self, task, **kwargs):
        return self._execute_fab(task, self.target.nodes, **kwargs)

    def get_pids(self, nodes):
        ret = dict()
        for cnode in nodes:
            ret[cnode.no] = dict(id=cnode.id, no=cnode.no,
                                 hostname=cnode.node.hostname)
        try:
            result = self._execute_fab(
                fabfile.get_paxos_service_pid, nodes=nodes)
            for addr, val in result.iteritems():
                for n, pid in val.iteritems():
                    ret[n]['pid'] = pid
        except fabfile.FabError as e:
            logger.error(e.message)
        return ret

    def get_status(self, nodes):
        ret = self.get_pids(nodes)
        try:
            result = self._execute_fab(fabfile.paxos_admin, nodes=nodes)
            for addr, val in result.iteritems():
                for n, v in val.iteritems():
                    ret[n].update(v)
        except fabfile.FabError as e:
            logger.error(e.message)
        return ret

    def get_cell_status(self):
        pids = reduce(lambda x, y: (x.update(self.get_pids([y])), x)[1],
                      self.target.nodes, {})
        logger.debug("PID: {}".format(pids))
        total = len(self.target.nodes)
        running = len([x for x in pids
                      if 'pid' in pids[x] and pids[x]['pid'] is not None])
        stopped = total - running
        detail = dict([(int(v['no']), (
            'running' if 'pid' in v and v['pid'] is not None else 'stopped'))
            for v in pids.values()])
        logger.debug("DETAIL: {}".format(detail))
        status = 'running' if running == total else (
            'stopped' if running == 0 else (
                'warning' if running >= (total + 1) // 2 else 'error'))
        ret = {'id': self.target.id, 'name': self.target.name,
               'status': status, 'running': running, 'stopped': stopped,
               'detail': detail}
        return ret


class CellOperation(_SshCellAgent):
    def deploy_conf(self, path):
        ex = []
        for node in self.target.nodes:
            try:
                self._execute_fab(
                    fabfile.deploy_conf, nodes=[node], conf_path=path,
                    warn_only=False)
            except fabfile.FabError as e:
                logger.exception(e)
                ex.append(str(node.node))
        if len(ex) > 0:
            raise fabfile.FabError(
                'Failed to deploy the configuration file.',
                ','.join(ex))

    def get_shutdown_no(self):
        ret = self.execute_fab(fabfile.get_shutdown_no)
        next_no = dict()
        for v in ret.values():
            next_no.update(v)
        return next_no

    def start(self, node, params=None):
        logger.debug("START CELL {}:no={}; {}".format(
            self.target.name, node.no, params))
        self._execute_fab(
            fabfile.deploy, node=node.node, nodes=[node], warn_only=False)
        self._execute_fab(fabfile.cell_setup, nodes=[node])
        self._execute_fab(fabfile.paxos_start, nodes=[node], warn_only=False)
        blinker.signal('refresh:cell').send(self.target.name)

    def stop(self, node, params=None):
        logger.debug("STOP CELL {}:no={}; {}".format(
            self.target.name, node.no, params))
        self._execute_fab(fabfile.paxos_stop, nodes=[node], warn_only=False)
        blinker.signal('refresh:cell').send(self.target.name)

    def get_misc_info(self, op, node):
        ret = self._execute_fab(fabfile.paxos_probe, [node], op=op,
                                warn_only=True)
        return ret.values()[0].data

    def get_misc_info_all(self, op):
        result = self.execute_fab(fabfile.paxos_probe, op=op)
        return reduce(lambda x, y: y.append(x), result.values())


class CellEvent(_SshCellAgent):

    EVENTS = ['status', 'status_detail']
    PRIORITY = 0

    def __init__(self, target, signal):
        super(CellEvent, self).__init__(target)
        self.signal = signal
        self.worker = {}
        for e in CellEvent.EVENTS:
            self.worker[e] = None
        self.event = gevent.event.Event()
        blinker.signal('refresh:cell').connect(
            self._set_event, sender=self.target.name)

    def _set_event(self, sender):
        logger.debug("EVENT SET: {}".format(sender))
        self.event.set()

    def _polling_get_status(self, pool):
        sender = self.target.name
        logger.debug("POLLING: sender={} signal={}".format(
            sender, self.signal))
        if self.signal.has_receivers_for(sender):
            logger.debug("POLLING: target={}".format(self.target.nodes))
            for cnode in self.target.nodes:
                pool.apply_async(
                    self.get_status, [[cnode]],
                    callback=lambda x: self.signal.send(sender, message=x))
        else:
            logger.debug("no POLLING: sender={} receiver={}".format(
                sender, self.signal.receivers))

    def _polling_get_cell_status(self, pool):
        sender = self.target.name
        if self.signal.has_receivers_for(sender):
            pool.apply_async(
                self.get_cell_status,
                callback=lambda x: self.signal.send(sender, message=x))

    def _polling(self, func):
        logger.debug("POLLING start")
        interval = self.get_interval()
        pool = gevent.pool.Pool(len(self.target.nodes))
        while True:
            logger.debug("EXEC: interval={}".format(interval))
            func(pool)
            self.event.wait(interval)
            self.event.clear()

    def polling(self, name):
        if name == 'status_detail':
            return lambda: self._polling(self._polling_get_status)
        elif name == 'status':
            return lambda: self._polling(self._polling_get_cell_status)
        else:
            return lambda: None

    def get_interval(self):
        try:
            return conf.getint(self.__module__, 'POLLING_INTERVAL')
        except (ConfigParser.NoOptionError, ValueError):
            return 60

    def enable(self, event):
        name = event['name']
        logger.debug("POLLING spawn: name={}".format(name))
        self.worker[name] = gevent.spawn(self.polling(_event_name2task(name)))

    def disable(self, event):
        name = event['name']
        logger.debug("KILL TASK: {}".format(name))
        gevent.kill(self.worker[name])
        self.worker[name] = None

    @classmethod
    def match(cls, event):
        return _event_name2task(event['name']) in cls.EVENTS


class NodeOperation(_SshAgent):

    def _execute(self, task, **kwargs):
        ssh_addr = str(self.target)
        env.hosts = [ssh_addr]
        env.password = self.target.password
        ret = execute(task, **kwargs)
        return ret.values()[0]

    def execute_fab(self, task, **kwargs):
        g = gevent.Greenlet(self._execute, task, **kwargs)
        fabqueue.put(g)
        return g.get()

    def _check_ssh(self):
        ret = {}
        try:
            ret['os'] = self.execute_fab(fabfile.uname)
        except fabfile.FabError as e:
            return ret
        try:
            ret['distribution'] = self.execute_fab(fabfile.distribution)
        except fabfile.FabError as e:
            pass
        try:
            ret['machine'] = self.execute_fab(fabfile.uname, opt='-p')
        except fabfile.FabError as e:
            pass
        return ret

    def deploy(self, force=False):
        try:
            self.execute_fab(fabfile.deploy,
                             node=self.target,
                             home=self.target.paxos_home,
                             force=force)
        except fabfile.FabError as e:
            logger.exception(e)
            raise DeployError(self.target)
