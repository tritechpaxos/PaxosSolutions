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


db = playhouse.db_url.connect('cmdb:///cmdb3')


class Ee0(peewee.Model):
    id = peewee.PrimaryKeyField(sequence='ee0_seq' if db.sequences else None)
    port = peewee.IntegerField(constraints=[peewee.Check('port > 0')])
    user = peewee.CharField(unique=True)
    available = peewee.BooleanField(default=False)

    class Meta:
        database = db


class TestCrud(unittest.TestCase):

    def setUp(self):
        db.create_tables([Ee0])
        Ee0.delete().execute()

    def tearDown(self):
        Ee0.delete().execute()
        db.drop_table(Ee0)

    def test_create(self):
        ee0 = Ee0(port=22, user='user0')
        ee0.save()

    def test_select(self):
        ee = Ee0(port=22, user='user0')
        ee.save()
        q = Ee0.select()
        self.assertEqual(1, len(q))
        ee0 = q[0]
        self.assertEqual(1, ee0.id)
        self.assertEqual(22, ee0.port)
        self.assertEqual('user0', ee0.user)

    def test_get(self):
        ee = Ee0(port=22, user='user0')
        ee.save()
        ee0 = Ee0.get(Ee0.id == 1)
        self.assertEqual(1, ee0.id)
        self.assertEqual(22, ee0.port)
        self.assertEqual('user0', ee0.user)

    def test_update(self):
        ee0 = Ee0(port=22, user='user0')
        ee0.save()
        ee0.port = 44
        ee0.save()

    def test_delete(self):
        ee0 = Ee0(port=22, user='user0')
        ee0.save()
        ee0.delete_instance()
