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
import json
import logging
import os.path
import collections
import copy
from peewee import fn
from wtforms import Form, validators, FieldList
from wtformsparsleyjs import (
    StringField, SelectField, TextAreaField, HiddenField, BooleanField,
    FileField,
)

from .. import database
from cellconfig.model import Cell, Node
from cellconfig.model_admin import SyslogConf
from cellconfig.ras.model import (
    RasGroup, RasServer, RasTarget, RasAction, RasScript)
from ..forms import CellForm


logger = logging.getLogger(__name__)


class RasGroupForm(Form):
    name = StringField(
        label=u'クラスタ名',
        validators=[validators.DataRequired(),
                    validators.Length(max=256),
                    validators.Regexp('^\w+$')],
    )
    comment = TextAreaField(
        label=u'コメント',
        default=u'',
    )
    cell = SelectField(
        label=u'RASセル',
        coerce=int,
        choices=[],
    )

    @classmethod
    def delete(cls, id):
        grp = RasGroup.get(RasGroup.id == id)
        with database.database.atomic():
            RasTargetForm.delete_nonexists_target('ras', grp.name)
            for tgt in grp.targets:
                RasTargetForm.delete(tgt.id)
            for srv in grp.servers:
                RasServerForm.delete(srv.id)
            grp.delete_instance()

    @classmethod
    def obj2dict(cls, obj):
        return {
            'name': obj.name,
            'comment': obj.comment,
            'cell': obj.cell.id,
        }

    def get_cell_name(self, id):
        cell = Cell.get(Cell.id == id)
        return cell.name

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(RasGroupForm, self).__init__(formdata, obj, prefix, data, meta,
                                           **kwargs)
        self._update_select()

    def _update_select(self):
        cells = Cell.select().where(Cell.type == 'RAS')
        self.cell.choices = [(c.id, c.name) for c in cells]

    def create(self, url):
        group = RasGroup()
        self.populate_obj(group)
        group.id = None
        group.save(url=url)
        return group


class RasServerForm(Form):

    node = SelectField(
        label=u'サーバ',
        coerce=int,
        choices=[],
    )

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, group=None, **kwargs):
        super(RasServerForm, self).__init__(formdata, obj, prefix, data, meta,
                                            **kwargs)
        self._update_select(group)
        try:
            self.node.data = obj.node.id
        except AttributeError:
            pass

    def _update_select(self, grpid):
        nodes0 = RasServer.select(
            RasServer.node).where(RasServer.group == grpid)
        nodes = Node.select().where(Node.id.not_in(nodes0))
        self.node.choices = [(n.id, n.hostname) for n in nodes]

    def create(self, grpid):
        srv = RasServer()
        self.populate_obj(srv)
        srv.id = None
        last_no = RasServer.select(
            fn.Max(RasServer.no)).where(RasServer.group == grpid).scalar()
        srv.no = last_no + 1 if last_no is not None else 0
        srv.group = grpid
        if len(srv.group.servers) == 0:
            grp = RasGroup.get(RasGroup.id == grpid)
            srv.save(callback=lambda: self.regist_auto_target(grp))
        else:
            srv.save()

    def exist_restart_action(self, cell):
        tgts = RasTarget.select().where(
            (RasTarget.name == cell.name) & (RasTarget.type == 'cell'))
        for tgt in tgts:
            acts = RasAction.select().where(
                (RasAction.target == tgt.id) &
                (RasAction.type == 'restart')).first()
            if acts is not None:
                return True
        return False

    def regist_ras_cell(self, group):
        cell = group.cell
        if self.exist_restart_action(cell):
            return
        logger.debug("REGIST CELL: {}".format(cell))
        tgt = RasTargetCellForm(data={'type': 'cell', 'cell': cell.id})
        tgt.create(group.id)
        tobj = RasTarget.select().where(
            (RasTarget.name == cell.name) &
            (RasTarget.type == 'cell') &
            (RasTarget.group == group.id)).first()
        act = RasActionRestartForm(data={'type': 'restart', 'name': 'restart'})
        act.create(tobj.id)

    def regist_ras_eye(self, group):
        logger.debug("REGIST RAS: {}".format(group))
        tgt = RasTargetRasForm(data={'type': 'ras', 'group': group.id})
        tgt.create(group.id)
        tobj = RasTarget.select().where(
            (RasTarget.name == group.name) &
            (RasTarget.type == 'ras') &
            (RasTarget.group == group.id)).first()
        act = RasActionRestartForm(data={'type': 'restart', 'name': 'restart'})
        act.create(tobj.id)

    def regist_auto_target(self, group):
        logger.debug("REGIST AUTO TARGET: {}".format(group))
        self.regist_ras_cell(group)
        self.regist_ras_eye(group)

    def get_hostname(self, nid):
        for c in self.node.choices:
            if c[0] == nid:
                return c[1]
        return None

    @classmethod
    def delete(cls, id):
        srv = RasServer.get(RasServer.id == id)
        with database.database.atomic():
            srv.stop()
            srv.delete_instance()


