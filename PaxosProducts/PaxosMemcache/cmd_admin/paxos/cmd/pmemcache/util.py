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
from errno import EEXIST
import os
from os.path import expanduser
import socket
import pwd
from psutil import net_if_addrs
import yaml
import logging
import signal
from subprocess import Popen, PIPE, CalledProcessError

from . import config, error


logger = logging.getLogger(__name__)


def register_class(cls, tag):
    suffix = '{module}.{name}'.format(module=cls.__module__, name=cls.__name__)
    yaml.add_representer(
        cls, lambda dumper, obj: dumper.represent_mapping(tag, obj.__dict__))
    yaml.add_constructor(
        tag, lambda loader, node: loader.construct_python_object(suffix, node))


def makedirs(path):
    try:
        os.makedirs(expanduser(path))
    except OSError as e:
        if e.errno != EEXIST:
            raise e


def match_hostaddr(host):
    host_addr = host.hostname if hasattr(host, 'hostname') else host
    if host_addr == socket.gethostname() or host_addr == socket.getfqdn():
        return True
    for addrs in net_if_addrs().values():
        for addr in addrs:
            if addr.family == socket.AF_INET and addr.address == host_addr:
                return True
    return False


def same_dir(host):
    if host.use_ssh_config:
        return False  # XXX
    if not match_hostaddr(host):
        return False
    return host.user == pwd.getpwuid(os.getuid())[0]


def check_output_timeout(*popenargs, **kwargs):
    if 'stdout' in kwargs:
        raise ValueError('stdout argument not allowed, it will be overridden.')
    timeout = kwargs.get("timeout")
    if timeout is None:
        timeout = config.CMD_TIMEOUT
    process = Popen(stdout=PIPE, *popenargs, **kwargs)
    signal.alarm(timeout)
    try:
        output, unused_err = process.communicate()
    except error.TimeoutError as e:
        process.terminate()
        raise e
    finally:
        signal.alarm(0)
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        raise CalledProcessError(retcode, cmd, output=output)
    return output


def check_call_timeout(*popenargs, **kwargs):
    timeout = kwargs.get("timeout")
    if timeout is None:
        timeout = config.CMD_TIMEOUT
    process = Popen(*popenargs, **kwargs)
    signal.alarm(timeout)
    try:
        retcode = process.wait()
    except error.TimeoutError as e:
        process.terminate()
        raise e
    finally:
        signal.alarm(0)
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        raise CalledProcessError(retcode, cmd)
    return 0
