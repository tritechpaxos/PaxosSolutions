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
#include "file.h"
#include "css.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>


static void
CssFile_dealloc(CssFile *self)
{
	if (self->file) {
		Py_BEGIN_ALLOW_THREADS
		PFSClose(self->file);
		Py_END_ALLOW_THREADS
		self->file = NULL;
	}
	Py_XDECREF(self->css);
	Py_XDECREF(self->dcss);
	Py_XDECREF(self->name);
	Py_XDECREF(self->mode);
	Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
CssFile_new(PyTypeObject *type, PyObject *args, PyObject kwds)
{
	CssFile *self;
	PyObject *itxt;

	self = (CssFile *) type->tp_alloc(type, 0);
	if (!self) {
		return NULL;
	}
	self->file = NULL;
	Py_INCREF(Py_None);
	self->css = Py_None;;
	Py_INCREF(Py_None);
	self->dcss = Py_None;;
	itxt =  PyUnicode_FromString("");
	if (itxt == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	Py_INCREF(itxt);
	self->name = itxt;
	Py_INCREF(itxt);
	self->mode = itxt;
	self->readable = 0;
	self->writable = 0;
	self->append_mode = 0;
	return (PyObject *) self;
}

static PyObject *
_cssfile_err_closed(void)
{
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	return NULL;
}

static PyObject *
_cssfile_err_mode(char *action)
{
	PyErr_Format(PyExc_IOError, "File not open for %s", action);
	return NULL;
}

static int
_cssfile_exists(CssFile *self, const char *path)
{
	struct stat st;
	int err;

	Py_BEGIN_ALLOW_THREADS
	err = PFSStat(CSS_F2S(self), (char *) path, &st);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, path);
		return 0;
	}
	return 1;
}

static int
_cssfile_isfile(CssFile *self, const char *path)
{
	struct stat st;
	int err;

	Py_BEGIN_ALLOW_THREADS
	err = PFSStat(CSS_F2S(self), (char *) path, &st);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, path);
		return 0;
	}
	return S_ISREG(st.st_mode) ? 1 : 0;
}

static size_t
_cssfile_getfilesize(CssFile *self)
{
	struct stat st;
	int err;
	char *path = self->file->f_Name;

	Py_BEGIN_ALLOW_THREADS
	err = PFSStat(CSS_F2S(self), path, &st);
	Py_END_ALLOW_THREADS
	if (err) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, path);
		return (size_t) (-1L);
	}
	return st.st_size;
}

static void
_cssfile_end_seek(CssFile *self, off_t offset)
{
	size_t size = _cssfile_getfilesize(self);
	if (PyErr_Occurred()) {
		return;
	}
	Py_BEGIN_ALLOW_THREADS
	PFSLseek(self->file, size + offset);
	Py_END_ALLOW_THREADS
}

static int
CssFile_init(CssFile *self, PyObject *args)
{
	PyObject *css = NULL;
	PyObject *o_name = NULL;
	char *name = NULL;
	PyObject *o_mode = NULL;
	char *mode;
	PyObject *dcss = NULL;
	int create = 0;
	int truncate = 0;

	if (!PyArg_ParseTuple(args, "O!ets|O:file", &CssType, &css,
			Py_FileSystemDefaultEncoding, &name, &mode, &dcss)) {
		return -1;
	}
#if PY_MAJOR_VERSION >= 3
	if (!PyArg_ParseTuple(args, "O!OU|O:file",
			&CssType, &css, &o_name, &o_mode, &dcss)) {
		PyMem_Free(name);
		return -1;
	}
#else
	if (!PyArg_ParseTuple(args, "O!OS|O:file",
			&CssType, &css, &o_name, &o_mode, &dcss)) {
		PyMem_Free(name);
		return -1;
	}
#endif

	Py_DECREF(self->css);
	Py_INCREF(css);
	self->css = css;
	if (dcss) {
		Py_DECREF(self->dcss);
		Py_INCREF(dcss);
		self->dcss = dcss;
	} else {
		Py_DECREF(self->dcss);
		Py_INCREF(css);
		self->dcss = css;
	}
	Py_DECREF(self->name);
	Py_INCREF(o_name);
	self->name = o_name;
	Py_DECREF(self->mode);
	Py_INCREF(o_mode);
	self->mode = o_mode;

	switch (mode[0]) {
	case 'r':
		self->readable = 1;
		if (!_cssfile_exists(self, name)) {
			PyMem_Free(name);
			return -1;
		}
		break;
	case 'a':
		self->append_mode = 1;
		 /* FALLTHROUGH */
	case 'w':
		self->writable = 1;
		if (!_cssfile_exists(self, name)) {
			create = 1;
		} else {
			truncate = 1;
		}
		break;
	default:
		PyErr_Format(PyExc_ValueError,
			"mode strign must begin with one of"
			" 'r', 'w' or 'a', not '%s'", mode);
		PyMem_Free(name);
		return -1;
	}
	if (strlen(mode) >= 2 && mode[1] == '+') {
		self->writable = self->readable = 1;
	}
	if (!create && !_cssfile_isfile(self, name)) {
		errno = EISDIR;
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
		PyMem_Free(name);
		return -1;
	}

	Py_BEGIN_ALLOW_THREADS
	if (create) {
		PFSCreate(CSS_F2S(self), name, 0);
	}
	if (truncate) {
		int ret;
		ret = PFSTruncate(CSS_F2S(self), name, 0);
		if (ret) {
			self->file = NULL;
			goto skip;
		}
	}
	self->file = PFSOpen(CSS_F2S(self), name, 0);
skip:
	Py_END_ALLOW_THREADS
	if (!self->file) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
		PyMem_Free(name);
		return -1;
	}
	errno = 0;
	PyErr_Clear();
	PyMem_Free(name);
	return 0;
}

