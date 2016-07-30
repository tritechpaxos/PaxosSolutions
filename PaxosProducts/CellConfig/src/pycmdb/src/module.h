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
#include <Python.h>
#include <neo_skl.h>

#define NEO_CLIENT_ID "pycmdb"

extern PyObject *Error;
extern PyObject *Warning;
extern PyObject *InterfaceError;
extern PyObject *DatabaseError;
extern PyObject *DataError;
extern PyObject *OperationalError;
extern PyObject *IntegrityError;
extern PyObject *InternalError;
extern PyObject *ProgrammingError;
extern PyObject *NotSupportedError;