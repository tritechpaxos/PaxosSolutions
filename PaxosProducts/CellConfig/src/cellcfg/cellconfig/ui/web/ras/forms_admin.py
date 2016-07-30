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
from wtforms import validators
from wtformsparsleyjs import StringField
from ..forms_admin import AdminConfBaseForm
from cellconfig.ras.model_admin import SmtpConf
from cellconfig.ras.model import RasGroup


class AdminConfSmtpForm(AdminConfBaseForm):

    server = StringField(
        label=u'SMTPサーバ',
        validators=[validators.DataRequired()],
        default=SmtpConf.DEFAULT['server'],
    )

    addr = StringField(
        label=u'送信者アドレス(From)',
        validators=[validators.DataRequired(), validators.Email()],
        default=SmtpConf.DEFAULT['addr'],
    )

    def conf_type(self):
        return SmtpConf.NAME
