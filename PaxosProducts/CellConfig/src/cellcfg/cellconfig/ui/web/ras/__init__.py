# -*- coding: utf-8 -*-
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
from ..views import tabs
from ..views_admin import admin_tabs, admin_labels, admin_forms
from .forms_admin import AdminConfSmtpForm


def setup_tabs():
    adm_lbl = u'設定'
    adm = tabs[adm_lbl]
    del(tabs[adm_lbl])
    tabs[u'監視'] = ('rasgrp_list', lambda x: x.startswith('/rasclstr'), False)
    tabs[adm_lbl] = adm


def setup_admin_tabs():
    admin_tabs[u'メール設定'] = ('admin_conf', 'smtp')
    admin_labels['smtp'] = u'メール'
    admin_forms['smtp'] = AdminConfSmtpForm


setup_tabs()
setup_admin_tabs()
