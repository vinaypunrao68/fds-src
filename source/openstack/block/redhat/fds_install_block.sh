#!/bin/bash
echo "yum -y install install python-pip thrift python-cinderclient nbd \
      python-devel.x86_x64 psutil"
yum -y install install python-pip thrift python-cinderclient nbd \
       python-devel.x86_x64

echo pip install psutil
pip install psutil

echo cp ./nbd.ko /lib/modules/$(uname -r)/kernel/drivers/block/
cp ./nbd.ko /lib/modules/$(uname -r)/kernel/drivers/block/

modprobe nbd
nbd_found=`lsmod | grep -c nbd`
if [ $nbd_found -ne 1 ]; then
    echo "Failed to modprobe nbd"
    exit 1
fi

CINDER_DRIVER_DIR=/usr/lib/python2.6/site-packages/cinder/volume/drivers
DRIVER=./fds_driver.tar.gz
DRIVER_DIR=./fds
BIN_DIR=/usr/sbin

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
cp $DRIVER_DIR/nbdadm.py $BIN_DIR/


mkdir -p $CINDER_DRIVER_DIR
cp -rf $DRIVER_DIR $CINDER_DRIVER_DIR

VOLUME_FILTERS=/usr/share/cinder/rootwrap/volume.filters
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
    echo "restarting cinder"
    echo "Restart manually"
    # service openstack-cinder-volume restart
    exit 0
else
    ## no fds found 
    cat $CINDER_CFG | \
        sed 's/\(enabled_backends=\)/enabled_backends=fds,/' > /tmp/cinder.conf
    cat $CINDER_FDS_CFG >> /tmp/cinder.conf

    cp -f /tmp/cinder.conf /etc/cinder/cinder.conf
fi

service openstack-cinder-volume restart
source openrc.sh

cinder --os-username admin --os-tenant-name admin type-create fds
cinder --os-username admin --os-tenant-name admin type-key fds set volume_backend_name=FDS
cinder --os-username admin --os-tenant-name admin extra-specs-list
