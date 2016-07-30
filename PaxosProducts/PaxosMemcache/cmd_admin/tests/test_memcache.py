###########################################################################
#   PaxosMemcache Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of PaxosMemcache.
#
#   PaxosMemcache is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   PaxosMemcache is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with PaxosMemcache.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import os.path
import yaml
import StringIO
import sys
import unittest

from paxos.cmd.pmemcache import config, css, memcache, error
from tests import utils


class TestMemcacheRegister(utils._PaxosTest):

    CSS1 = 'css1'
    CSSX = 'cssX'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.css = css.Css.get(self.CSS1)

    def test_default(self):
        self.execute('memcache', 'register', '-c', self.CSS1)
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_id(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-i', '100')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(100)
        self.assertEqual(m0.id, 100)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_id2(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-i', '100')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        self.execute('memcache', 'register', '-c', self.CSS1)
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 2)
        m0 = memcache.Memcache.get(101)
        self.assertEqual(m0.id, 101)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_namespace(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-N', 'space1')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        self.execute('memcache', 'register', '-c', self.CSS1)
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 2)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertEqual(m0.name_space, 'space1')
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        m1 = memcache.Memcache.get(1)
        self.assertEqual(m1.id, 1)
        self.assertEqual(m1.name_space, 'space0')

    def test_max_cache_item(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-I', '1000')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertEqual(m0.max_cache_item, 1000)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_disable_checksum(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-s')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertFalse(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_enable_checksum(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-S')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertTrue(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_checksum_interval(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-C', '100')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertIsNone(m0.checksum)
        self.assertEqual(m0.checksum_interval, 100)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_expire_interval(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-E', '100')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertEqual(m0.expire_interval, 100)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_worker(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-W', '10')
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 1)
        m0 = memcache.Memcache.get(0)
        self.assertEqual(m0.id, 0)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertEqual(m0.worker, 10)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_err_dup_id(self):
        self.execute('memcache', 'register', '-c', self.CSS1, '-i', '100')
        m0 = memcache.Memcache.get(100)
        self.assertEqual(m0.id, 100)
        with self.assertRaises(error.DuplicationError):
            self.execute('memcache', 'register', '-c', self.CSS1, '-i', '100')

    def test_err_bad_id(self):
        self.output = StringIO.StringIO()
        sys.stderr = self.output
        with self.assertRaises(SystemExit):
            self.execute('memcache', 'register', '-c', self.CSS1, '-i', 'xxx')
        sys.stderr = sys.__stderr__

    def test_err_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'register', '-c', self.CSSX)


class TestMemcacheUnregister(utils._PaxosTest):

    CSS1 = 'css1'
    CSSX = 'cssX'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.css = css.Css.get(self.CSS1)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache = memcache.Memcache.get(0)

    def test_success(self):
        self.execute('memcache', 'unregister', '-i', str(self.memcache.id))
        result = memcache.Memcache.find()
        self.assertEqual(len(result), 0)

    def test_error_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'unregister', '-i', '100')


