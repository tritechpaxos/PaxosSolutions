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


class ReportScript(RasScript):

    def __init__(self, name, text):
        super(ReportScript, self).__init__({'action': name})

    def execute(self, status):
        url = self.build_url(status)
        logger.debug("URL: {}".format(url))
        params = self.build_http_parameter(status)
        logger.debug("PARAMS: {}".format(params))
        headers = {'Content-Type': 'application/json; charset=utf-8'}
        resp = requests.put(url, data=params, headers=headers)

    def build_url(self, status):
        base = status['ras_cell_obj'].baseurl
        return base + 'event/' + status['path'] + '/' + status['entry']

    def build_http_parameter(self, status):
        return json.dumps({'mode': status['mode_label']})
