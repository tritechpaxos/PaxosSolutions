#!/bin/bash

build_source_tree() {
    local path=$1
    pushd ${path}
    make clean
    make
    popd
}

build_tools() {
    pushd ${tools_dir}
    bash build.sh
    popd
}

make_pkg() {
    pushd Paxos_2_1

    mkdir -p ${pkg_tmp}/bin
    install -t ${pkg_tmp}/bin/ paxos/bin/PaxosAdmin \
        session/bin/PaxosSessionProbe session/bin/PaxosSessionShutdown \
        session/bin/PaxosSessionChangeMember PFS/bin/PFSServer \
        PFS/bin/PFSProbe memcache/bin/PaxosMemcache \
        memcache/bin/PaxosMemcacheAdm

    mkdir -p ${pkg_tmp}/misc
    cp ${tools_dir}/tools.tar.gz ${tools_dir}/setup.sh ${tools_dir}/version.sh ${pkg_tmp}/misc

    cp -r ${doc_dir} ${license_dir} ${pkg_tmp}/misc

    pushd ${pkg_tmp}
    tar czpf ${pkg} .
    popd
    popd
}

get_release() {
    local dist=`lsb_release -a |& grep Distributor | cut -d: -f2 | tr -d '[[:space:]]'`
    local rel=`lsb_release -r | cut -d: -f2 | cut -d. -f1 | tr -d '[[:space:]]'`
    echo ${dist}_${rel}
}

pkg_tmp=`mktemp -d`
trap 'rm -rf ${pkg_tmp}' EXIT

paxos_dir=`pwd`/../../..
tools_dir=`pwd`/tools
pkg_dir=`pwd`/pkgs
service_dir=`pwd`/../service/`get_release`
doc_dir=`pwd`/../doc
license_dir=`pwd`/../license
pkg=${pkg_dir}/`get_release`.tar.gz

mkdir -p ${pkg_dir}

pushd ${paxos_dir}
build_source_tree NWGadget
build_source_tree Paxos_2_1
build_source_tree Paxos_2_1/memcache

build_tools

make_pkg
