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

echo "Cleaning up..."
# kill FDS if up
echo "Killing FDS on $CLUSTER"
./is_fds_up.py -K --fds-nodes $CLUSTER

# remove package
hosts=`echo $CLUSTER | sed 's/,/ /g'`
for node in $hosts ; do
    echo "$node -> stopping redis"
    scp $WORKSPACE/source/tools/redis.sh $node:/tmp/
    ssh $node 'bash /tmp/redis.sh clean'
    ssh $node 'bash /tmp/redis.sh stop'
    echo "$node -> remove fds-platform"
    ssh $node 'dpkg -P fds-platform'
    echo "$node -> clean up shared memory"
    ssh $node 'rm -f /dev/shm/0x*-0'
    echo "$node -> unmount FDS disks"
    ssh $node 'umount /fds/dev/*'
    echo "$node -> remove /fds"
    ssh $node 'rm -fr /fds'
done

# deploy inventory file/confs
echo "Deploy inventory file - config: $config om_node: $OM_NODE cluster: $CLUSTER"
./deploy_confs.sh $config $OM_NODE $CLUSTER

# bring up fds
echo "Bring up FDS"
pushd $WORKSPACE/ansible
./scripts/deploy_fds.sh $OM_NODE local
echo "Sleep for one minute more..."
sleep 60
popd

echo "Start Testing"
TEST_DIR=/regress/test-$RANDOM

# run tests
if [ "$TEST_NODE" = "local" ] ; then
    ./run_experiment.py -m $OM_NODE -J $TEST_JSON -d $TEST_DIR -c 1 -j -D $WORKSPACE --fds-nodes $CLUSTER -x
else
    ./run_experiment.py -m $OM_NODE -J $TEST_JSON -d $TEST_DIR -c 1 -j -t $TEST_NODE -D $WORKSPACE --fds-nodes $CLUSTER -x
fi

# stats processing
if [ "$test_type" = "fio" ] ; then
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG fio > $TEST_DIR/results.csv
elif [ "$test_type" = "s3_java" ] ; then
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG s3_java > $TEST_DIR/results.csv
else
    ./do_stats.sh $TEST_DIR $OM_NODE test.db $OM_NODE $TAG > $TEST_DIR/results.csv
fi