class TestMemcacheModify(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    CSSX = 'cssX'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.execute(
            'css', 'register', '-n', self.CSS2, 'vv-vm4:14000', 'vv-vm4:14001',
            'vv-vm5:15000', 'vv-vm5:15001', 'vv-vm6:16000', 'vv-vm6:16001')
        self.css = css.Css.get(self.CSS1)
        self.css2 = css.Css.get(self.CSS2)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache = memcache.Memcache.get(0)

    def test_name_space(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-N', 'space1')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space1')

    def test_max_cache_item(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-I', '1000')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertEqual(m0.max_cache_item, 1000)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_disable_checksum(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id), '-s')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertFalse(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_enable_checksum(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id), '-s')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertFalse(m0.checksum)
        self.execute('memcache', 'modify', '-i', str(self.memcache.id), '-S')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertTrue(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_checksum_interval(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-C', '100')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertEqual(m0.checksum_interval, 100)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_expire_interval(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-E', '100')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertEqual(m0.expire_interval, 100)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_worker(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-W', '10')
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertEqual(m0.worker, 10)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css)
        self.assertEqual(m0.name_space, 'space0')

    def test_css(self):
        self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                     '-c', self.CSS2)
        m0 = memcache.Memcache.get(self.memcache.id)
        self.assertIsNone(m0.checksum)
        self.assertIsNone(m0.checksum_interval)
        self.assertIsNone(m0.expire_interval)
        self.assertIsNone(m0.max_cache_item)
        self.assertIsNone(m0.worker)
        self.assertEqual(len(m0.pair), 0)
        self.assertEqual(m0.css, self.css2)
        self.assertEqual(m0.name_space, 'space0')

    def test_err_not_found_css(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'modify', '-i', str(self.memcache.id),
                         '-c', self.CSSX)

    def test_err_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute('memcache', 'modify', '-i', '999', '-c', self.CSS2)


class TestMemcachePairAdd(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.execute(
            'css', 'register', '-n', self.CSS2, 'vv-vm4:14000', 'vv-vm4:14001',
            'vv-vm5:15000', 'vv-vm5:15001', 'vv-vm6:16000', 'vv-vm6:16001')
        self.css1 = css.Css.get(self.CSS1)
        self.css2 = css.Css.get(self.CSS2)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache1 = memcache.Memcache.get(0)
        self.execute('memcache', 'register', '-c', self.CSS2, '-I', '1000',
                     '-s', '-C', '100', '-E', '100', '-W', '10')
        self.memcache2 = memcache.Memcache.get(1)

    def test_default(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm1')
        self.assertIsNone(mc1.pair[0].sport)
        self.assertIsNone(mc1.pair[0].chost)
        self.assertIsNone(mc1.pair[0].cport)

    def test_client_address(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-A', 'vv-vm0')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm1')
        self.assertIsNone(mc1.pair[0].sport)
        self.assertEqual(mc1.pair[0].chost, 'vv-vm0')
        self.assertIsNone(mc1.pair[0].cport)

    def test_server_port(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-p', '11111')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm1')
        self.assertEqual(mc1.pair[0].sport, 11111)
        self.assertIsNone(mc1.pair[0].chost)
        self.assertIsNone(mc1.pair[0].cport)

    def test_client_port(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-P', '11111')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm1')
        self.assertIsNone(mc1.pair[0].sport)
        self.assertIsNone(mc1.pair[0].chost)
        self.assertEqual(mc1.pair[0].cport, 11111)

    def test_add2(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm1')
        self.assertIsNone(mc1.pair[0].sport)
        self.assertIsNone(mc1.pair[0].chost)
        self.assertIsNone(mc1.pair[0].cport)
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm2', '-P', '11111')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 2)
        self.assertEqual(mc1.pair[1].shost, 'vv-vm2')
        self.assertIsNone(mc1.pair[1].sport)
        self.assertIsNone(mc1.pair[1].chost)
        self.assertEqual(mc1.pair[1].cport, 11111)

    def test_err_not_found_id(self):
        with self.assertRaises(error.NotFoundError):
            self.execute_raw('register', 'pair', '-i', '99', '-a', 'vv-vm1')

    def test_err_not_found_id(self):
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1')
        with self.assertRaises(error.DuplicationError):
            self.execute_raw('register', 'pair',
                             '-i', str(self.memcache1.id), '-a', 'vv-vm2')
        with self.assertRaises(error.DuplicationError):
            self.execute_raw('register', 'pair',
                             '-i', str(self.memcache1.id), '-a', 'vv-vm1',
                             '-A', 'vv-vm0', '-P', '11111')


class TestMemcachePairRemove(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.execute(
            'css', 'register', '-n', self.CSS2, 'vv-vm4:14000', 'vv-vm4:14001',
            'vv-vm5:15000', 'vv-vm5:15001', 'vv-vm6:16000', 'vv-vm6:16001')
        self.css1 = css.Css.get(self.CSS1)
        self.css2 = css.Css.get(self.CSS2)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache1 = memcache.Memcache.get(0)
        self.execute('memcache', 'register', '-c', self.CSS2, '-I', '1000',
                     '-s', '-C', '100', '-E', '100', '-W', '10')
        self.memcache2 = memcache.Memcache.get(1)
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1')
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm2', '-p', '11112',
                         '-A', 'vv-vm0', '-P', '11111')

    def test_default(self):
        self.execute_raw('unregister', 'pair',
                         '-i', str(self.memcache1.id), '-a', 'vv-vm1')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm2')

    def test_client_address(self):
        self.execute_raw('unregister', 'pair',
                         '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-A', '127.0.0.1')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm2')

    def test_server_port(self):
        self.execute_raw('unregister', 'pair',
                         '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-p', '11211')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm2')

    def test_client_port(self):
        self.execute_raw('unregister', 'pair',
                         '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1', '-P', '11211')
        mc1 = memcache.Memcache.get(self.memcache1.id)
        self.assertEqual(len(mc1.pair), 1)
        self.assertEqual(mc1.pair[0].shost, 'vv-vm2')

    def test_err_not_found_id(self):
        with self.assertRaises(error.NotFoundError):
            self.execute_raw('unregister', 'pair', '-i', '999',
                             '-a', 'vv-vm1')

    def test_err_not_found(self):
        with self.assertRaises(error.NotFoundError):
            self.execute_raw('unregister', 'pair',
                             '-i', str(self.memcache1.id), '-a', 'vv-vm2')


class TestMemcacheShow(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'
    SPACE0 = 'space0'
    SPACE1 = 'space1'

    def setUp(self):
        self.clear_local_conf_dir()
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.execute(
            'css', 'register', '-n', self.CSS2, 'vv-vm4:14000', 'vv-vm4:14001',
            'vv-vm5:15000', 'vv-vm5:15001', 'vv-vm6:16000', 'vv-vm6:16001')
        self.css1 = css.Css.get(self.CSS1)
        self.css2 = css.Css.get(self.CSS2)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache1 = memcache.Memcache.get(0)
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm1')
        self.execute_raw('register', 'pair', '-i', str(self.memcache1.id),
                         '-a', 'vv-vm2', '-p', '11111',
                         '-A', 'vv-vm0', '-P', '22222')
        self.execute('memcache', 'register', '-c', self.CSS2, '-I', '1000',
                     '-s', '-C', '100', '-E', '100', '-W', '10',
                     '-N', self.SPACE1)
        self.memcache2 = memcache.Memcache.get(1)
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def tearDown(self):
        sys.stdout = sys.__stdout__

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return [x for x in yaml.load_all(self.output.getvalue())]

    def test_default(self):
        result = self.exec_and_result('memcache', 'show')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], self.CSS1)
        self.assertEqual(result[0]['name space'], self.SPACE0)
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2], 'vv-vm1')
        self.assertEqual(result[0]['pair'][0][3], 11211)
        self.assertEqual(result[0]['pair'][1][0], 'vv-vm0')
        self.assertEqual(result[0]['pair'][1][1], 22222)
        self.assertEqual(result[0]['pair'][1][2], 'vv-vm2')
        self.assertEqual(result[0]['pair'][1][3], 11111)
        self.assertEqual(len(result[0].keys()), 4)
        self.assertEqual(result[1]['id'], 1)
        self.assertEqual(result[1]['css'], self.CSS2)
        self.assertFalse(result[1]['checksum'])
        self.assertEqual(result[1]['max cache item'], 1000)
        self.assertEqual(result[1]['checksum interval'], 100)
        self.assertEqual(result[1]['expire interval'], 100)
        self.assertEqual(result[1]['worker'], 10)
        self.assertEqual(result[1]['name space'], self.SPACE1)

    def test_css1(self):
        result = self.exec_and_result('memcache', 'show', '-c', self.CSS1)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], self.CSS1)
        self.assertEqual(result[0]['name space'], self.SPACE0)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2], 'vv-vm1')
        self.assertEqual(result[0]['pair'][0][3], 11211)
        self.assertEqual(result[0]['pair'][1][0], 'vv-vm0')
        self.assertEqual(result[0]['pair'][1][1], 22222)
        self.assertEqual(result[0]['pair'][1][2], 'vv-vm2')
        self.assertEqual(result[0]['pair'][1][3], 11111)
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(len(result[0].keys()), 4)

    def test_css2(self):
        result = self.exec_and_result('memcache', 'show', '-c', self.CSS2)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 1)
        self.assertEqual(result[0]['css'], self.CSS2)
        self.assertFalse(result[0]['checksum'])
        self.assertEqual(result[0]['max cache item'], 1000)
        self.assertEqual(result[0]['checksum interval'], 100)
        self.assertEqual(result[0]['expire interval'], 100)
        self.assertEqual(result[0]['worker'], 10)
        self.assertEqual(result[0]['name space'], self.SPACE1)

    def test_cssX(self):
        result = self.exec_and_result('memcache', 'show', '-c', 'xxx')
        self.assertEqual(len(result), 0)

    def test_id0(self):
        result = self.exec_and_result('memcache', 'show', '-i', '0')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], self.CSS1)
        self.assertEqual(result[0]['name space'], self.SPACE0)
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2], 'vv-vm1')
        self.assertEqual(result[0]['pair'][0][3], 11211)
        self.assertEqual(result[0]['pair'][1][0], 'vv-vm0')
        self.assertEqual(result[0]['pair'][1][1], 22222)
        self.assertEqual(result[0]['pair'][1][2], 'vv-vm2')
        self.assertEqual(result[0]['pair'][1][3], 11111)
        self.assertEqual(len(result[0].keys()), 4)

    def test_id1(self):
        result = self.exec_and_result('memcache', 'show', '-i', '1')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 1)
        self.assertEqual(result[0]['css'], self.CSS2)
        self.assertFalse(result[0]['checksum'])
        self.assertEqual(result[0]['max cache item'], 1000)
        self.assertEqual(result[0]['checksum interval'], 100)
        self.assertEqual(result[0]['expire interval'], 100)
        self.assertEqual(result[0]['worker'], 10)
        self.assertEqual(result[0]['name space'], self.SPACE1)

    def test_ns0(self):
        result = self.exec_and_result('memcache', 'show', '-N', self.SPACE0)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 0)
        self.assertEqual(result[0]['css'], self.CSS1)
        self.assertEqual(result[0]['name space'], self.SPACE0)
        self.assertEqual(len(result[0]['pair']), 2)
        self.assertEqual(result[0]['pair'][0][0], '127.0.0.1')
        self.assertEqual(result[0]['pair'][0][1], 11211)
        self.assertEqual(result[0]['pair'][0][2], 'vv-vm1')
        self.assertEqual(result[0]['pair'][0][3], 11211)
        self.assertEqual(result[0]['pair'][1][0], 'vv-vm0')
        self.assertEqual(result[0]['pair'][1][1], 22222)
        self.assertEqual(result[0]['pair'][1][2], 'vv-vm2')
        self.assertEqual(result[0]['pair'][1][3], 11111)
        self.assertEqual(len(result[0].keys()), 4)

    def test_ns1(self):
        result = self.exec_and_result('memcache', 'show', '-N', self.SPACE1)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['id'], 1)
        self.assertEqual(result[0]['css'], self.CSS2)
        self.assertFalse(result[0]['checksum'])
        self.assertEqual(result[0]['max cache item'], 1000)
        self.assertEqual(result[0]['checksum interval'], 100)
        self.assertEqual(result[0]['expire interval'], 100)
        self.assertEqual(result[0]['worker'], 10)
        self.assertEqual(result[0]['name space'], self.SPACE1)

    def test_idX(self):
        result = self.exec_and_result('memcache', 'show', '-i', '999')
        self.assertEqual(len(result), 0)


