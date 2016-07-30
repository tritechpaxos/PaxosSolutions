#!/bin/bash

arc=`pwd`/tools.tar.gz
cmd_dir=`pwd`/../../cmd_admin/
work=`mktemp -d`
trap "rm -r ${work}" EXIT

. version.sh

pushd ${work}

mkdir pkg0 wheelhouse
pushd pkg0
wget https://pypi.python.org/packages/source/s/setuptools/setuptools-${SETUPTOOLS_VERSION}.tar.gz
wget https://pypi.python.org/packages/source/p/pip/pip-${PIP_VERSION}.tar.gz
wget https://pypi.python.org/packages/source/v/virtualenv/virtualenv-${VIRTUALENV_VERSION}.tar.gz
popd

tar xzf pkg0/setuptools-${SETUPTOOLS_VERSION}.tar.gz
pushd setuptools-${SETUPTOOLS_VERSION}
python setup.py install --user
popd

tar xzf pkg0/pip-${PIP_VERSION}.tar.gz
pushd pip-${PIP_VERSION}
python setup.py install --user
popd

~/.local/bin/pip install wheel --user
~/.local/bin/pip wheel PyYAML paramiko ecdsa Fabric psutil pycrypto
mv *.whl wheelhouse

pushd ${cmd_dir}
rm dist/*.whl
python setup.py compile_catalog
python setup.py bdist_wheel
cp dist/* ${work}/wheelhouse
popd

tar czf ${arc} pkg0 wheelhouse

popd
