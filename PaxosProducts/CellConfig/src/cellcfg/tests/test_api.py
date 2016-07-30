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
from __future__ import absolute_import
import unittest
import json
import logging
from cellconfig import db
from cellconfig.model import Cell, Node, CellNode
from cellconfig.ui.web import app


logger = logging.getLogger(__name__)


class ApiTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        db.create_tables([Cell, Node, CellNode])

    @classmethod
    def tearDownClass(cls):
        db.drop_table(CellNode)
        db.drop_table(Node)
        db.drop_table(Cell)

    def setUp(self):
        self.app = app.test_client()

    def tearDown(self):
        CellNode.delete().execute()
        Cell.delete().execute()
        Node.delete().execute()

    def test_index(self):
        rv = self.app.get('/api')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 200)
        data = json.loads(rv.data)
        self.assertTrue('cell' in data)
        self.assertTrue('node' in data)

    def _get(self, path):
        rv = self.app.get(path)
        self.assertEqual(rv.mimetype, 'application/json')
        data = json.loads(rv.data)
        return (rv, data)

    def _post(self, path, data):
        jdata = json.dumps(data)
        rv = self.app.post(path, data=jdata, content_type='application/json')
        self.assertEqual(rv.mimetype, 'application/json')
        data = json.loads(rv.data)
        return (rv, data)

    def _put(self, path, data):
        jdata = json.dumps(data)
        rv = self.app.put(path, data=jdata, content_type='application/json')
        self.assertEqual(rv.mimetype, 'application/json')
        data = json.loads(rv.data)
        return (rv, data)

    def test_empty_node_list(self):
        rv, rdata = self._get('/api/node')
        self.assertEqual(rv.status_code, 200)
        self.assertTrue('node' in rdata)
        self.assertEqual(len(rdata['node']), 0)

    def _create_node(self, hostname):
        data = dict(hostname=hostname, user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('node' in rdata)
        self.assertTrue('id' in rdata['node'])
        return rdata['node']

    def test_node_create(self):
        self._create_node('192.168.1.1')

    def test_node_create_null_constraint(self):
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22)
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.paxos_home')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.port')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(hostname='192.168.1.1', user='paxos', port=22,
                    paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.password')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(hostname='192.168.1.1', password='paxos00', port=22,
                    paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.user')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(user='paxos', password='paxos00', port=22,
                    paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

    def test_node_create_illegal_request(self):
        data = []
        rv, rdata = self._post('/api/node', data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

        rv = self.app.post('/api/node')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 400)
        rdata = json.loads(rv.data)
        self.assertIsNone(rdata['data'])

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

    def test_node_create_illegal_param(self):
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=-1, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=65536, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='192.168.1.1', user=' ', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='   ')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port='abc', paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', id='abc')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)

    def test_node_create_same_name(self):
        self._create_node('192.168.1.1')

        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 409)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'UNIQUE constraint failed')

        data = dict(hostname=' 192.168.1.1 ', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 409)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'UNIQUE constraint failed')

    def test_node_create_with_id(self):
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', id=99)
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 201)
        self.assertEqual(rdata['node']['id'], data['id'])

        rv, rdata = self._get('/api/node')
        logger.debug(rv)
        logger.debug(rdata)
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['node']), 1)
        self.assertEqual(rdata['node'][0]['id'], 99)

    def test_node_update_by_post(self):
        node0 = self._create_node('192.168.1.1')
        data = dict(hostname='192.168.1.2', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', id=node0['id'])
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 200)
        rv, rdata = self._get('/api/node')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['node']), 1)

    def test_node_create_with_ext_data(self):
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', xxx='abc')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 400)

    def _assert_node_equals(self, node0, node1):
        for key in ['id', 'hostname', 'user', 'password', 'port',
                    'paxos_home']:
            self.assertEqual(node0[key], node1[key])

    def test_node_list(self):
        node0 = self._create_node('192.168.1.1')
        rv, rdata = self._get('/api/node')
        self.assertEqual(rv.status_code, 200)
        self.assertTrue('node' in rdata)
        node_list = rdata['node']
        self.assertEqual(len(node_list), 1)
        node1 = node_list[0]
        for key in ['id', 'hostname', 'user', 'password', 'port',
                    'paxos_home']:
            self.assertEqual(node0[key], node1[key])

    def test_node_list2(self):
        node0_list = [self._create_node('192.168.1.1'),
                      self._create_node('192.168.1.2')]
        node0_dict = dict([(n['id'], n) for n in node0_list])
        rv, rdata = self._get('/api/node')
        self.assertEqual(rv.status_code, 200)
        self.assertTrue('node' in rdata)
        node1_list = rdata['node']
        self.assertEqual(len(node1_list), 2)
        for node1 in node1_list:
            id = node1['id']
            self.assertTrue(id in node0_dict)
            node0 = node0_dict[id]
            self._assert_node_equals(node0, node1)

    def test_node_detail(self):
        node0_list = [self._create_node('192.168.1.1'),
                      self._create_node('192.168.1.2')]
        node0_dict = dict([(n['id'], n) for n in node0_list])
        for key, node0 in node0_dict.iteritems():
            rv, rdata = self._get('/api/node/' + str(key))
            self.assertEqual(rv.status_code, 200)
            node1 = rdata['node']
            self._assert_node_equals(node0, node1)

    def test_node_detail_not_exist_id(self):
        node0 = self._create_node('192.168.1.1')
        rv, rdata = self._get('/api/node/' + str(node0['id'] + 1))
        self.assertEqual(rv.status_code, 404)

    def test_node_delete(self):
        node0 = self._create_node('192.168.1.1')
        rv = self.app.delete('/api/node/' + str(node0['id']))
        self.assertEqual(rv.status_code, 204)

    def test_node_delete_not_exist_id(self):
        node0 = self._create_node('192.168.1.1')
        rv = self.app.delete('/api/node/' + str(node0['id'] + 1))
        self.assertEqual(rv.status_code, 204)

    def test_node_delete2(self):
        node0_list = [self._create_node('192.168.1.1'),
                      self._create_node('192.168.1.2')]
        rv = self.app.delete('/api/node/' + str(node0_list[0]['id']))
        self.assertEqual(rv.status_code, 204)
        rv, rdata = self._get('/api/node')
        self.assertEqual(rv.status_code, 200)
        node1_list = rdata['node']
        self.assertEqual(len(node1_list), 1)

    def test_node_update(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        data = dict(hostname='192.168.1.2', user='paxos2', password='paxos02',
                    port=23, paxos_home='/home/paxos2/PAXOS', id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals(data, node1)

    def _assert_node_equals3(self, old_value, update_value, new_value):
        for key in ['id', 'hostname', 'user', 'password', 'port',
                    'paxos_home']:
            if key in update_value:
                self.assertEqual(new_value[key], update_value[key])
            else:
                self.assertEqual(new_value[key], old_value[key])

    def test_node_update2(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        data = dict(hostname='192.168.1.2', id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals3(node0, data, node1)

        node0 = node1
        data = dict(user='paxos2', id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals3(node0, data, node1)

        node0 = node1
        data = dict(password='paxos02', id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals3(node0, data, node1)

        node0 = node1
        data = dict(port=23, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals3(node0, data, node1)

        node0 = node1
        data = dict(paxos_home='/home/paxos2/PAXOS', id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)
        node1 = rdata['node']
        self._assert_node_equals3(node0, data, node1)

    def test_node_update_null_constraint(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        data = dict(hostname=None, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(user=None, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.user')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(password=None, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.password')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(port=None, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.port')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        data = dict(paxos_home=None, id=id)
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'node.paxos_home')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

    def test_node_update_illegal_request(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        url = '/api/node/' + str(id)

        data = []
        rv, rdata = self._put(url, data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

        rv = self.app.put(url)
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 400)
        rdata = json.loads(rv.data)
        self.assertIsNone(rdata['data'])

        data = dict(hostname='192.168.1.2', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', id=id)
        rv, rdata = self._put(url, data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

    def test_node_update_illegal_param(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        url = '/api/node/' + str(id)

        data = dict(port=-1, id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(port=65536, id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(hostname='', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(user=' ', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(paxos_home='   ', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        data = dict(port='abc', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 400)

    def test_node_update_same_name(self):
        node0_list = [self._create_node('192.168.1.1'),
                      self._create_node('192.168.1.2')]
        id = node0_list[0]['id']
        url = '/api/node/' + str(id)

        data = dict(hostname='192.168.1.2', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 409)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'UNIQUE constraint failed')

        data = dict(hostname=' 192.168.1.2 ', id=id)
        rv, rdata = self._put(url, dict(node=data))
        self.assertEqual(rv.status_code, 409)
        self.assertEqual(rdata['column'], 'node.hostname')
        self.assertEqual(rdata['error'], 'UNIQUE constraint failed')

    def test_node_update_with_ext_data(self):
        node0 = self._create_node('192.168.1.1')
        id = node0['id']
        data = dict(id=id, xxx='abc')
        rv, rdata = self._put('/api/node/' + str(id), dict(node=data))
        self.assertEqual(rv.status_code, 200)

    def test_node_query_hostname(self):
        node0_list = [self._create_node('192.168.1.1'),
                      self._create_node('192.168.1.2')]

        rv, rdata = self._get('/api/node/name/192.168.1.1')
        self.assertEqual(rv.status_code, 200)
        self._assert_node_equals(node0_list[0], rdata['node'])

        rv, rdata = self._get('/api/node/name/192.168.1.2')
        self.assertEqual(rv.status_code, 200)
        self._assert_node_equals(node0_list[1], rdata['node'])

        rv, rdata = self._get('/api/node/name/192.168.1.3')
        self.assertEqual(rv.status_code, 404)

    def test_node_query_hostname2(self):
        node0 = self._create_node('127.0.0.1')
        rv, rdata = self._get('/api/node/name/localhost')
        self.assertEqual(rv.status_code, 200)
        self._assert_node_equals(node0, rdata['node'])

    def test_empty_cell_list(self):
        rv, rdata = self._get('/api/cell')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 0)

    def _create_cell(self, name, cell_type, nodes, comment='comment',
                     tcp_port=14000, udp_port=14000, tcp_addr_list=[],
                     udp_addr_list=[], **kwargs):
        data = dict(name=name, type=cell_type, comment=comment, nodes=[],
                    **kwargs)
        no = 0
        for node in nodes:
            if tcp_addr_list is None:
                tcp_addr = None
            elif no < len(tcp_addr_list):
                tcp_addr = tcp_addr_list[no]
            else:
                tcp_addr = node['hostname']
            if udp_addr_list is None:
                udp_addr = None
            elif no < len(udp_addr_list):
                udp_addr = udp_addr_list[no]
            else:
                udp_addr = node['hostname']
            cnode = dict(no=no, node=node['id'], tcp_addr=tcp_addr,
                         udp_addr=udp_addr, tcp_port=tcp_port,
                         udp_port=udp_port)
            no = no + 1
            data['nodes'].append(cnode)
        return self._post('/api/cell', dict(cell=data))

    def test_cell_create(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]

        rv, rdata = self._create_cell('cmdb', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('id' in rdata['cell'])

        rv, rdata = self._get('/api/cell')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 1)

        rv, rdata = self._create_cell('cmdb2', 'CMDB', node_list, comment=None)
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('id' in rdata['cell'])
        id = rdata['cell']['id']
        name = rdata['cell']['name']

        rv, rdata = self._get('/api/cell')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 2)

        rv, rdata = self._get('/api/cell/{}'.format(id))
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(rdata['cell']['name'], name)

        rv, rdata = self._get('/api/cell/name/{}'.format(name))
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(rdata['cell']['id'], id)
        self.assertEqual(rdata['cell']['name'], name)

    def test_cell_create_null_constraint(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]

        rv, rdata = self._create_cell(None, 'CMDB', node_list)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cell.name')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        rv, rdata = self._create_cell('cmdb0', None, node_list)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cell.type')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      tcp_port=None)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cellnode.tcp_port')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      udp_port=None)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cellnode.udp_port')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      tcp_addr_list=None)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cellnode.tcp_addr')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      udp_addr_list=None)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['column'], 'cellnode.udp_addr')
        self.assertEqual(rdata['error'], 'NOT NULL constraint failed')

    def test_cell_create_illegal_request(self):
        data = []
        rv, rdata = self._post('/api/cell', data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

        rv = self.app.post('/api/cell')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 400)
        rdata = json.loads(rv.data)
        self.assertIsNone(rdata['data'])

        data = dict(name='cmdb0', type='CMDB', comment=None, nodes=[])
        rv, rdata = self._post('/api/cell', data)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['data'], data)

    def test_cell_create_illegal_param(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]

        rv, rdata = self._create_cell(' ', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'XXX', node_list)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list, udp_port=-1)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      udp_port=65536)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list, tcp_port=-1)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      tcp_port=65536)
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      udp_addr_list=[' '])
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

        rv, rdata = self._create_cell('cmdb0', 'CMDB', node_list,
                                      tcp_addr_list=[' '])
        self.assertEqual(rv.status_code, 400)
        self.assertEqual(rdata['error'], 'CHECK constraint failed')

    def test_cell_create_same_name(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]

        rv, rdata = self._create_cell('aaa', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('id' in rdata['cell'])

        rv, rdata = self._create_cell('aaa', 'CSS', node_list)
        self.assertEqual(rv.status_code, 409)
        self.assertEqual(rdata['column'], 'cell.name')
        self.assertEqual(rdata['error'], 'UNIQUE constraint failed')

    def test_cell_create_with_id(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]
        rv, rdata = self._create_cell('aaa', 'CMDB', node_list, id=99)
        self.assertEqual(rv.status_code, 201)
        self.assertEqual(rdata['cell']['id'], 99)

        rv, rdata = self._get('/api/cell')
        logger.debug(rv)
        logger.debug(rdata)
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 1)
        self.assertEqual(rdata['cell'][0]['id'], 99)

    def test_cell_update_by_post(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]

        rv, rdata = self._create_cell('aaa', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        id = rdata['cell']['id']

        rv, rdata = self._get('/api/cell')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 1)

        rv, rdata = self._create_cell('bbb', 'CSS', node_list, id=id)
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(rdata['cell']['id'], id)
        rv, rdata = self._get('/api/cell')
        self.assertEqual(rv.status_code, 200)
        self.assertEqual(len(rdata['cell']), 1)

    def test_cell_create_with_ext_data(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]
        rv, rdata = self._create_cell('aaa', 'CMDB', node_list, xxx='abc')
        self.assertEqual(rv.status_code, 400)

    def test_cell_conf_file(self):
        node_list = [self._create_node('192.168.1.1'),
                     self._create_node('192.168.1.2'),
                     self._create_node('192.168.1.3')]
        rv, rdata = self._create_cell('aaa', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        id = rdata['cell']['id']

        headers = {'Accept': 'text/x.paxos-cell-conf'}
        rv = self.app.get('/api/cell/{}'.format(id), headers=headers)
        self.assertEqual(rv.status_code, 200)
        conf = ''
        conf += '0 192.168.1.1 14000 192.168.1.1 14000\n'
        conf += '1 192.168.1.2 14000 192.168.1.2 14000\n'
        conf += '2 192.168.1.3 14000 192.168.1.3 14000\n'
        self.assertEqual(rv.data, conf)

    def test_cell_node(self):
        nodes = ['192.168.1.1', '192.168.1.2', '192.168.1.3']
        node_list = [self._create_node(n) for n in nodes]
        rv, rdata = self._create_cell('cmdb', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('id' in rdata['cell'])
        id = rdata['cell']['id']

        for n in range(len(nodes)):
            rv, rdata = self._get('/api/cell/{}/node/{}'.format(id, n))
            self.assertEqual(rv.status_code, 200)
            logger.debug("DATA: {}".format(rdata))
            self.assertTrue(rdata['cell_node']['node']['hostname'], nodes[n])

    def test_cell_node_start(self):
        nodes = ['192.168.1.1', '192.168.1.2', '192.168.1.3']
        node_list = [self._create_node(n) for n in nodes]
        rv, rdata = self._create_cell('cmdb', 'CMDB', node_list)
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('id' in rdata['cell'])
        id = rdata['cell']['id']
        name = rdata['cell']['name']

        rv, rdata = self._put(
            '/api/cell/{}/node/{}/process'.format(id, 0),
            {'operation': 'start'})
        self.assertEqual(rv.status_code, 200)

        rv, rdata = self._put(
            '/api/cell/name/{}/node/{}/process'.format(name, 0),
            {'operation': 'start'})
        self.assertEqual(rv.status_code, 200)


if __name__ == '__main__':
    unittest.main()
