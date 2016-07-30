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
import cmdbpeewee
from cellconfig import db
from cellconfig.model import *
from cellconfig.model_admin import *


def create_tables(tables):
    for tbl in tables:
        db.drop_table(tbl, fail_silently=True)
    db.create_tables(tables)


def main():
    create_tables([Cell, Node, CellNode, AdminConf])
    try:
        from cellconfig.ras.model import *
        create_tables([RasGroup, RasServer, RasTarget, RasAction, RasScript])
    except:
        pass
