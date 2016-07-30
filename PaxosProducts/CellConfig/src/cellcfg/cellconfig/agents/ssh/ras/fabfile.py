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
import logging
import os
import os.path
import re
from fabric.api import run, task, put, local
from fabric.state import env
from fabric.context_managers import cd

from ..fabfile import FabError, _search_paxos_pkg, host_value
from ..paxos import Paxos
from .paxos import RasEye


logger = logging.getLogger(__name__)


def search_paxos_pkg(path, os_name, dist_name, machine):
    return _search_paxos_pkg(path, os_name, dist_name, machine, 'paxos-ras')


@task
def deploy_server(home, os_name, dist_name, machine, **kwargs):
    pkg_path = search_paxos_pkg('dist', os_name, dist_name, machine)
    run("mkdir -p " + host_value(home))
    work_dir = run("mktemp --tmpdir -d")
    try:
        put(pkg_path, work_dir)
        with cd(work_dir):
            fname = os.path.basename(pkg_path)
            run("tar xzf {}".format(fname))
            run("./setup.sh {}".format(host_value(home)))
    finally:
        run("rm -rf {}".format(work_dir))


@task
def deploy_conf(conf_path, cell, home, **kwargs):
    paxos = Paxos.create(cell=cell, home=host_value(home))
    dest_path = paxos.cell_conf_path()
    run("mkdir -p {0}".format(os.path.dirname(dest_path)))
    put(conf_path, dest_path)


@task
def start_server(group, home, server_no):
    for no in host_value(server_no):
        try:
            srv = RasEye(group, host_value(home), no)
            logger.debug("COMMAND: nohup {comm} &".format(
                comm=srv.command_line()))
            run("nohup {comm} &".format(comm=srv.command_line()), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def stop_server(group, home, server_no):
    logger.debug("STOP SERVER: fabfile")
    for no in host_value(server_no):
        try:
            adm = RasEye(group, host_value(home), no).admin()
            logger.debug("COMMAND: {}".format(adm.stop()))
            run(adm.stop(), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def set_event_server(group, home, server_no, ras_cell, path):
    for no in host_value(server_no):
        logger.debug(
            "SET EVENT: path={} no={} cell={}".format(path, no, ras_cell))
        adm = RasEye(group, host_value(home), no).admin()
        try:
            run(adm.set_event(ras_cell, path), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def unset_event_server(group, home, server_no, ras_cell, path):
    for no in host_value(server_no):
        adm = RasEye(group, host_value(home), no).admin()
        try:
            run(adm.unset_event(ras_cell, path), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def setup_cell(cell, home, server_no, ras_cell, path):
    for no in host_value(server_no):
        paxos = Paxos.create(cell=cell, home=host_value(home), server_no=no)
        try:
            run(paxos.ras_cluster(ras_cell, path), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def setup_ras(group, home, server_no, ras_cell):
    for no in host_value(server_no):
        adm = RasEye(group, host_value(home), no).admin()
        try:
            run(adm.regist_me(ras_cell), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))


@task
def teardown_ras(group, home, server_no, ras_cell):
    for no in host_value(server_no):
        adm = RasEye(group, host_value(home), no).admin()
        try:
            run(adm.unregist_me(ras_cell), pty=False)
        except FabError as e:
            logger.debug("ERROR: {}".format(str(e)))
