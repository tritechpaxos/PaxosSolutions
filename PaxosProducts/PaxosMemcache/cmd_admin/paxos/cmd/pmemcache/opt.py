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
import argparse
import sys
from os import getenv

from . import memcache, css, node, config
import paxos.cmd.pmemcache as pm

LOCAL_MODE = False if getenv(config.ENV_PAXOS_LOCAL) is None else True

console_mode = True if len(sys.argv) == 1 else False

parser = argparse.ArgumentParser(prog='' if console_mode else None,
                                 add_help=False if console_mode else True)
if pm.DEBUG:
    parser.add_argument('--debug', metavar='debug.log')
    parser.add_argument('-v', action='count', dest='verbose', default=0)

parser.add_argument('--version', action='version',
                    version='%(prog)s {0}'.format(pm.__version__))

subparsers = parser.add_subparsers()

parser_id = argparse.ArgumentParser(add_help=False)
parser_id.add_argument(
    '--id', '-i', type=int, metavar='id',
    help=_('identification number of PAXOS memcache'))
parser_id_req = argparse.ArgumentParser(add_help=False)
parser_id_req.add_argument(
    '--id', '-i', required=True, type=int, metavar='id',
    help=_('identification number of PAXOS memcache'))
parser_cell_req = argparse.ArgumentParser(add_help=False)
parser_cell_req.add_argument(
    '--css', '-c', required=True, metavar='cell',
    help=_('cell name of CSS'))
parser_cell = argparse.ArgumentParser(add_help=False)
parser_cell.add_argument(
    '--css', '-c', metavar='cell', help=_('cell name of CSS'))
parser_force = argparse.ArgumentParser(add_help=False)
parser_force.add_argument(
    '--force', action='store_true', help=_('forced to terminate the procss'))
memcache_opt = argparse.ArgumentParser(add_help=False)
memcache_opt.add_argument(
    '--name-space', '-N', metavar='space', default='space0',
    help=_('name space of  PAXOS memcache'))
memcache_opt.add_argument(
    '--max-cache-item', '-I', type=int, metavar='max',
    help=_('maximum value of the cache item (default:2000)'))
memcache_checksum_flag = memcache_opt.add_mutually_exclusive_group()
memcache_checksum_flag.add_argument(
    '--disable-checksum', '-s', action='store_false', dest='checksum',
    help=_('disable checksum processing'), default=None)
memcache_checksum_flag.add_argument(
    '--enable-checksum', '-S', action='store_true', dest='checksum',
    help=_('enable checksum processing (default)'), default=None)
memcache_opt.add_argument(
    '--checksum-interval', '-C', type=int, metavar='sec',
    help=_('interval for calculating the checksum'))
memcache_opt.add_argument(
    '--expire-interval', '-E', type=int, metavar='sec',
    help=_('interval to expire the cache'))
memcache_opt.add_argument(
    '--worker', '-W', type=int, metavar='num',
    help=_('number of worker (default:5)'))
memcache_ns = argparse.ArgumentParser(add_help=False)
memcache_ns.add_argument(
    '--name-space', '-N', metavar='space',
    help=_('name space of  PAXOS memcache'))
css_name = argparse.ArgumentParser(add_help=False)
css_name.add_argument(
    '--name', '-n', metavar='cell', help=_('cell name of CSS'))
css_name_req = argparse.ArgumentParser(add_help=False)
css_name_req.add_argument(
    '--name', '-n', required=True, metavar='cell', help=_('cell name of CSS'))
css_servers = argparse.ArgumentParser(add_help=False)
css_servers.add_argument(
    'servers', nargs=6, metavar='addr:port',
    help=_('list of CSS server(udp/tcp) constituting the cell'))
css_init = argparse.ArgumentParser(add_help=False)
css_init.add_argument(
    '--initialize', action='store_true',
    help=_('initialize the environment of CSS'))
css_mod_opts = argparse.ArgumentParser(add_help=False)
css_mod_opts.add_argument(
    '--no', '-N', required=True, metavar='no', type=int,
    help=_('number of replacement target'))
css_mod_opts.add_argument(
    '--address', '-a', required=True, nargs=2,
    metavar=('udp_addr:port', 'tcp_addr:port'),
    help=_('server address after replacement'))
