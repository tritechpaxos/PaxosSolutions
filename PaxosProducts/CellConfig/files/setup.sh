#!/bin/bash

PAXOS_HOME=${HOME}/PAXOS

error() {
  local msg=$1
  echo "ERROR: $msg" 1>&2
  exit 1
}

init() {
MANAGER_DIR=${PAXOS_HOME}/manager
DIST_DIR=`pwd`
mkdir -p ${MANAGER_DIR}
TMP_DIR=`mktemp --tmpdir -d paxos.XXXXXXXXXX`
local log_fname="paxos_`date '+%Y%m%d%H%M%S'`_XXXX"
LOG=`mktemp --tmpdir --suffix=.log ${log_fname}`
trap '/bin/rm -rf ${TMP_DIR}' EXIT
}

check_env() {
python_ver=`python --version |& cut -f2 -d\  | cut -f1,2 -d.`
if [ $python_ver != '2.7' ];then
  error "To install this package is required python 2.7."
fi
ldconfig -p | grep -sq libevent-2 || error "To install this package is required \"libevent\""
}

install_python_pkg() {
  local pkg=$1
  tar -xzf $pkg -C ${TMP_DIR}
  pushd "${TMP_DIR}/`basename $pkg .tar.gz`"
  python setup.py install --user
  popd
}

make_env() {
  pushd ${DIST_DIR}/virtualenv/
  install_python_pkg setuptools-*.tar.gz
  install_python_pkg pip-*.tar.gz
  ~/.local/bin/pip install virtualenv*.whl --user
  popd
  pushd ${PAXOS_HOME}
  if [ ! -x ${MANAGER_DIR}/bin/python ]; then
    ~/.local/bin/virtualenv manager
  fi
  popd
}

install_pkgs() {
  pushd ${MANAGER_DIR}
  bin/pip install -U ${DIST_DIR}/wheelhouse/*
  mkdir -p doc
  cp -r --preserve=mode,timestamps ${DIST_DIR}/doc/* ${MANAGER_DIR}/doc
  mkdir -p ${MANAGER_DIR}/run/ ${MANAGER_DIR}/conf/ ${MANAGER_DIR}/log/
  if [ ! -f ${MANAGER_DIR}/conf/gunicorn.conf ]; then
    cp --preserve=mode,timestamps ${DIST_DIR}/conf/gunicorn.conf ${MANAGER_DIR}/conf/
  fi
  popd
}

secret() {
for i in {0..24}; do
  r=$((RANDOM % 256))
  if [ \( \( $r -gt 39 \) -a \( $r -lt 92 \) -o \
       \( $r -gt 92 \) -a \( $r -lt 126 \) \) ]; then
  printf "\x$(printf %x $r)"
  else
  printf "\\\\x%02x" "$r"
  fi
done
}

setup() {
  pushd ${MANAGER_DIR}
  mkdir -p ${MANAGER_DIR}/conf/
  if [ ! -e conf/cellconfig.cfg ];then
    cat <<EOF > conf/cellconfig.cfg
[cellconfig]
DATABASE=sqlite:////$HOME/PAXOS/manager/conf/cellconfig.db
DNS = False
[cellconfig.agent]
AGENTS=cellconfig.agents.ssh:cellconfig.agents.ssh.ras
[cellconfig.ui.web]
SECRET_KEY = '`secret`'
PLUGIN=.ras.views
[cellconfig.agents.ssh.sshagent]
POLLING_INTERVAL = 60
EOF
  fi
  if [ ! -e conf/cellconfig.db ];then
    ${MANAGER_DIR}/bin/create_tables
  fi
  mkdir -p dist/linux/
  pushd dist/linux
  cp ${DIST_DIR}/paxos-core.*.tar.gz .
  cp ${DIST_DIR}/paxos-ras.*.tar.gz .
  dist_name=`python -c 'import platform;v=platform.linux_distribution()[0:2];ver=v[1].split(".");print "{}-{}".format(v[0].replace(" ","-").lower(),ver[0])'`
  ofile=`echo paxos-core.*.tar.gz | cut -f1 -d\ `
  ln -sf $ofile ${ofile/${dist_name}/linux}
  ofile=`echo paxos-ras*.tar.gz | cut -f1 -d\ `
  ln -sf $ofile ${ofile/${dist_name}/linux}
  popd
  popd
}

init
check_env
make_env >> $LOG 2>&1
install_pkgs >> $LOG 2>&1
setup >> $LOG 2>&1
echo "It has been successfully installed." 1>&2
