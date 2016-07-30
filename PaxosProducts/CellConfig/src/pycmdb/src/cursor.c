/**************************************************************************
 *  pycmdb Copyright (C) 2016 triTech Inc. All Rights Reserved.
 *
 *  This file is part of pycmdb.
 *
 *  Pycmdb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pycmdb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pycmdb.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/
#include "cursor.h"
#include "connection.h"
#include "module.h"
#include "adapter.h"

static void
Cursor_dealloc(Cursor *self)
{
    Py_XDECREF(self->connection);
    Py_XDECREF(self->description);
    Py_XDECREF(self->result);
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
Cursor_new(PyTypeObject *type, PyObject *args, PyObject kwds)
{
    Cursor *self;

    self = (Cursor *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    Py_INCREF(Py_None);
    self->connection = Py_None;
    Py_INCREF(Py_None);
    self->description = Py_None;
    Py_INCREF(Py_None);
    self->result = Py_None;

    return (PyObject *) self;
}

static int
Cursor_init(Cursor *self, PyObject *args)
{
    PyObject *connection = NULL;

    if (!PyArg_ParseTuple(args, "O!", &ConnectionType, &connection)) {
        return -1;
    }
    if (connection) {
        PyObject *tmp = self->connection;
        Py_INCREF(connection);
        self->connection = connection;
        Py_XDECREF(tmp);
    }
    self->closed = 0;
    self->rowcount = -1;
    self->arraysize = 1;
    return 0;
}

static int
Cursor_is_valid(Cursor *self)
{
    int closed;
    PyObject *ret;
    if (self->closed) {
        PyErr_SetString(InterfaceError, "already closed cursor");
        return -1;
    }
    if (!PyObject_TypeCheck(self->connection, &ConnectionType)) {
        PyErr_SetString(PyExc_TypeError,
                        "connection must be pycmdb.Connection");
        return -1;
    }
    ret = PyObject_GetAttrString(self->connection, "closed");
    closed = PyInt_AsLong(ret);
    if (closed == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (closed != 0) {
        PyErr_SetString(InterfaceError, "already closed connection");
        return -1;
    }
    return 0;
}

static PyObject *
Cursor_close(Cursor *self)
{
    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    self->closed = 1;
    Py_RETURN_NONE;
}

static int
is_programming_error(void)
{
    int err_class = (neo_errno >> 24) & 0x7f;
    switch (err_class) {
    case M_RDB:
       switch (neo_errno) {
       case E_RDB_NOEXIST:
       case E_RDB_EXIST:
       case E_RDB_INVAL:
       case E_RDB_TD:
       case E_RDB_UD:
       case E_RDB_SEARCH:
       case E_RDB_NOITEM:
           return 1;
       default:
           return 0;
       }
    case M_SQL:
       switch (neo_errno) {
       case E_SQL_NOMEM:
           return 0;
       default:
           return 1;
       }
    default:
        return 0;
    }
}

static int
Cursor_execute_parameters(Cursor *self, PyObject *parameters, r_stmt_t *stmt)
{
    Py_ssize_t i, size;
    int ret;

    if (parameters == NULL) {
        return 0;
    }

    if (!PySequence_Check(parameters)) {
        PyErr_SetString(PyExc_TypeError, "parameters must be sequence");
        return -1;
    }
    size = PySequence_Size(parameters);
    if (size == 0) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        PyObject *item = PySequence_GetItem(parameters, i);
        PyObject *newitem = adapt_obj(item);
        if (!newitem) {
            return -1;
        }
        if (PyString_Check(newitem)) {
            char *val = PyString_AsString(newitem);
            if (!val) {
                return -1;
            }
            Py_BEGIN_ALLOW_THREADS
            ret = sql_bind_str(stmt, i, val);
            Py_END_ALLOW_THREADS
        } else if (PyInt_Check(newitem)) {
            long val = PyInt_AsLong(newitem);
            if (val == -1 && PyErr_Occurred()) {
                return -1;
            }
            Py_BEGIN_ALLOW_THREADS
            ret = sql_bind_int(stmt, i, val);
            Py_END_ALLOW_THREADS
        } else if (PyLong_Check(newitem)) {
            long val = PyLong_AsLong(newitem);
            if (val == -1 && PyErr_Occurred()) {
                return -1;
            }
            Py_BEGIN_ALLOW_THREADS
            ret = sql_bind_long(stmt, i, val);
            Py_END_ALLOW_THREADS
        } else if (PyFloat_Check(newitem)) {
            double val = PyFloat_AsDouble(newitem);
            if (/*val == -1.0 &&*/ PyErr_Occurred()) {
                return -1;
            }
            Py_BEGIN_ALLOW_THREADS
            ret = sql_bind_double(stmt, i, val);
            Py_END_ALLOW_THREADS
        } else {
            ret = -1;
        }
        if (ret) {
            return ret;
        }
    }
    return 0;
}

