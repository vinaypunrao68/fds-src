#!/bin/bash

WORKSPACE=$1
TEST_NODE=$2
TAG=$3
TEST_JSON=$4
OM_NODE=$5
CLUSTER=$6


test_type=`echo $TAG | awk -F ":" '{print $1}'`
mix=`echo $TAG | awk -F ":" '{print $2}'`
config=`echo $TAG | awk -F ":" '{print $3}'`

echo "test_type: $test_type, mix: $mix, config: $config" 

# remove package
hosts=`echo $CLUSTER | sed 's/,/ /g'`
for node in $hosts ; do
    ssh $node 'apt-get remove -y fds-platform'
done

# deploy inventory file/confs
./deploy_confs.sh $config $OM_NODE $CLUSTER

# kill FDS if up
./is_fds_up.py -K --fds-nodes $CLUSTER

TEST_DIR=/regress/test-$RANDOM

# run tests
if [ "$TEST_NODE" = "local" ] ; then
    ./run_experiment.py -m $OM_NODE -J $TEST_JSON -d $TEST_DIR -c 1 -j -D $WORKSPACE --fds-nodes $CLUSTER
else
    ./run_experiment.py -m $OM_NODE -J $TEST_JSON -d $TEST_DIR -c 1 -j -t $TEST_NODE -D $WORKSPACE --fds-nodes $CLUSTER
fi

# stats processing
if [ "$test_type" = "fio" ] ; then
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG fio > $TEST_DIR/results.csv
elif [ "$test_type" = "s3_java" ] ; then
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG s3_java > $TEST_DIR/results.csv
else
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG > $TEST_DIR/results.csv
fi
