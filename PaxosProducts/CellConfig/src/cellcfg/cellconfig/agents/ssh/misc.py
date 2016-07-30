###########################################################################
#   Cellcfg Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of Cellcfg.
#
#   Cellcfg is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Cellcfg is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Cellcfg.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################


class MiscData(object):
    data = None

    def append(self, val):
        if isinstance(val, MiscData):
            self.append(val.data)
        elif isinstance(val, tuple):
            if self.data is None:
                self.data = val
            else:
                for i, v in enumerate(self.data):
                    v.extend(val[i])
        elif isinstance(val, list):
            if self.data is None:
                self.data = []
            self.data.extend(val)
        elif val is None:
            pass
        else:
            raise TypeError
        return self
