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
#ifndef __RDB_H
#define __RDB_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <neo_db.h>

extern int _fd_null;

#define _STDOUT_TO_NULL

#ifdef _STDOUT_TO_NULL
#define _PRE() \
do { \
	fd = dup(STDOUT_FILENO); \
	if (fd == -1) goto err_out; \
	err = dup2(_fd_null, STDOUT_FILENO); \
	if (err == -1) goto err_out; \
} while(0)

#define __POST(_ret) \
do { \
	err = fflush(stdout); \
	if (err) goto err_out; \
	err = dup2(fd, STDOUT_FILENO); \
	if (err == -1) goto err_out; \
	return _ret; \
err_out: \
	neo_errno = errno; \
} while(0)

#define _POST() __POST(ret)
#define _POST_VOID() __POST(;)
#else
#define _PRE()
#define _POST()
#define _POST_VOID()
#endif


static inline r_man_t *
RDB_LINK(p_name_t name, r_man_name_t database)
{
	r_man_t *ret = NULL;
	int err, fd;
	_PRE();
	ret = rdb_link(name, database);
	_POST();
#ifdef _STDOUT_TO_NULL
	if (ret) {
		rdb_unlink(ret);
		ret = NULL;
	}
#endif
	return ret;
}

static inline int
RDB_UNLINK(r_man_t *pmd)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_unlink(pmd);
	_POST();
	return ret;
}

static inline int
RDB_COMMIT(r_man_t *pmd)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_commit(pmd);
	_POST();
	return ret;
}

static inline int
RDB_ROLLBACK(r_man_t *pmd)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_rollback(pmd);
	_POST();
	return ret;
}

static inline r_tbl_t *
RDB_OPEN(r_man_t *pmd, r_tbl_name_t tbl_name)
{
	r_tbl_t *ret = NULL;
	int err, fd;
	_PRE();
	ret = rdb_open(pmd, tbl_name);
	_POST();
#ifdef _STDOUT_TO_NULL
	if (ret) {
		rdb_close(ret);
		ret = NULL;
	}
#endif
	return ret;
}

static inline int
RDB_CLOSE(r_tbl_t *ptd)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_close(ptd);
	_POST();
	return ret;
}

static inline int
RDB_HOLD(int n, r_hold_t hds[], int wait)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_hold(n, hds, wait);
	_POST();
	return ret;
}

static inline int
RDB_RELEASE(r_tbl_t *ptd)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = rdb_release(ptd);
	_POST();
	return ret;
}

static inline int
SQL_PREPARE(r_man_t *md, const char *statement, r_stmt_t **pstmt)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = sql_prepare(md, statement, pstmt);
	_POST();
	return ret;
}

static inline int
SQL_STATEMENT_EXECUTE(r_stmt_t *stmt)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = sql_statement_execute(stmt);
	_POST();
	return ret;
}

static inline void
SQL_RESULT_CLOSE(r_tbl_t *ptd)
{
	int err, fd;
	_PRE();
	SqlResultClose(ptd);
	_POST_VOID();
}

static inline int
SQL_RESULT_FIRST_OPEN(r_man_t *mdp, r_tbl_t **tdpp)
{
	int ret = -1;
	int err, fd;
	_PRE();
	ret = SqlResultFirstOpen(mdp, tdpp);
	_POST();
	return ret;
}
#endif
