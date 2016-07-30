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
from flask import request, redirect, url_for, abort, render_template

from .. import app
from cellconfig.model import Cell
from cellconfig.ras.model import RasGroup, RasServer, RasTarget, RasAction
from .forms import *
from . import api, event


logger = logging.getLogger(__name__)


@app.template_global()
def status2ctxclass(status):
    s = status['status']
    if s == 'running':
        return 'success'
    elif s == 'stopped':
        return 'active'
    elif s == 'warning':
        return 'warning'
    elif s == 'error':
        return 'danger'
    else:
        return ''


@app.template_global()
def actiontype2label(atype):
    return RasActionForm.LABEL[atype]


@app.template_global()
def status2text(status):
    s = status['status']
    if s == 'running':
        return u'正常'
    elif s == 'stopped':
        return u'停止'
    elif s == 'warning':
        return u'一部停止'
    elif s == 'error':
        return u'異常'
    else:
        return u'不明'


@app.route('/rascell', methods=['GET', 'POST'])
def rascell_list():
    if request.method == 'GET':
        cells = Cell.select().where(Cell.type == 'RAS')
        return render_template('ras/cell_list.html', cells=cells)
    elif request.method == 'POST':
        form = RasCellForm(request.form)
        if not form.validate():
            return render_template('ras/cell_create.html', form=form)
        form.create()
        return redirect(url_for('rasgrp_list'))


@app.route('/rascell/create')
def rascell_create():
    form = RasCellForm()
    return render_template('ras/cell_create.html', form=form)


@app.route('/rasclstr', methods=['GET', 'POST'])
def rasgrp_list():
    if request.method == 'GET':
        if Cell.select().where(Cell.type == 'RAS').count() == 0:
            return redirect(url_for('rascell_list'))
        grps = RasGroup.select()
        return render_template('ras/ras_index.html', groups=grps)
    elif request.method == 'POST':
        form = RasGroupForm(request.form)
        if not form.validate():
            return render_template('ras/group_create.html', form=form)
        logger.debug('URL: {}'.format(request.url_root))
        grp = form.create(request.url_root)
        logger.debug('GROUP: {}'.format(str(grp)))
        return redirect(url_for('rasgrp_list'))


@app.route('/rasclstr/create')
def rasgrp_create():
    form = RasGroupForm()
    return render_template('ras/group_create.html', form=form)


@app.route('/rasclstr/<int:id>', methods=['GET', 'POST'])
def rasgrp_detail(id):
    if request.method == 'GET':
        group = RasGroup.get(RasGroup.id == id)
        return render_template('ras/group_show.html', group=group)
    elif request.method == 'POST':
        if request.form['operation'] == 'Delete':
            RasGroupForm.delete(id)
            return redirect(url_for('rasgrp_list'))


@app.route('/rasclstr/<int:id>/update')
def rasgrp_update(id):
    pass


@app.route('/rasclstr/<int:id>/delete')
def rasgrp_delete(id):
    grp = RasGroup.get(RasGroup.id == id)
    form = RasGroupForm(data=RasGroupForm.obj2dict(grp))
    return render_template('ras/group_delete.html', form=form, id=id)


@app.route('/rasclstr/<int:grpid>/rassrv/create')
def rassrv_create(grpid):
    form = RasServerForm(group=grpid)
    return render_template('ras/server_create.html', form=form, grpid=grpid)


@app.route('/rasclstr/<int:grpid>/rassrv/<int:srvid>/delete')
def rassrv_delete(grpid, srvid):
    srv = RasServer.get(RasServer.id == srvid)
    form = RasServerForm(obj=srv)
    return render_template(
        'ras/server_delete.html',
        form=form, grpid=grpid, srvid=srvid, group_name=srv.group.name)


@app.route('/rasclstr/<int:grpid>/rassrv/<int:srvid>', methods=['POST'])
def rassrv_detail(grpid, srvid):
    if request.form['operation'] == 'Delete':
        RasServerForm.delete(srvid)
    else:
        abort(400)
    return redirect(url_for('rasgrp_detail', id=grpid))


@app.route('/rasclstr/<int:grpid>/rassrv', methods=['POST'])
def rassrv_list(grpid):
    if request.method == 'POST':
        form = RasServerForm(request.form, group=grpid)
        if not form.validate():
            return render_template('ras/server_create.html',
                                   form=form, grpid=grpid)
        form.create(grpid)
        return redirect(url_for('rasgrp_detail', id=grpid))


_TARGET_FORMS = {
    'cell': RasTargetCellForm,
    'ras': RasTargetRasForm,
    'app': RasTargetAppForm,
}


@app.route('/rasclstr/<int:grpid>/rastgt/create/<typ>')
def rastgt_create(grpid, typ):
    form = _TARGET_FORMS[typ]()
    return render_template(
        'ras/target_create.html', form=form, grpid=grpid, typ=typ)


