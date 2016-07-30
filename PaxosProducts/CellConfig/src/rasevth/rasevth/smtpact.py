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
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.mime.application import MIMEApplication
from email.Header import Header
from email.Utils import formatdate
import string
import requests
import json
from .rasevth import RasScript, RasEvent


class SmtpScript(RasScript):

    CHARSET = 'ISO-2022-JP'

    def execute(self, status):
        info = self.smtp_info(status)
        self.from_addr = info['addr']
        srv = smtplib.SMTP(info['server'])
        message = self.build_message(status)
        srv.sendmail(
            self.from_addr, [self.params['to']], message.as_string())
        srv.quit()

    def smtp_info(self, status):
        rcell = status['ras_cell_obj']
        resp = requests.get("{}/api/conf/smtp".format(rcell.baseurl))
        return resp.json()

    def build_subject(self, status):
        subject = self.params['title'].format(**status)
        try:
            subject.encode('ascii')
            return subject
        except UnicodeEncodeError:
            return Header(subject, self.CHARSET)

    def build_text(self, status):
        fmt = self.params['message']
        if string.find(fmt, '{default}') >= 0:
            status['default'] = (
                '{source}[{path}:{entry}]({sequence}) is {mode_label}. {ctime}'
                .format(**status))
        text = fmt.format(**status)
        try:
            text.encode('ascii')
            return MIMEText(text)
        except UnicodeEncodeError:
            return MIMEText(text.encode(self.CHARSET), "plain", self.CHARSET)

    def build_attach_data(self, status):
        rcell = status['ras_cell_obj']
        entry = status['entry']
        if status['mode'] == RasEvent.STOP:
            entry = '{}_BAK'.format(entry)
        data = rcell.entry_data(entry)
        msg = MIMEApplication(data)
        msg.add_header(
            'Content-Disposition', 'attachment', filename='cell.data')
        return msg

    def build_message(self, status):
        if self.params['attach']:
            msg = MIMEMultipart()
            msg.attach(self.build_text(status))
            msg.attach(self.build_attach_data(status))
        else:
            msg = self.build_text(status)
        msg['Subject'] = self.build_subject(status)
        msg['From'] = self.from_addr
        msg['To'] = self.params['to']
        msg['Date'] = formatdate(localtime=True)
        return msg
