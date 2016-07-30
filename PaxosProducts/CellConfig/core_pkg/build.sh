#!/bin/bash

setup() {
ROOT_DIR=`pwd`
PAXOS_DIR=${ROOT_DIR}/../../..
DIST_DIR=${ROOT_DIR}/dist
PKGS_DIR=${ROOT_DIR}/pkgs
ARC_DIR=${ROOT_DIR}/arc
SRC_DIR=${ROOT_DIR}/../src
TMP_DIR=`mktemp --tmpdir -d paxos.XXXXXXXXXX`
trap '/bin/rm -rf ${TMP_DIR}' EXIT

mkdir -p ${DIST_DIR}

pkg_name=`python -c 'import platform;v=platform.linux_distribution()[0:2];ver=v[1].split(".");print "{}-{}.{}".format(v[0].replace(" ","-").lower(),ver[0],platform.machine())'`
}

build() {
  dir=$1
  if [ $# -eq 2 ]; then
    tgt=$2
  fi
  pushd ${dir}
  if [ -n "${tgt}" ]; then
    make ${tgt}
  fi
  make
  popd
}


build_paxos()
{
pushd ${PAXOS_DIR}
build NWGadget
build Paxos_2 clean
build Paxos_2/RAS
popd
}

get_pkg_name() {
  local name=$1
  . ${ROOT_DIR}/VERSION
  local cmd="echo \$${name/-/_}"
  local ver=`eval $cmd`
  echo ${name}.${ver}.${pkg_name}.tar.gz
}

make_dist_core() {
  mkdir -p ${TMP_DIR}/build_core
  pushd ${TMP_DIR}/build_core
  mkdir -p core/bin core/lib core/include
  install -t core/bin/ \
	${PAXOS_DIR}/Paxos_2/xjPing/bin/xjPingPaxos \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSServer \
	${PAXOS_DIR}/Paxos_2/VV/bin/LVServer
  install -s -t core/bin/ \
	${PAXOS_DIR}/Paxos_2/paxos/bin/PaxosAdmin \
	${PAXOS_DIR}/Paxos_2/session/bin/PaxosSessionProbe \
	${PAXOS_DIR}/Paxos_2/session/bin/PaxosSessionShutdown \
	${PAXOS_DIR}/Paxos_2/session/bin/PaxosSessionChangeMember \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSProbe \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSRasActive \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSRasCluster \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSRasMonitor \
	${PAXOS_DIR}/Paxos_2/VV/bin/LVProbe \
	${PAXOS_DIR}/NWGadget/bin/LogFiles \
	${PAXOS_DIR}/NWGadget/bin/LogPrint \
	${PAXOS_DIR}/Paxos_2/PFS/bin/PFSClient \
	${PAXOS_DIR}/Paxos_2/paxos/bin/PaxosIsActive \
	${PAXOS_DIR}/Paxos_2/xjPing/bin/xjSqlPaxos \
	${PAXOS_DIR}/Paxos_2/xjPing/bin/xjListPaxos
  install -t core/lib/ \
	${PAXOS_DIR}/NWGadget/lib/libNWGadget.a \
	${PAXOS_DIR}/Paxos_2/paxos/lib/libPaxos.a \
	${PAXOS_DIR}/Paxos_2/session/lib/libPaxosSession.a \
	${PAXOS_DIR}/Paxos_2/xjPing/lib/libneo2.a \
	${PAXOS_DIR}/Paxos_2/PFS/lib/libPaxosPFS.a \
	${PAXOS_DIR}/Paxos_2/cache/lib/libCache.a \
	${PAXOS_DIR}/Paxos_2/cache/lib/libCacheDlg.a \
	${PAXOS_DIR}/Paxos_2/cache/lib/libCacheBlock.a \
	${PAXOS_DIR}/Paxos_2/status/lib/libStatus.a
  install -t core/include/ \
	${PAXOS_DIR}/NWGadget/h/*.h \
	${PAXOS_DIR}/Paxos_2/paxos/h/*.h \
	${PAXOS_DIR}/Paxos_2/session/h/*.h \
	${PAXOS_DIR}/Paxos_2/xjPing/h/*.h \
	${PAXOS_DIR}/Paxos_2/PFS/h/*.h \
	${PAXOS_DIR}/Paxos_2/cache/h/*.h \
	${PAXOS_DIR}/Paxos_2/status/h/*.h
  mkdir -p vv/bin
  install -t vv/bin/ \
	${PAXOS_DIR}/Paxos_2/VV/bin/tgtdVV \
	${PAXOS_DIR}/Paxos_2/VV/bin/InitDB.sh
  install -s -t vv/bin/ \
	${PAXOS_DIR}/Paxos_2/VV/bin/tgtadm \
	${PAXOS_DIR}/Paxos_2/VV/bin/VVAdmin \
	${PAXOS_DIR}/Paxos_2/VV/bin/VVProbe
  mkdir -p .install
  grep paxos_core ${ROOT_DIR}/VERSION > .install/paxos_core

  tar czf ${DIST_DIR}/`get_pkg_name paxos-core` .
  popd
}

make_dist_ras_binary() {
  mkdir -p ${TMP_DIR}/build_ras_bin ${TMP_DIR}/build_ras
  pushd ${TMP_DIR}/build_ras_bin
  mkdir -p ras/bin
  cp ${PAXOS_DIR}/Paxos_2/RAS/bin/RasEye ras/bin
  cp ${PAXOS_DIR}/Paxos_2/RAS/bin/RasEyeAdm ras/bin
  cp ${PAXOS_DIR}/Paxos_2/RAS/bin/RasMail ras/bin

  tar czf ${TMP_DIR}/build_ras/paxos-ras-binary.${pkg_name}.tar.gz .
  popd
}

make_pyenv_arc() {
  mkdir -p ${TMP_DIR}/build_pyenv ${TMP_DIR}/build_ras
  pushd ${TMP_DIR}/build_pyenv
  git clone https://github.com/yyuu/pyenv.git .pyenv
  git clone https://github.com/yyuu/pyenv-virtualenv.git .pyenv/plugins/pyenv-virtualenv
  tar czf ${TMP_DIR}/build_ras/pyenv.tar.gz .pyenv
  popd
}

install_tmp_pip() {
  python_ver=`python --version |& cut -f2 -d\  | cut -f1,2 -d.`
  mkdir -p ${TMP_DIR}/pip/.local/lib/python${python_ver}/site-packages
  pushd ${TMP_DIR}/pip
  tar xzf ${ARC_DIR}/setuptools-22.0.5.tar.gz
  pushd setuptools-22.0.5
  env PYTHONUSERBASE=${TMP_DIR}/pip/.local python setup.py install --user
  popd
  tar xzf ${ARC_DIR}/pip-8.1.2.tar.gz
  pushd pip-8.1.2
  env PYTHONUSERBASE=${TMP_DIR}/pip/.local python setup.py install --user
  popd
  popd
  pip_cmd="env PYTHONUSERBASE=${TMP_DIR}/pip/.local ${TMP_DIR}/pip/.local/bin/pip"
  env PYTHONUSERBASE=${TMP_DIR}/pip/.local $pip_cmd install --user virtualenv
  virtualenv_cmd="env PYTHONUSERBASE=${TMP_DIR}/pip/.local ${TMP_DIR}/pip/.local/bin/virtualenv"
}

fetch_python_pkgs() {
  mkdir -p ${TMP_DIR}/build_ras
  pushd ${TMP_DIR}/build_ras
  $pip_cmd download --no-binary :all: setuptools pip
  $pip_cmd download virtualenv
  popd
}

fetch_rasevth_cmd() {
  mkdir -p ${TMP_DIR}/build_rasevth
  pushd ${SRC_DIR}
  tar cO rasevth pycss | tar -xf - -C ${TMP_DIR}/build_rasevth
  popd
  pushd ${TMP_DIR}/build_rasevth
  $virtualenv_cmd rasevth
  pushd rasevth
  bin/python setup.py bdist_wheel
  mkdir -p ${TMP_DIR}/build_ras/rasevth
  cp dist/* ${TMP_DIR}/build_ras/rasevth
  popd
  $virtualenv_cmd pycss
  pushd pycss
  tar xzf ${DIST_DIR}/css_client-dev.tar.gz
  bin/python setup.py bdist_wheel
  cp dist/* ${TMP_DIR}/build_ras/rasevth
  popd
  pushd ${TMP_DIR}/build_ras/rasevth
  $pip_cmd download future requests
  popd
  popd
}

make_dist_ras() {
  mkdir -p ${TMP_DIR}/build_ras
  cp ${PKGS_DIR}/ras/scripts/* ${TMP_DIR}/build_ras
  make_dist_ras_binary
  make_pyenv_arc
  install_tmp_pip
  fetch_python_pkgs
  fetch_rasevth_cmd
  cp ${ROOT_DIR}/VERSION ${TMP_DIR}/build_ras

  pushd ${TMP_DIR}/build_ras
  tar czf ${DIST_DIR}/`get_pkg_name paxos-ras` .
  popd
}

make_cmdb_client_dev() {
  mkdir -p ${TMP_DIR}/build_cmdb_dev
  pushd ${TMP_DIR}/build_cmdb_dev
  mkdir -p paxos/lib paxos/include
  cp ${PAXOS_DIR}/NWGadget/h/*.h paxos/include/
  cp ${PAXOS_DIR}/Paxos_2/xjPing/h/*.h paxos/include/
  cp ${PAXOS_DIR}/NWGadget/lib/libNWGadget.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/paxos/lib/libPaxos.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/session/lib/libPaxosSession.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/xjPing/lib/libneo2.a paxos/lib/
  tar czf ${DIST_DIR}/cmdb_client-dev.tar.gz .
  popd
}

make_css_client_dev() {
  mkdir -p ${TMP_DIR}/build_css_dev
  pushd ${TMP_DIR}/build_css_dev
  mkdir -p paxos/lib paxos/include
  cp ${PAXOS_DIR}/NWGadget/h/*.h paxos/include/
  cp ${PAXOS_DIR}/Paxos_2/PFS/h/*.h paxos/include/
  cp ${PAXOS_DIR}/Paxos_2/cache/h/FileCache.h paxos/include/
  cp ${PAXOS_DIR}/Paxos_2/session/h/*.h paxos/include/
  cp ${PAXOS_DIR}/Paxos_2/paxos/h/* paxos/include/
  cp ${PAXOS_DIR}/NWGadget/lib/libNWGadget.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/paxos/lib/libPaxos.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/session/lib/libPaxosSession.a paxos/lib/
  cp ${PAXOS_DIR}/Paxos_2/PFS/lib/libPaxosPFS.a paxos/lib/
  tar czf ${DIST_DIR}/css_client-dev.tar.gz .
  popd
}

setup
build_paxos
make_cmdb_client_dev
make_css_client_dev
make_dist_core
make_dist_ras