@app.route('/rasclstr/<int:grpid>/rastgt', methods=['POST'])
def rastgt_list(grpid):
    typ = request.form['type']
    form = _TARGET_FORMS[typ](request.form)
    if not form.validate():
        return render_template(
            'ras/target_create.html', form=form, grpid=grpid, typ=typ)
    form.create(grpid)
    return redirect(url_for('rasgrp_detail', id=grpid))


@app.route('/rasclstr/<int:grpid>/rastgt/delete/<typ>/<int:tgtid>')
def rastgt_delete(grpid, typ, tgtid):
    tgt = RasTarget.get(RasTarget.id == tgtid)
    fclass = _TARGET_FORMS[typ]
    form = fclass(data=fclass.obj2dict(tgt))
    return render_template(
        'ras/target_delete.html',
        form=form, grpid=grpid, typ=typ, tgtid=tgtid,
        group_name=tgt.group.name)


@app.route('/rasclstr/<int:grpid>/rastgt/<int:tgtid>')
def rastgt_detail(grpid, tgtid):
    tgt = RasTarget.get(RasTarget.id == tgtid)
    return render_template('ras/target_show.html', target=tgt)


@app.route('/rasclstr/<int:grpid>/rastgt/<typ>/<int:tgtid>', methods=['POST'])
def rastgt_detail_op(grpid, typ, tgtid):
    if request.form['operation'] == 'Delete':
        _TARGET_FORMS[typ].delete(tgtid)
    else:
        abort(400)
    return redirect(url_for('rasgrp_detail', id=grpid))


_ACTION_FORMS = {
    'smtp': RasActionMailForm,
    'syslog': RasActionSyslogForm,
    'http': RasActionHttpForm,
    'restart': RasActionRestartForm,
    'script': RasActionScriptForm,
}


@app.route('/rasclstr/<int:grpid>/rastgt/<int:tgtid>/rasact/create/<atype>')
def rasact_create(grpid, tgtid, atype):
    form = _ACTION_FORMS[atype]()
    tgt = RasTarget.get(RasTarget.id == tgtid)
    return render_template(
        'ras/action_create.html',
        form=form, grpid=grpid, target=tgt, atype=atype)


@app.route('/rasclstr/<int:grpid>/rastgt<int:tgtid>/rasact', methods=['POST'])
def rasact_list(grpid, tgtid):
    atype = request.form['type']
    form = _ACTION_FORMS[atype](request.form)
    if not form.validate():
        tgt = RasTarget.get(RasTarget.id == tgtid)
        return render_template(
            'ras/action_create.html',
            form=form, grpid=grpid, target=tgt, atype=atype)
    script = request.files['script'] if 'script' in request.files else None
    form.create(tgtid, script)
    return redirect(url_for(
        'rastgt_detail', grpid=grpid, tgtid=tgtid))


@app.route(
    '/rasclstr/<int:grpid>/rastgt/<int:tgtid>/rasact/delete/<typ>/<int:actid>')
def rasact_delete(grpid, tgtid, typ, actid):
    act = RasAction.get(RasAction.id == actid)
    logger.debug('ACTION: {} type={}'.format(str(act), typ))
    fclass = _ACTION_FORMS[typ]
    form = fclass(data=fclass.obj2dict(act))
    return render_template(
        'ras/action_delete.html',
        form=form, grpid=grpid, tgtid=tgtid, typ=typ, actid=actid,
        group_name=act.target.group.name, target_name=act.target.name)


@app.route(
    '/rasclstr/<int:grpid>/rastgt/<int:tgtid>/rasact/update/<typ>/<int:actid>')
def rasact_update(grpid, tgtid, typ, actid):
    act = RasAction.get(RasAction.id == actid)
    fclass = _ACTION_FORMS[typ]
    form = fclass(data=fclass.obj2dict(act))
    return render_template(
        'ras/action_update.html',
        form=form, grpid=grpid, tgtid=tgtid, typ=typ, actid=actid)


@app.route(
    '/rasclstr/<int:grpid>/rastgt/<int:tgtid>/rasact/<typ>/<int:actid>',
    methods=['POST'])
def rasact_detail(grpid, tgtid, typ, actid):
    if request.form['operation'] == 'Delete':
        _ACTION_FORMS[typ].delete(actid)
    elif request.form['operation'] == 'Update':
        fclass = _ACTION_FORMS[typ]
        form = fclass(request.form)
        act = RasAction.get(RasAction.id == actid)
        if not form.validate():
            return render_template(
                'ras/action_update.html',
                form=form, grpid=grpid, tgtid=tgtid, typ=typ, actid=actid)
        script = (request.files['script']
                  if 'script' in request.files else None)
        form.update(actid, tgtid, script)
    else:
        abort(400)
    return redirect(url_for(
        'rastgt_detail', grpid=grpid, tgtid=tgtid))
