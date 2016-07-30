#!/bin/bash
 
work_dir=`mktemp -d`
trap "rm -r ${work_dir}" EXIT

. version.sh
 
tar -xzf tools.tar.gz -C ${work_dir}
 
pushd ${work_dir}
 
tar xzf pkg0/setuptools-${SETUPTOOLS_VERSION}.tar.gz
pushd setuptools-${SETUPTOOLS_VERSION}
python setup.py install --user
popd
 
tar xzf pkg0/pip-${PIP_VERSION}.tar.gz
pushd pip-${PIP_VERSION}
python setup.py install --user
popd
 
~/.local/bin/pip install --user pkg0/virtualenv-${VIRTUALENV_VERSION}.tar.gz
 
~/.local/bin/virtualenv ~/
 
~/bin/pip install -U wheelhouse/*

rm ~/pip-selfcheck.json
