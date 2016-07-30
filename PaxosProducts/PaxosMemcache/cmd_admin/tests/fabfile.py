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
from fabric.api import run, task, put, get, local
from fabric.state import env
from fabric.context_managers import cd
from fabric.contrib.files import exists
import fabric.operations
from paxos.cmd.pmemcache import config
import os.path


logger = logging.getLogger(__name__)


@task
def check():
    run('echo "ok"')


@task
def clear_config():
    run('rm -rf {0}'.format(config.CONFIG_DIR))


@task
def clear_data():
    run('rm -rf {0}'.format(config.DATA_DIR0))


@task
def clear_data_srv(css, no):
    with cd(config.DATA_DIR0):
        run('rm -rf {0}/{1}'.format(css.name, no), quiet=True)


@task
def check_pfs_dirs(css, no, exists=True):
    with cd(config.DATA_DIR0):
        ret = run('test -d {0}/{1}'.format(css.name, no), quiet=True)
        if not exists:
            return ret.return_code != 0
        if ret.return_code != 0:
            return False
        with cd('{0}/{1}'.format(css.name, no)):
            ret0 = run('test -d DATA', quiet=True)
            ret1 = run('test -d PFS', quiet=True)
            return ret0.return_code == 0 and ret1.return_code == 0


@task
def check_pfs_seq(css, no, tp, seq=None):
    ret = run('ps -C PFSServer -o args= --no-headers', quiet=True)
    logger.debug(ret)
    if ret.return_code != 0:
        return False
    for cmd in ret.stdout.splitlines():
        w = cmd.strip().split()
        if w[1] != css.name or int(w[2]) != no:
            continue
        if tp == 'TRUE':
            return tp == w[4] and seq == int(w[5])
        else:
            return tp == w[4]


@task
def get_pfs_start(css, no):
    ret = run('ps -C PFSServer -o start,args= --no-headers', quiet=True)
    if ret.return_code != 0:
        return None
    for cmd in ret.stdout.splitlines():
        w = cmd.strip().split()
        if w[2] == css.name and int(w[3]) == no:
            return w[0]


@task
def get_pfs_seq(css, no):
    path = '{0}/{1}/{2}'.format(config.DATA_DIR0, css.name, no)
    ret = run('test -d {0}'.format(path), quiet=True)
    if ret.return_code != 0:
        return 0
    with cd(path):
        ret = run('cat SHUTDOWN', quiet=True)
        if ret.return_code != 0:
            return None
        w = ret.stdout.strip().split()
        return int(w[0])


@task
def remove_shutdown(css, no):
    with cd('{0}/{1}/{2}'.format(config.DATA_DIR0, css.name, no)):
        run('rm SHUTDOWN')


@task
def rewrite_shutdown(css, no, val):
    with cd('{0}/{1}/{2}'.format(config.DATA_DIR0, css.name, no)):
        run('echo "{0} []" > SHUTDOWN'.format(val))


@task
def check_cell_conf(css_list):
    with cd(config.CONFIG_CELL_DIR):
        for css in css_list:
            if not(exists(os.path.join(config.CONFIG_CELL_DIR,
                                       '{0}.conf'.format(css)))):
                return False
        files = run('ls {0}/*'.format(config.CONFIG_CELL_DIR))
        for path in str(files).strip().split():
            fname = os.path.basename(path)
            (name, ext) = os.path.splitext(fname)
            if ext != '.conf':
                return False
            if name not in css_list:
                return False
    return True


@task
def stop_css(name, no):
    run('{0} css lstop -n {1} -N {2}'.format(config.CMD_PAXOS, name, no))
