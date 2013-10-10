#!/bin/bash

#
# If instead of Pharos generator, you want to run dd or iometer, pass some random
# values for all input params except DEVICE. Then, the parent script will use
# .stat files created by SH and create graphs for the experiment.
# 

RESULT_DIR=$1 # directory where the script will output results
PREFIX=$2     # prefix string for the result files
DEVICE=$3     # /dev/fbdN
VOLID=$4      # volume id -- just for output

FNAME=$(basename $0)
NAME=${FNAME%.*}
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PHAROS_DIR=$(dirname $SCRIPTDIR)/pharos

runtime=40           # length of experiment in seconds 
blocksize=4          # in KB
outstand_count=20    # number of outstanding IOs from workload generator 

tracename=$RESULT_DIR/$PREFIX"__"$NAME"_vol"$VOLID".stat" # note .stat extension otherwise plotting scrip will ignore it

if [ -z $DEVICE ]; then
  echo "Must specify /dev/fbdN device "
  echo "Usage: $0 output_dir prefix /dev/fbdN volume_id"
  exit 1
fi

if [ ! -e $DEVICE ]; then
  echo "Device $DEVICE does not exist"
#  exit 1
fi

if [ -z $VOLID ]; then
   echo "Must specify volume id"
   echo "Usage: $0 output_dir prefix /dev/fbdN volume_id"
fi

mkdir -p $RESULT_DIR

pushd .; cd $PHAROS_DIR
./Pharos -k $blocksize -q $outstand_count -d w -t $tracename $DEVICE r $runtime $VOLID $PREFIX
popd



