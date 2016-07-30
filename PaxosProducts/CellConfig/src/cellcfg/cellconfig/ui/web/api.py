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

import socket
import StringIO
import logging
import gevent

from flask import request, url_for, abort, jsonify, make_response
from playhouse.shortcuts import model_to_dict, dict_to_model
import peewee

from . import app, database
from cellconfig import use_dns
from cellconfig.model import Node, Cell, CellNode
from cellconfig.error import DeployError


logger = logging.getLogger(__name__)


def _error_handler(status_code, message):
    path = request.path.split('/')
    if len(path) > 0 and path[1] == 'api':
        resp = jsonify(dict(status=status_code, error=message))
        resp.status_code = status_code
        return resp
    else:
        return message, status_code


@app.errorhandler(peewee.OperationalError)
def operational_error_handler(error):
    return _error_handler(500, error.message)


@app.errorhandler(peewee.DoesNotExist)
def operational_error_handler(error):
    return _error_handler(404, 'Not Found')


@app.errorhandler(peewee.IntegrityError)
def integrity_error_handler(error):
    err, col = [v.strip() for v in error.message.split(':')]
    resp_status = 400
    if err == 'UNIQUE constraint failed':
        resp_status = 409
    path = request.path.split('/')
    if len(path) > 0 and path[1] == 'api':
        resp = jsonify(dict(status=resp_status, error=err, column=col))
        resp.status_code = resp_status
        return resp
    else:
        return _error_handler(resp_status, error.message)


@app.errorhandler(404)
def not_found_handler(error):
    return _error_handler(404, str(error))


@app.errorhandler(405)
def method_not_allow_handler(error):
    return _error_handler(405, str(error))


def _update_model(cls, inst, kv):
    meta = cls._meta
    for k, v in kv.iteritems():
        if k == 'id':
            continue
        elif k in meta.fields:
            setattr(inst, meta.fields[k].name, v)


def make_hostname_list(hostname):
    ret = [hostname]
    if use_dns():
        try:
            ret.append(socket.getfqdn(hostname))
            tp = socket.gethostbyname_ex(hostname)
            ret.extend(tp[1])
            ret.extend(tp[2])
        except socket.gaierror:
            pass
    return ret


def find_node_by_name(hostname):
    for hname in make_hostname_list(hostname):
        q = Node.select().where(Node.hostname == hname)
        if q.count() > 0:
            return q.first()
    return None


@app.route('/api')
def api_index():
    data = dict(
        node=url_for('api_node_list', _external=True),
        cell=url_for('api_cell_list', _external=True),
    )
    return jsonify(data)


def _exists_node(id):
    try:
        node = Node.get(Node.id == id)
    except Node.DoesNotExist:
        return False
    return True


def api_node_create():
    jdata = request.get_json()
    ret_status = 201
    try:
        node = dict_to_model(Node, jdata['node'])
        with database.database.atomic():
            if node.id is None:
                node.save()
            else:
                if not _exists_node(node.id):
                    node.save(force_insert=True)
                else:
                    ret_status = 200
                    node.save()
        resp = jsonify(dict(node=model_to_dict(node)))
        resp.status_code = ret_status
        resp.headers['Location'] = url_for('api_node_detail', id=node.id)
        return resp
    except (AttributeError, ValueError, TypeError, KeyError) as e:
        resp = jsonify(dict(status=400, error=e.message, data=jdata))
        resp.status_code = 400
        return resp


def _api_node_list():
    query = Node.select().dicts()
    node_list = []
    for node in query:
        node_list.append(node)
        node['url'] = url_for('api_node_detail', id=node['id'],
                              _external=True)
    return jsonify(node=node_list)


@app.route('/api/node', methods=['GET', 'POST'])
def api_node_list():
    if request.method == 'GET':
        return _api_node_list()
    elif request.method == 'POST':
        return api_node_create()


def api_node_update(id):
    jdata = request.get_json()
    try:
        node = Node.get(Node.id == id)
        params = jdata['node']
        _update_model(Node, node, params)
        node.save()
        return jsonify(dict(node=model_to_dict(node)))
    except (AttributeError, ValueError, TypeError, KeyError) as e:
        resp = jsonify(dict(status=400, error=e.message, data=jdata))
        resp.status_code = 400
        return resp


