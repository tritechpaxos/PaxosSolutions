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


class ApiDbErrorTestCase(unittest.TestCase):

    def setUp(self):
        self.app = app.test_client()
        db.create_tables([Cell, Node, CellNode])

    def tearDown(self):
        db.drop_table(CellNode, fail_silently=True)
        db.drop_table(Node, fail_silently=True)
        db.drop_table(Cell, fail_silently=True)

    def test_node_list(self):
        db.drop_table(Node)
        rv = self.app.get('/api/node')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

    def test_node_detail(self):
        db.drop_table(Node)
        rv = self.app.get('/api/node/1')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

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

    def test_node_create(self):
        db.drop_table(Node)
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 500)
        self.assertTrue('error' in rdata)

    def test_node_delete(self):
        db.drop_table(Node)
        rv = self.app.delete('/api/node/1')
        self.assertEqual(rv.mimetype, 'application/json')
        rdata = json.loads(rv.data)
        self.assertEqual(rv.status_code, 500)
        self.assertTrue('error' in rdata)

    def test_node_update(self):
        db.drop_table(Node)
        data = dict(hostname='192.168.1.1', user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS', id=1)
        rv, rdata = self._put('/api/node/1', dict(node=data))
        self.assertEqual(rv.status_code, 500)
        self.assertTrue('error' in rdata)

    def test_cell_list(self):
        db.drop_table(Cell)
        rv = self.app.get('/api/cell')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

    def test_cell_detail(self):
        db.drop_table(Cell)
        rv = self.app.get('/api/cell/1')
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

    def test_cell_delete(self):
        db.drop_table(Cell)
        rv = self.app.delete('/api/cell/1')
        self.assertEqual(rv.mimetype, 'application/json')
        rdata = json.loads(rv.data)
        self.assertEqual(rv.status_code, 500)
        self.assertTrue('error' in rdata)

    def test_cell_create(self):
        db.drop_table(Cell)
        data = dict(name='cmdb0', type='CMDB', comment='comment', nodes=[])
        rv, rdata = self._post('/api/cell', dict(cell=data))
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

    def test_cell_update(self):
        db.drop_table(Cell)
        data = dict(name='cmdb0', type='CMDB', comment='comment', nodes=[])
        rv, rdata = self._put('/api/cell/1', dict(cell=data))
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)

    def _create_node(self, hostname):
        data = dict(hostname=hostname, user='paxos', password='paxos00',
                    port=22, paxos_home='/home/paxos/PAXOS')
        rv, rdata = self._post('/api/node', dict(node=data))
        self.assertEqual(rv.status_code, 201)
        self.assertTrue('node' in rdata)
        self.assertTrue('id' in rdata['node'])
        return rdata['node']

    def test_cell_create2(self):
        db.drop_table(CellNode)
        nodes = [self._create_node('192.168.1.1'),
                 self._create_node('192.168.1.2'),
                 self._create_node('192.168.1.3')]
        data = dict(name='cmdb0', type='CMDB', comment='comment', nodes=[])
        no = 0
        for node in nodes:
            cnode = dict(no=no, node=node['id'], tcp_addr=node['hostname'],
                         udp_addr=node['hostname'], tcp_port=14000,
                         udp_port=14000)
            no = no + 1
            data['nodes'].append(cnode)

        rv, rdata = self._post('/api/cell', dict(cell=data))
        self.assertEqual(rv.mimetype, 'application/json')
        self.assertEqual(rv.status_code, 500)
        rdata = json.loads(rv.data)
        self.assertTrue('error' in rdata)


if __name__ == '__main__':
    unittest.main()
