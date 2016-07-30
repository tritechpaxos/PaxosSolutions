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
from flask import Response
from gevent.queue import Queue
import gevent
import blinker

from . import app
from cellconfig.model import Cell
from cellconfig.agent import EventListener


logger = logging.getLogger(__name__)


def encode_sse(r):
    return 'data: {0}\n\n'.format(json.dumps(r))


def _generator(setup, teardown, keepalive=False):
    q = Queue()

    def _callback(sender, **kwargs):
        q.put(encode_sse(kwargs['message']))

    setup(_callback)
    if keepalive:
        def ping():
            while True:
                q.put(':ping \n\n')
                gevent.sleep(30)
        g = gevent.spawn(ping)
    else:
        g = None
    try:
        while True:
            data = q.get()
            yield data
    finally:
        logger.debug("TEARDOWN")
        teardown(_callback)
        if g is not None:
            gevent.kill(g)


def _make_sse_response(r):
    resp = Response(r, mimetype='text/event-stream')
    resp.headers['X-Accel-Buffering'] = 'no'
    return resp


@app.route('/event/cell')
def cell_list_event():
    listener = EventListener('cell:status')
    cells = Cell.select()
    events = [{'name': 'status;{}'.format(cell.name), 'target': cell}
              for cell in cells]

    def setup(_callback):
        listener.regist(_callback)
        for ev in events:
            listener.enable(ev)

    def teardown(_callback):
        listener.unregist(_callback)
        for ev in events:
            listener.disable(ev)

    return _make_sse_response(_generator(setup, teardown))


@app.route('/event/cell/<int:id>')
def cell_event(id):
    cell = Cell.get(Cell.id == id)
    logger.debug("REQUEST: {}".format(cell.name))
    listener = EventListener('cell:status_detail', source=cell.name)
    ev = {'name': 'status_detail', 'target': cell}

    def setup(_callback):
        listener.regist(_callback)
        listener.enable(ev)

    def teardown(_callback):
        listener.unregist(_callback)
        listener.disable(ev)

    return _make_sse_response(_generator(setup, teardown))


@app.route('/event/error')
def error_event():
    sig = blinker.signal('error')

    def setup(_callback):
        sig.connect(_callback)

    def teardown(_callback):
        sig.disconnect(_callback)

    return _make_sse_response(_generator(setup, teardown, keepalive=True))
