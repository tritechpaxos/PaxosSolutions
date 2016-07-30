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
from os.path import expanduser, exists
from os import fstat, stat
import yaml

from . import config
from . import util


class ConfigFile(object):

    _loaded = False
    _mtime = None
    CONF_PATH = expanduser(config.MEMCACHE_CONF_PATH)

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, '__instance__'):
            cls.__instance__ = super(ConfigFile, cls).__new__(
                cls, *args, **kwargs)
        return cls.__instance__

    @classmethod
    def load(cls, force=False):
        conf = ConfigFile()
        if exists(ConfigFile.CONF_PATH):
            st = stat(ConfigFile.CONF_PATH)
        else:
            st = None
        if ((not cls._loaded) or (ConfigFile._mtime is None) or
           (st is None) or(ConfigFile._mtime < st.st_mtime) or force):
            conf.config = cls._load()
            cls._loaded = True
        return conf

    @classmethod
    def _load(cls):
        try:
            with open(ConfigFile.CONF_PATH) as f:
                txt = f.read()
                st = fstat(f.fileno())
                ConfigFile._mtime = st.st_mtime
            ret = yaml.load(txt)
            if ret is None:
                ret = dict()
            return ret
        except IOError:
            return dict()

    def save(self):
        util.makedirs(config.CONFIG_DIR)
        with open(ConfigFile.CONF_PATH, 'w') as f:
            yaml.dump(self.config, f)
            st = fstat(f.fileno())
            ConfigFile._mtime = st.st_mtime

    def _target(self, key):
        self.load()
        if key not in self.config:
            self.config[key] = []
        return self.config[key]

    def get(self, key, idx):
        target = self._target(key)
        return target[idx]

    def find(self, key, kv):
        result = []
        target = self._target(key)
        for item in target:
            if item.match(kv):
                result.append(item)
        return result

    def append(self, key, obj):
        target = self._target(key)
        if hasattr(obj, 'id') and obj.id is None:
            obj.id = (
                max(target, key=lambda o: o.id).id + 1
                if len(target) > 0 else 0)
        target.append(obj)

    def remove(self, key, obj):
        target = self._target(key)
        target.remove(obj)
