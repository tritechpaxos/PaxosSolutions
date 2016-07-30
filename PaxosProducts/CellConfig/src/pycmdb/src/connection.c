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
#include "connection.h"
#include "module.h"
#include "cursor.h"

static void
Connection_dealloc(Connection *self)
{
    if (self->table_map) {
        PyDict_Clear(self->table_map);
        Py_DECREF(self->table_map);
    }
    if (self->conn) {
        Py_BEGIN_ALLOW_THREADS
        RDB_UNLINK(self->conn);
        Py_END_ALLOW_THREADS
        self->conn = NULL;
    }
    Py_XDECREF(self->database);
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
Connection_new(PyTypeObject *type, PyObject *args, PyObject kwds)
{
    Connection *self;
    self = (Connection *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    self->database = PyString_FromString("");
    if (self->database == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->table_map = PyDict_New();
    if (self->table_map == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->conn = NULL;
    self->closed = 0;

    return (PyObject *) self;
}

static int
Connection_init(Connection *self, PyObject *args, PyObject *kwds)
{
    PyObject *database = NULL;
    static char *kwlist[] = {"database", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S", kwlist, &database)) {
        return -1;
    }
    Py_DECREF(self->database);
    Py_INCREF(database);
    self->database = database;

    Py_BEGIN_ALLOW_THREADS
    self->conn = RDB_LINK(NEO_CLIENT_ID, PyString_AsString(self->database));
    Py_END_ALLOW_THREADS
    if (!self->conn) {
        PyErr_SetString(InterfaceError, "connection failed");
        return -1;
    }
    self->closed = 0;
    PyDict_Clear(self->table_map);

    return 0;
}

static PyObject *
Connection_close(Connection *self)
{
    int ret;
    if (!self->closed) {
        PyDict_Clear(self->table_map);
        Py_BEGIN_ALLOW_THREADS
        ret = RDB_UNLINK(self->conn);
        Py_END_ALLOW_THREADS
        if (ret < 0) {
            PyErr_SetString(InterfaceError, "close failed");
            return NULL;
        }
        self->conn = NULL;
        self->closed = 1;
    }
    Py_RETURN_NONE;
}

static int
Connection_is_valid(Connection *self)
{
    if (self->closed) {
        PyErr_SetString(InterfaceError, "already closed connection");
        return -1;
    }
    return 0;
}

static PyObject *
Connection_commit(Connection *self)
{
    int ret;
    if (Connection_is_valid(self) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    ret = RDB_COMMIT(self->conn);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, "commit transaction error");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
Connection_rollback(Connection *self)
{
    int ret;
    if (Connection_is_valid(self) < 0) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    ret = RDB_ROLLBACK(self->conn);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, "rollback transaction error");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
Connection_cursor(Connection *self)
{
    if (Connection_is_valid(self) < 0) {
        return NULL;
    }
    return PyObject_CallFunction((PyObject *) &CursorType, "O", self);
}

#define CAPSULE_NAME "_pycmdb.table"

static void
Connection_destructor_table_capsule(PyObject *capsule)
{
    int ret;
    r_tbl_t *td;
    void *ptr = PyCapsule_GetPointer(capsule, CAPSULE_NAME);
    if (!ptr) {
        return;
    }
    td = (r_tbl_t *) ptr;

    Py_BEGIN_ALLOW_THREADS
    ret = RDB_CLOSE(td);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, neo_errsym());
    }
}

static r_tbl_t *
Connection_find_table(Connection *self, PyObject *table_name, int create)
{
    PyObject *tbl;

    tbl = PyDict_GetItem(self->table_map, table_name);
    if (tbl) {
        if (!PyCapsule_CheckExact(tbl)) {
            PyErr_SetString(PyExc_TypeError, "table type is invalid");
            return NULL;
        }
        return (r_tbl_t *) PyCapsule_GetPointer(tbl, CAPSULE_NAME);
    } else if (create) {
        int ret;
        r_tbl_t *td;
        char *tbl_name = PyString_AsString(table_name);

        Py_BEGIN_ALLOW_THREADS
        td = RDB_OPEN(self->conn, tbl_name);
        Py_END_ALLOW_THREADS
        if (!td) {
            PyErr_SetString(ProgrammingError, neo_errsym());
            return NULL;
        }
        tbl = PyCapsule_New(td, CAPSULE_NAME,
                            Connection_destructor_table_capsule);
        if (!tbl) {
            return NULL;
        }
        ret = PyDict_SetItem(self->table_map, table_name, tbl);
        if (ret < 0) {
            Py_DECREF(tbl);
            return NULL;
        }
        Py_DECREF(tbl);
        return td;
    }
    return NULL;
}

static int
Connection_close_table(Connection *self, PyObject *table_name)
{
    int ret;
    ret = PyDict_Contains(self->table_map, table_name);
    if (ret == 1) {
        ret = PyDict_DelItem(self->table_map, table_name);
    }
    return ret;
}

static int
Connection_lock_one_table(Connection *self, PyObject *table_name)
{
    int ret;
    r_hold_t hds[1];
    r_tbl_t *td;

    td = Connection_find_table(self, table_name, 1);
    if (!td) {
        return -1;
    }

    memset(hds, 0, sizeof(hds[0]));
    PyOS_snprintf((char *) hds[0].h_name, sizeof(hds[0].h_name), "%s",
                  PyString_AsString(table_name));
    hds[0].h_td = td;
    hds[0].h_mode = R_EXCLUSIVE;
    
    Py_BEGIN_ALLOW_THREADS
    ret = RDB_HOLD(sizeof(hds) / sizeof(r_hold_t), hds, R_WAIT);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, neo_errsym());
        return -1;
    }

    return 0;
}

static int
Connection_lock_tables(Connection *self, PyObject *table_tuple)
{
    int ret;
    Py_ssize_t size, i;
    r_hold_t *hds = NULL, *hd;

    if (!PyTuple_Check(table_tuple)) {
        PyErr_SetString(PyExc_TypeError, "table_tuple should be tuple");
        goto err_out;
    }
    size = PyTuple_Size(table_tuple);
    hds = PyMem_Malloc(sizeof(r_hold_t) * size);
    if (!hds) {
        goto err_out;
    }
    memset(hds, 0, sizeof(r_hold_t) * size);
    for (i = 0, hd = hds; i < size; i++, hd++) {
        PyObject *table_name;
        r_tbl_t *td;
        table_name = PyTuple_GetItem(table_tuple, i);
        if (!table_name) {
            goto err_out;
        }
        td = Connection_find_table(self, table_name, 1);
        if (!td) {
            goto err_out;
        }
        
        PyOS_snprintf((char *) hd->h_name, sizeof(hd->h_name), "%s",
                      PyString_AsString(table_name));
        hd->h_td = td;
        hd->h_mode = R_EXCLUSIVE;
    }
    
    Py_BEGIN_ALLOW_THREADS
    ret = RDB_HOLD(size, hds, R_WAIT);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, neo_errsym());
        goto err_out;
    }

    return 0;
