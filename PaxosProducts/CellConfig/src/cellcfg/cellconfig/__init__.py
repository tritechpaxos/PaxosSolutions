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
import os
import os.path
import tempfile
import ConfigParser
import logging.config
from playhouse.db_url import connect


ENV_CONF_FILE = 'CELLCONFIG_SETTINGS'
ENV_PAXOS_HOME = 'PAXOS_HOME'
ENV_PAXOS_CELL_CONFIG = 'PAXOS_CELL_CONFIG'

conf = ConfigParser.SafeConfigParser(
    {'pwd': os.getcwd(), 'mkstemp': tempfile.mkstemp()[1]})
conf.optionxform = str
if ENV_CONF_FILE in os.environ:
    conf.read(os.environ.get(ENV_CONF_FILE))
else:
    conf.read('conf/cellconfig.cfg')
if os.path.exists('logger.cfg'):
    logging.config.fileConfig('logger.cfg')

if ENV_PAXOS_HOME in os.environ:
    path = os.path.normpath(
        os.path.join(os.environ[ENV_PAXOS_HOME], 'cell/config'))
    try:
        os.makedirs(path)
    except os.error:
        pass
    os.environ[ENV_PAXOS_CELL_CONFIG] = path + '/'
else:
    os.environ[ENV_PAXOS_CELL_CONFIG] = os.path.abspath('.') + '/'


db = connect(conf.get(__package__, 'DATABASE'))


def use_dns():
    try:
        return conf.getboolean(__package__, 'DNS')
    except (ConfigParser.NoOptionError, ValueError):
        return False


def syslog_conf():
    from .model_admin import SyslogConf
    return SyslogConf()


def syslog_conf():
    from .model_admin import SyslogConf
    return SyslogConf()