def api_node_show(id):
    node = Node.select().where(Node.id == id).dicts().first()
    if node is not None:
        return jsonify(node=node)
    else:
        resp = jsonify(dict(status=404, error='Not Found', id=id))
        resp.status_code = 404
        return resp


@app.route('/api/node/<int:id>', methods=['GET', 'PUT', 'DELETE'])
def api_node_detail(id):
    if request.method == 'GET':
        return api_node_show(id)
    elif request.method == 'PUT':
        return api_node_update(id)
    elif request.method == 'DELETE':
        Node.delete().where(Node.id == id).execute()
        return make_response('', 204)


@app.route('/api/node/name/<name>', methods=['GET', 'PUT', 'DELETE'])
def api_node_detail_by_name(name):
    node = find_node_by_name(name)
    if node is None:
        abort(404)
    id = node.id
    return api_node_detail(id)


@app.route('/api/node/<int:id>/deployment', methods=['PUT'])
def api_node_deploy(id):
    node = Node.get(Node.id == id)
    try:
        node.deploy_async(force=True)
    except DeployError as e:
        resp = jsonify(dict(message=str(e)))
        resp.status_code = 500
        abort(resp)
    val = model_to_dict(node)
    val['label'] = str(node)
    val['os_str'] = node.os_str()
    return jsonify(dict(node=val))


def _cell_to_dict(cell):
    kv = model_to_dict(cell)
    kv['nodes'] = []
    for cnode in cell.nodes:
        cn_kv = model_to_dict(cnode)
        cn_kv['node'] = cn_kv['node']['id']
        del cn_kv['cell']
        kv['nodes'].append(cn_kv)
    return kv


def query_cell_all_list():
    query = Cell.select().dicts()
    cell_list = []
    for cell in query:
        cell_list.append(cell)
        cell['url'] = url_for('api_cell_detail', id=cell['id'],
                              _external=True)
    return jsonify(cell=cell_list)


def _exists_cell(id):
    try:
        cell = Cell.get(Cell.id == id)
    except Cell.DoesNotExist:
        return False
    return True


def api_cell_create():
    jdata = request.get_json()
    ret_status = 201
    try:
        with database.database.atomic():
            jcell = jdata['cell']
            cell = dict_to_model(Cell, jcell)
            if cell.id is None:
                cell.save()
            else:
                if not _exists_cell(cell.id):
                    cell.save(force_insert=True)
                else:
                    ret_status = 200
                    cell.save()
            jcell['id'] = cell.id
            for jcnode in jcell['nodes']:
                jcnode['cell'] = cell.id
                cnode = dict_to_model(CellNode, jcnode)
                cnode.save()
                jcnode['id'] = cnode.id
            resp = jsonify(cell=_cell_to_dict(cell))
            resp.status_code = ret_status
            resp.headers['Location'] = url_for('api_cell_detail', id=cell.id)
            return resp
    except (AttributeError, ValueError, TypeError, KeyError) as e:
        resp = jsonify(dict(status=400, error=e.message, data=jdata))
        resp.status_code = 400
        return resp


@app.route('/api/cell', methods=['GET', 'POST'])
def api_cell_list():
    if request.method == 'GET':
        return query_cell_all_list()
    elif request.method == 'POST':
        return api_cell_create()


def cell_get(cell):
    if 'text/x.paxos-cell-conf' in request.headers.get('Accept', ''):
        io = StringIO.StringIO()
        cell._generate_conf(io)
        conf = io.getvalue()
        io.close()
        resp = make_response(conf)
        resp.headers['Content-Type'] = 'text/x.paxos-cell-conf; charset=utf-8'
        return resp
    else:
        try:
            return jsonify(cell=_cell_to_dict(cell))
        except:
            abort(404)


def api_cell_get(id):
    cell = Cell.get(Cell.id == id)
    return cell_get(cell)