err_out:
    if (hds) {
        PyMem_Free(hds);
    }
    return -1;
}

static int
Connection_unlock_one_table(Connection *self, PyObject *table_name)
{
    int ret;
    r_tbl_t *td;

    td = Connection_find_table(self, table_name, 0);
    if (!td) {
        return PyErr_Occurred() ? -1 : 0;
    }

    Py_BEGIN_ALLOW_THREADS
    ret = RDB_RELEASE(td);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(OperationalError, neo_errsym());
        return -1;
    }

    ret = Connection_close_table(self, table_name);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

typedef int (*lock_op_t)(Connection *, PyObject *);

static PyObject *
Connection_lock_wrapper(Connection *self, PyObject *args, lock_op_t lock_op)
{
    PyObject *param;
    int ret;

    if (Connection_is_valid(self) < 0) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O", &param)) {
        return NULL;
    }

    if (PyString_Check(param)) {
        ret = lock_op(self, param);
        if (ret < 0) {
            return NULL;
        }
    } else if (PyUnicode_Check(param)) {
        PyObject *table_name = PyUnicode_AsUTF8String(param);
        if (!table_name) {
            return NULL;
        }
        ret = lock_op(self, table_name);
        if (ret < 0) {
            Py_DECREF(table_name);
            return NULL;
        }
    } else if (PySequence_Check(param)) {
        int i, size;
        size = (int) PySequence_Size(param);
        if (size < 0) {
            return NULL;
        }
        for (i = 0; i < size; i++) {
            PyObject *item = PySequence_GetItem(param, i);
            if (!item) {
                return NULL;
            }
            if (!PyString_Check(item) && !PyUnicode_Check(item)) {
                PyErr_SetString(PyExc_TypeError,
                                "table name should be str or unicode");
                return NULL;
            }
            
        }
        for (i = 0; i < size; i++) {
            PyObject *item = PySequence_GetItem(param, i);
            if (!item) {
                return NULL;
            }
            ret = lock_op(self, item);
            if (ret < 0) {
                return NULL;
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "table type is invalid");
        return NULL;
    }
   
    Py_RETURN_NONE;
}

