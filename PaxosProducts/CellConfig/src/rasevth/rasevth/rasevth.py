###########################################################################
#   Rasevth Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of Rasevth.
#
#   Rasevth is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Rasevth is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Rasevth.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from __future__ import absolute_import
import logging
import os.path
import json
import requests
import random
from datetime import datetime
import pycss


logger = logging.getLogger(__name__)


class RasScript(object):

    @classmethod
    def find(cls, tp):
        if tp == 'smtp':
            from .smtpact import SmtpScript
            return SmtpScript
        elif tp == 'syslog':
            from .syslogact import SyslogScript
            return SyslogScript
        elif tp == 'http':
            from .httpact import HttpScript
            return HttpScript
        elif tp == 'restart':
            from .restartact import RestartScript
            return RestartScript

    def __init__(self, params):
        self.name = params['action']
        self.params = params
        logger.info("SCRIPT={} PARAMS={}".format(self.name, self.params))

    def execute(self, status):
        pass

    def process(self, status):
        if 'mode' in status and 'mode' in self.params:
            if (status['mode'] & self.params['mode']) == 0:
                logger.info(
                    "skip: mode={} accept={}".format(status['mode'],
                                                     self.params['mode']))
                return
        self.execute(status)


class RasCell(object):
    def __init__(self, name, group, path):
        self.name = name
        self.group = group
        self.path = path
        self.conn = pycss.connect(self.name)
        self.baseurl = self._baseurl()
        self._init_scripts()

    def _baseurl(self):
        with self.conn.open('/cellcfg/list') as f:
            line = f.read()
        lines = line.strip().split('\n')
        return lines[random.randrange(len(lines))]

    def _init_scripts(self):
        url = "{}event/RAS/{}/{}".format(
            self.baseurl, self.group, os.path.relpath(self.path, "RAS"))
        resp = requests.get(url)
        self.scripts = []
        logger.debug("RESP: URL={} type={} val={}".format(
            url, type(resp), resp.json()))
        for script in resp.json():
            self.scripts.append(RasScript.find(script['action'])(script))

    def entry_data(self, entry):
        with self.conn.open(os.path.join('/', self.path, entry)) as f:
            return f.read()

    def __repr__(self):
        return "RasCell('{}', '{}', '{}')".format(
            self.name, self.group, self.path)


class RasEvent(object):

    START = 0x1
    STOP = 0x2
    UPDATE = 0x4
    MODE_MAP = {
        0: (START, 'started'), 1: (STOP, 'stopped'), 2: (UPDATE, 'updated')}

    def __init__(self, ras_cell, group, path, **kwargs):
        self.rcell = RasCell(ras_cell, group, path)
        self.status = kwargs
        self.status['ras_cell'] = ras_cell
        self.status['ras_cell_obj'] = self.rcell
        self.status['group'] = group
        self.status['path'] = path
        self.status['source'] = self.status['source'][0]
        self.status['ctime'] = datetime.fromtimestamp(
            self.status['time']).isoformat()
        mode = self.MODE_MAP[self.status['mode']]
        self.status['mode'], self.status['mode_label'] = mode
        logger.info("STATUS: {}".format(str(self.status)))

    def process(self):
        from .reportact import ReportScript
        scripts = [ReportScript('report', None)]
        scripts.extend(self.rcell.scripts)
        for script in scripts:
            try:
                logger.info("execute: script={}".format(script.name))
                script.process(self.status)
            except:
                logger.exception("execute failure({})".format(script.name))
