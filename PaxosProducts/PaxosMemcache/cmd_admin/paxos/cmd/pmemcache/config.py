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
import os.path

CONFIG_DIR = '~/etc'
CONFIG_CELL_DIR = os.path.join(CONFIG_DIR, 'cell')
CONFIG_CELL_PATTERN = os.path.join(CONFIG_CELL_DIR, '*.conf')
CONFIG_CELL_FORMAT = os.path.join(CONFIG_CELL_DIR, '{cell}.conf')
ENV_CONFIG_CELL = 'PAXOS_CELL_CONFIG'
ENV_LOG_SIZE = 'LOG_SIZE'
ENV_LOG_LEVEL = 'LOG_LEVEL'
ENV_PAXOS_LOCAL = '_PAXOS_LOCAL'

DATA_DIR0 = '~/data'
DATA_DIR = os.path.expanduser(DATA_DIR0)
DATA_CELL_FORMAT = os.path.join(DATA_DIR, '{cell}/{no}')

DATA_CELL_FORMAT_DIRS = [
    os.path.join(DATA_DIR, '{cell}/{no}/DATA'),
    os.path.join(DATA_DIR, '{cell}/{no}/PFS')]

MEMCACHE_CONF_FILE = 'paxos_memcache.conf'
MEMCACHE_CONF_PATH = os.path.join(CONFIG_DIR, MEMCACHE_CONF_FILE)

BIN_DIR0 = '~/bin'
BIN_DIR = os.path.expanduser(BIN_DIR0)
CMD_PAXOS_MEMCACHE = os.path.join(BIN_DIR, 'PaxosMemcache')
CMD_PAXOS_MEMCACHE_ADM = os.path.join(BIN_DIR, 'PaxosMemcacheAdm')
CMD_PAXOS_CSS = os.path.join(BIN_DIR, 'PFSServer')
CMD_PAXOS_SHUTDOWN = os.path.join(BIN_DIR, 'PaxosSessionShutdown')
CMD_PAXOS_ADMIN = os.path.join(BIN_DIR, 'PaxosAdmin')
CMD_PAXOS_PFSPROBE = os.path.join(BIN_DIR, 'PFSProbe')
CMD_PAXOS_CHANGE_MEMBER = os.path.join(BIN_DIR, 'PaxosSessionChangeMember')
CMD_PAXOS = os.path.join(BIN_DIR0, 'paxos')

PROC_STATE_RUNNING = 'running'
PROC_STATE_STOPPED = 'stopped'
PROC_STATE_ERROR = 'error'
CMD_TIMEOUT = 20
