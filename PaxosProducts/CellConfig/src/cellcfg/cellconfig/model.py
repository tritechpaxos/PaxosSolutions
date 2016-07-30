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
from functools import partial
from socket import getfqdn
from tempfile import mkstemp
from peewee import *
import gevent

from cellconfig import db as _db
from cellconfig import use_dns
from .agent import call


logger = logging.getLogger(__name__)


class BaseModel(Model):
    class Meta:
        database = _db


class Node(BaseModel):
    id = PrimaryKeyField(sequence='node_seq' if _db.sequences else None)
    hostname = CharField(unique=True, constraints=[Check('hostname <> ""')])
    user = CharField(constraints=[Check('user <> ""')])
    password = CharField()
    port = IntegerField(constraints=[Check('port >= 0 AND port <= 65535')])
    paxos_home = CharField(constraints=[Check('paxos_home <> ""')])
    os = CharField(null=True)
    distribution = CharField(null=True)
    machine = CharField(null=True)

    @property
    def name(self):
        return self.hostname

    def __init__(self, *args, **kwargs):
        super(Node, self).__init__(args, kwargs)

    def normalize(self):
        if self.hostname is not None:
            self.hostname = self.hostname.strip()
        if self.user is not None:
            self.user = self.user.strip()
        if self.paxos_home is not None:
            self.paxos_home = self.paxos_home.strip()

    def save(self, *args, **kwargs):
        self.normalize()
        super(Node, self).save(*args, **kwargs)

    def __str__(self):
        hname = getfqdn(self.hostname) if use_dns() else self.hostname
        params = dict(
            hostname=hname,
            user=self.user,
            port='')
        params['port'] = ':{0}'.format(self.port)
        return '{user}@{hostname}{port}'.format(**params)

    def os_str(self):
        if self.os is not None:
            if self.distribution is not None:
                return '{os} {dist}'.format(os=self.os,
                                            dist=self.distribution)
            else:
                return '{os}'.format(os=self.os)
        else:
            return '---'

    def machine_str(self):
        return self.machine if self.machine is not None else '---'

    def check_ssh(self):
        ret = self._check_ssh()
        for k, v in ret.iteritems():
            setattr(self, k, v)

    @call()
    def _check_ssh(self):
        pass

    @call(name='deploy', async=True)
    def deploy_async(self, force=False):
        self.check_and_save()

    def check_and_save(self):
        self.check_ssh()
        self.save()


class Cell(BaseModel):
    CELL_TYPE = ('CSS', 'CMDB', 'LV')
    id = PrimaryKeyField(sequence='cell_seq' if _db.sequences else None)
    name = CharField(unique=True, constraints=[Check('name <> ""')])
    type = CharField(
        constraints=[Check('type IN ("CMDB", "CSS", "LV", "RAS")')])
    comment = CharField(null=True)
    core = IntegerField(default=3, constraints=[Check('core >= 3')])
    ext = IntegerField(default=1, constraints=[Check('ext >= 0')])

    def __init__(self, *args, **kwargs):
        super(Cell, self).__init__(args, kwargs)

    def get_appname(self):
        if self.type == 'CMDB':
            return 'xjPingPaxos'
        elif self.type == 'CSS':
            return 'PFSServer'
        elif self.type == 'LV':
            return 'LVServer'
        elif self.type == 'RAS':
            return 'PFSServer'
        else:
            return ''

    def normalize(self):
        if self.name is not None:
            self.name = self.name.strip()
        if self.type is not None:
            self.type = self.type.strip()

    def save(self, *args, **kwargs):
        self.normalize()
        super(Cell, self).save(*args, **kwargs)

    def generate_conf(self, path):
        with open(path, 'w') as f:
            self._generate_conf(f)

    def _generate_conf(self, io):
        for node in self.nodes:
            io.write('{no} {uhost} {uport} {thost} {tport}\n'
                     .format(no=node.no,
                             uhost=node.udp_addr, uport=node.udp_port,
                             thost=node.tcp_addr, tport=node.tcp_port))

    def deploy_conf(self):
        tmp = mkstemp(text=True)[1]
        self.generate_conf(tmp)
        try:
            self._deploy_conf(tmp)
        finally:
            unlink(tmp)

    @call(name='deploy_conf')
    def _deploy_conf(self, path):
        pass

    @call(name='get_cell_status')
    def get_status(self):
        pass

    @call(name='get_status')
    def get_cnode_status(self, nodes):
        pass

    def get_node_status(self):
        return {cnode.no:
                dict(id=cnode.id, no=cnode.no, hostname=cnode.node.hostname)
                for cnode in self.nodes}

    @call()
    def get_shutdown_no(self):
        pass

    def start(self, params=None):
        st = self.get_status()
        logger.debug("STATUS: {}".format(st))
        ret = None
        if st['running'] == 0:
            next_no = self.get_shutdown_no()
            logger.debug("SHUTDOWN: {}".format(next_no))
            for e in sorted(next_no.items(), key=lambda x: x[1], reverse=True):
                node = self.nodes[e[0]]
                logger.debug("START: {}".format(e[0]))
                ret = self.start_node_async(node, params)
        else:
            for node in self.nodes:
                if st['detail'][node.no] == 'stopped':
                    ret = self.start_node_async(node, params)
        logger.debug("RET: {}".format(ret))
        return ret

    @call(name='start', async=True)
    def start_node_async(self, node, params=None):
        pass

    def stop(self, params=None):
        st = self.get_cnode_status(self.nodes)
        logger.debug("STOP STATUS: {}".format(st))
        for k, v in st.items():
            if v['ismaster'] or ('pid' not in v) or (v['pid'] is None):
                continue
            self.stop_node_async(self.nodes[k], params)
        for k, v in st.items():
            if (not v['ismaster']) or ('pid' not in v) or (v['pid'] is None):
                continue
            self.stop_node_async(self.nodes[k], params)

    @call(name='stop', async=True)
    def stop_node_async(self, node, params):
        pass

    @call(name='get_misc_info_all')
    def get_misc_info(self, op):
        pass

    @call(name='get_misc_info', async=True)
    def get_misc_info_node_async(self, op, node):
        pass

    def find_by_server_no(self, server_no):
        for node in self.nodes:
            if node.no == server_no:
                return node
        return None


