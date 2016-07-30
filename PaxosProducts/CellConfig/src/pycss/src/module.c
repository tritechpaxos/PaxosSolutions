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
#include "css.h"
#include "file.h"

static PyMethodDef module_methods[] = {
	{NULL, NULL},
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	"_pycss",
	NULL,
	-1,
	module_methods
};
#endif

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit__pycss(void)
#else
init_pycss(void)
#endif
{
	PyObject *m = NULL;

	init_css_stat_result_type();

	if (PyType_Ready(&CssType) < 0) {
		goto out;
	}
	if (PyType_Ready(&CssFileType) < 0) {
		goto out;
	}

#if PY_MAJOR_VERSION >= 3
	m = PyModule_Create(&module_def);
#else
	m = Py_InitModule("_pycss", module_methods);
#endif
	if (!m) {
		goto out;
	}

	Py_INCREF(&CssStatResultType);
	PyModule_AddObject(m, "stat_result", (PyObject *) &CssStatResultType);
	Py_INCREF(&CssType);
	PyModule_AddObject(m, "Css", (PyObject *) &CssType);
	Py_INCREF(&CssFileType);
	PyModule_AddObject(m, "CssFile", (PyObject *) &CssFileType);

out:
#if PY_MAJOR_VERSION >= 3
	return m;
#else
	return;
#endif
}
