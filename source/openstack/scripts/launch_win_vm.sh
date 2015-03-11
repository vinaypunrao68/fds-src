#!/bin/bash
BIN_PATH=~/bin
source $BIN_PATH/fds_params.sh

for i in `seq 1 $VM_COUNT`; do
    comp=$(($i % 2))
    echo $comp
    if [ $comp -eq 1 ]; then
        comp_node=${COMPUTE_NODE}1
    else
        comp_node=${COMPUTE_NODE}2
    fi
    # echo $comp_node
    echo nova boot --image $IMAGE_ID --flavor $VM_FLAVOR --key-name $VM_KEY --availability-zone $ZONE:$comp_node ${VM_NAME}-${i}
    nova boot --image $IMAGE_ID --flavor $VM_FLAVOR --key-name $VM_KEY --availability-zone $ZONE:$comp_node ${VM_NAME}-${i}
done
