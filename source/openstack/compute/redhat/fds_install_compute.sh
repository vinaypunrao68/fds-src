#!/bin/bash
if [ $# -eq 1 ]; then
    PIP_PROXY="--proxy=$1"
else
    PIP_PROXY=""
fi
yum -y install nbd gcc python-pip python-devel.x86_64
pip $PIP_PROXY install psutil

cp ./nbd.ko /lib/modules/$(uname -r)/kernel/drivers/block/

BIN_DIR=/usr/local/bin
DRIVER=./fds_driver.tar.gz
DRIVER_DIR=./fds
INITRC=/etc/init.d/
FDS_NBD_SVC=openstack-fds-nbdadmd

tar xzvf $DRIVER

if [ ! -e $DRIVER_DIR/nbdadm.py ]; then
    echo "FAIL: nbdadm.py not exist"
    exit 1
fi
echo cp $DRIVER_DIR/nbdadm.py $BIN_DIR/
cp $DRIVER_DIR/nbdadm.py $BIN_DIR/

if [ ! -e $DRIVER_DIR/nbdadmd.py ]; then
    echo "FAIL: nbdadmd.py not exist"
    exit 1
fi
echo cp $DRIVER_DIR/nbdadmd.py $BIN_DIR/
cp $DRIVER_DIR/nbdadmd.py $BIN_DIR/

if [ ! -e $DRIVER_DIR/openstack-fds-nbdadmd ]; then
    echo "FAIL: openstack-fds-nbdadmd not exist"
    exit 1
fi

echo cp $DRIVER_DIR/$FDS_NBD_SVC $INITRC/
cp $DRIVER_DIR/$FDS_NBD_SVC $INITRC/

echo service $FDS_NBD_SVC start
service $FDS_NBD_SVC start

echo chkconfig $FDS_NBD_SVC on
chkconfig $FDS_NBD_SVC on

depmod
modprobe nbd
lsmod | grep nbd