class RasTargetForm(Form):

    LABEL = collections.OrderedDict([
        ('cell', u'セル'),
        ('ras', u'監視クラスタ'),
        ('app', u'アプリケーション'),
    ])

    type = HiddenField(label=u'')

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(RasTargetForm, self).__init__(
            formdata, obj, prefix, data, meta, **kwargs)
        self.type.data = self._get_type()
        self._update_selectfield()

    def _update_selectfield(self):
        pass

    @classmethod
    def delete(cls, id):
        tgt = RasTarget.get(RasTarget.id == id)
        with database.database.atomic():
            for act in tgt.actions:
                RasActionForm.delete(act.id)
            tgt.delete_instance()
        tgt.teardown()

    @classmethod
    def delete_nonexists_target(cls, _type, name):
        tgts = RasTarget.select().where(
            (RasTarget.type == _type) & (RasTarget.name == name))
        for tgt in tgts:
            RasTargetForm.delete(tgt.id)


class RasTargetCellForm(RasTargetForm):

    cell = SelectField(
        label=u'セル名',
        coerce=int,
        choices=[],
    )
    comment = TextAreaField(
        label=u'コメント',
        default=u'',
    )

    @classmethod
    def obj2dict(cls, obj):
        cell = Cell.select().where(Cell.name == obj.name).first()
        logger.debug("CELL ID: {}".format(cell.id))
        return {
            'type': 'cell',
            'cell': cell.id,
            'comment': obj.comment,
        }

    def get_cell_name(self, id):
        cell = Cell.get(Cell.id == id)
        return cell.name

    def _update_selectfield(self):
        registerd = RasTarget.select(RasTarget.name).where(
            RasTarget.type == 'cell')
        self.cell.choices = [
            (c.id, c.name) for c in
            Cell.select().where(Cell.name.not_in(registerd))]

    def _get_type(self):
        return 'cell'

    def create(self, grpid):
        logger.debug("CREATE: {}".format(grpid))
        tgt = RasTarget()
        cell = Cell.get(Cell.id == self.cell.data)
        tgt.name = cell.name
        tgt.group = grpid
        tgt.type = 'cell'
        tgt.path = '{app}/{cell}'.format(
            app=cell.get_appname(), cell=cell.name)
        tgt.comment = self.comment.data
        tgt.save()


