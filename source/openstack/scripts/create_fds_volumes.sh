#!/bin/bash
BIN_PATH=~/bin
source $BIN_PATH/fds_params.sh

for i in `seq 1 $FDS_NUM_VOLS`; do
    echo cinder create --display-name ${FDS_VOL_NAME}-${i} --volume-type $FDS_VOL_TYPE --display-description $FDS_VOL_DESC $FDS_VOL_SIZE
    cinder create --display-name ${FDS_VOL_NAME}-${i} --volume-type $FDS_VOL_TYPE --display-description $FDS_VOL_DESC $FDS_VOL_SIZE
done
