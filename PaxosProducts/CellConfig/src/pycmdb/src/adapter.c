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
#include "adapter.h"

static PyObject *adapters;

static PyObject *
Adapter_unicode(PyObject *cls, PyObject *obj)
{
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "obj must be unicode");
        return NULL;
    }
    return PyUnicode_AsUTF8String(obj);
}

static PyObject *
Adapter_bool(PyObject *cls, PyObject *obj)
{
    if (!PyBool_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "obj must be bool");
        return NULL;
    }
    if (obj == Py_True) {
        return PyString_FromString("1");
    } else {
        return PyString_FromString("0");
    }
}

static PyObject *
adapter_none(PyObject *obj)
{
    return PyString_FromString("NULL");
}

PyObject *
adapt_obj(PyObject *obj)
{
    PyObject *ret;
    if (obj != Py_None) {
        PyObject *tp = (PyObject *) Py_TYPE(obj);
        PyObject *adapter = PyDict_GetItem(adapters, tp);
        if (adapter) {
            ret = PyObject_CallFunctionObjArgs(adapter, obj, NULL);
        } else {
            ret = obj;
        }
    } else {
        ret = adapter_none(NULL);
    }
    return ret;
}

int
init_adapters(PyObject *dict)
{
    PyObject *adapter;

    adapters = PyDict_New();
    if (!adapters) {
        return -1;
    }
    PyDict_SetItemString(dict, "adapters", adapters);

    adapter = PyObject_GetAttrString((PyObject *) &AdapterType, "to_unicode");
    PyDict_SetItem(adapters, (PyObject *) &PyUnicode_Type, adapter);

    adapter = PyObject_GetAttrString((PyObject *) &AdapterType, "to_bool");
    PyDict_SetItem(adapters, (PyObject *) &PyBool_Type, adapter);

    return 0;
}

static PyMethodDef Adapter_methods[] = {
    {"to_unicode", (PyCFunction) Adapter_unicode, METH_O|METH_CLASS, ""},
    {"to_bool", (PyCFunction) Adapter_bool, METH_O|METH_CLASS, ""},
    {NULL}
};

typedef struct {
    PyObject_HEAD
} Adapter;

PyTypeObject AdapterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pycmdb.Adapter",      /*tp_name*/
    sizeof(Adapter),   /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    0,                          /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Adapter_methods,        /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    0,                      /*tp_init*/
    0,                      /*tp_alloc*/
    0,                      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};
