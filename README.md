# PaxosSolutions

(株)トライテックの開発したPAXOS Solutionsです。  
  
ここでは、以下のものが公開されています。
* PAXOS CORE  
* PAXOS memcache  
* PAXOS VirtualVolume  
  
## PAXOS COREのセットアップ方法
### Dockerを利用する場合
以下のコマンドを実行してPaxosセル管理のコンテナを起動する。
```
 $ export HOSTPORT=7000
 $ docker run -d -p $HOSTPORT:7000 tritechpaxos/paxos-cellconfig
```
HOSTPORTの値は必要に応じて変更すること。

ブラウザから http://localhost:$HOSTPORT/ にアクセスすることでPaxosのセル管理を
利用できる。 他ホストからアクセスする場合はlocalhostの部分を適切なホスト名また
はIPアドレスに変更すること。

Docker関連のファイルについてはPaxosProducts/CellConfig-Docker/READMEを参照のこ
と。

### それ以外の場合
1. PaxosProducts/CellConfig/README の記述に従い必要なパッケージをインストールする
2. 以下のコマンドを実行してビルドを行う
```
　$ cd PaxosProducts/CellConfig/  
　$ ./build_pkg.sh  
``` 
3. ビルドに成功するとdist配下にアーカイブが作成される  
4. PaxosProducts/CellConfig/INSTALLファイルを参照して環境構築する  

## PAXOS memcacheのセットアップ方法
以下の手順でビルドを行うとpkgs配下にアーカイブが作成される。
```
　$ cd PaxosProducts/PaxosMemcache/  
　$ cd build_pkg/  
　$ ./build.sh  
```
ビルド後READMEファイルを参照して環境構築する  
  
## PAXOS VirtualVolumeのセットアップ方法  
Doc/Virtual Volume Setup.pptx　を参照して環境構築する  
  
