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
import collections
from flask import request, redirect, url_for, abort, render_template
from . import app
from cellconfig.model_admin import AdminConf, SyslogConf
from .forms_admin import AdminConfLogForm


admin_tabs = collections.OrderedDict(
    [(u'ログ設定', ('admin_conf', SyslogConf.NAME))])
admin_labels = {SyslogConf.NAME: u'ログ'}
admin_forms = {SyslogConf.NAME: AdminConfLogForm}


@app.template_global()
def get_admin_tabs():
    return admin_tabs.items()


@app.template_global()
def get_admin_label():
    return admin_labels


@app.route('/admin')
def admin_index():
    return redirect(url_for('admin_conf', typ=SyslogConf.NAME))


@app.route('/admin/conf/<typ>', methods=['GET', 'POST'])
def admin_conf(typ):
    if request.method == 'GET':
        form = _build_form(typ)
        return render_template('admin_conf.html', form=form, typ=typ)
    if request.method == 'POST':
        form = admin_forms[typ](request.form)
        if not form.validate():
            return render_template(
                'admin_conf_update.html', form=form, typ=typ)
        form.save()
        return redirect(url_for('admin_conf', typ=typ))


@app.route('/admin/conf/<typ>/update')
def admin_conf_update(typ):
    form = _build_form(typ)
    return render_template('admin_conf_update.html', form=form, typ=typ)


def _build_form(typ):
    fcls = admin_forms[typ]
    conf = AdminConf.select().where(AdminConf.name == typ).first()
    return fcls(data=fcls.obj2dict(conf)) if conf is not None else fcls()
