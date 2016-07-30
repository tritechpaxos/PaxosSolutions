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
import logging

from fabric.api import run, task, put, get, local
from fabric.state import env, output
from fabric.context_managers import cd, warn_only
from fabric.contrib.files import exists

from . import config, error
import paxos.cmd.pmemcache as pm


env.abort_exception = error.FabError
logger = logging.getLogger(__name__)


@task
def deploy(clean):
    if clean:
        cmd = 'rm -rf {0}'.format(config.CONFIG_CELL_DIR)
        logger.debug("[{0}] {1}".format(env.host_string, cmd))
        run(cmd)
    cmd = 'mkdir -p {0}'.format(config.CONFIG_CELL_DIR)
    logger.debug("[{0}] {1}".format(env.host_string, cmd))
    run(cmd)
    logger.debug("[{2}] PUT: {0} {1}".format(
        config.MEMCACHE_CONF_PATH, config.MEMCACHE_CONF_PATH, env.host_string))
    put(config.MEMCACHE_CONF_PATH, config.MEMCACHE_CONF_PATH)
    try:
        logger.debug("[{2}] PUT: {0} {1}".format(
            config.CONFIG_CELL_PATTERN, config.CONFIG_CELL_DIR,
            env.host_string))
        put(config.CONFIG_CELL_PATTERN, config.CONFIG_CELL_DIR)
    except ValueError:
        pass


@task
def fetch_config():
    local('mkdir -p {0}'.format(config.CONFIG_CELL_DIR))
    logger.debug("[{2}] GET: {0} {1}".format(
        os.path.join(config.CONFIG_DIR, '*'), config.CONFIG_DIR,
        env.host_string))
    get(os.path.join(config.CONFIG_DIR, '*'), config.CONFIG_DIR)


def css_cmd(subcmd, name, no, opt=''):
    if not(pm.DEBUG):
        cmd = config.CMD_PAXOS
    else:
        dbg_log = ('' if pm.debug_log is None
                   else ' --debug {0}'.format(pm.debug_log))
        v_opt = (' -' + 'v' * (pm.verbose - 1)) if pm.verbose > 1 else ''
        cmd = '{0}=1 {1}{2}{3}'.format(
            pm.ENV_PAXOS_DEBUG, config.CMD_PAXOS, dbg_log, v_opt)

    cmdline = (
        'env {env}=1 {cmd} css {subcmd} --name {name} --no {no}{opt}'
        .format(cmd=cmd, subcmd=subcmd, name=name, no=no, opt=opt,
                env=config.ENV_PAXOS_LOCAL))
    logger.debug("[{0}] {1}".format(env.host_string, cmdline))
    return run(cmdline, pty=False)


@task
def css_start(name, no, initialize=False):
    opt = ' --initialize' if initialize else ''
    return css_cmd('lstart', name, no, opt)


@task
def css_stop(name, no, force=False):
    opt = ' --force' if force else ''
    return css_cmd('lstop', name, no, opt)


@task
def css_state(name, no, mode):
    opt = ' --long' if mode == 'long' else ''
    return css_cmd('lstate', name, no, opt)


@task
def css_replace(name, no, *args):
    opt = ' ' + ' '.join(args)
    return css_cmd('lreplace', name, no, opt)


@task
def paxos_installed():
    return exists(config.CMD_PAXOS)
