[1.はじめに]
この文書では PAXOS memcache/CSS の設定方法について記す。

PAXOS memcache/CSSを設定する大まかな流れを以下に記す。
  1) ホスト名の登録
  2) CSSの登録
  3) PAXOS memcacheの登録
  4) 設定ファイルの配布
  5) CSSの起動
  6) PAXOS memcacheの起動

この章以降で上記の各操作について説明を行う。

以降の章で操作例を示す場合のマシン構成をここに記す。
-----------------------------------------------------------
PAXOS memcache
  192.168.1.10
  192.168.1.11

CSS
  192.168.1.100
  192.168.1.101
  192.168.1.102

memcached
  192.168.1.200
-----------------------------------------------------------
PAXOS memcache/CSSはサーバプログラムを実行するための専用ユーザを用意し、
そのユーザのホームディレクトリにプログラムが適切にインストールされてい
る必要がある。

この文書の例では全てのホストで paxos というユーザ名でPAXOS memcache/CSS
がインストールされているものとする。また各ホストにはユーザ名 paxos で
sshのログインが可能な状態になっているものとする。

[2.管理コマンド]
PAXOS memcache/CSSを設定するには管理コマンドを用いる。PAXOS memcache/CSS
が適切にインストールされていれば管理コマンドの配置をPATHに追加するよう
設定される。そのためコマンド名 paxos を指定すれば管理コマンドを起動す
ることができる。

例.2-1>>>---------------------------------------------
 $ paxos
 (paxos) quit
 $ which paxos
 /home/paxos/bin/paxos
 $
例.2-1<<<---------------------------------------------

例.2-1のように管理コマンドを引数なしで実行するとインタラクティブモード
で起動される。インタラクティブモードでは"(paxos)"というプロンプトが表示
され、各プロンプト表示毎に一つの管理コマンドを実行することができる。な
お、以降の説明で例を示す場合はインタラクティブモードからコマンドを実行
する前提で記述する。

管理コマンドを終了するには"quit"またはCtrl-Dを入力すればよい。

管理コマンドを用いて設定を行うため、設定の操作はPAXOS memcache/CSSが
インストールされたホストで行う必要がある。また設定ファイルの配布を行う
までは、設定事項はローカルファイルのみに記録されている。そのため操作
1)～4)の間は単一のホストで操作を行う必要がある。

[3.ホスト名の登録]
管理コマンドでは他ホストに対する操作を行うために内部でsshを用いている。
そのため各ホストに対してsshでログインするためのユーザ名等の情報を登録
する必要がある。

ホストの登録は "register host" コマンドを使用する。例えば192.168.1.10
をユーザ名 "paxos" で登録する場合は以下のようにコマンドを実行する。

例.3-1>>>---------------------------------------------
 (paxos) register host --user paxos 192.168.1.10
 (paxos) show host
  ---
  hostname: "192.168.1.10"
  user: "paxos"
 (paxos)
例.3-1<<<---------------------------------------------

上記の例では register コマンドで登録を行った後 "show" コマンドで登録
結果を確認している。

ユーザ名を省略した場合、デフォルトのユーザ名 "paxos" が用いられる。

例.3-2>>>---------------------------------------------
 (paxos) register host 192.168.1.11
 (paxos) show host 192.168.1.11
  ---
  hostname: "192.168.1.11"
  user: "paxos"
 (paxos)
例.3-2<<<---------------------------------------------

基本的にはホストの登録はPAXOS memcache/CSSを起動させる全てのホストにつ
いて行う必要がある。ただし、ユーザ名がデフォルトの "paxos" の場合、CSS
のホスト登録は省略できる。これについては次章の例で示す。

[4.CSSの登録]
CSSを登録するにはCSS名と各サーバ毎に以下の情報が必要となる。
  CSSサーバ(TCP)のIPアドレス
  CSSサーバ(TCP)のポート番号
  CSSサーバ(UDP)のIPアドレス
  CSSサーバ(UDP)のポート番号

登録は "register css" コマンドを用いる。例えば、192.168.1.100,
192.168.1.101, 192.168.1.102 の３台をTCP, UDP ともにポート番号24111で
CSS名を"css0"で登録する場合の例を示す。

