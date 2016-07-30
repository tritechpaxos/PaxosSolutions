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
import json
from peewee import PrimaryKeyField, CharField
from cellconfig import db as _db
from .model import BaseModel


class AdminConf(BaseModel):
    id = PrimaryKeyField(sequence='admin_seq' if _db.sequences else None)
    name = CharField(unique=True)
    data = CharField(null=True)


class SyslogConf(object):

    FACILITY = ['auth', 'authpriv', 'cron', 'daemon', 'kern', 'lpr', 'mail',
                'news', 'security', 'ftp', 'ntp', 'syslog', 'user', 'uucp',
                'local0', 'local1', 'local2', 'local3', 'local4', 'local5',
                'local6', 'local7']

    PRIORITY = ['debug', 'info', 'notice', 'warning', 'err', 'crit', 'alert',
                'emerg']

    NAME = 'log'

    DEFAULT = {
        'address': 'localhost',
        'facility': 'user',
        'priority': 'info',
        'level': 2,
    }

    def __init__(self):
        ent = AdminConf.select().where(AdminConf.name == self.NAME).first()
        self.value = json.loads(ent.data) if ent is not None else self.DEFAULT

    def server(self):
        srv = self.value['address'].strip()
        return None if srv == 'localhost' or srv == '127.0.0.1' else srv

    def priority(self):
        return '{facility}.{priority}'.format(
            facility=self.value['facility'], priority=self.value['priority'])

    def level(self):
        return self.value['level']