class RasTargetRasForm(RasTargetForm):

    group = SelectField(
        label=u'監視クラスタ名',
        coerce=int,
        choices=[],
    )
    comment = TextAreaField(
        label=u'コメント',
        default=u'',
    )

    @classmethod
    def obj2dict(cls, obj):
        grp = RasGroup.select().where(RasGroup.name == obj.name).first()
        logger.debug("RasGroup ID: {}".format(grp.id))
        return {
            'type': 'ras',
            'group': grp.id,
            'comment': obj.comment,
        }

    def _update_selectfield(self):
        registerd = RasTarget.select(RasTarget.name).where(
            RasTarget.type == 'ras')
        self.group.choices = [
            (g.id, g.name) for g in
            RasGroup.select().where(RasGroup.name.not_in(registerd))]

    def _get_type(self):
        return 'ras'

    def get_group_name(self, id):
        grp = RasGroup.get(RasGroup.id == id)
        return grp.name

    def create(self, grpid):
        tgt = RasTarget()
        group = RasGroup.get(RasGroup.id == self.group.data)
        tgt.name = group.name
        tgt.group = grpid
        tgt.type = 'ras'
        tgt.path = '{app}/{grp}'.format(app='RasEye', grp=group.name)
        tgt.comment = self.comment.data
        tgt.save()


class RasTargetAppForm(RasTargetForm):

    application = StringField(
        label=u'アプリケーション名',
        validators=[validators.DataRequired()],
    )
    name = StringField(
        label=u'監視名',
        validators=[validators.DataRequired()],
    )
    comment = TextAreaField(
        label=u'コメント',
        default=u'',
    )

    def _get_type(self):
        return 'app'

    def create(self, grpid):
        tgt = RasTarget()
        tgt.name = self.name.data
        tgt.group = grpid
        tgt.type = 'app'
        tgt.path = '{app}/{name}'.format(
            app=self.application.data, name=self.name.data)
        tgt.comment = self.comment.data
        tgt.save()


class RasActionForm(Form):

    _action_no = 0

    LABEL = collections.OrderedDict([
        ('smtp', u'メール通知'),
        ('syslog', u'Syslog通知'),
        ('http', u'WebAPI実行'),
        ('restart', u'自動起動'),
        ('script', u'スクリプト実行'),
    ])

    type = HiddenField(label=u'')
    name = StringField(
        label=u'アクション名',
        validators=[validators.DataRequired(),
                    validators.Length(min=1, max=256),
                    validators.Regexp('^\w+$')],
    )

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(RasActionForm, self).__init__(
            formdata, obj, prefix, data, meta, **kwargs)
        self.type.data = self._get_type()
        if not self.name.data:
            self.name.data = '{}{}'.format(
                self.type.data, RasActionForm._action_no)

    def get_labels(self, target):
        if target.type != 'app':
            return self.LABEL.items()
        else:
            labels = copy.copy(self.LABEL)
            del labels['restart']
            return labels.items()

    @classmethod
    def delete(cls, id):
        RasAction.delete().where(RasAction.id == id).execute()

    def update(self, actid, tgtid, script_data=None):
        action = RasAction()
        action.id = actid
        self._save(action, tgtid, script_data)

    def create(self, tgtid, script_data=None):
        action = RasAction()
        self._save(action, tgtid, script_data)
        RasActionForm._action_no += 1


class RasActionBaseForm(RasActionForm):

    MODE_LABEL = [u'起動', u'停止', u'更新']

    mode = FieldList(
        BooleanField(default=True),
        label=u'通知契機',
        min_entries=len(MODE_LABEL),
    )

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(RasActionBaseForm, self).__init__(
            formdata, obj, prefix, data, meta, **kwargs)
        for (e, l) in zip(self.mode.entries, RasActionBaseForm.MODE_LABEL):
            e.label = l

    @classmethod
    def obj2dict(cls, obj):
        ret = json.loads(obj.data)
        ret['type'] = obj.type
        ret['name'] = obj.name
        ret['mode'] = []
        for i in range(3):
            ret['mode'].append(((obj.mode & (1 << i)) != 0))
        return ret

    def _save(self, action, tgtid, script_data):
        action.target = tgtid
        action.name = self.name.data
        action.mode = 0
        for (n, m) in enumerate(self.mode.entries):
            if m.data:
                action.mode |= (1 << n)
        action.type = self.type.data
        data = dict(
            (field.name, field.data) for field in self
            if field.name not in ['name', 'mode', 'type'])
        logger.debug("DATA: {}".format(data))
        action.data = json.dumps(data)
        action.save()