css_mod_opts.add_argument('--force', action='store_true')
css_no = argparse.ArgumentParser(add_help=False)
css_no.add_argument(
    '--no', '-N', metavar='no', type=int, required=True,
    help=_('no. of CSS server'))
node_nd = argparse.ArgumentParser(add_help=False)
node_nd.add_argument(
    'hostname', metavar='hostname', help=_('hostname'))
node_opts = argparse.ArgumentParser(add_help=False)
node_opts.add_argument('--user', '-u', metavar='user', help=_('user name'))
node_opts.add_argument(
    '--port', '-p', metavar='port', type=int, help=_('port number of ssh'))
node_opts.add_argument(
    '--group', '-g', metavar='grp0[:grp1:...] ',
    help=_('group that the host belong to'))
node_grp = argparse.ArgumentParser(add_help=False)
node_grp.add_argument(
    '--group', '-g', metavar='grp0[:grp1:...] ',
    help=_('group that the host belong to'))

node_use_ssh = argparse.ArgumentParser(add_help=False)
node_use_ssh.add_argument(
    '--use-ssh-config', action='store_true',
    help=_('to use the setting of ssh_config'))
node_ssh_x = argparse.ArgumentParser(add_help=False)
node_modx = node_ssh_x.add_mutually_exclusive_group()
node_modx.add_argument(
    '--use-ssh-config', action='store_true',
    help=_('to use the setting of ssh_config'))
node_modx.add_argument(
    '--not-use-ssh-config', action='store_false', dest='use_ssh_config',
    help=_('do not use the setting of ssh_config'))
node_hosts = argparse.ArgumentParser(add_help=False)
node_hosts.add_argument(
    'hostname', metavar='hostname', nargs='*', help=_('hostname'))
st_mode_sq = argparse.ArgumentParser(add_help=False)
sq_mode_statex = st_mode_sq.add_mutually_exclusive_group()
sq_mode_statex.add_argument('--short', '-s', action='store_true')
sq_mode_statex.add_argument('--quiet', '-q', action='store_true')
st_mode_lsq = argparse.ArgumentParser(add_help=False)
lsq_mode_statex = st_mode_lsq.add_mutually_exclusive_group()
lsq_mode_statex.add_argument(
    '--short', '-s', action='store_const', const='short', dest='mode')
lsq_mode_statex.add_argument(
    '--quiet', '-q', action='store_const', const='quiet', dest='mode')
lsq_mode_statex.add_argument(
    '--long', '-l', action='store_const', const='long', dest='mode')
parser_pair_addr = argparse.ArgumentParser(add_help=False)
parser_pair_addr.add_argument(
    '--server-address', '-a', required=True, metavar='address',
    help=_('IPv4 address of the server'))
parser_pair_addr.add_argument(
    '--server-port', '-p', metavar='port', type=int,
    help=_('port number of the server'))
parser_pair_addr.add_argument(
    '--client-address', '-A', metavar='address',
    help=_('IPv4 address to be exposed to the client'))
parser_pair_addr.add_argument(
    '--client-port', '-P', metavar='port', type=int,
    help=_('port number to be exposed to the client'))
deploy_clean = argparse.ArgumentParser(add_help=False)
deploy_clean.add_argument(
    '--clean', action='store_true',
    help=_('delete the old configuration files'))

if LOCAL_MODE:
    parser_css = subparsers.add_parser(
        'css', help=_('configure the PAXOS CSS'))
    css_subparsers = parser_css.add_subparsers()

