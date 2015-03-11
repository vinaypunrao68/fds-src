#!/bin/bash
BIN_PATH=~/bin
source $BIN_PATH/fds_params.sh

for i in `seq 1 $VM_COUNT`; do
    echo nova delete ${VM_NAME}-${i}
    nova delete ${VM_NAME}-${i}
done