class RasActionMailForm(RasActionBaseForm):

    to = StringField(
        label=u'宛先(To)',
        validators=[validators.DataRequired(),
                    validators.Email()],
    )
    title = StringField(
        label=u'件名',
        validators=[validators.DataRequired()],
    )
    message = TextAreaField(
        label=u'本文',
        default='{default}'
    )
    attach = BooleanField(
        label=u'データファイル添付',
    )

    def _get_type(self):
        return 'smtp'


class RasActionSyslogForm(RasActionBaseForm):

    PROTOCOL = ['udp', 'tcp', 'unix']
    FACILITY = SyslogConf.FACILITY
    PRIORITY = SyslogConf.PRIORITY

    address = StringField(
        label=u'アドレス(:ポート)',
        validators=[validators.DataRequired()]
    )
    protocol = SelectField(
        label=u'プロトコル',
        choices=[(i, i) for i in PROTOCOL],
    )
    facility = SelectField(
        label=u'ファシリティ',
        choices=[(i, i) for i in FACILITY],
        default='user',
    )
    priority = SelectField(
        label=u'プライオリティ',
        choices=[(i, i) for i in PRIORITY],
        default='info',
    )
    message = TextAreaField(
        label=u'メッセージ',
        default='{default}',
    )

    def _get_type(self):
        return 'syslog'


class RasActionRestartForm(RasActionForm):

    retry = StringField(
        label=u'再試行',
        default='5',
        validators=[validators.DataRequired(),
                    validators.NumberRange(min=0)],
    )
    interval = StringField(
        label=u'インターバル（秒）',
        default='60',
        validators=[validators.DataRequired(),
                    validators.NumberRange(min=0)],
    )

    def _get_type(self):
        return 'restart'

    def _save(self, action, tgtid, script_data=None):
        tgt = RasTarget.get(RasTarget.id == tgtid)
        action.target = tgtid
        action.name = self.name.data
        action.mode = 1 << 1
        action.type = 'restart'
        action.data = json.dumps(
            {'type': tgt.type, 'retry': int(self.retry.data),
             'interval': int(self.interval.data)})
        action.save()

    @classmethod
    def obj2dict(cls, obj):
        ret = json.loads(obj.data)
        ret['type'] = obj.type
        ret['name'] = obj.name
        return ret


class RasActionScriptForm(RasActionBaseForm):

    cmdline = TextAreaField(
        label=u'コマンドライン',
        validators=[validators.DataRequired()],
    )

    workdir = StringField(
        label=u'作業ディレクトリ',
        validators=[validators.DataRequired()],
        default='{default}',
    )

    script = FileField(
        label=u'スクリプト',
    )

    def _get_type(self):
        return 'script'

    def _save(self, action, tgtid, script_data=None):
        with database.database.atomic():
            super(RasActionScriptForm, self)._save(action, tgtid, script_data)
            script = RasScript()
            logger.debug("SCRIPT: {}".format(script_data))
            if script_data is not None:
                script.script = script_data
            script.action = action
            script.save()


class RasActionHttpForm(RasActionBaseForm):

    METHODS = ['GET', 'POST', 'PUT', 'PATCH', 'DELETE']

    method = SelectField(
        label=u'メソッド',
        choices=[(i, i) for i in METHODS]
    )
    url = StringField(
        label=u'URL',
        validators=[validators.DataRequired(),
                    validators.URL()]
    )
    params = TextAreaField(
        label=u'パラメータ',
    )
    header = TextAreaField(
        label=u'ヘッダ',
    )

    def _get_type(self):
        return 'http'


class RasCellForm(CellForm):

    def __init__(self, formdata=None, obj=None, prefix='', data=None,
                 meta=None, **kwargs):
        super(RasCellForm, self).__init__(formdata, obj, prefix, data, meta,
                                          **kwargs)
        self.type.choices = [('RAS', 'RAS')]
