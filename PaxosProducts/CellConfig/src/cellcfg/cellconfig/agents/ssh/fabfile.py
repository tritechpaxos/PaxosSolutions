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
import glob
import re
import logging

from fabric.api import run, task, put, local
from fabric.state import env
from fabric.context_managers import cd

from .paxos import Paxos
from .misc import MiscData


logger = logging.getLogger(__name__)


class FabError(Exception):
    def __init__(self, message, hostname=None):
        super(FabError, self).__init__(message)
        self.message = message
        self.host = hostname

    def __str__(self):
        return (self.message if self.host is None
                else "{} ({})".format(self.message, self.host))

env.abort_on_prompts = True
env.abort_exception = FabError


@task
def uname(opt='-s'):
    return run("uname {}".format(opt))


@task
def distribution():
    script = """
import platform
os_name = platform.linux_distribution()[0:2]
print '{0} {1}'.format(*os_name)
"""
    try:
        return run('python -c "{0}"'.format(script))
    except FabError as e:
        lines = run('lsb_release -i -r')
        ret = ''
        for line in lines.splitlines():
            ret = ret + ' ' + line.split(':')[1].strip()
        return ret.lstrip()


def _version_cmp(a, b):
    if len(a) == 0 and len(b) == 0:
        return 0
    if a[0] > b[0]:
        return 1
    elif a[0] < b[0]:
        return -1
    else:
        return cmp(a[1:], b[1:])


def _latest_version(pkg_name, paths):

    pat = re.compile(pkg_name + '\.(\d+)\.(\d+)\.(\d+)\.')
    matches = [(pat.search(path), path) for path in paths]
    versions = [(map(int, m[0].groups()), m[1]) for m in matches
                if m[0] is not None]
    if len(versions) == 0:
        return None
    latest = reduce(lambda a, b: a if _version_cmp(a[0], b[0]) >= 0 else b,
                    versions)
    return latest[1]


def _search_paxos_pkg(path, os_name, dist_name, machine, pkg_name):
    def make_candidate(x, e):
        if x is None:
            return ([e], e)
        new_e = '-'.join([x[1], e])
        new_l = list(x[0])
        new_l.append(new_e)
        return (new_l, new_e)

    if os_name is None:
        raise FabError('os_name is None')
    os_name = os_name.lower()
    if dist_name is not None:
        words = re.split('[\s\.]+', dist_name.lower())
        dist = reduce(make_candidate, words, None)[0]
        dist.reverse()
        dist.append(os_name)
    else:
        dist = [os_name]
    for d in dist:
        spath = os.path.join(
            path, os_name, '{}.*.{}.{}.tar.gz'.format(pkg_name, d, machine))
        logger.debug("SEARCH PATH: {}".format(spath))
        paths = glob.glob(spath)
        if len(paths) > 0:
            return _latest_version(pkg_name, paths)
    raise FabError(
        'unable to find the {}.{} of the package.'.format(pkg_name, dist_name))


def search_paxos_pkg(path, os_name, dist_name, machine):
    return _search_paxos_pkg(path, os_name, dist_name, machine, 'paxos-core')


def host_value(value):
    return value[env.host_string] if isinstance(value, dict) else value


def _version_check(home, pkg_name, pkg_path):
    pat = re.compile(pkg_name + '\.(\d+)\.(\d+)\.(\d+)\.')
    pkg_ver = map(int, pat.search(pkg_path).groups())
    with cd(home):
        filename = os.path.basename(pkg_path)
        try:
            ver_txt = str(run('cat .install/paxos_core'))
            pat = pkg_name.replace('-', '_') + '=(\d+)\.(\d+)\.(\d+)'
            cur_ver = map(int, re.search(pat, ver_txt).groups())
            return _version_cmp(cur_ver, pkg_ver) >= 0
        except Exception as e:
            logger.exception(e)
            return False


@task
def deploy(node, home, force=False, **kwargs):
    phome = host_value(home)
    pkg_path = search_paxos_pkg(
        'dist', node.os, node.distribution, node.machine)
    if (not force) and _version_check(phome, 'paxos-core', pkg_path):
        logger.debug("SKIP DEPLOY")
        return
    run("mkdir -p " + phome)
    put(pkg_path, phome)
    with cd(phome):
        filename = os.path.basename(pkg_path)
        run("tar xzf " + filename)
        run("rm " + filename)


@task
def deploy_conf(conf_path, cell, home, **kwargs):
    paxos = Paxos.create(cell=cell, home=host_value(home))
    dest_path = paxos.cell_conf_path()
    run("mkdir -p {0}".format(os.path.dirname(dest_path)))
    put(conf_path, dest_path)


@task
def cell_setup(cell, home, server_no):
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        data_path = paxos.cell_data_path()
        run("mkdir -p {0}".format(data_path))
        with cd(data_path):
            run("mkdir -p {0}".format(paxos.cell_data_subdirs()))
        run("mkdir -p {0}".format(paxos.cell_log_path()))


@task
def get_paxos_service_pid(cell, server_no, **kwargs):
    ret = dict()
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, server_no=no)
        out = run("ps -C {0} -o pid=,args= -ww".format(paxos.program_name()))
        ret[no] = paxos.parse_ps(out)
    return ret


@task
def paxos_admin(cell, home, server_no):
    ret = dict()
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        admin = paxos.get_admin()
        output = run(admin.command())
        ret[no] = admin.parse(output)
    return ret


def _get_shutdown_no(paxos):
    path = paxos.shutdown_filename()
    logger.debug("PATH: {}".format(path))
    ret = run("test -f {}".format(path), quiet=True)
    if ret.succeeded:
        ret = run("cat {}".format(path))
        return int(str(ret).split()[0])
    else:
        logger.debug("NOEXIST: {}".format(path))
        return -1


@task
def get_shutdown_no(cell, home, server_no):
    next_no = dict()
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        next_no[no] = _get_shutdown_no(paxos)
    return next_no


@task
def paxos_start(cell, home, server_no):
    logger.debug("SERVER NO: no={}".format(server_no))
    for no in host_value(server_no):
        logger.debug("PAXOS START: no={} home={}".format(no, host_value(home)))
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        next_no = _get_shutdown_no(paxos)
        logger.debug("SHUTDOWN: {}".format(next_no))
        logger.debug('nohup {} &'.format(paxos.service_command_line(next_no)))
        run("nohup {} &".format(paxos.service_command_line(next_no)),
            pty=False)
        local("sleep 1")


@task
def paxos_stop(cell, home, server_no):
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        logger.debug("STOP: no={} cmd={}".format(no, paxos.session_shutdown()))
        run(paxos.session_shutdown())


@task
def paxos_probe(cell, home, server_no, op):
    ret = MiscData()
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        probe = paxos.get_probe(op)
        output = run(probe.command())
        ret.append(probe.parse(output))
    return ret
