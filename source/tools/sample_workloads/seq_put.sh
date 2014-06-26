#!/bin/bash

#
# If instead of Pharos generator, you want to run dd or iometer, pass some random
# values for all input params except DEVICE. Then, the parent script will use
# .stat files created by SH and create graphs for the experiment.
# 

RESULT_DIR=$1 # directory where the script will output results
PREFIX=$2     # prefix string for the result files
VOLUME=$3     # S3 volume

FNAME=$(basename $0)
NAME=${FNAME%.*}
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo $SCRIPTDIR

# For now we'll set it here, since AB is not in the same tree as fds-src
AB_DIR=/root/ab

runtime=100000       # number of IOs generator will produce ( positive number means length of experiment in seconds) 
blocksize=1048576    # in bytes
outstand_count=40    # number of outstanding IOs from workload generator 
access=s             # 'r' is random and 's' is sequential

tracename=$RESULT_DIR/$PREFIX"__"$NAME"_vol.stat" # note .stat extension otherwise plotting scrip will ignore it
echo $tracename

if [ -z $VOLUME ]; then
  echo "Must specify S3 Volume "
  echo "Usage: $0 <output_dir> <prefix> <volume name>"
  exit 1
fi

mkdir -p $RESULT_DIR

pushd .; cd $AB_DIR
#./Pharos -k $blocksize -o $outstand_count -d w -t $tracename $DEVICE $access $runtime $PREFIX
echo ab -E -n $runtime -c $outstand_count -p 5MB_rand.0 -b $blocksize -R ab_blobs.txt http://localhost:8000/$VOLUME/
#ab -E -n $runtime -c $outstand_count -p 5MB_rand.0 -b $blocksize -R ab_blobs.txt http://localhost:8000/$VOLUME/ > $tracename
./runme.sh -E -n $runtime -c $outstand_count -p 5MB_rand.0 -b $blocksize -R ab_blobs.txt http://localhost:8000/$VOLUME/
popd