static PyObject *
Connection_lock(Connection *self, PyObject *args)
{
    PyObject *param;
    int ret;
    if (Connection_is_valid(self) < 0) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O", &param)) {
        return NULL;
    }

    if (PyString_Check(param)) {
        ret = Connection_lock_one_table(self, param);
        if (ret < 0) {
            return NULL;
        }
    } else if (PyUnicode_Check(param)) {
        PyObject *table_name = PyUnicode_AsUTF8String(param);
        if (!table_name) {
            return NULL;
        }
        ret = Connection_lock_one_table(self, table_name);
        if (ret < 0) {
            Py_DECREF(table_name);
            return NULL;
        }
    } else if (PySequence_Check(param)) {
        int i, size;
        PyObject *table_tuple;
        size = (int) PySequence_Size(param);
        if (size < 0) {
            return NULL;
        }
        if (size == 0) {
            Py_RETURN_NONE;
        }
        table_tuple = PyTuple_New(size);
        if (!table_tuple) {
            return NULL;
        }
        for (i = 0; i < size; i++) {
            PyObject *item = PySequence_GetItem(param, i);
            if (!item) {
                return NULL;
            }
            if (PyString_Check(item)) {
                ret = PyTuple_SetItem(table_tuple, i, item);
                if (ret) {
                    return NULL;
                }
            } else if (PyUnicode_Check(item)) {
                PyObject *table_name = PyUnicode_AsUTF8String(item);
                if (!table_name) {
                    return NULL;
                }
                ret = PyTuple_SetItem(table_tuple, i, table_name);
                if (ret) {
                    return NULL;
                }
            } else {
                PyErr_SetString(PyExc_TypeError,
                                "table name should be str or unicode");
                return NULL;
            }
        }
        ret = Connection_lock_tables(self, table_tuple);
        if (ret < 0) {
            return NULL;
        }

    } else {
        PyErr_SetString(PyExc_TypeError, "table type is invalid");
        return NULL;
    }
   
    Py_RETURN_NONE;
}


static PyObject *
Connection_unlock(Connection *self, PyObject *args)
{
    return Connection_lock_wrapper(self, args, Connection_unlock_one_table);
}

static PyObject *
Connection_get_exception(PyObject *self, void *closure)
{
    PyObject *exception = *(PyObject **) closure;
    Py_INCREF(exception);
    return exception;
}

static PyMemberDef Connection_members[] = {
    {"database", T_OBJECT_EX, offsetof(Connection, database), READONLY,
     "database name"},
    {"closed", T_INT, offsetof(Connection, closed), READONLY,
     "connection is closed"},
    {NULL}
};

static PyMethodDef Connection_methods[] = {
    {"close", (PyCFunction) Connection_close, METH_NOARGS,
     "close database connection"},
    {"commit", (PyCFunction) Connection_commit, METH_NOARGS,
     "commit transaction"},
    {"rollback", (PyCFunction) Connection_rollback, METH_NOARGS,
     "rollback transaction"},
    {"cursor", (PyCFunction) Connection_cursor, METH_NOARGS,
     "get cursor object"},
    {"lock", (PyCFunction) Connection_lock, METH_VARARGS,
     "lock the table"},
    {"unlock", (PyCFunction) Connection_unlock, METH_VARARGS,
     "unlock the table"},
    {NULL}
};

static PyGetSetDef Connection_getseters[] = {
    {"Warning", Connection_get_exception, NULL, "Warning", &Warning},
    {"Error", Connection_get_exception, NULL, "Error", &Error},
    {"InterfaceError", Connection_get_exception, NULL, "InterfaceError", &InterfaceError},
    {"DatabaseError", Connection_get_exception, NULL, "DatabaseError", &DatabaseError},
    {"DataError", Connection_get_exception, NULL, "DataError", &DataError},
    {"OperationalError", Connection_get_exception, NULL, "OperationalError", &OperationalError},
    {"IntegrityError", Connection_get_exception, NULL, "IntegrityError", &IntegrityError},
    {"InternalError", Connection_get_exception, NULL, "InternalError", &InternalError},
    {"ProgrammingError", Connection_get_exception, NULL, "ProgrammingError", &ProgrammingError},
    {"NotSupportedError", Connection_get_exception, NULL, "NotSupportedError", &NotSupportedError},
    {NULL}
};

PyTypeObject ConnectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pycmdb.Connection",      /*tp_name*/
    sizeof(Connection),   /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor) Connection_dealloc,                          /*tp_dealloc*/
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
    Connection_methods,     /*tp_methods*/
    Connection_members,     /*tp_members*/
    Connection_getseters,   /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc) Connection_init,                      /*tp_init*/
    0,                      /*tp_alloc*/
    (newfunc) Connection_new,         /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};
