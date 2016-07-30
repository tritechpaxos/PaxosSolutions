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
from wtforms import Form, validators
from wtformsparsleyjs import IntegerField, StringField, SelectField
from . import database
from cellconfig.model_admin import AdminConf, SyslogConf


logger = logging.getLogger(__name__)


class AdminConfBaseForm(Form):

    def save(self):
        with database.database.atomic():
            conf = AdminConf.select().where(
                AdminConf.name == self.conf_type()).first()
            if conf is None:
                conf = AdminConf()
                conf.name = self.conf_type()
            data = dict((field.name, field.data) for field in self)
            conf.data = json.dumps(data)
            conf.save()

    @classmethod
    def obj2dict(cls, obj):
        ret = json.loads(obj.data)
        ret['name'] = obj.name
        return ret


class AdminConfLogForm(AdminConfBaseForm):

    level = IntegerField(
        label=u'ログレベル',
        validators=[validators.DataRequired()],
        default=SyslogConf.DEFAULT['level'],
    )

    address = StringField(
        label=u'Syslogサーバ',
        validators=[validators.DataRequired()],
        default=SyslogConf.DEFAULT['address'],
    )

    facility = SelectField(
        label=u'ファシリティ',
        choices=[(i, i) for i in SyslogConf.FACILITY],
        default=SyslogConf.DEFAULT['facility'],
    )
    priority = SelectField(
        label=u'プライオリティ',
        choices=[(i, i) for i in SyslogConf.PRIORITY],
        default=SyslogConf.DEFAULT['priority'],
    )

    def conf_type(self):
        return SyslogConf.NAME