例.4-1>>>---------------------------------------------
 (paxos) register css --name css0 192.168.1.100:24111 192.168.1.100:24111  192.168.1.101:24111 192.168.1.101:24111  192.168.1.102:24111 192.168.1.102:24111 
 (paxos) show css
 ---
 name: css2
 server:
   -
     No.: 0
     address_udp: 192.168.1.100
     port_udp: 24111
     address_tcp: 192.168.1.100
     port_tcp: 24111
   -
     No.: 1
     address_udp: 192.168.1.101
     port_udp: 24111
     address_tcp: 192.168.1.101
     port_tcp: 24111
   -
     No.: 2
     address_udp: 192.168.1.102
     port_udp: 24111
     address_tcp: 192.168.1.102
     port_tcp: 24111
 (paxos)
例.4-1<<<---------------------------------------------

上記の例では register コマンドで登録を行った後 "show" コマンドで登録
結果を確認している。

ところで３章に記したようにPAXOS memcache/CSSを起動するホストの情報は
全て登録する必要がある。CSSのサーバに指定したホストについては、既に
登録済であればその情報が用いられる。もし未登録の場合CSSの登録時に自動
的に登録が行われる。その場合登録されるホストのユーザ名はデフォルトの
"paxos"となる。

例.4-2>>>---------------------------------------------
<例.4-1の続き>

 (paxos) show host -g css0
 ---
 hostname: "192.168.1.100"
 user: "paxos"
 group: [css0]
 ---
 hostname: "192.168.1.101"
 user: "paxos"
 group: [css0]
 ---
 hostname: "192.168.1.102"
 user: "paxos"
 group: [css0]
 (paxos)
例.4-2<<<---------------------------------------------

[5.PAXOS memcacheの登録]
PAXOS memcacheを登録するにはPAXOS memcacheが使用するCSS名と、参照する
memcached 毎に以下の情報が必要となる。
  memcached の IPアドレス
  memcached の ポート番号	（デフォルト値: 11211)
  PAXOS memcache の IPアドレス	（デフォルト値: 127.0.0.1）
  PAXOS memcache の ポート番号	（デフォルト値: 11211)

操作はPAXOS memcacheのインスタンスの登録とmemcached 毎の登録の２段階で
行う。

まずPAXOS memcacheのインスタンスの登録を"register memcache" コマンドを
用いて行う。"css0" という名前のCSSを使用する場合の例を以下に示す。

例.5-1>>>---------------------------------------------
 (paxos) register memcache --css css0
 (paxos) show memcache
 ---
 id: 0
 css: css0
 name space: space0
 pair:
 (paxos)

例.5-1<<<---------------------------------------------

次にmemcachedに関する情報を登録する。登録には "register pair"コマンドを
用いる。192.168.1.200を登録する場合の例を以下に示す。

例.5-2>>>---------------------------------------------
<例.5-1の続き>

 (paxos) register pair --id 0 --server-address 192.168.1.200
 (paxos) show memcache
 ---
 id: 0
 css: css0
 name space: space0
 pair:
     - ["127.0.1", 11211, "192.168.1.200", 11211]
 (paxos)
例.5-2<<<---------------------------------------------

上記の例では "register pair"コマンドの引数に --id を指定している。この
引数に指定する値は "register memcache" コマンドでPAXOS memcacheのイン
スタンスを登録した時に割り当てられたIDである。これは"show memcache"コ
マンドで確認できる。

[6.設定ファイルの配布]
ここまでの操作によって登録された情報は管理コマンドを実行したホストのロー
カルファイルにのみ記録されている。この設定情報をPAXOS memcache/CSSの各
サーバに配布する必要がある。配布には "deploy" コマンドを用いる。配布対
象となるホストは"register host"あるいは"register css"で登録されたホスト
全てである。

使用例を以下に示す。

例.6-1>>>---------------------------------------------
 (paxos) deploy
　[paxos@192.168.1.10] Login password for 'paxos':
 (paxos) 
例.6-1<<<---------------------------------------------

各ホストにsshでログインする際にパスワードが必要となる場合はプロンプト
が表示されるのでパスワードを入力する。

配布した設定ファイルは各マシンの /home/paxos/etc/ に格納される。

[7.CSSの起動]
登録したCSSを起動する。起動には"start css"コマンドを用いる。

"css0" という名前のCSSを起動する場合の例を以下に示す。

例.7-1>>>---------------------------------------------
 (paxos) start css --name css0
 (paxos) status css --short
 css0: running
 (paxos)
例.7-1<<<---------------------------------------------

上記の例では start コマンドで起動を行った後 "status" コマンドで起動でき
たかどうかを確認している。CSSが実行中の場合はCSS名の後に "running" と
表示される。

起動に失敗した場合は status の結果が stopped または error となる。CSSサー
バのログが syslog に記録されるので起動に失敗した場合はログの確認を行う。
例えばウェルノウンポート番号を指定している場合に管理者権限が無いと以下
のようなメッセージがログに記録される。

  PFSServer-css0#0: START ERROR:Permission denied