static PyObject *
CssFile_close(CssFile *self)
{
	if (self->file) {
		Py_BEGIN_ALLOW_THREADS
		PFSClose(self->file);
		Py_END_ALLOW_THREADS
		self->file = NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *
CssFile_write(CssFile *self, PyObject *args)
{
	Py_buffer pbuf;
	char *s;
	Py_ssize_t n, n2;
	int err = 0;

	if (self->file == NULL) {
		return _cssfile_err_closed();
	}
	if (!self->writable) {
		return _cssfile_err_mode("writing");
	}
	
	if (!PyArg_ParseTuple(args, "s*", &pbuf)) {
		return NULL;
	}
	s = pbuf.buf;
	n = pbuf.len;

	if (self->append_mode) {
		_cssfile_end_seek(self, 0L);
		if (PyErr_Occurred()) {
			return NULL;
		}
	}
	Py_BEGIN_ALLOW_THREADS
	n2 = PFSWrite(self->file, s, n);
	Py_END_ALLOW_THREADS
	err = errno;
	PyBuffer_Release(&pbuf);
	if (n2 != n || err) {
		errno = err;
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	Py_RETURN_NONE;
}

static size_t
new_buffersize(CssFile *self, size_t currentsize)
{
	off_t pos, end;
	end = _cssfile_getfilesize(self);
	if (!PyErr_Occurred()) {
		pos = _cssfile_tell(self);
		if (end > pos && pos >= 0) {
			return currentsize + end - pos + 1;
		}
	} else {
		PyErr_Clear();
		errno = 0;
	}
	return currentsize + (currentsize >> 3) + 6;
}

static PyObject *
CssFile_read(CssFile *self, PyObject *args)
{
	PyObject *v;
	size_t buf_size;
	size_t read_size = 0;
	long req_len = -1;

	if (self->file == NULL) {
		return _cssfile_err_closed();
	}
	if (!self->readable) {
		return _cssfile_err_mode("reading");
	}
	if (_cssfile_getfilesize(self) == 0) {
		 /* workaround for bug of PFSRead() */
		return PyBytes_FromString("");
	}

	if (!PyArg_ParseTuple(args, "|l:read", &req_len)) {
		return NULL;
	}
	buf_size = (req_len >= 0) ? req_len : new_buffersize(self, 0);
	if (buf_size > PY_SSIZE_T_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"requested number of bytes is more than a Python "
			"string can hold");
		return NULL;
	}
	v = PyBytes_FromStringAndSize((char *) NULL, buf_size);
	if (v == NULL) {
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	read_size = PFSRead(self->file, PyBytes_AS_STRING(v), buf_size);
	Py_END_ALLOW_THREADS
	if (errno != 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		Py_DECREF(v);
		return NULL;
	}
	if (read_size != buf_size && _PyBytes_Resize(&v, read_size)) {
		return NULL;
	}
	return v;
}

static PyObject *
CssFile_tell(CssFile *self)
{
	if (self->file == NULL) {
		return _cssfile_err_closed();
	}
	return PyLong_FromLong(_cssfile_tell(self));
}

static PyObject *
CssFile_seek(CssFile *self, PyObject *args)
{
	int whence = 0;
	off_t offset;
	PyObject *offobj, *off_index;

	if (self->file == NULL) {
		return _cssfile_err_closed();
	}

	if (!PyArg_ParseTuple(args, "O|i:seek", &offobj, &whence)) {
		return NULL;
	}
	off_index = PyNumber_Index(offobj);
	if (!off_index) {
		return NULL;
	}
	offset = PyLong_AsLong(off_index);
	Py_DECREF(off_index);
	if (PyErr_Occurred()) {
		return NULL;
	}
	switch (whence) {
	case SEEK_SET:
		Py_BEGIN_ALLOW_THREADS
		PFSLseek(self->file, offset);
		Py_END_ALLOW_THREADS
		break;
	case SEEK_CUR:
		Py_BEGIN_ALLOW_THREADS
		PFSLseek(self->file, _cssfile_tell(self) + offset);
		Py_END_ALLOW_THREADS
		break;
	case SEEK_END:
		_cssfile_end_seek(self, offset);
		break;
	default:
		errno = EINVAL;
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyObject *
CssFile_truncate(CssFile *self, PyObject *args)
{
	off_t newsize;
	PyObject *newsizeobj = NULL;
	off_t initialpos;
	int ret;

	if (self->file == NULL) {
		return _cssfile_err_closed();
	}
	if (!self->writable) {
		return _cssfile_err_mode("writing");
	}
	if (!PyArg_UnpackTuple(args, "truncate", 0, 1, &newsizeobj)) {
		return NULL;
	}

	initialpos = _cssfile_tell(self);
	if (initialpos == -1) {
		goto onioerror;
	}
	if (newsizeobj != NULL) {
		newsize = PyLong_AsLong(newsizeobj);
		if (PyErr_Occurred()) {
			return NULL;
		}
	} else {
		newsize = initialpos;
	}
	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	ret = PFSTruncate(CSS_F2S(self), self->file->f_Name, newsize);
	Py_END_ALLOW_THREADS
	if (ret) {
		goto onioerror;
	}
	Py_RETURN_NONE;

onioerror:
	PyErr_SetFromErrno(PyExc_IOError);
	return NULL;
}

static PyObject *
CssFile_enter(CssFile *self)
{
	if (self->file == NULL) {
		return _cssfile_err_closed();
	}
	Py_INCREF(self);
	return (PyObject *) self;
}

static PyObject *
CssFile_exit(PyObject *self, PyObject *args)
{
	PyObject *ret = PyObject_CallMethod(self, "close", NULL);
	if (!ret) {
		return NULL;
	}
	Py_DECREF(ret);
	Py_RETURN_NONE;
}

static PyObject *
CssFile_get_closed(PyObject *self, void *closure)
{
	CssFile *f = (CssFile *) self; 
	if (f->file) {
		Py_RETURN_FALSE;
	} else {
		Py_RETURN_TRUE;
	}
}

static PyObject *
CssFile_repr(CssFile *self)
{
	PyObject *ret = NULL;
	PyObject *name = NULL;
	PyObject *mode = NULL;

#if PY_MAJOR_VERSION >= 3
	mode = PyObject_Str(self->mode);
#else
	mode = PyObject_Unicode(self->mode);
#endif
	if (PyUnicode_Check(self->name)) {
#if PY_MAJOR_VERSION >= 3
		name = PyObject_Repr(self->name);
		ret = PyUnicode_FromFormat("<%s file %S, mode '%S' at %p>",
			self->file == NULL ? "closed" : "open",
			name, mode, self);
#else
		name = PyUnicode_AsUnicodeEscapeString(self->name);
		ret = PyUnicode_FromFormat("<%s file u'%S', mode '%S' at %p>",
			self->file == NULL ? "closed" : "open",
			name, mode, self);
#endif
		Py_XDECREF(name);
		Py_XDECREF(mode);
		return ret;
	} else {
		name = PyObject_Repr(self->name);
		if (name == NULL) {
			return NULL;
		}
		ret = PyUnicode_FromFormat("<%s file %S, mode '%S' at %p>",
			self->file == NULL ? "closed" : "open",
			name, mode, self);
		Py_XDECREF(name);
		Py_XDECREF(mode);
		return ret;
	}
}

static PyMemberDef cssfile_members[] = {
	{"css", T_OBJECT_EX, offsetof(CssFile, dcss), READONLY, "css"},
	{"name", T_OBJECT_EX, offsetof(CssFile, name), READONLY, "file name"},
	{"mode", T_OBJECT_EX, offsetof(CssFile, mode), READONLY, "file mode"},
	{NULL}
};

static PyGetSetDef cssfile_getseters[] = {
	{"closed", CssFile_get_closed, NULL, "closed", NULL},
	{NULL}
};

static PyMethodDef cssfile_methods[] = {
	{"close", (PyCFunction) CssFile_close, METH_NOARGS,
	 "close() -> None\n\nClose the file."},
	{"write", (PyCFunction) CssFile_write, METH_VARARGS,
	 "write(str) -> None\n\nWrite string str to file."},
	{"read", (PyCFunction) CssFile_read, METH_VARARGS,
	 "read([size]) -> str\n\nRead at most size bytes, returned as string."},
	{"tell", (PyCFunction) CssFile_tell, METH_NOARGS,
	 "tell() -> current file position."},
	{"seek", (PyCFunction) CssFile_seek, METH_VARARGS,
	 "seek(offset[, whence]) -> None\n\nMove to new file position."},
	{"truncate", (PyCFunction) CssFile_truncate, METH_VARARGS,
	 "truncate([size]) -> None.\n\nTruncate the file to at most size bytes."},
	{"__enter__", (PyCFunction) CssFile_enter, METH_NOARGS,
	 "__enter__() -> self."},
	{"__exit__", (PyCFunction) CssFile_exit, METH_VARARGS,
	 "__exit__(*excinfo) -> None. Closes the file."},
	{NULL},
};

PyTypeObject CssFileType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pycss.CssFile",           /*tp_name*/
    sizeof(CssFile),            /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor) CssFile_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    (reprfunc) CssFile_repr,    /*tp_repr*/
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
    cssfile_methods,        /*tp_methods*/
    cssfile_members,        /*tp_members*/
    cssfile_getseters,      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc) CssFile_init, /*tp_init*/
    0,                      /*tp_alloc*/
    (newfunc) CssFile_new,  /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};
