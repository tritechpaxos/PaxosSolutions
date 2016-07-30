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
from flask import Response, make_response, request, jsonify
import blinker

from .. import app
from ..event import _generator, _make_sse_response
from cellconfig.agent import EventListener
from cellconfig.ras.model import RasGroup, RasTarget


logger = logging.getLogger(__name__)


@app.route('/event/ras')
def ras_event():
    listener = EventListener('ras:status')
    events = [
        {'name': 'status;{}'.format(ras.name), 'target': ras}
        for ras in RasGroup.select()]

    def setup(_callback):
        listener.regist(_callback)
        for ev in events:
            listener.enable(ev)

    def teardown(_callback):
        listener.unregist(_callback)
        for ev in events:
            listener.disable(ev)

    return _make_sse_response(_generator(setup, teardown))


@app.route('/event/ras/<int:id>')
def ras_clst_event(id):
    ras = RasGroup.get(RasGroup.id == id)
    params = []
    for op in ['status', 'server_status']:
        params.append(
            (EventListener('ras:{}'.format(op), source=ras.name),
             {'name': op, 'target': ras}))

    def setup(_callback):
        for (listener, event) in params:
            listener.regist(_callback)
            listener.enable(event)

    def teardown(_callback):
        for (listener, event) in params:
            listener.unregist(_callback)
            listener.disable(event)

    return _make_sse_response(_generator(setup, teardown))


@app.route('/event/RAS/<app>/<name>/<no>', methods=['PUT'])
def ras_report(app, name, no):
    from cellconfig.agents.ras.rasagent import RasEvent, CellEvent
    rsig = blinker.signal('ras:status')
    ssig = blinker.signal('ras:server_status')
    csig = blinker.signal('cell:status')
    tgts = RasTarget.select().where(RasTarget.path == '/'.join([app, name]))
    for tgt in tgts:
        RasEvent._send_signal(rsig, tgt)
        if tgt.type == 'cell':
            CellEvent._send_signal(csig, tgt.name, tgt)
        elif tgt.type == 'ras':
            ras = RasGroup.select().where(RasGroup.name == tgt.name).first()
            RasEvent._send_server_signal(ssig, tgt, ras)
    return make_response('ok')


@app.route('/event/RAS/<clst>/<app>/<name>')
def ras_action_info(clst, app, name):
    ras = RasGroup.select().where(RasGroup.name == clst).first()
    if ras is None:
        abort(404)
    actions = []
    tgts = RasTarget.select().where(
        (RasTarget.path == '/'.join([app, name])) &
        (RasTarget.group == ras.id))
    for tgt in tgts:
        for act in tgt.actions:
            aparams = {'name': act.name, 'mode': act.mode, 'action': act.type}
            aparams.update(json.loads(act.data))
            actions.append(aparams)
    return jsonify(actions)
