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
import syslog
import socket
import string
from logging.handlers import SysLogHandler, SYSLOG_UDP_PORT
from .rasevth import RasScript


class SyslogScript(RasScript):
    def execute(self, status):
        priority = self.build_priority()
        message = self.build_message(status)

        if self.params['address'].strip() in ['localhost', '127.0.0.1', '']:
            self.local_message(priority, message)
        else:
            address = self.build_address()
            self.remote_message(priority, message, address)

    def build_priority(self):
        f = self.params['facility']
        p = self.params['priority']
        return (getattr(syslog, 'LOG_' + f.upper()) |
                getattr(syslog, 'LOG_' + p.upper()))

    def build_message(self, status):
        fmt = self.params['message']
        if string.find(fmt, '{default}') >= 0:
            status['default'] = (
                '{source}[{path}:{entry}]({sequence}) is {mode_label}.'
                .format(**status))
        return fmt.format(**status)

    def build_address(self):
        if self.params['protocol'] in ['udp', 'tcp']:
            addr = self.params['address'].strip().split(':', 1)
            return (addr[0],
                    SYSLOG_UDP_PORT if len(addr) == 1 else int(addr[1]))
        else:
            return self.params['address'].strip()

    def local_message(self, priority, message):
        syslog.openlog(ident=self.params['name'].encode())
        syslog.syslog(priority, message)

    def remote_message(self, priority, message, address):
        socktype = (
            socket.SOCK_STREAM if self.params['protocol'] == 'tcp' else None)
        RemoteSyslog(address=address, priority=priority,
                     socktype=socktype).emit(message)


class RemoteSyslog(SysLogHandler):

    class Record(object):
        def __init__(self, message):
            self.message = message
            self.levelname = None

    def __init__(self, **kwargs):
        params = kwargs
        self.priority = kwargs['priority']
        del(params['priority'])
        super(RemoteSyslog, self).__init__(**params)

    def format(self, record):
        return record.message

    def encodePriority(self, facility, priority):
        return self.priority

    def mapPriority(self, x):
        pass

    def emit(self, record):
        super(RemoteSyslog, self).emit(RemoteSyslog.Record(record))
