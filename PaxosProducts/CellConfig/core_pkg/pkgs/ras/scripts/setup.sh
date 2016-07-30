#!/bin/bash

PAXOS_HOME=$HOME/PAXOS

setup()
{
if [ $# -gt 0 ]; then
PAXOS_HOME=$1
fi
work=`mktemp --tmpdir -d paxos.XXXXXXXXXX`
trap '/bin/rm -rf ${work}' EXIT
VERSION_FILE=${PAXOS_HOME}/.install/paxos_ras
PKG_VER_FILE=`pwd`/VERSION
local log_fname="paxos_ras_`date '+%Y%m%d%H%M%S'`_XXXX"
LOG=`mktemp --tmpdir --suffix=.log ${log_fname}`
}

extract_paxos() {
  mkdir -p ${PAXOS_HOME}
  tar -xzf paxos-ras-binary*.tar.gz -C ${PAXOS_HOME}
}

setup_pyenv() {
  export PYENV_ROOT="$PAXOS_HOME/.pyenv"
  export PATH="$PYENV_ROOT/bin:$HOME/.local/bin:$PATH"
  eval "$(pyenv init -)"
  eval "$(pyenv virtualenv-init -)"
}

install_pyenv() {
  if [ ! -d ${PAXOS_HOME}/.pyenv ];then
    mkdir -p ${PAXOS_HOME}
    tar -xzf pyenv.tar.gz -C ${PAXOS_HOME}
  fi
  setup_pyenv
}

install_python_pkg() {
  local pkg=$1
  if [[ $pkg =~ \.tar\.gz$ ]]; then
    tar -xzf $pkg -C $work
    pushd "${work}/`basename $pkg .tar.gz`"
  elif [[ $pkg =~ \.tar\.gz$ ]]; then
    unzip $pkg -d $work
    pushd "${work}/`basename $pkg .zip`"
  fi
  python setup.py install --user
  popd
}

install_virtualenv() {
  if [ ! -x $HOME/.local/bin/virtualenv ]; then
    install_python_pkg setuptools-*
    install_python_pkg pip-*
    pip install virtualenv*.whl --user
  fi
}

install_ras_env() {
  pyenv versions | grep -q ras
  if [ $? -ne 0 ]; then
    pyenv virtualenv ras
    mkdir -p "${PAXOS_HOME}/ras"
    pushd "${PAXOS_HOME}/ras"
    pyenv local ras
    popd
    $PAXOS_HOME/.pyenv/versions/ras/bin/pip install -U setuptools-* pip-*
  fi
}

install_rasevth() {
    $PAXOS_HOME/.pyenv/versions/ras/bin/pip install -U rasevth/*
}

check_installed()
{
  if [ -f ${VERSION_FILE} ]; then
    . ${VERSION_FILE}
    cur_paxos_ras=${paxos_ras}
    . ${PKG_VER_FILE}
    if [ "X$cur_paxos_ras" == "X$paxos_ras" ];then
      exit 0;
    fi
  fi
}

mark_installed()
{
  . ${PKG_VER_FILE}
  mkdir -p `dirname ${VERSION_FILE}`
  echo "paxos_ras=${paxos_ras}" > ${VERSION_FILE}
}

install_paxos_ras()
{
check_installed
extract_paxos
install_pyenv
install_virtualenv
install_ras_env
install_rasevth
mark_installed
}

setup $*
install_paxos_ras > $LOG 2>&1
