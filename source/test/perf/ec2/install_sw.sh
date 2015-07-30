#!/bin/bash

node=$1

SSH="sshpass -p passwd ssh -o StrictHostKeyChecking=no root@$node"
$SSH DEBIAN_FRONTEND=noninteractive apt-get install -y python-dev nbd-client fio python-pip
$SSH pip install influxdb==0.1.12
$SSH pip install psutil
$SSH ethtool -K eth0 tso off
$SSH ethtool -k eth0|grep tcp-segmentation-offload

