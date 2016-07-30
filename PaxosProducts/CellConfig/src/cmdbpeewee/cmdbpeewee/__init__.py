###########################################################################
#   cmdbpeewee Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of cmdbpeewee.
#
#   Cmdbpeewee is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cmdbpeewee is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with cmdbpeewee.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from urlparse import parse_qs
import playhouse.db_url
from .cmdbpeewee import CmdbDatabase
playhouse.db_url.schemes['cmdb'] = CmdbDatabase

_orig = getattr(playhouse.db_url, 'parseresult_to_dict')
setattr(playhouse.db_url, '_parseresult_to_dict', _orig)


def _parseresult_to_dict_new(parsed):
    connect_kwargs = playhouse.db_url._parseresult_to_dict(parsed)
    q = parsed.query
    if q is not None and len(q) > 0:
        connect_kwargs.update(parse_qs(q))
    return connect_kwargs

setattr(playhouse.db_url, 'parseresult_to_dict', _parseresult_to_dict_new)
