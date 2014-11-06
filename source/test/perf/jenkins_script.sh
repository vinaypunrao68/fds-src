#!/bin/bash -l

EMAIL_RECIPIENTS=$1
WORKSPACE=$2

cd $WORKSPACE

# need to do this for release

# build
make devsetup

jenkins_scripts/build-base-deps.py
jenkins_scripts/build-fds.py

# run test

TEST_IP="10.1.10.139"
PYRO_IP="10.1.10.139"
PYRO_PORT=47672

cd $WORKSPACE/source/test/perf

TEST_DIR=/regress/test-$RANDOM
cp $WORKSPACE/source/test/perf/regress/config/platform.conf.amcache $WORKSPACE/source/config/etc/platform.conf
./run_experiment.py -n $PYRO_IP -p $PYRO_PORT -m $HOSTNAME -M local-single -J regress/config/get.json -d $TEST_DIR -c 1 -j -t $TEST_IP -D $WORKSPACE --fds-nodes $HOSTNAME
./do_stats.sh $TEST_DIR $HOSTNAME test.db $HOSTNAME s3:get:amcache > results-0.csv

TEST_DIR=/regress/test-$RANDOM
cp $WORKSPACE/source/test/perf/regress/config/platform.conf.amcache0 $WORKSPACE/source/config/etc/platform.conf
./run_experiment.py -n $PYRO_IP -p $PYRO_PORT -m $HOSTNAME -M local-single -J regress/config/get.json -d $TEST_DIR -c 1 -j -t $TEST_IP -D $WORKSPACE --fds-nodes $HOSTNAME
./do_stats.sh $TEST_DIR $HOSTNAME test.db $HOSTNAME s3:get:amcache0 > results-1.csv

./present_results.py test.db s3:get:amcache,s3:get:amcache0 $EMAIL_RECIPIENTS

echo "Performance results" | mail -s "Performance regression" -a results-0.csv -a results-1.csv matteo@formationds.com
