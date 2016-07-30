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
import fabric.tasks
import fabric.state

from . import util, fabfile, error, model


class Node(model.Model):

    TAG = 'node'
    KEY = 'hostname'
    DEFAULT_USER = 'paxos'

    def __init__(self, hostname, user=None, port=None, groups=[],
                 use_ssh_config=False):
        self.hostname = hostname
        self.user = user
        self.port = port
        self.groups = groups
        self.use_ssh_config = use_ssh_config

    def match(self, kv):
        if 'hostname' in kv:
            if isinstance(kv['hostname'], list):
                if self.hostname not in kv['hostname']:
                    return False
            elif self.hostname != kv['hostname']:
                return False
        if 'user' in kv and self.user != kv['user']:
            return False
        if 'port' in kv and self.port != kv['port']:
            return False
        if 'groups' in kv:
            g0 = set(kv['groups'])
            g1 = set(self.groups)
            if len(g0 & g1) == 0:
                return False
        return True

    def ssh_address(self):
        return '{user}{host}{port}'.format(
            user='{0}@'.format(self.user) if self.user is not None else '',
            host=self.hostname,
            port=':{0}'.format(self.port) if self.port is not None else '')

    def __str__(self):
        user = ('user: "{0}"\n'.format(self.user)
                if self.user is not None else '')
        port = 'port: {0}\n'.format(self.port) if self.port is not None else ''
        grp = 'group: {0}\n'.format(
            '[' + ', '.join(self.groups) + ']') if len(self.groups) > 0 else ''
        ssh = 'use_ssh_config: yes' if self.use_ssh_config else ''
        return 'hostname: "{hostname}"\n{user}{port}{groups}{ssh}'.format(
            hostname=self.hostname, user=user, port=port, groups=grp, ssh=ssh)


util.register_class(Node, '!' + Node.TAG)


class RegisterCommand(model.Command):
    def __init__(self, hostname, user, port, group, use_ssh_config=False,
                 *args, **kwargs):
        super(RegisterCommand, self).__init__(*args, **kwargs)
        self.hostname = hostname
        if user is not None or use_ssh_config:
            self.user = user
        else:
            self.user = Node.DEFAULT_USER
        self.port = port
        self.groups = group.split(':') if group is not None else []
        self.use_ssh_config = use_ssh_config

    def run(self):
        if Node.exist(self.hostname):
            raise error.ExistError(_('host already exists'))
        node = Node(self.hostname, self.user, self.port, self.groups,
                    self.use_ssh_config)
        Node.append(node)


class UnregisterCommand(model.Command):
    def __init__(self, hostname, *args, **kwargs):
        super(UnregisterCommand, self).__init__(*args, **kwargs)
        self.hostname = hostname

    def in_use(self, node):
        from .css import Css
        return any([css.in_use(node) for css in Css.find()])

    def run(self):
        node = Node.get(self.hostname)
        if self.in_use(node):
            raise error.InUseError(_('this host is in use.'))
        node.delete_instance()


class ModifyCommand(model.Command):
    def __init__(self, hostname, user, port, group, use_ssh_config=None,
                 *args, **kwargs):
        super(ModifyCommand, self).__init__(*args, **kwargs)
        self.hostname = hostname
        self.user = user
        self.port = port
        self.groups = group.split(':') if group is not None else None
        self.use_ssh_config = use_ssh_config

    def run(self):
        node = Node.get(self.hostname)
        if self.user is not None:
            node.user = self.user
        if self.port is not None:
            node.port = self.port
        if self.groups is not None:
            node.groups = self.groups
        if self.use_ssh_config is not None:
            node.use_ssh_config = self.use_ssh_config
            if not node.use_ssh_config and node.user is None:
                node.user = node.DEFAULT_USER
        node.save()


class ShowCommand(model.Command):
    def __init__(self, hostname, user, port, group, *args, **kwargs):
        super(ShowCommand, self).__init__(*args, **kwargs)
        self.search = {}
        if hostname is not None and len(hostname):
            self.search['hostname'] = hostname
        if user is not None:
            self.search['user'] = user
        if port is not None:
            self.search['port'] = port
        if group is not None:
            self.search['groups'] = group.split(':')

    def run(self):
        nodes = Node.find(self.search)
        for n in nodes:
            print '---'
            print str(n).strip()


class ListCommand(model.Command):
    def __init__(self, user, port, group, *args, **kwargs):
        super(ListCommand, self).__init__(*args, **kwargs)
        self.search = {}
        if user is not None:
            self.search['user'] = user
        if port is not None:
            self.search['port'] = port
        if group is not None:
            self.search['groups'] = group.split(':')

    def run(self):
        nodes = Node.find(self.search)
        for n in nodes:
            print n.hostname


class DeployCommand(model.Command):
    def __init__(self, hostname, group=None, clean=False,
                 css=None, *args, **kwargs):
        super(DeployCommand, self).__init__(*args, **kwargs)
        self.clean = clean if clean is not None else False
        self.css = css
        self.search = {}
        if hostname is not None and len(hostname):
            self.search['hostname'] = hostname
        if group is not None:
            self.search['groups'] = group.split(':')

    def run(self):
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        nodes = Node.find(self.search)
        errors = []
        for n in nodes:
            if util.same_dir(n):
                continue
            fabric.state.env.hosts = [n.ssh_address()]
            if n.use_ssh_config:
                fabric.state.env.use_ssh_config = True
            try:
                fabric.tasks.execute(fabfile.deploy, self.clean)
            except (error.FabError, EOFError) as e:
                errors.append(e)
        if len(errors) > 0:
            raise error.DeployError(errors)


class FetchCommand(model.Command):
    def __init__(self, hostname, use_ssh_config, *args, **kwargs):
        super(FetchCommand, self).__init__(*args, **kwargs)
        self.hostname = hostname
        self.use_ssh_config = use_ssh_config

    def run(self):
        if self.use_ssh_config:
            fabric.state.env.use_ssh_config = True
        try:
            n = Node.get(self.hostname)
            if util.same_dir(n):
                return
            fabric.state.env.hosts = [n.ssh_address()]
            if n.use_ssh_config:
                fabric.state.env.use_ssh_config = True
        except error.NotFoundError:
            fabric.state.env.hosts = [self.hostname]
        if self.verbose == 0:
            fabric.state.output['everything'] = False
            fabric.state.output['aborts'] = False
        try:
            fabric.tasks.execute(fabfile.fetch_config)
        except (error.FabError, EOFError) as e:
            msgs = e.message.splitlines()
            raise error.FetchError(msgs[0].strip())
