# -*- coding: utf-8 -*-
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
import string
from wtforms import (
    Form, FormField, FieldList, validators)
from wtforms.compat import iteritems
from wtformsparsleyjs import (
    StringField, PasswordField, IntegerField, SelectField,
    TextAreaField, HiddenField)
from peewee import IntegrityError

from . import database
from cellconfig.model import Node, Cell, CellNode


logger = logging.getLogger(__name__)


class BaseForm(Form):
    pass


class NodeForm(BaseForm):
    hostname = StringField(
        label=u'ホスト名',
        validators=[validators.DataRequired()],
    )
    user = StringField(
        label=u'ユーザ',
        validators=[validators.DataRequired()],
        default='paxos',
    )
    password = PasswordField(
        label=u'パスワード',
    )
    port = IntegerField(
        label=u'sshポート番号',
        validators=[
            validators.NumberRange(
                min=0,
                max=65535,
            )
        ],
        default=22,
    )
    paxos_home = StringField(
        label=u'PAXOS HOME',
        validators=[validators.DataRequired()],
        default='/home/paxos/PAXOS',
    )

    def create(self):
        node = Node()
        self.populate_obj(node)
        node.id = None
        try:
            node.check_and_save()
            node.deploy_async()
            return True
        except IntegrityError as e:
            logger.error(str(e))
            self.hostname.errors.append(e.message)

    def update(self, id):
        node = Node().get(Node.id == id)
        self.populate_obj(node)
        node.check_and_save()

    @classmethod
    def delete(cls, id):
        Node.delete().where(Node.id == id).execute()


class CellNodeForm(BaseForm):
    node = SelectField(
        label=u'サーバ',
        coerce=int,
        choices=[],
    )
    udp_addr = StringField(
        label=u'ホストアドレス(UDP)',
        validators=[validators.DataRequired(),
                    validators.IPAddress()],
    )
    udp_port = IntegerField(
        label=u'UDPポート番号',
        validators=[
            validators.DataRequired(),
            validators.NumberRange(
                min=0,
                max=65535,
            )
        ],
        default=14000,
    )
    tcp_addr = StringField(
        label=u'ホストアドレス(TCP)',
        validators=[validators.DataRequired(),
                    validators.IPAddress()],
    )
    tcp_port = IntegerField(
        label=u'TCPポート番号',
        validators=[
            validators.DataRequired(),
            validators.NumberRange(
                min=0,
                max=65535,
            )
        ],
        default=14000,
    )
    node_id = HiddenField()


class CellForm(BaseForm):

    CNODE_MIN = 3
    name = StringField(
        label=u'セル名',
        validators=[validators.DataRequired(),
                    validators.Length(max=256),
                    validators.Regexp('^\w+$')],
    )
    type = SelectField(
        label=u'セル種別',
        choices=[(tp, tp) for tp in Cell.CELL_TYPE],
    )
    comment = TextAreaField(
        label=u'コメント',
    )
    nodes = FieldList(
        FormField(CellNodeForm),
        label=u'サーバ',
        min_entries=CNODE_MIN,
    )

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(CellForm, self).__init__(formdata, obj, prefix, data, meta,
                                       **kwargs)
        self.update_nodes()

    def update_nodes(self):
        nodes = Node.select()
        for cnode in self.nodes:
            cnode['node'].choices = [(n.id, n.hostname) for n in nodes]

    def populate_obj(self, obj):
        for name, field in iteritems(self._fields):
            if not isinstance(field, FieldList):
                field.populate_obj(obj, name)

    def populate_cellnodes(self, nodes):
        while len(self.nodes) > 0:
            self.nodes.pop_entry()
        for n in nodes:
            ent = self.nodes.append_entry(n)
            ent['node'].data = n.node.id
            ent['node_id'].data = n.id
        self.update_nodes()

    def exists_cnode(self, cnode):
        for ent in self.nodes:
            if ent['node_id'].data.isnumeric() and\
               int(ent['node_id'].data) == cnode.id:
                return True
        return False

    def get_hostname(self, host_id):
        if len(self.nodes) == 0:
            return None
        choices = self.nodes[0]['node'].choices
        for ch in choices:
            if ch[0] == host_id:
                return ch[1]
        return None

    def _save_cell(self, cell):
        self.populate_obj(cell)
        try:
            cell.save()
            return True
        except IntegrityError as e:
            logger.error(str(e))
            self.name.errors.append(e.message)

    def _save_cnode(self, nform, nid, idx, cell):
        cnode = CellNode()
        nform.form.populate_obj(cnode)
        cnode.id = nid
        cnode.no = idx
        cnode.cell = cell
        try:
            cnode.save()
            return True
        except IntegrityError as e:
            logger.error(str(e))
            if string.find(e.message, 'tcp') >= 0:
                nform.form.tcp_port.errors.append(e.message)
            elif string.find(e.message, 'udp') >= 0:
                nform.form.udp_port.errors.append(e.message)
            else:
                self.name.errors.append(e.message)

    def create(self):
        with database.database.atomic() as txn:
            cell = Cell()
            if not self._save_cell(cell):
                txn.rollback()
                return False
            for idx, nform in enumerate(self.nodes):
                if not self._save_cnode(nform, None, idx, cell):
                    txn.rollback()
                    return False
        try:
            cell.deploy_conf()
        except Exception as e:
            logger.exceptions(e)
        return True

    def update(self, id):
        with database.database.atomic() as txn:
            cell = Cell().get(Cell.id == id)
            if not self._save_cell(cell):
                txn.rollback()
                return False
            for cnode in cell.nodes:
                if not self.exists_cnode(cnode):
                    cnode.delete().where(CellNode.id == cnode.id).execute()
            for idx, nform in enumerate(self.nodes):
                node_id = nform.form['node_id'].data
                nid = (int(node_id)
                       if node_id is not None and node_id.isnumeric()
                       else None)
                if not self._save_cnode(nform, nid, idx, cell):
                    txn.rollback()
                    return False
        try:
            cell.deploy_conf()
        except Exception as e:
            logger.exceptions(e)
        return True

    def _pop_cnode(self, num):
        cnode_list = []
        for i in range(num):
            cnode = CellNode()
            nform = self.nodes.pop_entry()
            nform.form.populate_obj(cnode)
            cnode_list.insert(0, cnode)
        return cnode_list

    def _push_cnode(self, cnode_list):
        node_list = Node.select()
        for cnode in cnode_list:
            nform = self.nodes.append_entry(cnode)
            nform['node'].choices = [(n.id, n.hostname) for n in node_list]
            if cnode._data.get('node') is not None:
                nform['node'].data = cnode.node.id

    def delete_cnode_by_no(self, no):
        if len(self.nodes) <= CellForm.CNODE_MIN:
            return
        cnode_list = self._pop_cnode(len(self.nodes) - no - 1)
        self.nodes.pop_entry()
        self._push_cnode(cnode_list)

    def insert_cnode_by_no(self, no):
        cnode_list = self._pop_cnode(len(self.nodes) - no)
        cnode_list.insert(0, CellNode())
        self._push_cnode(cnode_list)

    @classmethod
    def delete(cls, id):
        with database.database.atomic():
            cell = Cell.get(Cell.id == id)
            try:
                from .ras.forms import RasTargetForm
                RasTargetForm.delete_nonexists_target('cell', cell.name)
            except Exception as e:
                logger.exception(e)
            cell.delete_instance()
            CellNode.delete().where(CellNode.cell == id).execute()
