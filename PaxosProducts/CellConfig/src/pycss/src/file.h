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
#include <Python.h>
#include <bytesobject.h>
#include <structmember.h>

#include <PFS.h>

typedef struct {
	PyObject_HEAD
	PyObject *css;
	PyObject *dcss;
	PyObject *name;
	PyObject *mode;
	File_t *file;
	int readable;
	int writable;
	int append_mode;
} CssFile;

#define CSS_F2S(x) (((Css *) (x)->css)->session)

extern PyTypeObject CssFileType;

static inline off_t
_cssfile_tell(CssFile *self)
{
	return self->file->f_Offset;
}
