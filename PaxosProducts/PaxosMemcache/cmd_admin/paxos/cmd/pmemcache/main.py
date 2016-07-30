###########################################################################
#   PaxosMemcache Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of PaxosMemcache.
#
#   PaxosMemcache is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   PaxosMemcache is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with PaxosMemcache.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
import sys
from contextlib import contextmanager
import fabric.state
import fabric.network
from . import opt, console


def cmd_line():
    conf = opt.parser.parse_args()
    v = vars(conf)
    return conf.cmd(**v).run()


@contextmanager
def fab_cleanup():
    try:
        yield
    finally:
        fabric.state.output.status = False
        fabric.network.disconnect_all()


def main():
    try:
        ret = 1
        with fab_cleanup():
            ret = cmd_line() if len(sys.argv) > 1 else console.run()
        exit(ret)
    except (Exception, KeyboardInterrupt) as e:
        sys.exit(e)
