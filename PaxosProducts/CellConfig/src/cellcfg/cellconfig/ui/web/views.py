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

import logging
import collections
from flask import request, redirect, url_for, abort, render_template

from . import app
from cellconfig.model import Node, Cell, CellNode
from .forms import NodeForm, CellForm


logger = logging.getLogger(__name__)
tabs = collections.OrderedDict([
    (u'セル', ('cell_list', lambda x: x.startswith('/cell'), False)),
    (u'サーバ', ('node_list', lambda x: x.startswith('/node'), True)),
    (u'設定', ('admin_index', lambda x: x.startswith('/admin'), True)),
])


@app.template_global()
def get_tabs():
    if Node.select().count() > 0:
        return tabs.items()
    else:
        return collections.OrderedDict(
            [(k, v) for k, v in tabs.items() if v[2]]).items()


@app.route('/node', methods=['GET', 'POST'])
def node_list():
    if request.method == 'GET':
        nodes = Node.select()
        return render_template('node_list.html', nodes=nodes)
    elif request.method == 'POST':
        form = NodeForm(request.form)
        if not (form.validate() and form.create()):
            return render_template('node_create.html', form=form)
        return redirect(url_for('node_list'))


@app.route('/node/create')
def node_create():
    form = NodeForm()
    return render_template('node_create.html', form=form)


@app.route('/node/<int:id>/update')
def node_update(id):
    node = Node.get(Node.id == id)
    form = NodeForm(obj=node)
    return render_template('node_update.html', form=form, id=id)


@app.route('/node/<int:id>/delete')
def node_delete(id):
    node = Node.get(Node.id == id)
    form = NodeForm(obj=node)
    return render_template('node_delete.html', form=form, id=id)


@app.route('/node/<int:id>', methods=['GET', 'POST'])
def node_detail(id):
    if request.method == 'GET':
        node = Node.get(Node.id == id)
        return render_template('node_show.html', node=node)
    elif request.method == 'POST':
        if request.form['operation'] == 'Update':
            form = NodeForm(request.form)
            if not form.validate():
                return render_template('node_update.html', form=form, id=id)
            form.update(id)
        elif request.form['operation'] == 'Delete':
            NodeForm.delete(id)
        else:
            abort(400)
        return redirect(url_for('node_list'))


@app.route('/')
def index():
    if Node.select().count() > 0:
        return redirect(url_for('cell_list'))
    else:
        return redirect(url_for('node_list'))


@app.route('/cell', methods=['GET', 'POST'])
def cell_list():
    if request.method == 'GET':
        cells = Cell.select().where(Cell.type != 'RAS')
        return render_template('cell_list.html', cells=cells)
    elif request.method == 'POST':
        form = CellForm(request.form)
        if not (form.validate() and form.create()):
            return render_template('cell_create.html', form=form)
        return redirect(url_for('cell_list'))


@app.route('/cell/create')
def cell_create():
    form = CellForm()
    return render_template('cell_create.html', form=form)


@app.route('/cell/<int:id>/update')
def cell_update(id):
    cell = Cell.get(Cell.id == id)
    form = CellForm(obj=cell)
    form.populate_cellnodes(cell.nodes)
    return render_template('cell_update.html', form=form, id=id)


@app.route('/cell/<int:id>/delete')
def cell_delete(id):
    cell = Cell.get(Cell.id == id)
    form = CellForm(obj=cell)
    form.populate_cellnodes(cell.nodes)
    return render_template('cell_delete.html', form=form, id=id)


@app.route('/cell/add_node/<int:no>', methods=['POST'])
def cell_add_node(no):
    form = CellForm(request.form)
    form.insert_cnode_by_no(no)
    if 'id' in request.form:
        id = request.form['id']
        return render_template('cell_update.html', form=form, id=id)
    else:
        return render_template('cell_create.html', form=form)


@app.route('/cell/del_node/<int:no>', methods=['POST'])
def cell_del_node(no):
    form = CellForm(request.form)
    form.delete_cnode_by_no(no)
    if 'id' in request.form:
        id = request.form['id']
        return render_template('cell_update.html', form=form, id=id)
    else:
        return render_template('cell_create.html', form=form)


@app.route('/cell/<int:id>', methods=['GET', 'POST'])
def cell_detail(id):
    if request.method == 'GET':
        cell = Cell.get(Cell.id == id)
        nstatus = cell.get_node_status()
        nodes = [v for k, v in sorted(nstatus.iteritems())]
        return render_template('cell_show.html', cell=cell, nodes=nodes)
    elif request.method == 'POST':
        if request.form['operation'] == 'Update':
            form = CellForm(request.form)
            if not (form.validate() and form.update(id)):
                return render_template('cell_update.html', form=form, id=id)
        elif request.form['operation'] == 'Delete':
            CellForm.delete(id)
        else:
            abort(400)
        return redirect(url_for('cell_list'))


