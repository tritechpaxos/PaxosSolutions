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
import logging
import types
import collections
import blinker
import gevent
import gevent.pool
import importlib

from cellconfig import conf


logger = logging.getLogger(__name__)
_pool = gevent.pool.Pool(5)


class call(object):
    def __init__(self, ns=None, name=None, async=False):
        self.ns = ns
        self.name = name
        self.async = async
        self.sig = blinker.signal('error')
        if conf.has_option(self.__module__, 'AGENTS'):
            for agent in conf.get(self.__module__, 'AGENTS').split(':'):
                importlib.import_module(agent)

    def __call__(self, f):
        def report_error(g):
            e = g.exception
            msg = u"ERROR: {}".format(str(e))
            logger.debug(
                "ERROR: src={} msg={} msg_cls={}".format(
                    f.__name__, str(e), type(e)))
            logger.exception(e)
            gevent.sleep(2)
            self.sig.send(f, message=msg)

        def wrapped_f(*args, **kwargs):
            ret = f(*args, **kwargs)
            name = f.__name__ if self.name is None else self.name
            try:
                agents = map(
                    lambda x: x(args[0]), call.find_agents(name, self.ns))
            except AttributeError:
                logger.exception('agents')
                return ret
            if len(agents) == 1:
                g = _pool.apply_async(getattr(agents[0], name),
                                      args[1:], kwargs)
            else:
                def func(prev, agent):
                    ret = prev if prev is not None else []
                    ret.append(getattr(agent, name)(*args[1:], **kwargs))
                    return ret
                g = _pool.apply_async(reduce, [func, agents, None])
            g.link_exception(report_error)
            return g if self.async else g.get()
        return wrapped_f

    agents = {}

    @classmethod
    def find_agents(cls, func_name, ns=None):
        ret = []
        if ns not in cls.agents:
            raise AttributeError()
        for agent in cls.agents[ns]:
            try:
                getattr(agent, func_name)
                ret.append(agent)
            except AttributeError:
                pass
        if len(ret) == 0:
            raise AttributeError()
        return ret

    @classmethod
    def regist_agent(cls, agent, ns=None):
        if ns not in cls.agents:
            cls.agents[ns] = []
        cls.agents[ns].append(agent)


class EventListener(object):

    def __init__(self, group, source=blinker.Signal.ANY):
        egroup = group.split(':', 1)
        self.ns = egroup[0] if len(egroup) > 1 else None
        self.group = group
        self.signal = blinker.signal(group)
        self.source = source
        for agent in conf.get(self.__module__, 'AGENTS').split(':'):
            importlib.import_module(agent)
        self.targets = {}
        logger.debug("INIT: group={} source={} signal={}".format(
            group, str(source), str(self.signal)))

    def regist(self, cb):
        self.signal.connect(cb, sender=self.source)
        logger.debug("REGIST: sender={} receiver={}".format(
            self.source, self.signal.receivers))

    def unregist(self, cb, tag=None):
        self.signal.disconnect(cb, sender=self.source)

    def enable(self, event):
        logger.debug("ENABLE: event={}".format(str(event)))
        agents_cls = EventListener.find_agent(event, self.ns)
        priority = max([c.PRIORITY for c in agents_cls])
        agents_cls = [c for c in agents_cls if c.PRIORITY == priority]
        tgt = event['target']
        name = event['name']
        self.targets[name] = [a(tgt, self.signal) for a in agents_cls]
        for agent in self.targets[name]:
            logger.debug("SETUP: agent={}".format(agent))
            agent.enable(event)

    def disable(self, event):
        name = event['name']
        for agent in self.targets[name]:
            agent.disable(event)
        del(self.targets[name])

    agents = {}

    @classmethod
    def find_agent(cls, event, ns=None):
        return [a for a in cls.agents[ns] if a.match(event)]

    @classmethod
    def regist_agent(cls, agent, ns=None):
        logger.debug("REGIST: ns={} agent={}".format(ns, agent))
        if ns not in cls.agents:
            cls.agents[ns] = []
        cls.agents[ns].append(agent)
