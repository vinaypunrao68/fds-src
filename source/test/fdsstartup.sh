#!/bin/bash
cd Build/linux-x86_64.debug/bin
if [ $? -eq 1 ]; then
    fds_root=$1
else
    fds_root=/fds
fi

echo "Starting orchMgr"
./orchMgr --fds-root $fds_root > ./orchMgr.out &
sleep 2

echo "Starting platformd"
./platformd --fds-root $fds_root > ./platformd.out &

sleep 5

echo "Asking OM to accept all discovered nodes..."
./fdscli --fds-root $fds_root --activate-nodes abc -k 1
sleep 2