class CellNode(BaseModel):

    id = PrimaryKeyField(sequence='cellnode_seq' if _db.sequences else None)
    node = ForeignKeyField(Node, related_name='cells')
    cell = ForeignKeyField(Cell, related_name='nodes')
    udp_addr = CharField(constraints=[Check('udp_addr <> ""')])
    udp_port = IntegerField(
        constraints=[Check('udp_port >= 0 AND udp_port <= 65535')])
    tcp_addr = CharField(constraints=[Check('tcp_addr <> ""')])
    tcp_port = IntegerField(
        constraints=[Check('tcp_port >= 0 AND tcp_port <= 65535')])
    no = IntegerField(constraints=[Check('no >= 0')])
    op_level = IntegerField(default=0)

    class Meta:
        indexes = (
            (('udp_addr', 'udp_port'), True),
            (('tcp_addr', 'tcp_port'), True),
        )

    def normalize(self):
        if self.udp_addr is not None:
            self.udp_addr = self.udp_addr.strip()
        if self.tcp_addr is not None:
            self.tcp_addr = self.tcp_addr.strip()

    def save(self, *args, **kwargs):
        self.normalize()
        super(CellNode, self).save(*args, **kwargs)

    def paxos_home(self):
        return self.node.paxos_home

    def _check_level(self, params):
        level = params['level'] if 'level' in params else 0
        if level >= self.op_level:
            self.op_level = 0
            self.save()
            return False
        else:
            return True

    def start(self, params):
        logger.debug("START CNODE: {}".format(params))
        if self._check_level(params):
            logger.debug("NOOP")
            return

        def do_retry(x, retry):
            gevent.sleep(params['interval'] if 'interval' in params else 60)
            g = self.cell.start_node_async(self, params)
            if retry > 0:
                retry -= 1
                g.link_exception(partial(do_retry, retry=retry))
        retry = params['retry'] if 'retry' in params else 0
        g = self.cell.start_node_async(self, params)
        g.link_exception(partial(do_retry, retry=retry))

    def _update_level(self, params):
        if 'level' in params:
            level = params['level']
            if level > self.op_level:
                self.op_level = level
                self.save()

    def stop(self, params):
        g = self.cell.stop_node_async(self, params)
        g.link_value(lambda x: self._update_level(params))

    def get_misc_info_async(self, op):
        return self.cell.get_misc_info_node_async(op, self)

    def get_current_status(self):
        ret = self.cell.get_cnode_status([self])
        logger.debug("CURRENT STATUS: {}".format(ret))
        if ret is None:
            return None
        elif self.no in ret and 'pid' in ret[self.no] and ret[self.no]['pid']:
            return ret[self.no]
        else:
            return None
