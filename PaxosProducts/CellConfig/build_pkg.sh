#!/bin/bash

init() {
TMP_DIR=`mktemp --tmpdir -d paxos.XXXXXXXXXX`
trap '/bin/rm -rf ${TMP_DIR}' EXIT
mkdir -p ${TMP_DIR}/dist/wheelhouse
CELLCFG_DIR=`pwd`
CORE_PKG_DIR=${CELLCFG_DIR}/core_pkg
ARC_DIR=${CORE_PKG_DIR}/arc
SRC_DIR=${CELLCFG_DIR}/src
pkg_name=`python -c 'import platform;v=platform.linux_distribution()[0:2];ver=v[1].split(".");print "{}-{}.{}".format(v[0].replace(" ","-").lower(),ver[0],platform.machine())'`
pkg_name0=`python -c 'import platform;v=platform.linux_distribution()[0:2];ver=v[1].split(".");print "{0}-{1}".format(v[0].replace(" ","-").lower(), ver[0])'`
}

error() {
  local msg=$1
  echo "ERROR: $msg" 1>&2
  exit 1
}

check_cmd() {
local cmd=$1
which $cmd > /dev/null || error "To build the package is required \"$cmd\"."
}

check_lib() {
local lib=$1
ldconfig -p | grep -sq $lib || error "To build the package is required \"$lib\""
}

check_inc() {
local inc=$1
pushd $TMP_DIR
local src=`mktemp --suffix .c paxos_XXXXXXX`
echo "#include <stdio.h>" > $src
echo "#include <$inc>" >> $src
gcc -c $src >& /dev/null
if [ $? -ne 0 ]; then
  rm $src
  error "To build the package is required \"$inc\""
fi
rm $src ${src/.c/.o}
popd
}

check_env() {
local tgt
for tgt in gcc make bison patch unzip git; do
check_cmd $tgt
done
for tgt in libreadline.so.6 libevent-2.0.so.5; do
check_lib $tgt
done
for tgt in openssl/md5.h event.h readline/readline.h python2.7/Python.h; do
check_inc $tgt
done
python_ver=`python --version |& cut -f2 -d\  | cut -f1,2 -d.`
if [ $python_ver != '2.7' ];then
  error "To build a package requires python 2.7."
fi
}

make_core_pkgs() {
pushd $CORE_PKG_DIR
bash ./build.sh
if [ $? -ne 0 ]; then
  error "failed to build the core package."
fi
popd
cp ${CORE_PKG_DIR}/dist/paxos-core.*.tar.gz ${TMP_DIR}/dist
cp ${CORE_PKG_DIR}/dist/paxos-ras.*.tar.gz ${TMP_DIR}/dist
}

make_pycmdb() {
mkdir -p ${TMP_DIR}/build_pycmdb
pushd ${SRC_DIR}
tar cO pycmdb | tar -xf - -C ${TMP_DIR}/build_pycmdb
popd
pushd ${TMP_DIR}/build_pycmdb
$virtualenv_cmd pycmdb
cd pycmdb
tar xzf ${CORE_PKG_DIR}/dist/cmdb_client-dev.tar.gz
bin/python setup.py bdist_wheel
cp dist/*.whl ${TMP_DIR}/dist/wheelhouse/
popd
}

make_pycss() {
mkdir -p ${TMP_DIR}/build_pycss
pushd ${SRC_DIR}
tar cO pycss | tar -xf - -C ${TMP_DIR}/build_pycss
popd
pushd ${TMP_DIR}/build_pycss
$virtualenv_cmd pycss
cd pycss
tar xzf ${CORE_PKG_DIR}/dist/css_client-dev.tar.gz
bin/python setup.py bdist_wheel
cp dist/*.whl ${TMP_DIR}/dist/wheelhouse/
popd
}

make_cmdbpeewee() {
mkdir -p ${TMP_DIR}/build_cmdbpeewee
pushd ${SRC_DIR}
tar cO cmdbpeewee | tar -xf - -C ${TMP_DIR}/build_cmdbpeewee
popd
pushd ${TMP_DIR}/build_cmdbpeewee
$virtualenv_cmd cmdbpeewee
cd cmdbpeewee
bin/python setup.py bdist_wheel
cp dist/*.whl ${TMP_DIR}/dist/wheelhouse/
popd
}

make_cellcfg() {
mkdir -p ${TMP_DIR}/build_cellcfg
pushd ${SRC_DIR}
tar cO cellcfg | tar -xf - -C ${TMP_DIR}/build_cellcfg
popd
pushd ${TMP_DIR}/build_cellcfg
$virtualenv_cmd cellcfg
cd cellcfg
bin/python setup.py bdist_wheel
cp dist/*.whl ${TMP_DIR}/dist/wheelhouse/
popd
}

install_tmp_pip() {
  mkdir -p ${TMP_DIR}/pip/.local/lib/python${python_ver}/site-packages
  pushd ${TMP_DIR}/pip
  curl -O https://bootstrap.pypa.io/get-pip.py
  env PYTHONUSERBASE=${TMP_DIR}/pip/.local python get-pip.py --user
  popd
  pip_cmd="env PYTHONUSERBASE=${TMP_DIR}/pip/.local ${TMP_DIR}/pip/.local/bin/pip"
  $pip_cmd install virtualenv --user
  virtualenv_cmd="env PYTHONUSERBASE=${TMP_DIR}/pip/.local ${TMP_DIR}/pip/.local/bin/virtualenv"
  $pip_cmd install wheel --user
}

make_private_pkgs() {
install_tmp_pip
make_pycmdb
make_pycss
make_cmdbpeewee
make_cellcfg
}

fetch_pypi() {
mkdir -p ${TMP_DIR}/build_pypi
pushd ${TMP_DIR}/build_pypi
$pip_cmd download -r ${CELLCFG_DIR}/files/requirements.txt
mv *.whl ${TMP_DIR}/dist/wheelhouse/
$pip_cmd wheel -w ${TMP_DIR}/dist/wheelhouse/ *
popd
}

fetch_virtualenv() {
mkdir -p ${TMP_DIR}/dist/virtualenv/
pushd ${TMP_DIR}/dist/virtualenv
$pip_cmd download setuptools pip
$pip_cmd download virtualenv
popd
}

make_pkg() {
cp ${CELLCFG_DIR}/files/setup.sh ${TMP_DIR}/dist/
cp -rp ${CELLCFG_DIR}/files/doc/ ${TMP_DIR}/dist/
pushd ${TMP_DIR}/dist/doc/
if [ -f SERVICE.${pkg_name0} ]; then
mv SERVICE.${pkg_name0} SERVICE
rm SERVICE.*
fi
popd
cp -rp ${CELLCFG_DIR}/files/conf/ ${TMP_DIR}/dist/
mkdir -p ${CELLCFG_DIR}/dist/
pushd ${TMP_DIR}
tar czf ${CELLCFG_DIR}/dist/paxos-cellcfg.${pkg_name}.tar.gz dist/
popd
}

init
check_env
make_core_pkgs
make_private_pkgs
fetch_pypi
fetch_virtualenv
make_pkg
