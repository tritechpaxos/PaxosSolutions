# PaxosSolutions

(株)トライテックの開発したPAXOS Solutionsです。  
  
ここでは、以下のものが公開されています。
* PAXOS CORE  
* PAXOS memcache  
* PAXOS VirtualVolume  
  
## PAXOS COREのセットアップ方法
### Dockerを利用する場合
以下のコマンドを実行してPaxosのセル管理コンテナイメージを作成する。（dockerが実行できることを前提としている）
```
 $ cd PaxosProducts/CellConfig-Docker/pkg-build
 $ ./build.sh
 $ cd ../paxos
 $ docker build -t paxos_cellconfig cellconfig
```

コンテナを実行する。
```
 $ docker run -d -p 7000:7000 paxos_cellconfig
```
ブラウザから http://(ホスト名):7000/ にアクセスすることでPaxosのセル管理を利用できる。

デモ用環境の構築手順についてはPaxosProducts/CellConfig-Docker/READMEを参照のこと。

### それ以外の場合
1. PAXOS/Products/CellConfig/README の記述に従い必要なパッケージをインストールする
2. 以下のコマンドを実行してビルドを行う
```
　$ cd PaxosProducts/CellConfig/  
　$ ./build_pkg.sh  
``` 
3. ビルドに成功するとdist配下にアーカイブが作成される  
4. INSTALLファイルを参照して環境構築する  

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
  
