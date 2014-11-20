#!/bin/bash

# ./run_exp_regress.sh $WORKSPACE $TEST_IP s3:get:amcache0

WORKSPACE=$1
TEST_IP=$2
TAG=$3
TEST_JSON=$4

PYRO_IP="10.1.10.139"
PYRO_PORT=47672

test_type=`echo $TAG | awk -F ":" '{print $1}'`
mix=`echo $TAG | awk -F ":" '{print $2}'`
config=`echo $TAG | awk -F ":" '{print $3}'`

echo "test_type: $test_type, mix: $mix, config: $config" 

./deploy_confs.sh $config $HOSTNAME
./is_fds_up.py -K
TEST_DIR=/regress/test-$RANDOM
./run_experiment.py -n $PYRO_IP -p $PYRO_PORT -m $HOSTNAME -M local-single -J $TEST_JSON -d $TEST_DIR -c 1 -j -t $TEST_IP -D $WORKSPACE --fds-nodes $HOSTNAME
if [ "$test_type" = "fio" ] ; then
    ./do_stats.sh $TEST_DIR $HOSTNAME test.db $HOSTNAME $TAG fio > $TEST_DIR/results.csv
elif [ "$test_type" = "s3_java" ] ; then
    ./do_stats.sh $TEST_DIR $HOSTNAME test.db $HOSTNAME $TAG s3_java > $TEST_DIR/results.csv
else
    ./do_stats.sh $TEST_DIR $HOSTNAME test.db $HOSTNAME $TAG > $TEST_DIR/results.csv
fi