if LOCAL_MODE:
    css_lstart = css_subparsers.add_parser(
        'lstart', parents=[css_name_req, css_no, css_init],
        help=_('start the PAXOS CSS'))
    css_lstart.set_defaults(cmd=css.LstartCommand)
    css_lstop = css_subparsers.add_parser(
        'lstop', parents=[css_name_req, css_no, parser_force],
        help=_('stop the local PAXOS CSS'))
    css_lstop.set_defaults(cmd=css.LstopCommand)
    css_lstate = css_subparsers.add_parser(
        'lstate', parents=[css_name_req, css_no],
        help=_('local state of the PAXOS CSS'))
    css_lstate.add_argument(
        '--long', '-l', action='store_const', const='long', dest='mode')
    css_lstate.set_defaults(cmd=css.LstateCommand)
    css_lmod = css_subparsers.add_parser(
        'lreplace', parents=[css_name_req], help=_('replace the CSS server'))
    css_lmod.add_argument(
        '--no', '-N', required=True, metavar='no', type=int,
        help=_('number of replacement target'))
    css_lmod_xmode = css_lmod.add_mutually_exclusive_group()
    css_lmod_xmode.add_argument(
        '--remove-dir', action='store_const', const='rm', dest='mode')
    css_lmod_xmode.add_argument(
        '--replace-dir', action='store_const', const='mv', dest='mode')
    css_lmod_xmode.add_argument(
        '--change-srv', action='store_const', const='srv', dest='mode')
    css_lmod.add_argument('--old', metavar='no', type=int)
    css_lmod.set_defaults(cmd=css.LreplaceCommand)

parser_register = subparsers.add_parser(
    'register', help=_('register the object')).add_subparsers()
parser_unregister = subparsers.add_parser(
    'unregister', help=_('unregister the object')).add_subparsers()
parser_modify = subparsers.add_parser(
    'modify', help=_('modify the registered contents of the object')
    ).add_subparsers()
parser_show = subparsers.add_parser(
    'show', help=_('show the registered contents of the object')
    ).add_subparsers()
parser_list = subparsers.add_parser(
    'list', help=_('shows a list of the object')).add_subparsers()
parser_start = subparsers.add_parser(
    'start', help=_('start the object')).add_subparsers()
parser_stop = subparsers.add_parser(
    'stop', help=_('stop the object')).add_subparsers()
parser_state = subparsers.add_parser(
    'status', help=_('show the state of the object')).add_subparsers()
parser_deploy = subparsers.add_parser(
    'deploy', parents=[node_grp, node_hosts, deploy_clean],
    help=_('deploy the configuration files to each host'))
parser_fetch = subparsers.add_parser(
    'fetch', parents=[node_nd, node_use_ssh],
    help=_('fetch the configuration files from the specified host'))
parser_apply = subparsers.add_parser(
    'apply', help=_('apply the changes to the object')).add_subparsers()
parser_replace = subparsers.add_parser(
    'replace', parents=[css_name_req, css_mod_opts],
    help=_('to run the member change of CSS server'))

memcache_register = parser_register.add_parser(
    'memcache', parents=[parser_cell_req, parser_id, memcache_opt],
    help=_('register PAXOS memcache'))
memcache_register.set_defaults(cmd=memcache.RegisterCommand)
pair_register = parser_register.add_parser(
    'pair', help=_('register PAXOS memcache pair'),
    parents=[parser_id_req, parser_pair_addr])
pair_register.set_defaults(cmd=memcache.PairAddCommand)
css_register = parser_register.add_parser(
    'css', help=_('register PAXOS CSS'), parents=[css_name_req, css_servers])
css_register.set_defaults(cmd=css.RegisterCommand)
node_register = parser_register.add_parser(
    'host', parents=[node_opts, node_nd, node_use_ssh],
    help=_('register PAXOS host'))
node_register.set_defaults(cmd=node.RegisterCommand)

memcache_unregister = parser_unregister.add_parser(
    'memcache', parents=[parser_id_req],
    help=_('unregister PAXOS memcache'))
memcache_unregister.set_defaults(cmd=memcache.UnregisterCommand)
pair_unregister = parser_unregister.add_parser(
    'pair', help=_('unregister PAXOS memcache pair'),
    parents=[parser_id_req, parser_pair_addr])
pair_unregister.set_defaults(cmd=memcache.PairRemoveCommand)
css_unregister = parser_unregister.add_parser(
    'css', help=_('unregister PAXOS CSS'), parents=[css_name_req])
css_unregister.set_defaults(cmd=css.UnregisterCommand)
node_unregister = parser_unregister.add_parser(
    'host', help=_('unregister PAXOS host'), parents=[node_nd])
node_unregister.set_defaults(cmd=node.UnregisterCommand)

