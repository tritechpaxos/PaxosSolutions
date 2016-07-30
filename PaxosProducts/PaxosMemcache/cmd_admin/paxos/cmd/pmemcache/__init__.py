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
import logging
import signal
from os import getenv
from . import error

__version__ = '0.6.2'

ENV_PAXOS_DEBUG = '_PAXOS_DEBUG'
DEBUG = False if getenv(ENV_PAXOS_DEBUG) is None else True

logger = logging.getLogger(__name__)
if DEBUG:
    logger.setLevel(logging.DEBUG)
else:
    logger.setLevel(logging.WARNING)

ch = logging.StreamHandler()
ch.setLevel(logging.WARNING)
formatter = logging.Formatter('%(levelname)s: %(message)s')
ch.setFormatter(formatter)

logger.addHandler(ch)

debug_log = None
verbose = 0


def alarm_handler(signum, frame):
    raise error.TimeoutError('timeout error')

signal.signal(signal.SIGALRM, alarm_handler)