def api_cell_update(id, jcell):
    with database.database.atomic():
        cell = Cell.get(Cell.id == id)
        jnodes = jcell['nodes']
        del jcell['nodes']
        _update_model(Cell, cell, jcell)
        cell.save()
        cnids = [jcnode['id'] for jcnode in jnodes if 'id' in jcnode]
        for cnode in cell.nodes:
            if cnode.id not in cnids:
                cnode.delete_instance()
        for jcnode in jnodes:
            if 'id' in jcnode:
                cnode = CellNode.get(CellNode.id == jcnode['id'])
                _update_model(CellNode, cnode, jcnode)
            else:
                jcnode['cell'] = cell.id
                cnode = dict_to_model(CellNode, jcnode)
            cnode.save()
        resp = jsonify(cell=_cell_to_dict(cell))
        return resp


def api_cell_op(id, op, params=None):
    cell = Cell.get(Cell.id == id)
    if op == 'start':
        cell.start(params)
        return make_response('', 204)
    elif op == 'stop':
        cell.stop(params)
        return make_response('', 204)
    elif op == 'deploy config':
        cell.deploy_conf()
        return make_response('', 204)
    elif op == 'deploy core':
        for cnode in cell.nodes:
            cnode.node.deploy()
        return make_response('', 204)
    else:
        abort(400, 'bad operation "{0}"'.format(op))


@app.route('/api/cell/<int:id>', methods=['GET', 'PUT', 'DELETE'])
def api_cell_detail(id):
    if request.method == 'GET':
        return api_cell_get(id)
    elif request.method == 'DELETE':
        with database.database.atomic():
            cell = Cell.get(Cell.id == id)
            cell.delete_instance(recursive=True)
        return make_response('', 204)
    elif request.method == 'PUT':
        params = request.get_json()
        logger.debug("PUT: {}".format(params))
        if 'op' in params:
            return api_cell_op(id, params['op'], params)
        elif 'cell' in params:
            return api_cell_update(id, params['cell'])
        else:
            abort(400)
    else:
        abort(405)


@app.route('/api/cell/name/<name>', methods=['GET', 'PUT', 'DELETE'])
def api_cell_detail_by_name(name):
    query = Cell.select().where(Cell.name == name)
    try:
        id = query[0].id
    except:
        abort(404)
    return api_cell_detail(id)


@app.route('/api/cell/<int:cid>/node/<int:no>')
def api_cnode_detail(cid, no):
    cnode = CellNode.select().where(
        (CellNode.no == no) & (CellNode.cell == cid)).dicts().first()
    if cnode is not None:
        node = Node.select().where(Node.id == cnode['node']).dicts().first()
        cnode['node'] = node
        return jsonify(cell_node=cnode)
    else:
        abort(404)


@app.route('/api/cell/<int:cid>/node/<int:no>/process', methods=['GET', 'PUT'])
def api_cnode_state(cid, no):
    cnode = CellNode.select().where(
        (CellNode.no == no) & (CellNode.cell == cid)).first()
    if cnode is None:
        abort(404)

    if request.method == 'GET':
        return jsonify(cell_node=cnode.get_current_status())
    if request.method == 'PUT':
        params = request.get_json()
        op = params['operation']
        if op == 'start':
            cnode.start(params)
            return jsonify(cell_node=cnode.get_current_status())
        elif op == 'stop':
            cnode.stop(params)
            return jsonify(cell_node=cnode.get_current_status())
        else:
            abort(400)


@app.route('/api/cell/name/<name>/node/<int:no>/process',
           methods=['GET', 'PUT'])
def api_cnode_state_by_name(name, no):
    query = Cell.select().where(Cell.name == name)
    try:
        id = query[0].id
    except:
        abort(404)
    return api_cnode_state(id, no)


@app.route('/api/cell/<int:cid>/misc/<string:dtype>')
def api_cell_misc_data(cid, dtype):
    cell = Cell.get(Cell.id == cid)
    g = [node.get_misc_info_async(dtype) for node in cell.nodes]
    gevent.joinall(g)
    ret = []
    for x in g:
        ret.extend(x.get())
    logger.debug("RET: {}".format(ret))
    return jsonify(ret)
