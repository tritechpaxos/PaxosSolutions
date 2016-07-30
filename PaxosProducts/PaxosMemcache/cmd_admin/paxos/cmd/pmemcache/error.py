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


class ExistError(Exception):
    pass


class NotFoundError(Exception):
    pass


class InUseError(Exception):
    pass


class IllegalNameError(Exception):
    pass


class IllegalParamError(Exception):
    pass


class IllegalStateError(Exception):
    pass


class DuplicationError(Exception):
    pass


class NotStoppedError(Exception):
    pass


class TimeoutError(Exception):
    pass


class FabError(Exception):
    pass


class DeployError(Exception):
    def __init__(self, errors):
        message = ', '.join([e.message for e in errors])
        super(DeployError, self).__init__(message)


class FetchError(Exception):
    pass
