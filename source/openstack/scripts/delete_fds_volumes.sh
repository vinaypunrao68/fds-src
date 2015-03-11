#!/bin/bash
BIN_PATH=~/bin
source $BIN_PATH/fds_params.sh

for i in `seq 1 $FDS_NUM_VOLS`; do
    echo cinder delete ${FDS_VOL_NAME}-${i}
    cinder delete ${FDS_VOL_NAME}-${i}
done
