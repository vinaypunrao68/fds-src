#!/bin/bash
pip uninstall -y psutil
yum -y remove nbd gcc python-pip python-devel.x86_64

rmmod nbd && rm /lib/modules/$(uname -r)/kernel/drivers/block/nbd.ko
