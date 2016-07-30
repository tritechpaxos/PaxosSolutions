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
import importlib
from flask import Flask
from playhouse.flask_utils import FlaskDB
from .proxy import ReverseProxied
from cellconfig import conf, db

app = Flask(__name__)
app.wsgi_app = ReverseProxied(app.wsgi_app)
app.config.update(dict(conf.items(__package__)))
database = FlaskDB(app, db)

import views
import views_admin
import api
import event

if conf.has_option(__package__, 'PLUGIN'):
    for plugin in conf.get(__package__, 'PLUGIN').split(':'):
        importlib.import_module(plugin, package=__package__)
