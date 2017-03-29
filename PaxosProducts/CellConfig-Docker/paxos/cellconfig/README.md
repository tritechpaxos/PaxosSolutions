# セル管理コンテナイメージの作成

以下のコマンドを実行してコンテナイメージを作成する。
```
 $ pushd ../../pkg-build/
 $ ./build.sh
 $ popd
 $ docker build -t tritechpaxos/paxos-cellconfig .
```
