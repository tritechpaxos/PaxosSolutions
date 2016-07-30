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
import pycmdb


class CmdbQueryCompiler(peewee.QueryCompiler):

    def parse_node(self, node, alias_map=None, conv=None):
        sql, params, unknown = self._parse(node, alias_map, conv)
        if unknown and conv and params:
            params = [conv.db_value(i) for i in params]

        if isinstance(node, peewee.Node):
            if node._negated:
                sql = 'NOT %s' % sql
            if node._alias:
                sql = ' '.join((sql, node._alias))
            if node._ordering:
                sql = ' '.join((sql, node._ordering))
        return sql, params

    def build_query(self, clauses, alias_map=None):
        clauses = [x for x in clauses
                   if not(isinstance(x, peewee.SQL) and
                          x.value.startswith('LIMIT'))]
        if len(clauses) > 0:
            op = clauses[0]
            if isinstance(op, peewee.SQL) and op.value.startswith('UPDATE'):
                alias_map = None
        return super(CmdbQueryCompiler, self).build_query(clauses, alias_map)

    def field_definition(self, field):
        column_type = self.get_column_type(field.get_db_field())
        ddl = field.__ddl__(column_type)
        ddl = [x for x in ddl
               if not(isinstance(x, peewee.SQL) and
                      (x.value == 'NOT NULL' or
                       x.value.startswith('DEFAULT NEXTVAL(') or
                       x.value.startswith('CHECK (')))]
        return peewee.Clause(*ddl)

    def _create_table(self, model_class, safe=False):
        statement = 'CREATE TABLE'
        meta = model_class._meta

        columns = []
        if meta.composite_key:
            pk_cols = [meta.fields[f].as_entity()
                       for f in meta.primary_key.field_names]
        for field in meta.sorted_fields:
            columns.append(self.field_definition(field))

        return peewee.Clause(
            peewee.SQL(statement),
            model_class.as_entity(),
            peewee.EnclosedClause(*(columns)))


def cmdb_insert_execute(self):
    conn = self.database.get_conn()
    tbl = self.model_class._meta.db_table
    conn.lock(['cmdbidtable', tbl])
    try:
        sql = ""
        for fld in self.model_class._meta.fields.values():
            if fld.unique:
                sql += " OR " if len(sql) > 0 else " WHERE "
                sql += "{0} = ?".format(fld.name)
        if len(sql) > 0:
            sql = "SELECT * FROM {0}".format(tbl) + sql
            for r in self._rows:
                params = []
                for fld in self.model_class._meta.fields.values():
                    if fld.unique:
                        params.append(r[fld])
                cursor = self.database.execute_sql(sql, params, False)
                if len(cursor.fetchall()) > 0:
                    raise peewee.IntegrityError('unique constraint')
        sql = "UPDATE cmdbidtable SET id = id + 1 WHERE name=?"
        pk = self.model_class._meta.primary_key
        params = (pk.sequence, )
        self.database.execute_sql(sql, params, False)
        sql = "SELECT id FROM cmdbidtable WHERE name=?"
        cursor = self.database.execute_sql(sql, params, False)
        id = cursor.fetchone()[0]
        field_dict = self._rows[0]
        field_dict[pk.db_column] = id
        return super(peewee.InsertQuery, self)._execute()
    finally:
        conn.unlock(['cmdbidtable', tbl])


def cmdb_update_execute(self):
    conn = self.database.get_conn()
    tbl = self.model_class._meta.db_table
    pk = self.model_class._meta.primary_key
    conn.lock(tbl)
    try:
        sql = ""
        for fld in self.model_class._meta.fields.values():
            if fld.unique:
                sql += " OR " if len(sql) > 0 else " WHERE "
                sql += "{0} = ?".format(fld.name)
        if len(sql) > 0:
            sql = "SELECT {1} FROM {0}".format(tbl, pk.name) + sql
            params = []
            for fld in self.model_class._meta.fields.values():
                if fld.unique:
                    params.append(self._update[fld])
            cursor = self.database.execute_sql(sql, params, False)
            res = cursor.fetchall()
            if len(res) > 1 or\
               (len(res) == 1 and res[0][0] != self._where.rhs):
                raise peewee.IntegrityError('unique constraint')
        return super(peewee.UpdateQuery, self)._execute()
    finally:
        conn.unlock(tbl)


class CmdbDatabase(peewee.Database):
    interpolation = '?'
    quote_char = ''
    foreign_keys = False
    sequences = True
    field_overrides = {
        'bigint': 'LONG',
        'blob': 'BYTES',
        'bool': 'INT',
        'datetime': 'DATE',
        'double': 'FLOAT',
        'float': 'FLOAT',
        'int': 'INT',
        'primary_key': 'INT',
        'text': 'VARCHAR',
        'time': 'DATE',
    }
    compiler_class = CmdbQueryCompiler

    def __init__(self, *args, **kwargs):
        super(CmdbDatabase, self).__init__(*args, **kwargs)
        setattr(peewee.InsertQuery, '_execute', cmdb_insert_execute)
        setattr(peewee.UpdateQuery, '_execute', cmdb_update_execute)

    def _connect(self, database, **kwargs):
        return pycmdb.connect(database, **kwargs)

    def create_index(self, model_class, fields, unique=False):
        pass

    def create_foreign_key(self, model_class, field, constraint=None):
        pass

    def sequence_exists(self, seq):
        try:
            cursor = self.execute_sql(
                "SELECT * FROM cmdbidtable WHERE name=?", (seq, ))
        except peewee.ProgrammingError:
            return None
        res = cursor.fetchone()
        return res is not None

    def create_sequence(self, seq):
        try:
            self.execute_sql(
                "CREATE TABLE cmdbidtable (name VARCHAR(128), id UINT)")
        except peewee.ProgrammingError:
            pass
        return self.execute_sql(
            "INSERT INTO cmdbidtable (name, id) VALUES (?, ?)", (seq, 0))

    def drop_table(self, model_class, fail_silently=False, cascade=False):
        ret = super(CmdbDatabase, self).drop_table(model_class,
                                                   fail_silently, cascade)
        pk = model_class._meta.primary_key
        if self.sequences and pk.sequence:
            if self.sequence_exists(pk.sequence):
                self.drop_sequence(pk.sequence)
        return ret

    def drop_sequence(self, seq):
        return self.execute_sql(
            "DELETE FROM cmdbidtable WHERE name=?", (seq, ))

    def last_insert_id(self, cursor, model):
        sql = "SELECT id FROM cmdbidtable WHERE name=?"
        pk = model._meta.primary_key
        params = (pk.sequence, )
        cursor = self.execute_sql(sql, params)
        return cursor.fetchone()[0]