class TestMemcacheList(utils._PaxosTest):

    CSS1 = 'css1'
    CSS2 = 'css2'

    def setUp(self):
        self.clear_local_conf_dir()
        self.output = StringIO.StringIO()
        sys.stdout = self.output

    def _regist_memcache(self):
        self.execute(
            'css', 'register', '-n', self.CSS1, 'vv-vm1:14000', 'vv-vm1:14001',
            'vv-vm2:15000', 'vv-vm2:15001', 'vv-vm3:16000', 'vv-vm3:16001')
        self.execute(
            'css', 'register', '-n', self.CSS2, 'vv-vm4:14000', 'vv-vm4:14001',
            'vv-vm5:15000', 'vv-vm5:15001', 'vv-vm6:16000', 'vv-vm6:16001')
        self.css1 = css.Css.get(self.CSS1)
        self.css2 = css.Css.get(self.CSS2)
        self.execute('memcache', 'register', '-c', self.CSS1)
        self.memcache1 = memcache.Memcache.get(0)
        self.execute('memcache', 'register', '-c', self.CSS2, '-I', '1000',
                     '-s', '-C', '100', '-E', '100', '-W', '10')
        self.memcache2 = memcache.Memcache.get(1)

    def tearDown(self):
        sys.stdout = sys.__stdout__

    def exec_and_result(self, *cmd):
        self.execute(*cmd)
        return self.output.getvalue().splitlines()

    def test_default(self):
        self._regist_memcache()
        result = self.exec_and_result('memcache', 'list')
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0], '0')
        self.assertEqual(result[1], '1')

    def test_empty(self):
        result = self.exec_and_result('memcache', 'list')
        self.assertEqual(len(result), 0)
