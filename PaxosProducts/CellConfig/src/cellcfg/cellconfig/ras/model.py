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
from tempfile import mkdtemp
from os import unlink, rmdir, environ
import os.path
import logging
import json
from functools import partial
from peewee import *
import gevent

from cellconfig import db as _db
from cellconfig import ENV_PAXOS_CELL_CONFIG
from ..model import BaseModel, Cell, Node
from ..agent import call
from .model_admin import SmtpConf
import pycss


logger = logging.getLogger(__name__)


class RasGroup(BaseModel):
    id = PrimaryKeyField(sequence='ras_group_seq' if _db.sequences else None)
    name = CharField(unique=True, constraints=[Check('name <> ""')])
    comment = CharField(null=True)
    cell = ForeignKeyField(Cell)
    enabled = BooleanField(default=True)

    def deploy(self):
        self.cell.deploy_conf()

    def start(self):
        return self.cell.start()

    def generate_cell_conf_file(self):
        try:
            logger.debug("CONF: {}".format(os.environ[ENV_PAXOS_CELL_CONFIG]))
            os.makedirs(os.environ[ENV_PAXOS_CELL_CONFIG])
        except os.error as e:
            logger.exception(e)
        conf_file = os.path.join(
            os.environ[ENV_PAXOS_CELL_CONFIG],
            '{}.conf'.format(self.cell.name))
        self.cell.generate_conf(conf_file)

    def regist_cellcfg(self, url_root):
        logger.debug("REGIST: {}".format(url_root))
        css = pycss.connect(self.cell.name.encode())
        css.mkdir('/cellcfg')
        lines = []
        if css._exists('/cellcfg/list'):
            with css.open('/cellcfg/list') as f:
                lines = f.read().strip().split('\n')
        lines.append(url_root)
        lines = list(set(lines))
        with css.open('/cellcfg/list', 'w') as f:
            for line in lines:
                f.write(line + '\n')

    def save(self, *args, **kwargs):
        if 'url' in kwargs:
            url = kwargs['url']
            del(kwargs['url'])
        else:
            url = None

        def after_save(x):
            self.generate_cell_conf_file()
            if url is not None:
                self.regist_cellcfg(url)

        with _db.atomic() as txn:
            try:
                super(RasGroup, self).save(*args, **kwargs)
                self.deploy()
                g = self.start()
                if isinstance(g, gevent.Greenlet):
                    g.link(after_save)
                else:
                    after_save(None)
            except:
                self.enabled = False
                super(RasGroup, self).save(*args, **kwargs)


class RasServer(BaseModel):
    id = PrimaryKeyField(sequence='ras_server_seq' if _db.sequences else None)
    node = ForeignKeyField(Node)
    no = IntegerField(constraints=[Check('no >= 0')])
    group = ForeignKeyField(RasGroup, related_name='servers')

    @call(ns='RasServer', name='deploy', async=True)
    def deploy_async(self):
        pass

    def start_with_params(self, params):
        logger.debug("START EYE: {}".format(params))

        def do_retry(x, retry):
            gevent.sleep(params['interval'] if 'interval' in params else 60)
            logger.debug("START EYE RETRY: {}".format(retry))
            g = self.start_async()
            if retry > 0:
                retry -= 1
                g.link_exception(partial(do_retry, retry=retry))
        retry = params['retry'] if 'retry' in params else 0
        g = self.start_async()
        if retry > 0:
            g.link_exception(partial(do_retry, retry=retry))
        return g

    @call(ns='RasServer', name='start', async=True)
    def start_async(self):
        pass

    @call(ns='RasServer')
    def stop(self):
        pass

    def save(self, *args, **kwargs):
        if 'callback' in kwargs:
            callback = kwargs['callback']
            del(kwargs['callback'])
            logger.debug("CALLBACK: {}".format(callback))
        else:
            callback = None

        def after_save(x):
            logger.debug("AFTER_SAVE: {}".format(callback))
            for tgt in self.group.targets:
                tgt.set_event(nodes=[self])
            if callback is not None:
                callback()

        def after_deploy(x):
            logger.debug("AFTER_DEPLOY: {}".format(str(self)))
            with _db.atomic() as txn:
                super(RasServer, self).save(*args, **kwargs)
                g = self.start_async()
                g.link_value(after_save)

        g = self.deploy_async()
        g.link_value(after_deploy)


class RasTarget(BaseModel):
    id = PrimaryKeyField(sequence='ras_target_seq' if _db.sequences else None)
    name = CharField()
    group = ForeignKeyField(RasGroup, related_name='targets')
    type = CharField(
        constraints=[Check('type IN ("cell", "ras", "app")')])
    path = CharField()
    comment = CharField()
    enabled = BooleanField(default=True)

    def setup(self):
        if self.type == 'cell':
            self.target = Cell.select().where(Cell.name == self.name)[0]
            logger.debug("TARGET: {}".format(self.target))
            self.set_event()
            self.setup_cell()
        elif self.type == 'ras':
            self.target = RasGroup.select().where(
                RasGroup.name == self.name)[0]
            logger.debug("TARGET: {}".format(self.target))
            self.set_event()
            self.setup_ras()

    @call(ns='RasTarget')
    def set_event(self, nodes=None):
        pass

    @call(ns='RasTarget')
    def setup_cell(self):
        pass

    @call(ns='RasTarget')
    def setup_ras(self):
        pass

    def teardown(self):
        if self.type == 'cell':
            self.target = Cell.select().where(Cell.name == self.name)[0]
            self.unset_event()
        elif self.type == 'ras':
            self.target = RasGroup.select().where(
                RasGroup.name == self.name)[0]
            self.unset_event()
            self.teardown_ras()

    @call(ns='RasTarget')
    def unset_event(self):
        pass

    @call(ns='RasTarget')
    def teardown_ras(self):
        pass

    def save(self, *args, **kwargs):
        with _db.atomic() as txn:
            self.setup()
            super(RasTarget, self).save(*args, **kwargs)

    @call(ns='RasTarget')
    def status(self):
        pass


class RasAction(BaseModel):
    id = PrimaryKeyField(sequence='ras_action_seq' if _db.sequences else None)
    target = ForeignKeyField(RasTarget, related_name='actions')
    name = CharField()
    mode = IntegerField()
    type = CharField()
    data = CharField()
    enabled = BooleanField(default=True)


class RasScript(BaseModel):
    id = PrimaryKeyField(sequence='ras_script_seq' if _db.sequences else None)
    action = ForeignKeyField(
        RasAction, related_name='action', on_delete='CASCADE')
    script = TextField(null=True)
