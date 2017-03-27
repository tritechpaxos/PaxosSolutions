/******************************************************************************
*  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
*
*  See GPL-LICENSE.txt for copyright and license details.
*
*  This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version. 
*
*  This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with
* this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/******************************************************************************
 *
 *
 *	svc_logmsg.h
 *
 *	説明	ログメッセージ
 *
 ******************************************************************************/
#ifndef	_SVC_LOGMSG_
#define	_SVC_LOGMSG_

#ifdef	ZZZ
#define	RDBCOMERR01 ""	/*"要求受信エラー:%s"*/
#define	RDBCOMERR02 ""	/*"応答送信エラー:%s"*/
#define	RDBCOMERR03 ""	/*"メモリ不足:%s"*/
#define	RDBCOMERR04 ""	/*"テーブルストアエラー:%s"*/
#define	RDBCOMERR05 ""	/*"テーブルドロップエラー:%s"*/
#define	RDBCOMERR06 ""	/*"ファイル生成エラー:%s"*/
#define	RDBCOMERR07 ""	/*"ファイルオープンエラー:%s"*/
#define	RDBCOMERR08 ""	/*"ファイル削除エラー:%s"*/
#define	RDBCOMERR09 ""	/*"テーブル操作禁止:%s"*/
#define	RDBCOMERR10 ""	/*"サーバ停止中で、操作できません:%s"*/

#define	MAINERR01 ""	/*"RPCプロトコルエラー:%s"*/
#define	MAINERR02 ""	/*"RPCコマンドエラー:%s"*/
#define	MAINERR03 ""	/*"クライアントとの通信が異常切断ざれました(abort-code=%#x): %s"*/

#define	LINKERR01 ""	/*"ポートエントリヌール:E_RDB_CONFUSED"*/

#define	OPENLOG01 "[%s] is opened."	/*"テーブル %s オープンされました"*/
#define	OPENLOG02 "[%s] is closed"	/*"テーブル %s クローズされました"*/
#define	OPENERR01 "open ERROR[%s]"	/*"テーブルオープンエラー:%s"*/
#define	OPENERR02 "close ERROR[%s]"	/*"オープンユーザクローズエラー:%s"*/

#define	CREATELOG01 ""	/*"テーブル %s 生成されました"*/
#define	CREATELOG02 ""	/*"テーブル %s 削除されました"*/
#define	CREATEERR01 ""	/*"テーブル既に存在しています:%s"*/
#define	CREATEERR02 ""	/*"テーブル削除できません:%s"*/

#define	DELETEERR01 ""	/*"レコード存在していません:%s"*/
#define	DELETEERR02 ""	/*"レコードロック中です:%s"*/

#define	DIRECTERR01 DELETEERR01
#define	DIRECTERR02 DELETEERR02

#define	STOPLOG01 ""	/*"サーバが一時停止されました"*/
#define	STOPLOG02 ""	/*"サーバが再開されました"*/
#define	STOPERR01 ""	/*"サーバが既に停止中です:%s"*/
#define	STOPERR02 ""	/*"サーバを一時停止することできません:%s"*/
#define	STOPERR03 ""	/*"サーバが既に運転中です:%s"*/

#define	INSERTERR01 ""	/*"指定するアイテム名エラー:%s"*/
#define	INSERTERR02 ""	/*"挿入レコードは既に存在しています:%s"*/
#define	INSERTERR03 ""	/*"テーブルの拡張できません:%s"*/

#define	HOLDERR01 ""	/*"あるレコードがロックされています:%s"*/
#define	HOLDERR02 ""	/*"テーブルロック中です:%s"*/
#define	HOLDERR03 ""	/*"テーブルのロックユーザは他人です:%s"*/

#define	EVENTERR01 ""	/*"同期イベントクラス指定エラー:%s"*/
#else
#define	RDBCOMERR01 "要求受信エラー:%s"
#define	RDBCOMERR02 "応答送信エラー:%s"
#define	RDBCOMERR03 "メモリ不足:%s"
#define	RDBCOMERR04 "テーブルストアエラー:%s"
#define	RDBCOMERR05 "テーブルドロップエラー:%s"
#define	RDBCOMERR06 "ファイル生成エラー:%s"
#define	RDBCOMERR07 "ファイルオープンエラー:%s"
#define	RDBCOMERR08 "ファイル削除エラー:%s"
#define	RDBCOMERR09 "テーブル操作禁止:%s"
#define	RDBCOMERR10 "サーバ停止中で、操作できません:%s"

#define	MAINERR01 "RPCプロトコルエラー:%s"
#define	MAINERR02 "RPCコマンドエラー:%s"
#define	MAINERR03 "クライアントとの通信が異常切断ざれました(abort-code=%#x):%s"

#define	LINKERR01 "ポートエントリヌル:E_RDB_CONFUSED"

#define	OPENLOG01 "テーブル %s オープンされました"
#define	OPENLOG02 "テーブル %s クローズされました"
#define	OPENERR01 "テーブルオープンエラー:%s"
#define	OPENERR02 "オープンユーザクローズエラー:%s"

#define	CREATELOG01 "テーブル %s 生成されました"
#define	CREATELOG02 "テーブル %s 削除されました"
#define	CREATEERR01 "テーブル既に存在しています:%s"
#define	CREATEERR02 "テーブル削除できません:%s"

#define	DELETEERR01 "レコード存在していません:%s"
#define	DELETEERR02 "レコードロック中です:%s"

#define	DIRECTERR01 DELETEERR01
#define	DIRECTERR02 DELETEERR02

#define	STOPLOG01 "サーバが一時停止されました"
#define	STOPLOG02 "サーバが再開されました"
#define	STOPERR01 "サーバが既に停止中です:%s"
#define	STOPERR02 "サーバを一時停止することできません:%s"
#define	STOPERR03 "サーバが既に運転中です:%s"

#define	INSERTERR01 "指定するアイテム名エラー:%s"
#define	INSERTERR02 "挿入レコードは既に存在しています:%s"
#define	INSERTERR03 "テーブルの拡張できません:%s"

#define	HOLDERR01 "あるレコードがロックされています:%s"
#define	HOLDERR02 "テーブルロック中です:%s"
#define	HOLDERR03 "テーブルのロックユーザは他人です:%s"

#define	EVENTERR01 "同期イベントクラス指定エラー:%s"
#endif

#endif /* _SVC_LOGMSG_ */