@app.route('/cell/<int:id>/misc')
def cell_misc(id):
    cell = Cell.get(Cell.id == id)
    target = request.args['target']

    info0 = []
    if target == 'accept':
        title = (u'クライアント情報', u'クライアント一覧')
        label = [u'#', u'サーバ', u'クライアントアドレス',
                 u'クライアントポート', u'プロセスID', u'セッション番号',
                 u'リクエスト順番号', u'再接続番号', u'試行回数', u'参照数',
                 u'fd', u'接続状態', u'処理開始リクエスト順番号',
                 u'処理終了リクエスト順番号', u'応答済リクエスト順番号',
                 u'コマンド番号', u'送信待ちイベント数',
                 u'イベント即送信可能性', u'生成時刻', u'アクセス時刻']
    elif target == 'FdEvent':
        title = (u'FDイベント情報', u'FDイベント情報')
        label = [u'#', u'サーバ', u'fd', u'FDイベントフラグ']
    elif target == 'out_of_band':
        title = (u'帯域外データ情報', u'帯域外データ一覧')
        label = [u'#', u'サーバ', u'クライアントアドレス',
                 u'クライアントポート', u'プロセスID', u'セッション番号',
                 u'リクエスト順番号', u'再接続番号', u'試行回数', u'検証済',
                 u'データのメモリアドレス', u'PAXOS順番号']
    elif target == 'path':
        title = (u'ディレクトリイベント情報',
                 u'イベント受付先のクライアント一覧')
        label = [u'#', u'サーバ', u'ディレクトリ名', u'参照数',
                 u'クライアントアドレス', u'クライアントポート',
                 u'プロセスID', u'セッション番号', u'リクエスト順番号',
                 u'再接続番号', u'試行回数', u'参照数', u'fd', u'接続状態',
                 u'処理開始リクエスト順番号', u'処理終了リクエスト順番号',
                 u'応答済みリクエスト順番号', u'コマンド番号',
                 u'送信待ちイベント数', u'イベント即送信可能性',
                 u'生成時刻', u'アクセス時刻', u'FD番号', u'イベントビット']
    elif target == 'lock':
        title = (u'ロックイベント情報', u'ロックイベント一覧')
        label = [u'#', u'サーバ', u'ロック名', u'ロック保有者数',
                 u'保有／待機', u'クライアントアドレス', u'クライアントポート',
                 u'プロセスID', u'セッション番号', u'リクエスト順番号',
                 u'再接続番号', u'試行回数', u'参照数', u'fd', u'接続状態',
                 u'処理開始リクエスト順番号', u'処理終了リクエスト順番号',
                 u'応答済みリクエスト順番号', u'コマンド番号',
                 u'送信待ちイベント数', u'イベント即送信可能性',
                 u'生成時刻', u'アクセス時刻', u'FD番号', u'イベントビット']
    elif target == 'queue':
        title = (u'キューイベント一覧', u'キューデータ一覧',
                 u'待機クライアント一覧')
        label = [[u'#', u'サーバ', u'キュー名', u'キューデータ数', u'ACK待ち',
                  u'データ長'],
                 [u'#', u'サーバ', u'キュー名', u'キューデータ数', u'ACK待ち',
                  u'待機種', u'クライアントアドレス', u'クライアントポート',
                  u'プロセスID', u'セッション番号', u'リクエスト順番号',
                  u'再接続番号', u'試行回数', u'参照数', u'fd', u'接続状態',
                  u'処理開始リクエスト順番号', u'処理終了リクエスト順番号',
                  u'応答済みリクエスト順番号', u'コマンド番号',
                  u'送信待ちイベント数', u'イベント即送信可能性',
                  u'生成時刻', u'アクセス時刻', u'FD番号', u'イベントビット']]
        info0 = cell.get_misc_info(target).data
    elif target == 'meta':
        title = (u'メタデータ情報', u'メタデータ一覧')
        label = [u'#', u'サーバ', u'メモリアドレス', u'参照数',
                 u'メタデータ(ファイル)名', u'fd番号', u'ファイルサイズ']
    elif target == 'fd':
        title = (u'Fdデータ情報', u'Fdデータ一覧')
        label = [u'#', u'サーバ', u'メモリアドレス', u'参照数',
                 u'fdデータ(ファイル)名', u'実ファイルのディレクトリ名',
                 u'実ファイルのファイル名(basename)', u'fd番号']
    elif target == 'block':
        title = (u'ブロックキャッシュデータ情報',
                 u'ブロックキャッシュデータ一覧')
        label = [u'#', u'サーバ', u'メモリアドレス', u'参照数', u'ファイル名',
                 u'開始オフセット(バイト)', u'データ長(バイト)', u'フラグ']
    elif target == 'df':
        title = (u'領域空き情報', u'領域空き状態')
        label = [u'#', u'サーバ', u'ファイルシステム', u'総量', u'使用済',
                 u'未使用', u'使用%', u'マウントポイント']
    else:
        abort(400)

    if isinstance(info0, tuple):
        tbl = []
        for i, v in enumerate(info0):
            tbl.append(dict(
                title=title[i + 1],
                label=label[i],
                info=sorted(v, key=lambda x: int(x[0])),
            ))
        return render_template('cell_misc2.html', cell=cell, title=title[0],
                               table=tbl)
    else:
        info = sorted(info0, key=lambda x: int(x[0]))
        return render_template('cell_misc.html', cell=cell, title=title,
                               label=label, info=info)
