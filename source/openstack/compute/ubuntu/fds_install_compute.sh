#!/bin/bash
if [ $# -eq 1 ]; then
    PIP_PROXY="--proxy=$1"
else
    PIP_PROXY=""
fi
echo apt-get -y install nbd-client gcc python-pip python-dev
apt-get -y install nbd-client gcc python-pip python-dev

echo pip $PIP_PROXY install psutil
pip $PIP_PROXY install psutil

#cp ./nbd.ko /lib/modules/$(uname -r)/kernel/drivers/block/

BIN_DIR=/usr/sbin
DRIVER=./fds_driver.tar.gz
DRIVER_DIR=./fds
INITRC=/etc/init/
FDS_NBD_SVC=nova-compute-fds-nbdadmd
FDS_NBD_SVC_CFG=$FDS_NBD_SVC.conf

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

if [ ! -e $DRIVER_DIR/$FDS_NBD_SVC_CFG ]; then
    echo "FAIL: $FDS_NBD_SVC_CFG not exist"
    exit 1
fi

echo cp $DRIVER_DIR/$FDS_NBD_SVC_CFG $INITRC/
cp $DRIVER_DIR/$FDS_NBD_SVC_CFG $INITRC/

echo service $FDS_NBD_SVC start
service $FDS_NBD_SVC start

depmod
modprobe nbd
lsmod | grep nbd
