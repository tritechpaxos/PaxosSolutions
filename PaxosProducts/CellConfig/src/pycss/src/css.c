/**************************************************************************
 *  pycss Copyright (C) 2016 triTech Inc. All Rights Reserved.
 *
 *  This file is part of pycss.
 *
 *  Pycss is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pycss is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pycss.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/
#include "css.h"
#include "file.h"
#include <string.h>
#include <errno.h>

static void
Css_dealloc(Css *self)
{
	if (self->session) {
		Py_BEGIN_ALLOW_THREADS
		PaxosSessionClose(self->session);
		PaxosSessionFree(self->session);
		Py_END_ALLOW_THREADS
		self->session = NULL;
	}
	Py_XDECREF(self->cell);
	Py_XDECREF(self->cwd);
	Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Css_new(PyTypeObject *type, PyObject *args, PyObject kwds)
{
	Css *self;
	self = (Css *) type->tp_alloc(type, 0);
	if (!self) {
		return NULL;
	}
	self->cell = PyUnicode_FromString("");
	if (self->cell == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	self->cwd = PyUnicode_FromString("/");
	if (self->cwd == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	self->session = NULL;
	return (PyObject *) self;
}

static struct Session *
css_session_open(char *cell, int no)
{
	struct Session *ses = NULL;
	int err;
	ses = PaxosSessionAlloc(Connect, Shutdown, CloseFd, GetMyAddr,
		Send, Recv, Malloc, Free, cell);
	if (!ses) {
		return NULL;
	}
	err = PaxosSessionOpen(ses, no, 1);
	if (err) {
		PaxosSessionFree(ses);
		return NULL;
	}
	return ses;
}

static int
Css_init(Css *self, PyObject *args, PyObject *kwds)
{
	PyObject *cell = NULL;
	int no = 0;
	static char *kwlist[] = {"cell", "no", NULL};
	char *cell_name = NULL;
#if PY_MAJOR_VERSION >= 3
	if (!PyArg_ParseTupleAndKeywords(args, kwds,
			"U|i", kwlist, &cell, &no)) {
#else
	if (!PyArg_ParseTupleAndKeywords(args, kwds,
			"S|i", kwlist, &cell, &no)) {
#endif
		printf("ERROR 0\n");
		return -1;
	}
	Py_DECREF(self->cell);
	Py_INCREF(cell);
	self->cell = cell;;
	self->no = no;
	if (PyUnicode_Check(cell)) {
		PyObject *ocell = NULL;
		ocell = PyUnicode_AsEncodedString(self->cell, NULL, NULL);
		if (!ocell) {
			return -1;
		}
		cell_name = PyBytes_AsString(ocell);
		if (!cell_name) {
			return -1;
		}
#if PY_MAJOR_VERSION < 3
	} else {
		cell_name = PyString_AsString(cell);
#endif
	}
	Py_BEGIN_ALLOW_THREADS
	self->session = css_session_open(cell_name, no);
	Py_END_ALLOW_THREADS
	if (!self->session) {
		PyErr_SetString(PyExc_IOError, "connection failed");
		return -1;
	}
	return 0;
}

#define _DIR_ENT_MAX 100
PFSDirent_t _dir_ent[_DIR_ENT_MAX];

static int
_css_listdir(struct Session *session, const char *path, PyObject *list)
{
	int err;
	int idx, num, i;
	PFSDirent_t *ent;

	num = _DIR_ENT_MAX;
	idx = 0;
	do {
		Py_BEGIN_ALLOW_THREADS
		err = PFSReadDir(session, (char *) path, _dir_ent, idx, &num);
		Py_END_ALLOW_THREADS
		if (err < 0) {
			PyErr_SetFromErrno(PyExc_OSError);
			return -1;
		}
		for (i = 0, ent = _dir_ent; i < num; i++, ent++) {
			if (!strcmp(".", ent->de_Name) ||
			    !strcmp("..", ent->de_Name)) {
				continue;
			}
#if PY_MAJOR_VERSION >= 3
			PyList_Append(list, PyUnicode_FromString(ent->de_Name));
#else
			PyList_Append(list, PyString_FromString(ent->de_Name));
#endif
		}
		idx = num;
	} while(num == _DIR_ENT_MAX);
	return 0;
}

static PyObject *
Css_listdir(Css *self, PyObject *args)
{
	char *path = NULL;
	char *pathfree = NULL;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "et:listdir",
			Py_FileSystemDefaultEncoding, &path)) {
		return NULL;
	}
	pathfree = path;
	result = PyList_New(0);
	if (!result) {
		PyMem_Free(pathfree);
		return NULL;
	}
	if (_css_listdir(self->session, path, result)) {
		Py_XDECREF(result);
		PyMem_Free(pathfree);
		return NULL;
	}
	PyMem_Free(pathfree);
	return result;
}

static PyObject *
Css_mkdir(Css *self, PyObject *args)
{
	int err;
	char *path = NULL;
	char *pathfree = NULL;

	if (!PyArg_ParseTuple(args, "et:mkdir",
			Py_FileSystemDefaultEncoding, &path)) {
		return NULL;
	}
	pathfree = path;
	Py_BEGIN_ALLOW_THREADS
	err = PFSMkdir(self->session, (char *) path);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_OSError, path);
		PyMem_Free(pathfree);
		return NULL;
	}
	PyMem_Free(pathfree);
	Py_RETURN_NONE;
}

static PyObject *
Css_rmdir(Css *self, PyObject *args)
{
	int err;
	char *path = NULL;
	char *pathfree = NULL;

	if (!PyArg_ParseTuple(args, "et:rmdir",
			Py_FileSystemDefaultEncoding, &path)) {
		return NULL;
	}
	pathfree = path;
	Py_BEGIN_ALLOW_THREADS
	err = PFSRmdir(self->session, (char *) path);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_OSError, path);
		PyMem_Free(pathfree);
		return NULL;
	}
	PyMem_Free(pathfree);
	Py_RETURN_NONE;
}

static PyObject *
_pystat_fromstructstat(struct stat *st)
{
	PyObject *v;
	v = PyStructSequence_New(&CssStatResultType);
	if (v == NULL) {
		return NULL;
	}
	PyStructSequence_SET_ITEM(v, 0, PyLong_FromLong((long) st->st_mode));
	PyStructSequence_SET_ITEM(v, 1, PyLong_FromLong((long) st->st_ino));
	PyStructSequence_SET_ITEM(v, 2, PyLong_FromLong((long) st->st_dev));
	PyStructSequence_SET_ITEM(v, 3, PyLong_FromLong((long) st->st_nlink));
	PyStructSequence_SET_ITEM(v, 4, PyLong_FromLong((long) st->st_uid));
	PyStructSequence_SET_ITEM(v, 5, PyLong_FromLong((long) st->st_gid));
	PyStructSequence_SET_ITEM(v, 6, PyLong_FromLong(st->st_size));
	PyStructSequence_SET_ITEM(v, 7, PyLong_FromLong((long) st->st_atime));
	PyStructSequence_SET_ITEM(v, 8, PyLong_FromLong((long) st->st_mtime));
	PyStructSequence_SET_ITEM(v, 9, PyLong_FromLong((long) st->st_ctime));
	if (PyErr_Occurred()) {
		Py_DECREF(v);
		return NULL;
	}
	return v;
}

static PyObject *
Css_stat(Css *self, PyObject *args)
{
	struct stat st;
	char *path = NULL;
	char *pathfree = NULL;
	int err;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "et:stat",
			Py_FileSystemDefaultEncoding, &path)) {
		return NULL;
	}
	pathfree = path;

	Py_BEGIN_ALLOW_THREADS
	err = PFSStat(self->session, path, &st);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_OSError, path);
		PyMem_Free(pathfree);
		return NULL;
	}

	result = _pystat_fromstructstat(&st);
	PyMem_Free(pathfree);
	return result;
}

static PyObject *
Css_remove(Css *self, PyObject *args)
{
	int err;
	char *path = NULL;
	char *pathfree = NULL;
	if (!PyArg_ParseTuple(args, "et:mkdir",
			Py_FileSystemDefaultEncoding, &path)) {
		return NULL;
	}
	pathfree = path;
	Py_BEGIN_ALLOW_THREADS
	err = PFSDelete(self->session, path);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_OSError, path);
		PyMem_Free(pathfree);
		return NULL;
	}
	PyMem_Free(pathfree);
	Py_RETURN_NONE;
}

static PyObject *
Css_open(Css *self, PyObject *args)
{
	PyObject *path;
	PyObject *dcell = (PyObject *) self;
	PyObject *mode = PyUnicode_FromString("r");
#if PY_MAJOR_VERSION >= 3
	if (!PyArg_ParseTuple(args, "O|UO", &path, &mode, &dcell)) {
		return NULL;
	}
#else
	if (!PyArg_ParseTuple(args, "O|SO", &path, &mode, &dcell)) {
		return NULL;
	}
#endif
	return PyObject_CallFunctionObjArgs((PyObject *) &CssFileType, self,
		path, mode, dcell, NULL);
}

static PyMethodDef css_methods[] = {
	{"listdir", (PyCFunction) Css_listdir, METH_VARARGS,
	 "Return a list containing the names of the entries in the directory."},
	{"mkdir", (PyCFunction) Css_mkdir, METH_VARARGS,
	 "mkdir(path)\n\nCreate a directory."},
	{"rmdir", (PyCFunction) Css_rmdir, METH_VARARGS,
	 "rmdir(path)\n\nRemove a directory."},
	{"stat", (PyCFunction) Css_stat, METH_VARARGS,
	 "stat(path) -> stat result\n\nPerform a stat on the given path."},
	{"remove", (PyCFunction) Css_remove, METH_VARARGS,
	 "remove(path)\n\nRemove a file (same as unlink(path)."},
	{"unlink", (PyCFunction) Css_remove, METH_VARARGS,
	 "unlink(path)\n\nRemove a file (same as remove(path)."},
	{"open", (PyCFunction) Css_open, METH_VARARGS,
	 "open(filename, flag) -> fd\n\nOpen a file."},
	{NULL},
};

static PyMemberDef css_members[] = {
	{"cell", T_OBJECT_EX, offsetof(Css, cell), READONLY, "cell"},
	{NULL}
};


PyTypeObject CssType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pycss.Css",               /*tp_name*/
    sizeof(Css),                /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor) Css_dealloc,   /*tp_dealloc*/
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
    css_methods,            /*tp_methods*/
    css_members,            /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc) Css_init,    /*tp_init*/
    0,                      /*tp_alloc*/
    (newfunc) Css_new,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

static PyStructSequence_Field stat_result_fields[] = {
    {"st_mode",    "protection bits"},
    {"st_ino",     "inode"},
    {"st_dev",     "device"},
    {"st_nlink",   "number of hard links"},
    {"st_uid",     "user ID of owner"},
    {"st_gid",     "group ID of owner"},
    {"st_size",    "total size, in bytes"},
    {"st_atime",   "time of last access"},
    {"st_mtime",   "time of last modification"},
    {"st_ctime",   "time of last change"},
    {0}
};

static PyStructSequence_Desc stat_result_desc = {
	"pycss.stat_result",
	"Result from PFSStat.",
	stat_result_fields,
	10
};

PyTypeObject CssStatResultType;

void
init_css_stat_result_type(void)
{
	PyStructSequence_InitType(&CssStatResultType, &stat_result_desc);
}
