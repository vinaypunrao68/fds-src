#!/bin/bash

node=$1

SSH="sshpass -p passwd ssh -o StrictHostKeyChecking=no -y root@$node"
$SSH DEBIAN_FRONTEND=noninteractive apt-get install -y python-dev nbd-client fio python-pip
$SSH pip install psutil