static int
Cursor_execute_operation(Cursor *self, PyObject *operation, r_stmt_t **pstmt)
{
    PyObject *operation0 = NULL;
    r_man_t *conn;
    r_stmt_t *stmt;
    int ret;

    if (!PyString_Check(operation) && !PyUnicode_Check(operation)) {
        PyErr_SetString(PyExc_ValueError, "operation must be str or unicode");
        return -1;
    }

    conn = ((Connection *) self->connection)->conn;

    if (PyUnicode_Check(operation)) {
        operation0 = operation;
        operation = PyUnicode_AsUTF8String(operation0);
        Py_DECREF(operation0);
        if (!operation) {
            return -1;
        }
    }
    Py_BEGIN_ALLOW_THREADS
    ret = SQL_PREPARE(conn, PyString_AsString(operation), &stmt);
    Py_END_ALLOW_THREADS
    if (ret == 0) {
        *pstmt = stmt;
    } else {
        *pstmt = NULL;
    }

    return ret;
}

static int
Cursor_build_result_description(Cursor *self, r_scm_t *scm)
{
    int i, cols;
    PyObject *desc = NULL;
    PyObject *tmp;
    r_item_t *item;

    tmp = self->description;
    cols = scm->s_n;
    self->description = PyTuple_New(cols);
    Py_DECREF(tmp);
    if (!self->description) {
        return -1;
    }

    for (i = 0, item = scm->s_item; i < cols; i++, item++) {
        desc = PyTuple_New(7);
        if (!desc) {
            return -1;
        }
        PyTuple_SetItem(desc, 0,
                        PyString_FromString(item->i_name));
        PyTuple_SetItem(desc, 1,
                        PyString_FromFormat("%s[%d]", item->i_type, item->i_attr));
        Py_INCREF(Py_None);
        PyTuple_SetItem(desc, 2, Py_None);
        PyTuple_SetItem(desc, 3, PyInt_FromLong(item->i_size));
        Py_INCREF(Py_None);
        PyTuple_SetItem(desc, 4, Py_None);
        Py_INCREF(Py_None);
        PyTuple_SetItem(desc, 5, Py_None);
        Py_INCREF(Py_None);
        PyTuple_SetItem(desc, 6, Py_None);
        PyTuple_SetItem(self->description, i, desc);
    }
    return 0;
}

