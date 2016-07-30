###########################################################################
#   cmdbpeewee Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of cmdbpeewee.
#
#   Cmdbpeewee is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cmdbpeewee is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with cmdbpeewee.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import peewee
import cmdbpeewee
import playhouse.db_url
import unittest
import tests
import threading


db0 = playhouse.db_url.connect('cmdb:///cmdb3')
db1 = playhouse.db_url.connect('cmdb:///cmdb3')


class Ee0a(peewee.Model):
    id = peewee.PrimaryKeyField(sequence='ee0_seq' if db0.sequences else None)
    port = peewee.IntegerField(constraints=[peewee.Check('port > 0')])
    user = peewee.CharField(unique=True)
    available = peewee.BooleanField(default=False)

    class Meta:
        database = db0
        db_table = 'ee0'


class Ee0b(peewee.Model):
    id = peewee.PrimaryKeyField(sequence='ee0_seq' if db1.sequences else None)
    port = peewee.IntegerField(constraints=[peewee.Check('port > 0')])
    user = peewee.CharField(unique=True)
    available = peewee.BooleanField(default=False)

    class Meta:
        database = db1
        db_table = 'ee0'


class TestSeq(unittest.TestCase):

    def setUp(self):
        db0.create_tables([Ee0a])
        Ee0a.delete().execute()

    def tearDown(self):
        Ee0a.delete().execute()
        db0.drop_table(Ee0a)

    def save_ee(self, tgt):
        tgt.save()

    def test_id(self):
        eeb = Ee0b(port=22, user='user0')
        eea = Ee0a(port=23, user='user1')
        t0 = threading.Thread(target=self.save_ee, args=(eea,))
        t1 = threading.Thread(target=self.save_ee, args=(eeb,))
        t0.start()
        t1.start()
        t0.join()
        t1.join()
#        eea.save()
#        eeb.save()
        ee1 = Ee0a.select().where(Ee0a.user == 'user0')[0]
        ee2 = Ee0b.select().where(Ee0b.user == 'user1')[0]
        self.assertEqual(22, ee1.port)
        self.assertEqual('user0', ee1.user)
        self.assertEqual(23, ee2.port)
        self.assertEqual('user1', ee2.user)
