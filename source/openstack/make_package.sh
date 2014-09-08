#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: make_package.sh <redhat|ubuntu>"
    exit 1
fi
dist=$1

if [ $dist != "redhat" -a $dist != "ubuntu" ]; then
    echo "Invalid distribution name: $dist"
    echo "Usage: make_package.sh <redhat|ubuntu>"
    exit 2
fi

dest=/tmp/openstack_package
dest_ctrl=$dest/controller
dest_compute=$dest/compute
dest_block=$dest/block
block=block
compute=compute
control=ctrl
cirros_path=http://cdn.download.cirros-cloud.net/0.3.1
cirros_name=cirros-0.3.1-x86_64-disk.img
cirros_name_raw=cirros-0.3.1-x86_64-disk.raw

mkdir -p /tmp/openstack_package

function copy_cinder_dir()
{
    dest=$1
    cd ../cinder
    ../../Build/linux-x86_64.debug/bin/thrift -out . -gen py ../apis/apis.thrift

    mkdir -p fds
    cp -rf * fds/
    tar czvf fds_driver.tar.gz fds/
    mv fds_driver.tar.gz $dest/
    rm -rf fds

    cd -
}

function make_pkg_controller()
{
    echo mkdir -p $dest_ctrl
    mkdir -p $dest_ctrl

    echo cp openrc.sh $dest_ctrl/
    cp openrc.sh $dest_ctrl/

    echo cp openstack_get_config.py $dest_ctrl/
    cp openstack_get_config.py $dest_ctrl/

    echo cp openstack_status.py $dest_ctrl/
    cp openstack_status.py $dest_ctrl/

    echo cp $control/fds_id_rsa $dest_ctrl/
    cp $control/fds_id_rsa $dest_ctrl/

    echo cp $control/fds_id_rsa.pub $dest_ctrl/
    cp $control/fds_id_rsa.pub $dest_ctrl/

    echo cp $control/start_fds_tmux $dest_ctrl/
    cp $control/start_fds_tmux $dest_ctrl/

    echo cp $control/tmux_fds.conf $dest_ctrl/
    cp $control/tmux_fds.conf $dest_ctrl/

    echo cp $control/$dist/fds_install_ctrl.sh $dest_ctrl/
    cp $control/$dist/fds_install_ctrl.sh $dest_ctrl/

    echo cp $control/$dist/fds_uninstall_ctrl.sh $dest_ctrl/
    cp $control/$dist/fds_uninstall_ctrl.sh $dest_ctrl/

    # create and move cirros images
    wget $cirros_path/$cirros_name
    sudo apt-get -y install qemu-utils
    qemu-img convert -f qcow2 -O raw $cirros_name $cirros_name_raw

    if [ ! -e $cirros_name ]; then
        echo "Cirros image not found: $cirros_name"
    fi

    if [ ! -e $cirros_name_raw ]; then
        echo "Cirros image not found: $cirros_name_raw"
    fi

    echo mv $cirros_name_raw $dest_ctrl/
    mv $cirros_name_raw $dest_ctrl/
    echo mv $cirros_name $dest_ctrl/
    mv $cirros_name $dest_ctrl/
}

function make_pkg_compute()
{
    mkdir -p $dest_compute

    echo cp openrc.sh $dest_compute/
    cp openrc.sh $dest_compute/

    echo cp $compute/$dist/fds_install_compute.sh $dest_compute/
    cp $compute/$dist/fds_install_compute.sh $dest_compute/

    echo cp $compute/$dist/fds_uninstall_compute.sh $dest_compute/
    cp $compute/$dist/fds_uninstall_compute.sh $dest_compute/

    echo cp $compute/$dist/* $dest_compute/
    cp $compute/$dist/* $dest_compute/

    copy_cinder_dir $dest_compute
}

function make_pkg_block()
{
    echo mkdir -p $dest_block
    mkdir -p $dest_block

    echo cp openrc.sh $dest_block/
    cp openrc.sh $dest_block/

    echo cp $block/cinder_fds_cfg_append.txt $dest_block/   
    cp $block/cinder_fds_cfg_append.txt $dest_block/   

    echo cp $block/$dist/fds_install_block.sh $dest_block/
    cp $block/$dist/fds_install_block.sh $dest_block/

    echo cp $block/$dist/fds_uninstall_block.sh $dest_block/
    cp $block/$dist/fds_uninstall_block.sh $dest_block/

    echo cp $block/$dist/* $dest_block/
    cp $block/$dist/* $dest_block/

    copy_cinder_dir $dest_block
}

make_pkg_controller
make_pkg_compute
make_pkg_block
