#!/bin/bash

# this is running from the spoon container
WORKSPACE=$1
machines_file=$2
# get list of hosts
machines=""
for e in `cat $machines_file`; do 
    machines="$machines $e"
done
marray=($(echo ${machines}))
echo "machines: $machines"
fio_machine=${marray[0]}
echo "fio_machine: $fio_machine"

if [ ! -e /regress ] ; then
  mkdir /regress
fi
echo "workspace: $WORKSPACE"

TEST_DIR=/regress/test-$RANDOM

echo "test_dir: $TEST_DIR"

mkdir $TEST_DIR

pushd $WORKSPACE/source/test/perf/dell_test

./run_dell_test.sh $TEST_DIR "$machines" 1 "$fio_machine" 1g "perf_ec2"

popd
