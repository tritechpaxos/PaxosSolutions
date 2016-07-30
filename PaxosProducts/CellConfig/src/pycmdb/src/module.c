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
#include "module.h"
#include "connection.h"
#include "cursor.h"
#include "adapter.h"

PyObject *Error;
PyObject *Warning;
PyObject *InterfaceError;
PyObject *DatabaseError;
PyObject *DataError;
PyObject *OperationalError;
PyObject *IntegrityError;
PyObject *InternalError;
PyObject *ProgrammingError;
PyObject *NotSupportedError;

int _fd_null = -1;

static int
init_neo(void)
{
    char *av[] = {NEO_CLIENT_ID, NULL};
    int ac = 1;
    int ret;

    ret = _neo_prologue(&ac, av);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

static PyMethodDef module_methods[] = {
    {NULL, NULL}
};

static int
init_errors(PyObject *dict)
{
    Error = PyErr_NewException("pycmdb.Error", PyExc_StandardError, NULL);
    if (!Error) {
        return -1;
    }
    PyDict_SetItemString(dict, "Error", Error);

    Warning = PyErr_NewException("pycmdb.Warning", PyExc_StandardError, NULL);
    if (!Warning) {
        return -1;
    }
    PyDict_SetItemString(dict, "Warning", Warning);

    InterfaceError = PyErr_NewException("pycmdb.InterfaceError", Error, NULL);
    if (!InterfaceError) {
        return -1;
    }
    PyDict_SetItemString(dict, "InterfaceError", InterfaceError);

    DatabaseError = PyErr_NewException("pycmdb.DatabaseError", Error, NULL);
    if (!DatabaseError) {
        return -1;
    }
    PyDict_SetItemString(dict, "DatabaseError", DatabaseError);

    DataError = PyErr_NewException("pycmdb.DataError", Error, NULL);
    if (!DataError) {
        return -1;
    }
    PyDict_SetItemString(dict, "DataError", DataError);

    OperationalError = PyErr_NewException("pycmdb.OperationalError",
                                          DatabaseError, NULL);
    if (!OperationalError) {
        return -1;
    }
    PyDict_SetItemString(dict, "OperationalError", OperationalError);

    IntegrityError = PyErr_NewException("pycmdb.IntegrityError",
                                        DatabaseError, NULL);
    if (!IntegrityError) {
        return -1;
    }
    PyDict_SetItemString(dict, "IntegrityError", IntegrityError);

    InternalError = PyErr_NewException("pycmdb.InternalError",
                                       DatabaseError, NULL);
    if (!InternalError) {
        return -1;
    }
    PyDict_SetItemString(dict, "InternalError", InternalError);

    ProgrammingError = PyErr_NewException("pycmdb.ProgrammingError",
                                          DatabaseError, NULL);
    if (!ProgrammingError) {

        return -1;
    }
    PyDict_SetItemString(dict, "ProgrammingError", ProgrammingError);

    NotSupportedError = PyErr_NewException("pycmdb.NotSupportedError",
                                           DatabaseError, NULL);
    if (!NotSupportedError) {
        return -1;
    }
    PyDict_SetItemString(dict, "NotSupportedError", NotSupportedError);

    return 0;
}

static int
_init_neo(void)
{
	_fd_null = open("/dev/null", O_WRONLY);
	if (_fd_null == -1) {
		neo_errno = errno;
		return -1;
	}
	return init_neo();
}

PyMODINIT_FUNC
init_pycmdb(void)
{
    PyObject *m;
    PyObject *dict;
    int ret;

    if (PyType_Ready(&ConnectionType) < 0) {
        return;
    }
    if (PyType_Ready(&CursorType) < 0) {
        return;
    }
    AdapterType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&AdapterType) < 0) {
        return;
    }

    m = Py_InitModule("_pycmdb", module_methods);
    if (m == NULL) {
        return;
    }

    Py_INCREF(&ConnectionType);
    PyModule_AddObject(m, "Connection", (PyObject *) &ConnectionType);

    Py_INCREF(&CursorType);
    PyModule_AddObject(m, "Cursor", (PyObject *) &CursorType);

    Py_INCREF(&AdapterType);
    PyModule_AddObject(m, "Adapter", (PyObject *) &AdapterType);

    dict = PyModule_GetDict(m);

    if (init_errors(dict) < 0) {
        return;
    }

    if (init_adapters(dict) < 0) {
        return;
    }

    PyModule_AddStringConstant(m, "apilevel", "2.0");
    PyModule_AddIntConstant(m, "threadsafety", 0);
    PyModule_AddStringConstant(m, "paramstyle", "qmark");

    Py_BEGIN_ALLOW_THREADS
    ret = _init_neo();
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetString(PyExc_ImportError, "pycmdb._pycmdb: init failed");
    }
}
