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
import copy
import signal
import string
import readline
import logging
import os
import atexit
from . import opt


logger = logging.getLogger(__name__)


def show_help(av):
    parser = opt.help_parser
    while True:
        tp = parser.parse_known_args(av)
        if len(tp[1]) == 0:
            tp[0].parser.print_help()
            break
        else:
            parser = tp[0].help_parser
            av = tp[1]


def parse_line(av):
    if av[0] == 'help':
        show_help(av)
    else:
        conf = opt.parser.parse_args(av)
        v = vars(conf)
        conf.cmd(**v).run()


def interpret(line):
    av = line.strip().split()
    if len(av) == 0:
        pass
    elif string.lower(av[0]) == 'quit':
        return False
    else:
        try:
            parse_line(av)
        except SystemExit as e:
            pass
        except Exception as e:
            logger.error(e.message)
    return True


class PaxosCompleter(object):

    OPTS = {
        'register': ['memcache', 'pair', 'css', 'host'],
        'unregister': ['memcache', 'pair', 'css', 'host'],
        'modify': ['memcache', 'host'],
        'show': ['memcache', 'css', 'host'],
        'list': ['memcache', 'css', 'host'],
        'start': ['memcache', 'css'],
        'stop': ['memcache', 'css'],
        'status': ['memcache', 'css'],
        'deploy': None,
        'fetch': None,
        'apply': ['pair'],
        'replace': None,
    }

    def match_keywords(self, words):
        keywords = copy.copy(self.OPTS)
        keywords['help'] = copy.copy(self.OPTS)
        try:
            for word in words:
                if word not in keywords.keys():
                    return None
                keywords = keywords[word]
            if isinstance(keywords, dict):
                keywords = sorted(keywords.keys())
            return keywords
        except (KeyError, IndexError):
            logger.debug('completion error')
            return None

    def make_candidates(self):
        self.candidates = []
        line = readline.get_line_buffer()
        pos0 = readline.get_begidx()
        pos1 = readline.get_endidx()
        prefix = line[pos0:pos1]
        words = line[:pos0].strip().split()
        keywords = self.match_keywords(words)
        if keywords:
            self.candidates = ([w for w in keywords if w.startswith(prefix)]
                               if prefix else keywords)

    def complete(self, text, state):
        if state == 0:
            self.make_candidates()
        return self.candidates[state] if state < len(self.candidates) else None


def setup_history():
    histfile = os.path.expanduser('~/.paxos_history')
    try:
        readline.read_history_file(histfile)
    except IOError:
        pass
    atexit.register(readline.write_history_file, histfile)


def run():
    setup_history()
    readline.set_completer(PaxosCompleter().complete)
    readline.parse_and_bind('tab: complete')
    while True:
        org_h = signal.signal(signal.SIGINT, signal.SIG_IGN)
        line = raw_input('(paxos) ')
        signal.signal(signal.SIGINT, org_h)
        try:
            if not interpret(line):
                break
        except KeyboardInterrupt:
            logger.warning('interrupted!')
