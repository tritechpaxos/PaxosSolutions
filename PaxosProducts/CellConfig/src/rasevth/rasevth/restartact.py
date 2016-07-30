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
from .rasevth import RasScript
import logging
import requests
import json


logger = logging.getLogger(__name__)


class RestartScript(RasScript):

    URL_FORMAT = {
        'cell': '{}api/cell/name/{}/node/{}/process',
        'ras': '{}api/ras/name/{}/server/{}/process',
    }

    def execute(self, status):
        url = self.restart_url(status, self.params['type'])
        self.restart(url)

    def restart(self, url):
        logger.debug("URL: {}".format(url))
        params = {
            'operation': 'start',
            'retry': self.params['retry'],
            'interval': self.params['interval'],
        }
        headers = {'Content-Type': 'application/json; charset=utf-8'}
        resp = requests.put(url, data=json.dumps(params), headers=headers)

    def restart_url(self, status, tp):
        base = status['ras_cell_obj'].baseurl
        return self.URL_FORMAT[tp].format(
            base, status['source'], status['entry'])
