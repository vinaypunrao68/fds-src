#!/bin/bash
echo apt-get -y install install python-pip
apt-get -y install install python-pip

echo apt-get -y install thrift
apt-get -y install thrift

echo apt-get -y install python-cinderclient
apt-get -y install python-cinderclient

echo apt-get -y install nbd-client
apt-get -y install nbd-client

echo apt-get -y install python-pip
apt-get -y install python-pip

echo apt-get -y install gcc python-dev
apt-get -y install gcc python-dev

echo pip install psutil
pip install psutil

echo pip install thrift
pip install thrift

modprobe nbd
nbd_found=`lsmod | grep -c nbd`
if [ $nbd_found -ne 1 ]; then
    echo "Failed to modprobe nbd"
    exit 1
fi

CINDER_DRIVER_DIR=/usr/lib/python2.7/dist-packages/cinder/volume/drivers
DRIVER=./fds_driver.tar.gz
DRIVER_DIR=./fds

# copy thrift binding and fds-cinder driver to cinder driver dir.
if [ ! -d "$CINDER_DRIVER_DIR" ]; then
    echo "Error: cinder volume drivers directory does not exist."
    echo "$CINDER_DRIVER"
    exit 1
fi

tar xzvf $DRIVER

if [ ! -e $DRIVER_DIR/nbdadm.py ]; then
    echo "nbdadm.py not exist"
    exit 1
fi
cp $DRIVER_DIR/nbdadm.py /usr/bin/


mkdir -p $CINDER_DRIVER_DIR
cp -rf $DRIVER_DIR $CINDER_DRIVER_DIR

VOLUME_FILTERS=/etc/cinder/rootwrap.d/volume.filters
if [ -e $VOLUME_FILTERS ]; then
    found=`cat $VOLUME_FILTERS | grep -c "nbdadm.py"`
    if [ $found -eq 0 ]; then
        echo "nbdadm.py: CommandFilter, nbdadm.py, root" >> $VOLUME_FILTERS
    fi
fi

CINDER_CFG=/etc/cinder/cinder.conf
CINDER_FDS_CFG=./cinder_fds_cfg_append.txt

cp $CINDER_CFG ${CINDER_CFG}_`date +%F-%H-%M-%S`
cat $CINDER_CFG | grep "^[^#]*enabled_backends=[^#]*fds"
if [ $? -eq 0 ]; then
    echo "cinder cfg already setup"
    exit 0
else
    ## no fds found 
    cat $CINDER_CFG | \
        sed 's/\(enabled_backends=\)/enabled_backends=fds,/' > /tmp/cinder.conf
    cat $CINDER_FDS_CFG >> /tmp/cinder.conf

    cp -f /tmp/cinder.conf /etc/cinder/cinder.conf
fi

service cinder-volume restart
source openrc.sh

cinder --os-username admin --os-tenant-name admin type-create fds
cinder --os-username admin --os-tenant-name admin type-key fds set volume_backend_name=FDS
cinder --os-username admin --os-tenant-name admin extra-specs-list
