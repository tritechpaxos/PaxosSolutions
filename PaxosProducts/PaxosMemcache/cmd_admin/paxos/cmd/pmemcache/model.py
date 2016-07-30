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
import os
import logging
import signal

from . import conf_file, error
import paxos.cmd.pmemcache as pm


logger = logging.getLogger(__name__)


class Model(object):
    conf = conf_file.ConfigFile()

    @classmethod
    def exist(cls, key):
        ret = cls.conf.find(cls.TAG, {cls.KEY: key})
        return len(ret) == 1

    @classmethod
    def get(cls, key):
        ret = cls.conf.find(cls.TAG, {cls.KEY: key})
        if len(ret) == 0:
            raise error.NotFoundError(_('not found: {0}').format(key))
        assert len(ret) == 1
        return ret[0]

    @classmethod
    def find(cls, search={}):
        return cls.conf.find(cls.TAG, search)

    @classmethod
    def append(cls, obj):
        cls.conf.append(cls.TAG, obj)
        cls.conf.save()

    def save(self):
        self.conf.save()

    def delete_instance(self):
        self.conf.remove(self.TAG, self)
        self.conf.save()


class Command(object):

    def setup_filelog(self, fname):
        fh = logging.FileHandler(fname)
        fh.setLevel(logging.DEBUG)
        formatter = logging.Formatter(
            '%(asctime)s %(levelname)s:%(filename)s:%(lineno)d: %(message)s')
        fh.setFormatter(formatter)
        logging.getLogger().addHandler(fh)

    def __init__(self, verbose=0, debug=None, *args, **kwargs):
        super(Command, self).__init__()
        if debug is not None:
            self.setup_filelog(debug)
            pm.debug_log = debug
        self.verbose = verbose
        pm.verbose = verbose
        if self.verbose == 2:
            pm.ch.setLevel(logging.INFO)
        elif self.verbose > 2:
            pm.ch.setLevel(logging.DEBUG)
            pm.logger.setLevel(logging.DEBUG)
        logger.debug(self.__class__.__name__)

    def run(self):
        pass


class BaseStopCommand(object):

    def get_pid(self, name, mat):
        args = [
            'ps', '-C', name, '-o', 'pid,command', '--no-headers']
        logger.info("call %s", ' '.join(args))
        try:
            lines = check_output_timeout(args)
            for line in lines.splitlines():
                words = line.split()
                if mat(words):
                    return int(words[0])
            return None
        except Exception as e:
            logger.debug('', exc_info=True)
            return None

    def force_stop(self, name, mat):
        pid = self.get_pid(name, mat)
        if pid is None:
            return
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
