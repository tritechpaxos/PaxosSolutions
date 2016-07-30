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
import json
from flask import request, url_for, abort, jsonify, make_response
from .. import app
from cellconfig.ras.model import RasServer, RasGroup
from cellconfig.ras.model_admin import SmtpConf


logger = logging.getLogger(__name__)


@app.route('/api/ras/<int:id>/server/<int:no>/process', methods=['PUT'])
def api_ras_srv_state(id, no):
    logger.debug("SRV STATE: {}, {}".format(id, no))
    srv = RasServer.select().where(
        (RasServer.no == no) & (RasServer.group == id)).first()
    if srv is None:
        logger.debug("SRV 404")
        abort(404)
    if request.method == 'PUT':
        data = request.get_json()
        logger.debug("SRV DATE: {}".format(data))
        if data['operation'] == 'start':
            logger.debug("SRV START")
            g = srv.start_with_params(data)

            def set_event(x):
                logger.debug("SET_EVENT")
                for tgt in srv.group.targets:
                    tgt.set_event(nodes=[srv])
            g.link_value(set_event)
            return 'OK', 200
        elif data['operation'] == 'stop':
            logger.debug("SRV STOP")
            srv.stop()
            return 'OK', 200


@app.route('/api/ras/name/<name>/server/<int:no>/process', methods=['PUT'])
def api_ras_srv_state_by_name(name, no):
    grp = RasGroup.select().where(RasGroup.name == name).first()
    if grp is None:
        logger.debug("SRV 404")
        abort(404)
    return api_ras_srv_state(grp.id, no)


@app.route('/api/conf/smtp')
def api_conf_smtp():
    smtp = json.loads(str(SmtpConf()))
    return jsonify(smtp)