[8.PAXOS memcacheの起動]
登録したPAXOS memcacheを起動する。起動には"start memcache"コマンドを用
いる。この操作では自ホストのPAXOS memcacheの起動のみを行う。そのため
この操作はPAOXS memcacheを起動する各ホストにログインして実行する必要が
ある。

192.168.1.10のPAXOS memcacheを起動する場合の例を以下に示す。

例.8-1>>>---------------------------------------------

 $ ssh -lpaxos 192.168.1.10
 paxos@192.168.1.10's password:

 $ paxos
 (paxos) start memcache
 (paxos) status memcache --short
 0: running
 (paxos)
例.8-1<<<---------------------------------------------

上記の例では start コマンドで起動を行った後 "status" コマンドで起動でき
たかどうかを確認している。PAXOS memcacheが実行中の場合はIDの表示の後に
"running" と表示される。

[9.PAXOS memcacheの停止]
実行中のPAXOS memcacheを停止させる。停止には"stop memcache"コマンドを
用いる。この操作では自ホストのPAXOS memcacheの起動のみを行う。そのた
めこの操作はPAOXS memcacheを起動する各ホストにログインして実行する必
要がある。

192.168.1.10のPAXOS memcacheを停止させる場合の例を以下に示す。

例.9-1>>>---------------------------------------------

 $ ssh -lpaxos 192.168.1.10
 paxos@192.168.1.10's password:

 $ paxos
 (paxos) status memcache --short
 0: running
 (paxos) stop memcache
 (paxos) status memcache --short
 0: stopped
 (paxos)
例.9-1<<<---------------------------------------------

上記の例では stop コマンドで停止を行った後 "status" コマンドで停止でき
たかどうかを確認している。PAXOS memcacheが停止中の場合はIDの表示の後に
"stopped" と表示される。

[10.CSSの停止]
起動中のCSSを停止させる。停止には"stop css"コマンドを用いる。

"css0" という名前のCSSを停止させる場合の例を以下に示す。

例.10-1>>>---------------------------------------------
 (paxos) status css --short
 css0: running
 (paxos) stop css --name css0
 (paxos) status css --short
 css0: stopped
 (paxos)
例.10-1<<<---------------------------------------------

上記の例では stop コマンドで停止を行った後 "status" コマンドで停止でき
たかどうかを確認している。CSSが停止中の場合はCSS名の後に "stopped" と
表示される。

なお動作中のPAXOS memcacheが未だCSSを参照している場合は、そのCSSの停止
操作はエラーとなる。先にPAXOS memcacheを停止する必要がある。

[11.CSSのメンバ交代]
３台で構成されるCSSサーバのうち１台を交代する操作を説明する。必要となる
情報を以下に記す。
  対象とするCSSの名前
  対象とするサーバ番号
  新規CSSサーバ(TCP)のIPアドレス
  新規CSSサーバ(TCP)のポート番号
  新規CSSサーバ(UDP)のIPアドレス
  新規CSSサーバ(UDP)のポート番号

CSSメンバ交代の操作はCSSサーバが３台とも正常に起動しているか、全ての
CSSサーバが停止しているかのどちらかの状態の場合のみ許容されている。また、
交代で登録されるサーバは既にPAXOS memcache/CSSがインストールされていな
ければならない。また交代するホストを register host コマンドで登録する必
要がある。

"css0" という名前のCSSのNo.0のサーバをIPアドレス192.168.1.109、ポー
ト番号24222に交代する場合の例を以下に示す。

例.11-1>>>---------------------------------------------
 (paxos) status css
 ---
 name: css0
 server:
   -
     No.: 0
     address: 192.168.1.100
     status: running
     port: 24111
   -
     No.: 1
     address: 192.168.1.101
     status: running
     port: 24111
   -
     No.: 2
     address: 192.168.1.102
     status: running
     port: 24111
 (paxos) register host 192.168.1.109
 (paxos) replace --name css0 --no 0 --address 192.168.1.109:24222 192.168.1.109:24222
 (paxos) status css
 css0: stopped
 ---
 name: css0
 server:
   -
     No.: 0
     address: 192.168.1.109
     status: running
     port: 24222
   -
     No.: 1
     address: 192.168.1.101
     status: running
     port: 24111
   -
     No.: 2
     address: 192.168.1.102
     status: running
     port: 24111
 (paxos)
例.11-1<<<---------------------------------------------


