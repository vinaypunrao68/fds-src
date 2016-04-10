#!/usr/bin/env bash
# This script is part of devsetup. It installs libnfs and libnfs-python package.
# System test uses libnfs packages to do NFS volume IO.
echo $HOME
declare -r LIBNFS_URL="https://github.com/sahlberg/libnfs.git"
declare -r LIBNFS_PYTHON_URL="https://github.com/sahlberg/libnfs-python.git"
declare -r LIBNFS_DIR=$HOME/libnfs
declare -r LIBNFS_PYTHON_DIR=$HOME/libnfs_python/


if [ -d $HOME/libnfs/ ]; then (cd ${LIBNFS_DIR} && git pull); else mkdir -p ${LIBNFS_DIR} && git clone ${LIBNFS_URL} ${LIBNFS_DIR}/;fi

if [ -d ${LIBNFS_PYTHON_DIR} ]; then (cd ${LIBNFS_PYTHON_DIR} && git pull); else mkdir -p ${LIBNFS_PYTHON_DIR} && git clone ${LIBNFS_PYTHON_URL} ${LIBNFS_PYTHON_DIR}/;fi

cd ${LIBNFS_DIR} && ./bootstrap && ./configure --prefix=/usr && make && sudo make install
cd ${LIBNFS_PYTHON_DIR} && sudo python setup.py install
cd ${LIBNFS_PYTHON_DIR}/libnfs && make clean && make
printf " import libnfs check"
python -c "import libnfs"