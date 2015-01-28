#!/bin/bash
BIN_PATH=~/bin
source $BIN_PATH/openrc.sh

# ${VM_NAME}-${i}
VM_NAME=win12
VM_FLAVOR=m1.large
VM_KEY=mykey
VM_COUNT=4
ZONE=nova
#compute1 compute2
COMPUTE_NODE=compute


IMAGE_ID=`glance image-list | grep win_server_2012 | awk '{print $2}'`

# ${FDS_VOL_NAME}-$i
FDS_VOL_NAME=fds-vol
FDS_VOL_SIZE=100
FDS_VOL_DESC=fds-test-volume
FDS_VOL_TYPE=fds
FDS_NUM_VOLS=10

if [ $FDS_NUM_VOLS -lt $VM_COUNT ]; then
    echo "VOL: $FDS_NUM_VOLS, Instances: $VM_COUNT"
    echo "ERROR: Number of volumes have to be greater or equals than number of Instance"
    exit 1
fi


# Bring all offline disks online in windows
# Get-Disk | where-object IsOffline -Eq $True | Set-Disk -IsOffline $False

# or use diskpart /s online_disk.dp

function exit_if_error()
{
    if [ $1 -ne 0 ]; then
        echo "ERROR: command failed with error $1"
        exit 1
    fi
}
