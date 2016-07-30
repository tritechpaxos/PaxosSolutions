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
import pycss
from .. import _event_name2task


logger = logging.getLogger(__name__)


class RasEvent(object):

    EVENTS = ['status', 'server_status']
    PRIORITY = 1

    def __init__(self, target, signal):
        self.target = target
        self.signal = signal

    def enable(self, event):
        from cellconfig.ras.model import RasTarget
        ras = event['target']
        task = _event_name2task(event['name'])
        if task == 'status':
            for tgt in ras.targets:
                self._send_signal(self.signal, tgt)
        elif task == 'server_status':
            tgt = RasTarget.select().where(
                (RasTarget.type == 'ras') &
                (RasTarget.name == ras.name)).first()
            if tgt is not None:
                self._send_server_signal(self.signal, tgt, ras)

    @classmethod
    def _send_signal(cls, sig, target):
        sender = target.group.name
        if not sig.has_receivers_for(sender):
            return
        msg = target.status()
        msg['etype'] = 'target'
        sig.send(sender, message=msg)

    @classmethod
    def _send_server_signal(cls, sig, target, ras):
        if (target.type != 'ras') or (ras is None):
            return
        status = target.status()['detail']
        sender = ras.name
        if not sig.has_receivers_for(sender):
            return
        for srv in ras.servers:
            msg = {
                'etype': 'server', 'no': srv.no, 'status': status[str(srv.no)],
                'hostname': srv.node.hostname, 'sid': srv.id, 'cid': ras.id}
            sig.send(sender, message=msg)

    def disable(self, event):
        pass

    @classmethod
    def match(cls, event):
        return _event_name2task(event['name']) in cls.EVENTS


class CellEvent(object):

    EVENTS = ['status']
    PRIORITY = 1

    def __init__(self, target, signal):
        self.target = target
        self.signal = signal

    def enable(self, event):
        from cellconfig.ras.model import RasTarget
        cell = event['target']
        tgt = RasTarget.select().where(
            (RasTarget.type == 'cell') & (RasTarget.name == cell.name)).first()
        self._send_signal(self.signal, cell.name, tgt)

    @classmethod
    def _send_signal(cls, sig, sender, target):
        if not (target and sig.has_receivers_for(sender)):
            return
        msg = target.status()
        msg['id'] = msg['cid']
        sig.send(sender, message=msg)

    def disable(self, event):
        pass

    @classmethod
    def match(cls, event):
        from cellconfig.ras.model import RasTarget
        if _event_name2task(event['name']) not in cls.EVENTS:
            return False
        cell = event['target']
        count = RasTarget.select().where(
            (RasTarget.type == 'cell') & (RasTarget.name == cell.name)).count()
        return count > 0


class RasTargetOperation(object):

    def __init__(self, target):
        self.target = target

    def status(self):
        if self.target.type == 'cell':
            return self._cell_status()
        elif self.target.type == 'ras':
            return self._ras_status()
        elif self.target.type == 'app':
            return self._app_status()

    def _get_status(self, ras_cell, path, entries):
        running = 0
        stopped = 0
        total = len(entries)
        css = pycss.connect(ras_cell.encode())
        css.chdir(path)
        detail = {}
        for node in entries:
            if css._exists(str(node.no)):
                running += 1
                detail[str(node.no)] = 'running'
            else:
                stopped += 1
                detail[str(node.no)] = 'stopped'
        return {'running': running, 'stopped': stopped, 'total': total,
                'detail': detail, 'id': self.target.id, }

    def _cell_status(self):
        cell_name = self.target.name
        from cellconfig.model import Cell
        cell = Cell.select().where(Cell.name == cell_name).first()
        try:
            st = self._get_status(
                self.target.group.cell.name,
                '/RAS/{}/{}'.format(cell.get_appname(), cell.name),
                cell.nodes)
            if st['running'] == st['total']:
                status = 'running'
            elif st['running'] == 0:
                status = 'stopped'
            elif st['running'] >= (st['total'] + 1) // 2:
                status = 'warning'
            else:
                status = 'error'
            st.update({'cid': cell.id, 'name': cell.name, 'status': status})
            return st
        except Exception as e:
            logger.exception(e)
            return {'cid': cell.id, 'name': cell.name, 'status': 'unknown',
                    'total': len(cell.nodes)}

    def _ras_status(self):
        group_name = self.target.name
        from cellconfig.ras.model import RasServer, RasGroup
        group = RasGroup.select().where(RasGroup.name == group_name).first()
        srvs = RasServer.select().where(RasServer.group == group.id)
        try:
            st = self._get_status(
                group.cell.name,
                '/RAS/RasEye/{}'.format(group.name),
                srvs)
            if st['running'] == st['total']:
                status = 'running'
            elif st['running'] == 0:
                status = 'stopped'
            else:
                status = 'warning'
            st.update({'cid': group.id, 'name': group.name, 'status': status})
            return st
        except Exception as e:
            logger.exception(e)
            return {'cid': group.id, 'name': group.name, 'status': 'unknown',
                    'total': len(srvs)}

    def _app_status(self):
        logger.debug("TARGET NAME: {}".format(self.target.name))
        running = 0
        stopped = 0
        total = 1
        try:
            css = pycss.connect(self.target.group.cell.name.encode())
            path = '/RAS/{}'.format(self.target.path)
            logger.debug("PATH: {}".format(path))
            if css._exists(path):
                running = 1
                status = 'running'
            else:
                stopped = 1
                status = 'stopped'
            return {'running': running, 'stopped': stopped, 'total': total,
                    'status': status, 'name': self.target.name}
        except Exception as e:
            logger.exception(e)
            return {'total': 1, 'status': 'unknown', 'name': self.target.name}
