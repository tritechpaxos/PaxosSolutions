###########################################################################
#   Rasevth Copyright (C) 2016 triTech Inc. All Rights Reserved.
#
#   This file is part of Rasevth.
#
#   Rasevth is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Rasevth is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Rasevth.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################
from __future__ import absolute_import
import argparse
import logging
from .rasevth import RasEvent


logger = logging.getLogger(__name__)


def main():
    try:
        parser = argparse.ArgumentParser()
        parser.add_argument('source', nargs=1)
        parser.add_argument('-i', dest='entry')
        parser.add_argument('-r', dest='ras_cell')
        parser.add_argument('-s', dest='sequence', type=int)
        parser.add_argument('-m', dest='mode', type=int)
        parser.add_argument('-p', dest='path')
        parser.add_argument('-t', dest='time', type=int)
        parser.add_argument('-C', dest='group')
        parser.add_argument('-I', dest='no')
        conf = parser.parse_args()
        params = vars(conf)
        logger.info("eventh call: {}".format(str(params)))
        RasEvent(**params).process()
    except:
        logger.exception("execute failure")