memcache_modify = parser_modify.add_parser(
    'memcache', help=_('modify the CSS of PAXOS memcache'),
    parents=[parser_id_req, parser_cell, memcache_opt])
memcache_modify.set_defaults(cmd=memcache.ModifyCommand)
node_modify = parser_modify.add_parser(
    'host', parents=[node_opts, node_nd, node_ssh_x],
    help=_('modify user or port number of the PAXOS host'))
node_modify.set_defaults(cmd=node.ModifyCommand)

memcache_show = parser_show.add_parser(
    'memcache', parents=[parser_id, parser_cell, memcache_ns],
    help=_('show the configuration of PAXOS memcache'))
memcache_show.set_defaults(cmd=memcache.ShowCommand)
css_show = parser_show.add_parser(
    'css', parents=[css_name], help=_('show the configuration of PAXOS CSS'))
css_show.set_defaults(cmd=css.ShowCommand)
node_show = parser_show.add_parser(
    'host', parents=[node_opts, node_hosts],
    help=_('show the configuration of the PAXOS host'))
node_show.set_defaults(cmd=node.ShowCommand)

memcache_list = parser_list.add_parser(
    'memcache', help=_('ID list of PAXOS memcache'))
memcache_list.set_defaults(cmd=memcache.ListCommand)
css_list = parser_list.add_parser('css', help=_('list of PAXOS CSS'))
css_list.set_defaults(cmd=css.ListCommand)
node_list = parser_list.add_parser(
    'host', parents=[node_opts], help=_('list of the PAXOS host'))
node_list.set_defaults(cmd=node.ListCommand)

memcache_start = parser_start.add_parser(
    'memcache', parents=[parser_id], help=_('start the PAXOS memcache'))
memcache_start.set_defaults(cmd=memcache.StartCommand)
css_start = parser_start.add_parser(
    'css', parents=[css_name, css_init], help=_('start the PAXOS CSS'))
css_start.set_defaults(cmd=css.StartCommand)

memcache_stop = parser_stop.add_parser(
    'memcache', parents=[parser_id, parser_force],
    help=_('stop the PAXOS memcache'))
memcache_stop.set_defaults(cmd=memcache.StopCommand)
css_stop = parser_stop.add_parser(
    'css', parents=[css_name, parser_force],
    help=_('stop the PAXOS CSS'))
css_stop.set_defaults(cmd=css.StopCommand)

memcache_state = parser_state.add_parser(
    'memcache', parents=[parser_id, parser_cell, memcache_ns, st_mode_sq],
    help=_('state of PAXOS memcache'))
memcache_state.set_defaults(cmd=memcache.StateCommand)
css_state = parser_state.add_parser(
    'css', parents=[css_name, st_mode_lsq], help=_('state of the PAXOS CSS'))
css_state.set_defaults(cmd=css.StateCommand)

pair_apply = parser_apply.add_parser(
    'pair', parents=[parser_id],
    help=_('apply the settings of PAXOS memcache pair'))
pair_apply.set_defaults(cmd=memcache.PairApplyCommand)

parser_deploy.set_defaults(cmd=node.DeployCommand)
parser_fetch.set_defaults(cmd=node.FetchCommand)
parser_replace.set_defaults(cmd=css.ReplaceCommand)


def add_help_parser(prs, help_prs0):
    help_prs = argparse.ArgumentParser(prog='')
    help_prs0.set_defaults(help_parser=help_prs)
    help_subprs = help_prs.add_subparsers()
    if prs._subparsers is None:
        return
    sbprses = [act for act in prs._subparsers._actions
               if isinstance(act, argparse._SubParsersAction)]
    for sbprs in sbprses:
        for k, v in sbprs._name_parser_map.iteritems():
            help_prs2 = help_subprs.add_parser(k, add_help=False)
            help_prs2.set_defaults(parser=v)
            add_help_parser(v, help_prs2)


if console_mode:
    help_parser = argparse.ArgumentParser(prog='')
    help_prs = help_parser.add_subparsers().add_parser('help', add_help=False)
    help_prs.set_defaults(parser=parser)
    add_help_parser(parser, help_prs)