static int
Cursor_build_result(Cursor *self)
{
    r_man_t *conn;
    r_tbl_t *td = NULL;
    int ret;
    int i, j, cols;
    PyObject *desc = NULL;
    PyObject *tmp;
    r_item_t *item;
    r_head_t *row;

    conn = ((Connection *) self->connection)->conn;

    Py_BEGIN_ALLOW_THREADS
    ret = SQL_RESULT_FIRST_OPEN(conn, &td);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError,
                        "failed to fetch the query results");
        return -1;
    }
    if (!td) {
        return 0;
    }

    ret = Cursor_build_result_description(self, td->t_scm);
    if (ret < 0) {
        return -1;
    }

    cols = td->t_scm->s_n;

    self->rowcount = td->t_rec_num;
    tmp = self->result;
    self->result = PyList_New(td->t_rec_num);
    Py_DECREF(tmp);
    if (!self->result) {
        return -1;
    }
    Py_INCREF(self->result);
    for (i = 0, row = (r_head_t *) td->t_rec; i < td->t_rec_num;
         i++, row = (r_head_t *) ((char *) row + td->t_rec_size)) {
        desc = PyTuple_New(cols);
        if (!desc) {
            return -1;
        }
        PyList_SetItem(self->result, i, desc);
        for (j = 0, item = td->t_scm->s_item; j < cols; j++, item++) {
            switch (item->i_attr) {
            case 1: /* hex */
            case 2: /* int */
                PyTuple_SetItem(desc, j,
                    PyInt_FromLong(*(int *) ((char *) row + item->i_off)));
                break;
            case 3: /* unsigned int */
                PyTuple_SetItem(desc, j,
                    PyInt_FromLong(*(unsigned *) ((char *) row + item->i_off)));
                break;
            case 4: /* float */
                PyTuple_SetItem(desc, j,
                    PyFloat_FromDouble(*(float *) ((char *) row + item->i_off)));
                break;
            case 5: /* str */
                PyTuple_SetItem(desc, j,
                    PyUnicode_FromString((char *) row + item->i_off));
                break;
            case 6: /* long */
                PyTuple_SetItem(desc, j,
                    PyLong_FromLongLong(*(long long *) ((char *) row + item->i_off)));
                break;
            case 7: /* unsigned long */
                PyTuple_SetItem(desc, j,
                    PyLong_FromUnsignedLongLong(*(unsigned long long *) ((char *) row + item->i_off)));
                break;
            case 9: /* bytes */
                PyTuple_SetItem(desc, j,
                    PyString_FromStringAndSize((char *) row + item->i_off, item->i_len));
                break;
            default:
                PyErr_SetString(DataError,
                                "illegal data type");
                return -1;
            }
        }
    }

    Py_BEGIN_ALLOW_THREADS
    SQL_RESULT_CLOSE(td);
    Py_END_ALLOW_THREADS

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        ret = SQL_RESULT_FIRST_OPEN(conn, &td);
        Py_END_ALLOW_THREADS
        if (!td) {
            break;
        }
        if (ret < 0) {
            PyErr_SetString(OperationalError,
                            "failed to fetch the query results");
            return -1;
        }
        Py_BEGIN_ALLOW_THREADS
        SQL_RESULT_CLOSE(td);
        Py_END_ALLOW_THREADS
    }

    return 0;
}

static int
Cursor_do_execute(Cursor *self, r_stmt_t *stmt)
{
    int ret;

    Py_BEGIN_ALLOW_THREADS
    ret = SQL_STATEMENT_EXECUTE(stmt);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        if (is_programming_error()) {
            PyErr_SetString(ProgrammingError, neo_errsym());
        } else {
            if (neo_errno) {
                PyErr_SetString(OperationalError, neo_errsym());
            } else {
                PyErr_SetString(OperationalError, "sql execute failed");
            }
        }
        return -1;
    }
    return 0;
}

