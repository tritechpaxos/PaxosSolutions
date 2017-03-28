#!/bin/sh

IMG_NAME=paxos-build-pkg
PKG_NAME=paxos-cellcfg.debian-8.x86_64.tar.gz

docker build -t ${IMG_NAME} .
docker run --rm ${IMG_NAME} > ${PKG_NAME}
docker rmi ${IMG_NAME}

cp ${PKG_NAME} ../paxos/cellconfig/
