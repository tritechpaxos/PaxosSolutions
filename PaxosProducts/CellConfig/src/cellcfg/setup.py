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
from setuptools import setup, find_packages


setup(
    name='cellconfig',
    version='0.3.5',
    long_description='Cell Config Application',
    packages=find_packages(),
    include_package_data=True,
    zip_safe=False,
    entry_points={
        'console_scripts': ['create_tables=cellconfig.create_tables:main'],
    },
    install_requires=['Flask', 'peewee', 'WTForms-ParsleyJS', 'Fabric3']
)