static PyObject *
Cursor_execute(Cursor *self, PyObject *args)
{
    PyObject *operation = NULL;
    PyObject *parameters = NULL;
    r_stmt_t *stmt = NULL;
    int ret;

    if (Cursor_is_valid(self) < 0) {
        goto error;
    }

    if (!PyArg_ParseTuple(args, "O|O", &operation, &parameters)) {
        goto error;
    }

    ret = Cursor_execute_operation(self, operation, &stmt);
    if (ret < 0) {
        goto error;
    }

    ret = Cursor_execute_parameters(self, parameters, stmt);
    if (ret < 0) {
        goto error;
    }

    if (Cursor_do_execute(self, stmt) < 0) {
        goto error;
    }
    sql_statement_close(stmt);
    stmt = NULL;

    if (Cursor_build_result(self) < 0) {
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (stmt) {
        sql_statement_close(stmt);
    }
    return NULL;
}

static PyObject *
Cursor_executemany(Cursor *self, PyObject *args)
{
    PyObject *operation = NULL;
    PyObject *seq_of_parameters = NULL;
    r_stmt_t *stmt = NULL;
    Py_ssize_t size, i;
    int ret;

    if (Cursor_is_valid(self) < 0) {
        goto error;
    }

    if (!PyArg_ParseTuple(args, "O|O", &operation, &seq_of_parameters)) {
        goto error;
    }

    ret = Cursor_execute_operation(self, operation, &stmt);
    if (ret < 0) {
        goto error;
    }

    if (!PySequence_Check(seq_of_parameters)) {
        PyErr_SetString(PyExc_TypeError, "seq_of_parameters must be sequence");
        goto error;
    }
    size = PySequence_Size(seq_of_parameters);

    for (i = 0; i < size; i++) {
        PyObject *parameters = PySequence_GetItem(seq_of_parameters, i);

        ret = Cursor_execute_parameters(self, parameters, stmt);
        if (ret < 0) {
            goto error;
        }

        if (Cursor_do_execute(self, stmt) < 0) {
            goto error;
        }

        if (Cursor_build_result(self) < 0) {
            goto error;
        }

        sql_statement_reset(stmt);
    }

    sql_statement_close(stmt);
    stmt = NULL;

    Py_RETURN_NONE;

error:
    if (stmt) {
        sql_statement_close(stmt);
    }
    return NULL;
}

static PyObject *
Cursor_fetchone(Cursor *self)
{
    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    if (!PyList_Check(self->result)) {
        PyErr_SetString(ProgrammingError, "no result yet");
        return NULL;
    }
    if (PyList_Size(self->result) > 0) {
        PyObject *result = PyList_GetItem(self->result, 0);
        Py_INCREF(result);
        PyList_SetSlice(self->result, 0, 1, NULL);
        return result;
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *
Cursor_fetchmany(Cursor *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"size", NULL};
    int size = self->arraysize;

    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &size)) {
        return NULL;
    }

    if (!PyList_Check(self->result)) {
        PyErr_SetString(ProgrammingError, "no result yet");
        return NULL;
    }

    self->arraysize = size;

    if (PyList_Size(self->result) > 0) {
        PyObject *result = PyList_GetSlice(self->result, 0, size);
        Py_INCREF(result);
        PyList_SetSlice(self->result, 0, size, NULL);
        return result;
    } else {
        PyObject *result = PyList_New(0);
        return result;
    }
}

static PyObject *
Cursor_fetchall(Cursor *self)
{
    PyObject *ret;
    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    if (!PyList_Check(self->result)) {
        PyErr_SetString(ProgrammingError, "no result yet");
        return NULL;
    }
    ret = self->result;
    Py_INCREF(Py_None);
    self->result = Py_None;
    Py_DECREF(ret);
    return ret;
}

static PyObject *
Cursor_setinputsizes(Cursor *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"sizes", NULL};
    PyObject *sizes;
    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &sizes)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
Cursor_setoutputsizes(Cursor *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"size", "column", NULL};
    PyObject *size;
    PyObject *column;
    if (Cursor_is_valid(self) < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "L|I", kwlist,
                                     &size, &column)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef cursor_methods[] = {
    {"close", (PyCFunction) Cursor_close, METH_NOARGS,
     "close cursor"},
    {"execute", (PyCFunction) Cursor_execute, METH_VARARGS,
     "execute sql statement"},
    {"executemany", (PyCFunction) Cursor_executemany, METH_VARARGS,
     "execute sql statements"},
    {"fetchone", (PyCFunction) Cursor_fetchone, METH_NOARGS,
     "get one record from the query results"},
    {"fetchmany", (PyCFunction) Cursor_fetchmany, METH_VARARGS|METH_KEYWORDS,
     "get multiple records from the query results"},
    {"fetchall", (PyCFunction) Cursor_fetchall, METH_NOARGS,
     "get all the records from the query results"},
    {"setinputsizes", (PyCFunction) Cursor_setinputsizes,
     METH_VARARGS|METH_KEYWORDS, ""},
    {"setoutputsizes", (PyCFunction) Cursor_setoutputsizes,
     METH_VARARGS|METH_KEYWORDS, ""},
    {NULL}
};

static struct PyMemberDef cursor_members[] = {
    {"description", T_OBJECT, offsetof(Cursor, description), READONLY},
    {"connection", T_OBJECT, offsetof(Cursor, connection), READONLY},
    {"rowcount", T_INT, offsetof(Cursor, rowcount), READONLY},
    {"arraysize", T_INT, offsetof(Cursor, arraysize), 0},
    {NULL}
};


PyTypeObject CursorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pycmdb.Cursor",      /*tp_name*/
    sizeof(Cursor),   /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor) Cursor_dealloc,                          /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    cursor_methods,         /*tp_methods*/
    cursor_members,         /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc) Cursor_init, /*tp_init*/
    0,                      /*tp_alloc*/
    (newfunc) Cursor_new,   /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};
